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

/* Original plug-in coded by Tim Newsome. 
 * 
 * Changed to make use of real-life units by Sven Neumann <sven@gimp.org>.
 * 
 * The interface code is heavily commented in the hope that it will
 * help other plug-in developers to adapt their plug-ins to make use
 * of the gimp_size_entry functionality.
 *
 * For more info see libgimp/gimpsizeentry.h
 */

#include <stdlib.h>
#include <stdio.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "libgimp/gimpintl.h"
#include "libgimp/gimpchainbutton.h"
#include "libgimp/gimpsizeentry.h"

/* Declare local functions. */
static void query  (void);
static void run    (char    *name,
		    int      nparams,
		    GParam  *param,
		    int     *nreturn_vals,
		    GParam **return_vals);
static gint dialog (gint32     image_ID,
		    GDrawable *drawable);
static void doit   (GDrawable *drawable);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,				/* init_proc */
  NULL,				/* quit_proc */
  query,			/* query_proc */
  run,				/* run_proc */
};

gint bytes;
gint sx1, sy1, sx2, sy2;
int run_flag = 0;

typedef struct
{
  gint width;
  gint height;
  gint x_offset;
  gint y_offset;
}
config;

config my_config =
{
  16, 16,			/* width, height */
  0, 0,				/* x_offset, y_offset */
};


MAIN ()

