/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmacolor/ligmacolor.h"
#include "libligmamath/ligmamath.h"

#include "core-types.h"

#include "config/ligmacoreconfig.h"

#include "gegl/ligma-babl.h"
#include "gegl/ligma-gegl-loops.h"
#include "gegl/ligmatilehandlervalidate.h"

#include "ligma.h"
#include "ligma-parallel.h"
#include "ligma-utils.h"
#include "ligmaasync.h"
#include "ligmachannel.h"
#include "ligmachunkiterator.h"
#include "ligmaimage.h"
#include "ligmaimage-color-profile.h"
#include "ligmadrawable-preview.h"
#include "ligmadrawable-private.h"
#include "ligmalayer.h"
#include "ligmatempbuf.h"

#include "ligma-priorities.h"


typedef struct
{
  const Babl        *format;
  GeglBuffer        *buffer;
  GeglRectangle      rect;
  gdouble            scale;

  LigmaChunkIterator *iter;
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
  data->buffer = g_object_ref (buffer);
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
    ligma_chunk_iterator_stop (data->iter, TRUE);

  g_slice_free (SubPreviewData, data);
}


/*  public functions  */


LigmaTempBuf *
ligma_drawable_get_new_preview (LigmaViewable *viewable,
                               LigmaContext  *context,
                               gint          width,
                               gint          height)
{
  LigmaItem  *item  = LIGMA_ITEM (viewable);
  LigmaImage *image = ligma_item_get_image (item);

  if (! image->ligma->config->layer_previews)
    return NULL;

  return ligma_drawable_get_sub_preview (LIGMA_DRAWABLE (viewable),
                                        0, 0,
                                        ligma_item_get_width  (item),
                                        ligma_item_get_height (item),
                                        width,
                                        height);
}

GdkPixbuf *
ligma_drawable_get_new_pixbuf (LigmaViewable *viewable,
                              LigmaContext  *context,
                              gint          width,
                              gint          height)
{
  LigmaItem  *item  = LIGMA_ITEM (viewable);
  LigmaImage *image = ligma_item_get_image (item);

  if (! image->ligma->config->layer_previews)
    return NULL;

  return ligma_drawable_get_sub_pixbuf (LIGMA_DRAWABLE (viewable),
                                       0, 0,
                                       ligma_item_get_width  (item),
                                       ligma_item_get_height (item),
                                       width,
                                       height);
}

const Babl *
ligma_drawable_get_preview_format (LigmaDrawable *drawable)
{
  const Babl  *space;
  gboolean     alpha;
  LigmaTRCType  trc;

  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);

  space = ligma_drawable_get_space (drawable);
  alpha = ligma_drawable_has_alpha (drawable);
  trc   = ligma_drawable_get_trc (drawable);

  switch (ligma_drawable_get_base_type (drawable))
    {
    case LIGMA_GRAY:
      return ligma_babl_format (LIGMA_GRAY,
                               ligma_babl_precision (LIGMA_COMPONENT_TYPE_U8,
                                                    trc),
                               alpha, space);

    case LIGMA_RGB:
    case LIGMA_INDEXED:
      return ligma_babl_format (LIGMA_RGB,
                               ligma_babl_precision (LIGMA_COMPONENT_TYPE_U8,
                                                    trc),
                               alpha, space);
    }

  g_return_val_if_reached (NULL);
}

