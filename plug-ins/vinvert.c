/*
 *  Value-Invert plug-in v1.0 by Adam D. Moss, adam@foxbox.org.  1997/3/28
 */

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

/*
 * BUGS:
 *     Is not undoable when operating on indexed images.
 *     Uses a kludge to force image redraw with indexed images.
 */



#include <stdio.h>
#include <stdlib.h>
#include "libgimp/gimp.h"


/* Declare local functions.
 */
static void      query  (void);
static void      indexed_vinvert (gint32 image_ID);
static void      run    (char      *name,
			 int        nparams,
			 GParam    *param,
			 int       *nreturn_vals,
			 GParam   **return_vals);

static void      vinvert            (GDrawable  *drawable);
static void      vinvert_render_row (const guchar *src_row,
				     guchar *dest_row,
				     gint row,
				     gint row_width,
				     gint bytes);

void  fp_rgb_to_hsv            (float *, float *, float *);
void  fp_hsv_to_rgb            (float *, float *, float *);



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
    { PARAM_IMAGE, "image", "Input image (used for indexed images)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure ("plug_in_vinvert",
			  "Invert the 'value' componant of an indexed/RGB image in HSV colourspace",
			  "This function takes an indexed/RGB image and inverts its 'value' in HSV space.  The upshot of this is that the colour and saturation at any given point remains the same, but its brightness is effectively inverted.  Quite strange.  Tends to produce unpleasant colour artifacts on images from lossy sources (ie. JPEG).",
			  "Adam D. Moss (adam@foxbox.org)",
			  "Adam D. Moss (adam@foxbox.org)",
			  "27th March 1997",
			  "<Image>/Filters/Image/Value Invert",
			  "RGB*, INDEXED*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}



void
fp_rgb_to_hsv (float *r,
            float *g,
            float *b)
{
  int red, green, blue;
  float h=0, s, v;
  int min, max;
  int delta;

  red = *r;
  green = *g;
  blue = *b;

  if (red > green)
    {
      if (red > blue)
        max = red;
      else
        max = blue;

      if (green < blue)
        min = green;
      else
        min = blue;
    }
  else
    {
      if (green > blue)
        max = green;
      else
        max = blue;

      if (red < blue)
        min = red;
      else
        min = blue;
    }

  v = max;

  if (max != 0)
    s = ((max - min) * 255) / (float) max;
  else
    s = 0;

  if (s == 0)
    h = 0;
  else
    {
      delta = max - min;
      if (red == max)
        h = (green - blue) / (float) delta;
      else if (green == max)
        h = 2 + (blue - red) / (float) delta;
      else if (blue == max)
        h = 4 + (red - green) / (float) delta;
      h *= 42.5;

      if (h < 0)
        h += 255;
      if (h > 255)
        h -= 255;
    }

  *r = h;
  *g = s;
  *b = v;
}


void
fp_hsv_to_rgb (float *h,
            float *s,
            float *v)
{
  float hue, saturation, value;
  float f, p, q, t;

  if (((int)*s) == 0)
    {
      *h = *v;
      *s = *v;
      *v = *v;
    }
  else
    {
      hue = *h * 6.0 / 255.0;
      saturation = *s / 255.0;
      value = *v / 255.0;

      f = hue - (int) hue;
      p = value * (1.0 - saturation);
      q = value * (1.0 - (saturation * f));
      t = value * (1.0 - (saturation * (1.0 - f)));

      switch ((int) hue)
        {
        case 0:
          *h = value * 255.0;
          *s = t * 255.0;
          *v = p * 255.0;
          break;
        case 1:
          *h = q * 255.0;
          *s = value * 255.0;
          *v = p * 255.0;
          break;
        case 2:
          *h = p * 255.0;
          *s = value * 255.0;
          *v = t * 255.0;
          break;
        case 3:
          *h = p * 255.0;
          *s = q * 255.0;
          *v = value * 255.0;
          break;
        case 4:
          *h = t * 255.0;
          *s = p * 255.0;
          *v = value * 255.0;
          break;
        case 5:
          *h = value * 255.0;
          *s = p * 255.0;
          *v = q * 255.0;
          break;
        }
    }
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
  gint32 image_ID;
  GStatusType status = STATUS_SUCCESS;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;


  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);
  image_ID = param[1].data.d_image;

  if (status == STATUS_SUCCESS)
    {
      /*  Make sure that the drawable is indexed or RGB color  */
      if (gimp_drawable_color (drawable->id))
	{
	  gimp_progress_init ("Value Invert...");
	  gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width ()
				       + 1));
	  vinvert (drawable);
	  gimp_displays_flush ();
	}
      else
	if (gimp_drawable_indexed (drawable->id))
	  {
	    indexed_vinvert (image_ID);

	    /* GIMP doesn't implicitly update an image whose cmap has
	     changed - it probably should. */

	    gimp_drawable_update (drawable->id, 0, 0,
				  gimp_drawable_width(drawable->id),
				  gimp_drawable_height(drawable->id));
	    gimp_displays_flush ();
	  }
	else
	  {
	    status = STATUS_EXECUTION_ERROR;
	  }
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}


