/* Stereogram 0.5 --- image filter plug-in for The Gimp
 * Copyright (C) 1997 Francisco Bustamante
 * pointnclick@geocities.com
 * http://www.geocities.com/SiliconValley/Lakes/4813/index.html
 *
 * The Gimp -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 */

/* This program is free software; you can redistribute it and/or modify
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

/* This plug-in based on information from the Stereogram FAQ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#define SIRDS 1
#define SIS   2

#define TILE_CACHE_SIZE 16

typedef struct {
	gint run;
} StereogramInterface;

static StereogramInterface sint = {
	FALSE
};

static void query (void);

static void run (char *name,
		 int nparams,
		 GParam *param,
		 int *nreturn_vals,
		 GParam **return_vals);

static gint create_stereogram(GDrawable *heightdrawable,
			      guchar *pixels,
			      GDrawable *patterndrawable,
			      int type);

static gint stereogram_dialog(void);

static void stereogram_close_callback(GtkWidget  *widget,
				      gpointer data);
static void stereogram_ok_callback(GtkWidget *widget,
				   gpointer data);
static void stereogram_drawable_callback (gint32     id,
					gpointer   data);
static void stereogram_toggle_update(GtkWidget *widget,
				     gpointer data);

static gint stereogram_constrain (gint32   image_id,
				  gint32   drawable_id,
				  gpointer data);

typedef struct {
	int t; /* Stereogram type */
	gint32 patID; /* Pattern Drawable ID*/
} StereogramVals;

static StereogramVals svals =
{
	SIRDS, /* type */
	-1 /* patID */
};

GPlugInInfo PLUG_IN_INFO =
{
	NULL, /*init_proc*/
	NULL,  /*quit_proc*/
	query, /*query_proc*/
	run, /*run_proc*/
};

MAIN()

static void
query(void)
{
	static GParamDef args[] =
	{
		{ PARAM_INT32, "run_mode", "Interactive, non-interactive" },
		{ PARAM_IMAGE, "image", "Input Image" },
		{ PARAM_DRAWABLE, "heightmap", "Height Map" },
		{ PARAM_INT32, "type", "Type of stereogram" },
		{ PARAM_DRAWABLE, "pat", "Pattern to use for stereogram" },
	};

	static GParamDef *return_vals = NULL;

	static int nargs = sizeof(args) / sizeof(args[0]);
	static int nreturn_vals = 0;

	gimp_install_procedure("plug_in_stereogram",
			       "Creates SIS (Single Image Stereograms) and SIRDS (Single Image Random Dot Stereogram)",
			       "Based on the Stereogram FAQ",
			       "Francisco Bustamante",
			       "Francisco Bustamante",
			       "1997",
			       "<Image>/Filters/Misc/Stereogram",
			       "GRAY",
			       PROC_PLUG_IN,
			       nargs, nreturn_vals,
			       args, return_vals);
} /*query*/

