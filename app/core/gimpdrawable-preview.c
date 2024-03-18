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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "config/gimpcoreconfig.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimp-gegl-loops.h"
#include "gegl/gimptilehandlervalidate.h"

#include "gimp.h"
#include "gimp-parallel.h"
#include "gimp-utils.h"
#include "gimpasync.h"
#include "gimpchannel.h"
#include "gimpchunkiterator.h"
#include "gimpimage.h"
#include "gimpimage-color-profile.h"
#include "gimpdrawable-preview.h"
#include "gimpdrawable-private.h"
#include "gimplayer.h"
#include "gimptempbuf.h"

#include "gimp-priorities.h"


typedef struct
{
  const Babl        *format;
  GeglBuffer        *buffer;
  GeglRectangle      rect;
  gdouble            scale;

  GimpChunkIterator *iter;
} SubPreviewData;


/*  local function prototypes  */

static SubPreviewData * sub_preview_data_new  (const Babl          *format,
                                               GeglBuffer          *buffer,
                                               const GeglRectangle *rect,
                                               gdouble              scale);
static void             sub_preview_data_free (SubPreviewData      *data);



/*  private functions  */


static SubPreviewData *
sub_preview_data_new (const Babl          *format,
                      GeglBuffer          *buffer,
                      const GeglRectangle *rect,
                      gdouble              scale)
{
  SubPreviewData *data = g_slice_new (SubPreviewData);

  data->format = format;
  /* We take ownership if the buffer reference. */
  data->buffer = buffer;
  data->rect   = *rect;
  data->scale  = scale;

  data->iter   = NULL;

  return data;
}

static void
sub_preview_data_free (SubPreviewData *data)
{
  g_object_unref (data->buffer);

  if (data->iter)
    gimp_chunk_iterator_stop (data->iter, TRUE);

  g_slice_free (SubPreviewData, data);
}


/*  public functions  */


GimpTempBuf *
gimp_drawable_get_new_preview (GimpViewable *viewable,
                               GimpContext  *context,
                               gint          width,
                               gint          height)
{
  GimpItem  *item  = GIMP_ITEM (viewable);
  GimpImage *image = gimp_item_get_image (item);

  if (! image->gimp->config->layer_previews)
    return NULL;

  return gimp_drawable_get_sub_preview (GIMP_DRAWABLE (viewable),
                                        0, 0,
                                        gimp_item_get_width  (item),
                                        gimp_item_get_height (item),
                                        width,
                                        height);
}

GdkPixbuf *
gimp_drawable_get_new_pixbuf (GimpViewable *viewable,
                              GimpContext  *context,
                              gint          width,
                              gint          height)
{
  GimpItem  *item  = GIMP_ITEM (viewable);
  GimpImage *image = gimp_item_get_image (item);

  if (! image->gimp->config->layer_previews)
    return NULL;

  return gimp_drawable_get_sub_pixbuf (GIMP_DRAWABLE (viewable),
                                       0, 0,
                                       gimp_item_get_width  (item),
                                       gimp_item_get_height (item),
                                       width,
                                       height);
}

const Babl *
gimp_drawable_get_preview_format (GimpDrawable *drawable)
{
  const Babl  *space;
  gboolean     alpha;
  GimpTRCType  trc;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  space = gimp_drawable_get_space (drawable);
  alpha = gimp_drawable_has_alpha (drawable);
  trc   = gimp_drawable_get_trc (drawable);

  switch (gimp_drawable_get_base_type (drawable))
    {
    case GIMP_GRAY:
      return gimp_babl_format (GIMP_GRAY,
                               gimp_babl_precision (GIMP_COMPONENT_TYPE_U8,
                                                    trc),
                               alpha, space);

    case GIMP_RGB:
    case GIMP_INDEXED:
      return gimp_babl_format (GIMP_RGB,
                               gimp_babl_precision (GIMP_COMPONENT_TYPE_U8,
                                                    trc),
                               alpha, space);
    }

  g_return_val_if_reached (NULL);
}

