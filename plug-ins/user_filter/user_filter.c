/*
 * Written 1997 Jens Ch. Restemeier <jchrr@hrz.uni-bielefeld.de>
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "gtk/gtk.h"
#include "libgimp/gimp.h"

#include "afstree.h"
#include "afsparse.h"

/*
 * PLUG-IN information
 */

#define NUM_SLIDERS 8
#define PLUG_IN_NAME "New Filter..."
#define PLUG_IN_PLACE "User Filters"
#define PLUG_IN_VERSION "1.0"
#define PLUG_IN_TYPE "<Image>/Filters/" PLUG_IN_PLACE "/" PLUG_IN_NAME

#define PREVIEW_SIZE 128

/*
 * Interface to GIMP 
 */

int sel_x1,sel_y1,sel_x2,sel_y2;
int img_height, img_width, img_bpp, img_has_alpha;

int tile_width, tile_height;
int read_tile_col, read_tile_row;
GTile *read_tile;

guchar bg_color[4];
GDrawable *drawable;

int dialog_create();
void load_afs(char *s);
void save_afs(char *s);

typedef struct _envir {
	/* environment */
	int storage[256];	/* 256 byte storage for put+get */
	int r; 			/* red at pixel position */
	int g;			/* green at pixel position */
	int b;			/* blue at pixel position */
	int a;			/* alpha at pixel postion */
	int c;			/* current color (=src(x,y,z)) */
	int i, u, v;		/* color int YUV space */
	int x;			/* x position */
	int y;			/* y position */
	int z;			/* current channel: 0=red, 1= green, 2=blue, 3=alpha */
	int X;			/* X-width of image */
	int Y;			/* Y-width of image */
	int Z;			/* number of channels of image */
	int d;			/* direction from center of the pic (-512 to 512) */
	int m;			/* distance to center of the pic */
	int M;			/* maximum distance from the center */
} s_envir;

/* functions */

inline int p_src(s_envir *e, int x, int y, int z);
inline int p_rad(s_envir *e, int d, int m, int z);
inline int p_val(s_envir *e, int i, int a, int b);
inline int p_ctl(s_envir *e, int i);
inline int p_dif(s_envir *e, int a, int b);
inline int p_put(s_envir *e, int v, int i);
inline int p_get(s_envir *e, int i);
inline int p_sin(s_envir *e, int i);
inline int p_cos(s_envir *e, int i);
inline int p_tan(s_envir *e, int i);
inline int p_sqr(s_envir *e, int i);
inline int p_cnv(s_envir *e, int m11, int m12, int m13, int m21, int m22, int m23, int m31, int m32, int m33, int d);
inline int p_div(s_envir *e, int a, int b);
inline int p_mod(s_envir *e, int a, int b);
inline int p_abs(s_envir *e, int a);
inline int p_rnd(s_envir *e, int a, int b);
inline int p_min(s_envir *e, int a, int b);
inline int p_max(s_envir *e, int a, int b);
inline int p_scl(s_envir *e, int a, int il, int ih, int ol, int oh);
inline int p_add(s_envir *e, int a, int b, int c);
inline int p_mix(s_envir *e, int a, int b, int n, int d);
inline int p_sub(s_envir *e, int a, int b, int c);
inline int p_c2d(s_envir *e, int x, int y);
inline int p_c2m(s_envir *e, int x, int y);
inline int p_r2x(s_envir *e, int d, int m);
inline int p_r2y(s_envir *e, int d, int m);

struct _data {
	int control[NUM_SLIDERS];
/* #ifndef COMPACT_MODE */
	char form[4][2048];
/* #endif */
} data;

/* formula for red/green/blue/alpha */
s_afstree *function_tree[4];

