/*
 * This is a plugin for the GIMP.
 *
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1996 Torsten Martinsen
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
 *
 * $Id$
 */

/*
 * This filter adds random noise to an image.
 * The amount of noise can be set individually for each RGB channel.
 * This filter does not operate on indexed images.
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

#define ENTRY_WIDTH  60
#define SCALE_WIDTH 125
#define TILE_CACHE_SIZE 16

typedef struct
{
  gint    independent;
  gdouble noise[4];     /*  per channel  */
} NoisifyVals;

typedef struct
{
  gint       run;
} NoisifyInterface;

/* Declare local functions.
 */
static void      query  (void);
static void      run    (char      *name,
			 int        nparams,
			 GParam    *param,
			 int       *nreturn_vals,
			 GParam   **return_vals);

static void      noisify        (GDrawable * drawable);
static gint      noisify_dialog (gint        channels);
static gdouble   gauss          (void);

static void      noisify_close_callback  (GtkWidget *widget,
					  gpointer   data);
static void      noisify_ok_callback     (GtkWidget *widget,
					  gpointer   data);
static void      noisify_toggle_update   (GtkWidget *widget,
					  gpointer   data);
static void      noisify_scale_update    (GtkAdjustment *adjustment,
					  double        *scale_val);
static void      noisify_entry_update    (GtkWidget *widget,
					  gdouble *value);
static void      dialog_create_value     (char *title,
					  GtkTable *table,
					  int row,
					  gdouble *value,
					  double left,
					  double right);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static NoisifyVals nvals =
{
  TRUE,
  { 0.20, 0.20, 0.20, 0.20 }
};

static NoisifyInterface noise_int =
{
  FALSE     /* run */
};


MAIN ()

