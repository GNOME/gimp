/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* WMF loading file filter for the GIMP
 * -Dom Lachowicz <cinamod@hotmail.com> 2003
 * -Francis James Franklin <fjf@alinameridon.com>
 *
 * TODO:
 * *) image preview
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libwmf/api.h>
#include <libwmf/gd.h>

#include <gtk/gtk.h>

#include <libgimpmath/gimpmath.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


typedef struct
{
  gdouble scale;
} WmfLoadVals;

static WmfLoadVals load_vals =
{
  1.0 /* scale */
};


static void      query       (void);
static void      run         (const gchar      *name,
                              gint              nparams,
                              const GimpParam  *param,
                              gint             *nreturn_vals,
                              GimpParam       **return_vals);
static gint32    load_image  (const gchar      *filename,
                              GimpRunMode       runmode,
                              gboolean          preview);
static gboolean  load_dialog (const gchar      *filename);


GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,   /* init_proc  */
  NULL,   /* quit_proc  */
  query,  /* query_proc */
  run,    /* run_proc   */
};

MAIN ()


/*
 * 'query()' - Respond to a plug-in query...
 */
static void
query (void)
{
  static GimpParamDef load_args[] =
    {
      { GIMP_PDB_INT32,  "run_mode",     "Interactive, non-interactive" },
      { GIMP_PDB_STRING, "filename",     "The name of the file to load" },
      { GIMP_PDB_STRING, "raw_filename", "The name of the file to load" },
      { GIMP_PDB_FLOAT,  "scale",        "Scale in which to load image" }
    };

  static GimpParamDef load_return_vals[] =
    {
      { GIMP_PDB_IMAGE,   "image",         "Output image" }
    };

  gimp_install_procedure ("file_wmf_load",
                          "Loads files in the WMF file format",
                          "Loads files in the WMF file format",
                          "Dom Lachowicz <cinamod@hotmail.com>",
                          "Dom Lachowicz <cinamod@hotmail.com>",
                          "(c) 2003 - Version 0.3.0",
                          "<Load>/WMF",
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_magic_load_handler ("file_wmf_load",
                                    "wmf,apm", "",
                                    "0,string,\\327\\315\\306\\232,0,string,\\1\\0\\11\\0");
}

/*
 * 'run()' - Run the plug-in...
 */
static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam     values[2];
  GimpRunMode          run_mode;
  GimpPDBStatusType    status = GIMP_PDB_SUCCESS;
  gint32               image_ID;

  INIT_I18N ();

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, "file_wmf_load") == 0)
    {
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          gimp_get_data ("file_wmf_load", &load_vals);

          if (!load_dialog (param[1].data.d_string))
            status = GIMP_PDB_CANCEL;
          break;

        case GIMP_RUN_NONINTERACTIVE:
          gimp_get_data ("file_wmf_load", &load_vals);
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          gimp_get_data ("file_wmf_load", &load_vals);

        default:
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          load_vals.scale = CLAMP (load_vals.scale, 0.01, 100.0);

          image_ID = load_image (param[1].data.d_string, run_mode, FALSE);
          gimp_set_data ("file_wmf_load", &load_vals, sizeof (load_vals));

          if (image_ID != -1)
            {
              *nreturn_vals = 2;
              values[1].type         = GIMP_PDB_IMAGE;
              values[1].data.d_image = image_ID;
            }
          else
            status = GIMP_PDB_EXECUTION_ERROR;
        }
    }
  else
    status = GIMP_PDB_CALLING_ERROR;

  values[0].data.d_status = status;
}

static gboolean
load_dialog (const gchar *filename)
{
  GtkWidget *dialog;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *table;
  GtkWidget *slider;
  GtkObject *scale;
  gboolean   run = FALSE;

  gimp_ui_init ("wmf", FALSE);

  dialog = gimp_dialog_new (_("Load Windows Metafile"), "wmf",
                            NULL, 0,
                            gimp_standard_help_func, "wmf",

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

                            NULL);

  /* Rendering */
  frame = gtk_frame_new (g_strdup_printf (_("Rendering %s"), filename));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), frame,
		      TRUE, TRUE, 0);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  /* Scale label */
  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  label = gtk_label_new (_("Scale (log 2):"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /* Scale slider */
  scale = gtk_adjustment_new (0.0, -2.0, 2.0, 0.2, 0.2, 0.0);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (scale));
  gtk_table_attach (GTK_TABLE (table), slider, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_widget_show (slider);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  gtk_widget_show (dialog);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      load_vals.scale = pow (2,
                             gtk_adjustment_get_value (GTK_ADJUSTMENT (scale)));
      run = TRUE;
    }

  gtk_widget_destroy (GTK_WIDGET (dialog));

  return run;
}

