/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Polarize plug-in --- maps a rectangul to a circle or vice-versa
 * Copyright (C) 1997 Daniel Dunbar
 * Email: ddunbar@diads.com
 * WWW:   http://millennium.diads.com/gimp/
 * Copyright (C) 1997 Federico Mena Quintero
 * federico@nuclecu.unam.mx
 * Copyright (C) 1996 Marc Bless
 * E-mail: bless@ai-lab.fh-furtwangen.de
 * WWW:    www.ai-lab.fh-furtwangen.de/~bless
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


/* Version 1.0:
 * This is the follow-up release.  It contains a few minor changes, the
 * most major being that the first time I released the wrong version of
 * the code, and this time the changes have been fixed.  I also added
 * tooltips to the dialog.
 *
 * Feel free to email me if you have any comments or suggestions on this
 * plugin.
 *               --Daniel Dunbar
 *                 ddunbar@diads.com
 */


/* Version .5:
 * This is the first version publicly released, it will probably be the
 * last also unless i can think of some features i want to add.
 *
 * This plug-in was created with loads of help from quartic (Frederico
 * Mena Quintero), and would surely not have come about without it.
 *
 * The polar algorithms is copied from Marc Bless' polar plug-in for
 * .54, many thanks to him also.
 * 
 * If you can think of a neat addition to this plug-in, or any other
 * info about it, please email me at ddunbar@diads.com.
 *                                     - Daniel Dunbar
 */

#include <math.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "gtk/gtk.h"
#include "libgimp/gimp.h"

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif /* M_PI */

#define sqr(x)	((x) * (x))
#define WITHIN(a, b, c) ((((a) <= (b)) && ((b) <= (c))) ? 1 : 0)


/***** Magic numbers *****/

#define PLUG_IN_NAME    "plug_in_polar_coords"
#define PLUG_IN_VERSION "July 1997, 0.5"

#define PREVIEW_SIZE 128
#define SCALE_WIDTH  200
#define ENTRY_WIDTH  60

#define CHECK_SIZE  8
#define CHECK_DARK  ((int) (1.0 / 3.0 * 255))
#define CHECK_LIGHT ((int) (2.0 / 3.0 * 255))


/***** Types *****/

typedef struct {
	gdouble circle;
	gdouble angle;
	gint backwards;
	gint inverse;
	gint polrec;
} polarize_vals_t;

typedef struct {
	GtkWidget *preview;
	guchar    *check_row_0;
	guchar    *check_row_1;
	guchar    *image;
	guchar    *dimage;

	gint run;
} polarize_interface_t;

typedef struct {
	gint       col, row;
	gint       img_width, img_height, img_bpp, img_has_alpha;
	gint       tile_width, tile_height;
	guchar     bg_color[4];
	GDrawable *drawable;
	GTile     *tile;
} pixel_fetcher_t;


/***** Prototypes *****/

static void query(void);
static void run(char    *name,
		int      nparams,
		GParam  *param,
		int     *nreturn_vals,
		GParam **return_vals);

static void   polarize(void);
static int    calc_undistorted_coords(double wx, double wy,
				      double *x, double *y);
static guchar bilinear(double x, double y, guchar *values);

static pixel_fetcher_t *pixel_fetcher_new(GDrawable *drawable);
static void             pixel_fetcher_set_bg_color(pixel_fetcher_t *pf, guchar r, guchar g, guchar b, guchar a);
static void             pixel_fetcher_get_pixel(pixel_fetcher_t *pf, int x, int y, guchar *pixel);
static void             pixel_fetcher_destroy(pixel_fetcher_t *pf);

static void build_preview_source_image(void);

static gint polarize_dialog(void);
static void dialog_update_preview(void);
static void dialog_create_value(char *title, GtkTable *table, int row, gdouble *value,
				double left, double right, double step);
static void dialog_scale_update(GtkAdjustment *adjustment, gdouble *value);
static void dialog_entry_update(GtkWidget *widget, gdouble *value);
static void dialog_close_callback(GtkWidget *widget, gpointer data);
static void dialog_ok_callback(GtkWidget *widget, gpointer data);
static void dialog_cancel_callback(GtkWidget *widget, gpointer data);

static void polar_toggle_callback(GtkWidget *widget, gpointer data);
static void set_tooltip (GtkTooltips *tooltips, GtkWidget *widget, const char *desc);

/***** Variables *****/

GPlugInInfo PLUG_IN_INFO = {
	NULL,   /* init_proc */
	NULL,   /* quit_proc */
	query,  /* query_proc */
	run     /* run_proc */
}; /* PLUG_IN_INFO */

