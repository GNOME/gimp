/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

/* This plugin by thorsten@arch.usyd.edu.au           */
/* Based on S&P's Gauss and Laplace filters              */

/* updated 11/04/97:
   Use 8-pixel neighbourhood to create outline,
   use min-max operation for local gradient,
   don't use rint;
   if gamma-channel: set to white if at least one colour channel is >15 */

/* update 03/10/97
   #ifdef MAX and MIN */

#include <stdlib.h>
#include "libgimp/gimp.h"
#include <stdio.h>
#include <math.h>

/* Declare local functions.
 */
static void      query  (void);
static void      run    (char      *name,
			 int        nparams,
			 GParam    *param,
			 int       *nreturn_vals,
			 GParam   **return_vals);

static void      laplace             (GDrawable  *drawable);
static void      laplace_prepare_row (GPixelRgn  *pixel_rgn,
				   guchar     *data,
				   int         x,
				   int         y,
				   int         w);


GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};


MAIN ()

static void
query ()
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (unused)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure ("plug_in_laplace",
			  "Edge Detection with Laplace Operation",
			  "This plugin creates one-pixel wide edges from the image, with the value proportional to the gradient. It uses the Laplace operator (a 3x3 kernel with -8 in the middle)The image has to be laplacered to get usefull results, a gauss_iir with 1.5 - 5.0 depending on the noise in the image is best",
			  "Thorsten Schnier",
			  "Thorsten Schnier",
			  "1997",
			  "<Image>/Filters/Edge-Detect/Laplace",
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  /*  Make sure that the drawable is gray or RGB color  */
  if (gimp_drawable_color (drawable->id) || gimp_drawable_gray (drawable->id))
    {
      gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));
      laplace (drawable);

      if (run_mode != RUN_NONINTERACTIVE)
	gimp_displays_flush ();
    }
  else
    {
      /* gimp_message ("laplace: cannot operate on indexed color images"); */
      status = STATUS_EXECUTION_ERROR;
    }

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static void
laplace_prepare_row (GPixelRgn *pixel_rgn,
		  guchar    *data,
		  int        x,
		  int        y,
		  int        w)
{
  int b;

  if (y == 0)
    gimp_pixel_rgn_get_row (pixel_rgn, data, x, (y + 1), w);
  else if (y == pixel_rgn->h)
    gimp_pixel_rgn_get_row (pixel_rgn, data, x, (y - 1), w);
  else
    gimp_pixel_rgn_get_row (pixel_rgn, data, x, y, w);

  /*  Fill in edge pixels  */
  for (b = 0; b < pixel_rgn->bpp; b++)
    {
      data[-(int)pixel_rgn->bpp + b] = data[b];
      data[w * pixel_rgn->bpp + b] = data[(w - 1) * pixel_rgn->bpp + b];
    }
}

#define SIGN(a) (((a) > 0) ? 1 : -1)
#define RMS(a,b) (sqrt (pow ((a),2) + pow ((b), 2)))
#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif
#define BLACK_REGION(val) ((val) > 128)
#define WHITE_REGION(val) ((val) <= 128)


static void minmax  (gint x1, gint x2, gint x3, gint x4, gint x5,
		     gint* min_result, gint* max_result)
{
  gint min1,min2,max1,max2;
  if (x1>x2) {max1=x1; min1=x2;} else {max1=x2; min1=x1;}
  if (x3>x4) {max2=x3; min2=x4;} else {max2=x4; min2=x3;}
  if (min1<min2)
    *min_result = MIN (min1, x5);
      else  *min_result = MIN (min2, x5);
  if (max1>max2)
    *max_result = MAX (max1, x5);
      else  *max_result = MAX (max2, x5);

}

