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

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gegl/gimp-gegl-mask-combine.h"
#include "gegl/gimp-gegl-utils.h"

#include "gimpchannel.h"
#include "gimpchannel-combine.h"


typedef struct
{
  GeglRectangle rect;

  gboolean      bounds_known;
  gboolean      empty;
  GeglRectangle bounds;
} GimpChannelCombineData;


/*  local function prototypes  */

static void       gimp_channel_combine_clear            (GimpChannel            *mask,
                                                         const GeglRectangle    *rect);
static void       gimp_channel_combine_clear_complement (GimpChannel            *mask,
                                                         const GeglRectangle    *rect);

static gboolean   gimp_channel_combine_start            (GimpChannel            *mask,
                                                         GimpChannelOps          op,
                                                         const GeglRectangle    *rect,
                                                         gboolean                full_extent,
                                                         gboolean                full_value,
                                                         GimpChannelCombineData *data);
static void       gimp_channel_combine_end              (GimpChannel            *mask,
                                                         GimpChannelCombineData *data);


/*  private functions  */

static void
gimp_channel_combine_clear (GimpChannel         *mask,
                            const GeglRectangle *rect)
{
  GeglBuffer    *buffer;
  GeglRectangle  area;
  GeglRectangle  update_area;

  if (mask->bounds_known && mask->empty)
    return;

  buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (mask));

  if (rect)
    {
      if (rect->width <= 0 || rect->height <= 0)
        return;

      if (mask->bounds_known)
        {
          if (! gegl_rectangle_intersect (&area,
                                          GEGL_RECTANGLE (mask->x1,
                                                          mask->y1,
                                                          mask->x2 - mask->x1,
                                                          mask->y2 - mask->y1),
                                          rect))
            {
              return;
            }
        }
      else
        {
          area = *rect;
        }

      update_area = area;
    }
  else
    {
      if (mask->bounds_known)
        {
          area.x      = mask->x1;
          area.y      = mask->y1;
          area.width  = mask->x2 - mask->x1;
          area.height = mask->y2 - mask->y1;
        }
      else
        {
          area.x      = 0;
          area.y      = 0;
          area.width  = gimp_item_get_width  (GIMP_ITEM (mask));
          area.height = gimp_item_get_height (GIMP_ITEM (mask));
        }

      update_area = area;

      gimp_gegl_rectangle_align_to_tile_grid (&area, &area, buffer);
    }

  gegl_buffer_clear (buffer, &area);

  gimp_drawable_update (GIMP_DRAWABLE (mask),
                        update_area.x, update_area.y,
                        update_area.width, update_area.height);
}

static void
gimp_channel_combine_clear_complement (GimpChannel         *mask,
                                       const GeglRectangle *rect)
{
  gint width  = gimp_item_get_width  (GIMP_ITEM (mask));
  gint height = gimp_item_get_height (GIMP_ITEM (mask));

  gimp_channel_combine_clear (
    mask,
    GEGL_RECTANGLE (0,
                    0,
                    width,
                    rect->y));

  gimp_channel_combine_clear (
    mask,
    GEGL_RECTANGLE (0,
                    rect->y + rect->height,
                    width,
                    height - (rect->y + rect->height)));

  gimp_channel_combine_clear (
    mask,
    GEGL_RECTANGLE (0,
                    rect->y,
                    rect->x,
                    rect->height));

  gimp_channel_combine_clear (
    mask,
    GEGL_RECTANGLE (rect->x + rect->width,
                    rect->y,
                    width - (rect->x + rect->width),
                    rect->height));
}