int calc_tree(s_envir *e, s_afstree *f)
{
	if (f==NULL) return 0;
	switch (f->op_type) {
		case OP_CONST: return f->value;
		case OP_ADD: return calc_tree(e, f->nodes[0]) + calc_tree(e, f->nodes[1]);
		case OP_SUB: return calc_tree(e, f->nodes[0]) - calc_tree(e, f->nodes[1]);
		case OP_MUL: return calc_tree(e, f->nodes[0]) * calc_tree(e, f->nodes[1]);
		case OP_AND: return calc_tree(e, f->nodes[0]) && calc_tree(e, f->nodes[1]);
		case OP_OR: return calc_tree(e, f->nodes[0]) || calc_tree(e, f->nodes[1]);
		case OP_EQ: return calc_tree(e, f->nodes[0]) == calc_tree(e, f->nodes[1]);
		case OP_NEQ: return calc_tree(e, f->nodes[0]) != calc_tree(e, f->nodes[1]);
		case OP_LT: return calc_tree(e, f->nodes[0]) < calc_tree(e, f->nodes[1]);
		case OP_LE: return calc_tree(e, f->nodes[0]) <= calc_tree(e, f->nodes[1]);
		case OP_BT: return calc_tree(e, f->nodes[0]) > calc_tree(e, f->nodes[1]);
		case OP_BE: return calc_tree(e, f->nodes[0]) >= calc_tree(e, f->nodes[1]);
		case OP_B_AND: return calc_tree(e, f->nodes[0]) & calc_tree(e, f->nodes[1]);
		case OP_B_OR: return calc_tree(e, f->nodes[0]) | calc_tree(e, f->nodes[1]);
		case OP_B_XOR: return calc_tree(e, f->nodes[0]) ^ calc_tree(e, f->nodes[1]);
		case OP_B_NOT: return ~calc_tree(e, f->nodes[0]); 
		case OP_VAR_r: return e->r;
		case OP_VAR_g: return e->g;
		case OP_VAR_b: return e->b;
		case OP_VAR_a: return e->a;
		case OP_VAR_c: return e->c;
		case OP_VAR_i: return e->i;
		case OP_VAR_u: return e->u;
		case OP_VAR_v: return e->v;
		case OP_VAR_x: return e->x;
		case OP_VAR_y: return e->y;
		case OP_VAR_z: return e->z;
		case OP_VAR_X: return e->X;
		case OP_VAR_Y: return e->Y;
		case OP_VAR_Z: return e->Z;
		case OP_VAR_d: return e->d;
		case OP_VAR_D: return 1024;
		case OP_VAR_m: return e->m;
		case OP_VAR_M: return e->M;
		case OP_VAR_mmin: return 0;
		case OP_VAR_dmin: return 0;
		case OP_NOT: return !calc_tree(e, f->nodes[0]);
		case F_SIN: return p_sin(e, calc_tree(e, f->nodes[0]));
		case F_COS: return p_cos(e, calc_tree(e, f->nodes[0]));
		case F_TAN: return p_tan(e, calc_tree(e, f->nodes[0]));
		case F_SQR: return p_sqr(e, calc_tree(e, f->nodes[0]));
		case F_CTL: return p_ctl(e, calc_tree(e, f->nodes[0]));
		case F_GET: return p_get(e, calc_tree(e, f->nodes[0]));
		case F_ABS: return p_abs(e, calc_tree(e, f->nodes[0]));
		case F_C2D: return p_c2d(e, calc_tree(e, f->nodes[0]),calc_tree(e, f->nodes[1]));
		case F_C2M: return p_c2m(e, calc_tree(e, f->nodes[0]),calc_tree(e, f->nodes[1]));
		case F_R2X: return p_r2x(e, calc_tree(e, f->nodes[0]),calc_tree(e, f->nodes[1]));
		case F_R2Y: return p_r2y(e, calc_tree(e, f->nodes[0]),calc_tree(e, f->nodes[1]));
		case OP_DIV: return p_div(e, calc_tree(e, f->nodes[0]),calc_tree(e, f->nodes[1]));
		case OP_MOD: return p_mod(e, calc_tree(e, f->nodes[0]),calc_tree(e, f->nodes[1]));
		case F_PUT: return p_put(e, calc_tree(e, f->nodes[0]),calc_tree(e, f->nodes[1]));
		case F_ADD: return p_add(e, calc_tree(e, f->nodes[0]),calc_tree(e, f->nodes[1]),calc_tree(e, f->nodes[2]));
		case F_MIX: return p_mix(e, calc_tree(e, f->nodes[0]),calc_tree(e, f->nodes[1]),calc_tree(e, f->nodes[2]),calc_tree(e, f->nodes[3]));
		case F_SUB: return p_sub(e, calc_tree(e, f->nodes[0]),calc_tree(e, f->nodes[1]),calc_tree(e, f->nodes[2]));
		case F_MIN: return p_min(e, calc_tree(e, f->nodes[0]),calc_tree(e, f->nodes[1]));
		case F_MAX: return p_max(e, calc_tree(e, f->nodes[0]),calc_tree(e, f->nodes[1]));
		case F_RND: return p_rnd(e, calc_tree(e, f->nodes[0]),calc_tree(e, f->nodes[1]));
		case F_DIF: return p_dif(e, calc_tree(e, f->nodes[0]),calc_tree(e, f->nodes[1]));
		case F_VAL: return p_val(e, calc_tree(e, f->nodes[0]),calc_tree(e, f->nodes[1]),calc_tree(e, f->nodes[2]));
		case F_SRC: return p_src(e, calc_tree(e, f->nodes[0]),calc_tree(e, f->nodes[1]),calc_tree(e, f->nodes[2]));
		case F_RAD: return p_rad(e, calc_tree(e, f->nodes[0]),calc_tree(e, f->nodes[1]),calc_tree(e, f->nodes[2]));
		case OP_COND: return calc_tree(e, f->nodes[0]) ? calc_tree(e, f->nodes[1]) : calc_tree(e, f->nodes[2]);
		case F_SCL: return p_scl(e, calc_tree(e, f->nodes[0]),calc_tree(e, f->nodes[1]),calc_tree(e, f->nodes[2]),calc_tree(e, f->nodes[3]),calc_tree(e, f->nodes[4]));
		case F_CNV: return p_cnv(e, 
				calc_tree(e, f->nodes[0]), calc_tree(e, f->nodes[1]), calc_tree(e, f->nodes[2]), 
				calc_tree(e, f->nodes[3]), calc_tree(e, f->nodes[4]), calc_tree(e, f->nodes[5]), 
				calc_tree(e, f->nodes[6]), calc_tree(e, f->nodes[7]), calc_tree(e, f->nodes[8]), 
				calc_tree(e, f->nodes[9]));
		case OP_COMMA: {
			calc_tree(e, f->nodes[0]);
			return calc_tree(e, f->nodes[1]);
		}
	}
	return 0;
}