static polarize_vals_t pcvals = {
	100.0, /* circle */
	0.0,  /* angle */
	0, /* backwards */
	1,  /* inverse */
	1  /* polar to rectangular? */
}; /* pcvals */

static polarize_interface_t pcint = {
	NULL,  /* preview */
	NULL,  /* check_row_0 */
	NULL,  /* check_row_1 */
	NULL,  /* image */
	NULL,  /* dimage */
	FALSE  /* run */
}; /* pcint */

static GDrawable *drawable;

static gint img_width, img_height, img_bpp, img_has_alpha;
static gint sel_x1, sel_y1, sel_x2, sel_y2;
static gint sel_width, sel_height;
static gint preview_width, preview_height;

static double cen_x, cen_y;
static double scale_x, scale_y;

/***** Functions *****/

/*****/

MAIN()


/*****/

static void
query(void)
{
	static GParamDef args[] = {
		{ PARAM_INT32,    "run_mode",  "Interactive, non-interactive" },
		{ PARAM_IMAGE,    "image",     "Input image" },
		{ PARAM_DRAWABLE, "drawable",  "Input drawable" },
		{ PARAM_FLOAT,    "circle",    "Circle depth in %" },
		{ PARAM_FLOAT,    "angle",     "Offset angle" },
		{ PARAM_INT32,    "backwards",    "Map backwards?" },
		{ PARAM_INT32,    "inverse",     "Map from top?" },
		{ PARAM_INT32,    "polrec",     "Polar to rectangular?" },
	}; /* args */

	static GParamDef *return_vals  = NULL;
	static int        nargs        = sizeof(args) / sizeof(args[0]);
	static int        nreturn_vals = 0;

	gimp_install_procedure(PLUG_IN_NAME,
			       "Converts and image to and from polar coords",
			       "Remaps and image from rectangular coordinates to polar coordinats or vice versa",
			       "Daniel Dunbar and Federico Mena Quintero",
			       "Daniel Dunbar and Federico Mena Quintero",
			       PLUG_IN_VERSION,
			       "<Image>/Filters/Distorts/Polar Coords",
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
	double       xhsiz, yhsiz;
	int          pwidth, pheight;

	status   = STATUS_SUCCESS;
	run_mode = param[0].data.d_int32;

	values[0].type          = PARAM_STATUS;
	values[0].data.d_status = status;

	*nreturn_vals = 1;
	*return_vals  = values;

	/* Get the active drawable info */

	drawable = gimp_drawable_get(param[2].data.d_drawable);

	img_width     = gimp_drawable_width(drawable->id);
	img_height    = gimp_drawable_height(drawable->id);
	img_bpp       = gimp_drawable_bpp(drawable->id);
	img_has_alpha = gimp_drawable_has_alpha(drawable->id);

	gimp_drawable_mask_bounds(drawable->id, &sel_x1, &sel_y1, &sel_x2, &sel_y2);

	/* Calculate scaling parameters */

	sel_width  = sel_x2 - sel_x1;
	sel_height = sel_y2 - sel_y1;

	cen_x = (double) (sel_x1 + sel_x2 - 1) / 2.0;
	cen_y = (double) (sel_y1 + sel_y2 - 1) / 2.0;

	xhsiz = (double) (sel_width - 1) / 2.0;
	yhsiz = (double) (sel_height - 1) / 2.0;

	if (xhsiz < yhsiz) {
		scale_x = yhsiz / xhsiz;
		scale_y = 1.0;
	} else if (xhsiz > yhsiz) {
		scale_x = 1.0;
		scale_y = xhsiz / yhsiz;
	} else {
		scale_x = 1.0;
		scale_y = 1.0;
	} /* else */

	/* Calculate preview size */

	if (sel_width > sel_height) {
		pwidth  = MIN(sel_width, PREVIEW_SIZE);
		pheight = sel_height * pwidth / sel_width;
	} else {
		pheight = MIN(sel_height, PREVIEW_SIZE);
		pwidth  = sel_width * pheight / sel_height;
	} /* else */

	preview_width  = MAX(pwidth, 2); /* Min size is 2 */
	preview_height = MAX(pheight, 2);

	/* See how we will run */

	switch (run_mode) {
		case RUN_INTERACTIVE:
			/* Possibly retrieve data */

			gimp_get_data(PLUG_IN_NAME, &pcvals);

			/* Get information from the dialog */

			if (!polarize_dialog())
				return;

			break;

		case RUN_NONINTERACTIVE:
			/* Make sure all the arguments are present */

			if (nparams != 8)
				status = STATUS_CALLING_ERROR;

			if (status == STATUS_SUCCESS) {
				pcvals.circle  = param[3].data.d_float;
				pcvals.angle  = param[4].data.d_float;
				pcvals.backwards  = param[5].data.d_int32;
				pcvals.inverse  = param[6].data.d_int32;
				pcvals.polrec  = param[7].data.d_int32;
			} /* if */

			break;

		case RUN_WITH_LAST_VALS:
			/* Possibly retrieve data */

			gimp_get_data(PLUG_IN_NAME, &pcvals);
			break;

		default:
			break;
	} /* switch */

	/* Distort the image */

	if ((status == STATUS_SUCCESS) &&
	    (gimp_drawable_color(drawable->id) ||
	     gimp_drawable_gray(drawable->id))) {
		/* Set the tile cache size */

		gimp_tile_cache_ntiles(2 * (drawable->width + gimp_tile_width() - 1) / gimp_tile_width());

		/* Run! */

		polarize();

		/* If run mode is interactive, flush displays */

		if (run_mode != RUN_NONINTERACTIVE)
			gimp_displays_flush();

		/* Store data */

		if (run_mode == RUN_INTERACTIVE)
			gimp_set_data(PLUG_IN_NAME, &pcvals, sizeof(polarize_vals_t));
	} else if (status == STATUS_SUCCESS)
		status = STATUS_EXECUTION_ERROR;

	values[0].data.d_status = status;

	gimp_drawable_detach(drawable);
} /* run */


/*****/

static void
polarize(void) {
  GPixelRgn        dest_rgn;
  guchar           *dest, *d;
  guchar           pixel[4][4];
  guchar           pixel2[4];
  guchar           values[4];
  gint             progress, max_progress;
  double           cx, cy;
  guchar           bg_color[4];
  gint    x1, y1, x2, y2;
  gint    x, y, b;
  gpointer pr;

  
  pixel_fetcher_t *pft;

#if 0
  printf("Waiting... (pid %d)\n", getpid());
  kill(getpid(), SIGSTOP);
#endif

  /* Get selection area */

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);

  /* Initialize pixel region */

  gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);
  
  pft = pixel_fetcher_new(drawable);

  gimp_palette_get_background(&bg_color[0], &bg_color[1], &bg_color[2]);
  pixel_fetcher_set_bg_color(pft, bg_color[0], bg_color[1], bg_color[2], (img_has_alpha ? 0 : 255));

  progress     = 0;
  max_progress = img_width * img_height;

  gimp_progress_init("Polarizing...");

  for (pr = gimp_pixel_rgns_register (1, &dest_rgn); pr != NULL; pr = gimp_pixel_rgns_process (pr)) {
    dest = dest_rgn.data;

    for (y = dest_rgn.y; y < (dest_rgn.y + dest_rgn.h); y++) {
      d = dest;

      for (x = dest_rgn.x; x < (dest_rgn.x + dest_rgn.w); x++) {
 	if (calc_undistorted_coords(x, y, &cx, &cy)) {
	  pixel_fetcher_get_pixel(pft, cx, cy, pixel[0]);
	  pixel_fetcher_get_pixel(pft, cx + 1, cy, pixel[1]);
	  pixel_fetcher_get_pixel(pft, cx, cy + 1, pixel[2]);
	  pixel_fetcher_get_pixel(pft, cx + 1, cy + 1, pixel[3]);

	  for (b = 0; b < img_bpp; b++) {
	    values[0] = pixel[0][b];
	    values[1] = pixel[1][b];
	    values[2] = pixel[2][b];
	    values[3] = pixel[3][b];
	   
	    d[b] = bilinear(cx, cy, values);
	  }
	} else {
	  pixel_fetcher_get_pixel(pft, x, y, pixel2);
   	  for (b = 0; b < img_bpp; b++) {
           d[b] = 255;
	  }	  
	}

	d += dest_rgn.bpp;
      }
      
      dest += dest_rgn.rowstride;
    }
    progress += dest_rgn.w *dest_rgn.h;
    
    gimp_progress_update((double) progress / max_progress);
  }
  
  gimp_drawable_flush(drawable);
  gimp_drawable_merge_shadow(drawable->id, TRUE);
  gimp_drawable_update(drawable->id, x1, y1, (x2 - x1), (y2 - y1));
} /* polarize */


