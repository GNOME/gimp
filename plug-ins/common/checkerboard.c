/*
 * This is a plug-in for the GIMP.
 *
 * Copyright (C) 1997 Brent Burton & the Edward Blevins
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
 */

#include <stdio.h>
#include <stdlib.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

#include "config.h"
#include "libgimp/stdplugins-intl.h"

/* Variables set in dialog box */
typedef struct data {
    gint mode;
    gint size;
} CheckVals;

typedef struct {
  gint run;
} CheckInterface;

static CheckInterface cint =
{
  FALSE
};

static void      query  (void);
static void      run    (gchar    *name,
			 gint      nparams,
			 GParam   *param,
			 gint     *nreturn_vals,
			 GParam  **return_vals);
static void      check  (GDrawable * drawable);

static gint      check_dialog (void);
static void      check_close_callback (GtkWidget *widget, gpointer   data);
static void      check_ok_callback (GtkWidget *widget, gpointer   data);
static void      check_toggle_update (GtkWidget *widget, gpointer   data);
static void      check_slider_update (GtkAdjustment *adjustment, gint  *size_val);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static CheckVals cvals =
{
   0,
   10
};

static int inblock(int pos, int size);



MAIN ()

static void
query ()
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (unused)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_INT32, "check_mode", "Regular or Physcobilly" },
    { PARAM_INT32, "check_size", "Size of the checks" }
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  INIT_I18N();

  gimp_install_procedure ("plug_in_checkerboard",
			  _("Adds a checkerboard pattern to an image"),
			  "More here later",
			  "Brent Burton & the Edward Blevins",
			  "Brent Burton & the Edward Blevins",
			  "1997",
			  _("<Image>/Filters/Render/Checkerboard"),
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
run    (gchar    *name,
	gint      nparams,
	GParam   *param,
	gint     *nreturn_vals,
	GParam  **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  INIT_I18N();

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      gimp_get_data ("plug_in_checkerboard", &cvals);
      if (! check_dialog())
	{
	  gimp_drawable_detach (drawable);
	  return;
	}
      break;

    case RUN_NONINTERACTIVE:
      if (nparams != 5)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  cvals.mode = param[3].data.d_int32;
	  cvals.size = param[4].data.d_int32;
	}
      break;

    case RUN_WITH_LAST_VALS:
      gimp_get_data ("plug_in_checkerboard", &cvals);
      break;

    default:
      break;
    }

  if (gimp_drawable_color (drawable->id) || gimp_drawable_gray (drawable->id))
    {
      gimp_progress_init (_("Adding Checkerboard..."));

      check (drawable);

      if (run_mode != RUN_NONINTERACTIVE)
	gimp_displays_flush ();

      if (run_mode == RUN_INTERACTIVE)
	gimp_set_data ("plug_in_checkerboard", &cvals, sizeof (CheckVals));
    }
  else
    {
      status = STATUS_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}



static void
check( GDrawable * drawable)
{
  GPixelRgn dest_rgn;
  GParam *return_vals;
  gint nreturn_vals;
  guchar *dest_row;
  guchar *dest;
  gint row, col;
  gint progress, max_progress;
  gint x1, y1, x2, y2, x, y;
  guint8 fg[4],bg[4];
  gint bp;
  gpointer pr;

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
  gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);

  progress = 0;
  max_progress = (x2 - x1) * (y2 - y1);

  /* Get the foreground and background colors */

  switch ( gimp_drawable_type (drawable->id) )
    {
    case RGBA_IMAGE:
      fg[3] = 255;
      bg[3] = 255;
    case RGB_IMAGE :

      return_vals = gimp_run_procedure ("gimp_palette_get_foreground",
					&nreturn_vals,
					PARAM_END);

      if (return_vals[0].data.d_status == STATUS_SUCCESS)
	{
	  fg[0] = return_vals[1].data.d_color.red;
	  fg[1] = return_vals[1].data.d_color.green;
	  fg[2] = return_vals[1].data.d_color.blue;
	}
      else
	{
	  fg[0] = 255;
	  fg[1] = 255;
	  fg[2] = 255;
	}
      return_vals = gimp_run_procedure ("gimp_palette_get_background",
					&nreturn_vals,
					PARAM_END);

      if (return_vals[0].data.d_status == STATUS_SUCCESS)
	{
	  bg[0] = return_vals[1].data.d_color.red;
	  bg[1] = return_vals[1].data.d_color.green;
	  bg[2] = return_vals[1].data.d_color.blue;
	}
      else
	{
	  bg[0] = 0;
	  bg[1] = 0;
	  bg[2] = 0;
	}
      break;
    case GRAYA_IMAGE:
      fg[1] = 255;
      bg[1] = 255;
    case GRAY_IMAGE :
      fg[0] = 255;
      bg[0] = 0;
      break;
    default:
      break;
    }

  for (pr = gimp_pixel_rgns_register (1, &dest_rgn); pr != NULL; pr = gimp_pixel_rgns_process (pr))
    {
      y = dest_rgn.y;

      dest_row = dest_rgn.data;
      for ( row = 0; row < dest_rgn.h; row++)
	{
	  dest = dest_row;
	  x = dest_rgn.x;

	  for (col = 0; col < dest_rgn.w; col++)
	    {
	      gint val, xp, yp;

	      if (cvals.mode) {
		/* Psychobilly Mode */
		val = (inblock(x,cvals.size) == inblock(y,cvals.size)) ? 0 : 1;
	      }
	      else {
		/* Normal, regular checkerboard mode.
		 * Determine base factor (even or odd) of block
		 * this x/y position is in.
		 */
		xp = x/cvals.size;
		yp = y/cvals.size;
		/* if both even or odd, color sqr */
		val = ( (xp&1) == (yp&1) ) ? 0 : 1;
	      }
	      for (bp = 0; bp < dest_rgn.bpp; bp++)
		dest[bp] = val ? fg[bp] : bg[bp];
	      dest += dest_rgn.bpp;
	      x++;
	    }
	  dest_row += dest_rgn.rowstride;
	  y++;
	}
      progress += dest_rgn.w * dest_rgn.h;
      gimp_progress_update ((double) progress / (double) max_progress);
    }
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));
}


