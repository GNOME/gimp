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
static void query(void);
static void run(char *name,
								int nparams,
								GParam * param,
								int *nreturn_vals,
								GParam ** return_vals);
static gint dialog();

static void doit(GDrawable * drawable);

GPlugInInfo PLUG_IN_INFO =
{
	NULL,													/* init_proc */
	NULL,													/* quit_proc */
	query,												/* query_proc */
	run,													/* run_proc */
};

gint bytes;
gint sx1, sy1, sx2, sy2;
int run_flag = 0;

typedef struct {
	gint min_width, max_width;
	gint min_height, max_height;
	float density;
} config;

config my_config =
{
	10, 30,												/* min_width, max_width */
	10, 30,												/* min_height, max_height */
	4															/* density */
};


MAIN()

static void query()
{
	static GParamDef args[] =
	{
		{PARAM_INT32, "run_mode", "Interactive, non-interactive"},
		{PARAM_IMAGE, "image", "Input image (unused)"},
		{PARAM_DRAWABLE, "drawable", "Input drawable"},
		{PARAM_FLOAT, "density", "Density"},
		{PARAM_INT32, "min_width", "Min. width"},
		{PARAM_INT32, "max_width", "Max. width"},
		{PARAM_INT32, "min_height", "Min. height"},
		{PARAM_INT32, "max_height", "Max. height"},
	};
	static GParamDef *return_vals = NULL;
	static int nargs = sizeof(args) / sizeof(args[0]);
	static int nreturn_vals = 0;

	gimp_install_procedure("plug_in_figures",
			       "Draws lots of rectangles.",
			       "Can be nice to use as \"textures\".",
			       "Tim Newsome",
			       "Tim Newsome",
			       "1997",
			       "<Image>/Filters/Render/Figures",
			       "RGB*, GRAY*",
			       PROC_PLUG_IN,
			       nargs, nreturn_vals,
			       args, return_vals);
}

static void run(char *name, int n_params, GParam * param, int *nreturn_vals,
								GParam ** return_vals)
{
	static GParam values[1];
	GDrawable *drawable;
	GRunModeType run_mode;
	GStatusType status = STATUS_SUCCESS;

	*nreturn_vals = 1;
	*return_vals = values;

	run_mode = param[0].data.d_int32;

	if (run_mode == RUN_NONINTERACTIVE) {
		if (n_params != 8) {
			status = STATUS_CALLING_ERROR;
		} else {
			my_config.density = param[3].data.d_float;
			my_config.min_width = param[4].data.d_int32;
			my_config.max_width = param[5].data.d_int32;
			my_config.min_height = param[6].data.d_int32;
			my_config.max_height = param[7].data.d_int32;
		}
	} else {
		/*  Possibly retrieve data  */
		gimp_get_data("plug_in_figures", &my_config);

		if (run_mode == RUN_INTERACTIVE) {
			/* Oh boy. We get to do a dialog box, because we can't really expect the
			 * user to set us up with the right values using gdb.
			 */
			if (!dialog()) {
				/* The dialog was closed, or something similarly evil happened. */
				status = STATUS_EXECUTION_ERROR;
			}
		}
	}

	if (status == STATUS_SUCCESS) {
		/*  Get the specified drawable  */
		drawable = gimp_drawable_get(param[2].data.d_drawable);

		/*  Make sure that the drawable is gray or RGB color  */
		if (gimp_drawable_color(drawable->id) || gimp_drawable_gray(drawable->id)) {
			gimp_progress_init("Drawing Figures...");
			gimp_tile_cache_ntiles(2 * (drawable->width / gimp_tile_width() + 1));

			srand(time(NULL));
			doit(drawable);

			if (run_mode != RUN_NONINTERACTIVE)
			        gimp_displays_flush();

			if (run_mode == RUN_INTERACTIVE)
				gimp_set_data("plug_in_figures", &my_config, sizeof(my_config));
		} else {
			status = STATUS_EXECUTION_ERROR;
		}
		gimp_drawable_detach(drawable);
	}

	values[0].type = PARAM_STATUS;
	values[0].data.d_status = status;
}

/*
 * Draws a rectangle in region, using tmp as tempspace, with center x, y,
 * width w, height h, with color color, and opaqueness a.
 */
