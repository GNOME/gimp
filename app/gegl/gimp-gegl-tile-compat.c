/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-gegl-tile-compat.h
 * Copyright (C) 2012 Michael Natterer <mitch@gimp.org>
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

#include <gegl.h>

#include "gimp-gegl-types.h"

#include "gimp-gegl-tile-compat.h"


gint
gimp_gegl_buffer_get_n_tile_rows (GeglBuffer *buffer,
                                  gint        tile_height)
{
  return (gegl_buffer_get_height (buffer) + tile_height - 1) / tile_height;
}

gint
gimp_gegl_buffer_get_n_tile_cols (GeglBuffer *buffer,
                                  gint        tile_width)
{
  return (gegl_buffer_get_width (buffer) + tile_width - 1) / tile_width;
}

gboolean
gimp_gegl_buffer_get_tile_rect (GeglBuffer    *buffer,
                                gint           tile_width,
                                gint           tile_height,
                                gint           tile_num,
                                GeglRectangle *rect)
{
  gint n_tile_rows;
  gint n_tile_columns;
  gint tile_row;
  gint tile_column;

  n_tile_rows    = gimp_gegl_buffer_get_n_tile_rows (buffer, tile_height);
  n_tile_columns = gimp_gegl_buffer_get_n_tile_cols (buffer, tile_width);

  if (tile_num > n_tile_rows * n_tile_columns - 1)
    return FALSE;

  tile_row    = tile_num / n_tile_columns;
  tile_column = tile_num % n_tile_columns;

  rect->x = tile_column * tile_width;
  rect->y = tile_row    * tile_height;

  if (tile_column == n_tile_columns - 1)
    rect->width = gegl_buffer_get_width (buffer) - rect->x;
  else
    rect->width = tile_width;

  if (tile_row == n_tile_rows - 1)
    rect->height = gegl_buffer_get_height (buffer) - rect->y;
  else
    rect->height = tile_height;

  return TRUE;
}
