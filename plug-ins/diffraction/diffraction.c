/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Diffraction plug-in --- Generate diffraction patterns
 * Copyright (C) 1997 Federico Mena Quintero and David Bleecker
 * federico@nuclecu.unam.mx
 * bleecker@math.hawaii.edu
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


#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include "gtk/gtk.h"
#include "libgimp/gimp.h"


/***** Magic numbers *****/
#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif /* M_PI */

#define ITERATIONS   100      /* 100 */
#define WEIRD_FACTOR 0.04     /* 0.04 */

#define PREVIEW_WIDTH   64
#define PREVIEW_HEIGHT  64
#define PROGRESS_WIDTH  32
#define PROGRESS_HEIGHT 16
#define SCALE_WIDTH     150


/***** Types *****/

typedef struct {
	gdouble lam_r;
	gdouble lam_g;
	gdouble lam_b;
	gdouble contour_r;
	gdouble contour_g;
	gdouble contour_b;
	gdouble edges_r;
	gdouble edges_g;
	gdouble edges_b;
	gdouble brightness;
	gdouble scattering;
	gdouble polarization;
} diffraction_vals_t;

typedef struct {
	GtkWidget *preview;
	GtkWidget *progress;
	guchar     preview_row[PREVIEW_WIDTH * 3];

	gint run;
} diffraction_interface_t;


/***** Prototypes *****/

static void query(void);
static void run(char    *name,
		int      nparams,
		GParam  *param,
		int     *nreturn_vals,
		GParam **return_vals);

static void diffraction(GDrawable *drawable);

static void   diff_init_luts(void);
static void   diff_diffract(double x, double y, double *r, double *g, double *b);
static double diff_point(double x, double y, double edges, double contours, double lam);
static double diff_intensity(double x, double y, double lam);

static gint diffraction_dialog(void);
static void dialog_update_preview(void);
static void dialog_create_value(char *title, GtkTable *table, int row, gdouble *value, double left, double right);
static void dialog_update_callback(GtkWidget *widget, gpointer data);
static void dialog_scale_update(GtkAdjustment *adjustment, gdouble *value);
static void dialog_close_callback(GtkWidget *widget, gpointer data);
static void dialog_ok_callback(GtkWidget *widget, gpointer data);
static void dialog_cancel_callback(GtkWidget *widget, gpointer data);


/***** Variables *****/

GPlugInInfo PLUG_IN_INFO = {
	NULL,  	 /* init_proc */
	NULL,  	 /* quit_proc */
	query, 	 /* query_proc */
	run    	 /* run_proc */
}; /* PLUG_IN_INFO */

static diffraction_vals_t dvals = {
	 0.815,   /* lam_r */
	 1.221,   /* lam_g */
	 1.123,   /* lam_b */
	 0.821,   /* contour_r */
	 0.821,   /* contour_g */
	 0.974,   /* contour_b */
	 0.610,   /* edges_r */
	 0.677,   /* edges_g */
	 0.636,   /* edges_b */
	 0.066,   /* brightness */
	37.126,   /* scattering */
	-0.473    /* polarization */
}; /* dvals */

static diffraction_interface_t dint = {
	NULL,  /* preview */
	NULL,  /* progress */
	{ 0 }, /* preview_row */
	FALSE  /* run */
}; /* dint */

static double cos_lut[ITERATIONS + 1];
static double param_lut1[ITERATIONS + 1];
static double param_lut2[ITERATIONS + 1];


/***** Functions *****/

/*****/

MAIN()


/*****/

