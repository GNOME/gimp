/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Bump map plug-in --- emboss an image by using another image as a bump map
 * Copyright (C) 1997 Federico Mena Quintero
 * federico@nuclecu.unam.mx
 * Copyright (C) 1997 Jens Lautenbacher
 * jens@lemming0.lem.uni-karlsruhe.de
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


/* This plug-in uses the algorithm described by John Schlag, "Fast
 * Embossing Effects on Raster Image Data" in Graphics Gems IV (ISBN
 * 0-12-336155-9).  It takes a grayscale image to be applied as a
 * bump-map to another image, producing a nice embossing effect.
 */

/* Version 2.04:
 *
 * - The preview is now scrollable via draging with button 1 in the
 * preview area. Thanks to Quartic for helping with gdk event handling.
 *
 * - The bumpmap's offset can alternatively be adjusted by dragging with
 * button 3 in the preview area.
 */
 
/* Version 2.03:
 *
 * - Now transparency in the bumpmap drawable is handled as specified
 * by the waterlevel parameter.  Thanks to Jens for suggesting it!
 *
 * - New cool ambient lighting method.  Thanks to Jens Lautenbacher
 * for creating it!  Something useful actually came out of those IRC
 * sessions ;-)
 *
 * - Added proper rounding wherever it seemed appropriate.  This fixes
 * some minor artifacts in the output.
 */


/* Version 2.02:
 *
 * - Fixed a stupid bug in the preview code (offsets were not wrapped
 * correctly in some situations).  Thanks to Jens Lautenbacher for
 * reporting it!
 */


/* Version 2.01:
 *
 * - For the preview, vertical scrolling and setting the vertical
 * bumpmap offset are now *much* faster.  Instead of calling
 * gimp_pixel_rgn_get_row() a lot of times, I now use an adapted
 * version of gimp_pixel_rgn_get_rect().
 */


/* Version 2.00:
 *
 * - Rewrote from the 0.54 version (well, from the 0.99.9
 * distribution, actually...).  New in this release are the correct
 * handling of all image depths, sizes, and offsets.  Also the
 * different map types, the compensation and map inversion options
 * were added.  The preview widget is new, too.
 */


/* TODO:
 *
 * - Speed-ups
 */

#include "config.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include "config.h"
#include "libgimp/stdplugins-intl.h"


/***** Magic numbers *****/

#define PLUG_IN_NAME    "plug_in_bump_map"
#define PLUG_IN_VERSION "August 1997, 2.04"

#define PREVIEW_SIZE 128
#define SCALE_WIDTH  200
#define ENTRY_WIDTH  60

#define CHECK_SIZE  8
#define CHECK_DARK  ((int) (1.0 / 3.0 * 255))
#define CHECK_LIGHT ((int) (2.0 / 3.0 * 255))


/***** Types *****/

enum {
	LINEAR = 0,
	SPHERICAL,
	SINUOSIDAL
};

enum {
	DRAG_NONE = 0,
	DRAG_SCROLL,
	DRAG_BUMPMAP
};

typedef struct {
	gint32  bumpmap_id;
	gdouble azimuth;
	gdouble elevation;
	gint    depth;
	gint    xofs;
	gint    yofs;
	gint    waterlevel;
	gint    ambient;
	gint    compensate;
	gint    invert;
	gint    type;
} bumpmap_vals_t;

typedef struct {
	int    lx, ly;       /* X and Y components of light vector */
	int    nz2, nzlz;    /* nz^2, nz*lz */
	int    background;   /* Shade for vertical normals */
	double compensation; /* Background compensation */
	guchar lut[256];     /* Look-up table for modes */
} bumpmap_params_t;

typedef struct {
	GtkWidget  *preview;
	int         preview_width;
	int         preview_height;
        int         mouse_x;
	int         mouse_y;
        int         preview_xofs;
        int         preview_yofs;
        int         drag_mode;
	
	guchar     *check_row_0;
	guchar     *check_row_1;

	guchar    **src_rows;
	guchar    **bm_rows;

	int         src_yofs;
	int         bm_yofs;

	GDrawable  *bm_drawable;
	int         bm_width;
	int         bm_height;
	int         bm_bpp;
	int         bm_has_alpha;

	GPixelRgn   src_rgn;
	GPixelRgn   bm_rgn;

	bumpmap_params_t params;

	gint        run;
} bumpmap_interface_t;


/***** Prototypes *****/

static void query(void);
static void run(char    *name,
		int      nparams,
		GParam  *param,
		int     *nreturn_vals,
		GParam **return_vals);

static void bumpmap(void);
static void bumpmap_init_params(bumpmap_params_t *params);
static void bumpmap_row(guchar           *src_row,
			guchar           *dest_row,
			int               width,
			int               bpp,
			int               has_alpha,
			guchar           *bm_row1,
			guchar           *bm_row2,
			guchar           *bm_row3,
			int               bm_width,
			int               bm_xofs,
			bumpmap_params_t *params);
static void bumpmap_convert_row(guchar *row, int width, int bpp, int has_alpha, guchar *lut);

static gint bumpmap_dialog(void);
static void dialog_init_preview(void);
static void dialog_new_bumpmap(void);
static void dialog_update_preview(void);
static gint dialog_preview_events(GtkWidget *widget, GdkEvent *event);
static void dialog_scroll_src(void);
static void dialog_scroll_bumpmap(void);
static void dialog_get_rows(GPixelRgn *pr, guchar **rows, int x, int y, int width, int height);
static void dialog_fill_src_rows(int start, int how_many, int yofs);
static void dialog_fill_bumpmap_rows(int start, int how_many, int yofs);

static void dialog_compensate_callback(GtkWidget *widget, gpointer data);
static void dialog_invert_callback(GtkWidget *widget, gpointer data);
static void dialog_map_type_callback(GtkWidget *widget, gpointer data);
static gint dialog_constrain(gint32 image_id, gint32 drawable_id, gpointer data);
static void dialog_bumpmap_callback(gint32 id, gpointer data);
static void dialog_create_dvalue(char *title, GtkTable *table, int row, gdouble *value,
				 double left, double right);
static void dialog_dscale_update(GtkAdjustment *adjustment, gdouble *value);
static void dialog_dentry_update(GtkWidget *widget, gdouble *value);
static void dialog_create_ivalue(char *title, GtkTable *table, int row, gint *value,
				 int left, int right, int full_update);
static void dialog_iscale_update_normal(GtkAdjustment *adjustment, gint *value);
static void dialog_iscale_update_full(GtkAdjustment *adjustment, gint *value);
static void dialog_ientry_update_normal(GtkWidget *widget, gint *value);
static void dialog_ientry_update_full(GtkWidget *widget, gint *value);
static void dialog_ok_callback(GtkWidget *widget, gpointer data);
static void dialog_cancel_callback(GtkWidget *widget, gpointer data);
static void dialog_close_callback(GtkWidget *widget, gpointer data);


/***** Variables *****/

GPlugInInfo PLUG_IN_INFO = {
	NULL,   /* init_proc */
	NULL,   /* quit_proc */
	query,  /* query_proc */
	run     /* run_proc */
}; /* PLUG_IN_INFO */

