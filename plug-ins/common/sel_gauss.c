/* Selective gaussian blur filter for the GIMP, version 0.1
 * Adapted from the original gaussian blur filter by Spencer Kimball and
 * Peter Mattis.
 *
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1999 Thom van Os <thom@vanos.com>
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
 *
 * Changelog:
 *
 * v0.1 	990202, TVO
 * 	First release
 *
 * To do:
 *	- support for horizontal or vertical only blur
 *	- use memory more efficiently, smaller regions at a time
 *	- integrating with other convolution matrix based filters ?
 *	- create more selective and adaptive filters
 *	- threading
 *	- optimization
 *
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif /* M_PI */

#define ENTRY_WIDTH 100

typedef struct
{
  gdouble radius;
  gint maxdelta;
} BlurValues;

typedef struct
{
  gint run;
} BlurInterface;


/* Declare local functions.
 */
static void      query  (void);
static void      run    (gchar     *name,
			 gint       nparams,
			 GParam     *param,
			 gint      *nreturn_vals,
			 GParam   **return_vals);

static void      sel_gauss        (GDrawable *drawable,
				   gdouble    radius,
				   gint       maxdelta);

/*
 * Gaussian blur interface
 */
static gint      sel_gauss_dialog (void);

/*
 * Gaussian blur helper functions
 */
static void sel_gauss_close_callback  (GtkWidget *widget, gpointer data);
static void sel_gauss_ok_callback     (GtkWidget *widget, gpointer data);
static void sel_gauss_entry_callback  (GtkWidget *widget, gpointer data);
static void sel_gauss_delta_callback  (GtkWidget *widget, gpointer data);

GPlugInInfo PLUG_IN_INFO =
{
	NULL,	/* init_proc */
	NULL,	/* quit_proc */
	query,	/* query_proc */
	run,	/* run_proc */
};

static BlurValues bvals =
{
	5.0,	/*  radius  */
	50	/* maxdelta */
};

static BlurInterface bint =
{
	FALSE	/* run */
};

MAIN ()

