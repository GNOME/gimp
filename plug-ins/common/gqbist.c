/*
 * Written 1997 Jens Ch. Restemeier <jchrr@hrz.uni-bielefeld.de>
 * This program is based on an algorithm / article by
 * Jörn Loviscach.
 *
 * It appeared in c't 10/95, page 326 and is called 
 * "Ausgewürfelt - Moderne Kunst algorithmisch erzeugen".
 * (~modern art created with algorithms)
 * 
 * It generates one main formula (the middle button) and 8 variations of it.
 * If you select a variation it becomes the new main formula. If you
 * press "OK" the main formula will be applied to the image.
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

/*
 * History:
 * 1.0 first release
 * 1.2 now handles RGB*
 * 1.5 fixed a small bug
 * 1.6 fixed a bug that was added by v1.5 :-(
 * 1.7 added patch from Art Haas to make it compile with HP-UX, a small clean-up
 * 1.8 Dscho added transform file load/save, bug-fixes 
 * 1.9 rewrote renderloop.
 * 1.9a fixed a bug.
 * 1.9b fixed MAIN()
 * 1.10 added optimizer
 */
                 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "gtk/gtk.h"
#include "libgimp/gimp.h"

/** qbist renderer ***********************************************************/

#define MAX_TRANSFORMS	36
#define NUM_TRANSFORMS	9
#define NUM_REGISTERS	6

#define PLUG_IN_NAME "plug_in_qbist"
#define PLUG_IN_VERSION "March 1998, 1.10"
#define PREVIEW_SIZE 64

/** types *******************************************************************/

typedef gfloat vreg[3];

typedef struct _info {
	int transformSequence	[MAX_TRANSFORMS];
	int source		[MAX_TRANSFORMS];
	int control		[MAX_TRANSFORMS];
	int dest		[MAX_TRANSFORMS];
} s_info;

#define PROJECTION	0
#define SHIFT		1
#define SHIFTBACK	2
#define ROTATE		3
#define ROTATE2		4
#define MULTIPLY	5
#define SINE		6
#define CONDITIONAL	7
#define COMPLEMENT	8

/** prototypes **************************************************************/

void query(void);
void run(char *name, int nparams, GParam *param, int *nreturn_vals, GParam **return_vals);

void dialog_cancel();
void dialog_new_variations(GtkWidget *widget, gpointer data);
void dialog_update_previews(GtkWidget *widget, gpointer data);
void dialog_select_preview (GtkWidget *widget, s_info *n_info);
int dialog_create();

s_info qbist_info;

/** qbist functions *********************************************************/

void create_info(s_info *info)
{
	int k;
	for (k=0; k<MAX_TRANSFORMS; k++) {
		info->transformSequence[k]=rand() % NUM_TRANSFORMS;
		info->source[k]=rand() % NUM_REGISTERS;
		info->control[k]=rand() % NUM_REGISTERS;
		info->dest[k]=rand() % NUM_REGISTERS;
	}
	info->dest[rand() % MAX_TRANSFORMS]=0;
}

void modify_info(s_info *o_info, s_info *n_info)
{
	int k, n; 
	memcpy(n_info, o_info, sizeof(s_info)); 
	n=rand() % MAX_TRANSFORMS;
	for (k=0;k<n; k++) {
		switch (rand() % 4) {
			case 0: n_info->transformSequence[rand() % MAX_TRANSFORMS] = rand() % NUM_TRANSFORMS; break;
			case 1: n_info->source[rand() % MAX_TRANSFORMS]            = rand() % NUM_REGISTERS; break;
			case 2: n_info->control[rand() % MAX_TRANSFORMS]           = rand() % NUM_REGISTERS; break;
			case 3: n_info->dest[rand() % MAX_TRANSFORMS]              = rand() % NUM_REGISTERS; break;
		}
	}
}

/*
 * Optimizer
 */
int used_trans_flag[MAX_TRANSFORMS];
int used_reg_flag[NUM_REGISTERS];

void check_last_modified(s_info info, int p, int n)
{
	p--;
	while ((p>=0) && (info.dest[p]!=n)) p--;
	if (p<0) 
		used_reg_flag[n]=1;
	else {
		used_trans_flag[p]=1;
		check_last_modified(info, p, info.source[p]);
		check_last_modified(info, p, info.control[p]);
	}
}