GimpTempBuf *
gimp_drawable_get_sub_preview (GimpDrawable *drawable,
                               gint          src_x,
                               gint          src_y,
                               gint          src_width,
                               gint          src_height,
                               gint          dest_width,
                               gint          dest_height)
{
  GimpItem    *item;
  GimpImage   *image;
  GeglBuffer  *buffer;
  GimpTempBuf *preview;
  gdouble      scale;
  gint         scaled_x;
  gint         scaled_y;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (src_x >= 0, NULL);
  g_return_val_if_fail (src_y >= 0, NULL);
  g_return_val_if_fail (src_width  > 0, NULL);
  g_return_val_if_fail (src_height > 0, NULL);
  g_return_val_if_fail (dest_width  > 0, NULL);
  g_return_val_if_fail (dest_height > 0, NULL);

  item = GIMP_ITEM (drawable);

  g_return_val_if_fail ((src_x + src_width)  <= gimp_item_get_width  (item), NULL);
  g_return_val_if_fail ((src_y + src_height) <= gimp_item_get_height (item), NULL);

  image = gimp_item_get_image (item);

  if (! image->gimp->config->layer_previews)
    return NULL;

  buffer = gimp_drawable_get_buffer_with_effects (drawable);

  preview = gimp_temp_buf_new (dest_width, dest_height,
                               gimp_drawable_get_preview_format (drawable));

  scale = MIN ((gdouble) dest_width  / (gdouble) src_width,
               (gdouble) dest_height / (gdouble) src_height);

  scaled_x = RINT ((gdouble) src_x * scale);
  scaled_y = RINT ((gdouble) src_y * scale);

  gegl_buffer_get (buffer,
                   GEGL_RECTANGLE (scaled_x, scaled_y, dest_width, dest_height),
                   scale,
                   gimp_temp_buf_get_format (preview),
                   gimp_temp_buf_get_data (preview),
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);
  g_object_unref (buffer);

  return preview;
}

GdkPixbuf *
gimp_drawable_get_sub_pixbuf (GimpDrawable *drawable,
                              gint          src_x,
                              gint          src_y,
                              gint          src_width,
                              gint          src_height,
                              gint          dest_width,
                              gint          dest_height)
{
  GimpItem           *item;
  GimpImage          *image;
  GeglBuffer         *buffer;
  GdkPixbuf          *pixbuf;
  gdouble             scale;
  gint                scaled_x;
  gint                scaled_y;
  GimpColorTransform *transform;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (src_x >= 0, NULL);
  g_return_val_if_fail (src_y >= 0, NULL);
  g_return_val_if_fail (src_width  > 0, NULL);
  g_return_val_if_fail (src_height > 0, NULL);
  g_return_val_if_fail (dest_width  > 0, NULL);
  g_return_val_if_fail (dest_height > 0, NULL);

  item = GIMP_ITEM (drawable);

  g_return_val_if_fail ((src_x + src_width)  <= gimp_item_get_width  (item), NULL);
  g_return_val_if_fail ((src_y + src_height) <= gimp_item_get_height (item), NULL);

  image = gimp_item_get_image (item);

  if (! image->gimp->config->layer_previews)
    return NULL;

  buffer = gimp_drawable_get_buffer_with_effects (drawable);

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                           dest_width, dest_height);

  scale = MIN ((gdouble) dest_width  / (gdouble) src_width,
               (gdouble) dest_height / (gdouble) src_height);

  scaled_x = RINT ((gdouble) src_x * scale);
  scaled_y = RINT ((gdouble) src_y * scale);

  transform = gimp_image_get_color_transform_to_srgb_u8 (image);

  if (transform)
    {
      GimpTempBuf *temp_buf;
      GeglBuffer  *src_buf;
      GeglBuffer  *dest_buf;

      temp_buf = gimp_temp_buf_new (dest_width, dest_height,
                                    gimp_drawable_get_format (drawable));

      gegl_buffer_get (buffer,
                       GEGL_RECTANGLE (scaled_x, scaled_y,
                                       dest_width, dest_height),
                       scale,
                       gimp_temp_buf_get_format (temp_buf),
                       gimp_temp_buf_get_data (temp_buf),
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

      src_buf  = gimp_temp_buf_create_buffer (temp_buf);
      dest_buf = gimp_pixbuf_create_buffer (pixbuf);

      gimp_temp_buf_unref (temp_buf);

      gimp_color_transform_process_buffer (transform,
                                           src_buf,
                                           GEGL_RECTANGLE (0, 0,
                                                           dest_width, dest_height),
                                           dest_buf,
                                           GEGL_RECTANGLE (0, 0, 0, 0));

      g_object_unref (src_buf);
      g_object_unref (dest_buf);
    }
  else
    {
      gegl_buffer_get (buffer,
                       GEGL_RECTANGLE (scaled_x, scaled_y,
                                       dest_width, dest_height),
                       scale,
                       gimp_pixbuf_get_format (pixbuf),
                       gdk_pixbuf_get_pixels (pixbuf),
                       gdk_pixbuf_get_rowstride (pixbuf),
                       GEGL_ABYSS_CLAMP);
    }

  g_object_unref (buffer);

  return pixbuf;
}