static void
query ()
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (unused)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_INT32, "independent", "Noise in channels independent" },
    { PARAM_FLOAT, "noise_1", "Noise in the first channel (red, gray)" },
    { PARAM_FLOAT, "noise_2", "Noise in the second channel (green, gray_alpha)" },
    { PARAM_FLOAT, "noise_3", "Noise in the third channel (blue)" },
    { PARAM_FLOAT, "noise_4", "Noise in the fourth channel (alpha)" }
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure ("plug_in_noisify",
			  "Adds random noise to a drawable's channels",
			  "More here later",
			  "Torsten Martinsen",
			  "Torsten Martinsen",
			  "1996",
			  "<Image>/Filters/Noise/Noisify...",
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_noisify", &nvals);

      /*  First acquire information with a dialog  */
      if (! noisify_dialog (drawable->bpp))
	{
	  gimp_drawable_detach (drawable);
	  return;
	}
      break;

    case RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 8)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  nvals.independent = (param[3].data.d_int32) ? TRUE : FALSE;
	  nvals.noise[0] = param[4].data.d_float;
	  nvals.noise[1] = param[5].data.d_float;
	  nvals.noise[2] = param[6].data.d_float;
	  nvals.noise[3] = param[7].data.d_float;
	}
      break;

    case RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_noisify", &nvals);
      break;

    default:
      break;
    }

  /*  Make sure that the drawable is gray or RGB color  */
  if (gimp_drawable_is_rgb (drawable->id) || gimp_drawable_is_gray (drawable->id))
    {
      gimp_progress_init ("Adding Noise...");
      gimp_tile_cache_ntiles (TILE_CACHE_SIZE);

      /*  seed the random number generator  */
      srand (time (NULL));

      /*  compute the luminosity which exceeds the luminosity threshold  */
      noisify (drawable);

      if (run_mode != RUN_NONINTERACTIVE)
	gimp_displays_flush ();

      /*  Store data  */
      if (run_mode == RUN_INTERACTIVE)
	gimp_set_data ("plug_in_noisify", &nvals, sizeof (NoisifyVals));
    }
  else
    {
      /* gimp_message ("blur: cannot operate on indexed color images"); */
      status = STATUS_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static void
noisify (GDrawable *drawable)
{
  GPixelRgn src_rgn, dest_rgn;
  guchar *src_row, *dest_row;
  guchar *src, *dest;
  gint row, col, b;
  gint x1, y1, x2, y2, p;
  gint noise;
  gint progress, max_progress;
  gpointer pr;

  /* initialize */

  noise = 0;

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
  gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);

  /* Initialize progress */
  progress = 0;
  max_progress = (x2 - x1) * (y2 - y1);

  for (pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn); pr != NULL; pr = gimp_pixel_rgns_process (pr))
    {
      src_row = src_rgn.data;
      dest_row = dest_rgn.data;

      for (row = 0; row < src_rgn.h; row++)
	{
	  src = src_row;
	  dest = dest_row;

	  for (col = 0; col < src_rgn.w; col++)
	    {
	      if (nvals.independent == FALSE)
		noise = (gint) (nvals.noise[0] * gauss() * 127);

	      for (b = 0; b < src_rgn.bpp; b++)
		{
		  if (nvals.independent == TRUE)
		    noise = (gint) (nvals.noise[b] * gauss() * 127);

		  
		  p = src[b] + noise;
		  if (p < 0)
		    p = 0;
		  else if (p > 255)
		    p = 255;
		  if (nvals.noise[b] != 0)
		    dest[b] = p;
		  
		}
	      src += src_rgn.bpp;
	      dest += dest_rgn.bpp;
	    }

	  src_row += src_rgn.rowstride;
	  dest_row += dest_rgn.rowstride;
	}

      /* Update progress */
      progress += src_rgn.w * src_rgn.h;
      gimp_progress_update ((double) progress / (double) max_progress);
    }

  /*  update the blurred region  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));
}

static gint
noisify_dialog (gint channels)
{
  GtkWidget *dlg;
  GtkWidget *button;
  GtkWidget *hbbox;
  GtkWidget *toggle;
  GtkWidget *frame;
  GtkWidget *table;
  gchar buffer[32];
  gchar **argv;
  gint argc;
  int i;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("noisify");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "Noisify");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) noisify_close_callback,
		      NULL);

  /*  Action area  */
  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dlg)->action_area), 2);
  gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dlg)->action_area), FALSE);
  hbbox = gtk_hbutton_box_new ();
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbbox), 4);
  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (dlg)->action_area), hbbox, FALSE, FALSE, 0);
  gtk_widget_show (hbbox);
 
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) noisify_ok_callback,
		      dlg);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  parameter settings  */
  frame = gtk_frame_new ("Parameter Settings");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  table = gtk_table_new (channels + 1, 3, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_container_add (GTK_CONTAINER (frame), table);

  toggle = gtk_check_button_new_with_label ("Independent");
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 2, 0, 1, GTK_FILL, 0, 0, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) noisify_toggle_update,
		      &nvals.independent);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), nvals.independent);
  gtk_widget_show (toggle);

  /*  for (i = 0; i < channels; i++)
   {
      sprintf (buffer, "Channel #%d", i);
      dialog_create_value(buffer, GTK_TABLE(table), i+1, &nvals.noise[i], 0.0, 1.0);
    }
    */
  
  if (channels == 1) 
    {
      sprintf (buffer, "Gray");
      dialog_create_value(buffer, GTK_TABLE(table), 1, &nvals.noise[0], 0.0, 1.0);
    }

  else if (channels == 2)
    {
      sprintf (buffer, "Gray");
      dialog_create_value(buffer, GTK_TABLE(table), 1, &nvals.noise[0], 0.0, 1.0);
      sprintf (buffer, "Alpha");
      dialog_create_value(buffer, GTK_TABLE(table), 2, &nvals.noise[1], 0.0, 1.0);
    }
  
  else if (channels == 3)
    {
      sprintf (buffer, "Red");
      dialog_create_value(buffer, GTK_TABLE(table), 1, &nvals.noise[0], 0.0, 1.0);
      sprintf (buffer, "Green");
      dialog_create_value(buffer, GTK_TABLE(table), 2, &nvals.noise[1], 0.0, 1.0);
      sprintf (buffer, "Blue");
      dialog_create_value(buffer, GTK_TABLE(table), 3, &nvals.noise[2], 0.0, 1.0);
    }

  else if (channels == 4)
    {
      sprintf (buffer, "Red");
      dialog_create_value(buffer, GTK_TABLE(table), 1, &nvals.noise[0], 0.0, 1.0);
      sprintf (buffer, "Green");
      dialog_create_value(buffer, GTK_TABLE(table), 2, &nvals.noise[1], 0.0, 1.0);
      sprintf (buffer, "Blue");
      dialog_create_value(buffer, GTK_TABLE(table), 3, &nvals.noise[2], 0.0, 1.0);
      sprintf (buffer, "Alpha");
      dialog_create_value(buffer, GTK_TABLE(table), 4, &nvals.noise[3], 0.0, 1.0);
    }
  
  else
    {
      for (i = 0; i < channels; i++)
	{
	  sprintf (buffer, "Channel #%d", i);
	  dialog_create_value(buffer, GTK_TABLE(table), i+1, &nvals.noise[i], 0.0, 1.0);
	}
    }
  

  gtk_widget_show (frame);
  gtk_widget_show (table);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return noise_int.run;
}