void optimize(s_info info)
{
	int i;
	/* double-arg fix: */
	for (i=0; i<MAX_TRANSFORMS; i++) {
		used_trans_flag[i]=0;
		if (i<NUM_REGISTERS)
			used_reg_flag[i]=0;
		/* double-arg fix: */
		switch (info.transformSequence[i]) {
			case ROTATE: 
			case ROTATE2: 
			case COMPLEMENT: 
				info.control[i]=info.dest[i];
				break;
		}
	}
	/* check for last modified item */
	check_last_modified(info, MAX_TRANSFORMS, 0);
}

void qbist(s_info info, gchar *buffer, int xp, int yp, int num, int width, int height, int bpp)
{
	gushort gx;
	vreg reg [NUM_REGISTERS];
	int i;
	gushort sr, cr, dr;

	if (num<=0) return;

	for(gx=0; gx<num; gx ++) {
		for(i=0; i<NUM_REGISTERS; i++) {
			if (used_reg_flag[i]) {
				reg[i][0] = ((float)gx+xp) / ((float)(width));
				reg[i][1] = ((float)yp) / ((float)(height));
				reg[i][2] = ((float)i) / ((float)NUM_REGISTERS);
			}
		}
		for(i=0;i<MAX_TRANSFORMS; i++) {
			sr=info.source[i];cr=info.control[i];dr=info.dest[i];
			
			if (used_trans_flag[i]) switch (info.transformSequence[i]) {
				case PROJECTION: {
					gfloat scalarProd;
					scalarProd = (reg[sr][0]*reg[cr][0])+(reg[sr][1]*reg[cr][1])+(reg[sr][2]*reg[cr][2]);
					reg[dr][0] = scalarProd*reg[sr][0];
					reg[dr][1] = scalarProd*reg[sr][1];
					reg[dr][2] = scalarProd*reg[sr][2];
					break;
				}
				case SHIFT: 
					reg[dr][0] = reg[sr][0]+reg[cr][0];
					if (reg[dr][0] >= 1.0) reg[dr][0] -= 1.0;
					reg[dr][1] = reg[sr][1]+reg[cr][1];
					if (reg[dr][1] >= 1.0) reg[dr][1] -= 1.0;
					reg[dr][2] = reg[sr][2]+reg[cr][2];
					if (reg[dr][2] >= 1.0) reg[dr][2] -= 1.0;
					break;
				case SHIFTBACK: 
					reg[dr][0] = reg[sr][0]-reg[cr][0];
					if (reg[dr][0] <= 0.0) reg[dr][0] += 1.0;
					reg[dr][1] = reg[sr][1]-reg[cr][1];
					if (reg[dr][1] <= 0.0) reg[dr][1] += 1.0;
					reg[dr][2] = reg[sr][2]-reg[cr][2];
					if (reg[dr][2] <= 0.0) reg[dr][2] += 1.0;
					break;
				case ROTATE: 
					reg[dr][0] = reg[sr][1];
					reg[dr][1] = reg[sr][2];
					reg[dr][2] = reg[sr][0];
					break;
				case ROTATE2: 
					reg[dr][0] = reg[sr][2];
					reg[dr][1] = reg[sr][0];
					reg[dr][2] = reg[sr][1];
					break;
				case MULTIPLY: 
					reg[dr][0] = reg[sr][0]*reg[cr][0];
					reg[dr][1] = reg[sr][1]*reg[cr][1];
					reg[dr][2] = reg[sr][2]*reg[cr][2];
					break;
				case SINE: 
					reg[dr][0] = 0.5+(0.5*sin(20.0*reg[sr][0]*reg[cr][0]));
					reg[dr][1] = 0.5+(0.5*sin(20.0*reg[sr][1]*reg[cr][1]));
					reg[dr][2] = 0.5+(0.5*sin(20.0*reg[sr][2]*reg[cr][2]));
					break;
				case CONDITIONAL: 
					if ((reg[cr][0]+reg[cr][1]+reg[cr][2]) > 0.5)	{
						reg[dr][0] = reg[sr][0];
						reg[dr][1] = reg[sr][1];
						reg[dr][2] = reg[sr][2];
					} else {
						reg[dr][0] = reg[cr][0];
						reg[dr][1] = reg[cr][1];
						reg[dr][2] = reg[cr][2];
					}
					break;
				case COMPLEMENT: 
					reg[dr][0] = 1.0-reg[sr][0];
					reg[dr][1] = 1.0-reg[sr][1];
					reg[dr][2] = 1.0-reg[sr][2];
					break;
			}
		}
		for (i=0; i<bpp; i++) {
			if (i<3) { 
				int a;
				a=255.0 * reg[0][i];
				buffer[i]=(a<0) ? 0 : ((a>255) ? 255 : a); 
			} else {
				buffer[i]=255;
			}
		}
		buffer+=bpp;
	}
}

