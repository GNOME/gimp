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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmacolor/ligmacolor.h"

#include "core-types.h"

#include "gegl/ligma-babl.h"
#include "gegl/ligma-gegl-loops.h"

#include "ligmaimage.h"
#include "ligmaimage-color-profile.h"
#include "ligmaimage-preview.h"
#include "ligmapickable.h"
#include "ligmaprojectable.h"
#include "ligmaprojection.h"
#include "ligmatempbuf.h"


const Babl *
ligma_image_get_preview_format (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  switch (ligma_image_get_base_type (image))
    {
    case LIGMA_RGB:
    case LIGMA_GRAY:
      return ligma_babl_format_change_component_type (
        ligma_projectable_get_format (LIGMA_PROJECTABLE (image)),
        LIGMA_COMPONENT_TYPE_U8);

    case LIGMA_INDEXED:
      return babl_format ("R'G'B'A u8");
    }

  g_return_val_if_reached (NULL);
}

void
ligma_image_get_preview_size (LigmaViewable *viewable,
                             gint          size,
                             gboolean      is_popup,
                             gboolean      dot_for_dot,
                             gint         *width,
                             gint         *height)
{
  LigmaImage *image = LIGMA_IMAGE (viewable);
  gdouble    xres;
  gdouble    yres;

  ligma_image_get_resolution (image, &xres, &yres);

  ligma_viewable_calc_preview_size (ligma_image_get_width  (image),
                                   ligma_image_get_height (image),
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
ligma_image_get_popup_size (LigmaViewable *viewable,
                           gint          width,
                           gint          height,
                           gboolean      dot_for_dot,
                           gint         *popup_width,
                           gint         *popup_height)
{
  LigmaImage *image = LIGMA_IMAGE (viewable);

  if (ligma_image_get_width  (image) > width ||
      ligma_image_get_height (image) > height)
    {
      gboolean scaling_up;

      ligma_viewable_calc_preview_size (ligma_image_get_width  (image),
                                       ligma_image_get_height (image),
                                       width  * 2,
                                       height * 2,
                                       dot_for_dot, 1.0, 1.0,
                                       popup_width,
                                       popup_height,
                                       &scaling_up);

      if (scaling_up)
        {
          *popup_width  = ligma_image_get_width  (image);
          *popup_height = ligma_image_get_height (image);
        }

      return TRUE;
    }

  return FALSE;
}

LigmaTempBuf *
ligma_image_get_new_preview (LigmaViewable *viewable,
                            LigmaContext  *context,
                            gint          width,
                            gint          height)
{
  LigmaImage   *image = LIGMA_IMAGE (viewable);
  const Babl  *format;
  LigmaTempBuf *buf;
  gdouble      scale_x;
  gdouble      scale_y;

  scale_x = (gdouble) width  / (gdouble) ligma_image_get_width  (image);
  scale_y = (gdouble) height / (gdouble) ligma_image_get_height (image);

  format = ligma_image_get_preview_format (image);

  buf = ligma_temp_buf_new (width, height, format);

  gegl_buffer_get (ligma_pickable_get_buffer (LIGMA_PICKABLE (image)),
                   GEGL_RECTANGLE (0, 0, width, height),
                   MIN (scale_x, scale_y),
                   ligma_temp_buf_get_format (buf),
                   ligma_temp_buf_get_data (buf),
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

  return buf;
}

GdkPixbuf *
ligma_image_get_new_pixbuf (LigmaViewable *viewable,
                           LigmaContext  *context,
                           gint          width,
                           gint          height)
{
  LigmaImage          *image = LIGMA_IMAGE (viewable);
  GdkPixbuf          *pixbuf;
  gdouble             scale_x;
  gdouble             scale_y;
  LigmaColorTransform *transform;

  scale_x = (gdouble) width  / (gdouble) ligma_image_get_width  (image);
  scale_y = (gdouble) height / (gdouble) ligma_image_get_height (image);

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                           width, height);

  transform = ligma_image_get_color_transform_to_srgb_u8 (image);

  if (transform)
    {
      LigmaTempBuf *temp_buf;
      GeglBuffer  *src_buf;
      GeglBuffer  *dest_buf;

      temp_buf = ligma_temp_buf_new (width, height,
                                    ligma_pickable_get_format (LIGMA_PICKABLE (image)));

      gegl_buffer_get (ligma_pickable_get_buffer (LIGMA_PICKABLE (image)),
                       GEGL_RECTANGLE (0, 0, width, height),
                       MIN (scale_x, scale_y),
                       ligma_temp_buf_get_format (temp_buf),
                       ligma_temp_buf_get_data (temp_buf),
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

      src_buf  = ligma_temp_buf_create_buffer (temp_buf);
      dest_buf = ligma_pixbuf_create_buffer (pixbuf);

      ligma_temp_buf_unref (temp_buf);

      ligma_color_transform_process_buffer (transform,
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
      gegl_buffer_get (ligma_pickable_get_buffer (LIGMA_PICKABLE (image)),
                       GEGL_RECTANGLE (0, 0, width, height),
                       MIN (scale_x, scale_y),
                       ligma_pixbuf_get_format (pixbuf),
                       gdk_pixbuf_get_pixels (pixbuf),
                       gdk_pixbuf_get_rowstride (pixbuf),
                       GEGL_ABYSS_CLAMP);
    }

  return pixbuf;
}
