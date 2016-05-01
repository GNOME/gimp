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
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "config/gimpcoreconfig.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimp-gegl-loops.h"

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpimage.h"
#include "gimpimage-color-profile.h"
#include "gimpdrawable-preview.h"
#include "gimpdrawable-private.h"
#include "gimplayer.h"
#include "gimptempbuf.h"


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
  gboolean alpha;
  gboolean linear;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  alpha  = gimp_drawable_has_alpha (drawable);
  linear = gimp_drawable_get_linear (drawable);

  switch (gimp_drawable_get_base_type (drawable))
    {
    case GIMP_GRAY:
      return gimp_babl_format (GIMP_GRAY,
                               gimp_babl_precision (GIMP_COMPONENT_TYPE_U8,
                                                    linear),
                               alpha);

    case GIMP_RGB:
      return gimp_babl_format (GIMP_RGB,
                               gimp_babl_precision (GIMP_COMPONENT_TYPE_U8,
                                                    linear),
                               alpha);

    case GIMP_INDEXED:
      if (alpha)
        return babl_format ("R'G'B'A u8");
      else
        return babl_format ("R'G'B' u8");
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

  buffer = gimp_drawable_get_buffer (drawable);

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
  GimpColorTransform  transform;
  const Babl         *src_format;
  const Babl         *dest_format;

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

  buffer = gimp_drawable_get_buffer (drawable);

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                           dest_width, dest_height);

  scale = MIN ((gdouble) dest_width  / (gdouble) src_width,
               (gdouble) dest_height / (gdouble) src_height);

  scaled_x = RINT ((gdouble) src_x * scale);
  scaled_y = RINT ((gdouble) src_y * scale);

  transform = gimp_image_get_color_transform_to_srgb_u8 (image,
                                                         &src_format,
                                                         &dest_format);

  if (transform)
    {
      GimpTempBuf *temp_buf;
      GeglBuffer  *src_buf;
      GeglBuffer  *dest_buf;

      temp_buf = gimp_temp_buf_new (dest_width, dest_height, src_format);

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

      gimp_gegl_convert_color_transform (src_buf,
                                         GEGL_RECTANGLE (0, 0,
                                                         dest_width, dest_height),
                                         src_format,
                                         dest_buf,
                                         GEGL_RECTANGLE (0, 0, 0, 0),
                                         dest_format,
                                         transform,
                                         NULL);

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

  return pixbuf;
}
