/*
 * Written 1997 Jens Ch. Restemeier <jchrr@hrz.uni-bielefeld.de>
 * This program is based on an algorithm / article by
 * Jörn Loviscach.
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*
 * History:
 * 1.0 first release
 * 1.2 now handles RGB*
 * 1.5 fixed a small bug
 * 1.6 fixed a bug that was added by v1.5 :-(
 * 1.7 added patch from Art Haas to make it compile with HP-UX, a small clean-up
 */
                 
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "gtk/gtk.h"
#include "libgimp/gimp.h"

/** qbist renderer ***********************************************************/

#define MAX_TRANSFORMS	36
#define NUM_TRANSFORMS	9
#define NUM_REGISTERS	6

#define PLUG_IN_NAME "plug_in_qbist"
#define PLUG_IN_VERSION "August 1997, 1.6"

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

static void query(void);
static void run(
	char    *name,
	int	nparams,
	GParam	*param,
	int	*nreturn_vals,
	GParam	**return_vals);

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
	for(k=0; k<MAX_TRANSFORMS; k++) {
		info->transformSequence[k]=rand() % NUM_TRANSFORMS;
		info->source[k]=rand() % NUM_REGISTERS;
		info->control[k]=rand() % NUM_REGISTERS;
		info->dest[k]=rand() % NUM_REGISTERS;
	}
	info->dest[rand() % MAX_TRANSFORMS]=0;
}

void modify_info(s_info *o_info, s_info *n_info)
{
	int k,n; 
	memcpy(n_info, o_info, sizeof(s_info)); 
	n=rand() % MAX_TRANSFORMS;
	for(k=0;k<n; k++) {
		switch (rand() % 4) {
			case 0: n_info->transformSequence[rand() % MAX_TRANSFORMS] = rand() % NUM_TRANSFORMS; break;
			case 1: n_info->source[rand() % MAX_TRANSFORMS]            = rand() % NUM_REGISTERS; break;
			case 2: n_info->control[rand() % MAX_TRANSFORMS]           = rand() % NUM_REGISTERS; break;
			case 3: n_info->dest[rand() % MAX_TRANSFORMS]              = rand() % NUM_REGISTERS; break;
		}
	}
}

void qbist(s_info info, gchar *buffer, int xp, int yp, int num, int width, int height, int bpp)
{
	gushort gx;
	vreg reg [NUM_REGISTERS];
	int i;

	if (num<=0) return;

	for(gx=0; gx<num; gx ++) {
		for(i=0; i<NUM_REGISTERS; i++) {
			reg[i][0] = ((float)gx+xp) / ((float)(width-1));
			reg[i][1] = ((float)yp) / ((float)(height-1));
			reg[i][2] = ((float)i) / ((float)NUM_REGISTERS);
		}
		for(i=0;i<MAX_TRANSFORMS; i++) {
			gushort sr,cr,dr;
			sr=info.source[i];cr=info.control[i];dr=info.dest[i];
			
			switch (info.transformSequence[i]) {
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
					if (reg[dr][0] < 0.0) reg[dr][0] += 1.0;
					reg[dr][1] = reg[sr][1]-reg[cr][1];
					if (reg[dr][1] < 0.0) reg[dr][1] += 1.0;
					reg[dr][2] = reg[sr][2]-reg[cr][2];
					if (reg[dr][2] < 0.0) reg[dr][2] += 1.0;
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
				buffer[i]=floor(255.0 * reg[0][i]);
			} else {
				buffer[i]=255;
			}
		}
		buffer+=bpp;
	}
}

/** Plugin interface *********************************************************/

GPlugInInfo PLUG_IN_INFO = {
	NULL,   /* init_proc */
	NULL,   /* quit_proc */
	query,  /* query_proc */
	run     /* run_proc */
};

MAIN();

static void
query(void)
{
        static GParamDef args[] = {
		{ PARAM_INT32, "run_mode", "Interactive, non-interactive" },
		{ PARAM_IMAGE, "image", "Input image (unused)" },
		{ PARAM_DRAWABLE, "drawable", "Input drawable" }
	}; /* args */

        static GParamDef *return_vals  = NULL;
        static int        nargs        = sizeof(args) / sizeof(args[0]);
        static int        nreturn_vals = 0;

        gimp_install_procedure(PLUG_IN_NAME,
                               "Create images based on a random genetic formula",
                               "This Plug-in is based on an article by "
                               "Jörn Loviscach. It generates modern art "
                               "pictures from a random genetic formula.",
                               "Jörn Loviscach, Jens Ch. Restemeier",
                               "Jörn Loviscach, Jens Ch. Restemeier",
                               PLUG_IN_VERSION,
                               "<Image>/Filters/Render/Qbist",
                               "RGB*",
                               PROC_PLUG_IN,
                               nargs,
                               nreturn_vals,
                               args,
                               return_vals);
} /* query */

gint g_min(gint a, gint b) 
{
	return (a<b) ? a : b;
}

gint g_max(gint a, gint b)
{
	return (a>b) ? a : b;
}