static void
query(void)
{
	static GParamDef args[] = {
		{ PARAM_INT32,    "run_mode",     "Interactive, non-interactive" },
		{ PARAM_IMAGE,    "image",        "Input image" },
		{ PARAM_DRAWABLE, "drawable",     "Input drawable" },
		{ PARAM_FLOAT,    "lam_r",        "Light frequency (red)" },
		{ PARAM_FLOAT, 	  "lam_g",        "Light frequency (green)" },
		{ PARAM_FLOAT, 	  "lam_b",        "Light frequency (blue)" },
		{ PARAM_FLOAT, 	  "contour_r",    "Number of contours (red)" },
		{ PARAM_FLOAT, 	  "contour_g",    "Number of contours (green)" },
		{ PARAM_FLOAT, 	  "contour_b",    "Number of contours (blue)" },
		{ PARAM_FLOAT, 	  "edges_r",      "Number of sharp edges (red)" },
		{ PARAM_FLOAT, 	  "edges_g",      "Number of sharp edges (green)" },
		{ PARAM_FLOAT, 	  "edges_b",      "Number of sharp edges (blue)" },
		{ PARAM_FLOAT, 	  "brightness",   "Brightness and shifting/fattening of contours" },
		{ PARAM_FLOAT, 	  "scattering",   "Scattering (Speed vs. quality)" },
		{ PARAM_FLOAT, 	  "polarization", "Polarization" }
	}; /* args */

	static GParamDef *return_vals = NULL;
	static int        nargs = sizeof(args) / sizeof(args[0]);
	static int        nreturn_vals = 0;

	gimp_install_procedure("plug_in_diffraction",
			       "Generate diffraction patterns",
			       "Help?  What help?  Real men do not need help :-)",  /* FIXME */
			       "Federico Mena Quintero",
			       "Federico Mena Quintero & David Bleecker",
			       "April 1997, 0.5",
			       "<Image>/Filters/Render/Diffraction Patterns",
			       "RGB*",
			       PROC_PLUG_IN,
			       nargs,
			       nreturn_vals,
			       args,
			       return_vals);
} /* query */


/*****/

static void
run(char    *name,
    int      nparams,
    GParam  *param,
    int     *nreturn_vals,
    GParam **return_vals)
{
	static GParam values[1];

	GDrawable    *active_drawable;
	GRunModeType  run_mode;
	GStatusType   status;

	/* Initialize */

	diff_init_luts();

	status   = STATUS_SUCCESS;
	run_mode = param[0].data.d_int32;

	values[0].type          = PARAM_STATUS;
	values[0].data.d_status = status;

	*nreturn_vals = 1;
	*return_vals  = values;

	switch (run_mode) {
		case RUN_INTERACTIVE:
			/* Possibly retrieve data */

			gimp_get_data("plug_in_diffraction", &dvals);

			/* Get information from the dialog */

			if (!diffraction_dialog())
				return;

			break;

		case RUN_NONINTERACTIVE:
			/* Make sure all the arguments are present */

			if (nparams != 15)
				status = STATUS_CALLING_ERROR;

			if (status == STATUS_SUCCESS) {
				dvals.lam_r 	   = param[3].data.d_float;
				dvals.lam_g 	   = param[4].data.d_float;
				dvals.lam_b        = param[5].data.d_float;
				dvals.contour_r    = param[6].data.d_float;
				dvals.contour_g    = param[7].data.d_float;
				dvals.contour_b    = param[8].data.d_float;
				dvals.edges_r      = param[9].data.d_float;
				dvals.edges_g      = param[10].data.d_float;
				dvals.edges_b      = param[11].data.d_float;
				dvals.brightness   = param[12].data.d_float;
				dvals.scattering   = param[13].data.d_float;
				dvals.polarization = param[14].data.d_float;
			} /* if */

			break;

		case RUN_WITH_LAST_VALS:
			/* Possibly retrieve data */

			gimp_get_data("plug_in_diffraction", &dvals);
			break;

		default:
			break;
	} /* switch */

	/* Get the active drawable */

	active_drawable = gimp_drawable_get(param[2].data.d_drawable);

	/* Create the diffraction pattern */

	if ((status == STATUS_SUCCESS) && gimp_drawable_color(active_drawable->id)) {
		/* Set the tile cache size */

		gimp_tile_cache_ntiles((active_drawable->width + gimp_tile_width() - 1) / gimp_tile_width());

		/* Run! */

		diffraction(active_drawable);

		/* If run mode is interactive, flush displays */

		if (run_mode != RUN_NONINTERACTIVE)
			gimp_displays_flush();

		/* Store data */

		if (run_mode == RUN_INTERACTIVE)
			gimp_set_data("plug_in_diffraction", &dvals, sizeof(diffraction_vals_t));
	} else if (status == STATUS_SUCCESS)
		status = STATUS_EXECUTION_ERROR;

	values[0].data.d_status = status;

	gimp_drawable_detach(active_drawable);
} /* run */


