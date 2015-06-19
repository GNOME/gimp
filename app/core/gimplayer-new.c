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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gimpbuffer.h"
#include "gimpimage.h"
#include "gimplayer.h"
#include "gimplayer-new.h"


GimpLayer *
gimp_layer_new (GimpImage            *image,
                gint                  width,
                gint                  height,
                const Babl           *format,
                const gchar          *name,
                gdouble               opacity,
                GimpLayerModeEffects  mode)
{
  GimpLayer *layer;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (width > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);
  g_return_val_if_fail (format != NULL, NULL);

  layer = GIMP_LAYER (gimp_drawable_new (GIMP_TYPE_LAYER,
                                         image, name,
                                         0, 0, width, height,
                                         format));

  opacity = CLAMP (opacity, GIMP_OPACITY_TRANSPARENT, GIMP_OPACITY_OPAQUE);

  layer->opacity = opacity;
  layer->mode    = mode;

  return layer;
}

/**
 * gimp_layer_new_from_buffer:
 * @buffer:     The buffer to make the new layer from.
 * @dest_image: The image the new layer will be added to.
 * @format:     The #Babl format of the new layer.
 * @name:       The new layer's name.
 * @opacity:    The new layer's opacity.
 * @mode:       The new layer's mode.
 *
 * Copies %buffer to a layer taking into consideration the
 * possibility of transforming the contents to meet the requirements
 * of the target image type
 *
 * Return value: The new layer.
 **/
GimpLayer *
gimp_layer_new_from_buffer (GimpBuffer           *buffer,
                            GimpImage            *dest_image,
                            const Babl           *format,
                            const gchar          *name,
                            gdouble               opacity,
                            GimpLayerModeEffects  mode)
{
  g_return_val_if_fail (GIMP_IS_BUFFER (buffer), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (dest_image), NULL);
  g_return_val_if_fail (format != NULL, NULL);

  return gimp_layer_new_from_gegl_buffer (gimp_buffer_get_buffer (buffer),
                                          dest_image, format,
                                          name, opacity, mode);
}

/**
 * gimp_layer_new_from_gegl_buffer:
 * @buffer:     The buffer to make the new layer from.
 * @dest_image: The image the new layer will be added to.
 * @format:     The #Babl format of the new layer.
 * @name:       The new layer's name.
 * @opacity:    The new layer's opacity.
 * @mode:       The new layer's mode.
 *
 * Copies %buffer to a layer taking into consideration the
 * possibility of transforming the contents to meet the requirements
 * of the target image type
 *
 * Return value: The new layer.
 **/
GimpLayer *
gimp_layer_new_from_gegl_buffer (GeglBuffer           *buffer,
                                 GimpImage            *dest_image,
                                 const Babl           *format,
                                 const gchar          *name,
                                 gdouble               opacity,
                                 GimpLayerModeEffects  mode)
{
  GimpLayer  *layer;
  GeglBuffer *dest;

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (dest_image), NULL);
  g_return_val_if_fail (format != NULL, NULL);

  /*  do *not* use the buffer's format because this function gets
   *  buffers of any format passed, and converts them
   */
  layer = gimp_layer_new (dest_image,
                          gegl_buffer_get_width  (buffer),
                          gegl_buffer_get_height (buffer),
                          format,
                          name, opacity, mode);

  dest = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));
  gegl_buffer_copy (buffer, NULL, GEGL_ABYSS_NONE, dest, NULL);

  return layer;
}

/**
 * gimp_layer_new_from_pixbuf:
 * @pixbuf:     The pixbuf to make the new layer from.
 * @dest_image: The image the new layer will be added to.
 * @format:     The #Babl format of the new layer.
 * @name:       The new layer's name.
 * @opacity:    The new layer's opacity.
 * @mode:       The new layer's mode.
 *
 * Copies %pixbuf to a layer taking into consideration the
 * possibility of transforming the contents to meet the requirements
 * of the target image type
 *
 * Return value: The new layer.
 **/
GimpLayer *
gimp_layer_new_from_pixbuf (GdkPixbuf            *pixbuf,
                            GimpImage            *dest_image,
                            const Babl           *format,
                            const gchar          *name,
                            gdouble               opacity,
                            GimpLayerModeEffects  mode)
{
  GeglBuffer *buffer;
  GimpLayer  *layer;
  gint        width;
  gint        height;

  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (dest_image), NULL);
  g_return_val_if_fail (format != NULL, NULL);

  width  = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  layer = gimp_layer_new (dest_image, width, height,
                          format, name, opacity, mode);

  buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                   gimp_pixbuf_get_format (pixbuf),
                   gdk_pixbuf_get_pixels (pixbuf),
                   gdk_pixbuf_get_rowstride (pixbuf));

  return layer;
}
