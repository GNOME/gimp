/* GIMP - The GNU Image Manipulation Program
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

#ifndef __TILE_MANAGER_H__
#define __TILE_MANAGER_H__


/* Creates a new tile manager with the specified width for the
 *  toplevel. The toplevel size is used to compute the number of
 *  levels and their size. Each level is 1/2 the width and height of
 *  the level above it.
 *
 * The toplevel is level 0. The smallest level in the hierarchy
 *  is "nlevels - 1". That level will be smaller than TILE_WIDTH x
 *  TILE_HEIGHT
 */
TileManager * tile_manager_new               (gint width,
                                              gint height,
                                              gint bpp);

/* Ref/Unref a tile manager.
 */
TileManager * tile_manager_ref               (TileManager *tm);
void          tile_manager_unref             (TileManager *tm);

/* Set the validate procedure for the tile manager.  The validate
 *  procedure is called when an invalid tile is referenced. If the
 *  procedure is NULL, then the tile is set to valid and its memory is
 *  allocated, but not initialized.
 */
void          tile_manager_set_validate_proc (TileManager      *tm,
                                              TileValidateProc  proc);

/* Get a specified tile from a tile manager.
 */
Tile        * tile_manager_get_tile          (TileManager *tm,
                                              gint         xpixel,
                                              gint         ypixel,
                                              gint         wantread,
                                              gint         wantwrite);

/* Get a specified tile from a tile manager.
 */
Tile        * tile_manager_get               (TileManager *tm,
                                              gint         tile_num,
                                              gint         wantread,
                                              gint         wantwrite);

void          tile_manager_map_tile          (TileManager *tm,
                                              gint         xpixel,
                                              gint         ypixel,
                                              Tile        *srctile);

void          tile_manager_map               (TileManager *tm,
                                              gint         tile_num,
                                              Tile        *srctile);

/* Validate a tiles memory.
 */
void          tile_manager_validate          (TileManager  *tm,
                                              Tile         *tile);

void          tile_invalidate                (Tile        **tile_ptr,
                                              TileManager  *tm,
                                              gint          tile_num);
void          tile_invalidate_tile           (Tile        **tile_ptr,
                                              TileManager  *tm,
                                              gint          xpixel,
                                              gint          ypixel);

/* Given a toplevel tile, this procedure will invalidate
 *  (set the dirty bit) for this toplevel tile.
 */
void          tile_manager_invalidate_tiles  (TileManager       *tm,
                                              Tile              *toplevel_tile);

void          tile_manager_set_user_data     (TileManager       *tm,
                                              gpointer           user_data);
gpointer      tile_manager_get_user_data     (const TileManager *tm);

gint          tile_manager_width             (const TileManager *tm);
gint          tile_manager_height            (const TileManager *tm);
gint          tile_manager_bpp               (const TileManager *tm);

void          tile_manager_get_offsets       (const TileManager *tm,
                                              gint              *x,
                                              gint              *y);
void          tile_manager_set_offsets       (TileManager       *tm,
                                              gint               x,
                                              gint               y);

gint64        tile_manager_get_memsize       (const TileManager *tm,
                                              gboolean           sparse);

void          tile_manager_get_tile_coordinates (TileManager *tm,
                                                 Tile        *tile,
                                                 gint        *x,
                                                 gint        *y);

void          tile_manager_map_over_tile        (TileManager *tm,
                                                 Tile        *tile,
                                                 Tile        *srctile);

void              read_pixel_data    (TileManager  *tm,
                                      gint          x1,
                                      gint          y1,
                                      gint          x2,
                                      gint          y2,
                                      guchar       *buffer,
                                      guint         stride);

void              write_pixel_data   (TileManager  *tm,
                                      gint          x1,
                                      gint          y1,
                                      gint          x2,
                                      gint          y2,
                                      const guchar *buffer,
                                      guint         stride);

/*   Fill buffer with the pixeldata for the pixel at coordinates x,y
 *   if x,y is outside the area of the tilemanger, nothing is done.
 */
void              read_pixel_data_1  (TileManager  *tm,
                                      gint          x,
                                      gint          y,
                                      guchar       *buffer);

void              write_pixel_data_1 (TileManager  *tm,
                                      gint          x,
                                      gint          y,
                                      const guchar *buffer);

#endif /* __TILE_MANAGER_H__ */
