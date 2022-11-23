/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * ligmalayer.c
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

#include "ligma.h"


static LigmaLayer * ligma_layer_real_copy (LigmaLayer *layer);


G_DEFINE_TYPE (LigmaLayer, ligma_layer, LIGMA_TYPE_DRAWABLE)

#define parent_class ligma_layer_parent_class


static void
ligma_layer_class_init (LigmaLayerClass *klass)
{
  klass->copy = ligma_layer_real_copy;
}

static void
ligma_layer_init (LigmaLayer *layer)
{
}


/* Public API. */

/**
 * ligma_layer_get_by_id:
 * @layer_id: The layer id.
 *
 * Returns a #LigmaLayer representing @layer_id. This function calls
 * ligma_item_get_by_id() and returns the item if it is layer or %NULL
 * otherwise.
 *
 * Returns: (nullable) (transfer none): a #LigmaLayer for @layer_id or
 *          %NULL if @layer_id does not represent a valid layer. The
 *          object belongs to libligma and you must not modify or unref
 *          it.
 *
 * Since: 3.0
 **/
LigmaLayer *
ligma_layer_get_by_id (gint32 layer_id)
{
  LigmaItem *item = ligma_item_get_by_id (layer_id);

  if (LIGMA_IS_LAYER (item))
    return (LigmaLayer *) item;

  return NULL;
}

/**
 * ligma_layer_new:
 * @image:   The image to which to add the layer.
 * @name:    The layer name.
 * @width:   The layer width.
 * @height:  The layer height.
 * @type:    The layer type.
 * @opacity: The layer opacity.
 * @mode:    The layer combination mode.
 *
 * Create a new layer.
 *
 * This procedure creates a new layer with the specified width, height,
 * and type. Name, opacity, and mode are also supplied parameters. The
 * new layer still needs to be added to the image, as this is not
 * automatic. Add the new layer with the ligma_image_insert_layer()
 * command. Other attributes such as layer mask modes, and offsets
 * should be set with explicit procedure calls.
 *
 * Returns: (transfer none): The newly created layer.
 *          The object belongs to libligma and you should not free it.
 *
 * Since: 3.0
 */
LigmaLayer *
ligma_layer_new (LigmaImage     *image,
                const gchar   *name,
                gint           width,
                gint           height,
                LigmaImageType  type,
                gdouble        opacity,
                LigmaLayerMode  mode)
{
  return _ligma_layer_new (image,
                          width,
                          height,
                          type,
                          name,
                          opacity,
                          mode);
}

/**
 * ligma_layer_copy:
 * @layer: The layer to copy.
 *
 * Copy a layer.
 *
 * This procedure copies the specified layer and returns the copy. The
 * newly copied layer is for use within the original layer's image. It
 * should not be subsequently added to any other image.
 *
 * Returns: (transfer none): The newly copied layer.
 *          The object belongs to libligma and you should not free it.
 *
 * Since: 3.0
 */
LigmaLayer *
ligma_layer_copy (LigmaLayer *layer)
{
  return LIGMA_LAYER_GET_CLASS (layer)->copy (layer);
}

/**
 * ligma_layer_new_from_pixbuf:
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
 * image has to be an RGB image and just like with ligma_layer_new()
 * you will still need to add the layer to it.
 *
 * If you pass @progress_end > @progress_start to this function,
 * ligma_progress_update() will be called for. You have to call
 * ligma_progress_init() beforehand then.
 *
 * Returns: (transfer none): The newly created layer.
 *          The object belongs to libligma and you should not free it.
 *
 * Since: 2.2
 */