/*****/

static void
diffraction(GDrawable *drawable)
{
	GPixelRgn dest_rgn;
	gpointer  pr;
	gint      x1, y1, x2, y2;
	gint      width, height;
	gint      has_alpha;
	gint      row, col;
	guchar   *dest_row;
	guchar   *dest;
	gint      progress, max_progress;
	double    left, right, bottom, top;
	double    dhoriz, dvert;
	double    px, py;
	double    r, g, b;

	/* Get the mask bounds and image size */

	gimp_drawable_mask_bounds(drawable->id, &x1, &y1, &x2, &y2);

	width  = x2 - x1;
	height = y2 - y1;

	has_alpha = gimp_drawable_has_alpha(drawable->id);

	/* Initialize pixel regions */

	gimp_pixel_rgn_init(&dest_rgn, drawable, x1, y1, width, height, TRUE, TRUE);

	progress     = 0;
	max_progress = width * height;

	gimp_progress_init("Creating diffraction pattern...");

	/* Create diffraction pattern */

	left   = -5.0;
	right  =  5.0;
	bottom = -5.0;
	top    =  5.0;

	dhoriz = (right - left) / (width - 1);
	dvert  = (bottom - top) / (height - 1);

	for (pr = gimp_pixel_rgns_register(1, &dest_rgn);
	     pr != NULL; pr = gimp_pixel_rgns_process(pr)) {
		dest_row = dest_rgn.data;

		py = top + dvert * (dest_rgn.y - y1);

		for (row = 0; row < dest_rgn.h; row++) {
			dest = dest_row;

			px = left + dhoriz * (dest_rgn.x - x1);

			for (col = 0; col < dest_rgn.w; col++) {
				diff_diffract(px, py, &r, &g, &b);

				*dest++ = 255.0 * r;
				*dest++ = 255.0 * g;
				*dest++ = 255.0 * b;

				if (has_alpha)
					*dest++ = 255;

				px += dhoriz;
			} /* for */

			/* Update progress */

			progress += dest_rgn.w;
			gimp_progress_update((double) progress / max_progress);

			py       += dvert;
			dest_row += dest_rgn.rowstride;
		} /* for */
	} /* for */

	gimp_drawable_flush(drawable);
	gimp_drawable_merge_shadow(drawable->id, TRUE);
	gimp_drawable_update(drawable->id, x1, y1, width, height);
} /* diffraction */


static void
diff_init_luts(void)
{
	int    i;
	double a;
	double sina;

	a = -M_PI;

	for (i = 0; i <= ITERATIONS; i++) {
		sina = sin(a);

		cos_lut[i]    = cos(a);

		param_lut1[i] = 0.75 * sina;
		param_lut2[i] = 0.5 * (4.0 * cos_lut[i] * cos_lut[i] + sina * sina);

		a += (2.0 * M_PI / ITERATIONS);
	} /* for */
} /* diff_init_luts */


/*****/

static void
diff_diffract(double x, double y, double *r, double *g, double *b)
{
	*r = diff_point(x, y, dvals.edges_r, dvals.contour_r, dvals.lam_r);
	*g = diff_point(x, y, dvals.edges_g, dvals.contour_g, dvals.lam_g);
	*b = diff_point(x, y, dvals.edges_b, dvals.contour_b, dvals.lam_b);
} /* diff_diffract */


/*****/