static 
void query (void)
{
  static GParamDef args[] =
  {
    {PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    {PARAM_IMAGE, "image", "Input image"},
    {PARAM_DRAWABLE, "drawable", "Input drawable"},
    {PARAM_INT32, "width", "Width"},
    {PARAM_INT32, "height", "Height"},
    {PARAM_INT32, "x_offset", "X Offset"},
    {PARAM_INT32, "y_offset", "Y Offset"},
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure ("plug_in_grid",
			  "Draws a grid.",
			  "",
			  "Tim Newsome",
			  "Tim Newsome, Sven Neumann",
			  "1997, 1999",
			  "<Image>/Filters/Render/Grid",
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
run (char    *name, 
     int      n_params, 
     GParam  *param, 
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  gint32 image_ID;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  *nreturn_vals = 1;
  *return_vals = values;

  run_mode = param[0].data.d_int32;
  image_ID = param[1].data.d_int32;
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  if (run_mode == RUN_NONINTERACTIVE)
    {
      if (n_params != 7)
	{
	  status = STATUS_CALLING_ERROR;
	}
      if( status == STATUS_SUCCESS)
	{
	  my_config.width = param[3].data.d_int32;
	  my_config.height = param[4].data.d_int32;
	  my_config.x_offset = param[5].data.d_int32;
	  my_config.y_offset = param[6].data.d_int32;
	}
    }
  else
    {
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_grid", &my_config);

      if (run_mode == RUN_INTERACTIVE)
	{
	  if (!dialog (image_ID, drawable))
	    {
	      /* The dialog was closed, or something similarly evil happened. */
	      status = STATUS_EXECUTION_ERROR;
	    }
	}
    }

  if (my_config.width <= 0 || my_config.height <= 0)
    {
      status = STATUS_EXECUTION_ERROR;
    }

  if (status == STATUS_SUCCESS)
    {
      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_is_rgb (drawable->id) || gimp_drawable_is_gray (drawable->id))
	{
	  gimp_progress_init (_("Drawing Grid..."));
	  gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));

	  doit (drawable);

	  if (run_mode != RUN_NONINTERACTIVE)
	    gimp_displays_flush ();

	  if (run_mode == RUN_INTERACTIVE)
	    gimp_set_data ("plug_in_grid", &my_config, sizeof (my_config));
	}
      else
	{
	  status = STATUS_EXECUTION_ERROR;
	}
      gimp_drawable_detach (drawable);
    }

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
}

static void
doit (GDrawable * drawable)
{
  GPixelRgn srcPR, destPR;
  gint width, height;
  int w, h, b;
  guchar *copybuf;
  guchar color[4] = {0, 0, 0, 0};

  /* Get the input area. This is the bounding box of the selection in
   *  the image (or the entire image if there is no selection). Only
   *  operating on the input area is simply an optimization. It doesn't
   *  need to be done for correct operation. (It simply makes it go
   *  faster, since fewer pixels need to be operated on).
   */
  gimp_drawable_mask_bounds (drawable->id, &sx1, &sy1, &sx2, &sy2);

  /* Get the size of the input image. (This will/must be the same
   *  as the size of the output image.
   */
  width = drawable->width;
  height = drawable->height;
  bytes = drawable->bpp;

  if (gimp_drawable_has_alpha (drawable->id))
    {
      color[bytes - 1] = 0xff;
    }

  /*  initialize the pixel regions  */
  gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, drawable, 0, 0, width, height, TRUE, TRUE);

  /* First off, copy the old one to the new one. */
  copybuf = malloc (width * bytes);

  for (h = sy1; h < sy2; h++)
    {
      gimp_pixel_rgn_get_row (&srcPR, copybuf, sx1, h, (sx2-sx1));
      if ((h - my_config.y_offset) % my_config.height == 0)
	{ /* Draw row */
	  for (w = sx1; w < sx2; w++)
	    {
	      for (b = 0; b < bytes; b++)
		{
		  copybuf[(w-sx1) * bytes + b] = color[b];
		}
	    }
	}
      else
	{
	  for (w = sx1; w < sx2; w++)
	    {
	      if ((w - my_config.x_offset) % my_config.width == 0)
		{
		  for (b = 0; b < bytes; b++)
		    {
		      copybuf[(w-sx1) * bytes + b] = color[b];
		    }
		}
	    }
	}
      gimp_pixel_rgn_set_row (&destPR, copybuf, sx1, h , (sx2-sx1) );
      gimp_progress_update ((double) h / (double) (sy2 - sy1));
    }
  free (copybuf);

  /*  update the timred region  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, sx1, sy1, sx2 - sx1, sy2 - sy1);
}

/***************************************************
 * GUI stuff
 */

static void
close_callback (GtkWidget * widget, gpointer data)
{
  gtk_main_quit ();
}

static void
ok_callback (GtkWidget * widget, gpointer data)
{
  GtkWidget *entry;

  run_flag = 1;

  entry = gtk_object_get_data (GTK_OBJECT (data), "size");
  my_config.width  = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 0) + 0.5);
  my_config.height = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 1) + 0.5);
  
  entry = gtk_object_get_data (GTK_OBJECT (data), "offset");
  my_config.x_offset = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 0) + 0.5);
  my_config.y_offset = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 1) + 0.5);
 
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
entry_callback (GtkWidget * widget, gpointer data)
{
  static gdouble x = -1.0;
  static gdouble y = -1.0;
  gdouble new_x;
  gdouble new_y;

  new_x = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0);
  new_y = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1);

  if (gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (data)))
    {
      if (new_x != x)
	{
	  y = new_y = x = new_x;
	  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (widget), 1, y);
	}
      if (new_y != y)
	{
	  x = new_x = y = new_y;
	  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (widget), 0, x);
	}
    }
  else
    {
      if (new_x != x)
	x = new_x;
      if (new_y != y)
	y = new_y;
    }     
}

