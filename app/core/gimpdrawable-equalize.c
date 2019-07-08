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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "gimpdrawable.h"
#include "gimpdrawable-equalize.h"
#include "gimpdrawable-histogram.h"
#include "gimpdrawable-operation.h"
#include "gimphistogram.h"
#include "gimpimage.h"
#include "gimpselection.h"

#include "gimp-intl.h"


void
gimp_drawable_equalize (GimpDrawable *drawable,
                        gboolean      mask_only)
{
  GimpImage     *image;
  GimpChannel   *selection;
  GimpHistogram *histogram;
  GeglNode      *equalize;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));

  image = gimp_item_get_image (GIMP_ITEM (drawable));
  selection = gimp_image_get_mask (image);

  histogram = gimp_histogram_new (FALSE);
  gimp_drawable_calculate_histogram (drawable, histogram, FALSE);

  equalize = gegl_node_new_child (NULL,
                                  "operation", "gimp:equalize",
                                  "histogram", histogram,
                                  NULL);

  if (! mask_only)
    gimp_selection_suspend (GIMP_SELECTION (selection));

  gimp_drawable_apply_operation (drawable, NULL,
                                 C_("undo-type", "Equalize"),
                                 equalize);

  if (! mask_only)
    gimp_selection_resume (GIMP_SELECTION (selection));

  g_object_unref (equalize);
  g_object_unref (histogram);
}