/** Plugin interface *********************************************************/

GPlugInInfo PLUG_IN_INFO = {
	NULL,	/* init_proc */
	NULL,	/* quit_proc */
	query,	/* query_proc */
	run	/* run_proc */
};

/* Definition of parameters */
GParamDef args[] = {
	{ PARAM_INT32, "run_mode", "Interactive, non-interactive" },
	{ PARAM_IMAGE, "image", "Input image (unused)" },
	{ PARAM_DRAWABLE, "drawable", "Input drawable" }
};

GParamDef *return_vals  = NULL;
int        nargs        = sizeof(args) / sizeof(args[0]);
int        nreturn_vals = 0;

MAIN()

void query(void)
{
        gimp_install_procedure(PLUG_IN_NAME, 
                               "Create images based on a random genetic formula", 
                               "This Plug-in is based on an article by "
                               "Jörn Loviscach (appeared in c't 10/95, page 326). It generates modern art "
                               "pictures from a random genetic formula.", 
                               "Jörn Loviscach, Jens Ch. Restemeier", 
                               "Jörn Loviscach, Jens Ch. Restemeier", 
                               PLUG_IN_VERSION, 
                               "<Image>/Filters/Render/Pattern/Qbist", 
                               "RGB*", 
                               PROC_PLUG_IN, 
                               nargs, 
                               nreturn_vals, 
                               args, 
                               return_vals);
}

/* Return values */
GParam values[1];

void run(char *name, int nparams, GParam *param, int *nreturn_vals, GParam **return_vals)
{
	gint sel_x1, sel_y1, sel_x2, sel_y2;
	gint img_height, img_width, img_bpp, img_has_alpha;

	GDrawable 	*drawable;
        GRunModeType	run_mode;
        GStatusType	status;

        *nreturn_vals = 1;
        *return_vals  = values;

        status = STATUS_SUCCESS;
	
	if (param[0].type!=PARAM_INT32)
		status=STATUS_CALLING_ERROR;
        run_mode = param[0].data.d_int32;

        if (param[2].type!=PARAM_DRAWABLE)
		status=STATUS_CALLING_ERROR;
        drawable = gimp_drawable_get(param[2].data.d_drawable);

        img_width     = gimp_drawable_width(drawable->id);
        img_height    = gimp_drawable_height(drawable->id);
        img_bpp       = gimp_drawable_bpp(drawable->id);
        img_has_alpha = gimp_drawable_has_alpha(drawable->id);
        gimp_drawable_mask_bounds(drawable->id, &sel_x1, &sel_y1, &sel_x2, &sel_y2);

	if (!gimp_drawable_is_rgb(drawable->id))
		status=STATUS_CALLING_ERROR;
		
	if (status==STATUS_SUCCESS) {
		create_info(&qbist_info);
        	switch (run_mode) {
        	        case RUN_INTERACTIVE:
        	                /* Possibly retrieve data */
        	                gimp_get_data(PLUG_IN_NAME, &qbist_info);
        	
        	                /* Get information from the dialog */
        	                if (dialog_create()) {
        	                	status=STATUS_SUCCESS;
		                        gimp_set_data(PLUG_IN_NAME, &qbist_info, sizeof(s_info));
				} else
					status=STATUS_EXECUTION_ERROR;
        	                break;

        	        case RUN_NONINTERACTIVE:
				status=STATUS_CALLING_ERROR;
        	                break;
	
	                case RUN_WITH_LAST_VALS:
	                        /* Possibly retrieve data */
	                        gimp_get_data(PLUG_IN_NAME, &qbist_info);
	                        status=STATUS_SUCCESS;
	                        break;
	                default:
				status=STATUS_CALLING_ERROR;
	                        break;
	        } 
        	if (status == STATUS_SUCCESS) {
        		GPixelRgn imagePR;
			guchar *row_data;
			gint row;

	                gimp_tile_cache_ntiles((drawable->width + gimp_tile_width() - 1) / gimp_tile_width());
			gimp_pixel_rgn_init (&imagePR, drawable, 0,0, img_width, img_height, TRUE, TRUE);
			row_data=(guchar *)malloc((sel_x2-sel_x1)*img_bpp);

			optimize(qbist_info);

			gimp_progress_init ("Qbist ...");
			for (row=sel_y1; row<sel_y2; row++) {
				qbist(qbist_info, (gchar *)row_data, 0, row, sel_x2-sel_x1, sel_x2-sel_x1, sel_y2-sel_y1, img_bpp);
				gimp_pixel_rgn_set_row(&imagePR, row_data, sel_x1, row, (sel_x2-sel_x1));
				if ((row % 5) == 0) 
					gimp_progress_update((gfloat)(row-sel_y1)/(gfloat)(sel_y2-sel_y1));
			}

			free(row_data);
			gimp_drawable_flush (drawable);
			gimp_drawable_merge_shadow (drawable->id, TRUE);
			gimp_drawable_update (drawable->id, sel_x1, sel_y1, (sel_x2 - sel_x1), (sel_y2 - sel_y1));
			            				
                        gimp_displays_flush();
	        } 
	}
	
	values[0].type = PARAM_STATUS;
        values[0].data.d_status = status;
        gimp_drawable_detach(drawable);
}

