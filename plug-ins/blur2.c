/*
 * This is a plug-in for the GIMP.
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 */

/*
 * This filter is like the standard 'blur', except that it uses
 * a convolution kernel of variable size.
 *
 * I am greatly indebted to Johan Klockars <d8klojo@dtek.chalmers.se>
 * for supplying the algorithm used here, which is of complexity O(1).
 * Compared with the original naive algorithm, which is O(k^2) (where
 * k is the kernel size), it gives _massive_ speed improvements.
 * The code is quite simple, too.
 */

#include <stdlib.h>
#include <stdio.h> /**/
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

#define ENTRY_WIDTH     30
#define SCALE_WIDTH     125

typedef struct {
    gdouble mask_size;
} BlurVals;

typedef struct {
  gint run;
} BlurInterface;

/* Declare local functions.
 */
static void query(void);
static void run(char *name,
		int nparams,
		GParam * param,
		int *nreturn_vals,
		GParam ** return_vals);

static void blur(GDrawable * drawable);

static gint      blur2_dialog ();

static void      blur2_close_callback  (GtkWidget *widget,
					gpointer   data);
static void      blur2_ok_callback     (GtkWidget *widget,
					gpointer   data);
static void      blur2_scale_update    (GtkAdjustment *adjustment,
					  double        *scale_val);
static void      blur2_entry_update    (GtkWidget *widget,
					  gdouble *value);
static void      dialog_create_value     (char *title,
					  GtkTable *table,
					  int row,
					  gdouble *value,
					  double left,
					  double right);


GPlugInInfo PLUG_IN_INFO =
{
    NULL,			/* init_proc */
    NULL,			/* quit_proc */
    query,			/* query_proc */
    run,			/* run_proc */
};

static BlurVals bvals =
{
    6+5.0				/*  mask size  */
};

static BlurInterface bint =
{
  FALSE     /*  run  */
};


MAIN()

static void
query()
{
    static GParamDef args[] =
    {
	{PARAM_INT32, "run_mode", "Interactive, non-interactive"},
	{PARAM_IMAGE, "image", "Input image (unused)"},
	{PARAM_DRAWABLE, "drawable", "Input drawable"},
	{PARAM_INT32, "mask_size", "Blur mask size"},
    };
    static GParamDef *return_vals = NULL;
    static int nargs = sizeof(args) / sizeof(args[0]);
    static int nreturn_vals = 0;

    gimp_install_procedure("plug_in_blur2",
			   "Blur the contents of the specified drawable",
			   "This function applies a NxN blurring convolution kernel to the specified drawable.",
			   "Torsten Martinsen",
			   "Torsten Martinsen",
			   "1996-1997",
			   "<Image>/Filters/Blur/Variable Blur",
			   "RGB, GRAY",
			   PROC_PLUG_IN,
			   nargs, nreturn_vals,
			   args, return_vals);
}

static void
run(char *name,
    int nparams,
    GParam * param,
    int *nreturn_vals,
    GParam ** return_vals)
{
    static GParam values[1];
    GDrawable *drawable;
    GRunModeType run_mode;
    GStatusType status = STATUS_SUCCESS;

    run_mode = param[0].data.d_int32;

    switch (run_mode) {
    case RUN_INTERACTIVE:
	/*  Possibly retrieve data  */
	gimp_get_data("plug_in_blur2", &bvals);

	/*  First acquire information with a dialog  */
	if (!blur2_dialog())
	    return;
	break;

    case RUN_NONINTERACTIVE:
	/*  Make sure all the arguments are there!  */
	if (nparams != 4)
	    status = STATUS_CALLING_ERROR;
	if (status == STATUS_SUCCESS) {
	    bvals.mask_size = (gdouble) param[3].data.d_int32;
	}
	if (status == STATUS_SUCCESS &&
	    (bvals.mask_size < 1.0))
	    status = STATUS_CALLING_ERROR;
	break;

    case RUN_WITH_LAST_VALS:
	/*  Possibly retrieve data  */
	gimp_get_data("plug_in_blur2", &bvals);
	break;

    default:
	break;
    }

    /*  Get the specified drawable  */
    drawable = gimp_drawable_get(param[2].data.d_drawable);

    /*  Make sure that the drawable is gray or RGB color  */
    if (gimp_drawable_color(drawable->id) || gimp_drawable_gray(drawable->id)) {
	gimp_progress_init("Variable Blur");
	gimp_tile_cache_ntiles(2 * (drawable->width / gimp_tile_width() + 1));
	blur(drawable);

	if (run_mode != RUN_NONINTERACTIVE)
	    gimp_displays_flush();

	/*  Store data  */
	if (run_mode == RUN_INTERACTIVE)
	    gimp_set_data ("plug_in_blur2", &bvals, sizeof (BlurVals));
    } else {
	/* gimp_message ("blur2: cannot operate on indexed color images"); */
	status = STATUS_EXECUTION_ERROR;
    }

    *nreturn_vals = 1;
    *return_vals = values;

    values[0].type = PARAM_STATUS;
    values[0].data.d_status = status;

    gimp_drawable_detach(drawable);
}