LigmaLayer *
ligma_layer_new_from_pixbuf (LigmaImage     *image,
                            const gchar   *name,
                            GdkPixbuf     *pixbuf,
                            gdouble        opacity,
                            LigmaLayerMode  mode,
                            gdouble        progress_start,
                            gdouble        progress_end)
{
  GeglBuffer *dest_buffer;
  LigmaLayer  *layer;
  gint        width;
  gint        height;
  gint        bpp;
  gdouble     range = progress_end - progress_start;

  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);

  if (ligma_image_get_base_type (image) != LIGMA_RGB)
    {
      g_warning ("ligma_layer_new_from_pixbuf() needs an RGB image");
      return NULL;
    }

  if (gdk_pixbuf_get_colorspace (pixbuf) != GDK_COLORSPACE_RGB)
    {
      g_warning ("ligma_layer_new_from_pixbuf() assumes that GdkPixbuf is RGB");
      return NULL;
    }

  width  = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);
  bpp    = gdk_pixbuf_get_n_channels (pixbuf);

  layer = ligma_layer_new (image, name, width, height,
                          bpp == 3 ? LIGMA_RGB_IMAGE : LIGMA_RGBA_IMAGE,
                          opacity, mode);

  if (! layer)
    return NULL;

  dest_buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (layer));

  gegl_buffer_set (dest_buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                   ligma_pixbuf_get_format (pixbuf),
                   gdk_pixbuf_get_pixels (pixbuf),
                   gdk_pixbuf_get_rowstride (pixbuf));

  g_object_unref (dest_buffer);

  if (range > 0.0)
    ligma_progress_update (progress_end);

  return layer;
}

/**
 * ligma_layer_new_from_surface:
 * @image:           The RGB image to which to add the layer.
 * @name:            The layer name.
 * @surface:         A Cairo image surface.
 * @progress_start:  start of progress
 * @progress_end:    end of progress
 *
 * Create a new layer from a #cairo_surface_t.
 *
 * This procedure creates a new layer from the given
 * #cairo_surface_t. The image has to be an RGB image and just like
 * with ligma_layer_new() you will still need to add the layer to it.
 *
 * If you pass @progress_end > @progress_start to this function,
 * ligma_progress_update() will be called for. You have to call
 * ligma_progress_init() beforehand then.
 *
 * Returns: (transfer none): The newly created layer.
 *          The object belongs to libligma and you should not free it.
 *
 * Since: 2.8
 */
LigmaLayer *
ligma_layer_new_from_surface (LigmaImage            *image,
                             const gchar          *name,
                             cairo_surface_t      *surface,
                             gdouble               progress_start,
                             gdouble               progress_end)
{
  GeglBuffer    *src_buffer;
  GeglBuffer    *dest_buffer;
  LigmaLayer     *layer;
  gint           width;
  gint           height;
  cairo_format_t format;
  gdouble        range = progress_end - progress_start;

  g_return_val_if_fail (surface != NULL, NULL);
  g_return_val_if_fail (cairo_surface_get_type (surface) ==
                        CAIRO_SURFACE_TYPE_IMAGE, NULL);

  if (ligma_image_get_base_type (image) != LIGMA_RGB)
    {
      g_warning ("ligma_layer_new_from_surface() needs an RGB image");
      return NULL;
    }

  width  = cairo_image_surface_get_width (surface);
  height = cairo_image_surface_get_height (surface);
  format = cairo_image_surface_get_format (surface);

  if (format != CAIRO_FORMAT_ARGB32 &&
      format != CAIRO_FORMAT_RGB24)
    {
      g_warning ("ligma_layer_new_from_surface() assumes that surface is RGB");
      return NULL;
    }

  layer = ligma_layer_new (image, name, width, height,
                          format == CAIRO_FORMAT_RGB24 ?
                          LIGMA_RGB_IMAGE : LIGMA_RGBA_IMAGE,
                          100.0,
                          ligma_image_get_default_new_layer_mode (image));

  if (layer == NULL)
    return NULL;

  src_buffer = ligma_cairo_surface_create_buffer (surface);
  dest_buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (layer));

  gegl_buffer_copy (src_buffer, NULL, GEGL_ABYSS_NONE,
                    dest_buffer, NULL);

  g_object_unref (src_buffer);
  g_object_unref (dest_buffer);

  if (range > 0.0)
    ligma_progress_update (progress_end);

  return layer;
}


/*  private functions  */

static LigmaLayer *
ligma_layer_real_copy (LigmaLayer *layer)
{
  return _ligma_layer_copy (layer, FALSE);
}