/** User interface ***********************************************************/

GtkWidget *preview[9];
s_info info[9];
gint result;

void dialog_close(GtkWidget *widget, gpointer data)
{
	gtk_main_quit();
}

void dialog_cancel(GtkWidget *widget, gpointer data)
{
	gtk_widget_destroy(GTK_WIDGET(data));
}

void dialog_ok(GtkWidget *widget, gpointer data)
{
	result=TRUE;
	gtk_widget_destroy(GTK_WIDGET(data));
}

void dialog_new_variations(GtkWidget *widget, gpointer data)
{
	int i;
	for (i=1; i<9; i++)
		modify_info(&(info[0]), &(info[i]));
}

void dialog_update_previews(GtkWidget *widget, gpointer data)
{
	int i, j;
	guchar buf[PREVIEW_SIZE * 3];
	for (j=0;j<9;j++) {
		optimize(info[(j+5) % 9]);
		for (i = 0; i < PREVIEW_SIZE; i++) {
			qbist(info[(j+5) % 9], (gchar *)buf, 0, i, PREVIEW_SIZE, PREVIEW_SIZE, PREVIEW_SIZE, 3);
			gtk_preview_draw_row (GTK_PREVIEW (preview[j]), buf, 0, i, PREVIEW_SIZE);
		}
		gtk_widget_draw(preview[j], NULL);
	}
}

void dialog_select_preview (GtkWidget *widget, s_info *n_info)
{
	memcpy(&(info[0]), n_info, sizeof(s_info));
	dialog_new_variations(widget, NULL);
	dialog_update_previews(widget, NULL);
}

/* File I/O stuff */

#define LOBITE(x) ((x)&0xff)
#define HIBITE(x) ((x)>>8)
#define MACBITES(x) (HIBITE(x)+LOBITE(x)<<8)
#define GETMACUSHORT(f) ((fgetc(f)<<8)+(fgetc(f)))
#define PUTMACUSHORT(u, f) fprintf(f, "%c%c", HIBITE(u), LOBITE(u));

int load_data(char *name)
{
	int i;
	FILE *f;
	f=fopen(name, "rb"); 
	if(f==NULL) return(0);
	for(i=0;i<MAX_TRANSFORMS;i++) info[0].transformSequence[i]=GETMACUSHORT(f);
	for(i=0;i<MAX_TRANSFORMS;i++) info[0].source[i]=GETMACUSHORT(f);
	for(i=0;i<MAX_TRANSFORMS;i++) info[0].control[i]=GETMACUSHORT(f);
	for(i=0;i<MAX_TRANSFORMS;i++) info[0].dest[i]=GETMACUSHORT(f);
	fclose(f);
	return(1);
}

void save_data(char *name)
{
	int i=0;
	FILE *f;

	f=fopen(name, "wb");
	for(i=0;i<MAX_TRANSFORMS;i++) PUTMACUSHORT(info[0].transformSequence[i], f);
	for(i=0;i<MAX_TRANSFORMS;i++) PUTMACUSHORT(info[0].source[i], f);
	for(i=0;i<MAX_TRANSFORMS;i++) PUTMACUSHORT(info[0].control[i], f);
	for(i=0;i<MAX_TRANSFORMS;i++) PUTMACUSHORT(info[0].dest[i], f);
	fclose(f);
}

void file_selection_save(GtkWidget *widget, GtkWidget *file_select)
{
	save_data(gtk_file_selection_get_filename (GTK_FILE_SELECTION(file_select)));
	gtk_widget_destroy(file_select);
}

void file_selection_load(GtkWidget *widget, GtkWidget *file_select)
{
	load_data(gtk_file_selection_get_filename (GTK_FILE_SELECTION(file_select)));
	gtk_widget_destroy(file_select);
	dialog_new_variations(widget, NULL);
	dialog_update_previews(widget, NULL);
}

void file_selection_cancel(GtkWidget *widget, GtkWidget *file_select)
{
	gtk_widget_destroy(file_select);
}