static bumpmap_vals_t bmvals = {
	-1,     /* bumpmap_id */
	135.0,  /* azimuth */
	45.0,   /* elevation */
	3,      /* depth */
	0,      /* xofs */
	0,      /* yofs */
	0,      /* waterlevel */
	0,      /* ambient */
	FALSE,  /* compensate */
	FALSE,  /* invert */
	LINEAR  /* type */
}; /* bmvals */

static bumpmap_interface_t bmint = {
	NULL,      /* preview */
	0,         /* preview_width */
	0,         /* preview_height */
	0,         /* mouse_x */
	0,         /* mouse_y */
	0,         /* preview_xofs */
	0,         /* preview_yofs */
	DRAG_NONE, /* drag_mode */
	NULL,      /* check_row_0 */
	NULL,      /* check_row_1 */
	NULL,      /* src_rows */
	NULL,      /* bm_rows */
	0,         /* src_yofs */
	-1,        /* bm_yofs */
	NULL,      /* bm_drawable */
	0,         /* bm_width */
	0,         /* bm_height */
	0,         /* bm_bpp */
	0,         /* bm_has_alpha */
	{ 0 },     /* src_rgn */
	{ 0 },     /* bm_rgn */
	{ 0 },     /* params */
	FALSE      /* run */
}; /* bmint */

static char *map_types[] = {
	N_("Linear map"),
	N_("Spherical map"),
	N_("Sinusoidal map")
}; /* map_types */

GDrawable *drawable = NULL;

int sel_x1, sel_y1, sel_x2, sel_y2;
int sel_width, sel_height;
int img_bpp, img_has_alpha;


/***** Functions *****/

/*****/

MAIN()


/*****/

