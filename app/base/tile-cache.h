/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __TILE_CACHE_H__
#define __TILE_CACHE_H__


void   tile_cache_init                 (guint64 cache_size);
void   tile_cache_exit                 (void);

void   tile_cache_set_size             (guint64 cache_size);
void   tile_cache_suspend_idle_swapper (void);

void   tile_cache_insert               (Tile   *tile);
void   tile_cache_flush                (Tile   *tile);

#endif /* __TILE_CACHE_H__ */
