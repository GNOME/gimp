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

static gint
gimp_babl_format_to_legacy_bpp (const Babl *format)
{
  return babl_format_get_n_components (format);
}

TileManager *
gimp_buffer_to_tiles (GeglBuffer *buffer)
{
  const Babl    *format     = gegl_buffer_get_format (buffer);
  TileManager   *new_tiles  = NULL;
  GeglNode      *source     = NULL;
  GeglNode      *sink       = NULL;

  g_return_val_if_fail (buffer != NULL, NULL);

  /* Setup and process the graph */
  new_tiles = tile_manager_new (gegl_buffer_get_width (buffer),
                                gegl_buffer_get_height (buffer),
                                gimp_babl_format_to_legacy_bpp (format));
  source = gegl_node_new_child (NULL,
                                "operation", "gegl:buffer-source",
                                "buffer",    buffer,
                                NULL);
  sink = gegl_node_new_child (NULL,
                              "operation",    "gimp:tilemanager-sink",
                              "tile-manager", new_tiles,
                              NULL);
  gegl_node_link_many (source, sink, NULL);
  gegl_node_process (sink);

  /* Clenaup */
  g_object_unref (sink);
  g_object_unref (source);

  return new_tiles;
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
