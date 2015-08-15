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
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "config/gimpcoreconfig.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimp-gegl-loops.h"

#include "gimp.h"
#include "gimpbuffer.h"
#include "gimpimage.h"
#include "gimplayer.h"
#include "gimplayer-new.h"


/*  local function prototypes  */

static gboolean   gimp_layer_new_convert_profile (GimpLayer     *layer,
                                                  GeglBuffer    *src_buffer,
                                                  const guint8  *icc_data,
                                                  gsize          icc_length,
                                                  GError       **error);


/*  public functions  */

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
  const guint8 *icc_data;
  gsize         icc_len;

  g_return_val_if_fail (GIMP_IS_BUFFER (buffer), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (dest_image), NULL);
  g_return_val_if_fail (format != NULL, NULL);

  icc_data = gimp_buffer_get_icc_profile (buffer, &icc_len);

  return gimp_layer_new_from_gegl_buffer (gimp_buffer_get_buffer (buffer),
                                          dest_image, format,
                                          name, opacity, mode,
                                          icc_data, icc_len);
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
                                 GimpLayerModeEffects  mode,
                                 const guint8         *buffer_icc_data,
                                 gsize                 buffer_icc_length)
{
  GimpLayer *layer;

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (dest_image), NULL);
  g_return_val_if_fail (format != NULL, NULL);
  g_return_val_if_fail (buffer_icc_data != NULL || buffer_icc_length == 0, NULL);

  /*  do *not* use the buffer's format because this function gets
   *  buffers of any format passed, and converts them
   */
  layer = gimp_layer_new (dest_image,
                          gegl_buffer_get_width  (buffer),
                          gegl_buffer_get_height (buffer),
                          format,
                          name, opacity, mode);

  if (buffer_icc_data)
    {
      gimp_layer_new_convert_profile (layer, buffer,
                                      buffer_icc_data, buffer_icc_length,
                                      NULL);
    }
  else
    {
      gegl_buffer_copy (buffer, NULL, GEGL_ABYSS_NONE,
                        gimp_drawable_get_buffer (GIMP_DRAWABLE (layer)), NULL);
    }

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
  GimpLayer  *layer;
  GeglBuffer *buffer;
  guint8     *icc_data;
  gsize       icc_len;

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
      gimp_layer_new_convert_profile (layer, buffer, icc_data, icc_len, NULL);
      g_free (icc_data);
    }
  else if (gdk_pixbuf_get_colorspace (pixbuf) == GDK_COLORSPACE_RGB)
    {
      GimpColorProfile *profile = gimp_color_profile_new_srgb ();
      const guint8     *icc_data;

      icc_data = gimp_color_profile_get_icc_profile (profile, &icc_len);
      gimp_layer_new_convert_profile (layer, buffer, icc_data, icc_len, NULL);

      g_object_unref (profile);
    }
  else
    {
      gegl_buffer_copy (buffer, NULL, GEGL_ABYSS_NONE,
                        gimp_drawable_get_buffer (GIMP_DRAWABLE (layer)), NULL);
    }

  g_object_unref (buffer);

  return layer;
}


/*  private functions  */

static gboolean
gimp_layer_new_convert_profile (GimpLayer     *layer,
                                GeglBuffer    *src_buffer,
                                const guint8  *icc_data,
                                gsize          icc_length,
                                GError       **error)
{
  GimpDrawable     *drawable    = GIMP_DRAWABLE (layer);
  GimpImage        *image       = gimp_item_get_image (GIMP_ITEM (layer));
  GimpColorConfig  *config      = image->gimp->config->color_management;
  GeglBuffer       *dest_buffer = gimp_drawable_get_buffer (drawable);
  GimpColorProfile *src_profile;
  GimpColorProfile *dest_profile;

  /*  FIXME: this is the wrong check, need something like file import
   *  conversion config
   */
  if (config->mode == GIMP_COLOR_MANAGEMENT_OFF)
    {
      gegl_buffer_copy (src_buffer, NULL, GEGL_ABYSS_NONE, dest_buffer, NULL);
      return TRUE;
    }

  src_profile = gimp_color_profile_new_from_icc_profile (icc_data, icc_length,
                                                         error);
  if (! src_profile)
    {
      gegl_buffer_copy (src_buffer, NULL, GEGL_ABYSS_NONE, dest_buffer, NULL);
      return FALSE;
    }

  dest_profile = gimp_color_managed_get_color_profile (GIMP_COLOR_MANAGED (image));

  gimp_gegl_convert_color_profile (src_buffer,  NULL, src_profile,
                                   dest_buffer, NULL, dest_profile,
                                   GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL,
                                   FALSE);

  g_object_unref (src_profile);
  g_object_unref (dest_profile);

  return TRUE;
}