static double
diff_point(double x, double y, double edges, double contours, double lam)
{
	return fabs(edges * sin(contours * atan(dvals.brightness * diff_intensity(x, y, lam))));
} /* diff_point */


/*****/

static double
diff_intensity(double x, double y, double lam)
{
	int    i;
	double cxy, sxy;
	double param;
	double polpi2;
	double cospolpi2, sinpolpi2;

	cxy = 0.0;
	sxy = 0.0;

	lam *= 4.0;

	for (i = 0; i <= ITERATIONS; i++) {
		param = lam *
			(cos_lut[i] * x +
			 param_lut1[i] * y -
			 param_lut2[i]);

		cxy += cos(param);
		sxy += sin(param);
	} /* for */

	cxy *= WEIRD_FACTOR;
	sxy *= WEIRD_FACTOR;

	polpi2 = dvals.polarization * (M_PI / 2.0);

	cospolpi2 = cos(polpi2);
	sinpolpi2 = sin(polpi2);

	return dvals.scattering * ((cospolpi2 + sinpolpi2) * cxy * cxy +
				   (cospolpi2 - sinpolpi2) * sxy * sxy);
} /* diff_intensity */

#if 0
/*****/

static double
diff_intensity(double x, double y, double lam)
{
	int    i;
	double cxy, sxy;
	double s;
	double cosa, sina;
	double twocosa, param;
	double polpi2;
	double cospolpi2, sinpolpi2;

	s   = dvals.scattering;
	cxy = 0.0;
	sxy = 0.0;

	for (i = 0; i <= ITERATIONS; i++) {
		cosa = cos_lut[i];
		sina = sin_lut[i];

		twocosa = 2.0 * cosa;

		param = 4.0 * lam *
			(cosa * x +
			 0.75 * sina * y -
			 0.5 * (twocosa * twocosa + sina * sina));

		cxy += 0.04 * cos(param);
		sxy += 0.04 * sin(param);
	} /* for */

	polpi2 = dvals.polarization * (M_PI / 2.0);

	cospolpi2 = cos(polpi2);
	sinpolpi2 = sin(polpi2);

	return s * ((cospolpi2 + sinpolpi2) * cxy * cxy +
		    (cospolpi2 - sinpolpi2) * sxy * sxy);
} /* diff_intensity */
#endif


/*****/