static gint
dialog (gint32     image_ID,
	GDrawable *drawable)
{
  GtkWidget *dlg;
  GtkWidget *hbbox;
  GtkWidget *button;
  GtkWidget *hbox;
  GtkWidget *size;
  GtkWidget *offset;
  GUnit      unit;
  gdouble    xres;
  gdouble    yres;
  gchar **argv;
  gint argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("grid");

  gtk_init (&argc, &argv);
  gtk_rc_parse(gimp_gtkrc());

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), _("Grid"));
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) close_callback, NULL);

  /*  Get the image resolution and unit  */
  gimp_image_get_resolution (image_ID, &xres, &yres);
  unit   = gimp_image_get_unit (image_ID);

  /*  Action area  */
  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dlg)->action_area), 2);
  gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dlg)->action_area), FALSE);
  hbbox = gtk_hbutton_box_new ();
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbbox), 4);
  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (dlg)->action_area), hbbox, FALSE, FALSE, 0);
  gtk_widget_show (hbbox);
 
  button = gtk_button_new_with_label (_("OK"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) ok_callback,
		      dlg);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Cancel"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  The size entries  */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), hbox, FALSE, FALSE, 4);

  size = gimp_size_entry_new (2,                            /*  number_of_fields  */ 
			      unit,                         /*  unit              */
			      "%a",                         /*  unit_format       */
			      TRUE,                         /*  menu_show_pixels  */
			      TRUE,                         /*  menu_show_percent */
			      FALSE,                        /*  show_refval       */
			      75,                           /*  spinbutton_usize  */
			      GIMP_SIZE_ENTRY_UPDATE_SIZE); /*  update_policy     */

  /*  set the resolution to the image resolution  */
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (size), 0, xres, TRUE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (size), 1, yres, TRUE);

  /*  set the size (in pixels) that will be treated as 0% and 100%  */
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (size), 0, 0.0, (gdouble)(drawable->width));
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (size), 1, 0.0, (gdouble)(drawable->height));

  /*  set upper and lower limits (in pixels)  */
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (size), 0, 0.0, (gdouble)(drawable->width));
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (size), 1, 0.0, (gdouble)(drawable->height));

  /*  initialize the values  */
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (size), 0, (gdouble)my_config.width);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (size), 1, (gdouble)my_config.height);

  /*  attach labels  */
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (size), _("X"), 0, 1, 0.5);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (size), _("Y"), 0, 2, 0.5);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (size), _("Size: "), 1, 0, 0.0); 

  /*  put a chain_button under the size_entries  */
  button = gimp_chain_button_new (GIMP_CHAIN_BOTTOM);
  if (my_config.width == my_config.height)
    gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (button), TRUE);
  gtk_table_attach_defaults (GTK_TABLE (size), button, 1, 3, 2, 3);
  gtk_widget_show (button);
 
  /*  connect to the 'value_changed' and "unit_changed" signals because we have to 
      take care of keeping the entries in sync when the chainbutton is active        */
  gtk_signal_connect (GTK_OBJECT (size), "value_changed", (GtkSignalFunc) entry_callback, button);
  gtk_signal_connect (GTK_OBJECT (size), "unit_changed", (GtkSignalFunc) entry_callback, button);

  gtk_box_pack_end (GTK_BOX (hbox), size, FALSE, FALSE, 4);
  gtk_widget_show (size);
  gtk_widget_show (hbox);

  /*  the offset entries  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), hbox, FALSE, FALSE, 0);
  offset = gimp_size_entry_new (2, unit, "%a", TRUE, TRUE, FALSE, 75, 
				GIMP_SIZE_ENTRY_UPDATE_SIZE); 
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (offset), 0, xres, TRUE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (offset), 1, yres, TRUE);
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (offset), 0, 0.0, (gdouble)(drawable->width));
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (offset), 1, 0.0, (gdouble)(drawable->height));
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (offset), 0, 0.0, (gdouble)(drawable->width));
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (offset), 1, 0.0, (gdouble)(drawable->height));
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (offset), 0, (gdouble)my_config.x_offset);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (offset), 1, (gdouble)my_config.y_offset);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (offset), _("Offset: "), 1, 0, 0.0);
  gtk_box_pack_end (GTK_BOX (hbox), offset, FALSE, FALSE, 4);
  gtk_widget_show (offset);
  gtk_widget_show (hbox);

  gtk_widget_show (dlg);

  gtk_object_set_data (GTK_OBJECT (dlg), "size", size);
  gtk_object_set_data (GTK_OBJECT (dlg), "offset", offset);  

  gtk_main ();
  gdk_flush ();

  return run_flag;
}
