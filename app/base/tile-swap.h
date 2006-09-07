/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __TILE_SWAP_H__
#define __TILE_SWAP_H__


typedef enum
{
  SWAP_IN = 1,
  SWAP_OUT,
  SWAP_DELETE
} SwapCommand;


typedef gint (* SwapFunc) (gint         fd,
                           Tile        *tile,
                           SwapCommand  cmd,
                           gpointer     user_data);


void     tile_swap_init     (const gchar *path);
void     tile_swap_exit     (void);
gint     tile_swap_add      (gchar       *filename,
                             SwapFunc     swap_func,
                             gpointer     user_data);
void     tile_swap_remove   (gint         swap_num);
void     tile_swap_in       (Tile        *tile);
void     tile_swap_out      (Tile        *tile);
void     tile_swap_delete   (Tile        *tile);
gboolean tile_swap_test     (void);


#endif /* __TILE_SWAP_H__ */