/*****/

static int
calc_undistorted_coords(double wx, double wy,
			double *x, double *y)
{
  int    inside;
  double phi, phi2;
  double xx, xm, ym, yy;
  int xdiff, ydiff;
  double r;
  double m;
  double xmax, ymax, rmax;
  double x_calc, y_calc;
  double xi, yi;
  double circle, angl, t, angle;
  int x1, x2, y1, y2;

  /* initialize */

  phi = 0.0;
  r = 0.0;

  x1 = 0;
  y1 = 0;
  x2 = img_width;
  y2 = img_height;
  xdiff = x2 - x1;
  ydiff = y2 - y1;
  xm = xdiff / 2.0;
  ym = ydiff / 2.0;
  circle = pcvals.circle;
  angle = pcvals.angle;
  angl = (double)angle / 180.0 * M_PI;

  if (pcvals.polrec) {
    if (wx >= cen_x) {
      if (wy > cen_y) {
	phi = M_PI - atan (((double)(wx - cen_x))/((double)(wy - cen_y)));
	r   = sqrt (sqr (wx - cen_x) + sqr (wy - cen_y));
      } else if (wy < cen_y) {
	phi = atan (((double)(wx - cen_x))/((double)(cen_y - wy)));
	r   = sqrt (sqr (wx - cen_x) + sqr (cen_y - wy));
      } else {
	phi = M_PI / 2;
	r   = wx - cen_x; /* cen_x - x1; */
      }
    } else if (wx < cen_x) {
      if (wy < cen_y) {
	phi = 2 * M_PI - atan (((double)(cen_x -wx))/((double)(cen_y - wy)));
	r   = sqrt (sqr (cen_x - wx) + sqr (cen_y - wy));
      } else if (wy > cen_y) {
        phi = M_PI + atan (((double)(cen_x - wx))/((double)(wy - cen_y)));
        r   = sqrt (sqr (cen_x - wx) + sqr (wy - cen_y));
      } else {
        phi = 1.5 * M_PI;
        r   = cen_x - wx; /* cen_x - x1; */
      }
    }
    if (wx != cen_x) {
      m = fabs (((double)(wy - cen_y)) / ((double)(wx - cen_x)));
    } else {
      m = 0;
    }
    
    if (m <= ((double)(y2 - y1) / (double)(x2 - x1))) {
      if (wx == cen_x) {
	xmax = 0;
	ymax = cen_y - y1;
      } else {
	xmax = cen_x - x1;
	ymax = m * xmax;
      }
    } else {
      ymax = cen_y - y1;
      xmax = ymax / m;
    }
    
    rmax = sqrt ( (double)(sqr (xmax) + sqr (ymax)) );
    
    t = ((cen_y - y1) < (cen_x - x1)) ? (cen_y - y1) : (cen_x - x1);
    rmax = (rmax - t) / 100 * (100 - circle) + t;
    
    phi = fmod (phi + angl, 2*M_PI);
    
    if (pcvals.backwards)
      x_calc = x2 - 1 - (x2 - x1 - 1)/(2*M_PI) * phi;
    else
      x_calc = (x2 - x1 - 1)/(2*M_PI) * phi + x1;
    
    if (pcvals.inverse)
      y_calc = (y2 - y1)/rmax   * r   + y1;
    else
      y_calc = y2 - (y2 - y1)/rmax * r;
    
    xi = (int) (x_calc+0.5);
    yi = (int) (y_calc+0.5);
    
    if (WITHIN(0, xi, img_width - 1) && WITHIN(0, yi, img_height - 1)) {
      *x = x_calc;
      *y = y_calc;
      
      inside = TRUE;
    } else {
      inside = FALSE;
    }
  } else {
    
    if (pcvals.backwards)
      phi = (2 * M_PI) * (x2 - wx) / xdiff;
    else
      phi = (2 * M_PI) * (wx - x1) / xdiff;
    
    phi = fmod (phi + angl, 2 * M_PI);
    
    if (phi >= 1.5 * M_PI)
      phi2 = 2 * M_PI - phi;
    else
      if (phi >= M_PI)
	phi2 = phi - M_PI;
      else
	if (phi >= 0.5 * M_PI)
	  phi2 = M_PI - phi;
	else
	  phi2 = phi;
    
    xx = tan (phi2);
    if (xx != 0)
      m = (double) 1.0 / xx;
    else
      m = 0;
    
    if (m <= ((double)(ydiff) / (double)(xdiff)))
      {
	if (phi2 == 0)
	  {
	    xmax = 0;
	    ymax = ym - y1;
	  }
	else
	  {
	    xmax = xm - x1;
	    ymax = m * xmax;
	  }
      }
    else
      {
	ymax = ym - y1;
	xmax = ymax / m;
      }
    
    rmax = sqrt ((double)(sqr (xmax) + sqr (ymax)));
    
    t = ((ym - y1) < (xm - x1)) ? (ym - y1) : (xm - x1);
    
    rmax = (rmax - t) / 100.0 * (100 - circle) + t;
    
    if (pcvals.inverse)
      r = rmax * (double)((wy - y1) / (double)(ydiff));
    else
      r = rmax * (double)((y2 - wy) / (double)(ydiff));
    
    xx = r * sin (phi2);
    yy = r * cos (phi2);
    
    if (phi >= 1.5 * M_PI)
      {
	x_calc = (double)xm - xx;
	y_calc = (double)ym - yy;
      }
    else
      if (phi >= M_PI)
	{
	  x_calc = (double)xm - xx;
	  y_calc = (double)ym + yy;
	}
      else
	if (phi >= 0.5 * M_PI)
	  {
	    x_calc = (double)xm + xx;
	    y_calc = (double)ym + yy;
	  }
	else
	  {
	    x_calc = (double)xm + xx;
	    y_calc = (double)ym - yy;
	  }
    
    xi = (int)(x_calc + 0.5);
    yi = (int)(y_calc + 0.5);
  
    if (WITHIN(0, xi, img_width - 1) && WITHIN(0, yi, img_height - 1)) {
      *x = x_calc;
      *y = y_calc;
      
      inside = TRUE;
    } else {
      inside = FALSE;
    }
  }
  
  return inside;
}
 /* calc_undistorted_coords */

