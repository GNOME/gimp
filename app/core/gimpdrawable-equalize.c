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

#include <glib-object.h>

#include "core-types.h"

#include "base/gimphistogram.h"
#include "base/gimplut.h"
#include "base/lut-funcs.h"
#include "base/pixel-processor.h"
#include "base/pixel-region.h"

#include "config/gimpbaseconfig.h"

#include "gimp.h"
#include "gimpdrawable.h"
#include "gimpdrawable-equalize.h"
#include "gimpdrawable-histogram.h"
#include "gimpimage.h"

#include "gimp-intl.h"


void
gimp_drawable_equalize (GimpDrawable *drawable,
                        gboolean      mask_only)
{
  PixelRegion    srcPR, destPR;
  guchar        *mask;
  gint           bytes;
  gint           x1, y1, x2, y2;
  GimpHistogram *hist;
  GimpLut       *lut;
  GimpImage     *gimage;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  mask = NULL;

  bytes     = gimp_drawable_bytes (drawable);

  gimage = gimp_item_get_image (GIMP_ITEM (drawable));

  hist = gimp_histogram_new (GIMP_BASE_CONFIG (gimage->gimp->config));
  gimp_drawable_calculate_histogram (drawable, hist);

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

  gimp_drawable_merge_shadow (drawable, TRUE, _("Equalize"));

  gimp_drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));
}