/*
 * Return a Gaussian (aka normal) random variable.
 *
 * Adapted from ppmforge.c, which is part of PBMPLUS.
 * The algorithm comes from:
 * 'The Science Of Fractal Images'. Peitgen, H.-O., and Saupe, D. eds.
 * Springer Verlag, New York, 1988.
 */
static gdouble
gauss ()
{
  gint i;
  gdouble sum = 0.0;

  for (i = 0; i < 4; i++)
    sum += rand () & 0x7FFF;

  return sum * 5.28596089837e-5 - 3.46410161514;
}


/*  Noisify interface functions  */

static void
noisify_close_callback (GtkWidget *widget,
			gpointer   data)
{
  gtk_main_quit ();
}

static void
noisify_ok_callback (GtkWidget *widget,
		     gpointer   data)
{
  noise_int.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
noisify_toggle_update (GtkWidget *widget,
		       gpointer   data)
{
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}


/*
 * Thanks to Quartic for these.
 */
static void
dialog_create_value(char *title, GtkTable *table, int row, gdouble *value, double left, double right)
{
	GtkWidget *label;
	GtkWidget *scale;
	GtkWidget *entry;
	GtkObject *scale_data;
	char       buf[256];

	label = gtk_label_new(title);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_table_attach(table, label, 0, 1, row, row + 1, GTK_FILL, GTK_FILL, 4, 0);
	gtk_widget_show(label);

	scale_data = gtk_adjustment_new(*value, left, right,
					(right - left) / 200.0,
					(right - left) / 200.0,
					0.0);

	gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
			   (GtkSignalFunc) noisify_scale_update,
			   value);

	scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
	gtk_widget_set_usize(scale, SCALE_WIDTH, 0);
	gtk_table_attach(table, scale, 1, 2, row, row + 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
	gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
	gtk_scale_set_digits(GTK_SCALE(scale), 3);
	gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
	gtk_widget_show(scale);

	entry = gtk_entry_new();
	gtk_object_set_user_data(GTK_OBJECT(entry), scale_data);
	gtk_object_set_user_data(scale_data, entry);
	gtk_widget_set_usize(entry, ENTRY_WIDTH, 0);
	sprintf(buf, "%0.2f", *value);
	gtk_entry_set_text(GTK_ENTRY(entry), buf);
	gtk_signal_connect(GTK_OBJECT(entry), "changed",
			   (GtkSignalFunc) noisify_entry_update,
			   value);
	gtk_table_attach(GTK_TABLE(table), entry, 2, 3, row, row + 1, GTK_FILL, GTK_FILL, 4, 0);
	gtk_widget_show(entry);
}

static void
noisify_entry_update(GtkWidget *widget, gdouble *value)
{
	GtkAdjustment *adjustment;
	gdouble        new_value;

	new_value = atof(gtk_entry_get_text(GTK_ENTRY(widget)));

	if (*value != new_value) {
		adjustment = gtk_object_get_user_data(GTK_OBJECT(widget));

		if ((new_value >= adjustment->lower) &&
		    (new_value <= adjustment->upper)) {
			*value            = new_value;
			adjustment->value = new_value;

			gtk_signal_emit_by_name(GTK_OBJECT(adjustment), "value_changed");
		} /* if */
	} /* if */
}

static void
noisify_scale_update (GtkAdjustment *adjustment, gdouble *value)
{
	GtkWidget *entry;
	char       buf[256];

	if (*value != adjustment->value) {
		*value = adjustment->value;

		entry = gtk_object_get_user_data(GTK_OBJECT(adjustment));
		sprintf(buf, "%0.2f", *value);

		gtk_signal_handler_block_by_data(GTK_OBJECT(entry), value);
		gtk_entry_set_text(GTK_ENTRY(entry), buf);
		gtk_signal_handler_unblock_by_data(GTK_OBJECT(entry), value);
	} /* if */
}
