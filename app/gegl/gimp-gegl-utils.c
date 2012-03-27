/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-gegl-utils.h
 * Copyright (C) 2007 Michael Natterer <mitch@gimp.org>
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

#include <gegl.h>

#include "gimp-gegl-types.h"

#include "base/temp-buf.h"
#include "base/tile-manager.h"

#include "core/gimpprogress.h"

#include "gimp-gegl-utils.h"
#include "gimptilebackendtilemanager.h"


/**
 * gimp_bpp_to_babl_format:
 * @bpp: bytes per pixel
 *
 * Return the Babl format to use for a given number of bytes per pixel.
 * This function assumes that the data is 8bit.
 *
 * Return value: the Babl format to use
 **/
const Babl *
gimp_bpp_to_babl_format (guint bpp)
{
  g_return_val_if_fail (bpp > 0 && bpp <= 4, NULL);

  switch (bpp)
    {
    case 1:
      return babl_format ("Y' u8");
    case 2:
      return babl_format ("Y'A u8");
    case 3:
      return babl_format ("R'G'B' u8");
    case 4:
      return babl_format ("R'G'B'A u8");
    }

  return NULL;
}

/**
 * gimp_bpp_to_babl_format_with_alpha:
 * @bpp: bytes per pixel
 *
 * Return the Babl format to use for a given number of bytes per pixel.
 * This function assumes that the data is 8bit.
 *
 * Return value: the Babl format to use
 **/
const Babl *
gimp_bpp_to_babl_format_with_alpha (guint bpp)
{
  g_return_val_if_fail (bpp > 0 && bpp <= 4, NULL);

  switch (bpp)
    {
      case 1:
      case 2:
        return babl_format ("Y'A u8");
      case 3:
      case 4:
        return babl_format ("R'G'B'A u8");
    }

  return NULL;
}

const gchar *
gimp_interpolation_to_gegl_filter (GimpInterpolationType interpolation)
{
  switch (interpolation)
    {
    case GIMP_INTERPOLATION_NONE:    return "nearest";
    case GIMP_INTERPOLATION_LINEAR:  return "linear";
    case GIMP_INTERPOLATION_CUBIC:   return "cubic";
    case GIMP_INTERPOLATION_LANCZOS: return "lohalo";
    default:
      break;
    }

  return "nearest";
}

GeglBuffer *
gimp_pixbuf_create_buffer (GdkPixbuf *pixbuf)
{
  gint          width;
  gint          height;
  gint          rowstride;
  gint          channels;

  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);

  width     = gdk_pixbuf_get_width (pixbuf);
  height    = gdk_pixbuf_get_height (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  channels  = gdk_pixbuf_get_n_channels (pixbuf);

  return gegl_buffer_linear_new_from_data (gdk_pixbuf_get_pixels (pixbuf),
                                           gimp_bpp_to_babl_format (channels),
                                           GIMP_GEGL_RECT (0,0,width,height),
                                           rowstride,
                                           (GDestroyNotify) g_object_unref,
                                           g_object_ref (pixbuf));
}

GeglBuffer *
gimp_gegl_buffer_new (const GeglRectangle *rect,
                      const Babl          *format)
{
  TileManager *tiles;
  GeglBuffer  *buffer;

  g_return_val_if_fail (rect != NULL, NULL);
  g_return_val_if_fail (format != NULL, NULL);

  tiles = tile_manager_new (rect->width, rect->height,
                            babl_format_get_bytes_per_pixel (format));
  buffer = gimp_tile_manager_create_buffer (tiles, format);
  tile_manager_unref (tiles);

  return buffer;
}

GeglBuffer *
gimp_gegl_buffer_dup (GeglBuffer *buffer)
{
  const Babl  *format = gegl_buffer_get_format (buffer);
  TileManager *tiles;
  GeglBuffer  *dup;

  tiles = tile_manager_new (gegl_buffer_get_width (buffer),
                            gegl_buffer_get_height (buffer),
                            babl_format_get_bytes_per_pixel (format));

  dup = gimp_tile_manager_create_buffer (tiles, format);
  tile_manager_unref (tiles);

  gegl_buffer_copy (buffer, NULL, dup, NULL);

  return dup;
}