static void
query(void)
{
	static GParamDef args[] = {
		{ PARAM_INT32,    "run_mode",   "Interactive, non-interactive" },
		{ PARAM_IMAGE,    "image",      "Input image" },
		{ PARAM_DRAWABLE, "drawable",   "Input drawable" },
		{ PARAM_DRAWABLE, "bumpmap",    "Bump map drawable" },
		{ PARAM_FLOAT,    "azimuth",    "Azimuth" },
		{ PARAM_FLOAT,    "elevation",  "Elevation" },
		{ PARAM_INT32,    "depth",      "Depth" },
		{ PARAM_INT32,    "xofs",       "X offset" },
		{ PARAM_INT32,    "yofs",       "Y offset" },
		{ PARAM_INT32,    "waterlevel", "Level that full transparency should represent" },
		{ PARAM_INT32,    "ambient",    "Ambient lighting factor" },
		{ PARAM_INT32,    "compensate", "Compensate for darkening" },
		{ PARAM_INT32,    "invert",     "Invert bumpmap" },
		{ PARAM_INT32,    "type",       "Type of map (LINEAR (0), SPHERICAL (1), SINUOSIDAL (2))" }
	}; /* args */

	static GParamDef *return_vals  = NULL;
	static int        nargs        = sizeof(args) / sizeof(args[0]);
	static int        nreturn_vals = 0;

	INIT_I18N();

	gimp_install_procedure(PLUG_IN_NAME,
			       _("Create an embossing effect using an image as a bump map"),
			       ("This plug-in uses the algorithm described by John Schlag, "
			       "\"Fast Embossing Effects on Raster Image Data\" in Graphics GEMS IV "
			       "(ISBN 0-12-336155-9). It takes a grayscale image to be applied as "
			       "a bump map to another image and produces a nice embossing effect."),
			       "Federico Mena Quintero & Jens Lautenbacher",
			       "Federico Mena Quintero & Jens Lautenbacher",
			       PLUG_IN_VERSION,
			       _("<Image>/Filters/Map/Bump Map"),
			       "RGB*, GRAY*",
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

	GRunModeType run_mode;
	GStatusType  status;

	INIT_I18N_UI();

	status   = STATUS_SUCCESS;
	run_mode = param[0].data.d_int32;

	values[0].type          = PARAM_STATUS;
	values[0].data.d_status = status;

	*nreturn_vals = 1;
	*return_vals  = values;

	/* Get drawable information */

	drawable = gimp_drawable_get(param[2].data.d_drawable);

	gimp_drawable_mask_bounds(drawable->id, &sel_x1, &sel_y1, &sel_x2, &sel_y2);

	sel_width     = sel_x2 - sel_x1;
	sel_height    = sel_y2 - sel_y1;
	img_bpp       = gimp_drawable_bpp(drawable->id);
	img_has_alpha = gimp_drawable_has_alpha(drawable->id);

	/* See how we will run */

	switch (run_mode) {
		case RUN_INTERACTIVE:
			/* Possibly retrieve data */

			gimp_get_data(PLUG_IN_NAME, &bmvals);

			/* Get information from the dialog */

			if (!bumpmap_dialog())
				return;

			break;

		case RUN_NONINTERACTIVE:
			/* Make sure all the arguments are present */

			if (nparams != 14)
				status = STATUS_CALLING_ERROR;

			if (status == STATUS_SUCCESS) {
				bmvals.bumpmap_id = param[3].data.d_drawable;
				bmvals.azimuth    = param[4].data.d_float;
				bmvals.elevation  = param[5].data.d_float;
				bmvals.depth      = param[6].data.d_int32;
				bmvals.depth      = param[6].data.d_int32;
				bmvals.xofs       = param[7].data.d_int32;
				bmvals.yofs       = param[8].data.d_int32;
				bmvals.waterlevel = param[9].data.d_int32;
				bmvals.ambient    = param[10].data.d_int32;
				bmvals.compensate = param[11].data.d_int32;
				bmvals.invert     = param[12].data.d_int32;
				bmvals.type       = param[13].data.d_int32;
			} /* if */

			break;

		case RUN_WITH_LAST_VALS:
			/* Possibly retrieve data */

			gimp_get_data(PLUG_IN_NAME, &bmvals);
			break;

		default:
			break;
	} /* switch */

	/* Bumpmap the image */

	if (status == STATUS_SUCCESS) {
		if ((gimp_drawable_is_rgb(drawable->id) ||
		     gimp_drawable_is_gray(drawable->id))) {
			/* Set the tile cache size */
			
			gimp_tile_cache_ntiles(2*(drawable->width +
						  gimp_tile_width() - 1) /
					       gimp_tile_width());
			
			/* Run! */
			
			bumpmap();
			
			/* If run mode is interactive, flush displays */
			
			if (run_mode != RUN_NONINTERACTIVE)
				gimp_displays_flush();
			
			/* Store data */
			
			if (run_mode == RUN_INTERACTIVE)
				gimp_set_data(PLUG_IN_NAME, &bmvals, sizeof(bumpmap_vals_t));
		} /* if */
	} else
		status = STATUS_EXECUTION_ERROR;

	values[0].data.d_status = status;

	gimp_drawable_detach(drawable);
} /* run */


/*****/

static void
bumpmap(void)
{
	bumpmap_params_t  params;
	GDrawable        *bm_drawable;
	GPixelRgn         src_rgn, dest_rgn, bm_rgn;
	int               bm_width, bm_height, bm_bpp, bm_has_alpha;
	int               yofs1, yofs2, yofs3;
	guchar           *bm_row1, *bm_row2, *bm_row3, *bm_tmprow;
	guchar           *src_row, *dest_row;
	int               y;
	int               progress;
#if 0
	printf("bumpmap: waiting... (pid %d)\n", getpid());
	kill(getpid(), SIGSTOP);
#endif
	gimp_progress_init(_("Bump-mapping..."));
	
	/* Get the bumpmap drawable */

	if (bmvals.bumpmap_id != -1)
		bm_drawable = gimp_drawable_get(bmvals.bumpmap_id);
	else
		bm_drawable = drawable;

	/* Get image information */

	bm_width     = gimp_drawable_width(bm_drawable->id);
	bm_height    = gimp_drawable_height(bm_drawable->id);
	bm_bpp       = gimp_drawable_bpp(bm_drawable->id);
	bm_has_alpha = gimp_drawable_has_alpha(bm_drawable->id);

	/* Initialize offsets */

	if (bmvals.yofs < 0)
		yofs2 = bm_height - (-bmvals.yofs % bm_height);
	else
		yofs2 = bmvals.yofs % bm_height;

	yofs1 = (yofs2 + bm_height - 1) % bm_height;
	yofs3 = (yofs2 + 1) % bm_height;

	/* Initialize row buffers */

	bm_row1 = g_malloc(bm_width * bm_bpp * sizeof(guchar));
	bm_row2 = g_malloc(bm_width * bm_bpp * sizeof(guchar));
	bm_row3 = g_malloc(bm_width * bm_bpp * sizeof(guchar));

	src_row  = g_malloc(sel_width * img_bpp * sizeof(guchar));
	dest_row = g_malloc(sel_width * img_bpp * sizeof(guchar));

	/* Initialize pixel regions */

	gimp_pixel_rgn_init(&src_rgn, drawable, sel_x1, sel_y1, sel_width, sel_height, FALSE, FALSE);
	gimp_pixel_rgn_init(&dest_rgn, drawable, sel_x1, sel_y1, sel_width, sel_height, TRUE, TRUE);
	gimp_pixel_rgn_init(&bm_rgn, bm_drawable, 0, 0, bm_width, bm_height, FALSE, FALSE);

	/* Bumpmap */

	bumpmap_init_params(&params);

	gimp_pixel_rgn_get_row(&bm_rgn, bm_row1, 0, yofs1, bm_width);
	gimp_pixel_rgn_get_row(&bm_rgn, bm_row2, 0, yofs2, bm_width);
	gimp_pixel_rgn_get_row(&bm_rgn, bm_row3, 0, yofs3, bm_width);

	bumpmap_convert_row(bm_row1, bm_width, bm_bpp, bm_has_alpha, params.lut);
	bumpmap_convert_row(bm_row2, bm_width, bm_bpp, bm_has_alpha, params.lut);
	bumpmap_convert_row(bm_row3, bm_width, bm_bpp, bm_has_alpha, params.lut);

	progress = 0;

	for (y = sel_y1; y < sel_y2; y++) {
		gimp_pixel_rgn_get_row(&src_rgn, src_row, sel_x1, y, sel_width);

		bumpmap_row(src_row, dest_row, sel_width, img_bpp, img_has_alpha,
			    bm_row1, bm_row2, bm_row3, bm_width, bmvals.xofs, &params);

		gimp_pixel_rgn_set_row(&dest_rgn, dest_row, sel_x1, y, sel_width);

		/* Next line */

		bm_tmprow = bm_row1;
		bm_row1   = bm_row2;
		bm_row2   = bm_row3;
		bm_row3   = bm_tmprow;
		
		if (++yofs3 == bm_height)
			yofs3 = 0;

		gimp_pixel_rgn_get_row(&bm_rgn, bm_row3, 0, yofs3, bm_width);
		bumpmap_convert_row(bm_row3, bm_width, bm_bpp, bm_has_alpha, params.lut);

		gimp_progress_update((double) ++progress / sel_height);
	} /* for */

	/* Done */

	g_free(bm_row1);
	g_free(bm_row2);
	g_free(bm_row3);
	g_free(src_row);
	g_free(dest_row);

	if (bm_drawable != drawable)
		gimp_drawable_detach(bm_drawable);

	gimp_drawable_flush(drawable);
	gimp_drawable_merge_shadow(drawable->id, TRUE);
	gimp_drawable_update(drawable->id, sel_x1, sel_y1, sel_width, sel_height);
} /* bumpmap */


/*****/

static void
bumpmap_init_params(bumpmap_params_t *params)
{
	double azimuth;
	double elevation;
	int    lz, nz;
	int    i;
	double n;

	/* Convert to radians */

	azimuth   = G_PI * bmvals.azimuth / 180.0;
	elevation = G_PI * bmvals.elevation / 180.0;

	/* Calculate the light vector */

	params->lx = cos(azimuth) * cos(elevation) * 255.0;
	params->ly = sin(azimuth) * cos(elevation) * 255.0;
	lz         = sin(elevation) * 255.0;

	/* Calculate constant Z component of surface normal */

	nz           = (6 * 255) / bmvals.depth;
	params->nz2  = nz * nz;
	params->nzlz = nz * lz;

	/* Optimize for vertical normals */

	params->background = lz;

	/* Calculate darkness compensation factor */

	params->compensation = sin(elevation);

	/* Create look-up table for map type */

 	for (i = 0; i < 256; i++) {
		switch (bmvals.type) {
			case SPHERICAL:
				n = i / 255.0 - 1.0;
				params->lut[i] = (int) (255.0 * sqrt(1.0 - n * n) + 0.5);
				break;

			case SINUOSIDAL:
				n = i / 255.0;
				params->lut[i] = (int) (255.0 * (sin((-G_PI / 2.0) + G_PI * n) + 1.0) /
							2.0 + 0.5);
				break;

			case LINEAR:
			default:
				params->lut[i] = i;
		} /* switch */

		if (bmvals.invert)
			params->lut[i] = 255 - params->lut[i];
	} /* for */
} /* bumpmap_init_params */


/*****/

static void
bumpmap_row(guchar           *src_row,
	    guchar           *dest_row,
	    int               width,
	    int               bpp,
	    int               has_alpha,
	    guchar           *bm_row1,
	    guchar           *bm_row2,
	    guchar           *bm_row3,
	    int               bm_width,
	    int               bm_xofs,
	    bumpmap_params_t *params)
{
	guchar *src, *dest;
	int     xofs1, xofs2, xofs3;
	int     shade;
	int     ndotl;
	int     nx, ny;
	int     x, k;
	int     pbpp;
	int     result;

	if (has_alpha)
		pbpp = bpp - 1;
	else
		pbpp = bpp;

	if (bm_xofs < 0)
		xofs2 = bm_width - (-bm_xofs % bm_width);
	else
		xofs2 = bm_xofs % bm_width;

	xofs1 = (xofs2 + bm_width - 1) % bm_width;
	xofs3 = (xofs2 + 1) % bm_width;

	src  = src_row;
	dest = dest_row;

	for (x = 0; x < width; x++) {
		/* Calculate surface normal from bump map */

		nx = (bm_row1[xofs1] + bm_row2[xofs1] + bm_row3[xofs1] -
		      bm_row1[xofs3] - bm_row2[xofs3] - bm_row3[xofs3]);
		ny = (bm_row3[xofs1] + bm_row3[xofs2] + bm_row3[xofs3] -
		      bm_row1[xofs1] - bm_row1[xofs2] - bm_row1[xofs3]);

		/* Shade */

		if ((nx == 0) && (ny == 0))
			shade = params->background;
		else {
			ndotl = nx * params->lx + ny * params->ly + params->nzlz;

			if (ndotl < 0)
				shade = params->compensation * bmvals.ambient;
			else {
				shade = ndotl / sqrt(nx * nx + ny * ny + params->nz2);

				shade = shade + MAX(0, (255 * params->compensation - shade)) *
					        bmvals.ambient / 255;
			} /* else */
		} /* else */

		/* Paint */

		if (bmvals.compensate)
			for (k = pbpp; k; k--) {
				result  = (*src++ * shade) / (params->compensation * 255);
				*dest++ = MIN(255, result);
			} /* for */
		else
			for (k = pbpp; k; k--)
				*dest++ = *src++ * shade / 255;
		
		if (has_alpha)
			*dest++ = *src++;

		/* Next pixel */

		if (++xofs1 == bm_width)
			xofs1 = 0;

		if (++xofs2 == bm_width)
			xofs2 = 0;

		if (++xofs3 == bm_width)
			xofs3 = 0;
	} /* for */
} /* bumpmap_row */


/*****/

static void
bumpmap_convert_row(guchar *row, int width, int bpp, int has_alpha, guchar *lut)
{
	guchar *p;

	p = row;

	has_alpha = has_alpha ? 1 : 0;

	if (bpp >= 3)
		for (; width; width--) {
			if (has_alpha)
				*p++ = lut[(int) (bmvals.waterlevel +
						  (((int)
						    (0.30 * row[0] + 0.59 * row[1] + 0.11 * row[2] + 0.5) -
						    bmvals.waterlevel) *
						   row[3]) / 255.0)];
			else
				*p++ = lut[(int) (0.30 * row[0] + 0.59 * row[1] + 0.11 * row[2] + 0.5)];

			row += 3 + has_alpha;
		} /* for */
	else
		for (; width; width--) {
			if (has_alpha)
				*p++ = lut[bmvals.waterlevel +
					   ((row[0] - bmvals.waterlevel) * row[1]) / 255];
			else
				*p++ = lut[*row];

			row += 1 + has_alpha;
		} /* for */
} /* bumpmap_convert_row */


/*****/

static gint
bumpmap_dialog(void)
{
	GtkWidget  *dialog;
	GtkWidget  *top_table;
	GtkWidget  *frame;
	GtkWidget  *table;
	GtkWidget  *vbox;
	GtkWidget  *label;
	GtkWidget  *option_menu;
	GtkWidget  *menu;
	GtkWidget  *button;
	GSList     *group;
	gint        argc;
	gchar     **argv;
	guchar     *color_cube;
	int         i;
#if 0 
	printf("bumpmap: waiting... (pid %d)\n", getpid());
	kill(getpid(), SIGSTOP);
#endif  
	argc    = 1;
	argv    = g_new(gchar *, 1);
	argv[0] = g_strdup("bumpmap");

	gtk_init(&argc, &argv);
	gtk_rc_parse(gimp_gtkrc());
	gdk_set_use_xshm(gimp_use_xshm());
	
	gtk_preview_set_gamma(gimp_gamma());
	gtk_preview_set_install_cmap(gimp_install_cmap());
	color_cube = gimp_color_cube();
	gtk_preview_set_color_cube(color_cube[0], color_cube[1], color_cube[2], color_cube[3]);
	
	gtk_widget_set_default_visual(gtk_preview_get_visual());
	gtk_widget_set_default_colormap(gtk_preview_get_cmap());

	/* Dialog */

	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), _("Bump map"));
	gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
	gtk_container_border_width(GTK_CONTAINER(dialog), 0);
	gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
			   (GtkSignalFunc) dialog_close_callback,
			   NULL);

	top_table = gtk_table_new(2, 2, FALSE);
	gtk_container_border_width(GTK_CONTAINER(top_table), 6);
	gtk_table_set_row_spacings(GTK_TABLE(top_table), 4);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), top_table, FALSE, FALSE, 0);
	gtk_widget_show(top_table);

	/* Preview */

	table = gtk_table_new(2, 2, FALSE);
	gtk_container_border_width(GTK_CONTAINER(table), 0);
	gtk_table_attach(GTK_TABLE(top_table), table, 0, 1, 0, 1, 0, 0, 0, 0);
	gtk_widget_show(table);

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_table_attach(GTK_TABLE(table), frame, 0, 1, 0, 1, 0, 0, 0, 0);
	gtk_widget_show(frame);

	bmint.preview_width  = MIN(sel_width, PREVIEW_SIZE);
	bmint.preview_height = MIN(sel_height, PREVIEW_SIZE);

	bmint.preview = gtk_preview_new(GTK_PREVIEW_COLOR);
	gtk_preview_size(GTK_PREVIEW(bmint.preview), bmint.preview_width, bmint.preview_height);
	gtk_container_add(GTK_CONTAINER(frame), bmint.preview);
	gtk_widget_show(bmint.preview);
	
	gtk_widget_set_events(bmint.preview, 
			      GDK_BUTTON_PRESS_MASK |
			      GDK_BUTTON_RELEASE_MASK | 
			      GDK_BUTTON_MOTION_MASK |
			      GDK_POINTER_MOTION_HINT_MASK);
	gtk_signal_connect(GTK_OBJECT(bmint.preview), "event",
			   (GtkSignalFunc) dialog_preview_events,
			   NULL);
	
	dialog_init_preview();
	
	/* Table for upper-right controls */

	table = gtk_table_new(3, 1, FALSE);
	gtk_container_border_width(GTK_CONTAINER(table), 0);
	gtk_table_attach(GTK_TABLE(top_table), table, 1, 2, 0, 1,
			 GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_show(table);

	/* Compensate darkening */

	button = gtk_check_button_new_with_label(_("Compensate for darkening"));
	gtk_table_attach(GTK_TABLE(table), button, 0, 1, 0, 1,
			 GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), bmvals.compensate ? TRUE : FALSE);
	gtk_signal_connect(GTK_OBJECT(button), "toggled",
			   (GtkSignalFunc) dialog_compensate_callback,
			   NULL);
	gtk_widget_show(button);

	/* Invert bumpmap */

	button = gtk_check_button_new_with_label(_("Invert bumpmap"));
	gtk_table_attach(GTK_TABLE(table), button, 0, 1, 1, 2,
			 GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), bmvals.invert ? TRUE : FALSE);
	gtk_signal_connect(GTK_OBJECT(button), "toggled",
			   (GtkSignalFunc) dialog_invert_callback,
			   NULL);
	gtk_widget_show(button);

	/* Type of map */

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
	gtk_table_attach(GTK_TABLE(table), frame, 0, 1, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
	gtk_widget_show(frame);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(vbox), 4);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_widget_show(vbox);

	group = NULL;

	for (i = 0; i < (sizeof(map_types) / sizeof(map_types[0])); i++) {
		button = gtk_radio_button_new_with_label(group, gettext(map_types[i]));
		group  = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
		if (i == bmvals.type)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
		gtk_signal_connect(GTK_OBJECT(button), "toggled",
				   (GtkSignalFunc) dialog_map_type_callback,
				   (gpointer) ((long) i));
		gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
		gtk_widget_show(button);
	} /* for */

	/* Table for bottom controls */

	table = gtk_table_new(8, 3, FALSE);
	gtk_container_border_width(GTK_CONTAINER(table), 0);
	gtk_table_attach(GTK_TABLE(top_table), table, 0, 2, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	gtk_widget_show(table);

	/* Bump map menu */

	label = gtk_label_new(_("Bump map"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 4, 0);
	gtk_widget_show(label);

	option_menu = gtk_option_menu_new();
	gtk_table_attach(GTK_TABLE(table), option_menu, 1, 2, 0, 1,
			 GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_show(option_menu);

	menu = gimp_drawable_menu_new(dialog_constrain,
				      dialog_bumpmap_callback,
				      NULL,
				      bmvals.bumpmap_id);
	gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
	gtk_widget_show(option_menu);

	/* Controls */

	dialog_create_dvalue(_("Azimuth"), GTK_TABLE(table), 1, &bmvals.azimuth, 0.0, 360.0);
	dialog_create_dvalue(_("Elevation"), GTK_TABLE(table), 2, &bmvals.elevation, 0.5, 90.0);
	dialog_create_ivalue(_("Depth"), GTK_TABLE(table), 3, &bmvals.depth, 1, 65, FALSE);
	dialog_create_ivalue(_("X offset"), GTK_TABLE(table), 4, &bmvals.xofs, -1000, 1001, FALSE);
	dialog_create_ivalue(_("Y offset"), GTK_TABLE(table), 5, &bmvals.yofs, -1000, 1001, FALSE);
	dialog_create_ivalue(_("Waterlevel"), GTK_TABLE(table), 6, &bmvals.waterlevel, 0, 256, TRUE);
	dialog_create_ivalue(_("Ambient"), GTK_TABLE(table), 7, &bmvals.ambient, 0, 256, FALSE);

	/* Buttons */

	gtk_container_border_width(GTK_CONTAINER(GTK_DIALOG(dialog)->action_area), 6);
	
	button = gtk_button_new_with_label(_("OK"));
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   (GtkSignalFunc) dialog_ok_callback,
			   dialog);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
	gtk_widget_grab_default(button);
	gtk_widget_show(button);
	
	button = gtk_button_new_with_label(_("Cancel"));
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   (GtkSignalFunc) dialog_cancel_callback,
			   dialog);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
	gtk_widget_show(button);

	/* Done */
	
	gtk_widget_show(dialog);

	dialog_new_bumpmap();
	dialog_update_preview();
	
	gtk_main();
	gdk_flush();

	g_free(bmint.check_row_0);
	g_free(bmint.check_row_1);

	for (i = 0; i < bmint.preview_height; i++)
		g_free(bmint.src_rows[i]);

	g_free(bmint.src_rows);

	for (i = 0; i < (bmint.preview_height + 2); i++)
		g_free(bmint.bm_rows[i]);

	g_free(bmint.bm_rows);
	
	if (bmint.bm_drawable != drawable)
		gimp_drawable_detach(bmint.bm_drawable);

	return bmint.run;
} /* bumpmap_dialog */


