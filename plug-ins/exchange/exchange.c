/*
 * This is a plug-in for the GIMP.
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
 */

/*
 * Exchange one color with the other (settable threshold to convert from
 * one color-shade to another...might do wonders on certain images, or be
 * totally useless on others).
 * 
 * Author: robert@experimental.net
 * 
 * TODO:
 *		- locken van scales met elkaar
 */

/* 
 * 1999/03/17   Fixed RUN_NONINTERACTIVE and RUN_WITH_LAST_VALS. 
 *              There were uninitialized variables.
 *                                        --Sven <sven@gimp.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include "libgimp/gimp.h"
#include "gtk/gtk.h"

/* big scales */
#define	SCALE_WIDTH	225

/* preview width/height */
#define PREVIEW_SIZE	128

/* datastructure to store parameters in */
typedef struct
{
	guchar	fromred, fromgreen, fromblue, tored, togreen, toblue;
	guchar	red_threshold, green_threshold, blue_threshold;
	gint32	image;
	gint32	drawable;
}	myParams;

/* lets prototype */
static void	query();
static void	run(char *, int, GParam *, int *, GParam **);
static int	doDialog();
static void	exchange();
static void	real_exchange(gint, gint, gint, gint, int);
static void	doLabelAndScale(char *, GtkWidget *, guchar *);
static void	update_preview();

static void	ok_callback(GtkWidget *, gpointer);
static void	lock_callback(GtkWidget *, gpointer);
static void	scale_callback(GtkAdjustment *, gpointer);

/* some global variables */
GDrawable	*drw;
myParams	xargs = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int		running = 0;
GPixelRgn	origregion;
GtkWidget	*preview;
gint		sel_x1, sel_y1, sel_x2, sel_y2;
gint		prev_width, prev_height, sel_width, sel_height;
gint		lock_thres = 0;

/* lets declare what we want to do */
GPlugInInfo PLUG_IN_INFO =
{
	NULL,			       /* init_proc */
	NULL,			       /* quit_proc */
	query,			       /* query_proc */
	run,			       /* run_proc */
};

/* run program */
MAIN()

/* tell GIMP who we are */
static
void	query()
{
	static GParamDef args[] =
	{
		{ PARAM_INT32, "run_mode", "Interactive" },
		{ PARAM_IMAGE, "image", "Input image" },
		{ PARAM_DRAWABLE, "drawable", "Input drawable" },
		{ PARAM_INT8, "fromred", "Red value (from)" },
		{ PARAM_INT8, "fromgreen", "Green value (from)" },
		{ PARAM_INT8, "fromblue", "Blue value (from)" },
		{ PARAM_INT8, "tored", "Red value (to)" },
		{ PARAM_INT8, "togreen", "Green value (to)" },
		{ PARAM_INT8, "toblue", "Blue value (to)" },
		{ PARAM_INT8, "red_threshold", "Red threshold" },
		{ PARAM_INT8, "green_threshold", "Green threshold" },
		{ PARAM_INT8, "blue_threshold", "Blue threshold" },
	};
	static GParamDef *return_vals = NULL;
	static int nargs = sizeof(args) / sizeof(args[0]);
	static int nreturn_vals = 0;

	gimp_install_procedure("plug_in_exchange",
			       "Color Exchange",
			       "Exchange one color with another, optionally setting a threshold to convert from one shade to another",
			       "robert@experimental.net",
			       "robert@experimental.net",
			       "June 17th, 1997",
			       "<Image>/Filters/Colors/Color Exchange",
			       "RGB*",
			       PROC_PLUG_IN,
			       nargs, nreturn_vals,
			       args, return_vals);
}