static gint
diffraction_dialog(void)
{
	GtkWidget  *dialog;
	GtkWidget  *top_table;
	GtkWidget  *notebook;
	GtkWidget  *frame;
	GtkWidget  *vbox;
	GtkWidget  *table;
	GtkWidget  *label;
	GtkWidget  *button;
	gint        argc;
	gchar     **argv;
	guchar     *color_cube;

#if 0
	printf("Waiting... (pid %d)\n", getpid());
	kill(getpid(), SIGSTOP);
#endif

	argc    = 1;
	argv    = g_new(gchar *, 1);
	argv[0] = g_strdup("diffraction");

	gtk_init(&argc, &argv);
	gtk_rc_parse (gimp_gtkrc ());

	gdk_set_use_xshm (gimp_use_xshm ());

	gtk_preview_set_gamma(gimp_gamma());
	gtk_preview_set_install_cmap(gimp_install_cmap());
	color_cube = gimp_color_cube();
	gtk_preview_set_color_cube(color_cube[0], color_cube[1], color_cube[2], color_cube[3]);

	gtk_widget_set_default_visual(gtk_preview_get_visual());
	gtk_widget_set_default_colormap(gtk_preview_get_cmap());

	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), "Diffraction patterns");
	gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
	gtk_container_border_width(GTK_CONTAINER(dialog), 0);
	gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
			   (GtkSignalFunc) dialog_close_callback,
			   NULL);

	top_table = gtk_table_new(2, 2, FALSE);
	gtk_container_border_width(GTK_CONTAINER(top_table), 6);
	gtk_table_set_row_spacings(GTK_TABLE(top_table), 4);
	gtk_table_set_col_spacings(GTK_TABLE(top_table), 4);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), top_table, FALSE, FALSE, 0);
	gtk_widget_show(top_table);

	/* Preview */

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(vbox), 0);
	gtk_table_attach(GTK_TABLE(top_table), vbox, 0, 1, 0, 1,
			 GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_show(vbox);

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);

	dint.preview = gtk_preview_new(GTK_PREVIEW_COLOR);
	gtk_preview_size(GTK_PREVIEW(dint.preview), PREVIEW_WIDTH, PREVIEW_HEIGHT);
	gtk_container_add(GTK_CONTAINER(frame), dint.preview);
	gtk_widget_show(dint.preview);

	dint.progress = gtk_progress_bar_new();
	gtk_widget_set_usize(dint.progress, PROGRESS_WIDTH, PROGRESS_HEIGHT);
	gtk_box_pack_start(GTK_BOX(vbox), dint.progress, TRUE, FALSE, 0);
	gtk_widget_show(dint.progress);

	button = gtk_button_new_with_label("Preview!");
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   (GtkSignalFunc) dialog_update_callback,
			   NULL);
	gtk_table_attach(GTK_TABLE(top_table), button, 0, 1, 1, 2,
			 GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_show(button);

	/* Notebook */

	notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
	gtk_table_attach(GTK_TABLE(top_table), notebook, 1, 2, 0, 2, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	gtk_widget_show(notebook);

	/* Frequencies tab */

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(vbox), 4);

	table = gtk_table_new(3, 2, FALSE);
	gtk_container_border_width(GTK_CONTAINER(table), 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);
	gtk_widget_show(table);

	dialog_create_value("Red",   GTK_TABLE(table), 0, &dvals.lam_r, 0.0, 20.0);
	dialog_create_value("Green", GTK_TABLE(table), 1, &dvals.lam_g, 0.0, 20.0);
	dialog_create_value("Blue",  GTK_TABLE(table), 2, &dvals.lam_b, 0.0, 20.0);

	label = gtk_label_new("Frequencies");
	gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);
	gtk_widget_show(vbox);

	/* Contours tab */

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(vbox), 4);

	table = gtk_table_new(3, 2, FALSE);
	gtk_container_border_width(GTK_CONTAINER(table), 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);
	gtk_widget_show(table);

	dialog_create_value("Red",   GTK_TABLE(table), 0, &dvals.contour_r, 0.0, 10.0);
	dialog_create_value("Green", GTK_TABLE(table), 1, &dvals.contour_g, 0.0, 10.0);
	dialog_create_value("Blue",  GTK_TABLE(table), 2, &dvals.contour_b, 0.0, 10.0);

	label = gtk_label_new("Contours");
	gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);
	gtk_widget_show(vbox);

	/* Sharp edges tab */

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(vbox), 4);

	table = gtk_table_new(3, 2, FALSE);
	gtk_container_border_width(GTK_CONTAINER(table), 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);
	gtk_widget_show(table);

	dialog_create_value("Red",   GTK_TABLE(table), 0, &dvals.edges_r, 0.0, 1.0);
	dialog_create_value("Green", GTK_TABLE(table), 1, &dvals.edges_g, 0.0, 1.0);
	dialog_create_value("Blue",  GTK_TABLE(table), 2, &dvals.edges_b, 0.0, 1.0);

	label = gtk_label_new("Sharp edges");
	gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);
	gtk_widget_show(vbox);

	/* Other options tab */

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(vbox), 4);

	table = gtk_table_new(3, 2, FALSE);
	gtk_container_border_width(GTK_CONTAINER(table), 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);
	gtk_widget_show(table);

	dialog_create_value("Brightness",   GTK_TABLE(table), 0, &dvals.brightness, 0.0, 1.0);
	dialog_create_value("Scattering",   GTK_TABLE(table), 1, &dvals.scattering, 0.0, 100.0);
	dialog_create_value("Polarization", GTK_TABLE(table), 2, &dvals.polarization, -1.0, 1.0);

	label = gtk_label_new("Other options");
	gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);
	gtk_widget_show(vbox);

	/* Buttons */

	gtk_container_border_width(GTK_CONTAINER(GTK_DIALOG(dialog)->action_area), 6);

	button = gtk_button_new_with_label("OK");
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   (GtkSignalFunc) dialog_ok_callback,
			   dialog);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
	gtk_widget_grab_default(button);
	gtk_widget_show(button);

	button = gtk_button_new_with_label("Cancel");
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   (GtkSignalFunc) dialog_cancel_callback,
			   dialog);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
	gtk_widget_show(button);

	/* Done */

	gtk_widget_show(dialog);
	dialog_update_preview();

	gtk_main();
	gdk_flush();

	return dint.run;
} /* diffraction_dialog */