LigmaTempBuf *
ligma_drawable_get_sub_preview (LigmaDrawable *drawable,
                               gint          src_x,
                               gint          src_y,
                               gint          src_width,
                               gint          src_height,
                               gint          dest_width,
                               gint          dest_height)
{
  LigmaItem    *item;
  LigmaImage   *image;
  GeglBuffer  *buffer;
  LigmaTempBuf *preview;
  gdouble      scale;
  gint         scaled_x;
  gint         scaled_y;

  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (src_x >= 0, NULL);
  g_return_val_if_fail (src_y >= 0, NULL);
  g_return_val_if_fail (src_width  > 0, NULL);
  g_return_val_if_fail (src_height > 0, NULL);
  g_return_val_if_fail (dest_width  > 0, NULL);
  g_return_val_if_fail (dest_height > 0, NULL);

  item = LIGMA_ITEM (drawable);

  g_return_val_if_fail ((src_x + src_width)  <= ligma_item_get_width  (item), NULL);
  g_return_val_if_fail ((src_y + src_height) <= ligma_item_get_height (item), NULL);

  image = ligma_item_get_image (item);

  if (! image->ligma->config->layer_previews)
    return NULL;

  buffer = ligma_drawable_get_buffer (drawable);

  preview = ligma_temp_buf_new (dest_width, dest_height,
                               ligma_drawable_get_preview_format (drawable));

  scale = MIN ((gdouble) dest_width  / (gdouble) src_width,
               (gdouble) dest_height / (gdouble) src_height);

  scaled_x = RINT ((gdouble) src_x * scale);
  scaled_y = RINT ((gdouble) src_y * scale);

  gegl_buffer_get (buffer,
                   GEGL_RECTANGLE (scaled_x, scaled_y, dest_width, dest_height),
                   scale,
                   ligma_temp_buf_get_format (preview),
                   ligma_temp_buf_get_data (preview),
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

  return preview;
}

GdkPixbuf *
ligma_drawable_get_sub_pixbuf (LigmaDrawable *drawable,
                              gint          src_x,
                              gint          src_y,
                              gint          src_width,
                              gint          src_height,
                              gint          dest_width,
                              gint          dest_height)
{
  LigmaItem           *item;
  LigmaImage          *image;
  GeglBuffer         *buffer;
  GdkPixbuf          *pixbuf;
  gdouble             scale;
  gint                scaled_x;
  gint                scaled_y;
  LigmaColorTransform *transform;

  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (src_x >= 0, NULL);
  g_return_val_if_fail (src_y >= 0, NULL);
  g_return_val_if_fail (src_width  > 0, NULL);
  g_return_val_if_fail (src_height > 0, NULL);
  g_return_val_if_fail (dest_width  > 0, NULL);
  g_return_val_if_fail (dest_height > 0, NULL);

  item = LIGMA_ITEM (drawable);

  g_return_val_if_fail ((src_x + src_width)  <= ligma_item_get_width  (item), NULL);
  g_return_val_if_fail ((src_y + src_height) <= ligma_item_get_height (item), NULL);

  image = ligma_item_get_image (item);

  if (! image->ligma->config->layer_previews)
    return NULL;

  buffer = ligma_drawable_get_buffer (drawable);

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                           dest_width, dest_height);

  scale = MIN ((gdouble) dest_width  / (gdouble) src_width,
               (gdouble) dest_height / (gdouble) src_height);

  scaled_x = RINT ((gdouble) src_x * scale);
  scaled_y = RINT ((gdouble) src_y * scale);

  transform = ligma_image_get_color_transform_to_srgb_u8 (image);

  if (transform)
    {
      LigmaTempBuf *temp_buf;
      GeglBuffer  *src_buf;
      GeglBuffer  *dest_buf;

      temp_buf = ligma_temp_buf_new (dest_width, dest_height,
                                    ligma_drawable_get_format (drawable));

      gegl_buffer_get (buffer,
                       GEGL_RECTANGLE (scaled_x, scaled_y,
                                       dest_width, dest_height),
                       scale,
                       ligma_temp_buf_get_format (temp_buf),
                       ligma_temp_buf_get_data (temp_buf),
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

      src_buf  = ligma_temp_buf_create_buffer (temp_buf);
      dest_buf = ligma_pixbuf_create_buffer (pixbuf);

      ligma_temp_buf_unref (temp_buf);

      ligma_color_transform_process_buffer (transform,
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
                       ligma_pixbuf_get_format (pixbuf),
                       gdk_pixbuf_get_pixels (pixbuf),
                       gdk_pixbuf_get_rowstride (pixbuf),
                       GEGL_ABYSS_CLAMP);
    }

  return pixbuf;
}

static void
ligma_drawable_get_sub_preview_async_func (LigmaAsync      *async,
                                          SubPreviewData *data)
{
  LigmaTempBuf             *preview;
  LigmaTileHandlerValidate *validate;

  preview = ligma_temp_buf_new (data->rect.width, data->rect.height,
                               data->format);

  validate = ligma_tile_handler_validate_get_assigned (data->buffer);

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

          data->iter = ligma_chunk_iterator_new (region);
        }

      if (ligma_chunk_iterator_next (data->iter))
        {
          GeglRectangle rect;

          ligma_tile_handler_validate_begin_validate (validate);

          while (ligma_chunk_iterator_get_rect (data->iter, &rect))
            {
              ligma_tile_handler_validate_validate (validate,
                                                   data->buffer, &rect,
                                                   FALSE, FALSE);
            }

          ligma_tile_handler_validate_end_validate (validate);

          return;
        }

      data->iter = NULL;
    }

  gegl_buffer_get (data->buffer, &data->rect, data->scale,
                   ligma_temp_buf_get_format (preview),
                   ligma_temp_buf_get_data (preview),
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

  sub_preview_data_free (data);

  ligma_async_finish_full (async,
                          preview,
                          (GDestroyNotify) ligma_temp_buf_unref);
}