/*****/
static guchar
bilinear(double x, double y, guchar *values)
{
        double m0, m1;

        x = fmod(x, 1.0);
        y = fmod(y, 1.0);

        if (x < 0.0)
                x += 1.0;

        if (y < 0.0)
                y += 1.0;

        m0 = (double) values[0] + x * ((double) values[1] - values[0]);
        m1 = (double) values[2] + x * ((double) values[3] - values[2]);

        return (guchar) (m0 + y * (m1 - m0));
} /* bilinear */

/*****/

static pixel_fetcher_t *
pixel_fetcher_new(GDrawable *drawable)
{
	pixel_fetcher_t *pf;

	pf = g_malloc(sizeof(pixel_fetcher_t));

	pf->col           = -1;
	pf->row           = -1;
	pf->img_width     = gimp_drawable_width(drawable->id);
	pf->img_height    = gimp_drawable_height(drawable->id);
	pf->img_bpp       = gimp_drawable_bpp(drawable->id);
	pf->img_has_alpha = gimp_drawable_has_alpha(drawable->id);
	pf->tile_width    = gimp_tile_width();
	pf->tile_height   = gimp_tile_height();
	pf->bg_color[0]   = 0;
	pf->bg_color[1]   = 0;
	pf->bg_color[2]   = 0;
	pf->bg_color[3]   = 0;

	pf->drawable    = drawable;
	pf->tile        = NULL;

	return pf;
} /* pixel_fetcher_new */