/* calculate all pixel-relative information of the envir */
inline void calc_envir(s_envir *e) 
{
	e->d=p_c2d(e, e->x, e->y);
	e->m=p_c2m(e, e->x, e->y);
	e->r=p_src(e, e->x,e->y,0); 
	e->g=p_src(e, e->x,e->y,1); 
	e->b=p_src(e, e->x,e->y,2); 
	e->a=p_src(e, e->x,e->y,3);
}

inline int calc(s_envir *e, int channel)
{
	int v;
	/* prepare channel-specific values of envir */
	e->z=channel;
	e->c=p_src(e, e->x, e->y, e->z); 

	v=calc_tree(e, function_tree[channel]);

	/* scale */
	if (v<0) v=0; 
	if (v>255) v=255;
	return v;
}

inline int p_src(s_envir *e, int xx, int yy, int zz)
{
        int    col, row;
        int    coloff, rowoff;
        guchar *p;

	xx+=sel_x1; yy+=sel_y1;

	/* if outside visible image, use background color */
        if ((xx < 0) || (xx >= img_width) || (yy < 0) || (yy >= img_height)) {
		return(bg_color[zz]);
        }
        col    = xx / tile_width;
        coloff = xx % tile_width;
       	row    = yy / tile_height;
        rowoff = yy % tile_height;
        if ((col != read_tile_col) || (row != read_tile_row) || (read_tile == NULL)) {
        	if (read_tile != NULL) gimp_tile_unref(read_tile, FALSE);
		read_tile = gimp_drawable_get_tile(drawable, FALSE, row, col);
	        gimp_tile_ref(read_tile);
	        read_tile_col = col;
	        read_tile_row = row;
	}
	p = read_tile->data + img_bpp * (read_tile->ewidth * rowoff + coloff);
	if (zz < img_bpp) 
		return p[zz];
	else
		return bg_color[zz];
}

inline int p_rad(s_envir *e, int dd, int mm, int zz)
{ 
	return p_src(e, p_r2x(e,dd,mm), p_r2y(e,dd,mm), zz);
}

