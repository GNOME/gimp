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

#include "base/tile-manager.h"

#include "gimp-gegl-utils.h"
#include "gimptilebackendtilemanager.h"


/**
 * gimp_bpp_to_babl_format:
 * @bpp: bytes per pixel
 * @linear: whether the pixels are linear or gamma-corrected.
 *
 * Return the Babl format to use for a given number of bytes per pixel.
 * This function assumes that the data is 8bit.
 *
 * Return value: the Babl format to use
 **/
const Babl *
gimp_bpp_to_babl_format (guint    bpp,
                         gboolean linear)
{
  g_return_val_if_fail (bpp > 0 && bpp <= 4, NULL);

  if (linear)
    {
      switch (bpp)
        {
        case 1:
          return babl_format ("Y u8");
        case 2:
          return babl_format ("YA u8");
        case 3:
          return babl_format ("RGB u8");
        case 4:
          return babl_format ("RGBA u8");
        }
    }
  else
    {
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
    }

  return NULL;
}

const gchar *
gimp_layer_mode_to_gegl_operation (GimpLayerModeEffects mode)
{
  switch (mode)
    {
    case GIMP_NORMAL_MODE:        return "gegl:over";
    case GIMP_DISSOLVE_MODE:      return "gimp:dissolve-mode";
    case GIMP_BEHIND_MODE:        return "gimp:behind-mode";
    case GIMP_MULTIPLY_MODE:      return "gimp:multiply-mode";
    case GIMP_SCREEN_MODE:        return "gimp:screen-mode";
    case GIMP_OVERLAY_MODE:       return "gimp:overlay-mode";
    case GIMP_DIFFERENCE_MODE:    return "gimp:difference-mode";
    case GIMP_ADDITION_MODE:      return "gimp:addition-mode";
    case GIMP_SUBTRACT_MODE:      return "gimp:subtract-mode";
    case GIMP_DARKEN_ONLY_MODE:   return "gimp:darken-mode";
    case GIMP_LIGHTEN_ONLY_MODE:  return "gimp:lighten-mode";
    case GIMP_HUE_MODE:           return "gimp:hue-mode";
    case GIMP_SATURATION_MODE:    return "gimp:saturation-mode";
    case GIMP_COLOR_MODE:         return "gimp:color-mode";
    case GIMP_VALUE_MODE:         return "gimp:value-mode";
    case GIMP_DIVIDE_MODE:        return "gimp:divide-mode";
    case GIMP_DODGE_MODE:         return "gimp:dodge-mode";
    case GIMP_BURN_MODE:          return "gimp:burn-mode";
    case GIMP_HARDLIGHT_MODE:     return "gimp:hardlight-mode";
    case GIMP_SOFTLIGHT_MODE:     return "gimp:softlight-mode";
    case GIMP_GRAIN_EXTRACT_MODE: return "gimp:grain-extract-mode";
    case GIMP_GRAIN_MERGE_MODE:   return "gimp:grain-merge-mode";
    case GIMP_COLOR_ERASE_MODE:   return "gimp:color-erase-mode";
    case GIMP_ERASE_MODE:         return "gimp:erase-mode";
    case GIMP_REPLACE_MODE:       return "gimp:replace-mode";
    case GIMP_ANTI_ERASE_MODE:    return "gimp:anti-erase-mode";
    default:
      break;
    }

  return "gegl:over";
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
  GeglRectangle rect = { 0, };

  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);

  width     = gdk_pixbuf_get_width (pixbuf);
  height    = gdk_pixbuf_get_height (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  channels  = gdk_pixbuf_get_n_channels (pixbuf);

  rect.width = width;
  rect.height = height;

  return gegl_buffer_linear_new_from_data (gdk_pixbuf_get_pixels (pixbuf),
                                           gimp_bpp_to_babl_format (channels,
                                                                    TRUE),
                                           &rect, rowstride,
                                           (GDestroyNotify) g_object_unref,
                                           pixbuf);
}

GeglBuffer *
gimp_tile_manager_create_buffer (TileManager *tm,
                                 const Babl  *format,
                                 gboolean     write)
{
  GeglTileBackend *backend;
  GeglBuffer      *buffer;

  backend = gimp_tile_backend_tile_manager_new (tm, format, write);
  buffer = gegl_buffer_new_for_backend (NULL, backend);
  g_object_unref (backend);

  return buffer;
}

void
gimp_gegl_buffer_refetch_tiles (GeglBuffer *buffer)
{
  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  gegl_tile_source_reinit (GEGL_TILE_SOURCE (buffer));
}

void
gimp_gegl_color_set_rgba (GeglColor     *color,
                          const GimpRGB *rgb)
{
  g_return_if_fail (GEGL_IS_COLOR (color));
  g_return_if_fail (rgb != NULL);

  gegl_color_set_rgba (color, rgb->r, rgb->g, rgb->b, rgb->a);
}