/*****/

static void
pixel_fetcher_set_bg_color(pixel_fetcher_t *pf, guchar r, guchar g, guchar b, guchar a)
{
	pf->bg_color[0] = r;
	pf->bg_color[1] = g;
	pf->bg_color[2] = b;

	if (pf->img_has_alpha)
		pf->bg_color[pf->img_bpp - 1] = a;
} /* pixel_fetcher_set_bg_color */


/*****/

static void
pixel_fetcher_get_pixel(pixel_fetcher_t *pf, int x, int y, guchar *pixel)
{
	gint    col, row;
	gint    coloff, rowoff;
	guchar *p;
	int     i;

	if ((x < sel_x1) || (x >= sel_x2) ||
	    (y < sel_y1) || (y >= sel_y2)) {
		for (i = 0; i < pf->img_bpp; i++)
			pixel[i] = pf->bg_color[i];

		return;
	} /* if */

	col    = x / pf->tile_width;
	coloff = x % pf->tile_width;
	row    = y / pf->tile_height;
	rowoff = y % pf->tile_height;

	if ((col != pf->col) ||
	    (row != pf->row) ||
	    (pf->tile == NULL)) {
		if (pf->tile != NULL)
			gimp_tile_unref(pf->tile, FALSE);

		pf->tile = gimp_drawable_get_tile(pf->drawable, FALSE, row, col);
		gimp_tile_ref(pf->tile);

		pf->col = col;
		pf->row = row;
	} /* if */

	p = pf->tile->data + pf->img_bpp * (pf->tile->ewidth * rowoff + coloff);

	for (i = pf->img_bpp; i; i--)
		*pixel++ = *p++;
} /* pixel_fetcher_get_pixel */


/*****/

static void
pixel_fetcher_destroy(pixel_fetcher_t *pf)
{
	if (pf->tile != NULL)
		gimp_tile_unref(pf->tile, FALSE);

	g_free(pf);
} /* pixel_fetcher_destroy */


