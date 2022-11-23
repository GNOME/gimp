/* LIGMA - The GNU Image Manipulation Program
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
 * Code copied from all over the LIGMA source.
 * Tim Newsome <nuisance@cmu.edu>
 */

#include "config.h"

#include <string.h>

#include <aalib.h>

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "libligma/stdplugins-intl.h"


#define SAVE_PROC      "file-aa-save"
#define PLUG_IN_BINARY "file-aa"


typedef struct _Ascii      Ascii;
typedef struct _AsciiClass AsciiClass;

struct _Ascii
{
  LigmaPlugIn      parent_instance;
};

struct _AsciiClass
{
  LigmaPlugInClass parent_class;
};


#define ASCII_TYPE  (ascii_get_type ())
#define ASCII (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), ASCII_TYPE, Ascii))

GType                   ascii_get_type         (void) G_GNUC_CONST;

static GList          * ascii_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * ascii_create_procedure (LigmaPlugIn           *plug_in,
                                                const gchar          *name);

static LigmaValueArray * ascii_save             (LigmaProcedure        *procedure,
                                                LigmaRunMode           run_mode,
                                                LigmaImage            *image,
                                                gint                  n_drawables,
                                                LigmaDrawable        **drawables,
                                                GFile                *file,
                                                const LigmaValueArray *args,
                                                gpointer              run_data);

static gboolean         save_aa                (GFile                *file,
                                                LigmaDrawable         *drawable,
                                                GObject              *config,
                                                GError              **error);
static void             ligma2aa                (LigmaDrawable         *drawable,
                                                aa_context           *context);

static gboolean         save_dialog            (LigmaProcedure        *procedure,
                                                GObject              *config);


G_DEFINE_TYPE (Ascii, ascii, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (ASCII_TYPE)
DEFINE_STD_SET_I18N


static void
ascii_class_init (AsciiClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = ascii_query_procedures;
  plug_in_class->create_procedure = ascii_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
ascii_init (Ascii *ascii)
{
}

static GList *
ascii_query_procedures (LigmaPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (SAVE_PROC));
}

static LigmaProcedure *
ascii_create_procedure (LigmaPlugIn  *plug_in,
                        const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, SAVE_PROC))
    {
      gint i;

      procedure = ligma_save_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           ascii_save, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "*");

      ligma_procedure_set_menu_label (procedure, _("ASCII art"));

      ligma_procedure_set_documentation (procedure,
                                        "Saves grayscale image in various "
                                        "text formats",
                                        "This plug-in uses aalib to save "
                                        "grayscale image as ascii art into "
                                        "a variety of text formats",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Tim Newsome <nuisance@cmu.edu>",
                                      "Tim Newsome <nuisance@cmu.edu>",
                                      "1997");

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "text/plain");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "txt,ansi,text");

      for (i = 0; aa_formats[i]; i++);

      LIGMA_PROC_ARG_INT (procedure, "file-type",
                         "File type",
                         "File type to use",
                         0, i, 0,
                         G_PARAM_READWRITE);
    }

  return procedure;
}

