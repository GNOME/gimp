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
#ifndef __GIMAGE_MASK_H__
#define __GIMAGE_MASK_H__

#include "boundary.h"
#include "gimage.h"

/*  mask functions  */
int             gimage_mask_boundary      (GImage       *gimage,
					   BoundSeg    **segs_in,
					   BoundSeg    **segs_out,
					   int          *num_segs_in,
					   int          *num_segs_out);

int             gimage_mask_bounds        (GImage       *gimage,
					   int          *x1,
					   int          *y1,
					   int          *x2,
					   int          *y2);

void            gimage_mask_invalidate    (GImage       *gimage);

int             gimage_mask_value         (GImage       *gimage,
					   int           x,
					   int           y);

int             gimage_mask_is_empty      (GImage       *gimage);

void            gimage_mask_translate     (GImage       *gimage,
					   int           off_x,
					   int           off_y);

TileManager *   gimage_mask_extract       (GImage       *gimage,
					   GimpDrawable *drawable,
					   int           cut_gimage,
					   int           keep_indexed);

Layer *         gimage_mask_float         (GImage       *gimage,
					   GimpDrawable *drawable,
					   int           off_x,
					   int           off_y);

void            gimage_mask_clear         (GImage       *gimage);
void            gimage_mask_undo          (GImage       *gimage);
void            gimage_mask_invert        (GImage       *gimage);
void            gimage_mask_sharpen       (GImage       *gimage);
void            gimage_mask_all           (GImage       *gimage);
void            gimage_mask_none          (GImage       *gimage);

void            gimage_mask_feather       (GImage       *gimage,
					   double        feather_radius_x,
					   double        feather_radius_y);

void            gimage_mask_border        (GImage       *gimage,
					   int           border_radius_x,
					   int           border_radius_y);

void            gimage_mask_grow          (GImage       *gimage,
					   int           grow_pixels_x,
					   int           grow_pixels_y);

void            gimage_mask_shrink        (GImage       *gimage,
					   int           shrink_pixels_x,
					   int           shrink_pixels_y,
					   int           edge_lock);

void            gimage_mask_layer_alpha   (GImage       *gimage,
					   Layer        *layer);

void            gimage_mask_layer_mask    (GImage       *gimage,
					   Layer        *layer);

void            gimage_mask_load          (GImage       *gimage,
					   Channel      *channel);

Channel *       gimage_mask_save          (GImage       *gimage);

int             gimage_mask_stroke        (GImage       *gimage,
					   GimpDrawable *drawable);

#endif  /*  __GIMAGE_MASK_H__  */
