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

/* SVG loading file filter for the GIMP
 * -Dom Lachowicz <cinamod@hotmail.com> 2003
 *
 * TODO: 
 * *) image preview
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rsvg.h>

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

typedef struct
{
  double scale;
} SvgLoadVals;

static SvgLoadVals load_vals =
{
  1.0				/* scale */
};

typedef struct
{
  gint run;
} SvgLoadInterface;

static SvgLoadInterface load_interface =
{
  FALSE
};

typedef struct
{
  GtkWidget     *dialog;
  GtkAdjustment *scale;
} LoadDialogVals;

/* TODO: remove me, initialize gimp i18n services */
#define _(String) (String)

static void     query                     (void);
static void     run                       (const gchar            *name,
					   gint              nparams,
					   const GimpParam        *param,
					   gint             *nreturn_vals,
					   GimpParam       **return_vals);

GimpPlugInInfo PLUG_IN_INFO = {
  NULL,                         /* init_proc  */
  NULL,                         /* quit_proc  */
  query,                        /* query_proc */
  (GimpRunProc)run,             /* run_proc   */
};

MAIN ()

static void
check_load_vals (void)
{
  if (load_vals.scale < 0.01)
    load_vals.scale = 0.01;
  else if (load_vals.scale > 100.)
    load_vals.scale = 100.;
}

/*
 * 'load_image()' - Load a SVG image into a new image window.
 */
static gint32
load_image (gchar       *filename, 	/* I - File to load */ 
	    GimpRunMode  runmode, 
	    gboolean     preview)
{
  gint32        image;	        /* Image */
  gint32	layer;		/* Layer */
  GimpDrawable *drawable;	/* Drawable for layer */
  GimpPixelRgn	pixel_rgn;	/* Pixel region for layer */
  GdkPixbuf    *pixbuf;        /* SVG, rasterized */
  gchar        *status;

  pixbuf = rsvg_pixbuf_from_file_at_zoom (filename, load_vals.scale, load_vals.scale, NULL);

  if (!pixbuf)
    {
      g_message (_("can't open \"%s\"\n"), filename);
      gimp_quit ();
    }

  if (runmode != GIMP_RUN_NONINTERACTIVE)
    {
      status = g_strdup_printf (_("Loading %s:"), filename);
      gimp_progress_init (status);
      g_free (status);
    }

  image = gimp_image_new (gdk_pixbuf_get_width (pixbuf), 
			  gdk_pixbuf_get_height (pixbuf),
			  GIMP_RGB);
  if (image == -1)
    {
      g_message ("Can't allocate new image: %s\n", filename);
      gimp_quit ();
    }

  gimp_image_set_filename (image, filename);

  layer = gimp_layer_new (image, _("Background"),
			  gdk_pixbuf_get_width (pixbuf), 
			  gdk_pixbuf_get_height (pixbuf),
			  GIMP_RGBA_IMAGE, 100, GIMP_NORMAL_MODE);
  
  drawable = gimp_drawable_get (layer);
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0,
		       drawable->width, drawable->height, TRUE, FALSE);

  gimp_pixel_rgn_set_rect (&pixel_rgn, gdk_pixbuf_get_pixels (pixbuf),
			   0, 0, 
			   gdk_pixbuf_get_width (pixbuf),
			   gdk_pixbuf_get_height (pixbuf));

  g_object_unref (G_OBJECT(pixbuf));

  /* Tell the GIMP to display the image.
   */
  gimp_image_add_layer (image, layer, 0);
  gimp_drawable_flush (drawable);

  return image;
}

static void
load_ok_callback (GtkWidget *widget,
                  gpointer   data)

{
  LoadDialogVals *vals = (LoadDialogVals *)data;

  /* Read scale */
  load_vals.scale = pow (2, vals->scale->value);

  load_interface.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (vals->dialog));
}

