/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Colorify. Changes the pixel's luminosity to a specified color
 * Copyright (C) 1997 Francisco Bustamante
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
 */

/* Changes: 

   1.1 
   -Corrected small bug when calling color selection dialog 
   -Added LUTs to speed things a little bit up 

   1.0 
   -First release */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

#define PLUG_IN_NAME "Colorify"
#define PLUG_IN_VERSION "1.1"

static void      query (void);
static void      run   (char      *name,
			int        nparams,
			GParam    *param,
			int       *nreturn_vals,
			GParam   **return_vals);

typedef struct {
	guchar color[3];
} ColorifyVals;

typedef struct {
	gint run;
} ColorifyInterface;

typedef struct {
	guchar red;
	guchar green;
	guchar blue;
	GtkWidget *preview;
	gint button_num;
} ButtonInformation;

static ColorifyInterface cint =
{
	FALSE
};

static ColorifyVals cvals =
{
	{255, 255, 255}
};

static ButtonInformation button_info[] =
{
	{255, 0, 0, NULL, 0},
	{255, 255, 0, NULL, 0},
	{0, 255, 0, NULL, 0},
	{0, 255, 255, NULL, 0},
	{0, 0, 255, NULL, 0},
	{255, 0, 255, NULL, 0},
	{255, 255, 255, NULL, 0},
};

GPlugInInfo PLUG_IN_INFO =
{
	NULL,
	NULL,
	query,
	run,
};

gint lum_red_lookup[256], lum_green_lookup[256], lum_blue_lookup[256];
gint final_red_lookup[256], final_green_lookup[256], final_blue_lookup[256];

MAIN ()

static int colorify_dialog (guchar red, guchar green, guchar blue);
static void colorify (GDrawable *drawable);
static void set_preview_color (GtkWidget *preview, guchar red, guchar green, guchar blue);

static void
query ()
{
	static GParamDef args[] =
	{
		{ PARAM_INT32, "run_mode", "Interactive, non-interactive" },
		{ PARAM_IMAGE, "image", "Input image" },
		{ PARAM_DRAWABLE, "drawable", "Input drawable" },
		{ PARAM_COLOR, "color", "Color to apply"},
	};

	static GParamDef *return_vals  = NULL;
	static int        nargs        = sizeof(args) / sizeof(args[0]),
		          nreturn_vals = 0;

	gimp_install_procedure ("Colorify",
				"Similar to the \"Color\" mode for layers.",
				"Makes an average of the RGB channels and uses it to set the color",
				"Francisco Bustamante", "Francisco Bustamante",
				"0.0.1", "<Image>/Filters/Image/Colorify", "RGB",
				PROC_PLUG_IN,
				nargs, nreturn_vals,
				args, return_vals);
}

gint sel_x1, sel_x2, sel_y1, sel_y2, sel_width, sel_height;
GtkWidget *preview;
GtkWidget *c_dialog;


static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
	GRunModeType run_mode;
	GStatusType status;
	static GParam values[1];
	GDrawable *drawable;

	status = STATUS_SUCCESS;
	run_mode = param[0].data.d_int32;

	values[0].type = PARAM_STATUS;
	values[0].data.d_status = status;

	*nreturn_vals = 1;
	*return_vals = values;

	drawable = gimp_drawable_get (param[2].data.d_drawable);

	gimp_drawable_mask_bounds (drawable->id, &sel_x1, &sel_y1, &sel_x2, &sel_y2);

	sel_width = sel_x2 - sel_x1;
	sel_height = sel_y2 - sel_y1;

	switch (run_mode) {
		case RUN_INTERACTIVE :
			gimp_get_data(PLUG_IN_NAME, &cvals);
			if (!colorify_dialog (cvals.color[0], cvals.color[1], cvals.color[2]))
				return;
			break;

		case RUN_NONINTERACTIVE :
			if (nparams != 4)
				status = STATUS_CALLING_ERROR;
			if (status == STATUS_SUCCESS) {
					cvals.color[0] = param[3].data.d_color.red;
					cvals.color[1] = param[3].data.d_color.green;
					cvals.color[2] = param[3].data.d_color.blue;
			}
			break;

		case RUN_WITH_LAST_VALS :
			/*  Possibly retrieve data  */
			gimp_get_data (PLUG_IN_NAME, &cvals);
			break;

		default :
			break;
	}

	if (status == STATUS_SUCCESS) {
		gimp_progress_init("Colorifying...");

		colorify (drawable);

		if (run_mode == RUN_INTERACTIVE)
			gimp_set_data (PLUG_IN_NAME, &cvals, sizeof (ColorifyVals));

		if (run_mode != RUN_NONINTERACTIVE) {
			gimp_displays_flush ();
		}
	}

	values[0].data.d_status = status;
}