/* main function */
static
void	run(char *name, int nparams, GParam *param, int *nreturn_vals, GParam **return_vals)
{
	static GParam	values[1];
	GRunModeType	runmode;
	GStatusType 	status = STATUS_SUCCESS;

	*nreturn_vals = 1;
	*return_vals = values;

	values[0].type = PARAM_STATUS;
	values[0].data.d_status = status;

	runmode = param[0].data.d_int32;
	xargs.image = param[1].data.d_image;
	xargs.drawable = param[2].data.d_drawable;
	drw = gimp_drawable_get(xargs.drawable);

	switch (runmode)
	{
	        case RUN_INTERACTIVE:
				/* retrieve stored arguments (if any) */
				gimp_get_data("plug_in_exchange", &xargs);
				/* initialize using foreground color */
				gimp_palette_get_foreground(&xargs.fromred, &xargs.fromgreen, &xargs.fromblue);
				/* and initialize some other things */
				gimp_drawable_mask_bounds(drw->id, &sel_x1, &sel_y1, &sel_x2, &sel_y2);
				sel_width = sel_x2 - sel_x1;
				sel_height = sel_y2 - sel_y1;
				if (sel_width > PREVIEW_SIZE)
						prev_width = PREVIEW_SIZE;
				else
						prev_width = sel_width;
				if (sel_height > PREVIEW_SIZE)
						prev_height = PREVIEW_SIZE;
				else
						prev_height = sel_height;
				if (!doDialog())
					return;
				break;
		case RUN_WITH_LAST_VALS:
				gimp_get_data("plug_in_exchange", &xargs);
				/* 
				 * instead of recalling the last-set values,
				 * run with the current foreground as 'from'
				 * color, making ALT-F somewhat more useful.
				 */
				gimp_palette_get_foreground(&xargs.fromred, &xargs.fromgreen, &xargs.fromblue);
				break;
		case RUN_NONINTERACTIVE:
				if (nparams != 10)
					status = STATUS_EXECUTION_ERROR;
				if (status == STATUS_SUCCESS)
				{
					xargs.fromred = param[3].data.d_int8;
					xargs.fromgreen = param[4].data.d_int8;
					xargs.fromblue = param[5].data.d_int8;
					xargs.tored = param[6].data.d_int8;
					xargs.togreen = param[7].data.d_int8;
					xargs.toblue = param[8].data.d_int8;
					xargs.red_threshold = param[9].data.d_int32;
					xargs.green_threshold = param[10].data.d_int32;
					xargs.blue_threshold = param[11].data.d_int32;
				}
				break;
		default:	
				break;
	}
	if (status == STATUS_SUCCESS)
	{
		if (gimp_drawable_color(drw->id))
		{
			gimp_progress_init("Color Exchange...");
			gimp_tile_cache_ntiles(2 * (drw->width / gimp_tile_width() + 1));
			exchange();	
			gimp_drawable_detach(drw);
			/* store our settings */
			if (runmode == RUN_INTERACTIVE)
			  gimp_set_data("plug_in_exchange", &xargs, sizeof(myParams));
			/* and flush */
			if (runmode != RUN_NONINTERACTIVE)
			  gimp_displays_flush ();
		}
		else
			status = STATUS_EXECUTION_ERROR;
	}
	values[0].data.d_status = status;
}

/* do the exchanging */
static
void	exchange()
{
	/* do the real exchange */
	real_exchange(-1, -1, -1, -1, 0);
}

