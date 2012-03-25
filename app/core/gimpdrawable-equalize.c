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

#include "gimpdrawable.h"
#include "gimpdrawable-equalize.h"
#include "gimpdrawable-histogram.h"
#include "gimpdrawable-operation.h"
#include "gimphistogram.h"

#include "gimp-intl.h"


void
gimp_drawable_equalize (GimpDrawable *drawable,
                        gboolean      mask_only)
{
  GimpHistogram *hist;
  GeglNode      *equalize;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));

  hist = gimp_histogram_new ();
  gimp_drawable_calculate_histogram (drawable, hist);

  equalize = gegl_node_new_child (NULL,
                                  "operation", "gimp:equalize",
                                  "histogram", hist,
                                  NULL);

  gimp_drawable_apply_operation (drawable, NULL,
                                 C_("undo-type", "Equalize"),
                                 equalize);

  g_object_unref (equalize);

  gimp_histogram_unref (hist);
}