static guchar *
pixbuf_gd_convert (gint *gd_pixels,
                   gint  width,
                   gint  height)
{
  guchar  *pixels = NULL;
  guchar  *px_ptr = NULL;
  int     *gd_ptr = gd_pixels;
  gint     i = 0;
  gint     j = 0;
  gsize    alloc_size = width * height * sizeof (guchar) * 4;
  guint    pixel;
  guchar   r,g,b,a;

  pixels = (guchar *) g_try_malloc (alloc_size);
  if (! pixels)
    return NULL;

  px_ptr = pixels;

  for (i = 0; i < height; i++)
    for (j = 0; j < width; j++)
      {
        pixel = (guint) (*gd_ptr++);

        b = (guchar) (pixel & 0xff);
        pixel >>= 8;
        g = (guchar) (pixel & 0xff);
        pixel >>= 8;
        r = (guchar) (pixel & 0xff);
        pixel >>= 7;
        a = (guchar) (pixel & 0xfe);
        a ^= 0xff;

        *px_ptr++ = r;
        *px_ptr++ = g;
        *px_ptr++ = b;
        *px_ptr++ = a;
      }
  return pixels;
}

static guchar *
wmf_load_file (const gchar *filename,
               gint        *width,
               gint        *height)
{
  guchar         *pixels   = NULL;

  /* the bits we need to decode the WMF via libwmf2's GD layer  */
  wmf_error_t     err;
  gulong          flags;
  wmf_gd_t       *ddata = NULL;
  wmfAPI         *API   = NULL;
  wmfAPI_Options  api_options;
  wmfD_Rect       bbox;
  gint           *gd_pixels = NULL;
  gdouble         resolution_x;
  gdouble         resolution_y;

  resolution_x = resolution_y = 72.0;
  *width = *height = -1;

  flags = WMF_OPT_IGNORE_NONFATAL | WMF_OPT_FUNCTION;
  api_options.function = wmf_gd_function;

  err = wmf_api_create (&API, flags, &api_options);
  if (err != wmf_E_None)
    goto _wmf_error;

  ddata = WMF_GD_GetData (API);
  ddata->type = wmf_gd_image;

  err = wmf_file_open (API, filename);
  if (err != wmf_E_None)
    goto _wmf_error;

  err = wmf_scan (API, 0, &bbox);
  if (err != wmf_E_None)
    goto _wmf_error;

  err = wmf_display_size (API, width, height, resolution_x, resolution_y);
  if (err != wmf_E_None || *width <= 0 || *height <= 0)
    goto _wmf_error;

  *width  *= load_vals.scale;
  *height *= load_vals.scale;

  ddata->bbox          = bbox;
  ddata->width         = *width;
  ddata->height        = *height;

  err = wmf_play (API, 0, &bbox);
  if (err != wmf_E_None)
    goto _wmf_error;

  wmf_file_close (API);

  if (ddata->gd_image != NULL)
    gd_pixels = wmf_gd_image_pixels (ddata->gd_image);
  if (gd_pixels == NULL)
    goto _wmf_error;

  pixels = pixbuf_gd_convert (gd_pixels, *width, *height);
  if (pixels == NULL)
    goto _wmf_error;

  wmf_api_destroy (API);
  API = NULL;

 _wmf_error:
  if (API)
    wmf_api_destroy (API);

  return pixels;
}

/*
 * 'load_image()' - Load a WMF image into a new image window.
 */
static gint32
load_image (const gchar *filename,
	    GimpRunMode  runmode,
	    gboolean     preview)
{
  gint32        image;
  gint32	layer;
  GimpDrawable *drawable;
  GimpPixelRgn	pixel_rgn;
  gchar        *status;
  gint          i, rowstride;

  guchar       *pixels, *buf;
  gint          width, height;

  pixels = buf = wmf_load_file (filename, &width, &height);
  rowstride = width * 4;

  if (!pixels)
    {
      g_message (_("Can't open '%s'"), filename);
      gimp_quit ();
    }

  status = g_strdup_printf (_("Loading %s:"), filename);
  gimp_progress_init (status);
  g_free (status);

  image = gimp_image_new (width, height, GIMP_RGB);

  if (image == -1)
    {
      g_message ("Can't allocate new image: %s\n", filename);
      gimp_quit ();
    }

  gimp_image_set_filename (image, filename);

  layer = gimp_layer_new (image, _("Rendered WMF"),
                          width, height, GIMP_RGBA_IMAGE, 100, GIMP_NORMAL_MODE);

  drawable = gimp_drawable_get (layer);

  gimp_tile_cache_ntiles ((width / gimp_tile_width ()) + 1);

  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0,
                       drawable->width, drawable->height, TRUE, FALSE);

  for (i = 0; i < height; i++)
    {
      gimp_pixel_rgn_set_row (&pixel_rgn, buf, 0, i, width);
      buf += rowstride;

      if (i % 100 == 0)
        gimp_progress_update ((gdouble) i / (gdouble) height);
    }

  g_free (pixels);

  gimp_drawable_detach (drawable);

  gimp_progress_update (1.0);

  /* Tell the GIMP to display the image.
   */
  gimp_image_add_layer (image, layer, 0);
  gimp_drawable_flush (drawable);

  return image;
}