static void
run(char    *name,
    int      nparams,
    GParam  *param,
    int     *nreturn_vals,
    GParam **return_vals)
{
	gint sel_x1,sel_y1,sel_x2,sel_y2;
	gint img_height, img_width, img_bpp, img_has_alpha;

        GParam		values[1];
	GDrawable 	*drawable;
        GRunModeType	run_mode;
        GStatusType	status;

        status   = STATUS_SUCCESS;
        run_mode = param[0].data.d_int32;

        values[0].type          = PARAM_STATUS;
        values[0].data.d_status = status;

        *nreturn_vals = 1;
        *return_vals  = values;
        drawable = gimp_drawable_get(param[2].data.d_drawable);

        img_width     = gimp_drawable_width(drawable->id);
        img_height    = gimp_drawable_height(drawable->id);
        img_bpp       = gimp_drawable_bpp(drawable->id);
        img_has_alpha = gimp_drawable_has_alpha(drawable->id);

        gimp_drawable_mask_bounds(drawable->id, &sel_x1, &sel_y1, &sel_x2, &sel_y2);

	create_info(&qbist_info);

        switch (run_mode) {
                case RUN_INTERACTIVE:
                        /* Possibly retrieve data */
                        gimp_get_data(PLUG_IN_NAME, &qbist_info);
                        /* Get information from the dialog */

                        if (!dialog_create())
                                return;

                        break;

                case RUN_NONINTERACTIVE:
                        break;

                case RUN_WITH_LAST_VALS:
                        /* Possibly retrieve data */
                        gimp_get_data(PLUG_IN_NAME, &qbist_info);
                        break;
                default:
                        break;
        } 

        if ((status == STATUS_SUCCESS) && gimp_drawable_color(drawable->id)) {
		GTile *tile=NULL;
		gint col, row, line, tile_width, tile_height;
		gint max_progress, progress;

		tile_width = gimp_tile_width ();
		tile_height = gimp_tile_height ();
        
                /* Set the tile cache size */
                gimp_tile_cache_ntiles(2 * (drawable->width + gimp_tile_width() - 1) / gimp_tile_width());

                /* Run! */

		gimp_progress_init ("Qbist ...");
          	
          	max_progress=((sel_y2-sel_y1) / tile_height) * ((sel_x2-sel_x1) / tile_width);
          	progress=0;
          	
		for (row=(sel_y1 / tile_height); row<=((sel_y2-1) / tile_height); row++) {
        	  	for (col=(sel_x1 / tile_width); col<=((sel_x2-1) / tile_width); col++) {
				gint t_max_x, t_min_x, t_max_y, t_min_y;

          			tile = gimp_drawable_get_tile( drawable, TRUE, row, col );
				gimp_tile_ref(tile);
				
				t_min_x=g_max(0, sel_x1 - (col * tile_height));
				t_max_x=g_min(tile->ewidth, sel_x2 - (col * tile_width));
				t_min_y=g_max(0, sel_y1 - (row * tile_height));
				t_max_y=g_min(tile->eheight, sel_y2 - (row * tile_height));
				
          			for (line=t_min_y; line<t_max_y; line++) {
					qbist(qbist_info, tile->data + (((line * tile->ewidth)+t_min_x) * img_bpp) , (col * tile_width) + t_min_x, (row * tile_height) + line, t_max_x - t_min_x, img_width, img_height, img_bpp);
				}
				
				gimp_progress_update((double)progress / (double)max_progress);
				progress++;
				gimp_tile_unref (tile, TRUE);
			}
		}

		gimp_drawable_flush (drawable);
		gimp_drawable_merge_shadow (drawable->id, TRUE);
		gimp_drawable_update (drawable->id, sel_x1, sel_y1, (sel_x2 - sel_x1), (sel_y2 - sel_y1));
		            				
                /* If run mode is interactive, flush displays */
                if (run_mode != RUN_NONINTERACTIVE)
                        gimp_displays_flush();

                /* Store data */
                if (run_mode == RUN_INTERACTIVE)
                        gimp_set_data(PLUG_IN_NAME, &qbist_info, sizeof(s_info));
        } else if (status == STATUS_SUCCESS)
                status = STATUS_EXECUTION_ERROR;
        values[0].data.d_status = status;

        gimp_drawable_detach(drawable);
} /* run */


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
	result=FALSE;
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
		modify_info(&(info[0]),&(info[i]));
}

void dialog_update_previews(GtkWidget *widget, gpointer data)
{
	int i,j;
	guchar buf[PREVIEW_SIZE * 3];
	for (j=0;j<9;j++) {
		for (i = 0; i < PREVIEW_SIZE; i++) {
			qbist(info[(j+5) % 9], buf, 0, i, PREVIEW_SIZE, PREVIEW_SIZE, PREVIEW_SIZE, 3);
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
	gtk_window_set_title (GTK_WINDOW(dialog), "G-Qbist 1.0");
	gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		(GtkSignalFunc) dialog_close,
		NULL);
                                              
	table=gtk_table_new (3,3,FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 5);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);
	gtk_container_border_width (GTK_CONTAINER (table),5);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, FALSE, FALSE, 0);
	gtk_widget_show(table);

	memcpy((char *)&(info[0]),(char *)&qbist_info,sizeof(s_info));
	dialog_new_variations(NULL, NULL);
        
        for (i=0; i<9; i++) {

       		button=gtk_button_new();
		gtk_signal_connect (GTK_OBJECT (button), "clicked",
			(GtkSignalFunc) dialog_select_preview, (gpointer) &(info[(i+5)%9]));
		gtk_table_attach(GTK_TABLE(table),button,i%3,(i%3)+1,i/3,(i/3)+1,
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
		(GtkSignalFunc) dialog_ok, (gpointer) dialog);
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
	gtk_widget_show (button);

	button = gtk_button_new_with_label ("Cancel");
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
		(GtkSignalFunc) dialog_cancel, (gpointer) dialog);
	gtk_box_pack_end (GTK_BOX (GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default (button);
	gtk_widget_show (button);

	gtk_widget_show(dialog);

	result=0;
	
	gtk_main();
	gdk_flush();

	if (result)
		memcpy((char *)&qbist_info,(char *)&(info[0]),sizeof(s_info));
	return result;
}
