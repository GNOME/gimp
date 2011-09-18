/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpchannel-project.h
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
#include <gegl.h>

#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "gimpchannel.h"
#include "gimpchannel-project.h"


void
gimp_channel_project_region (GimpDrawable *drawable,
                             gint          x,
                             gint          y,
                             gint          width,
                             gint          height,
                             PixelRegion  *projPR,
                             gboolean      combine)
{
  GimpChannel *channel = GIMP_CHANNEL (drawable);
  PixelRegion  srcPR;
  TileManager *temp_tiles;
  guchar       col[3];
  guchar       opacity;

  gimp_rgba_get_uchar (&channel->color,
                       &col[0], &col[1], &col[2], &opacity);

  gimp_drawable_init_src_region (drawable, &srcPR,
                                 x, y, width, height,
                                 &temp_tiles);

  if (combine)
    {
      combine_regions (projPR, &srcPR, projPR, NULL, col,
                       opacity,
                       GIMP_NORMAL_MODE,
                       NULL,
                       (gimp_channel_get_show_masked (channel) ?
                        COMBINE_INTEN_A_CHANNEL_MASK :
                        COMBINE_INTEN_A_CHANNEL_SELECTION));
    }
  else
    {
      initial_region (&srcPR, projPR, NULL, col,
                      opacity,
                      GIMP_NORMAL_MODE,
                      NULL,
                      (gimp_channel_get_show_masked (channel) ?
                       INITIAL_CHANNEL_MASK :
                       INITIAL_CHANNEL_SELECTION));
    }

  if (temp_tiles)
    tile_manager_unref (temp_tiles);
}