static void
gimp_drawable_get_sub_preview_async_func (GimpAsync      *async,
                                          SubPreviewData *data)
{
  GimpTempBuf             *preview;
  GimpTileHandlerValidate *validate;

  preview = gimp_temp_buf_new (data->rect.width, data->rect.height,
                               data->format);

  validate = gimp_tile_handler_validate_get_assigned (data->buffer);

  if (validate)
    {
      if (! data->iter)
        {
          cairo_region_t        *region;
          cairo_rectangle_int_t  rect;

          rect.x      = floor (data->rect.x / data->scale);
          rect.y      = floor (data->rect.y / data->scale);
          rect.width  = ceil ((data->rect.x + data->rect.width)  /
                              data->scale) - rect.x;
          rect.height = ceil ((data->rect.x + data->rect.height) /
                              data->scale) - rect.y;

          region = cairo_region_copy (validate->dirty_region);

          cairo_region_intersect_rectangle (region, &rect);

          data->iter = gimp_chunk_iterator_new (region);
        }

      if (gimp_chunk_iterator_next (data->iter))
        {
          GeglRectangle rect;

          gimp_tile_handler_validate_begin_validate (validate);

          while (gimp_chunk_iterator_get_rect (data->iter, &rect))
            {
              gimp_tile_handler_validate_validate (validate,
                                                   data->buffer, &rect,
                                                   FALSE, FALSE);
            }

          gimp_tile_handler_validate_end_validate (validate);

          return;
        }

      data->iter = NULL;
    }

  gegl_buffer_get (data->buffer, &data->rect, data->scale,
                   gimp_temp_buf_get_format (preview),
                   gimp_temp_buf_get_data (preview),
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

  sub_preview_data_free (data);

  gimp_async_finish_full (async,
                          preview,
                          (GDestroyNotify) gimp_temp_buf_unref);
}

GimpAsync *
gimp_drawable_get_sub_preview_async (GimpDrawable *drawable,
                                     gint          src_x,
                                     gint          src_y,
                                     gint          src_width,
                                     gint          src_height,
                                     gint          dest_width,
                                     gint          dest_height)
{
  GimpItem       *item;
  GimpImage      *image;
  GeglBuffer     *buffer;
  SubPreviewData *data;
  gdouble         scale;
  gint            scaled_x;
  gint            scaled_y;
  static gint     no_async_drawable_previews = -1;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (src_x >= 0, NULL);
  g_return_val_if_fail (src_y >= 0, NULL);
  g_return_val_if_fail (src_width  > 0, NULL);
  g_return_val_if_fail (src_height > 0, NULL);
  g_return_val_if_fail (dest_width  > 0, NULL);
  g_return_val_if_fail (dest_height > 0, NULL);

  item = GIMP_ITEM (drawable);

  g_return_val_if_fail ((src_x + src_width)  <= gimp_item_get_width  (item), NULL);
  g_return_val_if_fail ((src_y + src_height) <= gimp_item_get_height (item), NULL);

  image = gimp_item_get_image (item);

  if (! image->gimp->config->layer_previews)
    return NULL;

  buffer = gimp_drawable_get_buffer_with_effects (drawable);

  if (no_async_drawable_previews < 0)
    {
      no_async_drawable_previews =
        (g_getenv ("GIMP_NO_ASYNC_DRAWABLE_PREVIEWS") != NULL);
    }

  if (no_async_drawable_previews)
    {
      GimpAsync *async = gimp_async_new ();

      gimp_async_finish_full (async,
                              gimp_drawable_get_sub_preview (drawable,
                                                             src_x,
                                                             src_y,
                                                             src_width,
                                                             src_height,
                                                             dest_width,
                                                             dest_height),
                              (GDestroyNotify) gimp_temp_buf_unref);

      return async;
    }

  scale = MIN ((gdouble) dest_width  / (gdouble) src_width,
               (gdouble) dest_height / (gdouble) src_height);

  scaled_x = RINT ((gdouble) src_x * scale);
  scaled_y = RINT ((gdouble) src_y * scale);

  data = sub_preview_data_new (
    gimp_drawable_get_preview_format (drawable),
    buffer,
    GEGL_RECTANGLE (scaled_x, scaled_y, dest_width, dest_height),
    scale);

  if (gimp_tile_handler_validate_get_assigned (buffer))
    {
      return gimp_idle_run_async_full (
        GIMP_PRIORITY_VIEWABLE_IDLE,
        (GimpRunAsyncFunc) gimp_drawable_get_sub_preview_async_func,
        data,
        (GDestroyNotify) sub_preview_data_free);
    }
  else
    {
      return gimp_parallel_run_async_full (
        +1,
        (GimpRunAsyncFunc) gimp_drawable_get_sub_preview_async_func,
        data,
        (GDestroyNotify) sub_preview_data_free);
    }
}