static void query ()
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (unused)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_FLOAT, "radius", "Radius of gaussian blur (in pixels > 1.0)" },
    { PARAM_INT32, "maxdelta", "Maximum delta" },
  };
  static GParamDef *return_vals = NULL;
  static gint nargs = sizeof (args) / sizeof (args[0]);
  static gint nreturn_vals = 0;

  gimp_install_procedure ("plug_in_sel_gauss",
			  "Applies a selective gaussian blur to the specified drawable.",
			  "This filter functions similar to the regular gaussian blur filter except that neighbouring pixels that differ more than the given maxdelta parameter will not be blended with. This way with the correct parameters, an image can be smoothed out without losing details. However, this filter can be rather slow.",
			  "Thom van Os",
			  "Thom van Os",
			  "1999",
			  "<Image>/Filters/Blur/Selective Gaussian Blur",
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void run (
	gchar	*name,
	gint	nparams,
	GParam	*param,
	gint	*nreturn_vals,
	GParam	**return_vals)
{
	static GParam	values[1];
	GRunModeType	run_mode;
	GStatusType	status = STATUS_SUCCESS;
	GDrawable	*drawable;
	gdouble	radius;

	run_mode = param[0].data.d_int32;

	*nreturn_vals = 1;
	*return_vals = values;

	values[0].type = PARAM_STATUS;
	values[0].data.d_status = status;

	switch (run_mode)
	{
	  case RUN_INTERACTIVE:
		/* Possibly retrieve data */
		gimp_get_data ("plug_in_sel_gauss", &bvals);

		/* First acquire information with a dialog */
		if (! sel_gauss_dialog ())
			return;
		break;

	  case RUN_NONINTERACTIVE:
		/* Make sure all the arguments are there! */
		if (nparams != 7)
			status = STATUS_CALLING_ERROR;
		if (status == STATUS_SUCCESS)
		{
			bvals.radius = param[3].data.d_float;
			bvals.maxdelta = (param[4].data.d_int32);
			if (bvals.maxdelta < 0) bvals.maxdelta = 0;
			else if (bvals.maxdelta > 255) bvals.maxdelta = 255;
		}
		if (status == STATUS_SUCCESS && (bvals.radius < 1.0))
			status = STATUS_CALLING_ERROR;
		break;

	  case RUN_WITH_LAST_VALS:
		/* Possibly retrieve data */
		gimp_get_data ("plug_in_sel_gauss", &bvals);
		break;

	  default:
		break;
	}

	if (status != STATUS_SUCCESS) {
		values[0].data.d_status = status;
		return;
	}

	/* Get the specified drawable */
	drawable = gimp_drawable_get (param[2].data.d_drawable);

	/* Make sure that the drawable is gray or RGB color */
	if (gimp_drawable_color (drawable->id) ||
		gimp_drawable_is_gray (drawable->id)) {

		gimp_progress_init ("Selective Gaussian Blur");

		radius = fabs (bvals.radius) + 1.0;

		/* run the gaussian blur */
		sel_gauss (drawable, radius, bvals.maxdelta);

		/* Store data */
		if (run_mode == RUN_INTERACTIVE)
			gimp_set_data ("plug_in_sel_gauss",
				&bvals, sizeof (BlurValues));

		if (run_mode != RUN_NONINTERACTIVE)
			gimp_displays_flush ();
	} else {
		gimp_message ("sel_gauss: cannot operate on indexed color images");
		status = STATUS_EXECUTION_ERROR;
	}

	gimp_drawable_detach (drawable);
	values[0].data.d_status = status;
}

static gint sel_gauss_dialog ()
{
	GtkWidget *dlg;
	GtkWidget *label;
	GtkWidget *entry;
	GtkWidget *button;
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *hbox;
	gchar buffer[12];
	gchar **argv;
	gint argc;

	argc = 1;
	argv = g_new (gchar *, 1);
	argv[0] = g_strdup ("gauss");

	gtk_init (&argc, &argv);
	gtk_rc_parse (gimp_gtkrc ());

	dlg = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (dlg), "Selective Gaussian Blur");
	gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
	gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		(GtkSignalFunc) sel_gauss_close_callback, NULL);

	/* Action area */
	button = gtk_button_new_with_label ("OK");
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
		(GtkSignalFunc) sel_gauss_ok_callback, dlg);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area),
		button, TRUE, TRUE, 0);
	gtk_widget_grab_default (button);
	gtk_widget_show (button);

	button = gtk_button_new_with_label ("Cancel");
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
		(GtkSignalFunc) gtk_widget_destroy, GTK_OBJECT (dlg));
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area),
		button, TRUE, TRUE, 0);
	gtk_widget_show (button);

	/* parameter settings */
	frame = gtk_frame_new ("Parameter Settings");
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
	gtk_container_border_width (GTK_CONTAINER (frame), 10);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame,
		TRUE, TRUE, 0);
	vbox = gtk_vbox_new (FALSE, 5);
	gtk_container_border_width (GTK_CONTAINER (vbox), 10);
	gtk_container_add (GTK_CONTAINER (frame), vbox);

	hbox = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

	label = gtk_label_new ("Blur Radius: ");
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, FALSE, 0);
	gtk_widget_show (label);

	entry = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
	gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
	sprintf (buffer, "%f", bvals.radius);
	gtk_entry_set_text (GTK_ENTRY (entry), buffer);
	gtk_signal_connect (GTK_OBJECT (entry), "changed",
		(GtkSignalFunc) sel_gauss_entry_callback, NULL);
	gtk_widget_show (entry);
	gtk_widget_show (hbox);

	hbox = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

	label = gtk_label_new ("Max. delta: ");
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, FALSE, 0);
	gtk_widget_show (label);

	entry = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
	gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
	sprintf (buffer, "%d", bvals.maxdelta);
	gtk_entry_set_text (GTK_ENTRY (entry), buffer);
	gtk_signal_connect (GTK_OBJECT (entry), "changed",
		(GtkSignalFunc) sel_gauss_delta_callback, NULL);
	gtk_widget_show (entry);
	gtk_widget_show (hbox);

	gtk_widget_show (vbox);
	gtk_widget_show (frame);
	gtk_widget_show (dlg);

	gtk_main ();
	gdk_flush ();

	return bint.run;
}

void init_matrix(gdouble radius, gdouble **mat, gint num)
{
	int dx, dy;
	gdouble sd, c1, c2;

	/* This formula isn't really correct, but it'll do */
	sd = radius / 3.329042969;
	c1 = 1.0 / sqrt(2.0 * M_PI * sd);
	c2 = -2.0 * (sd * sd);

	for (dy=0; dy<num; dy++) {
		for (dx=dy; dx<num; dx++) {
			mat[dx][dy] = c1 * exp(((dx*dx)+ (dy*dy))/ c2);
			mat[dy][dx] = mat[dx][dy];
		}
	}
}