/*****/

static void
dialog_init_preview(void)
{
	int x;
	
	/* Create checkerboard rows */

	bmint.check_row_0 = g_malloc(bmint.preview_width * sizeof(guchar));
	bmint.check_row_1 = g_malloc(bmint.preview_width * sizeof(guchar));

	for (x = 0; x < bmint.preview_width; x++)
		if ((x / CHECK_SIZE) & 1) {
			bmint.check_row_0[x] = CHECK_DARK;
			bmint.check_row_1[x] = CHECK_LIGHT;
		} else {
			bmint.check_row_0[x] = CHECK_LIGHT;
			bmint.check_row_1[x] = CHECK_DARK;
		} /* else */

	/* Initialize source rows */

	gimp_pixel_rgn_init(&bmint.src_rgn, drawable, sel_x1, sel_y1, sel_width, sel_height, FALSE, FALSE);

	bmint.src_rows = g_malloc(bmint.preview_height * sizeof(guchar *));

	for (x = 0; x < bmint.preview_height; x++)
		bmint.src_rows[x]  = g_malloc(sel_width * 4 * sizeof(guchar));

	dialog_fill_src_rows(0,
			     bmint.preview_height,
			     sel_y1 + bmint.preview_yofs);

	/* Initialize bumpmap rows */

	bmint.bm_rows = g_malloc((bmint.preview_height + 2) * sizeof(guchar *)); /* Plus 2 for conv. matrix */

	for (x = 0; x < (bmint.preview_height + 2); x++)
		bmint.bm_rows[x] = NULL;
} /* dialog_init_preview */

