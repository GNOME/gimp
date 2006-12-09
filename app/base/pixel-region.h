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

#ifndef __PIXEL_REGION_H__
#define __PIXEL_REGION_H__


struct _PixelRegion
{
  guchar      *data;           /*  pointer to region data        */
  TileManager *tiles;          /*  pointer to tiles              */
  Tile        *curtile;        /*  current tile                  */
  gint         offx;           /*  tile offsets                  */
  gint         offy;           /*  tile offsets                  */
  gint         rowstride;      /*  bytes per pixel row           */
  gint         x;              /*  origin                        */
  gint         y;              /*  origin                        */
  gint         w;              /*  width of region               */
  gint         h;              /*  height of region              */
  gint         bytes;          /*  bytes per pixel               */
  gboolean     dirty;          /*  will this region be dirtied?  */
  gint         process_count;  /*  used internally               */
};

struct _PixelRegionHolder
{
  PixelRegion *PR;
  guchar      *original_data;
  gint         startx;
  gint         starty;
  gint         count;
};

struct _PixelRegionIterator
{
  GSList *pixel_regions;
  gint    dirty_tiles;
  gint    region_width;
  gint    region_height;
  gint    portion_width;
  gint    portion_height;
  gint    process_count;
};


/*  PixelRegion functions  */

void     pixel_region_init          (PixelRegion         *PR,
                                     TileManager         *tiles,
                                     gint                 x,
                                     gint                 y,
                                     gint                 w,
                                     gint                 h,
                                     gboolean             dirty);
void     pixel_region_init_temp_buf (PixelRegion         *PR,
                                     TempBuf             *temp_buf,
                                     gint                 x,
                                     gint                 y,
                                     gint                 w,
                                     gint                 h);
void     pixel_region_init_data     (PixelRegion         *PR,
                                     guchar              *data,
                                     gint                 bytes,
                                     gint                 rowstride,
                                     gint                 x,
                                     gint                 y,
                                     gint                 w,
                                     gint                 h);
void     pixel_region_resize        (PixelRegion         *PR,
                                     gint                 x,
                                     gint                 y,
                                     gint                 w,
                                     gint                 h);
void     pixel_region_get_row       (PixelRegion         *PR,
                                     gint                 x,
                                     gint                 y,
                                     gint                 w,
                                     guchar              *data,
                                     gint                 subsample);
void     pixel_region_set_row       (PixelRegion         *PR,
                                     gint                 x,
                                     gint                 y,
                                     gint                 w,
                                     const guchar        *data);
void     pixel_region_get_col       (PixelRegion         *PR,
                                     gint                 x,
                                     gint                 y,
                                     gint                 h,
                                     guchar              *data,
                                     gint                 subsample);
void     pixel_region_set_col       (PixelRegion         *PR,
                                     gint                 x,
                                     gint                 y,
                                     gint                 h,
                                     const guchar        *data);
gboolean pixel_region_has_alpha     (PixelRegion         *PR);

PixelRegionIterator * pixel_regions_register     (gint    num_regions,
                                                  ...);
PixelRegionIterator * pixel_regions_process      (PixelRegionIterator *PRI);
void                  pixel_regions_process_stop (PixelRegionIterator *PRI);


#endif /* __PIXEL_REGION_H__ */
