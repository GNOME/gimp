/**
 * aa.c version 1.0
 * A plugin that uses libaa (ftp://ftp.ta.jcu.cz/pub/aa) to save images as
 * ASCII.
 * NOTE: This plugin *requires* aalib 1.2 or later. Earlier versions will
 * not work.
 * Code copied from all over the GIMP source.
 * Tim Newsome <nuisance@cmu.edu>
 */

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

#include "config.h"

#include <string.h>

#include <aalib.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define SAVE_PROC      "file-aa-save"
#define PLUG_IN_BINARY "file-aa"
#define PLUG_IN_ROLE   "gimp-file-aa"


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
                                               GimpDrawable         *drawable,
                                               GFile                *file,
                                               const GimpValueArray *args,
                                               gpointer              run_data);

static gboolean         save_aa               (GimpDrawable         *drawable,
                                               gchar                *filename,
                                               gint                  output_type);
static void             gimp2aa               (GimpDrawable         *drawable,
                                               aa_context           *context);

static gint             aa_dialog             (gint                  selected);


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

      GIMP_PROC_ARG_STRING (procedure, "file-type",
                            "File type",
                            "File type to use",
                            NULL,
                            G_PARAM_READWRITE);
    }

  return procedure;
}

/**
 * Searches aa_formats defined by aalib to find the index of the type
 * specified by string.
 * -1 means it wasn't found.
 */
static gint
get_type_from_string (const gchar *string)
{
  gint type = 0;
  aa_format **p = (aa_format **) aa_formats;

  while (*p && strcmp ((*p)->formatname, string))
    {
      p++;
      type++;
    }

  if (*p == NULL)
    return -1;

  return type;
}

static GimpValueArray *
ascii_save (GimpProcedure        *procedure,
            GimpRunMode           run_mode,
            GimpImage            *image,
            GimpDrawable         *drawable,
            GFile                *file,
            const GimpValueArray *args,
            gpointer              run_data)
{
  GimpPDBStatusType  status      = GIMP_PDB_SUCCESS;
  GimpExportReturn   export      = GIMP_EXPORT_CANCEL;
  gint               output_type = 0;
  GError            *error       = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_ui_init (PLUG_IN_BINARY);

      export = gimp_export_image (&image, &drawable, "AA",
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

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (SAVE_PROC, &output_type);
      output_type = aa_dialog (output_type);
      if (output_type < 0)
        status = GIMP_PDB_CANCEL;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      output_type = get_type_from_string (GIMP_VALUES_GET_STRING (args, 0));
      if (output_type < 0)
        status = GIMP_PDB_CALLING_ERROR;
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (SAVE_PROC, &output_type);
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (save_aa (drawable, g_file_get_path (file), output_type))
        {
          gimp_set_data (SAVE_PROC, &output_type, sizeof (output_type));
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image);

  return gimp_procedure_new_return_values (procedure, status, error);
}

/**
 * The actual save function. What it's all about.
 * The image type has to be GRAY.
 */
static gboolean
save_aa (GimpDrawable *drawable,
         gchar        *filename,
         gint          output_type)
{
  aa_savedata  savedata;
  aa_context  *context;
  aa_format    format = *aa_formats[output_type];

  format.width  = gimp_drawable_width  (drawable) / 2;
  format.height = gimp_drawable_height (drawable) / 2;

  /* Get a libaa context which will save its output to filename. */
  savedata.name   = filename;
  savedata.format = &format;

  context = aa_init (&save_d, &aa_defparams, &savedata);
  if (!context)
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

static gint
aa_dialog (gint selected)
{
  GtkWidget *dialog;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *combo;
  gint       i;

  /* Create the actual window. */
  dialog = gimp_export_dialog_new (_("Text"), PLUG_IN_BINARY, SAVE_PROC);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (dialog)),
                      hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("_Format:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  combo = g_object_new (GIMP_TYPE_INT_COMBO_BOX, NULL);
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_show (combo);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

  for (i = 0; aa_formats[i]; i++)
    gimp_int_combo_box_append (GIMP_INT_COMBO_BOX (combo),
                               GIMP_INT_STORE_VALUE, i,
                               GIMP_INT_STORE_LABEL, aa_formats[i]->formatname,
                               -1);

  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo), selected,
                              G_CALLBACK (gimp_int_combo_box_get_active),
                              &selected, NULL);

  gtk_widget_show (dialog);

  if (gimp_dialog_run (GIMP_DIALOG (dialog)) != GTK_RESPONSE_OK)
    selected = -1;

  gtk_widget_destroy (dialog);

  return selected;
}