/*****/

static gint
dialog_preview_events(GtkWidget *widget, GdkEvent *event)
{
	gint            x, y;
	gint            dx, dy;
	GdkEventButton *bevent;
	
	gtk_widget_get_pointer(widget, &x, &y);

	bevent = (GdkEventButton *) event;
	
	switch (event->type) {
		case GDK_BUTTON_PRESS:
			switch (bevent->button) {
				case 1:
					if (bevent->state & GDK_SHIFT_MASK)
						bmint.drag_mode = DRAG_BUMPMAP;
					else
						bmint.drag_mode = DRAG_SCROLL;
					break;
					
				case 3:
					bmint.drag_mode = DRAG_BUMPMAP;
					break;
					
				default:
					return FALSE;
			} /* switch */
			
			bmint.mouse_x = x;
			bmint.mouse_y = y;
			
			gtk_grab_add(widget);
			
			break;
			
		case GDK_BUTTON_RELEASE:
			if (bmint.drag_mode != DRAG_NONE) {
				gtk_grab_remove(widget);
				bmint.drag_mode = DRAG_NONE;
				dialog_update_preview();
			} /* if */
			
			break;
			
		case GDK_MOTION_NOTIFY:
			dx = x - bmint.mouse_x;
			dy = y - bmint.mouse_y;

			bmint.mouse_x = x;
			bmint.mouse_y = y;
			
			if ((dx == 0) && (dy == 0))
				break;

			switch (bmint.drag_mode) {
				case DRAG_SCROLL:
					bmint.preview_xofs = CLAMP(bmint.preview_xofs - dx,
								   0,
								   sel_width - bmint.preview_width);
					bmint.preview_yofs = CLAMP(bmint.preview_yofs - dy,
								   0,
								   sel_height - bmint.preview_height);
					break;

				case DRAG_BUMPMAP:
					bmvals.xofs = CLAMP(bmvals.xofs - dx, -1000, 1000);
					bmvals.yofs = CLAMP(bmvals.yofs - dy, -1000, 1000);
					break;

				default:
					return FALSE;
			} /* switch */

			dialog_update_preview();
			break; 
			
		default:
			break;
	} /* switch */
	
	return FALSE;
} /* dialog_preview_events */


