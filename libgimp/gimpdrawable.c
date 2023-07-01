/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpdrawable.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gimp.h"

#include "gimppixbuf.h"
#include "gimptilebackendplugin.h"


G_DEFINE_ABSTRACT_TYPE (GimpDrawable, gimp_drawable, GIMP_TYPE_ITEM)

#define parent_class gimp_drawable_parent_class


static void
gimp_drawable_class_init (GimpDrawableClass *klass)
{
}

static void
gimp_drawable_init (GimpDrawable *drawable)
{
}


/* Public API. */

/**
 * gimp_drawable_get_by_id:
 * @drawable_id: The drawable id.
 *
 * Returns a #GimpDrawable representing @drawable_id. This function
 * calls gimp_item_get_by_id() and returns the item if it is drawable
 * or %NULL otherwise.
 *
 * Returns: (nullable) (transfer none): a #GimpDrawable for
 *          @drawable_id or %NULL if @drawable_id does not represent a
 *          valid drawable. The object belongs to libgimp and you must
 *          not modify or unref it.
 *
 * Since: 3.0
 **/
GimpDrawable *
gimp_drawable_get_by_id (gint32 drawable_id)
{
  GimpItem *item = gimp_item_get_by_id (drawable_id);

  if (GIMP_IS_DRAWABLE (item))
    return (GimpDrawable *) item;

  return NULL;
}

/**
 * gimp_drawable_get_thumbnail_data:
 * @drawable: the drawable
 * @width:    the requested thumbnail width  (<= 1024 pixels)
 * @height:   the requested thumbnail height (<= 1024 pixels)
 * @actual_width: (out): the resulting thumbnail's actual width
 * @actual_height: (out): the resulting thumbnail's actual height
 * @bpp: (out): the bytes per pixel of the returned thubmnail data
 *
 * Retrieves thumbnail data for the drawable identified by @drawable.
 * The thumbnail will be not larger than the requested size.
 *
 * Returns: (transfer full) (nullable): thumbnail data or %NULL if
 *          @drawable is invalid.
 **/
GBytes *
gimp_drawable_get_thumbnail_data (GimpDrawable *drawable,
                                  gint          width,
                                  gint          height,
                                  gint         *actual_width,
                                  gint         *actual_height,
                                  gint         *bpp)
{
  GBytes *image_data;

  g_return_val_if_fail (actual_width != NULL, NULL);
  g_return_val_if_fail (actual_height != NULL, NULL);
  g_return_val_if_fail (bpp != NULL, NULL);

  _gimp_drawable_thumbnail (drawable,
                            width,
                            height,
                            actual_width,
                            actual_height,
                            bpp,
                            &image_data);

  return image_data;
}

/**
 * gimp_drawable_get_thumbnail:
 * @drawable: the drawable
 * @width:    the requested thumbnail width  (<= 1024 pixels)
 * @height:   the requested thumbnail height (<= 1024 pixels)
 * @alpha:    how to handle an alpha channel
 *
 * Retrieves a thumbnail pixbuf for the drawable identified by
 * @drawable. The thumbnail will be not larger than the requested
 * size.
 *
 * Returns: (transfer full): a new #GdkPixbuf
 *
 * Since: 2.2
 **/
GdkPixbuf *
gimp_drawable_get_thumbnail (GimpDrawable           *drawable,
                             gint                    width,
                             gint                    height,
                             GimpPixbufTransparency  alpha)
{
  gint       thumb_width, thumb_height, thumb_bpp;
  GBytes    *data;
  gsize      data_size;
  GdkPixbuf *pixbuf = NULL;

  g_return_val_if_fail (width  > 0 && width  <= 1024, NULL);
  g_return_val_if_fail (height > 0 && height <= 1024, NULL);

  data = gimp_drawable_get_thumbnail_data (drawable,
                                           width,
                                           height,
                                           &thumb_width,
                                           &thumb_height,
                                           &thumb_bpp);

  if (data)
    pixbuf = _gimp_pixbuf_from_data (g_bytes_unref_to_data (data, &data_size),
                                     thumb_width, thumb_height, thumb_bpp,
                                     alpha);

  return pixbuf;
}