GeglBuffer *
gimp_tile_manager_create_buffer (TileManager *tm,
                                 const Babl  *format)
{
  GeglTileBackend *backend;
  GeglBuffer      *buffer;

  backend = gimp_tile_backend_tile_manager_new (tm, format);
  buffer = gegl_buffer_new_for_backend (NULL, backend);
  g_object_unref (backend);

  return buffer;
}

/* temp hack */
GeglTileBackend * gegl_buffer_backend (GeglBuffer *buffer);

TileManager *
gimp_gegl_buffer_get_tiles (GeglBuffer *buffer)
{
  GeglTileBackend *backend;

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);

  backend = gegl_buffer_backend (buffer);

  g_return_val_if_fail (GIMP_IS_TILE_BACKEND_TILE_MANAGER (backend), NULL);

  gegl_buffer_flush (buffer);

  return gimp_tile_backend_tile_manager_get_tiles (backend);
}

GeglBuffer  *
gimp_temp_buf_create_buffer (TempBuf    *temp_buf,
                             const Babl *format)
{
  TempBuf    *new;
  GeglBuffer *buffer;
  gint        width, height;

  g_return_val_if_fail (temp_buf != NULL, NULL);
  g_return_val_if_fail (format != NULL, NULL);
  g_return_val_if_fail (babl_format_get_bytes_per_pixel (format) ==
                        temp_buf->bytes, NULL);

  width  = temp_buf->width;
  height = temp_buf->height;

  new = temp_buf_copy (temp_buf, NULL);

  buffer = gegl_buffer_linear_new_from_data (temp_buf_get_data (new),
                                             format,
                                             GIMP_GEGL_RECT (0, 0, width, height),
                                             width * new->bytes,
                                             (GDestroyNotify) temp_buf_free, new);

  g_object_set_data (G_OBJECT (buffer), "gimp-temp-buf", new);

  return buffer;
}

TempBuf *
gimp_gegl_buffer_get_temp_buf (GeglBuffer *buffer)
{
  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);

  return g_object_get_data (G_OBJECT (buffer), "gimp-temp-buf");
}

void
gimp_gegl_buffer_refetch_tiles (GeglBuffer *buffer)
{
  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  gegl_tile_source_reinit (GEGL_TILE_SOURCE (buffer));
}

GeglColor *
gimp_gegl_color_new (const GimpRGB *rgb)
{
  GeglColor *color;

  g_return_val_if_fail (rgb != NULL, NULL);

  color = gegl_color_new (NULL);
  gegl_color_set_rgba (color, rgb->r, rgb->g, rgb->b, rgb->a);

  return color;
}

static void
gimp_gegl_progress_notify (GObject          *object,
                           const GParamSpec *pspec,
                           GimpProgress     *progress)
{
  const gchar *text;
  gdouble      value;

  g_object_get (object, "progress", &value, NULL);

  text = g_object_get_data (object, "gimp-progress-text");

  if (text)
    {
      if (value == 0.0)
        {
          gimp_progress_start (progress, text, FALSE);
          return;
        }
      else if (value == 1.0)
        {
          gimp_progress_end (progress);
          return;
        }
    }

  gimp_progress_set_value (progress, value);
}

void
gimp_gegl_progress_connect (GeglNode     *node,
                            GimpProgress *progress,
                            const gchar  *text)
{
  GObject *operation = NULL;

  g_return_if_fail (GEGL_IS_NODE (node));
  g_return_if_fail (GIMP_IS_PROGRESS (progress));

  g_object_get (node, "gegl-operation", &operation, NULL);

  g_return_if_fail (operation != NULL);

  g_signal_connect (operation, "notify::progress",
                    G_CALLBACK (gimp_gegl_progress_notify),
                    progress);

  if (text)
    g_object_set_data_full (operation,
                            "gimp-progress-text", g_strdup (text),
                            (GDestroyNotify) g_free);

  g_object_unref (operation);
}