static LigmaValueArray *
ascii_save (LigmaProcedure        *procedure,
            LigmaRunMode           run_mode,
            LigmaImage            *image,
            gint                  n_drawables,
            LigmaDrawable        **drawables,
            GFile                *file,
            const LigmaValueArray *args,
            gpointer              run_data)
{
  LigmaProcedureConfig *config;
  LigmaPDBStatusType    status = LIGMA_PDB_SUCCESS;
  LigmaExportReturn     export = LIGMA_EXPORT_CANCEL;
  GError              *error  = NULL;

  gegl_init (NULL, NULL);

  config = ligma_procedure_create_config (procedure);
  ligma_procedure_config_begin_run (config, image, run_mode, args);

  switch (run_mode)
    {
    case LIGMA_RUN_INTERACTIVE:
    case LIGMA_RUN_WITH_LAST_VALS:
      ligma_ui_init (PLUG_IN_BINARY);

      export = ligma_export_image (&image, &n_drawables, &drawables, "AA",
                                  LIGMA_EXPORT_CAN_HANDLE_RGB     |
                                  LIGMA_EXPORT_CAN_HANDLE_GRAY    |
                                  LIGMA_EXPORT_CAN_HANDLE_INDEXED |
                                  LIGMA_EXPORT_CAN_HANDLE_ALPHA);

      if (export == LIGMA_EXPORT_CANCEL)
        return ligma_procedure_new_return_values (procedure,
                                                 LIGMA_PDB_CANCEL,
                                                 NULL);
      break;

    default:
      break;
    }

  if (n_drawables != 1)
    {
      g_set_error (&error, G_FILE_ERROR, 0,
                   _("ASCII art does not support multiple layers."));

      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_CALLING_ERROR,
                                               error);
    }

  if (run_mode == LIGMA_RUN_INTERACTIVE)
    {
      if (! save_dialog (procedure, G_OBJECT (config)))
        status = LIGMA_PDB_CANCEL;
    }

  if (status == LIGMA_PDB_SUCCESS)
    {
      if (! save_aa (file, drawables[0], G_OBJECT (config), &error))
        {
          status = LIGMA_PDB_EXECUTION_ERROR;
        }
    }

  ligma_procedure_config_end_run (config, status);
  g_object_unref (config);

  if (export == LIGMA_EXPORT_EXPORT)
    {
      ligma_image_delete (image);
      g_free (drawables);
    }

  return ligma_procedure_new_return_values (procedure, status, error);
}

static gboolean
save_aa (GFile         *file,
         LigmaDrawable  *drawable,
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

  format.width  = ligma_drawable_get_width  (drawable) / 2;
  format.height = ligma_drawable_get_height (drawable) / 2;

  /* Get a libaa context which will save its output to filename. */
  savedata.name   = g_file_get_path (file);
  savedata.format = &format;

  context = aa_init (&save_d, &aa_defparams, &savedata);
  if (! context)
    return FALSE;

  ligma2aa (drawable, context);

  aa_flush (context);
  aa_close (context);

  return TRUE;
}

static void
ligma2aa (LigmaDrawable *drawable,
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

  buffer = ligma_drawable_get_buffer (drawable);

  width  = aa_imgwidth  (context);
  height = aa_imgheight (context);

  switch (ligma_drawable_type (drawable))
    {
    case LIGMA_GRAY_IMAGE:
      format = babl_format ("Y' u8");
      break;

    case LIGMA_GRAYA_IMAGE:
      format = babl_format ("Y'A u8");
      break;

    case LIGMA_RGB_IMAGE:
    case LIGMA_INDEXED_IMAGE:
      format = babl_format ("R'G'B' u8");
      break;

    case LIGMA_RGBA_IMAGE:
    case LIGMA_INDEXEDA_IMAGE:
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
                         LIGMA_RGB_LUMINANCE (p[0], p[1], p[2]) + 0.5);
          break;

        case 4:  /* RGBA, blend over black */
          for (x = 0, p = buf; x < width; x++, p += 4)
            aa_putpixel (context, x, y,
                         ((guchar) (LIGMA_RGB_LUMINANCE (p[0], p[1], p[2]) + 0.5)
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
save_dialog (LigmaProcedure *procedure,
             GObject       *config)
{
  GtkWidget    *dialog;
  GtkWidget    *hbox;
  GtkWidget    *label;
  GtkListStore *store;
  GtkWidget    *combo;
  gint          i;
  gboolean      run;

  dialog = ligma_procedure_dialog_new (procedure,
                                      LIGMA_PROCEDURE_CONFIG (config),
                                      _("Export Image as Text"));

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_box_pack_start (GTK_BOX (ligma_export_dialog_get_content_area (dialog)),
                      hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("_Format:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  store = g_object_new (LIGMA_TYPE_INT_STORE, NULL);

  for (i = 0; aa_formats[i]; i++)
    gtk_list_store_insert_with_values (store, NULL, -1,
                                       LIGMA_INT_STORE_VALUE, i,
                                       LIGMA_INT_STORE_LABEL, aa_formats[i]->formatname,
                                       -1);

  combo = ligma_prop_int_combo_box_new (config, "file-type",
                                       LIGMA_INT_STORE (store));
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);

  gtk_widget_show (dialog);

  run = ligma_procedure_dialog_run (LIGMA_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}