static void colorify_row (guchar *row,
			  gint width);
static void close_callback (GtkWidget *widget,
			    gpointer data);
static void colorify_ok_callback (GtkWidget *widget,
				  gpointer data);
static void custom_color_callback (GtkWidget *widget,
				   gpointer data);
static void predefined_color_callback (GtkWidget *widget,
				       gpointer data);
static void color_changed (GtkWidget *widget,
			   gpointer data);

static void
colorify (GDrawable *drawable)
{
	GPixelRgn source_region, dest_region;
	guchar *row;
	gint y = 0;
	gint progress = 0;
	gint i = 0;

	for (i = 0; i < 256; i ++) {
		lum_red_lookup[i] = i * 0.30;
		lum_green_lookup[i] = i * 0.59;
		lum_blue_lookup[i] = i * 0.11;
		final_red_lookup[i] = i * cvals.color[0] / 255;
		final_green_lookup[i] = i * cvals.color[1] / 255;
		final_blue_lookup[i] = i * cvals.color[2] / 255;
	}

	row = g_malloc (sel_width * 3 * sizeof(guchar));

	gimp_pixel_rgn_init (&source_region, drawable, sel_x1, sel_y1, sel_width, sel_height, FALSE, FALSE);
	gimp_pixel_rgn_init (&dest_region, drawable, sel_x1, sel_y1, sel_width, sel_height, TRUE, TRUE);

	for (y = sel_y1; y < sel_y2; y++) {
		gimp_pixel_rgn_get_row (&source_region, row, sel_x1, y, sel_width);

		colorify_row (row, sel_width);

		gimp_pixel_rgn_set_row (&dest_region, row, sel_x1, y, sel_width);
		gimp_progress_update ((double) ++progress / sel_height);
		
	}

	g_free (row);

	gimp_drawable_flush (drawable);
 	gimp_drawable_merge_shadow (drawable->id, TRUE); 
	gimp_drawable_update (drawable->id, sel_x1, sel_y1, sel_width, sel_height);
}

static void
colorify_row (guchar *row,
	      gint width)
{
	gint cur_x;
	gint lum; /* luminosity */
	guchar *current = row;

	for (cur_x = 0; cur_x < width; cur_x++) {
		lum = lum_red_lookup[current[0]] + lum_green_lookup[current[1]] + lum_blue_lookup[current[2]];

		current[0] = final_red_lookup[lum];
		current[1] = final_green_lookup[lum];
		current[2] = final_blue_lookup[lum];
		
		current += 3;
	}
}
		
static int
colorify_dialog (guchar red,
		 guchar green,
		 guchar blue)
{
	GtkWidget *dialog;
	GtkWidget *label;
	GtkWidget *button;
	GtkWidget *frame;
	GtkWidget *table;
	gchar **argv;
	gint argc;
	gint i;
	GSList *group = NULL;

	argc = 1;
	argv = g_new (gchar *, 1);
	argv[0] = g_strdup ("colorify");

	gtk_init (&argc, &argv);
	gtk_rc_parse (gimp_gtkrc());

	dialog = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (dialog), "Colorify");
	gtk_window_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
	gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
			    (GtkSignalFunc) close_callback,
			    NULL);

	button = gtk_button_new_with_label ("Ok");
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    (GtkSignalFunc) colorify_ok_callback,
			    dialog);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, TRUE, TRUE, 0);
	gtk_widget_grab_default (button);
	gtk_widget_show (button);

	button = gtk_button_new_with_label ("Cancel");
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
				   (GtkSignalFunc) gtk_widget_destroy,
				   GTK_OBJECT (dialog));
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, TRUE, TRUE, 0);
	gtk_widget_show (button);

	frame = gtk_frame_new ("Color");
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
	gtk_container_border_width (GTK_CONTAINER (frame), 10);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), frame, TRUE, TRUE, 0);
	gtk_widget_show (frame);

	table = gtk_table_new (2, 7, TRUE);
	gtk_container_border_width (GTK_CONTAINER (table), 10);
	gtk_container_add (GTK_CONTAINER (frame), table);
	gtk_table_set_row_spacings (GTK_TABLE (table), 5);
	gtk_table_set_col_spacings (GTK_TABLE (table), 5);
	gtk_widget_show (table);

	label = gtk_label_new ("Custom Color: ");
	gtk_table_attach (GTK_TABLE (table), label, 4, 6, 0, 1,  GTK_FILL, GTK_FILL, 0, 0);
	gtk_widget_show (label);

	button = gtk_radio_button_new (group);
	gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);
 	gtk_widget_set_usize (button, 35, 35);
	gtk_signal_connect (GTK_OBJECT (button), "button_press_event",
			    (GtkSignalFunc) custom_color_callback,
			    NULL);
	gtk_table_attach (GTK_TABLE (table), button, 6, 7, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_widget_show (button);

	preview = gtk_preview_new (GTK_PREVIEW_COLOR);
	gtk_preview_size (GTK_PREVIEW (preview), 30, 30);
	set_preview_color (preview, cvals.color[0], cvals.color[1], cvals.color[2]);
	gtk_container_add (GTK_CONTAINER (button), preview);
	gtk_widget_show (preview);
	
	for(i = 0; i < 7; i++) {
		group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
		button = gtk_radio_button_new (group);
		gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);
		button_info[i].preview = gtk_preview_new (GTK_PREVIEW_COLOR);
		gtk_preview_size (GTK_PREVIEW (button_info[i].preview),
				  30, 30);
		gtk_container_add (GTK_CONTAINER (button), button_info[i].preview);
		set_preview_color (button_info[i].preview,
				   button_info[i].red,
				   button_info[i].green,
				   button_info[i].blue);
		button_info[i].button_num = i;
		gtk_signal_connect (GTK_OBJECT (button), "clicked",
				    (GtkSignalFunc) predefined_color_callback,
				    &button_info[i].button_num);
		gtk_widget_show (button_info[i].preview);
		
		gtk_table_attach (GTK_TABLE (table), button, i, i + 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
		gtk_widget_show (button);
	}
		
	gtk_widget_show (dialog);
	
	gtk_main();
	return cint.run;
}

