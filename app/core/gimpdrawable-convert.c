/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997-2004 Adam D. Moss <adam@gimp.org>
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

#include <cairo.h>
#include <gegl.h>

#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gegl/gimp-gegl-utils.h"

#include "gimpdrawable.h"
#include "gimpdrawable-convert.h"
#include "gimpimage.h"
#include "gimpimage-colormap.h"


void
gimp_drawable_convert_rgb (GimpDrawable *drawable,
                           GimpImage    *dest_image,
                           gboolean      push_undo)
{
  GimpImageType  type;
  GeglBuffer    *dest_buffer;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (GIMP_IS_IMAGE (dest_image));
  g_return_if_fail (! gimp_drawable_is_rgb (drawable));

  type = GIMP_RGB_IMAGE;

  if (gimp_drawable_has_alpha (drawable))
    type = GIMP_IMAGE_TYPE_WITH_ALPHA (type);

  dest_buffer =
    gimp_gegl_buffer_new (GIMP_GEGL_RECT (0, 0,
                                          gimp_item_get_width  (GIMP_ITEM (drawable)),
                                          gimp_item_get_height (GIMP_ITEM (drawable))),
                          gimp_image_get_format (dest_image, type));

  gegl_buffer_copy (gimp_drawable_get_buffer (drawable), NULL,
                    dest_buffer, NULL);

  gimp_drawable_set_buffer (drawable, push_undo, NULL, dest_buffer);
  g_object_unref (dest_buffer);
}

void
gimp_drawable_convert_grayscale (GimpDrawable *drawable,
                                 GimpImage    *dest_image,
                                 gboolean      push_undo)
{
  GimpImageType  type;
  GeglBuffer    *dest_buffer;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (GIMP_IS_IMAGE (dest_image));
  g_return_if_fail (! gimp_drawable_is_gray (drawable));

  type = GIMP_GRAY_IMAGE;

  if (gimp_drawable_has_alpha (drawable))
    type = GIMP_IMAGE_TYPE_WITH_ALPHA (type);

  dest_buffer =
    gimp_gegl_buffer_new (GIMP_GEGL_RECT (0, 0,
                                          gimp_item_get_width  (GIMP_ITEM (drawable)),
                                          gimp_item_get_height (GIMP_ITEM (drawable))),
                          gimp_image_get_format (dest_image, type));

  gegl_buffer_copy (gimp_drawable_get_buffer (drawable), NULL,
                    dest_buffer, NULL);

  gimp_drawable_set_buffer (drawable, push_undo, NULL, dest_buffer);
  g_object_unref (dest_buffer);
}

void
gimp_drawable_convert_indexed (GimpDrawable *drawable,
                               GimpImage    *dest_image,
                               gboolean      push_undo)
{
  GeglBuffer *dest_buffer;
  const Babl *format;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (GIMP_IS_IMAGE (dest_image));
  g_return_if_fail (! gimp_drawable_is_gray (drawable));

  if (gimp_drawable_has_alpha (drawable))
    format = gimp_image_colormap_get_rgba_format (dest_image);
  else
    format = gimp_image_colormap_get_rgb_format (dest_image);

  dest_buffer =
    gimp_gegl_buffer_new (GIMP_GEGL_RECT (0, 0,
                                          gimp_item_get_width  (GIMP_ITEM (drawable)),
                                          gimp_item_get_height (GIMP_ITEM (drawable))),
                          format);

  gegl_buffer_copy (gimp_drawable_get_buffer (drawable), NULL,
                    dest_buffer, NULL);

  gimp_drawable_set_buffer (drawable, push_undo, NULL, dest_buffer);
  g_object_unref (dest_buffer);
}
