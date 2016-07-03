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

#include <string.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gegl/gimp-gegl-utils.h"

#include "gimp.h"
#include "gimpcontext.h"
#include "gimpdrawable.h"
#include "gimpdrawable-offset.h"
#include "gimpimage.h"
#include "gimppickable.h"

#include "gimp-intl.h"


void
gimp_drawable_offset (GimpDrawable   *drawable,
                      GimpContext    *context,
                      gboolean        wrap_around,
                      GimpOffsetType  fill_type,
                      gint            offset_x,
                      gint            offset_y)
{
  GimpItem      *item;
  GeglBuffer    *src_buffer;
  GeglBuffer    *new_buffer;
  GeglRectangle  src_rect;
  GeglRectangle  dest_rect;
  gint           width, height;
  gint           src_x, src_y;
  gint           dest_x, dest_y;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  item = GIMP_ITEM (drawable);

  width  = gimp_item_get_width  (item);
  height = gimp_item_get_height (item);

  if (wrap_around)
    {
      /*  avoid modulo operation on negative values  */
      while (offset_x < 0)
        offset_x += width;
      while (offset_y < 0)
        offset_y += height;

      offset_x %= width;
      offset_y %= height;
    }
  else
    {
      offset_x = CLAMP (offset_x, -width, width);
      offset_y = CLAMP (offset_y, -height, height);
    }

  if (offset_x == 0 && offset_y == 0)
    return;

  src_buffer = gimp_drawable_get_buffer (drawable);

  new_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height),
                                gimp_drawable_get_format (drawable));

  if (! wrap_around)
    {
      if (fill_type == GIMP_OFFSET_BACKGROUND)
        {
          GimpRGB    bg;
          GeglColor *color;

          gimp_context_get_background (context, &bg);
          gimp_pickable_srgb_to_image_color (GIMP_PICKABLE (drawable),
                                             &bg, &bg);

          color = gimp_gegl_color_new (&bg);
          gegl_buffer_set_color (new_buffer, NULL, color);
          g_object_unref (color);
        }
    }

  if (offset_x >= 0)
    {
      src_x = 0;
      dest_x = offset_x;
      width = CLAMP ((width - offset_x), 0, width);
    }
  else
    {
      src_x = -offset_x;
      dest_x = 0;
      width = CLAMP ((width + offset_x), 0, width);
    }

  if (offset_y >= 0)
    {
      src_y = 0;
      dest_y = offset_y;
      height = CLAMP ((height - offset_y), 0, height);
    }
  else
    {
      src_y = -offset_y;
      dest_y = 0;
      height = CLAMP ((height + offset_y), 0, height);
    }

  /*  Copy the center region  */
  if (width && height)
    {
      gegl_buffer_copy (src_buffer,
                        GEGL_RECTANGLE (src_x,  src_y,  width, height),
                        GEGL_ABYSS_NONE,
                        new_buffer,
                        GEGL_RECTANGLE (dest_x,dest_y,  width, height));
    }

  if (wrap_around)
    {
      /*  Copy appropriately for wrap around  */

      if (offset_x >= 0 && offset_y >= 0)
        {
          src_x = gimp_item_get_width  (item) - offset_x;
          src_y = gimp_item_get_height (item) - offset_y;
        }
      else if (offset_x >= 0 && offset_y < 0)
        {
          src_x = gimp_item_get_width (item) - offset_x;
          src_y = 0;
        }
      else if (offset_x < 0 && offset_y >= 0)
        {
          src_x = 0;
          src_y = gimp_item_get_height (item) - offset_y;
        }
      else if (offset_x < 0 && offset_y < 0)
        {
          src_x = 0;
          src_y = 0;
        }

      dest_x = (src_x + offset_x) % gimp_item_get_width (item);
      if (dest_x < 0)
        dest_x = gimp_item_get_width (item) + dest_x;

      dest_y = (src_y + offset_y) % gimp_item_get_height (item);
      if (dest_y < 0)
        dest_y = gimp_item_get_height (item) + dest_y;

      /*  intersecting region  */
      if (offset_x != 0 && offset_y != 0)
        {
          gegl_buffer_copy (src_buffer,
                            GEGL_RECTANGLE (src_x, src_y,
                                            ABS (offset_x), ABS (offset_y)),
                            GEGL_ABYSS_NONE,
                            new_buffer,
                            GEGL_RECTANGLE (dest_x, dest_y, 0, 0));
        }

      /*  X offset  */
      if (offset_x != 0)
        {
          if (offset_y >= 0)
            {
              src_rect.x      = src_x;
              src_rect.y      = 0;
              src_rect.width  = ABS (offset_x);
              src_rect.height = gimp_item_get_height (item) - ABS (offset_y);

              dest_rect.x = dest_x;
              dest_rect.y = dest_y + offset_y;
            }
          else if (offset_y < 0)
            {
              src_rect.x      = src_x;
              src_rect.y      = src_y - offset_y;
              src_rect.width  = ABS (offset_x);
              src_rect.height = gimp_item_get_height (item) - ABS (offset_y);

              dest_rect.x = dest_x;
              dest_rect.y = 0;
            }

          gegl_buffer_copy (src_buffer, &src_rect, GEGL_ABYSS_NONE,
                            new_buffer, &dest_rect);
        }

      /*  X offset  */
      if (offset_y != 0)
        {
          if (offset_x >= 0)
            {
              src_rect.x      = 0;
              src_rect.y      = src_y;
              src_rect.width  = gimp_item_get_width (item) - ABS (offset_x);
              src_rect.height = ABS (offset_y);

              dest_rect.x = dest_x + offset_x;
              dest_rect.y = dest_y;
            }
          else if (offset_x < 0)
            {
              src_rect.x      = src_x - offset_x;
              src_rect.y      = src_y;
              src_rect.width  = gimp_item_get_width (item) - ABS (offset_x);
              src_rect.height = ABS (offset_y);

              dest_rect.x = 0;
              dest_rect.y = dest_y;
            }

          gegl_buffer_copy (src_buffer, &src_rect, GEGL_ABYSS_NONE,
                            new_buffer, &dest_rect);
        }
    }

  gimp_drawable_set_buffer (drawable,
                            gimp_item_is_attached (item),
                            C_("undo-type", "Offset Drawable"),
                            new_buffer);
  g_object_unref (new_buffer);
}