static void
close_callback (GtkWidget *widget,
		gpointer data)
{
	gtk_main_quit();
}

static void
colorify_ok_callback (GtkWidget *widget,
		      gpointer data)
{
	gtk_widget_destroy (GTK_WIDGET (data));
	cint.run = TRUE;	
}

static void
set_preview_color (GtkWidget *preview,
		   guchar red,
		   guchar green,
		   guchar blue)
{
	gint i;
	guchar buf[3 * 30];

	for (i = 0; i < 30; i ++) {
		buf [3 * i] = red;
		buf [3 * i + 1] = green;
		buf [3 * i + 2] = blue;
	}

	for (i = 0; i < 30; i ++) 
		gtk_preview_draw_row (GTK_PREVIEW (preview), buf, 0, i, 30);

	gtk_widget_draw (preview, NULL);
}

static void
custom_color_callback (GtkWidget *widget,
		       gpointer data)
{
	GtkColorSelectionDialog *csd;
	gdouble colour[3];

	c_dialog = gtk_color_selection_dialog_new ("Colorify Custom Color");
	csd = GTK_COLOR_SELECTION_DIALOG (c_dialog);
 	gtk_color_selection_set_update_policy (GTK_COLOR_SELECTION(csd->colorsel), 
 					       GTK_UPDATE_DISCONTINUOUS); 

	gtk_widget_destroy (csd->help_button);
	gtk_widget_destroy (csd->cancel_button);

	gtk_signal_connect (GTK_OBJECT (csd->ok_button), "clicked",
			    (GtkSignalFunc) color_changed,
			    NULL);

	colour[0] = cvals.color[0] / 255.0;
	colour[1] = cvals.color[1] / 255.0;
	colour[2] = cvals.color[2] / 255.0;

	gtk_color_selection_set_color (GTK_COLOR_SELECTION (csd->colorsel),
				       colour);
	gtk_window_position (GTK_WINDOW(c_dialog), GTK_WIN_POS_MOUSE);
	gtk_widget_show (c_dialog);
}

static void
predefined_color_callback (GtkWidget *widget,
			   gpointer data)
{
	gint *num;

	num = (gint *) data;
	
	cvals.color[0] = button_info[*num].red;
	cvals.color[1] = button_info[*num].green;
	cvals.color[2] = button_info[*num].blue;
}

static void
color_changed (GtkWidget *widget,
	       gpointer data)
{
	gdouble colour[3];

	gtk_color_selection_get_color (GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (c_dialog)->colorsel),
				       colour);

	cvals.color[0] = (guchar) (colour[0] * 255.0);
	cvals.color[1] = (guchar) (colour[1] * 255.0);
	cvals.color[2] = (guchar) (colour[2] * 255.0);
	
	set_preview_color (preview, cvals.color[0], cvals.color[1], cvals.color[2]);
	gtk_widget_destroy (c_dialog);
}