/*****/

static void
build_preview_source_image(void)
{
	double           left, right, bottom, top;
	double           px, py;
	double           dx, dy;
	int              x, y;
	guchar          *p;
	guchar           pixel[4];
	pixel_fetcher_t *pf;

	pcint.check_row_0 = g_malloc(preview_width * sizeof(guchar));
	pcint.check_row_1 = g_malloc(preview_width * sizeof(guchar));
	pcint.image       = g_malloc(preview_width * preview_height * 4 * sizeof(guchar));
	pcint.dimage      = g_malloc(preview_width * preview_height * 3 * sizeof(guchar));

	left   = sel_x1;
	right  = sel_x2 - 1;
	bottom = sel_y2 - 1;
	top    = sel_y1;

	dx = (right - left) / (preview_width - 1);
	dy = (bottom - top) / (preview_height - 1);

	py = top;

	pf = pixel_fetcher_new(drawable);

	p = pcint.image;

	for (y = 0; y < preview_height; y++) {
		px = left;

		for (x = 0; x < preview_width; x++) {
			/* Checks */

			if ((x / CHECK_SIZE) & 1) {
				pcint.check_row_0[x] = CHECK_DARK;
				pcint.check_row_1[x] = CHECK_LIGHT;
			} else {
				pcint.check_row_0[x] = CHECK_LIGHT;
				pcint.check_row_1[x] = CHECK_DARK;
			} /* else */

			/* Thumbnail image */

			pixel_fetcher_get_pixel(pf, (int) px, (int) py, pixel);

			if (img_bpp < 3) {
				if (img_has_alpha)
					pixel[3] = pixel[1];
				else
					pixel[3] = 255;

				pixel[1] = pixel[0];
				pixel[2] = pixel[0];
			} else
				if (!img_has_alpha)
					pixel[3] = 255;

			*p++ = pixel[0];
			*p++ = pixel[1];
			*p++ = pixel[2];
			*p++ = pixel[3];

			px += dx;
		} /* for */

		py += dy;
	} /* for */

	pixel_fetcher_destroy(pf);
} /* build_preview_source_image */


/*****/