static gint
load_dialog (gchar *file_name)
{
  LoadDialogVals *vals;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *table;
  GtkWidget *slider;

  gimp_ui_init ("svg", FALSE);

  vals = g_new (LoadDialogVals, 1);

  vals->dialog = gimp_dialog_new ( _("Load SVG"), "svg",
				  NULL, NULL,
				  GTK_WIN_POS_MOUSE,
				  FALSE, TRUE, FALSE,

				  GTK_STOCK_CANCEL, gtk_widget_destroy,
				  NULL, 1, NULL, FALSE, TRUE,

				  GTK_STOCK_OK, load_ok_callback,
				  vals, NULL, NULL, TRUE, FALSE,

				  NULL);

  g_signal_connect (vals->dialog, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  /* Rendering */
  frame = gtk_frame_new (g_strdup_printf ( _("Rendering %s"), file_name));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (vals->dialog)->vbox), frame,
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

  label = gtk_label_new ( _("Scale (log 2):"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /* Scale slider */
  vals->scale = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, -2.0, 2.0, 0.2, 0.2, 0.0));
  slider = gtk_hscale_new (vals->scale);
  gtk_table_attach (GTK_TABLE (table), slider, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_widget_show (slider);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  gtk_widget_show (vals->dialog);

  gtk_main ();
  gdk_flush ();

  g_free (vals);

  return load_interface.run;
}

/*
 * 'query()' - Respond to a plug-in query...
 */
static void
query (void)
{
  static GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,    "run_mode",     "Interactive, non-interactive" },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING,   "raw_filename", "The name of the file to load" }
  };

  static GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE,   "image",         "Output image" }
  };

  static GimpParamDef load_setargs_args[] =
  {
    { GIMP_PDB_FLOAT, "scale", "Scale in which to load image" }
  };

  gimp_install_procedure ("file_svg_load",
                          "Loads files in the SVG file format",
                          "Loads files in the SVG file format",
                          "Dom Lachowicz <cinamod@hotmail.com>",
                          "Dom Lachowicz <cinamod@hotmail.com>",
                          "(c) 2003 - " VERSION,
			  "<Load>/SVG",
			  NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_install_procedure ("file_svg_load_setargs",
			  "set additional parameters for the procedure file_svg_load",
			  "set additional parameters for the procedure file_svg_load",
			  "Dom Lachowicz <cinamod@hotmail.com>",
                          "Dom Lachowicz",
                          "2003",
			  NULL,
			  NULL,
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (load_setargs_args), 0,
			  load_setargs_args, NULL);

  gimp_register_magic_load_handler ("file_svg_load",
				    "svg", "",
				    "0,string,<?xml,0,string,<svg");
}

/*
 * 'run()' - Run the plug-in...
 */
static void
run (const gchar      *name,
     gint        nparams,
     const GimpParam  *param,
     gint       *nreturn_vals,
     GimpParam **return_vals)
{
  static GimpParam     values[2];
  GimpRunMode          run_mode;
  GimpPDBStatusType    status = GIMP_PDB_SUCCESS;
  gint32               image_ID;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  /* MUST call this before any RSVG funcs */
  g_type_init ();

  if (strcmp (name, "file_svg_load") == 0)
    {
      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	  gimp_get_data ("file_svg_load", &load_vals);

	  if (!load_dialog (param[1].data.d_string))
	    status = GIMP_PDB_CANCEL;
	  break;
	  
	case GIMP_RUN_NONINTERACTIVE:
	  gimp_get_data ("file_svg_load", &load_vals);
	  break;

	case GIMP_RUN_WITH_LAST_VALS:
	  gimp_get_data ("file_svg_load", &load_vals);

	default:
	  break;
	}

      if (status == GIMP_PDB_SUCCESS)
	{
	  check_load_vals ();
	  
	  image_ID = load_image (param[1].data.d_string, run_mode, FALSE);
	  gimp_set_data ("file_svg_load", &load_vals, sizeof (load_vals));

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
