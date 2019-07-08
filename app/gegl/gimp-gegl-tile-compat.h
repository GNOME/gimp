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

#ifndef __GIMP_GEGL_TILE_COMPAT_H__
#define __GIMP_GEGL_TILE_COMPAT_H__


gint       gimp_gegl_buffer_get_n_tile_rows (GeglBuffer    *buffer,
                                             gint           tile_height);
gint       gimp_gegl_buffer_get_n_tile_cols (GeglBuffer    *buffer,
                                             gint           tile_width);
gboolean   gimp_gegl_buffer_get_tile_rect   (GeglBuffer    *buffer,
                                             gint           tile_width,
                                             gint           tile_height,
                                             gint           tile_num,
                                             GeglRectangle *rect);


#endif /* __GIMP_GEGL_TILE_COMPAT_H__ */