/* show our dialog */
static
int	doDialog()
{
	GtkWidget	*dialog;
	GtkWidget	*button;
	GtkWidget	*frame;
	GtkWidget	*table;
	GtkWidget	*mainbox;
	GtkWidget	*tobox;
	GtkWidget	*frombox;
	GtkWidget	*prevbox;
	guchar 		*color_cube;
	gchar		**argv;
	gint		argc;
	int		framenumber;

	argc = 1;
	argv = g_new(gchar *, 1);
	argv[0] = g_strdup("exchange");

	gtk_init(&argc, &argv);
        gtk_rc_parse(gimp_gtkrc());

	/* stuff for preview */
	gtk_preview_set_gamma(gimp_gamma());
	gtk_preview_set_install_cmap(gimp_install_cmap()); 
	color_cube = gimp_color_cube();
	gtk_preview_set_color_cube(color_cube[0], color_cube[1], color_cube[2], color_cube[3]); 
	gtk_widget_set_default_visual(gtk_preview_get_visual());
	gtk_widget_set_default_colormap(gtk_preview_get_cmap());
	
	/* load pixelregion */
	gimp_pixel_rgn_init(&origregion, drw, 0, 0, PREVIEW_SIZE, PREVIEW_SIZE, FALSE, FALSE);

	/* set up the dialog */
	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), "Color Exchange");
	gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
	gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
			   (GtkSignalFunc) gtk_main_quit,
			   NULL);

	/* lets create some buttons */
	button = gtk_button_new_with_label("Ok");
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   (GtkSignalFunc) ok_callback,
			   dialog);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
	                   button, TRUE, TRUE, 0);
	gtk_widget_grab_default(button);
	gtk_widget_show(button); 
	
	button = gtk_button_new_with_label("Cancel");
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
			   (GtkSignalFunc) gtk_widget_destroy,
			   GTK_OBJECT(dialog));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
	                   button, TRUE, TRUE, 0);
	gtk_widget_show(button); 

	/* do some boxes here */
	mainbox = gtk_vbox_new(FALSE, 5);
	gtk_container_border_width(GTK_CONTAINER(mainbox), 10);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), mainbox, TRUE, TRUE, 0);

	prevbox = gtk_hbox_new(FALSE, 5);
	gtk_container_border_width(GTK_CONTAINER(prevbox), 0);
	gtk_box_pack_start(GTK_BOX(mainbox), prevbox, TRUE, TRUE, 0);

	frombox = gtk_hbox_new(FALSE, 5);
	gtk_container_border_width(GTK_CONTAINER(frombox), 0);
	gtk_box_pack_start(GTK_BOX(mainbox), frombox, TRUE, TRUE, 0);

	tobox = gtk_hbox_new(FALSE, 5);
	gtk_container_border_width(GTK_CONTAINER(tobox), 0);
	gtk_box_pack_start(GTK_BOX(mainbox), tobox, TRUE, TRUE, 0);

	frame = gtk_frame_new("Preview");
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
	gtk_container_border_width(GTK_CONTAINER(frame), 0);
	gtk_box_pack_start(GTK_BOX(prevbox), frame, TRUE, TRUE, 0);
	gtk_widget_show(frame);

	preview = gtk_preview_new(GTK_PREVIEW_COLOR);
	gtk_preview_size(GTK_PREVIEW(preview), prev_width, prev_height);
	gtk_container_add(GTK_CONTAINER(frame), preview);
	update_preview();
	gtk_widget_show(preview); 

	/* and our scales */
	for (framenumber = 0; framenumber < 2; framenumber++)
	{
		frame = gtk_frame_new(framenumber ? "To color" : "From color");
		gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
		gtk_container_border_width(GTK_CONTAINER(frame), 0);
		gtk_box_pack_start(framenumber ? GTK_BOX(tobox) : GTK_BOX(frombox),
		                   frame, TRUE, TRUE, 0);
		gtk_widget_show(frame);
		table = gtk_table_new(8, 2, FALSE);
		gtk_container_border_width(GTK_CONTAINER(table), 0);
		gtk_container_add(GTK_CONTAINER(frame), table);
		doLabelAndScale("Red", table, framenumber ? &xargs.tored : &xargs.fromred);
		if (! framenumber)
			doLabelAndScale("Red threshold", table, &xargs.red_threshold);
		doLabelAndScale("Green", table, framenumber ? &xargs.togreen : &xargs.fromgreen);
		if (! framenumber)
			doLabelAndScale("Green threshold", table, &xargs.green_threshold);
		doLabelAndScale("Blue", table, framenumber ? &xargs.toblue : &xargs.fromblue);
		if (! framenumber)
			doLabelAndScale("Blue threshold", table, &xargs.blue_threshold);
		if (! framenumber)
		{
			GtkWidget	*button;
			
			button = gtk_check_button_new_with_label("Lock thresholds");
			gtk_table_attach(GTK_TABLE(table), button, 1, 2, 6, 7, GTK_FILL, 0, 0, 0);
			gtk_signal_connect(GTK_OBJECT(button), "clicked", (GtkSignalFunc) lock_callback, dialog);
			gtk_widget_show(button);
		}
		gtk_widget_show(table);
	}

	/* show everything */
	gtk_widget_show(prevbox);
	gtk_widget_show(tobox);
	gtk_widget_show(frombox);
	gtk_widget_show(mainbox);
	gtk_widget_show(dialog);
	gtk_main();
	gdk_flush();

	return running;
}

static
void	doLabelAndScale(char *labelname, GtkWidget *table, guchar *dest)
{
	static	int	idx = -1;
	GtkWidget	*label, *scale;
	GtkObject	*scale_data;

	idx++;
	label = gtk_label_new(labelname);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, idx, idx + 1, GTK_FILL, 0, 0, 0);
	scale_data = gtk_adjustment_new(*dest, 0.0, 255.0, 0.0, 0.0, 0.0);
	scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
	gtk_widget_set_usize(scale, SCALE_WIDTH, 0);
	/* just need 1:1 resolution on scales */
	gtk_scale_set_digits(GTK_SCALE(scale), 0);
	gtk_table_attach(GTK_TABLE(table), scale, 1, 2, idx, idx + 1, GTK_FILL, 0, 0, 0);
	gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_TOP);
	gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_DISCONTINUOUS);
	gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
			   (GtkSignalFunc) scale_callback,
			   dest);
	gtk_widget_show(label);
	gtk_widget_show(scale);
}

static
void	ok_callback(GtkWidget *widget, gpointer data)
{
	running = 1;
	gtk_widget_destroy(GTK_WIDGET(data));
}

static
void	lock_callback(GtkWidget *widget, gpointer data)
{
	lock_thres = 1 - lock_thres;
}

static
void	scale_callback(GtkAdjustment *adj, gpointer data)
{
	guchar	*val = data;

	*val = (guchar) adj->value;
	update_preview();
}

