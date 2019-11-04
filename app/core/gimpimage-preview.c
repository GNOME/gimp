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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimp-gegl-loops.h"

#include "gimpimage.h"
#include "gimpimage-color-profile.h"
#include "gimpimage-preview.h"
#include "gimppickable.h"
#include "gimpprojectable.h"
#include "gimpprojection.h"
#include "gimptempbuf.h"


const Babl *
gimp_image_get_preview_format (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  switch (gimp_image_get_base_type (image))
    {
    case GIMP_RGB:
    case GIMP_GRAY:
      return gimp_babl_format_change_component_type (
        gimp_projectable_get_format (GIMP_PROJECTABLE (image)),
        GIMP_COMPONENT_TYPE_U8);

    case GIMP_INDEXED:
      return babl_format ("R'G'B'A u8");
    }

  g_return_val_if_reached (NULL);
}

void
gimp_image_get_preview_size (GimpViewable *viewable,
                             gint          size,
                             gboolean      is_popup,
                             gboolean      dot_for_dot,
                             gint         *width,
                             gint         *height)
{
  GimpImage *image = GIMP_IMAGE (viewable);
  gdouble    xres;
  gdouble    yres;

  gimp_image_get_resolution (image, &xres, &yres);

  gimp_viewable_calc_preview_size (gimp_image_get_width  (image),
                                   gimp_image_get_height (image),
                                   size,
                                   size,
                                   dot_for_dot,
                                   xres,
                                   yres,
                                   width,
                                   height,
                                   NULL);
}

gboolean
gimp_image_get_popup_size (GimpViewable *viewable,
                           gint          width,
                           gint          height,
                           gboolean      dot_for_dot,
                           gint         *popup_width,
                           gint         *popup_height)
{
  GimpImage *image = GIMP_IMAGE (viewable);

  if (gimp_image_get_width  (image) > width ||
      gimp_image_get_height (image) > height)
    {
      gboolean scaling_up;

      gimp_viewable_calc_preview_size (gimp_image_get_width  (image),
                                       gimp_image_get_height (image),
                                       width  * 2,
                                       height * 2,
                                       dot_for_dot, 1.0, 1.0,
                                       popup_width,
                                       popup_height,
                                       &scaling_up);

      if (scaling_up)
        {
          *popup_width  = gimp_image_get_width  (image);
          *popup_height = gimp_image_get_height (image);
        }

      return TRUE;
    }

  return FALSE;
}

GimpTempBuf *
gimp_image_get_new_preview (GimpViewable *viewable,
                            GimpContext  *context,
                            gint          width,
                            gint          height)
{
  GimpImage   *image = GIMP_IMAGE (viewable);
  const Babl  *format;
  GimpTempBuf *buf;
  gdouble      scale_x;
  gdouble      scale_y;

  scale_x = (gdouble) width  / (gdouble) gimp_image_get_width  (image);
  scale_y = (gdouble) height / (gdouble) gimp_image_get_height (image);

  format = gimp_image_get_preview_format (image);

  buf = gimp_temp_buf_new (width, height, format);

  gegl_buffer_get (gimp_pickable_get_buffer (GIMP_PICKABLE (image)),
                   GEGL_RECTANGLE (0, 0, width, height),
                   MIN (scale_x, scale_y),
                   gimp_temp_buf_get_format (buf),
                   gimp_temp_buf_get_data (buf),
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

  return buf;
}

GdkPixbuf *
gimp_image_get_new_pixbuf (GimpViewable *viewable,
                           GimpContext  *context,
                           gint          width,
                           gint          height)
{
  GimpImage          *image = GIMP_IMAGE (viewable);
  GdkPixbuf          *pixbuf;
  gdouble             scale_x;
  gdouble             scale_y;
  GimpColorTransform *transform;

  scale_x = (gdouble) width  / (gdouble) gimp_image_get_width  (image);
  scale_y = (gdouble) height / (gdouble) gimp_image_get_height (image);

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                           width, height);

  transform = gimp_image_get_color_transform_to_srgb_u8 (image);

  if (transform)
    {
      GimpTempBuf *temp_buf;
      GeglBuffer  *src_buf;
      GeglBuffer  *dest_buf;

      temp_buf = gimp_temp_buf_new (width, height,
                                    gimp_pickable_get_format (GIMP_PICKABLE (image)));

      gegl_buffer_get (gimp_pickable_get_buffer (GIMP_PICKABLE (image)),
                       GEGL_RECTANGLE (0, 0, width, height),
                       MIN (scale_x, scale_y),
                       gimp_temp_buf_get_format (temp_buf),
                       gimp_temp_buf_get_data (temp_buf),
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

      src_buf  = gimp_temp_buf_create_buffer (temp_buf);
      dest_buf = gimp_pixbuf_create_buffer (pixbuf);

      gimp_temp_buf_unref (temp_buf);

      gimp_color_transform_process_buffer (transform,
                                           src_buf,
                                           GEGL_RECTANGLE (0, 0,
                                                           width, height),
                                           dest_buf,
                                           GEGL_RECTANGLE (0, 0, 0, 0));

      g_object_unref (src_buf);
      g_object_unref (dest_buf);
    }
  else
    {
      gegl_buffer_get (gimp_pickable_get_buffer (GIMP_PICKABLE (image)),
                       GEGL_RECTANGLE (0, 0, width, height),
                       MIN (scale_x, scale_y),
                       gimp_pixbuf_get_format (pixbuf),
                       gdk_pixbuf_get_pixels (pixbuf),
                       gdk_pixbuf_get_rowstride (pixbuf),
                       GEGL_ABYSS_CLAMP);
    }

  return pixbuf;
}
