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
#ifndef __PIXEL_REGION_H__
#define __PIXEL_REGION_H__

#include "tile_manager.h"
#include "tile_accessor.h"
#include "pixel_processor.h" /* this is temporary, */

typedef struct _PixelRegion PixelRegion;

struct _PixelRegion
{
  unsigned char *    data;           /*  pointer to region data  */
  TileManager *      tiles;          /*  pointer to tiles  */
  TileAccessor       tileacc;	     /*  current tile  */
  int		     offx, offy;     /*  tile offsets */
  int                rowstride;      /*  bytes per pixel row  */
  int                x, y;           /*  origin  */
  int                w, h;           /*  width and  height of region  */
  int                bytes;          /*  bytes per pixel  */
  int                dirty;          /*  will this region be dirtied?  */
  int                process_count;  /*  used internally  */
};


/*  PixelRegion functions  */
void  pixel_region_init          (PixelRegion *, TileManager *, int, int, int, int, int);
void  pixel_region_resize        (PixelRegion *, int, int, int, int);
void  pixel_region_get_async     (PixelRegion *PR, int ulx, int uly,
				  int lrx, int lry);
void  pixel_region_get_row       (PixelRegion *, int, int, int, unsigned char *, int);
void  pixel_region_set_row       (PixelRegion *, int, int, int, unsigned char *);
void  pixel_region_get_col       (PixelRegion *, int, int, int, unsigned char *, int);
void  pixel_region_set_col       (PixelRegion *, int, int, int, unsigned char *);
void *pixel_regions_register     (int, ...);
void *pixel_regions_process      (void *);
void  pixel_regions_process_stop (void *);

#endif /* __PIXEL_REGION_H__ */
