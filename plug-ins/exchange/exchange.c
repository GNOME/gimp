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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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
 *	- preview (working on it already :)
 *	- threshold for each channel (not hard to implement, but really
 *	  needs a preview window)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "libgimp/gimp.h"
#include "gtk/gtk.h"

/* big scales */
#define	SCALE_WIDTH	225

/* datastructure to store parameters in */
typedef struct
{
	guchar	fromred, fromgreen, fromblue, tored, togreen, toblue;
	guchar	threshold;
	gint32	image;
	gint32	drawable;
}	myParams;

/* lets prototype */
static void	query();
static void	run(char *, int, GParam *, int *, GParam **);
static int	doDialog();
static void	exchange(GDrawable *);
static void	doLabelAndScale(char *, GtkWidget *, guchar *);

static void	ok_callback(GtkWidget *, gpointer);
static void	close_callback(GtkWidget *, gpointer);
static void	scale_callback(GtkAdjustment *, gpointer);

/* some global variables */
myParams	xargs = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int		running = 0;

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
		{ PARAM_INT8, "threshold", "Threshold" },
	};
	static GParamDef *return_vals = NULL;
	static int nargs = sizeof(args) / sizeof(args[0]);
	static int nreturn_vals = 0;

	gimp_install_procedure("plug_in_exchange",
			       "Color exchange",
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
	GDrawable 	*drawable;
	gint32  	imageID;
	GStatusType 	status = STATUS_SUCCESS;

	*nreturn_vals = 1;
	*return_vals = values;

	values[0].type = PARAM_STATUS;
	values[0].data.d_status = status;

	switch (runmode = param[0].data.d_int32)
	{
		case RUN_INTERACTIVE:
				/* retrieve stored arguments (if any) */
				gimp_get_data("plug_in_exchange", &xargs);
				/* initialize using foreground color */
				gimp_palette_get_foreground(&xargs.fromred, &xargs.fromgreen, &xargs.fromblue);
				if (!xargs.image && !xargs.drawable)
					xargs.threshold = 0;
				/* and initialize some other things */
				xargs.image = param[1].data.d_image;
				xargs.drawable = param[2].data.d_drawable;
				if (!doDialog())
					return;
				break;
		case RUN_WITH_LAST_VALS:
				/* 
				 * instead of recalling the last-set values,
				 * run with the current foreground as 'from'
				 * color, making ALT-F somewhat more useful.
				 */
				gimp_palette_get_foreground(&xargs.fromred, &xargs.fromgreen, &xargs.fromblue);
				break;
		case RUN_NONINTERACTIVE:
		  if(nparams != 10)
		    status = STATUS_EXECUTION_ERROR;
		  if (status == STATUS_SUCCESS)
		    {
		      xargs.fromred = param[3].data.d_int8;
		      xargs.fromgreen = param[4].data.d_int8;
		      xargs.fromblue = param[5].data.d_int8;
		      xargs.tored = param[6].data.d_int8;
		      xargs.togreen = param[7].data.d_int8;
		      xargs.toblue = param[8].data.d_int8;
		      xargs.threshold = param[9].data.d_int32;
		    }
		  break;
			
		default:	
				break;
	}

	/*  Get the specified drawable  */
	drawable = gimp_drawable_get(param[2].data.d_drawable);
	imageID = param[1].data.d_image;

	if (status == STATUS_SUCCESS)
	{
		if (gimp_drawable_color(drawable->id))
		{
			gimp_progress_init("Color exchange...");
			gimp_tile_cache_ntiles(2 * (drawable->width / gimp_tile_width() + 1));
			exchange(drawable);
			/* store our settings */
			gimp_set_data("plug_in_exchange", &xargs, sizeof(myParams));
			/* and flush */
			gimp_displays_flush();
		}
		else
			status = STATUS_EXECUTION_ERROR;
	}
	values[0].data.d_status = status;
	gimp_drawable_detach(drawable);
}

