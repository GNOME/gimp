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
#include "gimplut.h"
#include "gimphistogram.h"

#include "libgimp/gimpintl.h"

struct hist_lut_struct
{
  GimpHistogram *histogram;
  int part[5][257];
};

static float      equalize_lut_func(struct hist_lut_struct *hlut, 
				    int nchannels, int channel, float value);
static void       equalize (GImage *, GimpDrawable *, int);
static Argument * equalize_invoker (Argument *);
static GimpLut  * eq_histogram_lut (GimpHistogram *hist, int bytes);


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
  PixelRegion srcPR, destPR;
  unsigned char *mask;
  int has_alpha;
  int alpha, bytes;
  int x1, y1, x2, y2;
  GimpHistogram *hist;
  GimpLut *lut;

  mask = NULL;

  bytes = drawable_bytes (drawable);
  has_alpha = drawable_has_alpha (drawable);
  alpha = has_alpha ? (bytes - 1) : bytes;

  hist = gimp_histogram_new();
  gimp_histogram_calculate_drawable(hist, drawable);


  /* Build equalization LUT */
  lut = eq_histogram_lut (hist, bytes);

  /*  Apply the histogram  */
  drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

  pixel_region_init (&srcPR, drawable_data (drawable), x1, y1, (x2 - x1), (y2 - y1), FALSE);
  pixel_region_init (&destPR, drawable_shadow (drawable), x1, y1, (x2 - x1), (y2 - y1), TRUE);

  pixel_regions_process_parallel((p_func)gimp_lut_process, lut, 
				 2, &srcPR, &destPR);

  gimp_lut_free(lut);
  gimp_histogram_free(hist);

  drawable_merge_shadow (drawable, TRUE);
  drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));
}


/*****/

static GimpLut *
eq_histogram_lut (GimpHistogram *hist, int bytes)
{
  int    i, k, j;
  struct hist_lut_struct hlut;
  double pixels_per_value;
  double desired;
  GimpLut *lut;
  double sum, dif;

  /* Find partition points */
  pixels_per_value = gimp_histogram_get_count(hist, 0, 255) / 256.0;

  for (k = 0; k < bytes; k++)
    {
      /* First and last points in partition */
      hlut.part[k][0]   = 0;
      hlut.part[k][256] = 256;
      
      /* Find intermediate points */
      j = 0;
      sum = gimp_histogram_get_channel(hist, k, 0) + 
	    gimp_histogram_get_channel(hist, k, 1);
      for (i = 1; i < 256; i++)
	{
	  desired = i * pixels_per_value;
	  while (sum <= desired)
	  {
	    j++;
	    sum += gimp_histogram_get_channel(hist, k, j + 1);
	  }

	  /* Nearest sum */
	  dif = sum - gimp_histogram_get_channel(hist, k, j);
	  if ((sum - desired) > (dif / 2.0))
	    hlut.part[k][i] = j;
	  else
	    hlut.part[k][i] = j + 1;
	}
    }

  lut = gimp_lut_new();

  gimp_lut_setup(lut, (GimpLutFunc) equalize_lut_func,
		 (void *) &hlut, bytes);

  return lut;
}

static float
equalize_lut_func(struct hist_lut_struct *hlut, 
		  int nchannels, int channel, float value)
{
  int i = 0, j;
  j = (int)(value * 255.0 + 0.5);
  while (hlut->part[channel][i + 1] <= j)
    i++;
  return i / 255.0;
}


/*  The equalize procedure definition  */
ProcArg equalize_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_INT32,
    "mask_only",
    "equalization option"
  }
};

ProcRecord equalize_proc =
{
  "gimp_equalize",
  "Equalize the contents of the specified drawable",
  "This procedure equalizes the contents of the specified drawable.  Each intensity channel is equalizeed independently.  The equalizeed intensity is given as inten' = (255 - inten).  Indexed color drawables are not valid for this operation.  The 'mask_only' option specifies whether to adjust only the area of the image within the selection bounds, or the entire image based on the histogram of the selected area.  If there is no selection, the entire image is adjusted based on the histogram for the entire image.",
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