static
void	update_preview()
{
	real_exchange(sel_x1, sel_y1, sel_x1 + prev_width, sel_y1 + prev_height, 1);
	gtk_widget_draw(preview, NULL);
	gdk_flush();
}

static
void	real_exchange(gint x1, gint y1, gint x2, gint y2, int dopreview)
{
	GPixelRgn	srcPR, destPR;
	guchar		*src_row, *dest_row;
	int		x, y, bpp = drw->bpp;
	int		width, height;

	/* fill if necessary */
	if (x1 == -1 || y1 == -1 || x2 == -1 || y2 == -1)
	{
		x1 = sel_x1;
		y1 = sel_y1;
		x2 = sel_x2;
		y2 = sel_y2;
	}
	/* check for valid coordinates */
	width = x2 - x1;
	height = y2 - y1;
	/* allocate memory */
	src_row = g_malloc(drw->width * bpp * sizeof(guchar));
	dest_row =  g_malloc(drw->width * bpp * sizeof(guchar));
	/* initialize the pixel regions */
	/*
	gimp_pixel_rgn_init(&srcPR, drw, x1, y1, width, height, FALSE, FALSE);
	*/
	gimp_pixel_rgn_init(&srcPR, drw, 0, 0, drw->width, drw->height, FALSE, FALSE);
	if (! dopreview)
		gimp_pixel_rgn_init(&destPR, drw, 0, 0, width, height, TRUE, TRUE);
	for (y = y1; y < y2; y++)
	{
		gimp_pixel_rgn_get_row(&srcPR, src_row, 0, y, drw->width);
		for (x = x1; x < x2; x++)
		{
			gint	pixel_red, pixel_green, pixel_blue;
			gint	min_red, max_red,
					min_green, max_green,
					min_blue, max_blue;
			gint	new_red, new_green, new_blue;
			gint	idx, rest;
			
			/* get boundary values */
			min_red = MAX(xargs.fromred - xargs.red_threshold, 0);
			min_green = MAX(xargs.fromgreen - xargs.green_threshold, 0);
			min_blue = MAX(xargs.fromblue - xargs.blue_threshold, 0);

			max_red = MIN(xargs.fromred + xargs.red_threshold, 255);
			max_green = MIN(xargs.fromgreen + xargs.green_threshold, 255);
			max_blue = MIN(xargs.fromblue + xargs.blue_threshold, 255);

			/* get current pixel-values */
			pixel_red = src_row[x * bpp];
			pixel_green = src_row[x * bpp + 1];
			pixel_blue = src_row[x * bpp + 2];

			/* shift down for preview */
			if (dopreview)
				idx = (x - x1) * bpp;
			else
				idx = x * bpp;

			/* want this pixel? */
			if (pixel_red >= min_red && pixel_red <= max_red && pixel_green >= min_green && pixel_green <= max_green && pixel_blue >= min_blue && pixel_blue <= max_blue)
			{
				gint	red_delta, green_delta, blue_delta;

				red_delta = pixel_red - xargs.fromred;
				green_delta = pixel_green - xargs.fromgreen;
				blue_delta = pixel_blue - xargs.fromblue;
				new_red = (guchar) MAX(MIN(xargs.tored + red_delta, 255), 0);
				new_green = (guchar) MAX(MIN(xargs.togreen + green_delta, 255), 0);
				new_blue = (guchar) MAX(MIN(xargs.toblue + blue_delta, 255), 0);
			}
			else
			{
				new_red = pixel_red;
				new_green = pixel_green;
				new_blue = pixel_blue;
			}

			/* fill buffer (cast it too) */
			dest_row[idx + 0] = (guchar) new_red;
			dest_row[idx + 1] = (guchar) new_green;
			dest_row[idx + 2] = (guchar) new_blue;
			
			/* copy rest (most likely alpha-channel) */
			for (rest = 3; rest < bpp; rest++)
				dest_row[idx + rest] = src_row[x * bpp + rest];
		}
		/* store the dest */
		if (dopreview)
			gtk_preview_draw_row(GTK_PREVIEW(preview), dest_row, 0, y - y1, width);
		else
			gimp_pixel_rgn_set_row(&destPR, dest_row, 0, y, drw->width);
		/* and tell the user what we're doing */
		if (! dopreview && (y % 10) == 0)
			gimp_progress_update((double) y / (double) height);
	}
	g_free(src_row);
	g_free(dest_row);
	if (! dopreview)
	{
		/* update the processed region */
		gimp_drawable_flush(drw);
		gimp_drawable_merge_shadow(drw->id, TRUE);
		gimp_drawable_update(drw->id, x1, y1, width, height);
	}
}








