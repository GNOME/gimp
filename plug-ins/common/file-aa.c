/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * aa.c version 1.0
 * A plugin that uses libaa (ftp://ftp.ta.jcu.cz/pub/aa) to save images as
 * ASCII.
 * NOTE: This plugin *requires* aalib 1.2 or later. Earlier versions will
 * not work.
 * Code copied from all over the GIMP source.
 * Tim Newsome <nuisance@cmu.edu>
 */

#include "config.h"

#include <string.h>

#include <aalib.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define SAVE_PROC      "file-aa-save"
#define PLUG_IN_BINARY "file-aa"


typedef struct _Ascii      Ascii;
typedef struct _AsciiClass AsciiClass;

struct _Ascii
{
  GimpPlugIn      parent_instance;
};

struct _AsciiClass
{
  GimpPlugInClass parent_class;
};


#define ASCII_TYPE  (ascii_get_type ())
#define ASCII (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), ASCII_TYPE, Ascii))

GType                   ascii_get_type         (void) G_GNUC_CONST;

static GList          * ascii_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * ascii_create_procedure (GimpPlugIn           *plug_in,
                                                const gchar          *name);

static GimpValueArray * ascii_save             (GimpProcedure        *procedure,
                                                GimpRunMode           run_mode,
                                                GimpImage            *image,
                                                gint                  n_drawables,
                                                GimpDrawable        **drawables,
                                                GFile                *file,
                                                const GimpValueArray *args,
                                                gpointer              run_data);

static gboolean         save_aa                (GFile                *file,
                                                GimpDrawable         *drawable,
                                                GObject              *config,
                                                GError              **error);
static void             gimp2aa                (GimpDrawable         *drawable,
                                                aa_context           *context);

static gboolean         save_dialog            (GimpProcedure        *procedure,
                                                GObject              *config);


G_DEFINE_TYPE (Ascii, ascii, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (ASCII_TYPE)


static void
ascii_class_init (AsciiClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = ascii_query_procedures;
  plug_in_class->create_procedure = ascii_create_procedure;
}

static void
ascii_init (Ascii *ascii)
{
}

static GList *
ascii_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (SAVE_PROC));
}

static GimpProcedure *
ascii_create_procedure (GimpPlugIn  *plug_in,
                        const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, SAVE_PROC))
    {
      gint i;

      procedure = gimp_save_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           ascii_save, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, N_("ASCII art"));

      gimp_procedure_set_documentation (procedure,
                                        "Saves grayscale image in various "
                                        "text formats",
                                        "This plug-in uses aalib to save "
                                        "grayscale image as ascii art into "
                                        "a variety of text formats",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Tim Newsome <nuisance@cmu.edu>",
                                      "Tim Newsome <nuisance@cmu.edu>",
                                      "1997");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "text/plain");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "txt,ansi,text");

      for (i = 0; aa_formats[i]; i++);

      GIMP_PROC_ARG_INT (procedure, "file-type",
                         "File type",
                         "File type to use",
                         0, i, 0,
                         G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
ascii_save (GimpProcedure        *procedure,
            GimpRunMode           run_mode,
            GimpImage            *image,
            gint                  n_drawables,
            GimpDrawable        **drawables,
            GFile                *file,
            const GimpValueArray *args,
            gpointer              run_data)
{
  GimpProcedureConfig *config;
  GimpPDBStatusType    status = GIMP_PDB_SUCCESS;
  GimpExportReturn     export = GIMP_EXPORT_CANCEL;
  GError              *error  = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  config = gimp_procedure_create_config (procedure);
  gimp_procedure_config_begin_run (config, image, run_mode, args);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_ui_init (PLUG_IN_BINARY);

      export = gimp_export_image (&image, &n_drawables, &drawables, "AA",
                                  GIMP_EXPORT_CAN_HANDLE_RGB     |
                                  GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                  GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                  GIMP_EXPORT_CAN_HANDLE_ALPHA);

      if (export == GIMP_EXPORT_CANCEL)
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CANCEL,
                                                 NULL);
      break;

    default:
      break;
    }

  if (n_drawables != 1)
    {
      g_set_error (&error, G_FILE_ERROR, 0,
                   _("ASCII art does not support multiple layers."));

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               error);
    }

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      if (! save_dialog (procedure, G_OBJECT (config)))
        status = GIMP_PDB_CANCEL;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (! save_aa (file, drawables[0], G_OBJECT (config), &error))
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  gimp_procedure_config_end_run (config, status);
  g_object_unref (config);

  if (export == GIMP_EXPORT_EXPORT)
    {
      gimp_image_delete (image);
      g_free (drawables);
    }

  return gimp_procedure_new_return_values (procedure, status, error);
}