static void draw_rectangle(guchar * tmp, GPixelRgn * region, int x, int y,
													 int w, int h, guchar color[], float a)
{
	int i, j, k;
	int rx1, rx2, ry1, ry2;

	rx1 = x - w / 2;
	rx2 = rx1 + w;
	ry1 = y - h / 2;
	ry2 = ry1 + h;

	rx1 = rx1 < sx1 ? sx1 : rx1;
	ry1 = ry1 < sy1 ? sy1 : ry1;
	rx2 = rx2 > sx2 ? sx2 : rx2;
	ry2 = ry2 > sy2 ? sy2 : ry2;

	gimp_pixel_rgn_get_rect(region, tmp, rx1, ry1, rx2 - rx1, ry2 - ry1);
	for (j = 0; j < (rx2 - rx1); j++) {
		for (k = 0; k < (ry2 - ry1); k++) {
			for (i = 0; i < bytes; i++) {
				tmp[(j + (rx2 - rx1) * k) * bytes + i] =
					(float) tmp[(j + (rx2 - rx1) * k) * bytes + i] * (1 - a) +
					(float) color[i] * a;
			}
		}
	}
	gimp_pixel_rgn_set_rect(region, tmp, rx1, ry1, rx2 - rx1, ry2 - ry1);
}

static void doit(GDrawable * drawable)
{
	GPixelRgn srcPR, destPR;
	gint width, height;
	int x, y, w, h;
	int i, j;
	guchar *tmp, *copybuf;
	guchar color[4];
	int objects;

	/* Get the input area. This is the bounding box of the selection in
	 *  the image (or the entire image if there is no selection). Only
	 *  operating on the input area is simply an optimization. It doesn't
	 *  need to be done for correct operation. (It simply makes it go
	 *  faster, since fewer pixels need to be operated on).
	 */
	gimp_drawable_mask_bounds(drawable->id, &sx1, &sy1, &sx2, &sy2);

	/* Get the size of the input image. (This will/must be the same
	 *  as the size of the output image.
	 */
	width = drawable->width;
	height = drawable->height;
	bytes = drawable->bpp;

	tmp = (guchar *) malloc(my_config.max_width * my_config.max_height * bytes);
	if (tmp == NULL) {
		return;
	}
	/*  initialize the pixel regions  */
	gimp_pixel_rgn_init(&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
	gimp_pixel_rgn_init(&destPR, drawable, 0, 0, width, height, TRUE, TRUE);

	/* First off, copy the old one to the new one. */
	copybuf = malloc(width * bytes);
	for (i = sy1; i < sy2; i++) {
		gimp_pixel_rgn_get_row(&srcPR, copybuf, sx1, i, width);
		gimp_pixel_rgn_set_row(&destPR, copybuf, sx1, i, width);
	}
	free(copybuf);

	objects = my_config.density * (float) (width * height) /
		(float) (((my_config.min_width + my_config.max_width) / 2) *
						 ((my_config.min_height + my_config.max_height)) / 2);

	for (i = 0; i < objects; i++) {
		w = my_config.min_width + rand() * (double) (my_config.max_width -
																	 my_config.min_width) / (double) RAND_MAX;
		h = my_config.min_height + rand() * (double) (my_config.max_height -
																	my_config.min_height) / (double) RAND_MAX;
		x = sx1 + rand() * (double) width / (double) RAND_MAX;
		y = sy1 + rand() * (double) height / (double) RAND_MAX;
		for (j = 0; j < bytes; j++) {
			color[j] = rand() * 255. / (double) RAND_MAX;
		}
		draw_rectangle(tmp, &destPR, x, y, w, h, color,
									 rand() / (double) RAND_MAX);
		if (!(i % 64)) {
			gimp_progress_update((double) i / (double) objects);
		}
	}

	/*  update the timred region  */
	gimp_drawable_flush(drawable);
	gimp_drawable_merge_shadow(drawable->id, TRUE);
	gimp_drawable_update(drawable->id, sx1, sy1, sx2 - sx1, sy2 - sy1);
}

/***************************************************
 * GUI stuff
 */

static void close_callback(GtkWidget * widget, gpointer data)
{
	gtk_main_quit();
}

static void ok_callback(GtkWidget * widget, gpointer data)
{
	run_flag = 1;
	gtk_widget_destroy(GTK_WIDGET(data));
}

static void entry_callback(GtkWidget * widget, gpointer data)
{
	if (data == &my_config.density)
		my_config.density = atof(gtk_entry_get_text(GTK_ENTRY(widget)));
	else if (data == &my_config.min_width)
		my_config.min_width = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));
	else if (data == &my_config.max_width)
		my_config.max_width = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));
	else if (data == &my_config.min_height)
		my_config.min_height = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));
	else if (data == &my_config.max_height)
		my_config.max_height = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));
}

