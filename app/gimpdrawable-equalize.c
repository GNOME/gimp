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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "appenv.h"
#include "drawable.h"
#include "equalize.h"
#include "interface.h"
#include "gimage.h"

#include "libgimp/gimpintl.h"

static void       equalize (GImage *, GimpDrawable *, int);
static void       eq_histogram (double [3][256], unsigned char [3][256], int, double);
static Argument * equalize_invoker (Argument *);


void
image_equalize (gimage_ptr)
     void *gimage_ptr;
{
  GImage *gimage;
  GimpDrawable *drawable;
  int mask_only = TRUE;

  gimage = (GImage *) gimage_ptr;
  drawable = gimage_active_drawable (gimage);

  if (drawable_indexed (drawable))
    {
      g_message (_("Equalize does not operate on indexed drawables."));
      return;
    }
  equalize (gimage, drawable, mask_only);
}


static void
equalize(gimage, drawable, mask_only)
     GImage *gimage;
     GimpDrawable *drawable;
     int mask_only;
{
  Channel *sel_mask;
  PixelRegion srcPR, destPR, maskPR, *sel_maskPR;
  double hist[3][256];
  unsigned char lut[3][256];
  unsigned char *src, *s;
  unsigned char *dest, *d;
  unsigned char *mask, *m;
  int no_mask;
  int h, j, b;
  int has_alpha;
  int alpha, bytes;
  int off_x, off_y;
  int x1, y1, x2, y2;
  double count;
  void *pr;

  mask = NULL;

  sel_mask = gimage_get_mask (gimage);
  drawable_offsets (drawable, &off_x, &off_y);
  bytes = drawable_bytes (drawable);
  has_alpha = drawable_has_alpha (drawable);
  alpha = has_alpha ? (bytes - 1) : bytes;
  count = 0.0;

  /*  Determine the histogram from the drawable data and the attendant mask  */
  no_mask = (drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2) == FALSE);
  pixel_region_init (&srcPR, drawable_data (drawable), x1, y1, (x2 - x1), (y2 - y1), FALSE);
  sel_maskPR = (no_mask) ? NULL : &maskPR;
  if (sel_maskPR)
    pixel_region_init (sel_maskPR, drawable_data (GIMP_DRAWABLE (sel_mask)), x1 + off_x, y1 + off_y, (x2 - x1), (y2 - y1), FALSE);

  /* Initialize histogram */
  for (b = 0; b < alpha; b++)
    for (j = 0; j < 256; j++)
      hist[b][j] = 0.0;

  for (pr = pixel_regions_register (2, &srcPR, sel_maskPR); pr != NULL; pr = pixel_regions_process (pr))
    {
      src = srcPR.data;
      if (sel_maskPR)
	mask = sel_maskPR->data;
      h = srcPR.h;

      while (h--)
	{
	  s = src;
	  m = mask;

	  for (j = 0; j < srcPR.w; j++)
	    {
	      if (sel_maskPR)
		{
		  for (b = 0; b < alpha; b++)
		    hist[b][s[b]] += (double) *m / 255.0;
		  count += (double) *m / 255.0;
		}
	      else
		{
		  for (b = 0; b < alpha; b++)
		    hist[b][s[b]] += 1.0;
		  count += 1.0;
		}

	      s += bytes;

	      if (sel_maskPR)
		m ++;
	    }

	  src += srcPR.rowstride;

	  if (sel_maskPR)
	    mask += sel_maskPR->rowstride;
	}
    }

  /* Build equalization LUT */
  eq_histogram (hist, lut, alpha, count);

  /*  Apply the histogram  */
  pixel_region_init (&srcPR, drawable_data (drawable), x1, y1, (x2 - x1), (y2 - y1), FALSE);
  pixel_region_init (&destPR, drawable_shadow (drawable), x1, y1, (x2 - x1), (y2 - y1), TRUE);

  for (pr = pixel_regions_register (2, &srcPR, &destPR); pr != NULL; pr = pixel_regions_process (pr))
    {
      src = srcPR.data;
      dest = destPR.data;
      h = srcPR.h;

      while (h--)
	{
	  s = src;
	  d = dest;

	  for (j = 0; j < srcPR.w; j++)
	    {
	      for (b = 0; b < alpha; b++)
		d[b] = lut[b][s[b]];

	      if (has_alpha)
		d[alpha] = s[alpha];

	      s += bytes;
	      d += bytes;
	    }

	  src += srcPR.rowstride;
	  dest += destPR.rowstride;
	}
    }

  drawable_merge_shadow (drawable, TRUE);
  drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));
}


/*****/

static void
eq_histogram (hist, lut, bytes, count)
     double hist[3][256];
     unsigned char lut[3][256];
     int bytes;
     double count;
{
  int    i, k, j;
  int    part[3][257]; /* Partition */
  double pixels_per_value;
  double desired;

  /* Calculate partial sums */
  for (k = 0; k < bytes; k++)
    for (i = 1; i < 256; i++)
      hist[k][i] += hist[k][i - 1];

  /* Find partition points */
  pixels_per_value = count / 256.0;

  for (k = 0; k < bytes; k++)
    {
      /* First and last points in partition */
      part[k][0]   = 0;
      part[k][256] = 256;

      /* Find intermediate points */
      j = 0;
      for (i = 1; i < 256; i++)
	{
	  desired = i * pixels_per_value;
	  while (hist[k][j + 1] <= desired)
	    j++;

	  /* Nearest sum */
	  if ((desired - hist[k][j]) < (hist[k][j + 1] - desired))
	    part[k][i] = j;
	  else
	    part[k][i] = j + 1;
	}
    }

  /* Create equalization LUT */
  for (k = 0; k < bytes; k++)
    for (j = 0; j < 256; j++)
      {
	i = 0;
	while (part[k][i + 1] <= j)
	  i++;

	lut[k][j] = i;
      }
}


/*  The equalize procedure definition  */
ProcArg equalize_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    N_("the drawable")
  },
  { PDB_INT32,
    "mask_only",
    N_("equalization option")
  }
};

ProcRecord equalize_proc =
{
  "gimp_equalize",
  N_("Equalize the contents of the specified drawable"),
  N_("This procedure equalizes the contents of the specified drawable.  Each intensity channel is equalizeed independently.  The equalizeed intensity is given as inten' = (255 - inten).  Indexed color drawables are not valid for this operation.  The 'mask_only' option specifies whether to adjust only the area of the image within the selection bounds, or the entire image based on the histogram of the selected area.  If there is no selection, the entire image is adjusted based on the histogram for the entire image."),
  "Federico Mena Quintero & Spencer Kimball & Peter Mattis",
  "Federico Mena Quintero & Spencer Kimball & Peter Mattis",
  "1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  equalize_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { equalize_invoker } },
};


static Argument *
equalize_invoker (args)
     Argument *args;
{
  int success = TRUE;
  int int_value;
  int mask_only;
  GImage *gimage;
  GimpDrawable *drawable;

  drawable = NULL;

  /*  the drawable  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL)                                        
        success = FALSE;
      else
        gimage = drawable_gimage (drawable);
    }
  /*  the mask only option  */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      mask_only = (int_value) ? TRUE : FALSE;
    }
  /*  make sure the drawable is not indexed color  */
  if (success)
    success = ! drawable_indexed (drawable);

  if (success)
    equalize (gimage, drawable, mask_only);

  return procedural_db_return_args (&equalize_proc, success);
}