static void
indexed_vinvert(gint32 image_ID)
{
  guchar *cmap;
  gint ncols;

  cmap = gimp_image_get_cmap (image_ID, &ncols);

  if (cmap==NULL)
    {
      printf("vinvert: cmap was NULL!  Quitting...\n");
      gimp_quit();
    }

  vinvert_render_row(
		     cmap,
		     cmap,
		     0,
		     ncols,
		     3
		     );

  gimp_image_set_cmap (image_ID, cmap, ncols);
}



static void
vinvert_render_row (const guchar *src_row,
		  guchar *dest_row,
		  gint row,
		  gint row_width,
		  gint bytes)
{
  gint col, bytenum;

  for (col = 0; col < row_width ; col++)
    {
      float v1, v2, v3;

      v1 = (float)src_row[col*bytes];
      v2 = (float)src_row[col*bytes +1];
      v3 = (float)src_row[col*bytes +2];

      fp_rgb_to_hsv(&v1, &v2, &v3);
      v3 = 255.0-v3;
      fp_hsv_to_rgb(&v1, &v2, &v3);

      dest_row[col*bytes] = (int)v1;
      dest_row[col*bytes +1] = (int)v2;
      dest_row[col*bytes +2] = (int)v3;

      if (bytes>3)
	for (bytenum = 3; bytenum<bytes; bytenum++)
	  {
	    dest_row[col*bytes+bytenum] = src_row[col*bytes+bytenum];
	  }
    }
}





static void
vinvert (GDrawable *drawable)
{
  GPixelRgn srcPR, destPR;
  gint width, height;
  gint bytes;
  guchar *src_row;
  guchar *dest_row;
  gint row;
  gint x1, y1, x2, y2;


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

  /*  allocate row buffers  */
  src_row = (guchar *) malloc ((x2 - x1) * bytes);
  dest_row = (guchar *) malloc ((x2 - x1) * bytes);


  /*  initialize the pixel regions  */
  gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, drawable, 0, 0, width, height, TRUE, TRUE);


  for (row = y1; row < y2; row++)
    {
      gimp_pixel_rgn_get_row (&srcPR, src_row, x1, row, (x2 - x1));

      vinvert_render_row (src_row,
			dest_row,
			row,
			(x2 - x1),
			bytes
			);

      /*  store the dest  */
      gimp_pixel_rgn_set_row (&destPR, dest_row, x1, row, (x2 - x1));

      if ((row % 10) == 0)
	gimp_progress_update ((double) row / (double) (y2 - y1));
    }

  /*  update the processed region  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));

  free (src_row);
  free (dest_row);
}