static gboolean
save_aa (GFile         *file,
         GimpDrawable  *drawable,
         GObject       *config,
         GError       **error)
{
  aa_savedata  savedata;
  aa_context  *context;
  aa_format    format;
  gint         output_type;

  g_object_get (config,
                "file-type", &output_type,
                NULL);

  memcpy (&format, aa_formats[output_type], sizeof (aa_format));

  format.width  = gimp_drawable_width  (drawable) / 2;
  format.height = gimp_drawable_height (drawable) / 2;

  /* Get a libaa context which will save its output to filename. */
  savedata.name   = g_file_get_path (file);
  savedata.format = &format;

  context = aa_init (&save_d, &aa_defparams, &savedata);
  if (! context)
    return FALSE;

  gimp2aa (drawable, context);

  aa_flush (context);
  aa_close (context);

  return TRUE;
}

static void
gimp2aa (GimpDrawable *drawable,
         aa_context   *context)
{
  GeglBuffer      *buffer;
  const Babl      *format;
  aa_renderparams *renderparams;
  gint             width;
  gint             height;
  gint             x, y;
  gint             bpp;
  guchar          *buf;
  guchar          *p;

  buffer = gimp_drawable_get_buffer (drawable);

  width  = aa_imgwidth  (context);
  height = aa_imgheight (context);

  switch (gimp_drawable_type (drawable))
    {
    case GIMP_GRAY_IMAGE:
      format = babl_format ("Y' u8");
      break;

    case GIMP_GRAYA_IMAGE:
      format = babl_format ("Y'A u8");
      break;

    case GIMP_RGB_IMAGE:
    case GIMP_INDEXED_IMAGE:
      format = babl_format ("R'G'B' u8");
      break;

    case GIMP_RGBA_IMAGE:
    case GIMP_INDEXEDA_IMAGE:
      format = babl_format ("R'G'B'A u8");
      break;

    default:
      g_return_if_reached ();
      break;
    }

  bpp = babl_format_get_bytes_per_pixel (format);

  buf = g_new (guchar, width * bpp);

  for (y = 0; y < height; y++)
    {
      gegl_buffer_get (buffer, GEGL_RECTANGLE (0, y, width, 1), 1.0,
                       format, buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      switch (bpp)
        {
        case 1:  /* GRAY */
          for (x = 0, p = buf; x < width; x++, p++)
            aa_putpixel (context, x, y, *p);
          break;

        case 2:  /* GRAYA, blend over black */
          for (x = 0, p = buf; x < width; x++, p += 2)
            aa_putpixel (context, x, y, (p[0] * (p[1] + 1)) >> 8);
          break;

        case 3:  /* RGB */
          for (x = 0, p = buf; x < width; x++, p += 3)
            aa_putpixel (context, x, y,
                         GIMP_RGB_LUMINANCE (p[0], p[1], p[2]) + 0.5);
          break;

        case 4:  /* RGBA, blend over black */
          for (x = 0, p = buf; x < width; x++, p += 4)
            aa_putpixel (context, x, y,
                         ((guchar) (GIMP_RGB_LUMINANCE (p[0], p[1], p[2]) + 0.5)
                          * (p[3] + 1)) >> 8);
          break;

        default:
          g_assert_not_reached ();
          break;
        }
    }

  g_free (buf);

  g_object_unref (buffer);

  renderparams = aa_getrenderparams ();
  renderparams->dither = AA_FLOYD_S;

  aa_render (context, renderparams, 0, 0,
             aa_scrwidth (context), aa_scrheight (context));
}

static gboolean
save_dialog (GimpProcedure *procedure,
             GObject       *config)
{
  GtkWidget    *dialog;
  GtkWidget    *hbox;
  GtkWidget    *label;
  GtkListStore *store;
  GtkWidget    *combo;
  gint          i;
  gboolean      run;

  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Export Image as Text"));

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (dialog)),
                      hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("_Format:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  store = g_object_new (GIMP_TYPE_INT_STORE, NULL);

  for (i = 0; aa_formats[i]; i++)
    gtk_list_store_insert_with_values (store, NULL, -1,
                                       GIMP_INT_STORE_VALUE, i,
                                       GIMP_INT_STORE_LABEL, aa_formats[i]->formatname,
                                       -1);

  combo = gimp_prop_int_combo_box_new (config, "file-type",
                                       GIMP_INT_STORE (store));
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);

  gtk_widget_show (dialog);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}