static void
run (char *name,
     int nparams,
     GParam *param,
     int *nreturn_vals,
     GParam **return_vals)
{
	GRunModeType run_mode;
	static GParam values[1];
	GDrawable *heightdrawable;
	GDrawable *patterndrawable;
	GDrawable *destdrawable;
	GStatusType status = STATUS_SUCCESS;
	GPixelRgn destrgn;
	guchar *dest_pixels;
	static gint32 DestImageID;
	gint32 layerID;
	
	run_mode = param[0].data.d_int32;

	*nreturn_vals = 1;
	*return_vals = values;

	values[0].type = PARAM_STATUS;
	values[0].data.d_status = status;

	switch(run_mode) {
		case RUN_INTERACTIVE :
			/*Retrieve data */
			gimp_get_data("plug_in_stereogram", &svals);
			if(!stereogram_dialog())
				return;
			break;
			
		case RUN_NONINTERACTIVE :
			if((nparams < 4) || ((nparams != 5) && (param[3].data.d_int32 == SIS)))
				status = STATUS_CALLING_ERROR;
			if (status == STATUS_SUCCESS) {
				svals.t = param[3].data.d_int32;
				if(nparams == 5)
					svals.patID = param[4].data.d_drawable;
			}
			
		case RUN_WITH_LAST_VALS :
			/* Retrieve data */
			gimp_get_data("plug_in_stereogram", &svals);
			break;
			
		default:
			break;
	}
	
	heightdrawable = gimp_drawable_get(param[2].data.d_drawable);

	DestImageID = gimp_image_new(heightdrawable->width, heightdrawable->height, RGB);

	layerID = gimp_layer_new(DestImageID, "Background", heightdrawable->width,
				 heightdrawable->height,
				 RGB, 100, NORMAL_MODE);
	
	gimp_image_add_layer(DestImageID, layerID, 0);
	
	destdrawable = gimp_drawable_get(layerID);
	
	dest_pixels = (guchar *) g_malloc((destdrawable->width) * (destdrawable->height) * 3);
	
	if(status == STATUS_SUCCESS) {
		if(gimp_drawable_gray(param[2].data.d_drawable)) {
			gimp_progress_init("Stereogramming...");

			gimp_tile_cache_ntiles(TILE_CACHE_SIZE);
			
			patterndrawable = NULL;
			if(svals.patID != -1)
				patterndrawable = gimp_drawable_get(svals.patID);

			create_stereogram(heightdrawable, dest_pixels,
					  patterndrawable, svals.t);
			if(patterndrawable)
				gimp_drawable_detach(patterndrawable);
			if(run_mode == RUN_INTERACTIVE)
				gimp_set_data("plug_in_stereogram", &svals, sizeof(StereogramVals));
		}
	}
	gimp_pixel_rgn_init(&destrgn, destdrawable, 0, 0,
			    destdrawable->width,
			    destdrawable->height,
			    FALSE, FALSE);

	gimp_pixel_rgn_set_rect(&destrgn, dest_pixels, 0, 0,
				destdrawable->width,
				destdrawable->height);

	gimp_drawable_detach(destdrawable);
	gimp_drawable_detach(heightdrawable);

	gimp_display_new(DestImageID);
	values[0].data.d_status = status;
}

guchar red[6] = {0, 170, 20, 60, 135, 255};
guchar green[6] = {0, 32, 56, 153, 23, 255};
guchar blue[6] = {0, 200, 140, 40, 100, 255};

static void
set_to_zeros(gint *buf, gint len)
{
	gint i;

	for( i = 0; i < len; i ++) {
		buf[i] = -1;
	}
}

static void
pixel_set(gint * buf, gint position, gint value)
{
	buf[position] = value;
}

static gint
is_pixel_set(gint *buf, gint position)
{
	if(buf[position] != -1)
		return TRUE;
	else
		return FALSE;
}

static void
put_pixel_in_blank(guchar *pattern, gint pat_width,
		   gint pat_height, guchar *dest,
		   gint offset, gint dest_line, gint *set)
{
	guchar *temp;
	guchar *temp_pat;
	gint pat_x, pat_y;
	gint tempx;

	tempx = offset;

	while((set[tempx] == -1) && (tempx > 0))
		tempx--;
	if(tempx == 0) {
		set[tempx] = 0; 
		pat_x = set[tempx];
		pat_y = dest_line % pat_height;
	
		temp = dest + tempx * 3;
		temp_pat = pattern + (pat_y * pat_width * 3) + (pat_x * 3);

		*temp++ = *temp_pat++;
		*temp++ = *temp_pat++;
		*temp = *temp_pat;
		tempx++;
	}
	tempx ++;
	
	while(tempx <= offset) {
		set[tempx] = ((set[tempx - 1] + 1) % pat_width); 
		pat_x = set[tempx];
		pat_y = dest_line % pat_height;
	
		temp = dest + tempx * 3;
		temp_pat = pattern + (pat_y * pat_width * 3) + (pat_x * 3);

		*temp++ = *temp_pat++;
		*temp++ = *temp_pat++;
		*temp = *temp_pat;
		tempx++;
	}
}

void
line_fill(guchar *pattern, gint pat_width,
	  gint pat_height, guchar *dest,
	  gint dest_line, gint *set,
	  gint line_width)
{
	gint cur_x;

	for(cur_x = 0; cur_x < line_width; cur_x ++) {
		if(set[cur_x] == -1) {
			put_pixel_in_blank(pattern, pat_width,
					   pat_height, dest,
					   cur_x, dest_line, set);
		}
	}
	
}
	