static gint dialog()
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
	argv = g_new(gchar *, 1);
	argv[0] = g_strdup("plasma");

	gtk_init(&argc, &argv);
	gtk_rc_parse (gimp_gtkrc ());

	dlg = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dlg), "Figures");
	gtk_window_position(GTK_WINDOW(dlg), GTK_WIN_POS_MOUSE);
	gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
										 (GtkSignalFunc) close_callback, NULL);

	/*  Action area  */
	button = gtk_button_new_with_label("OK");
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
										 (GtkSignalFunc) ok_callback,
										 dlg);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area), button, TRUE, TRUE, 0);
	gtk_widget_grab_default(button);
	gtk_widget_show(button);

	button = gtk_button_new_with_label("Cancel");
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
														(GtkSignalFunc) gtk_widget_destroy,
														GTK_OBJECT(dlg));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area), button, TRUE, TRUE, 0);
	gtk_widget_show(button);

	/* The main table */
	/* Set its size (y, x) */
	table = gtk_table_new(5, 3, FALSE);
	gtk_container_border_width(GTK_CONTAINER(table), 10);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), table, TRUE, TRUE, 0);
	gtk_widget_show(table);

	gtk_table_set_row_spacings(GTK_TABLE(table), 10);
	gtk_table_set_col_spacings(GTK_TABLE(table), 10);

	/*********************
	 * The density entry *
	 *********************/
	label = gtk_label_new("Density:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0,
			0);
	gtk_widget_show(label);

	entry = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), entry, 1, 3, 1, 2,
									 GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_set_usize(entry, 50, 0);
	sprintf(buffer, "%0.2f", my_config.density);
	gtk_entry_set_text(GTK_ENTRY(entry), buffer);
	gtk_signal_connect(GTK_OBJECT(entry), "changed",
										 (GtkSignalFunc) entry_callback,
										 &my_config.density);
	gtk_widget_show(entry);

	/**********************
	 * The min/max labels *
	 **********************/
	label = gtk_label_new("Min.");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), label, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 0,
			0);
	gtk_widget_show(label);

	label = gtk_label_new("Max.");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), label, 2, 3, 2, 3, GTK_FILL, GTK_FILL, 0,
			0);
	gtk_widget_show(label);

	/************************
	 * The min_width entry: *
	 ************************/
	label = gtk_label_new("Width:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4, GTK_FILL, GTK_FILL, 0,
			0);
	gtk_widget_show(label);

	entry = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 3, 4, GTK_EXPAND | GTK_FILL,
			GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_set_usize(entry, 50, 0);
	sprintf(buffer, "%i", my_config.min_width);
	gtk_entry_set_text(GTK_ENTRY(entry), buffer);
	gtk_signal_connect(GTK_OBJECT(entry), "changed",
			(GtkSignalFunc) entry_callback, &my_config.min_width);
	gtk_widget_show(entry);

	/************************
	 * The max_width entry: *
	 ************************/
	entry = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), entry, 2, 3, 3, 4, GTK_EXPAND | GTK_FILL,
			GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_set_usize(entry, 50, 0);
	sprintf(buffer, "%i", my_config.max_width);
	gtk_entry_set_text(GTK_ENTRY(entry), buffer);
	gtk_signal_connect(GTK_OBJECT(entry), "changed",
			(GtkSignalFunc) entry_callback, &my_config.max_width);
	gtk_widget_show(entry);

	gtk_widget_show(dlg);

	/************************
	 * The min_height entry: *
	 ************************/
	label = gtk_label_new("Height:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5, GTK_FILL, GTK_FILL, 0,
			0);
	gtk_widget_show(label);

	entry = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 4, 5, GTK_EXPAND | GTK_FILL,
			GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_set_usize(entry, 50, 0);
	sprintf(buffer, "%i", my_config.min_height);
	gtk_entry_set_text(GTK_ENTRY(entry), buffer);
	gtk_signal_connect(GTK_OBJECT(entry), "changed",
			(GtkSignalFunc) entry_callback, &my_config.min_height);
	gtk_widget_show(entry);

	/************************
	 * The max_height entry: *
	 ************************/
	entry = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), entry, 2, 3, 4, 5, GTK_EXPAND | GTK_FILL,
			GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_set_usize(entry, 50, 0);
	sprintf(buffer, "%i", my_config.max_height);
	gtk_entry_set_text(GTK_ENTRY(entry), buffer);
	gtk_signal_connect(GTK_OBJECT(entry), "changed",
			(GtkSignalFunc) entry_callback, &my_config.max_height);
	gtk_widget_show(entry);

	gtk_widget_show(dlg);

	gtk_main();
	gdk_flush();

	return run_flag;
}
