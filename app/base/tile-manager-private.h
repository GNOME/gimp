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

#ifndef __TILE_MANAGER_PVT_H__
#define __TILE_MANAGER_PVT_H__


struct _TileManager
{
  gint               ref_count;     /*  reference counter                    */

  gint               width;         /*  the width of the tiled area          */
  gint               height;        /*  the height of the tiled area         */
  gint               bpp;           /*  the bpp of each tile                 */

  gint               ntile_rows;    /*  the number of tiles in each row      */
  gint               ntile_cols;    /*  the number of tiles in each columns  */

  Tile             **tiles;         /*  the tiles for this level             */
  TileValidateProc   validate_proc; /*  this proc is called when an attempt  *
                                     *  to get an invalid tile is made       */
  gpointer           user_data;     /*  data to pass to the validate_proc    */

  gint               cached_num;    /*  number of cached tile                */
  Tile              *cached_tile;   /*  the actual cached tile               */
};


#endif /* __TILE_MANAGER_PVT_H__ */