static gint
create_stereogram(GDrawable *heightdrawable,
		  guchar *pixels,
		  GDrawable *patterndrawable,
		  int type)
{
	int E;
	int dist_to_screen;
	GPixelRgn patrgn;
	GPixelRgn hgtrgn;
	gint src_height, src_width;
	gint pat_height, pat_width;
	gint dest_height, dest_width;
	guchar *pattern = NULL;
	guchar *final_line;
	guchar *temp;
	guchar *source_line;
	guchar *temp_source;
	gint cur_progress = 0, max_progress = heightdrawable->height;
	gint cur_x = 0, cur_y = 0, curpat_y = 0;
	gint separation;
	int i, left, right;
	gint *set;
	
	dest_width = src_width = heightdrawable->width;
	dest_height = src_height = heightdrawable->height;
	
	source_line = (guchar *) g_malloc(heightdrawable->width);

	gimp_pixel_rgn_init(&hgtrgn, heightdrawable, 0, 0,
			    src_width,
			    src_height,
			    FALSE, FALSE);

	set = (gint *) g_malloc(dest_width * sizeof(gint));
	
	if(svals.t == SIS) {
		pat_width = patterndrawable->width;
		pat_height = patterndrawable->height;
		gimp_pixel_rgn_init(&patrgn, patterndrawable, 0, 0,
				    pat_width,
				    pat_height,
				    FALSE, FALSE);
		pattern = (guchar *) g_malloc(pat_height * pat_width * 3);
		gimp_pixel_rgn_get_rect(&patrgn, pattern, 0, 0, pat_width, pat_height);
	}
	else {
		pat_width = 80;
		pat_height = 80;
		pattern = (guchar *) g_malloc(pat_height * pat_width * 3);
		for(cur_y = 0; cur_y < pat_height; cur_y++) {
			for(cur_x = 0; cur_x < pat_width; cur_x++) {
				temp = pattern + ((cur_y * pat_width) + cur_x) * 3;
				i = rand() % 6;
				*temp++ = red[i];
				*temp++ = green[i];
				*temp = blue[i];
			}
		}
	}

	E = 192;
	dist_to_screen = 900;
	curpat_y = 0;
	
	for(cur_y = 0; cur_y < src_height; cur_y++) {
		set_to_zeros(set, dest_width);
		gimp_pixel_rgn_get_row(&hgtrgn, source_line, 0, cur_y, src_width);
		final_line = pixels + (cur_y * dest_width * 3);
		for(cur_x = 0; cur_x < src_width; cur_x++) {
			temp_source = source_line + cur_x;
			separation = ((800 - *temp_source) * E) / ( dist_to_screen + (800 - *temp_source));
			left = cur_x - (separation / 2);
			right = left + separation;
			if((right < dest_width) && (left >= 0)) {
				if(is_pixel_set(set, left) == FALSE) {
					put_pixel_in_blank(pattern, pat_width,
							   pat_height, final_line,
							   left, cur_y, set);
				}
				temp = final_line + left * 3;
				*(final_line + (right * 3)) = *temp;
				*(final_line + (right * 3 + 1)) = *++temp;
				*(final_line + (right * 3 + 2)) = *++temp;
				pixel_set(set, right, set[left]);
			}
		}
		line_fill(pattern, pat_width,
			  pat_height, final_line,
			  cur_y, set,
			  src_width);
		cur_progress++;
		if((cur_progress % 10) == 0) {
			gimp_progress_update((gdouble) cur_progress / (gdouble) max_progress);
		}
	}
	if(pattern)
		g_free(pattern);

	return 1;
} /*create_stereogram*/