/**
 * gimp_drawable_get_sub_thumbnail_data:
 * @drawable:             the drawable ID
 * @src_x:                the x coordinate of the area
 * @src_y:                the y coordinate of the area
 * @src_width:            the width of the area
 * @src_height:           the height of the area
 * @dest_width:           the requested thumbnail width  (<= 1024 pixels)
 * @dest_height:          the requested thumbnail height (<= 1024 pixels)
 * @actual_width: (out):  the width of the returned thumbnail
 * @actual_height: (out): the height of the returned thumbnail
 * @bpp: (out):           the bytes per pixel of the returned thumbnail data
 *
 * Retrieves thumbnail data for the drawable identified by @drawable.
 * The thumbnail will be not larger than the requested size.
 *
 * Returns: (transfer full): thumbnail data or %NULL if
 *          @drawable is invalid.
 **/
GBytes *
gimp_drawable_get_sub_thumbnail_data (GimpDrawable *drawable,
                                      gint          src_x,
                                      gint          src_y,
                                      gint          src_width,
                                      gint          src_height,
                                      gint          dest_width,
                                      gint          dest_height,
                                      gint         *actual_width,
                                      gint         *actual_height,
                                      gint         *bpp)
{
  GBytes *image_bytes;

  _gimp_drawable_sub_thumbnail (drawable,
                                src_x, src_y,
                                src_width, src_height,
                                dest_width,
                                dest_height,
                                actual_width,
                                actual_height,
                                bpp,
                                &image_bytes);

  return image_bytes;
}

/**
 * gimp_drawable_get_sub_thumbnail:
 * @drawable:    the drawable ID
 * @src_x:       the x coordinate of the area
 * @src_y:       the y coordinate of the area
 * @src_width:   the width of the area
 * @src_height:  the height of the area
 * @dest_width:  the requested thumbnail width  (<= 1024 pixels)
 * @dest_height: the requested thumbnail height (<= 1024 pixels)
 * @alpha:       how to handle an alpha channel
 *
 * Retrieves a thumbnail pixbuf for the drawable identified by
 * @drawable. The thumbnail will be not larger than the requested
 * size.
 *
 * Returns: (transfer full): a new #GdkPixbuf
 *
 * Since: 2.2
 **/
GdkPixbuf *
gimp_drawable_get_sub_thumbnail (GimpDrawable           *drawable,
                                 gint                    src_x,
                                 gint                    src_y,
                                 gint                    src_width,
                                 gint                    src_height,
                                 gint                    dest_width,
                                 gint                    dest_height,
                                 GimpPixbufTransparency  alpha)
{
  gint       thumb_width  = dest_width;
  gint       thumb_height = dest_height;
  gint       thumb_bpp;
  GBytes    *data;
  gsize      data_size;
  GdkPixbuf *pixbuf = NULL;

  g_return_val_if_fail (src_x >= 0, NULL);
  g_return_val_if_fail (src_y >= 0, NULL);
  g_return_val_if_fail (src_width  > 0, NULL);
  g_return_val_if_fail (src_height > 0, NULL);
  g_return_val_if_fail (dest_width  > 0 && dest_width  <= 1024, NULL);
  g_return_val_if_fail (dest_height > 0 && dest_height <= 1024, NULL);

  data = gimp_drawable_get_sub_thumbnail_data (drawable,
                                               src_x, src_y,
                                               src_width, src_height,
                                               dest_width, dest_height,
                                               &thumb_width,
                                               &thumb_height,
                                               &thumb_bpp);

  if (data)
    pixbuf = _gimp_pixbuf_from_data (g_bytes_unref_to_data (data, &data_size),
                                     thumb_width, thumb_height, thumb_bpp,
                                     alpha);
  g_bytes_unref (data);

  return pixbuf;
}

/**
 * gimp_drawable_get_buffer:
 * @drawable: the ID of the #GimpDrawable to get the buffer for.
 *
 * Returns a #GeglBuffer of a specified drawable. The buffer can be used
 * like any other GEGL buffer. Its data will we synced back with the core
 * drawable when the buffer gets destroyed, or when gegl_buffer_flush()
 * is called.
 *
 * Returns: (transfer full): The #GeglBuffer.
 *
 * See Also: gimp_drawable_get_shadow_buffer()
 *
 * Since: 2.10
 */
