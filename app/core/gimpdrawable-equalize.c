/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>

#include "core-types.h"

#include "base/gimphistogram.h"
#include "base/gimplut.h"
#include "base/lut-funcs.h"

#include "gimpdrawable.h"
#include "gimpdrawable-equalize.h"
#include "gimpdrawable-histogram.h"
#include "gimpdrawable-process.h"

#include "gimp-intl.h"


void
gimp_drawable_equalize (GimpDrawable *drawable,
                        gboolean      mask_only)
{
  GimpHistogram *hist;
  GimpLut       *lut;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));

  hist = gimp_histogram_new ();
  gimp_drawable_calculate_histogram (drawable, hist);

  lut = equalize_lut_new (hist, gimp_drawable_bytes (drawable));
  gimp_histogram_unref (hist);

  gimp_drawable_process_lut (drawable, NULL, C_("undo-type", "Equalize"), lut);
  gimp_lut_free (lut);
}
