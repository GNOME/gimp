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
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimp-gegl-loops.h"

#include "gimpbuffer.h"
#include "gimpimage.h"
#include "gimpimage-color-profile.h"
#include "gimplayer.h"
#include "gimplayer-new.h"


/*  local function prototypes  */

static void   gimp_layer_new_convert_buffer (GimpLayer         *layer,
                                             GeglBuffer        *src_buffer,
                                             GimpColorProfile  *src_profile,
                                             GError           **error);


/*  public functions  */

GimpLayer *
gimp_layer_new (GimpImage     *image,
                gint           width,
                gint           height,
                const Babl    *format,
                const gchar   *name,
                gdouble        opacity,
                GimpLayerMode  mode)
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

  gimp_layer_set_opacity (layer, opacity, FALSE);
  gimp_layer_set_mode (layer, mode, FALSE);

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
gimp_layer_new_from_buffer (GimpBuffer    *buffer,
                            GimpImage     *dest_image,
                            const Babl    *format,
                            const gchar   *name,
                            gdouble        opacity,
                            GimpLayerMode  mode)
{
  g_return_val_if_fail (GIMP_IS_BUFFER (buffer), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (dest_image), NULL);
  g_return_val_if_fail (format != NULL, NULL);

  return gimp_layer_new_from_gegl_buffer (gimp_buffer_get_buffer (buffer),
                                          dest_image, format,
                                          name, opacity, mode,
                                          gimp_buffer_get_color_profile (buffer));
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
gimp_layer_new_from_gegl_buffer (GeglBuffer       *buffer,
                                 GimpImage        *dest_image,
                                 const Babl       *format,
                                 const gchar      *name,
                                 gdouble           opacity,
                                 GimpLayerMode     mode,
                                 GimpColorProfile *buffer_profile)
{
  GimpLayer           *layer;
  const GeglRectangle *extent;

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (dest_image), NULL);
  g_return_val_if_fail (format != NULL, NULL);
  g_return_val_if_fail (buffer_profile == NULL ||
                        GIMP_IS_COLOR_PROFILE (buffer_profile), NULL);

  extent = gegl_buffer_get_extent (buffer);

  /*  do *not* use the buffer's format because this function gets
   *  buffers of any format passed, and converts them
   */
  layer = gimp_layer_new (dest_image,
                          extent->width, extent->height,
                          format,
                          name, opacity, mode);

  if (extent->x != 0 || extent->y != 0)
    gimp_item_translate (GIMP_ITEM (layer), extent->x, extent->y, FALSE);

  gimp_layer_new_convert_buffer (layer, buffer, buffer_profile, NULL);

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
gimp_layer_new_from_pixbuf (GdkPixbuf     *pixbuf,
                            GimpImage     *dest_image,
                            const Babl    *format,
                            const gchar   *name,
                            gdouble        opacity,
                            GimpLayerMode  mode)
{
  GimpLayer        *layer;
  GeglBuffer       *buffer;
  guint8           *icc_data;
  gsize             icc_len;
  GimpColorProfile *profile = NULL;

  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (dest_image), NULL);
  g_return_val_if_fail (format != NULL, NULL);

  layer = gimp_layer_new (dest_image,
                          gdk_pixbuf_get_width  (pixbuf),
                          gdk_pixbuf_get_height (pixbuf),
                          format, name, opacity, mode);

  buffer = gimp_pixbuf_create_buffer (pixbuf);

  icc_data = gimp_pixbuf_get_icc_profile (pixbuf, &icc_len);
  if (icc_data)
    {
      profile = gimp_color_profile_new_from_icc_profile (icc_data, icc_len,
                                                         NULL);
      g_free (icc_data);
    }

  gimp_layer_new_convert_buffer (layer, buffer, profile, NULL);

  if (profile)
    g_object_unref (profile);

  g_object_unref (buffer);

  return layer;
}


/*  private functions  */

static void
gimp_layer_new_convert_buffer (GimpLayer         *layer,
                               GeglBuffer        *src_buffer,
                               GimpColorProfile  *src_profile,
                               GError           **error)
{
  GimpDrawable     *drawable    = GIMP_DRAWABLE (layer);
  GimpImage        *image       = gimp_item_get_image (GIMP_ITEM (layer));
  GeglBuffer       *dest_buffer = gimp_drawable_get_buffer (drawable);
  GimpColorProfile *dest_profile;

  if (! gimp_image_get_is_color_managed (image))
    {
      gimp_gegl_buffer_copy (src_buffer, NULL, GEGL_ABYSS_NONE,
                             dest_buffer, NULL);
      return;
    }

  if (! src_profile)
    {
      const Babl *src_format = gegl_buffer_get_format (src_buffer);

      src_profile = gimp_babl_format_get_color_profile (src_format);
    }

  dest_profile =
    gimp_color_managed_get_color_profile (GIMP_COLOR_MANAGED (layer));

  gimp_gegl_convert_color_profile (src_buffer,  NULL, src_profile,
                                   dest_buffer, NULL, dest_profile,
                                   GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL,
                                   TRUE, NULL);
}