inline int p_cnv(s_envir *e, int m11, int m12, int m13, int m21, int m22, int m23, int m31, int m32, int m33, int dd)
{
	if (dd==0) return 255;
	return (m11*p_src(e, e->x-1,e->y-1, e->z) + m12*p_src(e, e->x,e->y-1,e->z) + m13*p_src(e, e->x+1,e->y-1,e->z) +
	        m21*p_src(e, e->x-1,e->y  , e->z) + m22*p_src(e, e->x,e->y  ,e->z) + m23*p_src(e, e->x+1,e->y  ,e->z) +
	        m31*p_src(e, e->x-1,e->y+1, e->z) + m32*p_src(e, e->x,e->y+1,e->z) + m33*p_src(e, e->x+1,e->y+1,e->z)) / dd;
}

inline int p_val(s_envir *e, int i, int a, int b)
{
	return ((data.control[i % NUM_SLIDERS]*(b-a))/255)+a;
}

inline int p_ctl(s_envir *e, int i)
{
	return data.control[i%NUM_SLIDERS];
}

inline int p_dif(s_envir *e, int a, int b)
{
	return abs(a-b);
}

inline int p_put(s_envir *e, int v, int i)
{
	e->storage[i % 256]=v;
	return v;
}

inline int p_get(s_envir *e, int i)
{
	return e->storage[i % 256];
}

inline int p_sin(s_envir *e, int i)
{
	return (sin(M_PI * ((float)i / 512.0))*512.0);
}

inline int p_cos(s_envir *e, int i)
{
	return (cos(M_PI * ((float)i / 512.0))*512.0);
}

inline int p_tan(s_envir *e, int i)
{
	/* This is "Windows mode" ! A Macintosh would return -1024-1024 ! */
	switch (i % 512) {
		case 0:	  return 0;
		case 256: return 512;
	}
	return (tan(M_PI * ((float)i / 512.0))*512.0);
}

inline int p_sqr(s_envir *e, int i)
{
	if (i<0) return 0;
	return sqrt(i);
}

inline int p_div(s_envir *e, int a, int b)
{
	return (b) ? a / b : 1;
}

inline int p_mod(s_envir *e, int a, int b)
{
	return (b) ? a % b : 0;
}

inline int p_abs(s_envir *e, int a)
{
	return a>0 ? a : -a;
}

inline int p_min(s_envir *e, int a, int b) 
{
	return (a<b) ? a : b;
}

inline int p_max(s_envir *e, int a, int b)
{
	return (a>b) ? a : b;
}

inline int p_rnd(s_envir *e, int a, int b)
{
	if (a==b) return a;
	return (rand() % (b-a)) + a;
}

inline int p_scl(s_envir *e, int a, int il, int ih, int ol, int oh)
{
	return (((a-il)*(oh-ol))/(ih-il))+ol;
}

inline int p_add(s_envir *e, int a, int b, int c)
{
	return p_min(e, a+b, c);
}

inline int p_mix(s_envir *e, int a, int b, int n, int d)
{
	if (d==0) return 0;
	return a*n/d+b*(d-n)/d;
}

inline int p_sub(s_envir *e, int a, int b, int c)
{
	return p_max(e, p_dif(e,a,b), c);
}

inline int p_c2d(s_envir *e, int x, int y)
{
	int xm, ym, d;
	xm=x-(e->X>>1); ym=y-(e->Y>>1);

	d=(ym>0) ? 512 : 0;
	if (xm!=0) d=(256.0 * (atan((float)ym / (float)abs(xm))/(M_PI/2.0)))+256;
	return (xm>0) ? d : -d;
}

inline int p_c2m(s_envir *e, int x, int y)
{
	int xm, ym;
	xm=x-(e->X>>1); ym=y-(e->Y>>1);
	return sqrt(xm*xm+ym*ym)+1;
}

inline int p_r2x(s_envir *e, int dd, int mm)
{
	return (e->X>>1) + (sin(M_PI * (float)dd / 512.0 )*(float)mm);
}

inline int p_r2y(s_envir *e, int dd, int mm)
{
	return (e->Y>>1) - (cos(M_PI * (float)dd / 512.0 )*(float)mm); 
}

/*
 * GIMP interface
 */

