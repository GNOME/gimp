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

#ifndef __PIXEL_REGION_P_H__
#define __PIXEL_REGION_P_H__

#include <glib.h>
#include "pixel_region.h"

typedef struct _PixelRegionHolder PixelRegionHolder;

struct _PixelRegionHolder
{
  PixelRegion *PR;
  guchar      *original_data;
  gint         startx;
  gint         starty;
  gint         count;
};


typedef struct _PixelRegionIterator PixelRegionIterator;

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

#endif  /*  __PIXEL_REGION_P_H__  */