static gint
polarize_dialog(void)
{
	GtkWidget  *dialog;
	GtkWidget  *top_table;
	GtkWidget  *frame;
	GtkWidget  *table;
	GtkWidget  *button;
	GtkWidget  *toggle;
	GtkWidget  *hbox;
	GtkTooltips  *tips;
	GdkColor tips_fg, tips_bg;
	gint        argc;
	gchar     **argv;
	guchar     *color_cube;
#if 0
	printf("Waiting... (pid %d)\n", getpid());
	kill(getpid(), SIGSTOP);
#endif
	argc    = 1;
	argv    = g_new(gchar *, 1);
	argv[0] = g_strdup("polarize");

	gtk_init(&argc, &argv);
	gtk_rc_parse (gimp_gtkrc ());

	gdk_set_use_xshm(gimp_use_xshm());

	gtk_preview_set_gamma(gimp_gamma());
	gtk_preview_set_install_cmap(gimp_install_cmap());
	color_cube = gimp_color_cube();
	gtk_preview_set_color_cube(color_cube[0], color_cube[1], color_cube[2], color_cube[3]);

	gtk_widget_set_default_visual(gtk_preview_get_visual());
	gtk_widget_set_default_colormap(gtk_preview_get_cmap());

	build_preview_source_image();

	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), "Polarize");
	gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
	gtk_container_border_width(GTK_CONTAINER(dialog), 0);
	gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
			   (GtkSignalFunc) dialog_close_callback,
			   NULL);

	top_table = gtk_table_new(3, 3, FALSE);
	gtk_container_border_width(GTK_CONTAINER(top_table), 6);
	gtk_table_set_row_spacings(GTK_TABLE(top_table), 4);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), top_table, FALSE, FALSE, 0);
	gtk_widget_show(top_table);

	/* Initialize Tooltips */

	/* use black as foreground: */
	tips = gtk_tooltips_new ();
	tips_fg.red   = 0;
	tips_fg.green = 0;
	tips_fg.blue  = 0;
	/* postit yellow (khaki) as background: */
	gdk_color_alloc (gtk_widget_get_colormap (dialog), &tips_fg);
	tips_bg.red   = 61669;
	tips_bg.green = 59113;
	tips_bg.blue  = 35979;
	gdk_color_alloc (gtk_widget_get_colormap (dialog), &tips_bg);
	gtk_tooltips_set_colors (tips,&tips_bg,&tips_fg);
	
	/* Preview */

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_table_attach(GTK_TABLE(top_table), frame, 1, 2, 0, 1, 0, 0, 0, 0);
	gtk_widget_show(frame);

	pcint.preview = gtk_preview_new(GTK_PREVIEW_COLOR);
	gtk_preview_size(GTK_PREVIEW(pcint.preview), preview_width, preview_height);
	gtk_container_add(GTK_CONTAINER(frame), pcint.preview);
	gtk_widget_show(pcint.preview);

	/* Controls */

	table = gtk_table_new(2, 3, FALSE);
	gtk_container_border_width(GTK_CONTAINER(table), 0);
	gtk_table_attach(GTK_TABLE(top_table), table, 0, 3, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	gtk_widget_show(table);

	dialog_create_value("Circle depth in percent", GTK_TABLE(table), 0, &pcvals.circle, 0, 100.0, 1.0);
	dialog_create_value("Offset angle", GTK_TABLE(table), 1, &pcvals.angle, 0, 359, 1.0);


	/* togglebuttons for backwards, top, polar->rectangular */

	hbox = gtk_hbox_new (TRUE, 2);
	gtk_table_attach( GTK_TABLE(top_table), hbox, 0, 3, 2, 3, 
							GTK_FILL, 0 , 0, 0);

	toggle = gtk_check_button_new_with_label("Map Backwards");
	gtk_toggle_button_set_state( GTK_TOGGLE_BUTTON(toggle), pcvals.backwards);
	gtk_signal_connect(GTK_OBJECT(toggle), "toggled", 
			   (GtkSignalFunc) polar_toggle_callback,
			   &pcvals.backwards);
	gtk_box_pack_start( GTK_BOX (hbox), toggle, TRUE, TRUE, 0);
	gtk_widget_show(toggle);
	set_tooltip(tips,toggle,"If checked the mapping will begin at the right side, as opposed to beginning at the left.");

	toggle = gtk_check_button_new_with_label("Map from Top");
	gtk_toggle_button_set_state( GTK_TOGGLE_BUTTON(toggle), pcvals.inverse);
	gtk_signal_connect( GTK_OBJECT(toggle), "toggled", 
			    (GtkSignalFunc) polar_toggle_callback,
			    &pcvals.inverse);
	gtk_box_pack_start( GTK_BOX (hbox), toggle, TRUE, TRUE, 0);
	gtk_widget_show(toggle);
	set_tooltip(tips,toggle,"If unchecked the mapping will put the bottom row in the middle and the top row on the outside.  If checked it will be the opposite.");

	toggle = gtk_check_button_new_with_label("Polar to Rectangular");
	gtk_toggle_button_set_state( GTK_TOGGLE_BUTTON(toggle), pcvals.polrec);
	gtk_signal_connect( GTK_OBJECT(toggle), "toggled", 
			    (GtkSignalFunc) polar_toggle_callback,
			    &pcvals.polrec);
	gtk_box_pack_start( GTK_BOX (hbox), toggle, TRUE, TRUE, 0);
	gtk_widget_show(toggle);
	set_tooltip(tips,toggle,"If unchecked the image will be circularly mapped onto a rectangle.  If checked the image will be mapped onto a circle.");

	gtk_widget_show(hbox);

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
	gtk_object_unref(GTK_OBJECT(tips));
	gdk_flush();

	g_free(pcint.check_row_0);
	g_free(pcint.check_row_1);
	g_free(pcint.image);
	g_free(pcint.dimage);

	return pcint.run;
} /* polarize_dialog */


/*****/