void query(void)
{
        static GParamDef args[] = {
		{ PARAM_INT32, "run_mode", "Interactive, non-interactive" },
		{ PARAM_IMAGE, "image", "Input image (unused)" },
		{ PARAM_DRAWABLE, "drawable", "Input drawable" }
	};

        static GParamDef *return_vals  = NULL;
        static int        nargs        = sizeof(args) / sizeof(args[0]);
        static int        nreturn_vals = 0;

        gimp_install_procedure(PLUG_IN_NAME,
                               "",
                               "",
                               "",
                               "",
                               PLUG_IN_VERSION,
                               PLUG_IN_TYPE,
                               "RGB*,GRAY*",
                               PROC_PLUG_IN,
                               nargs,
                               nreturn_vals,
                               args,
                               return_vals);
}

gint g_min(gint a, gint b) 
{
	return (a<b) ? a : b;
}
        
gint g_max(gint a, gint b)
{
	return (a>b) ? a : b;
}

void run(char *name, int nparams, GParam *param, int *nreturn_vals, GParam **return_vals)
{

        GParam		values[1];
        GRunModeType	run_mode;
        GStatusType	status;

	int c;

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

	gimp_palette_get_background(&(bg_color[0]), &(bg_color[1]), &(bg_color[2]));
	bg_color[3]=0;

	tile_width = gimp_tile_width ();
	tile_height = gimp_tile_height ();
	read_tile_col=read_tile_row=0;

	/* Set the tile cache size */
	gimp_tile_cache_ntiles(2 * (drawable->width + gimp_tile_width() - 1) / gimp_tile_width());

        gimp_drawable_mask_bounds(drawable->id, &sel_x1, &sel_y1, &sel_x2, &sel_y2);

	/* init function-trees */
	for (c=0; c<4; c++) 
		function_tree[4]=NULL;

	/* Possibly retrieve data */
	strcpy(data.form[0], "r");
	strcpy(data.form[1], "g");
	strcpy(data.form[2], "b");
	strcpy(data.form[3], "a");
	for (c=0; c<NUM_SLIDERS; c++) 
		data.control[c]=0;
	gimp_get_data(PLUG_IN_NAME, &data);

        switch (run_mode) {
                case RUN_INTERACTIVE: {
                        /* Get information from the dialog */
                        if (!dialog_create()) {
                                return;
 			}
                        break;
		}
                case RUN_NONINTERACTIVE:
                        break;
                case RUN_WITH_LAST_VALS:
                        /* Possibly retrieve data */
                        gimp_get_data(PLUG_IN_NAME, &data);
                        break;
                default:
                        break;
        } 

        if ((status == STATUS_SUCCESS) && gimp_drawable_color(drawable->id)) {
		int max_progress, progress;
		int i,j,k;
		int xs, ys;
		int write_tile_col, write_tile_row;
		s_envir e;
		GTile *write_tile;

		gimp_progress_init ("Filter plugin ...");
          	
          	max_progress=sel_y2-sel_y1;
          	progress=0;

		/* translate formula into tree */
		for (i=0; i<4; i++) {
			function_tree[i]=get_afstree(data.form[i]);
		}

		xs=sel_x2-sel_x1; ys=sel_y2-sel_y1;
		e.M=sqrt(xs*xs+ys*ys)/2;
          	e.X=xs; 
          	e.Y=ys; 
          	e.Z=img_bpp;

          	max_progress=((sel_y2-sel_y1) / tile_height) * ((sel_x2-sel_x1) / tile_width);
          	progress=0;
          	
		for (write_tile_row=(sel_y1 / tile_height); write_tile_row<=((sel_y2-1) / tile_height); write_tile_row++) {
        	  	for (write_tile_col=(sel_x1 / tile_width); write_tile_col<=((sel_x2-1) / tile_width); write_tile_col++) {
				gint t_max_x, t_min_x, t_max_y, t_min_y;

          			write_tile = gimp_drawable_get_tile( drawable, TRUE, write_tile_row, write_tile_col );
				gimp_tile_ref(write_tile);
				
				t_min_x=g_max(0, sel_x1 - (write_tile_col * tile_height));
				t_max_x=g_min(write_tile->ewidth, sel_x2 - (write_tile_col * tile_width));
				t_min_y=g_max(0, sel_y1 - (write_tile_row * tile_height));
				t_max_y=g_min(write_tile->eheight, sel_y2 - (write_tile_row * tile_height));
				
          			for (i=t_min_y; i<t_max_y; i++) {
					for (j=t_min_x; j<t_max_x; j++) {
						guchar *c; 

						c=write_tile->data+(((i * write_tile->ewidth)+j) * img_bpp);

						e.x=j+(write_tile_col*tile_width)-sel_x1;
						e.y=i+(write_tile_row*tile_height)-sel_y1;
						calc_envir(&e);
						for (k=0; k<img_bpp;k++) {
							if (k<4) {
								c[k]=calc(&e, k);
							} else {
								c[k]=bg_color[k];
							}							
						}
					}
				}
				
				gimp_progress_update((double)progress / (double)max_progress);
				progress++;
				gimp_tile_unref (write_tile, TRUE);
			}
		}

		/* destroy tree */
		for (i=0; i<4; i++) {
			if (function_tree[i]!=NULL) 
				free_tree(function_tree[i]);
		}

		gimp_drawable_flush (drawable);
		gimp_drawable_merge_shadow (drawable->id, TRUE);
		gimp_drawable_update (drawable->id, sel_x1, sel_y1, (sel_x2 - sel_x1), (sel_y2 - sel_y1));
		            				
                /* If run mode is interactive, flush displays */
                if (run_mode != RUN_NONINTERACTIVE)
                        gimp_displays_flush();

                /* Store data */
                if (run_mode == RUN_INTERACTIVE)
                        gimp_set_data(PLUG_IN_NAME, &data, sizeof(data));
        } else if (status == STATUS_SUCCESS)
                status = STATUS_EXECUTION_ERROR;
        values[0].data.d_status = status;

	if (read_tile!=NULL) 
		gimp_tile_unref(read_tile, FALSE);
        gimp_drawable_detach(drawable);
}