static void matrixmult(guchar *src, guchar *dest, gint width, gint height,
	gdouble **mat, gint numrad, gint bytes, gint has_alpha, gint maxdelta)
{
	gint i, j, b, nb, x, y;
	gint six, dix, tmp;
	gdouble sum, fact, d, alpha=1.0;

	nb = bytes - (has_alpha?1:0);

	for (y = 0; y< height; y++) {
	  for (x = 0; x< width; x++) {
	    dix = (bytes*((width*y)+x));
	    if (has_alpha)
		dest[dix+nb] = src[dix+nb];
	    for (b=0; b<nb; b++) {
		sum = 0.0;
		fact = 0.0;
		for (i= 1-numrad; i<numrad; i++) {
			if (((x+i)< 0) || ((x+i)>= width))
				continue;
			for (j= 1-numrad; j<numrad; j++) {
				if (((y+j)<0)||((y+j)>=height))
					continue;
				six = (bytes*((width*(y+j))+x+i));
				if (has_alpha) {
					if (!src[six+nb])
						continue;
					alpha = (double)src[six+nb] / 255.0;
				}
				tmp = src[dix+b] - src[six+b];
				if ((tmp>maxdelta)||
					(tmp<-maxdelta))
					continue;
				d = mat[ABS(i)][ABS(j)];
				if (has_alpha) {
					d *= alpha;
				}
				sum += d * src[six+b];
				fact += d;
			}
		}
		if (fact == 0.0)
			dest[dix+b] = src[dix+b];
		else
			dest[dix+b] = sum/fact;
	    }
	  }
	  if (!(y%5))
	  	gimp_progress_update((double)y / (double)height);
	}
}

static void sel_gauss (GDrawable *drawable, gdouble radius, gint maxdelta)
{
	GPixelRgn src_rgn, dest_rgn;
	gint width, height;
	gint bytes;
	gint has_alpha;
	guchar *dest;
	guchar *src;
	gint x1, y1, x2, y2;
	gint i;
	gdouble **mat;
	gint numrad;

	gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);

	width = (x2 - x1);
	height = (y2 - y1);
	bytes = drawable->bpp;
	has_alpha = gimp_drawable_has_alpha(drawable->id);

	if ((width<1)||(height<1)||(bytes<1))
		return;

	numrad = (gint)(radius + 1.0);
	mat = (gdouble **) g_malloc (numrad * sizeof(gdouble *));
	for (i=0; i<numrad; i++)
		mat[i] = (gdouble *) g_malloc (numrad * sizeof(gdouble));
  	init_matrix(radius, mat, numrad);

	src = (guchar *) g_malloc (width * height * bytes);
	dest = (guchar *) g_malloc (width * height * bytes);

	gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, drawable->width,
		drawable->height, FALSE, FALSE);

	gimp_pixel_rgn_get_rect (&src_rgn, src, x1, y1, width, height);

	matrixmult(src, dest, width, height, mat, numrad,
		bytes, has_alpha, maxdelta);

	gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1, width, height,
		TRUE, TRUE);
	gimp_pixel_rgn_set_rect (&dest_rgn, dest, x1, y1, width, height);

	/*  merge the shadow, update the drawable  */
	gimp_drawable_flush (drawable);
	gimp_drawable_merge_shadow (drawable->id, TRUE);
	gimp_drawable_update (drawable->id, x1, y1, width, height);

	/* free up buffers */
	g_free (src);
	g_free (dest);
	for (i=0; i<numrad; i++)
		g_free (mat[i]);
	g_free (mat);
}

/*  Gauss interface functions  */

static void sel_gauss_close_callback (GtkWidget *widget, gpointer data)
{
	gtk_main_quit ();
}

static void sel_gauss_ok_callback(GtkWidget *widget, gpointer data)
{
	bint.run = TRUE;
	gtk_widget_destroy (GTK_WIDGET (data));
}

static void sel_gauss_entry_callback(GtkWidget *widget, gpointer data)
{
	bvals.radius = atof(gtk_entry_get_text(GTK_ENTRY(widget)));
	if (bvals.radius < 1.0)
		bvals.radius = 1.0;
}

static void sel_gauss_delta_callback(GtkWidget *widget, gpointer data)
{
	bvals.maxdelta = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));
	if (bvals.maxdelta < 0)
		bvals.maxdelta = 0;
	if (bvals.maxdelta > 255)
		bvals.maxdelta = 255;
}
