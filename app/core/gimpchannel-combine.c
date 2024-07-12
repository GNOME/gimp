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

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpchannel-combine.h"
#include "gimpimage.h"
#include "gimpimage-merge.h"
#include "gimpimage-new.h"
#include "gimplayer.h"

#include "vectors/gimppath.h"


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

      gegl_rectangle_align_to_buffer (&area, &area, buffer,
                                      GEGL_RECTANGLE_ALIGNMENT_SUPERSET);
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
                                    off_x + gegl_buffer_get_x (add_on_buffer),
                                    off_y + gegl_buffer_get_y (add_on_buffer),
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

/**
 * gimp_channel_combine_items:
 * @channel:
 * @items:
 * @op:
 *
 * Edit @channel with the given @items, according to the boolean @op.
 * @items need to belong to the same image, but it doesn't have to be
 * the owner image of @channel.
 * If @items contain a single layer, it will be used as-is, without
 * caring about opacity, mode or visibility.
 * If @items contain several layers, they will be used composited using
 * their opacity, mode, visibility, etc. properties within the image
 * (similar as if a "merge visible" had been applied to the image then
 * the resulting layer used alone).
 * If @items contain channels or paths, they will be added as a set
 * (i.e. as a single item which is an union of other items). E.g. an
 * combine in GIMP_CHANNEL_OP_INTERSECT mode does not intersect all
 * @items with each other and @channel. It first adds-alike all @items
 * together, then intersects the result with @channel.
 */
void
gimp_channel_combine_items (GimpChannel    *mask,
                            GList          *items,
                            GimpChannelOps  op)
{
  GimpImage   *image;
  GimpImage   *items_image = NULL;
  GimpImage   *temp_image  = NULL;

  GimpChannel *channel     = NULL;
  GList       *layers      = NULL;
  GList       *iter;

  g_return_if_fail (GIMP_IS_CHANNEL (mask));
  g_return_if_fail (g_list_length (items) > 0);

  for (iter = items; iter; iter = iter->next)
    {
      g_return_if_fail (GIMP_IS_LAYER (iter->data)   ||
                        GIMP_IS_CHANNEL (iter->data) ||
                        GIMP_IS_PATH (iter->data));

      if (items_image == NULL)
        items_image = gimp_item_get_image (iter->data);
      else
        g_return_if_fail (items_image == gimp_item_get_image (iter->data));

      if (GIMP_IS_LAYER (iter->data))
        layers = g_list_prepend (layers, iter->data);
    }

  image = gimp_item_get_image (GIMP_ITEM (mask));
  if (g_list_length (layers) > 1)
    {
      GList *merged_layers;

      temp_image = gimp_image_new_from_drawables (image->gimp, items, FALSE, FALSE);
      merged_layers = gimp_image_merge_visible_layers (temp_image,
                                                       gimp_get_user_context (temp_image->gimp),
                                                       GIMP_CLIP_TO_IMAGE,
                                                       FALSE, TRUE, NULL);
      g_return_if_fail (g_list_length (merged_layers) == 1);
      gimp_item_to_selection (GIMP_ITEM (merged_layers->data),
                              GIMP_CHANNEL_OP_REPLACE,
                              TRUE, FALSE, 0.0, 0.0);
      channel = gimp_image_get_mask (temp_image);
    }

  if (! channel)
    {
      channel = gimp_channel_new (image,
                                  gimp_image_get_width (image),
                                  gimp_image_get_height (image),
                                  NULL, NULL);
      gimp_channel_clear (channel, NULL, FALSE);

      if (g_list_length (layers) == 1)
        {
          if (gimp_drawable_has_alpha (layers->data))
            {
              GimpChannel *alpha;
              gint         offset_x;
              gint         offset_y;

              alpha = gimp_channel_new_from_alpha (image,
                                                   layers->data, NULL, NULL);
              gimp_item_get_offset (layers->data, &offset_x, &offset_y);
              gimp_channel_combine_mask (channel, alpha,
                                         GIMP_CHANNEL_OP_REPLACE,
                                         offset_x, offset_y);
              g_object_unref (alpha);
            }
          else
            {
              gimp_channel_all (channel, FALSE);
            }
        }
    }

  for (iter = items; iter; iter = iter->next)
    {
      if (! GIMP_IS_LAYER (iter->data))
        {
          gint offset_x;
          gint offset_y;

          gimp_item_get_offset (iter->data, &offset_x, &offset_y);
          gimp_channel_combine_buffer (channel,
                                       gimp_drawable_get_buffer (GIMP_DRAWABLE (iter->data)),
                                       GIMP_CHANNEL_OP_ADD, offset_x, offset_y);
        }
    }

  gimp_channel_combine_buffer (mask,
                               gimp_drawable_get_buffer (GIMP_DRAWABLE (channel)),
                               op, 0, 0);

  if (temp_image)
    g_object_unref (temp_image);
  else
    g_object_unref (channel);

  g_list_free (layers);
}