GeglBuffer *
gimp_drawable_get_buffer (GimpDrawable *drawable)
{
  if (gimp_item_is_valid (GIMP_ITEM (drawable)))
    {
      GeglTileBackend *backend;
      GeglBuffer      *buffer;

      backend = _gimp_tile_backend_plugin_new (drawable, FALSE);
      buffer = gegl_buffer_new_for_backend (NULL, backend);
      g_object_unref (backend);

      return buffer;
    }

  return NULL;
}

/**
 * gimp_drawable_get_shadow_buffer:
 * @drawable: the ID of the #GimpDrawable to get the buffer for.
 *
 * Returns a #GeglBuffer of a specified drawable's shadow tiles. The
 * buffer can be used like any other GEGL buffer. Its data will we
 * synced back with the core drawable's shadow tiles when the buffer
 * gets destroyed, or when gegl_buffer_flush() is called.
 *
 * Returns: (transfer full): The #GeglBuffer.
 *
 * Since: 2.10
 */
GeglBuffer *
gimp_drawable_get_shadow_buffer (GimpDrawable *drawable)
{
  if (gimp_item_is_valid (GIMP_ITEM (drawable)))
    {
      GeglTileBackend *backend;
      GeglBuffer      *buffer;

      backend = _gimp_tile_backend_plugin_new (drawable, TRUE);
      buffer = gegl_buffer_new_for_backend (NULL, backend);
      g_object_unref (backend);

      return buffer;
    }

  return NULL;
}

/**
 * gimp_drawable_get_format:
 * @drawable: the ID of the #GimpDrawable to get the format for.
 *
 * Returns the #Babl format of the drawable.
 *
 * Returns: The #Babl format.
 *
 * Since: 2.10
 */
const Babl *
gimp_drawable_get_format (GimpDrawable *drawable)
{
  const Babl *format     = NULL;
  gchar      *format_str = _gimp_drawable_get_format (drawable);

  /* _gimp_drawable_get_format() only returns the encoding, so we
   * create the actual space from the image's profile
   */

  if (format_str)
    {
      const Babl *space = NULL;
      GimpImage  *image = gimp_item_get_image (GIMP_ITEM (drawable));

      if (gimp_item_is_layer (GIMP_ITEM (drawable)))
        {
          GimpColorProfile *profile = gimp_image_get_color_profile (image);

          if (profile)
            {
              GError *error = NULL;

              space = gimp_color_profile_get_space
                (profile,
                 GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                 &error);

              if (! space)
                {
                  g_printerr ("%s: failed to create Babl space from "
                              "profile: %s\n",
                              G_STRFUNC, error->message);
                  g_clear_error (&error);
                }

              g_object_unref (profile);
            }
        }

      if (gimp_drawable_is_indexed (drawable))
        {
          const Babl *palette;
          const Babl *palette_alpha;
          guchar     *colormap;
          gint        colormap_len, n_colors;

          babl_new_palette_with_space (format_str, space,
                                       &palette, &palette_alpha);

          if (gimp_drawable_has_alpha (drawable))
            format = palette_alpha;
          else
            format = palette;

          colormap = gimp_image_get_colormap (image, &colormap_len, &n_colors);

          if (colormap)
            {
              babl_palette_set_palette (format,
                                        babl_format_with_space ("R'G'B' u8",
                                                                space),
                                        colormap, n_colors);
              g_free (colormap);
            }
        }
      else
        {
          format = babl_format_with_space (format_str, space);
        }

      g_free (format_str);
    }

  return format;
}
/**
 * gimp_drawable_get_thumbnail_format:
 * @drawable: the ID of the #GimpDrawable to get the thumbnail format for.
 *
 * Returns the #Babl thumbnail format of the drawable.
 *
 * Returns: The #Babl thumbnail format.
 *
 * Since: 2.10.14
 */
const Babl *
gimp_drawable_get_thumbnail_format (GimpDrawable *drawable)
{
  const Babl *format     = NULL;
  gchar      *format_str = _gimp_drawable_get_thumbnail_format (drawable);

  if (format_str)
    {
      format = babl_format (format_str);
      g_free (format_str);
    }

  return format;
}