static void
dialog_update_preview(void)
{
	double  left, right, bottom, top;
	double  dx, dy;
	double  px, py;
	double  cx = 0.0, cy = 0.0;
	int     ix, iy;
	int     x, y;
	double  scale_x, scale_y;
	guchar *p_ul, *i, *p;
	guchar *check_ul;
	int     check;
	guchar  outside[4];

	gimp_palette_get_background(&outside[0], &outside[1], &outside[2]);
	outside[3] = (img_has_alpha ? 0 : 255);

	if (img_bpp < 3) {
		outside[1] = outside[0];
		outside[2] = outside[0];
	} /* if */

	left   = sel_x1;
	right  = sel_x2 - 1;
	bottom = sel_y2 - 1;
	top    = sel_y1;

	dx = (right - left) / (preview_width - 1);
	dy = (bottom - top) / (preview_height - 1);

	scale_x = (double) preview_width / (right - left + 1);
	scale_y = (double) preview_height / (bottom - top + 1);

	py = top;

	p_ul = pcint.dimage;
/*	p_lr = pcint.dimage + 3 * (preview_width * preview_height - 1);*/

	for (y = 0; y < preview_height; y++) {
		px = left;

		if ((y / CHECK_SIZE) & 1)
			check_ul = pcint.check_row_0;
		else
			check_ul = pcint.check_row_1;

		for (x = 0; x < preview_width; x++) {
			calc_undistorted_coords(px, py, &cx, &cy);

			cx = (cx - left) * scale_x;
			cy = (cy - top) * scale_y;

			ix = (int) (cx + 0.5);
			iy = (int) (cy + 0.5);

			check = check_ul[x];

			if ((ix >= 0) && (ix < preview_width) &&
			    (iy >= 0) && (iy < preview_height))
				i = pcint.image + 4 * (preview_width * iy + ix);
			else
				i = outside;

			p_ul[0] = check + ((i[0] - check) * i[3]) / 255;
			p_ul[1] = check + ((i[1] - check) * i[3]) / 255;
			p_ul[2] = check + ((i[2] - check) * i[3]) / 255;

			p_ul += 3;

			px += dx;
		} /* for */

		py += dy;
	} /* for */

	p = pcint.dimage;

	for (y = 0; y < img_height; y++) {
		gtk_preview_draw_row(GTK_PREVIEW(pcint.preview), p, 0, y, preview_width);

		p += preview_width * 3;
	} /* for */

	gtk_widget_draw(pcint.preview, NULL);
	gdk_flush();
} /* dialog_update_preview */


/*****/

static void
dialog_create_value(char *title, GtkTable *table, int row, gdouble *value,
		    double left, double right, double step)
{
	GtkWidget *label;
	GtkWidget *scale;
	GtkWidget *entry;
	GtkObject *scale_data;
	char       buf[256];

	label = gtk_label_new(title);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_table_attach(table, label, 0, 1, row, row + 1, GTK_FILL, GTK_FILL, 4, 0);
	gtk_widget_show(label);

	scale_data = gtk_adjustment_new(*value, left, right,
					step,
					step,
					0.0);

	gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
			   (GtkSignalFunc) dialog_scale_update,
			   value);

	scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
	gtk_widget_set_usize(scale, SCALE_WIDTH, 0);
	gtk_table_attach(table, scale, 1, 2, row, row + 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
	gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
	gtk_scale_set_digits(GTK_SCALE(scale), 3);
	gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
	gtk_widget_show(scale);

	entry = gtk_entry_new();
	gtk_object_set_user_data(GTK_OBJECT(entry), scale_data);
	gtk_object_set_user_data(scale_data, entry);
	gtk_widget_set_usize(entry, ENTRY_WIDTH, 0);
	sprintf(buf, "%0.3f", *value);
	gtk_entry_set_text(GTK_ENTRY(entry), buf);
	gtk_signal_connect(GTK_OBJECT(entry), "changed",
			   (GtkSignalFunc) dialog_entry_update,
			   value);
	gtk_table_attach(GTK_TABLE(table), entry, 2, 3, row, row + 1, GTK_FILL, GTK_FILL, 4, 0);
	gtk_widget_show(entry);
} /* dialog_create_value */


/*****/

static void
dialog_scale_update(GtkAdjustment *adjustment, gdouble *value)
{
	GtkWidget *entry;
	char       buf[256];

	if (*value != adjustment->value) {
		*value = adjustment->value;

		entry = gtk_object_get_user_data(GTK_OBJECT(adjustment));
		sprintf(buf, "%0.3f", *value);

		gtk_signal_handler_block_by_data(GTK_OBJECT(entry), value);
		gtk_entry_set_text(GTK_ENTRY(entry), buf);
		gtk_signal_handler_unblock_by_data(GTK_OBJECT(entry), value);

		dialog_update_preview();
	} /* if */
} /* dialog_scale_update */


/*****/

static void
dialog_entry_update(GtkWidget *widget, gdouble *value)
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
} /* dialog_entry_update */


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
	pcint.run = TRUE;
	gtk_widget_destroy(GTK_WIDGET(data));
} /* dialog_ok_callback */


/*****/

static void
dialog_cancel_callback(GtkWidget *widget, gpointer data)
{
	gtk_widget_destroy(GTK_WIDGET(data));
} /* dialog_cancel_callback */

static void polar_toggle_callback (GtkWidget *widget, gpointer   data)
{
  gint *toggle_val;

  toggle_val = (gint *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;

  dialog_update_preview();
}

static void
set_tooltip (GtkTooltips *tooltips, GtkWidget *widget, const char *desc)
{
  if (desc && desc[0])
    gtk_tooltips_set_tip (tooltips, widget, (char *) desc, NULL);
}