/*****/

static void
dialog_new_bumpmap(void)
{
	int i;
	int yofs;

	/* Get drawable */

	if (bmint.bm_drawable && (bmint.bm_drawable != drawable))
		gimp_drawable_detach(bmint.bm_drawable);
	
	if (bmvals.bumpmap_id != -1)
		bmint.bm_drawable = gimp_drawable_get(bmvals.bumpmap_id);
	else
		bmint.bm_drawable = drawable;
	
	/* Get sizes */

	bmint.bm_width     = gimp_drawable_width(bmint.bm_drawable->id);
	bmint.bm_height    = gimp_drawable_height(bmint.bm_drawable->id);
	bmint.bm_bpp       = gimp_drawable_bpp(bmint.bm_drawable->id);
	bmint.bm_has_alpha = gimp_drawable_has_alpha(bmint.bm_drawable->id);
	
	/* Initialize pixel region */

	gimp_pixel_rgn_init(&bmint.bm_rgn, bmint.bm_drawable, 0, 0, bmint.bm_width, bmint.bm_height,
			    FALSE, FALSE);

	/* Initialize row buffers */

	yofs = bmvals.yofs + bmint.preview_yofs - 1; /* Minus 1 for conv. matrix */

	if (yofs < 0)
		yofs = bmint.bm_height - (-yofs % bmint.bm_height);
	else
		yofs %= bmint.bm_height;

	bmint.bm_yofs = yofs;

	for (i = 0; i < (bmint.preview_height + 2); i++) {
		if (bmint.bm_rows[i])
			g_free(bmint.bm_rows[i]);

		bmint.bm_rows[i] = g_malloc(bmint.bm_width * bmint.bm_bpp * sizeof(guchar));
	} /* for */

	bumpmap_init_params(&bmint.params);
	dialog_fill_bumpmap_rows(0, bmint.preview_height + 2, yofs);
} /* dialog_new_bumpmap */


/*****/

static void
dialog_update_preview()
{
	static guchar dest_row[PREVIEW_SIZE * 4];
	static guchar preview_row[PREVIEW_SIZE * 3];
	
	guchar          *check_row;
	guchar           check;
	int              xofs;
	int              x, y;
	guchar          *sp, *p;

	bumpmap_init_params(&bmint.params);

	/* Scroll the row buffers */

	dialog_scroll_src();
	dialog_scroll_bumpmap();

	/* Bumpmap */

	xofs = bmint.preview_xofs;

	for (y = 0; y < bmint.preview_height; y++) {
		bumpmap_row(bmint.src_rows[y] + 4 * xofs, dest_row,
			    bmint.preview_width, 4, TRUE,
			    bmint.bm_rows[y], bmint.bm_rows[y + 1], bmint.bm_rows[y + 2],
			    bmint.bm_width, xofs + bmvals.xofs, &bmint.params);
		
		/* Paint row */
		
		sp = dest_row;
		p  = preview_row;
		
		if ((y / CHECK_SIZE) & 1)
			check_row = bmint.check_row_0;
		else
			check_row = bmint.check_row_1;
		
		for (x = 0; x < bmint.preview_width; x++) {
			check = check_row[x];
			
			p[0] = check + ((sp[0] - check) * sp[3]) / 255;
			p[1] = check + ((sp[1] - check) * sp[3]) / 255;
			p[2] = check + ((sp[2] - check) * sp[3]) / 255;
			
			sp += 4;
			p  += 3;
		} /* for */

		gtk_preview_draw_row(GTK_PREVIEW(bmint.preview), preview_row, 0, y, bmint.preview_width);
	} /* for */

	gtk_widget_draw(bmint.preview, NULL);
	gdk_flush();
} /* dialog_update_preview */


/*****/

#define SWAP_ROWS(a, b, t) { t = a; a = b; b = t; }

static void
dialog_scroll_src(void)
{
	int     yofs;
	int     y, ofs;
	guchar *tmp;

	yofs = bmint.preview_yofs;

	if (yofs == bmint.src_yofs)
		return;

	if (yofs < bmint.src_yofs) {
		ofs = bmint.src_yofs - yofs;

		/* Scroll useful rows... */

		if (ofs < bmint.preview_height)
			for (y = (bmint.preview_height - 1); y >= ofs; y--)
				SWAP_ROWS(bmint.src_rows[y], bmint.src_rows[y - ofs], tmp);

		/* ... and get the new ones */

		dialog_fill_src_rows(0, MIN(ofs, bmint.preview_height), sel_y1 + yofs);
	} else {
		ofs = yofs - bmint.src_yofs;

		/* Scroll useful rows... */

		if (ofs < bmint.preview_height)
			for (y = 0; y < (bmint.preview_height - ofs); y++)
				SWAP_ROWS(bmint.src_rows[y], bmint.src_rows[y + ofs], tmp);

		/* ... and get the new ones */

		dialog_fill_src_rows(bmint.preview_height - MIN(ofs, bmint.preview_height),
				     MIN(ofs, bmint.preview_height),
				     sel_y1 + yofs + bmint.preview_height - MIN(ofs, bmint.preview_height));
	} /* else */

	bmint.src_yofs = yofs;
} /* dialog_scroll_src */


/*****/

static void
dialog_scroll_bumpmap(void)
{
	int     yofs;
	int     y, ofs;
	guchar *tmp;

	yofs = bmvals.yofs + bmint.preview_yofs - 1; /* Minus 1 for conv. matrix */

	if (yofs < 0)
		yofs = bmint.bm_height - (-yofs % bmint.bm_height);
	else
		yofs %= bmint.bm_height;

	if (yofs == bmint.bm_yofs)
		return;

	if (yofs < bmint.bm_yofs) {
		ofs = bmint.bm_yofs - yofs;

		/* Scroll useful rows... */

		if (ofs < (bmint.preview_height + 2))
			for (y = (bmint.preview_height + 1); y >= ofs; y--)
				SWAP_ROWS(bmint.bm_rows[y], bmint.bm_rows[y - ofs], tmp);

		/* ... and get the new ones */

		dialog_fill_bumpmap_rows(0,
					 MIN(ofs, bmint.preview_height + 2),
					 yofs);
	} else {
		ofs = yofs - bmint.bm_yofs;

		/* Scroll useful rows... */

		if (ofs < (bmint.preview_height + 2))
			for (y = 0; y < (bmint.preview_height + 2 - ofs); y++)
				SWAP_ROWS(bmint.bm_rows[y], bmint.bm_rows[y + ofs], tmp);

		/* ... and get the new ones */

		dialog_fill_bumpmap_rows(bmint.preview_height + 2 - MIN(ofs, bmint.preview_height + 2),
					 MIN(ofs, bmint.preview_height + 2),
					 (yofs + bmint.preview_height + 2 -
					  MIN(ofs, bmint.preview_height + 2)) % bmint.bm_height);
	} /* else */

	bmint.bm_yofs = yofs;
} /* dialog_scroll_bumpmap */