GPlugInInfo PLUG_IN_INFO = {
	NULL,   /* init_proc */
	NULL,   /* quit_proc */
	query,  /* query_proc */
	run     /* run_proc */
};

MAIN();

/*
 * GTK user interface
 */

GtkWidget *dialog;
GtkObject *adjustment[NUM_SLIDERS];
GtkWidget *preview;
GtkWidget *entry[4];

int result, preview_enable;

/* prototypes for signals */
void dialog_update_preview(GtkWidget *widget, gpointer data);
void file_selection_load(GtkWidget *widget, GtkWidget *file_select);
void file_selection_save(GtkWidget *widget, GtkWidget *file_select);
void dialog_load(GtkWidget *widget, gpointer d);
void dialog_save(GtkWidget *widget, gpointer d);
void dialog_close(GtkWidget *widget, gpointer data);
void dialog_cancel(GtkWidget *widget, gpointer data);
void dialog_button_preview_update (GtkWidget *widget, gpointer data);
void dialog_ok(GtkWidget *widget, gpointer data);
void dialog_slider_update (GtkWidget *widget, int *data);

void file_selection_load(GtkWidget *widget, GtkWidget *file_select)
{
	int tmp_preview_enable;
	gtk_widget_set_sensitive (dialog, TRUE);
	tmp_preview_enable=preview_enable; preview_enable=0;
	load_afs(gtk_file_selection_get_filename (GTK_FILE_SELECTION (file_select)));
	preview_enable=tmp_preview_enable;
	gtk_widget_destroy(file_select);
	dialog_update_preview(preview, NULL);
}

void file_selection_save(GtkWidget *widget, GtkWidget *file_select)
{
	gtk_widget_set_sensitive (dialog, TRUE);
	save_afs(gtk_file_selection_get_filename (GTK_FILE_SELECTION (file_select)));
	gtk_widget_destroy(file_select);
}

void file_selection_cancel(GtkWidget *widget, GtkWidget *file_select)
{
	gtk_widget_destroy(file_select);
	gtk_widget_set_sensitive (dialog, TRUE);
}

void dialog_reset(GtkWidget *widget, gpointer d)
{
	int n, tmp_preview_enable;
	tmp_preview_enable=preview_enable;
	preview_enable=0;
	for (n=0; n<NUM_SLIDERS; n++) {
		GTK_ADJUSTMENT(adjustment[n])->value=0;
		gtk_signal_emit_by_name(GTK_OBJECT(adjustment[n]),"value_changed");
	}
	gtk_entry_set_text(GTK_ENTRY(entry[0]), "r");
	gtk_entry_set_text(GTK_ENTRY(entry[1]), "g");
	gtk_entry_set_text(GTK_ENTRY(entry[2]), "b");
	gtk_entry_set_text(GTK_ENTRY(entry[3]), "a");
	preview_enable=tmp_preview_enable;
	dialog_update_preview(preview, NULL);
}