void dialog_load(GtkWidget *widget, gpointer d)
{
	GtkWidget *file_select;
	file_select = gtk_file_selection_new ("Load QBE file...");
	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_select)->ok_button), 
		"clicked", (GtkSignalFunc) file_selection_load, 
		(gpointer)file_select);
	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_select)->cancel_button), 
		"clicked", (GtkSignalFunc) file_selection_cancel, 
		(gpointer)file_select);
	gtk_widget_show(file_select);
}

void dialog_save(GtkWidget *widget, gpointer d)
{
	GtkWidget *file_select;
	file_select=gtk_file_selection_new("Save (middle transform) as QBE file...");
	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_select)->ok_button), 
		"clicked", (GtkSignalFunc) file_selection_save, 
		(gpointer)file_select);
	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_select)->cancel_button), 
		"clicked", (GtkSignalFunc) file_selection_cancel, 
		(gpointer)file_select);
	gtk_widget_show(file_select);
}


int dialog_create()
{
	GtkWidget *dialog;
	GtkWidget *button;
	GtkWidget *table;

	guchar *color_cube;
	gchar **argv;
	gint argc;

	int i;

	srand(time(NULL));

	argc = 1;
	argv = g_new (gchar *, 1);
	argv[0] = g_strdup ("video");

	gtk_init (&argc, &argv);
	gtk_rc_parse (gimp_gtkrc ());

	gdk_set_use_xshm (gimp_use_xshm ());
	gtk_preview_set_gamma (gimp_gamma ());
	gtk_preview_set_install_cmap (gimp_install_cmap ());
	color_cube = gimp_color_cube ();
	gtk_preview_set_color_cube (color_cube[0], color_cube[1], 
		color_cube[2], color_cube[3]);

	gtk_widget_set_default_visual (gtk_preview_get_visual ());
	gtk_widget_set_default_colormap (gtk_preview_get_cmap ());

	dialog=gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW(dialog), "G-Qbist 1.10");
	gtk_signal_connect (GTK_OBJECT (dialog), "destroy", 
		(GtkSignalFunc) dialog_close, 
		NULL);
                                              
	table=gtk_table_new (3, 3, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 5);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);
	gtk_container_border_width (GTK_CONTAINER (table), 5);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, TRUE, TRUE, 0);
	gtk_widget_show(table);

	memcpy((char *)&(info[0]), (char *)&qbist_info, sizeof(s_info));
	dialog_new_variations(NULL, NULL);
        
        for (i=0; i<9; i++) {

       		button=gtk_button_new();
		gtk_signal_connect (GTK_OBJECT (button), "clicked", 
			(GtkSignalFunc) dialog_select_preview, (gpointer) &(info[(i+5)%9]));
		gtk_table_attach(GTK_TABLE(table), button, i%3, (i%3)+1, i/3, (i/3)+1, 
			GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0); 		
		gtk_widget_show(button);

		preview[i] = gtk_preview_new(GTK_PREVIEW_COLOR);
		gtk_preview_size(GTK_PREVIEW(preview[i]), PREVIEW_SIZE, PREVIEW_SIZE);
		gtk_container_add(GTK_CONTAINER(button), preview[i]);
		gtk_widget_show(preview[i]);
	}

	dialog_update_previews(NULL, NULL);
	                                                                                                                          
	button = gtk_button_new_with_label ("OK");
	gtk_signal_connect (GTK_OBJECT (button), "clicked", 
		(GtkSignalFunc) dialog_ok, 
		(gpointer) dialog);
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
	gtk_widget_show (button);

	button = gtk_button_new_with_label ("Load");
	gtk_signal_connect(GTK_OBJECT(button), "clicked", 
		(GtkSignalFunc)dialog_load, 
		(gpointer)dialog);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
	gtk_widget_show(button);

	button = gtk_button_new_with_label ("Save");
	gtk_signal_connect(GTK_OBJECT(button), "clicked", 
		(GtkSignalFunc)dialog_save, 
		(gpointer)dialog);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
	gtk_widget_show(button);

	button = gtk_button_new_with_label ("Cancel");
	gtk_signal_connect (GTK_OBJECT (button), "clicked", 
		(GtkSignalFunc) dialog_cancel, 
		(gpointer) dialog);
	gtk_box_pack_end (GTK_BOX (GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default (button);
	gtk_widget_show (button);

	gtk_widget_show(dialog);

	result=FALSE;

	gtk_main();
	gdk_flush();

	if (result)
		memcpy((char *)&qbist_info, (char *)&(info[0]), sizeof(s_info));
	return result;
}