static void
blur(GDrawable * drawable)
{
    GPixelRgn srcPR, destPR;
    gint width, height;
    gint bytes;
    guchar *dest, *d;
    guchar *cr, *cc;
    gint sum[4];
    gint row, col, i, n, div;
    gint x1, y1, x2, y2;

    /* Get the input area. This is the bounding box of the selection in
     *  the image (or the entire image if there is no selection). Only
     *  operating on the input area is simply an optimization. It doesn't
     *  need to be done for correct operation. (It simply makes it go
     *  faster, since fewer pixels need to be operated on).
     */
    gimp_drawable_mask_bounds(drawable->id, &x1, &y1, &x2, &y2);

    /* Get the size of the input image. (This will/must be the same
     *  as the size of the output image).
     */
    width = drawable->width;
    height = drawable->height;
    bytes = drawable->bpp;

    n = bvals.mask_size;

    /* Include edges if possible */
    x1 = MAX(x1 - n / 2, 0);
    x2 = MIN(x2 + n / 2 + 1, width);
    y1 = MAX(y1 - n / 2, 0);
    y2 = MIN(y2 + n / 2 + 1, height);

    /*  initialize the pixel regions  */
    gimp_pixel_rgn_init(&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
    gimp_pixel_rgn_init(&destPR, drawable, 0, 0, width, height, TRUE, TRUE);

    /*  allocate row buffers  */
    cr = (guchar *) malloc((x2 - x1 + 2) * bytes);
    dest = (guchar *) malloc((x2 - x1) * bytes);

    /* loop through the rows, applying horizontal blur  */
    for (row = y1; row < y2; row++) {
	gimp_pixel_rgn_get_row(&srcPR, cr, x1, row, (x2 - x1));

	d = dest + bytes * (n / 2);
	for (i = 0; i < bytes; ++i)
	    sum[i] = 0;
	for (i = 0; i < n * bytes; ++i)
	    sum[i % bytes] += cr[i];

	for (col = 0; col < (x2 - x1 - n) * bytes; col++) {
	    sum[col % bytes] += cr[col + n * bytes] - cr[col];
	    *d++ = sum[col % bytes]/n;
	}

	/*  store the dest  */
	gimp_pixel_rgn_set_row(&destPR, dest, x1, row, (x2 - x1));

	if ((row % 5) == 0)
	    gimp_progress_update((double) (row/2) / (double) (x2 - x1));
    }

    free(cr);
    free(dest);

    cc = (guchar *) malloc((y2 - y1 + 2) * bytes);
    dest = (guchar *) malloc((y2 - y1) * bytes);

    /* loop through the columns, applying vertical blur */
    for (col = x1; col < x2; col++) {
	gimp_pixel_rgn_get_col(&destPR, cc, col, y1, (y2 - y1));

	d = dest + bytes * (n / 2);
	for (i = 0; i < bytes; ++i)
	    sum[i] = 0;
	for (i = 0; i < n * bytes; ++i)
	    sum[i % bytes] += cc[i];

	for (row = 0; row < (y2 - y1 - n) * bytes; row++) {
	    sum[row % bytes] += cc[row + n * bytes] - cc[row];
	    *d++ = sum[row % bytes]/n;
	}

	/*  store the dest  */
	gimp_pixel_rgn_set_col(&destPR, dest, col, y1, (y2 - y1));

	if ((col % 5) == 0)
	    gimp_progress_update(((double) (x2-x1)/2+(col/2)) / (double) (x2-x1));
    }

    div = n/2+1;
    if (y1-n/2 < 0)
	for (col = x1; col < x2; ++col) {
	    gimp_pixel_rgn_get_col(&srcPR, cc, col, 0, div);
	    gimp_pixel_rgn_set_col(&destPR, cc, col, 0, div);
	}
    if (y2+n/2 > height)
	for (col = x1; col < x2; ++col) {
	    gimp_pixel_rgn_get_col(&srcPR, cc, col, height-div, div);
	    gimp_pixel_rgn_set_col(&destPR, cc, col, height-div, div);
	}

    if (x1-n/2 < 0)
	for (row = y1; row < y2; ++row) {
	    gimp_pixel_rgn_get_row(&srcPR, cr, 0, row, div);
	    gimp_pixel_rgn_set_row(&destPR, cr, 0, row, div);
	}
    if (x2+n/2 > width)
	for (row = y1; row < y2; ++row) {
	    gimp_pixel_rgn_get_row(&srcPR, cr, width-div, row, div);
	    gimp_pixel_rgn_set_row(&destPR, cr, width-div, row, div);
	}
    
#if 0
    /* Do remaining top pixels, if any */
    if (y1-n/2 < 0) {
	for (col = x1; col < x2; ++col) {
	    div = n/2+1;
	    gimp_pixel_rgn_get_col(&srcPR, cc, col, 0, div);

	    d = dest;
	    s = cc;
	    for (i = 0; i < bytes; ++i)
		sum[i] = 0;
	    for (i = 0; i < div * bytes; ++i)
		sum[i % bytes] += *s++;

	    memset(dest, 0, (y2 - y1) * bytes);
	    for (i = 0; i < bytes; ++i)
		*d++ = sum[i]/div;

	    for (row = 0; row < n/2; ++row) {
		++div; 
		for (i = 0; i < bytes; ++i) {
 		    sum[i] += *s++;
		    *d++ = sum[i]/div;
		}
	    }

	    gimp_pixel_rgn_set_col(&destPR, dest, col, 0, n/2+1);
	}
    }

    /* Do remaining bottom pixels, if any */
    if (y2+n/2 > height) {
	for (col = x1; col < x2; ++col) {
	    div = n/2+1;
	    gimp_pixel_rgn_get_col(&srcPR, cc, col, height-1-div, div);

	    s = cc+div*bytes-1;
	    d = dest+div*bytes-1;	/* points to last byte */
	    for (i = 0; i < bytes; ++i)
		sum[i] = 0;
	    for (i = 0; i < div * bytes; ++i)
		sum[i % bytes] += *s--;

	    for (i = 0; i < bytes; ++i)
		*d-- = sum[i]/div;
	    for (row = 0; row < n/2; ++row) {
		for (i = 0; i < bytes; ++i) {
		    sum[i] += *s--;
		    *d-- = sum[i]/div;
		}
		++div; 
	    }
	    gimp_pixel_rgn_set_col(&destPR, dest, col, height-1-div, n/2+1);
	}
    }
#endif
    
    free(cc);
    free(dest);

    /*  update the blurred region  */
    gimp_drawable_flush(drawable);
    gimp_drawable_merge_shadow(drawable->id, TRUE);
    gimp_drawable_update(drawable->id, x1, y1, (x2 - x1), (y2 - y1));
}

static gint
blur2_dialog ()
{
  GtkWidget *dlg;
  GtkWidget *button;
  GtkWidget *frame;
  GtkWidget *table;
  gchar **argv;
  gint argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("blur2");

  gtk_init (&argc, &argv);

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "Variable Blur");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) blur2_close_callback,
		      NULL);

  /*  Action area  */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) blur2_ok_callback,
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

  /*  parameter settings  */
  frame = gtk_frame_new ("Parameter Settings");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  table = gtk_table_new (3, 3, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_container_add (GTK_CONTAINER (frame), table);

  dialog_create_value("Mask Size", GTK_TABLE(table), 1, &bvals.mask_size, 3.0, 50.0);

  gtk_widget_show (frame);
  gtk_widget_show (table);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return bint.run;
}