void dialog_load(GtkWidget *widget, gpointer d)
{
	GtkWidget *file_select;
	file_select = gtk_file_selection_new ("Load AFS file...");
	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_select)->ok_button),
		"clicked", (GtkSignalFunc) file_selection_load,
		(gpointer)file_select);
	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_select)->cancel_button),
		"clicked", (GtkSignalFunc) file_selection_cancel,
		(gpointer)file_select);
	gtk_widget_show(file_select);
	gtk_widget_set_sensitive (dialog, FALSE);
}

void dialog_save(GtkWidget *widget, gpointer d)
{
	GtkWidget *file_select;
	file_select = gtk_file_selection_new ("Load AFS file...");
	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_select)->ok_button),
		"clicked", (GtkSignalFunc) file_selection_save,
		(gpointer)file_select);
	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_select)->cancel_button),
		"clicked", (GtkSignalFunc) file_selection_cancel,
		(gpointer)file_select);
	gtk_widget_show(file_select);
	gtk_widget_set_sensitive (dialog, FALSE);
}

void load_afs(char *s)
{
	FILE *f;
	char buffer[2048];
	int i,c,n;
	if ((f=fopen(s, "r"))==NULL) 
		return;
	/* read header */
	c=0; while ((i=fgetc(f))!='\r') { buffer[c++]=i; if (feof(f)) return; } buffer[c]=0;
	/* read sliders */
	for (n=0; n<8; n++) {
		c=0; while ((i=fgetc(f))!='\r') { buffer[c++]=i;  if (feof(f)) return; } buffer[c]=0;

		GTK_ADJUSTMENT(adjustment[n])->value=atoi(buffer);
		gtk_signal_emit_by_name(GTK_OBJECT(adjustment[n]),"value_changed");
	}
	/* read channels */
	for (n=0; n<4; n++) {
		c=0; 
		do {
			i=fgetc(f);
			/* one \r -> skip, two \r -> end of line */
			if (i=='\r') i=fgetc(f);
			if (i!='\r') buffer[c++]=i;
			if (feof(f)) return;
		} while (i!='\r');
		buffer[c]=0;

		gtk_entry_set_text(GTK_ENTRY(entry[n]), buffer);
	}
}

void save_afs(char *s)
{
	FILE *f;
	int i;
	if ((f=fopen(s, "w"))==NULL) 
		return;
	/* write header */
	fprintf(f, "%%RGB-1.0\r");
	/* write sliders */
	for (i=0; i<8; i++) {
		if (i<NUM_SLIDERS) {
			int w;
			w=GTK_ADJUSTMENT(adjustment[i])->value;
			fprintf(f,"%i\r", w);
		} else {
			fprintf(f,"0\r");
		}
	}
	/* write channels */
	for (i=0; i<4; i++) {
		fprintf(f, "%s\r\r", gtk_entry_get_text(GTK_ENTRY(entry[i])));
	}
	fclose(f);
}

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

void dialog_button_preview_update (GtkWidget *widget, gpointer data)
{
	preview_enable=GTK_TOGGLE_BUTTON(widget)->active;
}

void dialog_update_preview(GtkWidget *widget, gpointer unused)
{
	int i,j;
	guchar buf[PREVIEW_SIZE*3];
	s_envir e;

	for (i=0; i<4; i++) {
		function_tree[i]=get_afstree(data.form[i]);
	}

	e.X=e.Y=PREVIEW_SIZE;
	e.Z=img_bpp;
	e.M=sqrt(PREVIEW_SIZE*PREVIEW_SIZE*2)/2;
	for (i = 0; i < PREVIEW_SIZE; i++) {
		for (j=0; j < PREVIEW_SIZE; j++) {
			e.x=j; e.y=i;
			calc_envir(&e);
			buf[j*3+0]=calc(&e, 0); 
			buf[j*3+1]=calc(&e, 1); 
			buf[j*3+2]=calc(&e, 2);
		}
		gtk_preview_draw_row (GTK_PREVIEW (preview), buf, 0, i, PREVIEW_SIZE);
	}

	for (i=0; i<4; i++) {
		if (function_tree[i]!=NULL) 
			free_tree(function_tree[i]);
	}

	gtk_widget_draw(preview, NULL);
	gdk_flush();
}