/*****/

static void
dialog_get_rows(GPixelRgn *pr, guchar **rows, int x, int y, int width, int height)
{
	/* This is shamelessly ripped off from gimp_pixel_rgn_get_rect().
	 * Its function is exactly the same, but it can fetch an image
	 * rectangle to a sparse buffer which is defined as separate
	 * rows instead of one big linear region.
	 */
	
	GTile  *tile;
	guchar *src, *dest;
	int     xstart, ystart;
	int     xend, yend;
	int     xboundary;
	int     yboundary;
	int     xstep, ystep;
	int     b, bpp;
	int     tx, ty;
	int     tile_width, tile_height;

	tile_width  = gimp_tile_width();
	tile_height = gimp_tile_height();

	bpp       = pr->bpp;

	xstart = x;
	ystart = y;
	xend   = x + width;
	yend   = y + height;
	ystep  = 0; /* Shut up -Wall */

	while (y < yend) {
		x = xstart;

		while (x < xend) {
			tile = gimp_drawable_get_tile2(pr->drawable, pr->shadow, x, y);
			gimp_tile_ref(tile);

			xstep     = tile->ewidth - (x % tile_width);
			ystep     = tile->eheight - (y % tile_height);
			xboundary = x + xstep;
			yboundary = y + ystep;
			xboundary = MIN(xboundary, xend);
			yboundary = MIN(yboundary, yend);

			for (ty = y; ty < yboundary; ty++) {
				src  = tile->data + tile->bpp * (tile->ewidth * (ty % tile_height) +
								 (x % tile_width));
				dest = rows[ty - ystart] + bpp * (x - xstart);

				for (tx = x; tx < xboundary; tx++)
					for (b = bpp; b; b--)
						*dest++ = *src++;
			} /* for */

			gimp_tile_unref(tile, FALSE);

			x += xstep;
		} /* while */

		y += ystep;
	} /* while */
} /* dialog_get_rows */


/*****/

static void
dialog_fill_src_rows(int start, int how_many, int yofs)
{
	int x, y;
	guchar *sp, *p;
	
	dialog_get_rows(&bmint.src_rgn,
			bmint.src_rows + start,
			sel_x1,
			yofs,
			sel_width,
			how_many);

	/* Convert to RGBA.  We move backwards! */

	for (y = start; y < (start + how_many); y++) {
		sp = bmint.src_rows[y] + img_bpp * sel_width - 1;
		p  = bmint.src_rows[y] + 4 * sel_width - 1;
		
		for (x = 0; x < sel_width; x++) {
			if (img_has_alpha)
				*p-- = *sp--;
			else
				*p-- = 255;
			
			if (img_bpp < 3) {
				*p-- = *sp;
				*p-- = *sp;
				*p-- = *sp--;
			} else {
				*p-- = *sp--;
				*p-- = *sp--;
				*p-- = *sp--;
			} /* else */
		} /* for */
	} /* for */
} /* dialog_fill_src_rows */


/*****/

static void
dialog_fill_bumpmap_rows(int start, int how_many, int yofs)
{
	int buf_row_ofs;
	int remaining;
	int this_pass;

	buf_row_ofs = start;
	remaining   = how_many;

	while (remaining > 0) {
		this_pass = MIN(remaining, bmint.bm_height - yofs);

		dialog_get_rows(&bmint.bm_rgn,
				bmint.bm_rows + buf_row_ofs,
				0,
				yofs,
				bmint.bm_width,
				this_pass);

		yofs         = (yofs + this_pass) % bmint.bm_height;
		remaining   -= this_pass;
		buf_row_ofs += this_pass;
	} /* while */

	/* Convert rows */
	
	for (; how_many; how_many--) {
		bumpmap_convert_row(bmint.bm_rows[start], bmint.bm_width, bmint.bm_bpp, bmint.bm_has_alpha,
				    bmint.params.lut);

		start++;
	} /* for */
} /* dialog_fill_bumpmap_rows */


/*****/

static void
dialog_compensate_callback(GtkWidget *widget, gpointer data)
{
	bmvals.compensate = GTK_TOGGLE_BUTTON(widget)->active;

	dialog_update_preview();
} /* dialog_compensate_callback */


/*****/

static void
dialog_invert_callback(GtkWidget *widget, gpointer data)
{
	bmvals.invert = GTK_TOGGLE_BUTTON(widget)->active;

	bumpmap_init_params(&bmint.params);
	dialog_fill_bumpmap_rows(0, bmint.preview_height + 2, bmint.bm_yofs);
	dialog_update_preview();
} /* dialog_invert_callback */


/*****/

static void
dialog_map_type_callback(GtkWidget *widget, gpointer data)
{
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		bmvals.type = (long) data;

		bumpmap_init_params(&bmint.params);
		dialog_fill_bumpmap_rows(0, bmint.preview_height + 2, bmint.bm_yofs);
		dialog_update_preview();
	} /* if */
} /* dialog_map_type_callback */


/*****/

static gint
dialog_constrain(gint32 image_id, gint32 drawable_id, gpointer data)
{
	if (drawable_id == -1)
		return TRUE;

	return (gimp_drawable_is_rgb(drawable_id) || gimp_drawable_is_gray(drawable_id));
} /* dialog_constrain */


/*****/

static void
dialog_bumpmap_callback(gint32 id, gpointer data)
{
	bmvals.bumpmap_id = id;
	dialog_new_bumpmap();
	dialog_update_preview();
} /* dialog_bumpmap_callback */


/*****/

static void
dialog_create_dvalue(char *title, GtkTable *table, int row, gdouble *value, double left, double right)
{
	GtkWidget *label;
	GtkWidget *scale;
	GtkWidget *entry;
	GtkObject *scale_data;
	char       buf[256];

	label = gtk_label_new(title);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
	gtk_table_attach(table, label, 0, 1, row, row + 1, GTK_FILL, GTK_FILL, 4, 0);
	gtk_widget_show(label);

	scale_data = gtk_adjustment_new(*value, left, right, 1.0, 1.0, 0.0);

	gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
			   (GtkSignalFunc) dialog_dscale_update,
			   value);

	scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
	gtk_widget_set_usize(scale, SCALE_WIDTH, 0);
	gtk_table_attach(table, scale, 1, 2, row, row + 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
	gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
	gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
	gtk_widget_show(scale);

	entry = gtk_entry_new();
	gtk_object_set_user_data(GTK_OBJECT(entry), scale_data);
	gtk_object_set_user_data(scale_data, entry);
	gtk_widget_set_usize(entry, ENTRY_WIDTH, 0);
	sprintf(buf, "%0.2f", *value);
	gtk_entry_set_text(GTK_ENTRY(entry), buf);
	gtk_signal_connect(GTK_OBJECT(entry), "changed",
			   (GtkSignalFunc) dialog_dentry_update,
			   value);
	gtk_table_attach(GTK_TABLE(table), entry, 2, 3, row, row + 1, GTK_FILL, GTK_FILL, 4, 0);
	gtk_widget_show(entry);
} /* dialog_create_dvalue */