static void
blur2_close_callback (GtkWidget *widget,
		       gpointer   data)
{
  gtk_main_quit ();
}

static void
blur2_ok_callback (GtkWidget *widget,
		    gpointer   data)
{
  bint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
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
					1.0, 5.0, 5.0);

	gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
			   (GtkSignalFunc) blur2_scale_update,
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
	sprintf(buf, "%.0f", *value);
	gtk_entry_set_text(GTK_ENTRY(entry), buf);
	gtk_signal_connect(GTK_OBJECT(entry), "changed",
			   (GtkSignalFunc) blur2_entry_update,
			   value);
	gtk_table_attach(GTK_TABLE(table), entry, 2, 3, row, row + 1, GTK_FILL, GTK_FILL, 4, 0);
	gtk_widget_show(entry);
}

static void
blur2_entry_update(GtkWidget *widget, gdouble *value)
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
blur2_scale_update (GtkAdjustment *adjustment, gdouble *value)
{
	GtkWidget *entry;
	char       buf[256];

	if (*value != adjustment->value) {
		*value = adjustment->value;

		entry = gtk_object_get_user_data(GTK_OBJECT(adjustment));
		sprintf(buf, "%.0f", *value);

		gtk_signal_handler_block_by_data(GTK_OBJECT(entry), value);
		gtk_entry_set_text(GTK_ENTRY(entry), buf);
		gtk_signal_handler_unblock_by_data(GTK_OBJECT(entry), value);
	} /* if */
}