static gboolean
gimp_channel_combine_start (GimpChannel            *mask,
                            GimpChannelOps          op,
                            const GeglRectangle    *rect,
                            gboolean                full_extent,
                            gboolean                full_value,
                            GimpChannelCombineData *data)
{
  GeglBuffer    *buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (mask));
  GeglRectangle  extent;
  gboolean       intersects;

  extent.x      = 0;
  extent.y      = 0;
  extent.width  = gimp_item_get_width  (GIMP_ITEM (mask));
  extent.height = gimp_item_get_height (GIMP_ITEM (mask));

  intersects = gegl_rectangle_intersect (&data->rect, rect, &extent);

  data->bounds_known  = mask->bounds_known;
  data->empty         = mask->empty;

  data->bounds.x      = mask->x1;
  data->bounds.y      = mask->y1;
  data->bounds.width  = mask->x2 - mask->x1;
  data->bounds.height = mask->y2 - mask->y1;

  gegl_buffer_freeze_changed (buffer);

  /*  Determine new boundary  */
  switch (op)
    {
    case GIMP_CHANNEL_OP_REPLACE:
      gimp_channel_combine_clear (mask, NULL);

      if (! intersects)
        {
          data->bounds_known = TRUE;
          data->empty        = TRUE;

          return FALSE;
        }

      data->bounds_known = FALSE;

      if (full_extent)
        {
          data->bounds_known = TRUE;
          data->empty        = FALSE;
          data->bounds       = data->rect;
        }
      break;

    case GIMP_CHANNEL_OP_ADD:
      if (! intersects)
        return FALSE;

      data->bounds_known = FALSE;

      if (full_extent && (mask->bounds_known ||
                          gegl_rectangle_equal (&data->rect, &extent)))
        {
          data->bounds_known = TRUE;
          data->empty        = FALSE;

          if (mask->bounds_known && ! mask->empty)
            {
              gegl_rectangle_bounding_box (&data->bounds,
                                           &data->bounds, &data->rect);
            }
          else
            {
              data->bounds = data->rect;
            }
        }
      break;

    case GIMP_CHANNEL_OP_SUBTRACT:
      if (intersects && mask->bounds_known)
        {
          if (mask->empty)
            {
              intersects = FALSE;
            }
          else
            {
              intersects = gegl_rectangle_intersect (&data->rect,
                                                     &data->rect,
                                                     &data->bounds);
            }
        }

      if (! intersects)
        return FALSE;

      if (full_value &&
          gegl_rectangle_contains (&data->rect,
                                   mask->bounds_known ? &data->bounds :
                                                        &extent))
        {
          gimp_channel_combine_clear (mask, NULL);

          data->bounds_known = TRUE;
          data->empty        = TRUE;

          return FALSE;
        }

      data->bounds_known = FALSE;

      gegl_buffer_set_abyss (buffer, &data->rect);
      break;

    case GIMP_CHANNEL_OP_INTERSECT:
      if (intersects && mask->bounds_known)
        {
          if (mask->empty)
            {
              intersects = FALSE;
            }
          else
            {
              intersects = gegl_rectangle_intersect (&data->rect,
                                                     &data->rect,
                                                     &data->bounds);
            }
        }

      if (! intersects)
        {
          gimp_channel_combine_clear (mask, NULL);

          data->bounds_known = TRUE;
          data->empty        = TRUE;

          return FALSE;
        }

      if (full_value && mask->bounds_known &&
          gegl_rectangle_contains (&data->rect, &data->bounds))
        {
          return FALSE;
        }

      data->bounds_known = FALSE;

      gimp_channel_combine_clear_complement (mask, &data->rect);

      gegl_buffer_set_abyss (buffer, &data->rect);
      break;
    }

  return TRUE;
}

