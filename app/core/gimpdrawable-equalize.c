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

#include "config.h"

#include <gtk/gtk.h>

#include "apptypes.h"

#include "drawable.h"
#include "equalize.h"
#include "gimpimage.h"
#include "gimplut.h"
#include "lut_funcs.h"
#include "gimphistogram.h"
#include "pixel_processor.h"
#include "pixel_region.h"

#include "libgimp/gimpintl.h"


void
image_equalize (GimpImage *gimage)
{
  GimpDrawable *drawable;

  drawable = gimp_image_active_drawable (gimage);

  if (gimp_drawable_is_indexed (drawable))
    {
      g_message (_("Equalize does not operate on indexed drawables."));
      return;
    }

  equalize (gimage, drawable, TRUE);
}


void
equalize (GimpImage    *gimage,
    	  GimpDrawable *drawable,
	  gboolean      mask_only)
{
  PixelRegion    srcPR, destPR;
  guchar        *mask;
  gint           has_alpha;
  gint           alpha, bytes;
  gint           x1, y1, x2, y2;
  GimpHistogram *hist;
  GimpLut       *lut;

  mask = NULL;

  bytes     = gimp_drawable_bytes (drawable);
  has_alpha = gimp_drawable_has_alpha (drawable);
  alpha     = has_alpha ? (bytes - 1) : bytes;

  hist = gimp_histogram_new ();
  gimp_histogram_calculate_drawable (hist, drawable);


  /* Build equalization LUT */
  lut = eq_histogram_lut_new (hist, bytes);

  /*  Apply the histogram  */
  gimp_drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

  pixel_region_init (&srcPR, gimp_drawable_data (drawable),
		     x1, y1, (x2 - x1), (y2 - y1), FALSE);
  pixel_region_init (&destPR, gimp_drawable_shadow (drawable),
		     x1, y1, (x2 - x1), (y2 - y1), TRUE);

  pixel_regions_process_parallel ((p_func) gimp_lut_process, lut, 
				  2, &srcPR, &destPR);

  gimp_lut_free (lut);
  gimp_histogram_free (hist);

  gimp_drawable_merge_shadow (drawable, TRUE);
  drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));
}