/* do the exchanging */
static
void	exchange(GDrawable *drawable)
{
	GPixelRgn	srcPR, destPR;
	guchar 		*src_row;
	guchar 		*dest_row;
	gint    	width, height;
	gint    	x, y, bpp;
	gint    	x1, y1, x2, y2;

	/* 
	 * Get the input area. This is the bounding box of the selection in
	 * the image (or the entire image if there is no selection). Only
	 * operating on the input area is simply an optimization. It doesn't
	 * need to be done for correct operation. (It simply makes it go
	 * faster, since fewer pixels need to be operated on).
	 */
	gimp_drawable_mask_bounds(drawable->id, &x1, &y1, &x2, &y2);

        /* 
         * Get the size of the input image. (This will/must be the same
         * as the size of the output image.
         */
	width = drawable->width;
	height = drawable->height;
	bpp = drawable->bpp;

	/* allocate row buffers */
	src_row = (guchar *) malloc((x2 - x1) * bpp);
	dest_row = (guchar *) malloc((x2 - x1) * bpp);

	/* initialize the pixel regions */
	gimp_pixel_rgn_init(&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
	gimp_pixel_rgn_init(&destPR, drawable, 0, 0, width, height, TRUE, TRUE);

	for (y = y1; y < y2; y++)
	{
		gimp_pixel_rgn_get_row(&srcPR, src_row, x1, y, (x2 - x1));
		for (x = x1; x < x2; x++)
		{
			guchar	red, green, blue,
				minred, mingreen, minblue,
				maxred, maxgreen, maxblue;
			int	rest, wanted = 0,
				redx = 0, greenx = 0, bluex = 0;

			/* get boundary values */
			minred = MAX((int) xargs.fromred - xargs.threshold, 0);
			mingreen = MAX((int) xargs.fromgreen - xargs.threshold, 0);
			minblue = MAX((int) xargs.fromblue - xargs.threshold, 0);

			maxred = MIN((int) xargs.fromred + xargs.threshold, 255);
			maxgreen = MIN((int) xargs.fromgreen + xargs.threshold, 255);
			maxblue = MIN((int) xargs.fromblue + xargs.threshold, 255);
			
			/* get current pixel values */
			red = src_row[x * bpp];
			green = src_row[x * bpp + 1];
			blue = src_row[x * bpp + 2];

			/* 
			 * check if we want this pixel (does it fall between
			 * our boundary?)
			 */
			if (red >= minred && red <= maxred &&
			    green >= mingreen && green <= maxgreen &&
			    blue >= minblue && blue <= maxblue)
			{
				redx = red - xargs.fromred;
				greenx = green - xargs.fromgreen;
				bluex = blue - xargs.fromblue;
				wanted = 1;
			}

			/* exchange if needed */
			dest_row[x * bpp] = wanted ? MAX(MIN(xargs.tored + redx, 255), 0) : src_row[x * bpp];
			dest_row[x * bpp + 1] = wanted ? MAX(MIN(xargs.togreen + greenx, 255), 0) : src_row[x * bpp + 1];
			dest_row[x * bpp + 2] = wanted ? MAX(MIN(xargs.toblue + bluex, 255), 0) : src_row[x * bpp + 2];

			/* copy rest (most likely alpha-channel) */
			for (rest = 3; rest < bpp; rest++)
				dest_row[x * bpp + rest] = src_row[x * bpp + rest];
		}
		/* store the dest */
		gimp_pixel_rgn_set_row(&destPR, dest_row, x1, y, (x2 - x1));
		/* and tell the user what we're doing */
		if ((y % 10) == 0)
			gimp_progress_update((double) y / (double) (y2 - y1));
	}
	/* update the processed region */
	gimp_drawable_flush(drawable);
	gimp_drawable_merge_shadow(drawable->id, TRUE);
	gimp_drawable_update(drawable->id, x1, y1, (x2 - x1), (y2 - y1));
	/* and clean up */
	free(src_row);
	free(dest_row);
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
	gchar		**argv;
	gint		argc;
	int		framenumber;

	argc = 1;
	argv = g_new(gchar *, 1);
	argv[0] = g_strdup("exchange");

	gtk_init(&argc, &argv);

	/* set up the dialog */
	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), "Color Exchange");
	gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
	gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
			   (GtkSignalFunc) close_callback,
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
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   (GtkSignalFunc) close_callback,
			   dialog);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
	                   button, TRUE, TRUE, 0);
	gtk_widget_show(button); 

	/* do some boxes here */
	mainbox = gtk_vbox_new(FALSE, 5);
	gtk_container_border_width(GTK_CONTAINER(mainbox), 10);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), mainbox, TRUE, TRUE, 0);
	frombox = gtk_hbox_new(FALSE, 5);
	gtk_container_border_width(GTK_CONTAINER(frombox), 10);
	gtk_box_pack_start(GTK_BOX(mainbox), frombox, TRUE, TRUE, 0);
	tobox = gtk_hbox_new(FALSE, 5);
	gtk_container_border_width(GTK_CONTAINER(tobox), 10);
	gtk_box_pack_start(GTK_BOX(mainbox), tobox, TRUE, TRUE, 0);

	/* and our scales */
	for (framenumber = 0; framenumber < 2; framenumber++)
	{
		frame = gtk_frame_new(framenumber ? "To color" : "From color");
		gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
		gtk_container_border_width(GTK_CONTAINER(frame), 10);
		gtk_box_pack_start(framenumber ? GTK_BOX(tobox) : GTK_BOX(frombox),
		                   frame, TRUE, TRUE, 0);
		gtk_widget_show(frame);
		table = gtk_table_new(8, 2, FALSE);
		gtk_container_border_width(GTK_CONTAINER(table), 10);
		gtk_container_add(GTK_CONTAINER(frame), table);
		doLabelAndScale("Red", table, framenumber ? &xargs.tored : &xargs.fromred);
		doLabelAndScale("Green", table, framenumber ? &xargs.togreen : &xargs.fromgreen);
		doLabelAndScale("Blue", table, framenumber ? &xargs.toblue : &xargs.fromblue);
		if (!framenumber)
			doLabelAndScale("Threshold", table, &xargs.threshold);
		gtk_widget_show(table);
	}

	/* show everything */
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
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, idx, idx + 1, GTK_FILL, 0, 5, 0);
	scale_data = gtk_adjustment_new(*dest, 0.0, 255.0, 0.0, 0.0, 0.0);
	scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
	gtk_widget_set_usize(scale, SCALE_WIDTH, 0);
	/* just need 1:1 resolution on scales */
	gtk_scale_set_digits(GTK_SCALE(scale), 0);
	gtk_table_attach(GTK_TABLE(table), scale, 1, 2, idx, idx + 1, GTK_FILL, 0, 0, 0);
	gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_TOP);
	gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_DELAYED);
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
void	close_callback(GtkWidget *widget, gpointer data)
{
	gtk_main_quit();
}

static
void	scale_callback(GtkAdjustment *adj, gpointer data)
{
	guchar	*val = data;

	*val = (guchar) adj->value;
}
