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

#define GIMP_DISABLE_DEPRECATION_WARNINGS

#include "gimp.h"

#include "gimptilebackendplugin.h"


guchar *
gimp_drawable_get_thumbnail_data (gint32  drawable_ID,
                                  gint   *width,
                                  gint   *height,
                                  gint   *bpp)
{
  gint    ret_width;
  gint    ret_height;
  guchar *image_data;
  gint    data_size;

  _gimp_drawable_thumbnail (drawable_ID,
                            *width,
                            *height,
                            &ret_width,
                            &ret_height,
                            bpp,
                            &data_size,
                            &image_data);

  *width  = ret_width;
  *height = ret_height;

  return image_data;
}

guchar *
gimp_drawable_get_sub_thumbnail_data (gint32  drawable_ID,
                                      gint    src_x,
                                      gint    src_y,
                                      gint    src_width,
                                      gint    src_height,
                                      gint   *dest_width,
                                      gint   *dest_height,
                                      gint   *bpp)
{
  gint    ret_width;
  gint    ret_height;
  guchar *image_data;
  gint    data_size;

  _gimp_drawable_sub_thumbnail (drawable_ID,
                                src_x, src_y,
                                src_width, src_height,
                                *dest_width,
                                *dest_height,
                                &ret_width,
                                &ret_height,
                                bpp,
                                &data_size,
                                &image_data);

  *dest_width  = ret_width;
  *dest_height = ret_height;

  return image_data;
}

/**
 * gimp_drawable_get_buffer:
 * @drawable_ID: the ID of the #GimpDrawable to get the buffer for.
 *
 * Returns a #GeglBuffer of a specified drawable. The buffer can be used
 * like any other GEGL buffer. Its data will we synced back with the core
 * drawable when the buffer gets destroyed, or when gegl_buffer_flush()
 * is called.
 *
 * Return value: The #GeglBuffer.
 *
 * See Also: gimp_drawable_get_shadow_buffer()
 *
 * Since: 2.10
 */
GeglBuffer *
gimp_drawable_get_buffer (gint32 drawable_ID)
{
  if (gimp_item_is_valid (drawable_ID))
    {
      GeglTileBackend *backend;
      GeglBuffer      *buffer;

      backend = _gimp_tile_backend_plugin_new (drawable_ID, FALSE);
      buffer = gegl_buffer_new_for_backend (NULL, backend);
      g_object_unref (backend);

      return buffer;
    }

  return NULL;
}

/**
 * gimp_drawable_get_shadow_buffer:
 * @drawable_ID: the ID of the #GimpDrawable to get the buffer for.
 *
 * Returns a #GeglBuffer of a specified drawable's shadow tiles. The
 * buffer can be used like any other GEGL buffer. Its data will we
 * synced back with the core drawable's shadow tiles when the buffer
 * gets destroyed, or when gegl_buffer_flush() is called.
 *
 * Return value: The #GeglBuffer.
 *
 * See Also: gimp_drawable_get_shadow_buffer()
 *
 * Since: 2.10
 */
GeglBuffer *
gimp_drawable_get_shadow_buffer (gint32 drawable_ID)
{
  if (gimp_item_is_valid (drawable_ID))
    {
      GeglTileBackend *backend;
      GeglBuffer      *buffer;

      backend = _gimp_tile_backend_plugin_new (drawable_ID, TRUE);
      buffer = gegl_buffer_new_for_backend (NULL, backend);
      g_object_unref (backend);

      return buffer;
    }

  return NULL;
}

/**
 * gimp_drawable_get_format:
 * @drawable_ID: the ID of the #GimpDrawable to get the format for.
 *
 * Returns the #Babl format of the drawable.
 *
 * Return value: The #Babl format.
 *
 * Since: 2.10
 */
const Babl *
gimp_drawable_get_format (gint32 drawable_ID)
{
  const Babl *format     = NULL;
  gchar      *format_str = _gimp_drawable_get_format (drawable_ID);

  /* _gimp_drawable_get_format() only returns the encoding, so we
   * create the actual space from the image's profile
   */

  if (format_str)
    {
      gint32      image_ID = gimp_item_get_image (drawable_ID);
      const Babl *space    = NULL;

      if (gimp_item_is_layer (drawable_ID))
        {
          GimpColorProfile *profile = gimp_image_get_color_profile (image_ID);

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

      if (gimp_drawable_is_indexed (drawable_ID))
        {
          const Babl *palette;
          const Babl *palette_alpha;
          guchar     *colormap;
          gint        n_colors;

          babl_new_palette_with_space (format_str, space,
                                       &palette, &palette_alpha);

          if (gimp_drawable_has_alpha (drawable_ID))
            format = palette_alpha;
          else
            format = palette;

          colormap = gimp_image_get_colormap (image_ID, &n_colors);

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
 * @drawable_ID: the ID of the #GimpDrawable to get the thumbnail format for.
 *
 * Returns the #Babl thumbnail format of the drawable.
 *
 * Return value: The #Babl thumbnail format.
 *
 * Since: 2.10.14
 */
const Babl *
gimp_drawable_get_thumbnail_format (gint32 drawable_ID)
{
  const Babl *format     = NULL;
  gchar      *format_str = _gimp_drawable_get_thumbnail_format (drawable_ID);

  if (format_str)
    format = babl_format (format_str);

  return format;
}