void dialog_slider_update (GtkWidget *widget, int *data)
{
	*data=GTK_ADJUSTMENT(widget)->value;
	if (preview_enable) 
		dialog_update_preview(preview, NULL);
}

void dialog_entry_update (GtkWidget *widget, char *data)
{
	strcpy(data, gtk_entry_get_text(GTK_ENTRY(widget)));
}

int dialog_create()
{
	GtkWidget *button;
	GtkWidget *hbox, *vbox;
	GtkWidget *frame;

	guchar *color_cube;
	char **argv;
	int argc;

	int i;
	srandom(time(NULL));

	argc = 1;
	argv = g_new (char *, 1);
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
	gtk_window_set_title (GTK_WINDOW(dialog), PLUG_IN_NAME);
	gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		(GtkSignalFunc) dialog_close,
		NULL);
	
	hbox=gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	/*
	 * Preview area
	 */
	frame=gtk_frame_new("Preview");
	gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);
	vbox=gtk_vbox_new(FALSE,5);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_widget_show(vbox);

	button=gtk_button_new();
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
		(GtkSignalFunc) dialog_update_preview, NULL);
	gtk_box_pack_start (GTK_BOX (vbox), button , FALSE, FALSE, 0);
	gtk_widget_show(button);	

	preview=gtk_preview_new(GTK_PREVIEW_COLOR);
	gtk_preview_size(GTK_PREVIEW(preview), PREVIEW_SIZE, PREVIEW_SIZE);
	gtk_container_add(GTK_CONTAINER(button), preview);
	gtk_widget_show (preview);

	button=gtk_check_button_new_with_label("enable update");
	gtk_signal_connect (GTK_OBJECT (button), 
		"toggled", 
		(GtkSignalFunc) dialog_button_preview_update,
		0);
	gtk_box_pack_end (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	gtk_widget_show (button);
	preview_enable=1;
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button), preview_enable);

	/*
	 * Slider area
	 */
	frame=gtk_frame_new("Values");
	gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);
	gtk_widget_set_usize(frame, 256, -1);
	gtk_widget_show(frame);
	vbox=gtk_vbox_new(FALSE,5);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_widget_show(vbox);

	for (i=0; i<NUM_SLIDERS; i++) {
		GtkWidget *scale;
		adjustment[i]=gtk_adjustment_new (data.control[i], 0.0, 255.0, 1.0, 1.0, 0.0);
		gtk_signal_connect(GTK_OBJECT(adjustment[i]),
			"value_changed",
			(GtkSignalFunc)dialog_slider_update ,
			(gpointer)&(data.control[i]));
 		        
		scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment[i]));
		gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
		gtk_scale_set_draw_value(GTK_SCALE(scale), TRUE);
		gtk_box_pack_start (GTK_BOX (vbox), scale, TRUE, TRUE, 0);
		gtk_widget_show(scale);
	}

/* #ifndef COMPACT_MODE */
	/* entry area for formula */
	for (i=0; i<4; i++) {
		entry[i]=gtk_entry_new();
		gtk_signal_connect(GTK_OBJECT(entry[i]),
			"changed",
			(GtkSignalFunc)dialog_entry_update ,
			(gpointer)(data.form[i]));
		gtk_entry_set_text(GTK_ENTRY(entry[i]), data.form[i]);
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), entry[i], FALSE, FALSE, 0);
		gtk_widget_show(entry[i]);
	}

	hbox=gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	button = gtk_button_new_with_label ("Reset");
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
		(GtkSignalFunc) dialog_reset, (gpointer) dialog);
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
	gtk_widget_show (button);

	button = gtk_button_new_with_label ("Load...");
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
		(GtkSignalFunc) dialog_load, (gpointer) dialog);
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
	gtk_widget_show (button);

	button = gtk_button_new_with_label ("Save ...");
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
		(GtkSignalFunc) dialog_save, (gpointer) dialog);
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
	gtk_widget_show (button);
/* #endif */

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

	dialog_update_preview(preview, NULL);

	result=0;
	
	gtk_main();

	gdk_flush();

	return result;
}
