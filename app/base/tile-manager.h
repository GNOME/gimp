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

#ifndef __TILE_MANAGER_H__
#define __TILE_MANAGER_H__


#define GIMP_TYPE_TILE_MANAGER               (gimp_tile_manager_get_type ())
#define GIMP_VALUE_HOLDS_TILE_MANAGER(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_TILE_MANAGER))

GType         gimp_tile_manager_get_type     (void) G_GNUC_CONST;

#ifdef GIMP_UNSTABLE
void          tile_manager_exit              (void);
#endif

/* Creates a new tile manager with the specified size */
TileManager * tile_manager_new               (gint width,
                                              gint height,
                                              gint bpp);

/* Ref/Unref a tile manager.
 */
TileManager * tile_manager_ref               (TileManager *tm);
void          tile_manager_unref             (TileManager *tm);

/* Make a copy of the tile manager.
 */
TileManager * tile_manager_duplicate         (TileManager *tm);

/* Set the validate procedure for the tile manager.  The validate
 *  procedure is called when an invalid tile is referenced. If the
 *  procedure is NULL, then the tile is set to valid and its memory is
 *  allocated, but not initialized.
 */
void          tile_manager_set_validate_proc (TileManager      *tm,
                                              TileValidateProc  proc,
                                              gpointer          user_data);

/* Get a specified tile from a tile manager.
 */
Tile        * tile_manager_get_tile          (TileManager *tm,
                                              gint         xpixel,
                                              gint         ypixel,
                                              gboolean     wantread,
                                              gboolean     wantwrite);

/* Get a specified tile from a tile manager.
 */
Tile        * tile_manager_get               (TileManager *tm,
                                              gint         tile_num,
                                              gboolean     wantread,
                                              gboolean     wantwrite);

Tile        * tile_manager_get_at            (TileManager *tm,
                                              gint         tile_col,
                                              gint         tile_row,
                                              gboolean     wantread,
                                              gboolean     wantwrite);

void          tile_manager_map_tile          (TileManager *tm,
                                              gint         xpixel,
                                              gint         ypixel,
                                              Tile        *srctile);

void          tile_manager_map               (TileManager *tm,
                                              gint         tile_num,
                                              Tile        *srctile);

/* Validate a tiles memory.
 */
void          tile_manager_validate_tile     (TileManager  *tm,
                                              Tile         *tile);

void          tile_manager_invalidate_area   (TileManager       *tm,
                                              gint               x,
                                              gint               y,
                                              gint               w,
                                              gint               h);

gint          tile_manager_width             (const TileManager *tm);
gint          tile_manager_height            (const TileManager *tm);
gint          tile_manager_bpp               (const TileManager *tm);

gint64        tile_manager_get_memsize       (const TileManager *tm,
                                              gboolean           sparse);

void          tile_manager_get_tile_coordinates (TileManager *tm,
                                                 Tile        *tile,
                                                 gint        *x,
                                                 gint        *y);
void          tile_manager_get_tile_col_row     (TileManager *tm,
                                                 Tile        *tile,
                                                 gint        *tile_col,
                                                 gint        *tile_row);

void          tile_manager_map_over_tile        (TileManager *tm,
                                                 Tile        *tile,
                                                 Tile        *srctile);

void          tile_manager_read_pixel_data      (TileManager  *tm,
                                                 gint          x1,
                                                 gint          y1,
                                                 gint          x2,
                                                 gint          y2,
                                                 guchar       *buffer,
                                                 guint         stride);

void          tile_manager_write_pixel_data     (TileManager  *tm,
                                                 gint          x1,
                                                 gint          y1,
                                                 gint          x2,
                                                 gint          y2,
                                                 const guchar *buffer,
                                                 guint         stride);

/*   Fill buffer with the pixeldata for the pixel at coordinates x,y
 *   if x,y is outside the area of the tilemanger, nothing is done.
 */
void          tile_manager_read_pixel_data_1    (TileManager  *tm,
                                                 gint          x,
                                                 gint          y,
                                                 guchar       *buffer);

void          tile_manager_write_pixel_data_1   (TileManager  *tm,
                                                 gint          x,
                                                 gint          y,
                                                 const guchar *buffer);

#endif /* __TILE_MANAGER_H__ */
