/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include "gimpdrawable.h"
#include "gimpimage.h"
#include "gimpimage-pick-color.h"
#include "gimppickable.h"


gboolean
gimp_image_pick_color (GimpImage     *image,
                       GimpDrawable  *drawable,
                       gint           x,
                       gint           y,
                       gboolean       sample_merged,
                       gboolean       sample_average,
                       gdouble        average_radius,
                       GimpImageType *sample_type,
                       GimpRGB       *color,
                       gint          *color_index)
{
  GimpPickable *pickable;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (drawable == NULL || GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (drawable == NULL ||
                        gimp_item_get_image (GIMP_ITEM (drawable)) == image,
                        FALSE);

  if (! sample_merged)
    {
      if (! drawable)
        drawable = gimp_image_get_active_drawable (image);

      if (! drawable)
        return FALSE;
    }

  if (sample_merged)
    {
      pickable = GIMP_PICKABLE (image->projection);
    }
  else
    {
      gint off_x, off_y;

      gimp_item_offsets (GIMP_ITEM (drawable), &off_x, &off_y);
      x -= off_x;
      y -= off_y;

      pickable = GIMP_PICKABLE (drawable);
    }

  /* Do *not* call gimp_pickable_flush() here because it's too expensive
   * to call it unconditionally each time e.g. the cursor view is updated.
   * Instead, call gimp_pickable_flush() in the callers if needed.
   */

  if (sample_type)
    *sample_type = gimp_pickable_get_image_type (pickable);

  return gimp_pickable_pick_color (pickable, x, y,
                                   sample_average, average_radius,
                                   color, color_index);
}