static void
gimp_channel_combine_end (GimpChannel            *mask,
                          GimpChannelCombineData *data)
{
  GeglBuffer *buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (mask));

  gegl_buffer_set_abyss (buffer, gegl_buffer_get_extent (buffer));

  gegl_buffer_thaw_changed (buffer);

  mask->bounds_known = data->bounds_known;

  if (data->bounds_known)
    {
      mask->empty = data->empty;

      if (data->empty)
        {
          mask->x1 = 0;
          mask->y1 = 0;
          mask->x2 = gimp_item_get_width  (GIMP_ITEM (mask));
          mask->y2 = gimp_item_get_height (GIMP_ITEM (mask));
        }
      else
        {
          mask->x1 = data->bounds.x;
          mask->y1 = data->bounds.y;
          mask->x2 = data->bounds.x + data->bounds.width;
          mask->y2 = data->bounds.y + data->bounds.height;
        }
    }

  gimp_drawable_update (GIMP_DRAWABLE (mask),
                        data->rect.x, data->rect.y,
                        data->rect.width, data->rect.height);
}


/*  public functions  */

void
gimp_channel_combine_rect (GimpChannel    *mask,
                           GimpChannelOps  op,
                           gint            x,
                           gint            y,
                           gint            w,
                           gint            h)
{
  GimpChannelCombineData data;

  g_return_if_fail (GIMP_IS_CHANNEL (mask));

  if (gimp_channel_combine_start (mask, op, GEGL_RECTANGLE (x, y, w, h),
                                  TRUE, TRUE, &data))
    {
      GeglBuffer *buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (mask));

      gimp_gegl_mask_combine_rect (buffer, op, x, y, w, h);
    }

  gimp_channel_combine_end (mask, &data);
}

void
gimp_channel_combine_ellipse (GimpChannel    *mask,
                              GimpChannelOps  op,
                              gint            x,
                              gint            y,
                              gint            w,
                              gint            h,
                              gboolean        antialias)
{
  gimp_channel_combine_ellipse_rect (mask, op, x, y, w, h,
                                     w / 2.0, h / 2.0, antialias);
}

void
gimp_channel_combine_ellipse_rect (GimpChannel    *mask,
                                   GimpChannelOps  op,
                                   gint            x,
                                   gint            y,
                                   gint            w,
                                   gint            h,
                                   gdouble         rx,
                                   gdouble         ry,
                                   gboolean        antialias)
{
  GimpChannelCombineData data;

  g_return_if_fail (GIMP_IS_CHANNEL (mask));

  if (gimp_channel_combine_start (mask, op, GEGL_RECTANGLE (x, y, w, h),
                                  TRUE, FALSE, &data))
    {
      GeglBuffer *buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (mask));

      gimp_gegl_mask_combine_ellipse_rect (buffer, op, x, y, w, h,
                                           rx, ry, antialias);
    }

  gimp_channel_combine_end (mask, &data);
}

void
gimp_channel_combine_mask (GimpChannel    *mask,
                           GimpChannel    *add_on,
                           GimpChannelOps  op,
                           gint            off_x,
                           gint            off_y)
{
  GeglBuffer *add_on_buffer;

  g_return_if_fail (GIMP_IS_CHANNEL (mask));
  g_return_if_fail (GIMP_IS_CHANNEL (add_on));

  add_on_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (add_on));

  gimp_channel_combine_buffer (mask, add_on_buffer,
                               op, off_x, off_y);
}

void
gimp_channel_combine_buffer (GimpChannel    *mask,
                             GeglBuffer     *add_on_buffer,
                             GimpChannelOps  op,
                             gint            off_x,
                             gint            off_y)
{
  GimpChannelCombineData data;

  g_return_if_fail (GIMP_IS_CHANNEL (mask));
  g_return_if_fail (GEGL_IS_BUFFER (add_on_buffer));

  if (gimp_channel_combine_start (mask, op,
                                  GEGL_RECTANGLE (
                                    off_x,
                                    off_y,
                                    gegl_buffer_get_width  (add_on_buffer),
                                    gegl_buffer_get_height (add_on_buffer)),
                                  FALSE, FALSE, &data))
    {
      GeglBuffer *buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (mask));

      gimp_gegl_mask_combine_buffer (buffer, add_on_buffer, op,
                                     off_x, off_y);
    }

  gimp_channel_combine_end (mask, &data);
}