LigmaAsync *
ligma_drawable_get_sub_preview_async (LigmaDrawable *drawable,
                                     gint          src_x,
                                     gint          src_y,
                                     gint          src_width,
                                     gint          src_height,
                                     gint          dest_width,
                                     gint          dest_height)
{
  LigmaItem       *item;
  LigmaImage      *image;
  GeglBuffer     *buffer;
  SubPreviewData *data;
  gdouble         scale;
  gint            scaled_x;
  gint            scaled_y;
  static gint     no_async_drawable_previews = -1;

  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (src_x >= 0, NULL);
  g_return_val_if_fail (src_y >= 0, NULL);
  g_return_val_if_fail (src_width  > 0, NULL);
  g_return_val_if_fail (src_height > 0, NULL);
  g_return_val_if_fail (dest_width  > 0, NULL);
  g_return_val_if_fail (dest_height > 0, NULL);

  item = LIGMA_ITEM (drawable);

  g_return_val_if_fail ((src_x + src_width)  <= ligma_item_get_width  (item), NULL);
  g_return_val_if_fail ((src_y + src_height) <= ligma_item_get_height (item), NULL);

  image = ligma_item_get_image (item);

  if (! image->ligma->config->layer_previews)
    return NULL;

  buffer = ligma_drawable_get_buffer (drawable);

  if (no_async_drawable_previews < 0)
    {
      no_async_drawable_previews =
        (g_getenv ("LIGMA_NO_ASYNC_DRAWABLE_PREVIEWS") != NULL);
    }

  if (no_async_drawable_previews)
    {
      LigmaAsync *async = ligma_async_new ();

      ligma_async_finish_full (async,
                              ligma_drawable_get_sub_preview (drawable,
                                                             src_x,
                                                             src_y,
                                                             src_width,
                                                             src_height,
                                                             dest_width,
                                                             dest_height),
                              (GDestroyNotify) ligma_temp_buf_unref);

      return async;
    }

  scale = MIN ((gdouble) dest_width  / (gdouble) src_width,
               (gdouble) dest_height / (gdouble) src_height);

  scaled_x = RINT ((gdouble) src_x * scale);
  scaled_y = RINT ((gdouble) src_y * scale);

  data = sub_preview_data_new (
    ligma_drawable_get_preview_format (drawable),
    buffer,
    GEGL_RECTANGLE (scaled_x, scaled_y, dest_width, dest_height),
    scale);

  if (ligma_tile_handler_validate_get_assigned (buffer))
    {
      return ligma_idle_run_async_full (
        LIGMA_PRIORITY_VIEWABLE_IDLE,
        (LigmaRunAsyncFunc) ligma_drawable_get_sub_preview_async_func,
        data,
        (GDestroyNotify) sub_preview_data_free);
    }
  else
    {
      return ligma_parallel_run_async_full (
        +1,
        (LigmaRunAsyncFunc) ligma_drawable_get_sub_preview_async_func,
        data,
        (GDestroyNotify) sub_preview_data_free);
    }
}
