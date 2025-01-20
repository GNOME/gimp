/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimplayer.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include "gimp.h"



G_DEFINE_TYPE (GimpLayer, gimp_layer, GIMP_TYPE_DRAWABLE)

#define parent_class gimp_layer_parent_class


static void
gimp_layer_class_init (GimpLayerClass *klass)
{
}

static void
gimp_layer_init (GimpLayer *layer)
{
}


/* Public API. */

/**
 * gimp_layer_get_by_id:
 * @layer_id: The layer id.
 *
 * Returns a #GimpLayer representing @layer_id. This function calls
 * gimp_item_get_by_id() and returns the item if it is layer or %NULL
 * otherwise.
 *
 * Returns: (nullable) (transfer none): a #GimpLayer for @layer_id or
 *          %NULL if @layer_id does not represent a valid layer. The
 *          object belongs to libgimp and you must not modify or unref
 *          it.
 *
 * Since: 3.0
 **/
GimpLayer *
gimp_layer_get_by_id (gint32 layer_id)
{
  GimpItem *item = gimp_item_get_by_id (layer_id);

  if (GIMP_IS_LAYER (item))
    return (GimpLayer *) item;

  return NULL;
}

/**
 * gimp_layer_new_from_pixbuf:
 * @image:          The RGB image to which to add the layer.
 * @name:           The layer name.
 * @pixbuf:         A GdkPixbuf.
 * @opacity:        The layer opacity.
 * @mode:           The layer combination mode.
 * @progress_start: start of progress
 * @progress_end:   end of progress
 *
 * Create a new layer from a %GdkPixbuf.
 *
 * This procedure creates a new layer from the given %GdkPixbuf.  The
 * image has to be an RGB image and just like with gimp_layer_new()
 * you will still need to add the layer to it.
 *
 * If you pass @progress_end > @progress_start to this function,
 * gimp_progress_update() will be called for. You have to call
 * gimp_progress_init() beforehand then.
 *
 * Returns: (transfer none): The newly created layer.
 *          The object belongs to libgimp and you should not free it.
 *
 * Since: 2.2
 */
GimpLayer *
gimp_layer_new_from_pixbuf (GimpImage     *image,
                            const gchar   *name,
                            GdkPixbuf     *pixbuf,
                            gdouble        opacity,
                            GimpLayerMode  mode,
                            gdouble        progress_start,
                            gdouble        progress_end)
{
  GeglBuffer *dest_buffer;
  GimpLayer  *layer;
  gint        width;
  gint        height;
  gint        bpp;
  gdouble     range = progress_end - progress_start;

  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);

  if (gimp_image_get_base_type (image) != GIMP_RGB)
    {
      g_warning ("gimp_layer_new_from_pixbuf() needs an RGB image");
      return NULL;
    }

  if (gdk_pixbuf_get_colorspace (pixbuf) != GDK_COLORSPACE_RGB)
    {
      g_warning ("gimp_layer_new_from_pixbuf() assumes that GdkPixbuf is RGB");
      return NULL;
    }

  width  = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);
  bpp    = gdk_pixbuf_get_n_channels (pixbuf);

  layer = gimp_layer_new (image, name, width, height,
                          bpp == 3 ? GIMP_RGB_IMAGE : GIMP_RGBA_IMAGE,
                          opacity, mode);

  if (! layer)
    return NULL;

  dest_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  gegl_buffer_set (dest_buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                   gimp_pixbuf_get_format (pixbuf),
                   gdk_pixbuf_get_pixels (pixbuf),
                   gdk_pixbuf_get_rowstride (pixbuf));

  g_object_unref (dest_buffer);

  if (range > 0.0)
    gimp_progress_update (progress_end);

  return layer;
}

/**
 * gimp_layer_new_from_surface:
 * @image:           The RGB image to which to add the layer.
 * @name:            The layer name.
 * @surface:         A Cairo image surface.
 * @progress_start:  start of progress
 * @progress_end:    end of progress
 *
 * Create a new layer from a [type@cairo.Surface].
 *
 * This procedure creates a new layer from the given
 * [type@cairo.Surface]. The image has to be an RGB image and just like
 * with gimp_layer_new() you will still need to add the layer to it.
 *
 * If you pass @progress_end > @progress_start to this function,
 * gimp_progress_update() will be called for. You have to call
 * gimp_progress_init() beforehand then.
 *
 * Returns: (transfer none): The newly created layer.
 *          The object belongs to libgimp and you should not free it.
 *
 * Since: 2.8
 */
GimpLayer *
gimp_layer_new_from_surface (GimpImage            *image,
                             const gchar          *name,
                             cairo_surface_t      *surface,
                             gdouble               progress_start,
                             gdouble               progress_end)
{
  GeglBuffer    *src_buffer;
  GeglBuffer    *dest_buffer;
  GimpLayer     *layer;
  gint           width;
  gint           height;
  cairo_format_t format;
  gdouble        range = progress_end - progress_start;

  g_return_val_if_fail (surface != NULL, NULL);
  g_return_val_if_fail (cairo_surface_get_type (surface) ==
                        CAIRO_SURFACE_TYPE_IMAGE, NULL);

  if (gimp_image_get_base_type (image) != GIMP_RGB)
    {
      g_warning ("gimp_layer_new_from_surface() needs an RGB image");
      return NULL;
    }

  width  = cairo_image_surface_get_width (surface);
  height = cairo_image_surface_get_height (surface);
  format = cairo_image_surface_get_format (surface);

  if (format != CAIRO_FORMAT_ARGB32 &&
      format != CAIRO_FORMAT_RGB24)
    {
      g_warning ("gimp_layer_new_from_surface() assumes that surface is RGB");
      return NULL;
    }

  layer = gimp_layer_new (image, name, width, height,
                          format == CAIRO_FORMAT_RGB24 ?
                          GIMP_RGB_IMAGE : GIMP_RGBA_IMAGE,
                          100.0,
                          gimp_image_get_default_new_layer_mode (image));

  if (layer == NULL)
    return NULL;

  src_buffer = gimp_cairo_surface_create_buffer (surface, NULL);
  dest_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  gegl_buffer_copy (src_buffer, NULL, GEGL_ABYSS_NONE,
                    dest_buffer, NULL);

  g_object_unref (src_buffer);
  g_object_unref (dest_buffer);

  if (range > 0.0)
    gimp_progress_update (progress_end);

  return layer;
}
