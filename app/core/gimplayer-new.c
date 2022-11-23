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
#include "libligmaconfig/ligmaconfig.h"

#include "core-types.h"

#include "gegl/ligma-babl.h"
#include "gegl/ligma-gegl-loops.h"

#include "ligmabuffer.h"
#include "ligmaimage.h"
#include "ligmaimage-color-profile.h"
#include "ligmalayer.h"
#include "ligmalayer-new.h"


/*  local function prototypes  */

static void   ligma_layer_new_convert_buffer (LigmaLayer         *layer,
                                             GeglBuffer        *src_buffer,
                                             LigmaColorProfile  *src_profile,
                                             GError           **error);


/*  public functions  */

LigmaLayer *
ligma_layer_new (LigmaImage     *image,
                gint           width,
                gint           height,
                const Babl    *format,
                const gchar   *name,
                gdouble        opacity,
                LigmaLayerMode  mode)
{
  LigmaLayer *layer;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (width > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);
  g_return_val_if_fail (format != NULL, NULL);

  layer = LIGMA_LAYER (ligma_drawable_new (LIGMA_TYPE_LAYER,
                                         image, name,
                                         0, 0, width, height,
                                         format));

  ligma_layer_set_opacity (layer, opacity, FALSE);
  ligma_layer_set_mode (layer, mode, FALSE);

  return layer;
}

/**
 * ligma_layer_new_from_buffer:
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
 * Returns: The new layer.
 **/
LigmaLayer *
ligma_layer_new_from_buffer (LigmaBuffer    *buffer,
                            LigmaImage     *dest_image,
                            const Babl    *format,
                            const gchar   *name,
                            gdouble        opacity,
                            LigmaLayerMode  mode)
{
  g_return_val_if_fail (LIGMA_IS_BUFFER (buffer), NULL);
  g_return_val_if_fail (LIGMA_IS_IMAGE (dest_image), NULL);
  g_return_val_if_fail (format != NULL, NULL);

  return ligma_layer_new_from_gegl_buffer (ligma_buffer_get_buffer (buffer),
                                          dest_image, format,
                                          name, opacity, mode,
                                          ligma_buffer_get_color_profile (buffer));
}

/**
 * ligma_layer_new_from_gegl_buffer:
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
 * Returns: The new layer.
 **/
LigmaLayer *
ligma_layer_new_from_gegl_buffer (GeglBuffer       *buffer,
                                 LigmaImage        *dest_image,
                                 const Babl       *format,
                                 const gchar      *name,
                                 gdouble           opacity,
                                 LigmaLayerMode     mode,
                                 LigmaColorProfile *buffer_profile)
{
  LigmaLayer           *layer;
  const GeglRectangle *extent;

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);
  g_return_val_if_fail (LIGMA_IS_IMAGE (dest_image), NULL);
  g_return_val_if_fail (format != NULL, NULL);
  g_return_val_if_fail (buffer_profile == NULL ||
                        LIGMA_IS_COLOR_PROFILE (buffer_profile), NULL);

  extent = gegl_buffer_get_extent (buffer);

  /*  do *not* use the buffer's format because this function gets
   *  buffers of any format passed, and converts them
   */
  layer = ligma_layer_new (dest_image,
                          extent->width, extent->height,
                          format,
                          name, opacity, mode);

  if (extent->x != 0 || extent->y != 0)
    ligma_item_translate (LIGMA_ITEM (layer), extent->x, extent->y, FALSE);

  ligma_layer_new_convert_buffer (layer, buffer, buffer_profile, NULL);

  return layer;
}

/**
 * ligma_layer_new_from_pixbuf:
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
 * Returns: The new layer.
 **/
LigmaLayer *
ligma_layer_new_from_pixbuf (GdkPixbuf     *pixbuf,
                            LigmaImage     *dest_image,
                            const Babl    *format,
                            const gchar   *name,
                            gdouble        opacity,
                            LigmaLayerMode  mode)
{
  LigmaLayer        *layer;
  GeglBuffer       *buffer;
  guint8           *icc_data;
  gsize             icc_len;
  LigmaColorProfile *profile = NULL;

  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);
  g_return_val_if_fail (LIGMA_IS_IMAGE (dest_image), NULL);
  g_return_val_if_fail (format != NULL, NULL);

  layer = ligma_layer_new (dest_image,
                          gdk_pixbuf_get_width  (pixbuf),
                          gdk_pixbuf_get_height (pixbuf),
                          format, name, opacity, mode);

  buffer = ligma_pixbuf_create_buffer (pixbuf);

  icc_data = ligma_pixbuf_get_icc_profile (pixbuf, &icc_len);
  if (icc_data)
    {
      profile = ligma_color_profile_new_from_icc_profile (icc_data, icc_len,
                                                         NULL);
      g_free (icc_data);
    }

  ligma_layer_new_convert_buffer (layer, buffer, profile, NULL);

  if (profile)
    g_object_unref (profile);

  g_object_unref (buffer);

  return layer;
}


/*  private functions  */

static void
ligma_layer_new_convert_buffer (LigmaLayer         *layer,
                               GeglBuffer        *src_buffer,
                               LigmaColorProfile  *src_profile,
                               GError           **error)
{
  LigmaDrawable     *drawable    = LIGMA_DRAWABLE (layer);
  GeglBuffer       *dest_buffer = ligma_drawable_get_buffer (drawable);
  LigmaColorProfile *dest_profile;

  if (! src_profile)
    {
      const Babl *src_format = gegl_buffer_get_format (src_buffer);

      src_profile = ligma_babl_format_get_color_profile (src_format);
    }
  else
    {
      g_object_ref (src_profile);
    }

  dest_profile =
    ligma_color_managed_get_color_profile (LIGMA_COLOR_MANAGED (layer));

  ligma_gegl_convert_color_profile (src_buffer,  NULL, src_profile,
                                   dest_buffer, NULL, dest_profile,
                                   LIGMA_COLOR_RENDERING_INTENT_PERCEPTUAL,
                                   TRUE, NULL);

  g_object_unref (src_profile);
}
