/* The GIMP -- an image manipulation program * Copyright (C) 1995 Spencer
 * Kimball and Peter Mattis * * This program is free software; you can
 * redistribute it and/or modify * it under the terms of the GNU General
 * Public License as published by * the Free Software Foundation; either
 * version 2 of the License, or * (at your option) any later version. * *
 * This program is distributed in the hope that it will be useful, * but
 * WITHOUT ANY WARRANTY; without even the implied warranty of *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the * GNU
 * General Public License for more details. * * You should have received a
 * copy of the GNU General Public License * along with this program; if not,
 * write to the Free Software * Foundation, Inc., 675 Mass Ave, Cambridge, MA
 * 02139, USA. */
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "libgimp/gimp.h"
#include "gtk/gtk.h"

/* Declare local functions. */
static void query (void);
static void run (char *name,
		 int nparams,
		 GParam * param,
		 int *nreturn_vals,
		 GParam ** return_vals);
static gint dialog ();

static void doit (GDrawable * drawable);

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
    gint width, height;
    gint x_offset, y_offset;
  }
config;

config my_config =
{
  16, 16,			/* width, height */
  0, 0,				/* x_offset, y_offset */
};


MAIN ()

     static void query ()
{
  static GParamDef args[] =
  {
    {PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    {PARAM_IMAGE, "image", "Input image (unused)"},
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
			  "Tim Newsome",
			  "1997",
			  "<Image>/Filters/Render/Grid",
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void 
run (char *name, int n_params, GParam * param, int *nreturn_vals,
     GParam ** return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  *nreturn_vals = 1;
  *return_vals = values;

  run_mode = param[0].data.d_int32;

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
	  /* Oh boy. We get to do a dialog box, because we can't really expect the
	   * user to set us up with the right values using gdb.
	   */
	  if (!dialog ())
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
      /*  Get the specified drawable  */
      drawable = gimp_drawable_get (param[2].data.d_drawable);

      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_color (drawable->id) || gimp_drawable_gray (drawable->id))
	{
	  gimp_progress_init ("Drawing Grid...");
	  gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));

	  srand (time (NULL));
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
  guchar color[4] =
  {0, 0, 0, 0};

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
      gimp_pixel_rgn_get_row (&srcPR, copybuf, sx1, h, width);
      if ((h - my_config.y_offset) % my_config.height == 0)
	{
	  for (w = sx1; w < sx2; w++)
	    {
	      for (b = 0; b < bytes; b++)
		{
		  copybuf[w * bytes + b] = color[b];
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
		      copybuf[w * bytes + b] = color[b];
		    }
		}
	    }
	}
      gimp_pixel_rgn_set_row (&destPR, copybuf, sx1, h, width);
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
  run_flag = 1;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void 
entry_callback (GtkWidget * widget, gpointer data)
{
  if (data == &my_config.width)
    my_config.width = atof (gtk_entry_get_text (GTK_ENTRY (widget)));
  else if (data == &my_config.height)
    my_config.height = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));
  else if (data == &my_config.x_offset)
    my_config.x_offset = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));
  else if (data == &my_config.y_offset)
    my_config.y_offset = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));
}

static gint 
dialog ()
{
  GtkWidget *dlg;
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *table;
  gchar buffer[12];
  gchar **argv;
  gint argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("plasma");

  gtk_init (&argc, &argv);
  gtk_rc_parse(gimp_gtkrc());

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "Grid");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) close_callback, NULL);

  /*  Action area  */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) ok_callback,
		      dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  /* The main table */
  /* Set its size (y, x) */
  table = gtk_table_new (4, 3, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  gtk_table_set_row_spacings (GTK_TABLE (table), 10);
  gtk_table_set_col_spacings (GTK_TABLE (table), 10);

	/**********************
	 * The X/Y labels *
	 **********************/
  label = gtk_label_new ("X");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0,
		    0);
  gtk_widget_show (label);

  label = gtk_label_new ("Y");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, 1, 2, GTK_FILL, GTK_FILL, 0,
		    0);
  gtk_widget_show (label);

	/************************
	 * The width entry: *
	 ************************/
  label = gtk_label_new ("Size:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 0,
		    0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL,
		    GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_set_usize (entry, 50, 0);
  sprintf (buffer, "%i", my_config.width);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) entry_callback, &my_config.width);
  gtk_widget_show (entry);

	/************************
	 * The height entry: *
	 ************************/
  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 2, 3, 2, 3, GTK_EXPAND | GTK_FILL,
		    GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_set_usize (entry, 50, 0);
  sprintf (buffer, "%i", my_config.height);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) entry_callback, &my_config.height);
  gtk_widget_show (entry);

  gtk_widget_show (dlg);

	/************************
	 * The x_offset entry: *
	 ************************/
  label = gtk_label_new ("Offset:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4, GTK_FILL, GTK_FILL, 0,
		    0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 3, 4, GTK_EXPAND | GTK_FILL,
		    GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_set_usize (entry, 50, 0);
  sprintf (buffer, "%i", my_config.x_offset);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) entry_callback, &my_config.x_offset);
  gtk_widget_show (entry);

	/************************
	 * The y_offset entry: *
	 ************************/
  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 2, 3, 3, 4, GTK_EXPAND | GTK_FILL,
		    GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_set_usize (entry, 50, 0);
  sprintf (buffer, "%i", my_config.y_offset);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) entry_callback, &my_config.y_offset);
  gtk_widget_show (entry);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return run_flag;
}
