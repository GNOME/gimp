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
#include <stdlib.h>
#include "libgimp/gimp.h"

/* Declare local functions.
 */
static void      query  (void);
static void      run    (char      *name,
			 int        nparams,
			 GParam    *param,
			 int       *nreturn_vals,
			 GParam   **return_vals);

static void      blur             (GDrawable  *drawable);
static void      blur_prepare_row (GPixelRgn  *pixel_rgn,
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

  gimp_install_procedure ("plug_in_blur",
			  "Blur the contents of the specified drawable",
			  "This function applies a 3x3 blurring convolution kernel to the specified drawable.",
			  "Spencer Kimball & Peter Mattis",
			  "Spencer Kimball & Peter Mattis",
			  "1995-1996",
			  "<Image>/Filters/Blur/Blur",
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
      gimp_progress_init ("blur");
      gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));
      blur (drawable);

      if (run_mode != RUN_NONINTERACTIVE)
	gimp_displays_flush ();
    }
  else
    {
      /* gimp_message ("blur: cannot operate on indexed color images"); */
      status = STATUS_EXECUTION_ERROR;
    }

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static void
blur_prepare_row (GPixelRgn *pixel_rgn,
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
      data[-pixel_rgn->bpp + b] = data[b];
      data[w * pixel_rgn->bpp + b] = data[(w - 1) * pixel_rgn->bpp + b];
    }
}

static void
blur (GDrawable *drawable)
{
  GPixelRgn srcPR, destPR;
  gint width, height;
  gint bytes;
  guchar *dest, *d;
  guchar *prev_row, *pr;
  guchar *cur_row, *cr;
  guchar *next_row, *nr;
  guchar *tmp;
  gint row, col;
  gint x1, y1, x2, y2;
  gint has_alpha, ind;
  
    
  /* Get the input area. This is the bounding box of the selection in
   *  the image (or the entire image if there is no selection). Only
   *  operating on the input area is simply an optimization. It doesn't
   *  need to be done for correct operation. (It simply makes it go
   *  faster, since fewer pixels need to be operated on).
   */
  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);

  /* Get the size of the input image. (This will/must be the same
   *  as the size of the output image.
   */
  width = drawable->width;
  height = drawable->height;
  bytes = drawable->bpp;
  has_alpha = gimp_drawable_has_alpha(drawable->id);
  
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

  blur_prepare_row (&srcPR, pr, x1, y1 - 1, (x2 - x1));
  blur_prepare_row (&srcPR, cr, x1, y1, (x2 - x1));

  /*  loop through the rows, applying the blur convolution  */
  for (row = y1; row < y2; row++)
    {
      /*  prepare the next row  */
      blur_prepare_row (&srcPR, nr, x1, row + 1, (x2 - x1));

      d = dest;
      ind = 0;
      for (col = 0; col < (x2 - x1) * bytes; col++)
	  {
	      ind++;
	      
	      if (ind==bytes || !(has_alpha))
		  {  /* we always do the alpha channel, 
			or if there's none we have no problem
			so the algorithm stays the same */
		  *d++ = ((gint) pr[col - bytes] + (gint) pr[col] + (gint) pr[col + bytes] +
			  (gint) cr[col - bytes] + (gint) cr[col] + (gint) cr[col + bytes] +
			  (gint) nr[col - bytes] + (gint) nr[col] + (gint) nr[col + bytes]) / 9;
		  ind=0;
	      }
	      else { /* we have an alpha channel picture, and do the color part here */
		      *d++ = ((gint) (((gdouble) (pr[col - bytes] * pr[col - ind]) 
				       + (gdouble) (pr[col]         * pr[col + bytes - ind])
				       + (gdouble) (pr[col + bytes] * pr[col + 2*bytes - ind])
				       + (gdouble) (cr[col - bytes] * cr[col - ind])
				       + (gdouble) (cr[col]         * cr[col + bytes - ind]) 
				       + (gdouble) (cr[col + bytes] * cr[col + 2*bytes - ind])
				       + (gdouble) (nr[col - bytes] * nr[col - ind])
				       + (gdouble) (nr[col]         * nr[col + bytes - ind])
				       + (gdouble) (nr[col + bytes] * nr[col + 2*bytes - ind]))
				      / ((gdouble) pr[col - ind]
					 + (gdouble) pr[col + bytes - ind]
					 + (gdouble) pr[col + 2*bytes - ind]
					 + (gdouble) cr[col - ind]
					 + (gdouble) cr[col + bytes - ind]
					 + (gdouble) cr[col + 2*bytes - ind]
					 + (gdouble) nr[col - ind]
					 + (gdouble) nr[col + bytes - ind]
					 + (gdouble) nr[col + 2*bytes - ind])));
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

  /*  update the blurred region  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));

  free (prev_row);
  free (cur_row);
  free (next_row);
  free (dest);
}