static int
inblock( gint pos, gint size)
{
    static gint *in = NULL;		/* initialized first time */
    gint i,j,k, len;

    /* avoid a FP exception */
    if (size == 1)
      size = 2;

    len = size*size;
    /*
     * Initialize the array; since we'll be called thousands of
     * times with the same size value, precompute the array.
     */
    if (in == NULL) {
	in = (int*)malloc(sizeof(int)* len);
	if (in == NULL) {
	    return 0;
	} else {
	    int cell = 1;	/* cell value */
	    /*
	     * i is absolute index into in[]
	     * j is current number of blocks to fill in with a 1 or 0.
	     * k is just counter for the j cells.
	     */
	    i=0;
	    for (j=1; j<=size; j++ ) { /* first half */
		for (k=0; k<j; k++ ) {
		    in[i++] = cell;
		}
		cell = !cell;
	    }
	    for ( j=size-1; j>=1; j--) { /* second half */
		for (k=0; k<j; k++ ) {
		    in[i++] = cell;
		}
		cell = !cell;
	    }
	}
    } /* if n NULL */

    /* place pos within 0..(len-1) grid and return the value. */
    return in[ pos % (len-1) ];
}


static gint
check_dialog ()
{
  GtkWidget *dlg;
  GtkWidget *button;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *toggle;
  GtkWidget *slider;
  GtkObject *size_data;
  gchar **argv;
  gint argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("whirl");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), _("Checkerboard"));
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) check_close_callback,
		      NULL);

  /*  Action area  */
  button = gtk_button_new_with_label (_("OK"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) check_ok_callback,
                      dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Cancel"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  /*  parameter settings  */
  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  table = gtk_table_new (5, 5, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_container_add (GTK_CONTAINER (frame), table);

  toggle = gtk_check_button_new_with_label (_("Psychobilly"));
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 1, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) check_toggle_update,
		      &cvals.mode);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), cvals.mode);
  gtk_widget_show (toggle);

  label = gtk_label_new (_("Check Size"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  size_data = gtk_adjustment_new (cvals.size, 1, 400, 1, 1, 0);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (size_data));
  gtk_widget_set_usize (slider, 300, 0);
  gtk_table_attach (GTK_TABLE (table), slider, 0, 1, 4, 5, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (slider), 0);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (size_data), "value_changed",
		      (GtkSignalFunc) check_slider_update,
		      &cvals.size);
  gtk_widget_show (slider);

  gtk_widget_show (frame);
  gtk_widget_show (table);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return cint.run;
}

static void
check_slider_update (GtkAdjustment *adjustment,
		     gint        *size_val)
{
  *size_val = adjustment->value;
}


static void
check_toggle_update (GtkWidget *widget,
		       gpointer   data)
{
  gint *toggle_val;

  toggle_val = (gint *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}

static void
check_close_callback (GtkWidget *widget,
			 gpointer   data)
{
  gtk_main_quit ();
}

static void
check_ok_callback (GtkWidget *widget,
		      gpointer   data)
{
  cint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}