/*****/

static void
dialog_update_preview(void)
{
	double  left, right, bottom, top;
	double  px, py;
	double  dx, dy;
	double  r, g, b;
	int     x, y;
	guchar *p;

	left   = -5.0;
	right  =  5.0;
	bottom = -5.0;
	top    =  5.0;

	dx = (right - left) / (PREVIEW_WIDTH - 1);
	dy = (bottom - top) / (PREVIEW_HEIGHT - 1);

	py = top;

	for (y = 0; y < PREVIEW_HEIGHT; y++) {
		px = left;
		p  = dint.preview_row;

		for (x = 0; x < PREVIEW_WIDTH; x++) {
			diff_diffract(px, py, &r, &g, &b);

			*p++ = 255.0 * r;
			*p++ = 255.0 * g;
			*p++ = 255.0 * b;

			px += dx;
		} /* for */

		gtk_preview_draw_row(GTK_PREVIEW(dint.preview), dint.preview_row, 0, y, PREVIEW_WIDTH);

		gtk_progress_bar_update(GTK_PROGRESS_BAR(dint.progress), (double) y / (PREVIEW_HEIGHT - 1));

		py += dy;
	} /* for */

	gtk_widget_draw(dint.preview, NULL);
	gtk_progress_bar_update(GTK_PROGRESS_BAR(dint.progress), 0.0);
	gdk_flush();
} /* dialog_update_preview */


/*****/

static void
dialog_create_value(char *title, GtkTable *table, int row, gdouble *value, double left, double right)
{
	GtkWidget *label;
	GtkWidget *scale;
	GtkObject *scale_data;

	label = gtk_label_new(title);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
	gtk_table_attach(table, label, 0, 1, row, row + 1, GTK_FILL, GTK_FILL, 4, 0);
	gtk_widget_show(label);

	scale_data = gtk_adjustment_new(*value, left, right,
					(right - left) / 50.0,
					(right - left) / 100.0,
					0.0);

	scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
	gtk_widget_set_usize(scale, SCALE_WIDTH, 0);
	gtk_table_attach(table, scale, 1, 2, row, row + 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_TOP);
	gtk_scale_set_digits(GTK_SCALE(scale), 3);
	gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
	gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
			   (GtkSignalFunc) dialog_scale_update,
			   value);

	gtk_widget_show(scale);
} /* dialog_create_value */


/*****/

static void
dialog_update_callback(GtkWidget *widget, gpointer data)
{
	dialog_update_preview();
} /* dialog_update_callback */


/*****/

static void
dialog_scale_update(GtkAdjustment *adjustment, gdouble *value)
{
	*value = adjustment->value;
} /* dialog_scale_update */


/*****/

static void
dialog_close_callback(GtkWidget *widget, gpointer data)
{
	gtk_main_quit();
} /* dialog_close_callback */


/*****/

static void
dialog_ok_callback(GtkWidget *widget, gpointer data)
{
	dint.run = TRUE;
	gtk_widget_destroy(GTK_WIDGET(data));
} /* dialog_ok_callback */


/*****/

static void
dialog_cancel_callback(GtkWidget *widget, gpointer data)
{
	gtk_widget_destroy(GTK_WIDGET(data));
} /* dialog_cancel_callback */