/*****/

static void
dialog_dscale_update(GtkAdjustment *adjustment, gdouble *value)
{
	GtkWidget *entry;
	char       buf[256];
	
	if (*value != adjustment->value) {
		*value = adjustment->value;
		
		entry = gtk_object_get_user_data(GTK_OBJECT(adjustment));
		sprintf(buf, "%0.2f", *value);
		
		gtk_signal_handler_block_by_data(GTK_OBJECT(entry), value);
		gtk_entry_set_text(GTK_ENTRY(entry), buf);
		gtk_signal_handler_unblock_by_data(GTK_OBJECT(entry), value);
		
		dialog_update_preview();
	} /* if */
} /* dialog_dscale_update */


/*****/

static void
dialog_dentry_update(GtkWidget *widget, gdouble *value)
{
	GtkAdjustment *adjustment;
	gdouble        new_value;
	
	new_value = atof(gtk_entry_get_text(GTK_ENTRY(widget)));
	
	if (*value != new_value) {
		adjustment = gtk_object_get_user_data(GTK_OBJECT(widget));
		
		if ((new_value >= adjustment->lower) &&
		    (new_value <= adjustment->upper)) {
			*value            = new_value;
			adjustment->value = new_value;
			
			gtk_signal_emit_by_name(GTK_OBJECT(adjustment), "value_changed");
			
			dialog_update_preview();
		} /* if */
	} /* if */
} /* dialog_dentry_update */


/*****/

static void
dialog_create_ivalue(char *title, GtkTable *table, int row, gint *value,
		     int left, int right, int full_update)
{
	GtkWidget *label;
	GtkWidget *scale;
	GtkWidget *entry;
	GtkObject *scale_data;
	char       buf[256];

	label = gtk_label_new(title);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
	gtk_table_attach(table, label, 0, 1, row, row + 1, GTK_FILL, GTK_FILL, 4, 0);
	gtk_widget_show(label);

	scale_data = gtk_adjustment_new(*value, left, right, 1.0, 1.0, 1.0);

	if (full_update)
		gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
				   (GtkSignalFunc) dialog_iscale_update_full,
				   value);
	else
		gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
				   (GtkSignalFunc) dialog_iscale_update_normal,
				   value);
	
	scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
	gtk_widget_set_usize(scale, SCALE_WIDTH, 0);
	gtk_table_attach(table, scale, 1, 2, row, row + 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
	gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
	gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
	gtk_widget_show(scale);

	entry = gtk_entry_new();
	gtk_object_set_user_data(GTK_OBJECT(entry), scale_data);
	gtk_object_set_user_data(scale_data, entry);
	gtk_widget_set_usize(entry, ENTRY_WIDTH, 0);
	sprintf(buf, "%d", *value);
	gtk_entry_set_text(GTK_ENTRY(entry), buf);

	if (full_update)
		gtk_signal_connect(GTK_OBJECT(entry), "changed",
				   (GtkSignalFunc) dialog_ientry_update_full,
				   value);
	else
		gtk_signal_connect(GTK_OBJECT(entry), "changed",
				   (GtkSignalFunc) dialog_ientry_update_normal,
				   value);
	
	gtk_table_attach(GTK_TABLE(table), entry, 2, 3, row, row + 1, GTK_FILL, GTK_FILL, 4, 0);
	gtk_widget_show(entry);
} /* dialog_create_ivalue */


/*****/

static void
dialog_iscale_update_normal(GtkAdjustment *adjustment, gint *value)
{
	GtkWidget *entry;
	char       buf[256];
	
	if (*value != adjustment->value) {
		*value = adjustment->value;
		
		entry = gtk_object_get_user_data(GTK_OBJECT(adjustment));
		sprintf(buf, "%d", *value);
		
		gtk_signal_handler_block_by_data(GTK_OBJECT(entry), value);
		gtk_entry_set_text(GTK_ENTRY(entry), buf);
		gtk_signal_handler_unblock_by_data(GTK_OBJECT(entry), value);
		
		dialog_update_preview();
	} /* if */
} /* dialog_iscale_update_normal */


/*****/

static void
dialog_iscale_update_full(GtkAdjustment *adjustment, gint *value)
{
	GtkWidget *entry;
	char       buf[256];
	
	if (*value != adjustment->value) {
		*value = adjustment->value;
		
		entry = gtk_object_get_user_data(GTK_OBJECT(adjustment));
		sprintf(buf, "%d", *value);
		
		gtk_signal_handler_block_by_data(GTK_OBJECT(entry), value);
		gtk_entry_set_text(GTK_ENTRY(entry), buf);
		gtk_signal_handler_unblock_by_data(GTK_OBJECT(entry), value);
		
		bumpmap_init_params(&bmint.params);
		dialog_fill_bumpmap_rows(0, bmint.preview_height + 2, bmint.bm_yofs);
		dialog_update_preview();
	} /* if */
} /* dialog_iscale_update_full */


/*****/

static void
dialog_ientry_update_normal(GtkWidget *widget, gint *value)
{
	GtkAdjustment *adjustment;
	gdouble        new_value;
	
	new_value = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));
	
	if (*value != new_value) {
		adjustment = gtk_object_get_user_data(GTK_OBJECT(widget));
		
		if ((new_value >= adjustment->lower) &&
		    (new_value <= adjustment->upper)) {
			*value            = new_value;
			adjustment->value = new_value;
			
			gtk_signal_emit_by_name(GTK_OBJECT(adjustment), "value_changed");
			
			dialog_update_preview();
		} /* if */
	} /* if */
} /* dialog_ientry_update_normal */


/*****/

static void
dialog_ientry_update_full(GtkWidget *widget, gint *value)
{
	GtkAdjustment *adjustment;
	gdouble        new_value;
	
	new_value = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));
	
	if (*value != new_value) {
		adjustment = gtk_object_get_user_data(GTK_OBJECT(widget));
		
		if ((new_value >= adjustment->lower) &&
		    (new_value <= adjustment->upper)) {
			*value            = new_value;
			adjustment->value = new_value;
			
			gtk_signal_emit_by_name(GTK_OBJECT(adjustment), "value_changed");
			
			bumpmap_init_params(&bmint.params);
			dialog_fill_bumpmap_rows(0, bmint.preview_height + 2, bmint.bm_yofs);
			dialog_update_preview();
		} /* if */
	} /* if */
} /* dialog_ientry_update_full */


/*****/

static void
dialog_ok_callback(GtkWidget *widget, gpointer data)
{
	bmint.run = TRUE;
	gtk_widget_destroy(GTK_WIDGET(data));
} /* dialog_ok_callback */


/*****/

static void
dialog_cancel_callback(GtkWidget *widget, gpointer data)
{
	gtk_widget_destroy(GTK_WIDGET(data));
} /* dialog_cancel_callback */


/*****/

static void
dialog_close_callback(GtkWidget *widget, gpointer data)
{
	gtk_main_quit();
} /* dialog_close_callback */

