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


/*
 * Declare some local functions.
 */
static void     query       (void);
static void     run         (const gchar      *name,
                             gint              nparams,
                             const GimpParam  *param,
                             gint             *nreturn_vals,
                             GimpParam       **return_vals);
static gboolean save_aa     (gint32            drawable_ID,
                             gchar            *filename,
                             gint              output_type);
static void     gimp2aa     (gint32            drawable_ID,
                             aa_context       *context);

static gint     aa_dialog   (gint              selected);


/*
 * Some global variables.
 */

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};


MAIN ()

static void
query (void)
{
  static const GimpParamDef save_args[] =
  {
    {GIMP_PDB_INT32,    "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"},
    {GIMP_PDB_IMAGE,    "image",        "Input image"},
    {GIMP_PDB_DRAWABLE, "drawable",     "Drawable to save"},
    {GIMP_PDB_STRING,   "filename",     "The name of the file to save the image in"},
    {GIMP_PDB_STRING,   "raw-filename", "The name entered"},
    {GIMP_PDB_STRING,   "file-type",    "File type to use"}
  };

  gimp_install_procedure (SAVE_PROC,
                          "Saves grayscale image in various text formats",
                          "This plug-in uses aalib to save grayscale image "
                          "as ascii art into a variety of text formats",
                          "Tim Newsome <nuisance@cmu.edu>",
                          "Tim Newsome <nuisance@cmu.edu>",
                          "1997",
                          N_("ASCII art"),
                          "RGB*, GRAY*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_register_file_handler_mime (SAVE_PROC, "text/plain");
  gimp_register_save_handler (SAVE_PROC, "txt,ansi,text", "");
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

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[2];
  GimpRunMode       run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  gint              output_type = 0;
  gint32            image_ID;
  gint32            drawable_ID;
  GimpExportReturn  export = GIMP_EXPORT_CANCEL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  run_mode    = param[0].data.d_int32;
  image_ID    = param[1].data.d_int32;
  drawable_ID = param[2].data.d_int32;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_ui_init (PLUG_IN_BINARY, FALSE);

      export = gimp_export_image (&image_ID, &drawable_ID, "AA",
                                  GIMP_EXPORT_CAN_HANDLE_RGB     |
                                  GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                  GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                  GIMP_EXPORT_CAN_HANDLE_ALPHA);

      if (export == GIMP_EXPORT_CANCEL)
        {
          values[0].data.d_status = GIMP_PDB_CANCEL;
          return;
        }
      break;
    default:
      break;
    }

  if (! (gimp_drawable_is_rgb (drawable_ID) ||
         gimp_drawable_is_gray (drawable_ID)))
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          gimp_get_data (SAVE_PROC, &output_type);
          output_type = aa_dialog (output_type);
          if (output_type < 0)
            status = GIMP_PDB_CANCEL;
          break;

        case GIMP_RUN_NONINTERACTIVE:
          /*  Make sure all the arguments are there!  */
          if (nparams != 6)
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
          else
            {
              output_type = get_type_from_string (param[5].data.d_string);
              if (output_type < 0)
                status = GIMP_PDB_CALLING_ERROR;
            }
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          gimp_get_data (SAVE_PROC, &output_type);
          break;

        default:
          break;
        }
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (save_aa (drawable_ID, param[3].data.d_string, output_type))
        {
          gimp_set_data (SAVE_PROC, &output_type, sizeof (output_type));
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image_ID);

  values[0].data.d_status = status;
}

/**
 * The actual save function. What it's all about.
 * The image type has to be GRAY.
 */
static gboolean
save_aa (gint32  drawable_ID,
         gchar  *filename,
         gint    output_type)
{
  aa_savedata  savedata;
  aa_context  *context;
  aa_format    format = *aa_formats[output_type];

  format.width  = gimp_drawable_width (drawable_ID)  / 2;
  format.height = gimp_drawable_height (drawable_ID) / 2;

  /* Get a libaa context which will save its output to filename. */
  savedata.name   = filename;
  savedata.format = &format;

  context = aa_init (&save_d, &aa_defparams, &savedata);
  if (!context)
    return FALSE;

  gimp2aa (drawable_ID, context);
  aa_flush (context);
  aa_close (context);

  return TRUE;
}

static void
gimp2aa (gint32      drawable_ID,
         aa_context *context)
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

  buffer = gimp_drawable_get_buffer (drawable_ID);

  width  = aa_imgwidth  (context);
  height = aa_imgheight (context);

  switch (gimp_drawable_type (drawable_ID))
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
                              &selected);

  gtk_widget_show (dialog);

  if (gimp_dialog_run (GIMP_DIALOG (dialog)) != GTK_RESPONSE_OK)
    selected = -1;

  gtk_widget_destroy (dialog);

  return selected;
}