static gint
stereogram_dialog(void)
{
	GtkWidget *dlg;
	GtkWidget *label;
	GtkWidget *optionmenu;
	GtkWidget *menu;
	GtkWidget *button;
	GtkWidget *table;
	GtkWidget *frame;
	GtkWidget *toggle;
	GtkWidget *toggle_hbox;
	GSList *group = NULL;
	gchar **argv;
	gint argc;
	gint sis = (svals.t == SIS);
	gint sirds = (svals.t == SIRDS);

	argc = 1;
	argv = g_new(gchar *, 1);
	argv[0] = g_strdup("stereogram");

	gtk_init(&argc, &argv);

	dlg = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dlg), "Stereogram");
	gtk_window_position(GTK_WINDOW(dlg), GTK_WIN_POS_MOUSE);
	gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
			   (GtkSignalFunc) stereogram_close_callback,
			   NULL);

	button = gtk_button_new_with_label("OK");
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   (GtkSignalFunc) stereogram_ok_callback,
			   dlg);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area), button, TRUE, TRUE, 0);
	gtk_widget_grab_default(button);
	gtk_widget_show(button);

	button = gtk_button_new_with_label("Cancel");
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
				  (GtkSignalFunc) stereogram_close_callback,
				  GTK_OBJECT(dlg));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area), button, TRUE, TRUE, 0);
	gtk_widget_show(button);

	frame = gtk_frame_new("Stereogram Options");
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
	gtk_container_border_width(GTK_CONTAINER(frame), 10);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), frame, TRUE, TRUE, 0);

	table = gtk_table_new(3, 8, FALSE);
	gtk_container_border_width(GTK_CONTAINER(table), 5);
	gtk_container_add(GTK_CONTAINER(frame), table);

	gtk_table_set_row_spacing (GTK_TABLE (table), 0, 10);
	gtk_table_set_row_spacing (GTK_TABLE (table), 1, 10);
	gtk_table_set_col_spacing (GTK_TABLE (table), 0, 10);
	gtk_table_set_col_spacing (GTK_TABLE (table), 1, 10);

	label = gtk_label_new("Stereogram Pattern: ");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

	optionmenu = gtk_option_menu_new();

	menu = gimp_drawable_menu_new(stereogram_constrain,
				      stereogram_drawable_callback,
				      NULL, svals.patID);

	gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu), menu);
	gtk_table_attach(GTK_TABLE(table), optionmenu, 1, 3, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
	gtk_widget_show(optionmenu);
	
	toggle_hbox = gtk_hbox_new(FALSE, 10);
	gtk_container_border_width(GTK_CONTAINER(toggle_hbox), 5);
	gtk_table_attach(GTK_TABLE(table), toggle_hbox, 0, 3, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
	
	label = gtk_label_new("Stereogram type: ");
	gtk_box_pack_start(GTK_BOX(toggle_hbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);
	
	toggle = gtk_radio_button_new_with_label(group, "SIS");
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
	gtk_box_pack_start(GTK_BOX(toggle_hbox), toggle, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
			   (GtkSignalFunc) stereogram_toggle_update,
			   &sis);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), sis);
	gtk_widget_show(toggle);

	toggle = gtk_radio_button_new_with_label(group, "SIRDS");
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
	gtk_box_pack_start(GTK_BOX(toggle_hbox), toggle, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
			   (GtkSignalFunc) stereogram_toggle_update,
			   &sirds);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), sirds);
	gtk_widget_show(toggle);

	gtk_widget_show(toggle_hbox);
	gtk_widget_show(table);
	gtk_widget_show(frame);
	gtk_widget_show(dlg);

	gtk_main();
	gdk_flush();

	if(sis)
		svals.t = SIS;
	else
		svals.t = SIRDS;

	return sint.run;
}

static void
stereogram_close_callback(GtkWidget  *widget,
			  gpointer data)
{
	gtk_main_quit();
}

static void
stereogram_ok_callback(GtkWidget *widget,
		       gpointer data)
{
	sint.run = TRUE;
	gtk_widget_destroy(GTK_WIDGET(data));
}
	
static void
stereogram_toggle_update(GtkWidget *widget,
			 gpointer data)
{
	int * toggle_val;

	toggle_val = (int *) data;

	if (GTK_TOGGLE_BUTTON (widget)->active)
		*toggle_val = TRUE;
	else
		*toggle_val = FALSE;
}

static gint
stereogram_constrain (gint32   image_id,
		    gint32   drawable_id,
		    gpointer data)
{
  if (drawable_id == -1)
    return TRUE;

  return (gimp_drawable_color (drawable_id));
}


static void
stereogram_drawable_callback (gint32     id,
			    gpointer   data)
{
	svals.patID = id;
}

