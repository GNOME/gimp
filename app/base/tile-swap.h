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

#ifndef __TILE_SWAP_H__
#define __TILE_SWAP_H__


void     tile_swap_init     (const gchar *path);
void     tile_swap_exit     (void);

gboolean tile_swap_test     (void);

void     tile_swap_in       (Tile        *tile);
void     tile_swap_out      (Tile        *tile);
void     tile_swap_delete   (Tile        *tile);


#endif /* __TILE_SWAP_H__ */
