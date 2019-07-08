/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include "gimpchannel.h"
#include "gimpdrawable.h"
#include "gimpimage.h"
#include "gimpimage-pick-color.h"
#include "gimplayer.h"
#include "gimppickable.h"


gboolean
gimp_image_pick_color (GimpImage     *image,
                       GimpDrawable  *drawable,
                       gint           x,
                       gint           y,
                       gboolean       sample_merged,
                       gboolean       sample_average,
                       gdouble        average_radius,
                       const Babl   **sample_format,
                       gpointer       pixel,
                       GimpRGB       *color)
{
  GimpPickable *pickable;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (drawable == NULL || GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (drawable == NULL ||
                        gimp_item_get_image (GIMP_ITEM (drawable)) == image,
                        FALSE);

  if (sample_merged && drawable)
    {
      if ((GIMP_IS_LAYER (drawable) &&
           gimp_image_get_n_layers (image) == 1) ||
          (GIMP_IS_CHANNEL (drawable) &&
           gimp_image_get_n_channels (image) == 1))
        {
          /* Let's add a special exception when an image has only one
           * layer. This was useful in particular for indexed image as
           * it allows to pick the right index value even when "Sample
           * merged" is checked. There are more possible exceptions, but
           * we can't just take them all in considerations unless we
           * want to make code extra-complicated).
           * See #3041.
           */
          sample_merged = FALSE;
        }
    }

  if (! sample_merged)
    {
      if (! drawable)
        drawable = gimp_image_get_active_drawable (image);

      if (! drawable)
        return FALSE;
    }

  if (sample_merged)
    {
      pickable = GIMP_PICKABLE (image);
    }
  else
    {
      gint off_x, off_y;

      gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);
      x -= off_x;
      y -= off_y;

      pickable = GIMP_PICKABLE (drawable);
    }

  /* Do *not* call gimp_pickable_flush() here because it's too expensive
   * to call it unconditionally each time e.g. the cursor view is updated.
   * Instead, call gimp_pickable_flush() in the callers if needed.
   */

  if (sample_format)
    *sample_format = gimp_pickable_get_format (pickable);

  return gimp_pickable_pick_color (pickable, x, y,
                                   sample_average, average_radius,
                                   pixel, color);
}