static void
laplace (GDrawable *drawable)
{
  GPixelRgn srcPR, destPR;
  gint width, height;
  gint bytes;
  gint current;
  gint gradient;
  gint max_gradient = 0;
  gint alpha;
  gint counter;
  guchar *dest, *d;
  guchar *prev_row, *pr;
  guchar *cur_row, *cr;
  guchar *next_row, *nr;
  guchar *tmp;
  gint row, col;
  gint x1, y1, x2, y2;
  gint minval, maxval;
  float scale = 1.0;

  /* Get the input area. This is the bounding box of the selection in
   *  the image (or the entire image if there is no selection). Only
   *  operating on the input area is simply an optimization. It doesn't
   *  need to be done for correct operation. (It simply makes it go
   *  faster, since fewer pixels need to be operated on).
   */

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
  gimp_progress_init ("Laplace");

  /* Get the size of the input image. (This will/must be the same
   *  as the size of the output image.
   */
  width = drawable->width;
  height = drawable->height;
  bytes = drawable->bpp;
  alpha = gimp_drawable_has_alpha (drawable -> id);
  /*  allocate row buffers  */
  prev_row = (guchar *) malloc ((x2 - x1 + 2) * bytes);
  cur_row = (guchar *) malloc ((x2 - x1 + 2) * bytes);
  next_row = (guchar *) malloc ((x2 - x1 + 2) * bytes);
  dest = (guchar *) malloc ((x2 - x1) * bytes);

  /*  initialize the pixel regions  */
  gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, drawable, 0, 0, width, height, TRUE, TRUE);

  pr = prev_row + bytes;
  cr = cur_row + bytes;
  nr = next_row + bytes;

  laplace_prepare_row (&srcPR, pr, x1, y1 - 1, (x2 - x1));
  laplace_prepare_row (&srcPR, cr, x1, y1, (x2 - x1));

  /*  loop through the rows, applying the laplace convolution  */
  for (row = y1; row < y2; row++)
    {
      /*  prepare the next row  */
      laplace_prepare_row (&srcPR, nr, x1, row + 1, (x2 - x1));

      d = dest;
      for (col = 0; col < (x2 - x1) * bytes; col++)
	if (alpha && (((col + 1) % bytes) == 0)) /* the alpha channel */
	  *d++ = cr[col];
	else
	  {
	    minmax (pr[col], cr[col - bytes], cr[col], cr[col + bytes],
		    nr[col], &minval, &maxval); /* four-neighbourhood */
	    gradient = (0.5 * MIN ((maxval - cr [col]), (cr[col]- minval)));
	    max_gradient = MAX(abs(gradient), max_gradient);
	    *d++ = ((pr[col - bytes] + pr[col]     + pr[col + bytes] +
		     cr[col - bytes] - (8 * cr[col]) + cr[col + bytes] +
		     nr[col - bytes] + nr[col]       + nr[col + bytes]) > 0) ?
	      gradient : (128 + gradient);
	  }

      /*  store the dest  */
      gimp_pixel_rgn_set_row (&destPR, dest, x1, row, (x2 - x1));

      /*  shuffle the row pointers  */
      tmp = pr;
      pr = cr;
      cr = nr;
      nr = tmp;

      if ((row % 5) == 0)
	gimp_progress_update ((double) row / (double) (y2 - y1));
    }


  /* now clean up: leave only edges, but keep gradient value */


  gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, width, height, FALSE, TRUE);

  pr = prev_row + bytes;
  cr = cur_row + bytes;
  nr = next_row + bytes;

  laplace_prepare_row (&srcPR, pr, x1, y1 - 1, (x2 - x1));
  laplace_prepare_row (&srcPR, cr, x1, y1, (x2 - x1));

  gimp_progress_init ("Cleanup");
  scale = (255.0 / (float) max_gradient);
  counter =0;

  /*  loop through the rows, applying the laplace convolution  */
  for (row = y1; row < y2; row++)
    {
      /*  prepare the next row  */
      laplace_prepare_row (&srcPR, nr, x1, row + 1, (x2 - x1));

      d = dest;
      for (col = 0; col < (x2 - x1) * bytes; col++) {
	current = cr[col];
	current = (WHITE_REGION(current) &&
		   (BLACK_REGION (pr[col - bytes]) ||
		    BLACK_REGION (pr[col]) ||
		    BLACK_REGION (pr[col + bytes]) ||
		    BLACK_REGION (cr[col - bytes]) ||

		    BLACK_REGION (cr[col + bytes]) ||
		    BLACK_REGION (nr[col - bytes]) ||
		    BLACK_REGION (nr[col]) ||
		    BLACK_REGION (nr[col + bytes]))) ?
	  (gint) (scale * ((float) ((current >= 128) ?
				    (current-128) : current)))
	  : 0;
	if (alpha && (((col + 1) % bytes) == 0)) { /* the alpha channel */
	  *d++ = (counter == 0) ? 0 : 255;
	  counter = 0; }
	else {
	  *d++ = current;
	  if (current > 15) counter ++;
	}
      }
      /*  store the dest  */
      gimp_pixel_rgn_set_row (&destPR, dest, x1, row, (x2 - x1));

      /*  shuffle the row pointers  */
      tmp = pr;
      pr = cr;
      cr = nr;
      nr = tmp;

      if ((row % 5) == 0)
	gimp_progress_update ((double) row / (double) (y2 - y1));
    }

  /*  update the laplaced region  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));

  free (prev_row);
  free (cur_row);
  free (next_row);
  free (dest);
}
