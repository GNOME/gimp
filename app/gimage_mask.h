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
gboolean        gimage_mask_boundary      (GImage       *gimage,
					   BoundSeg    **segs_in,
					   BoundSeg    **segs_out,
					   gint         *num_segs_in,
					   gint         *num_segs_out);

gboolean        gimage_mask_bounds        (GImage       *gimage,
					   gint         *x1,
					   gint         *y1,
					   gint         *x2,
					   gint         *y2);

void            gimage_mask_invalidate    (GImage       *gimage);

gint            gimage_mask_value         (GImage       *gimage,
					   gint          x,
					   gint          y);

gboolean        gimage_mask_is_empty      (GImage       *gimage);

void            gimage_mask_translate     (GImage       *gimage,
					   gint          off_x,
					   gint          off_y);

TileManager *   gimage_mask_extract       (GImage       *gimage,
					   GimpDrawable *drawable,
					   gboolean      cut_gimage,
					   gboolean      keep_indexed);

Layer *         gimage_mask_float         (GImage       *gimage,
					   GimpDrawable *drawable,
					   gint          off_x,
					   gint          off_y);

void            gimage_mask_clear         (GImage       *gimage);
void            gimage_mask_undo          (GImage       *gimage);
void            gimage_mask_invert        (GImage       *gimage);
void            gimage_mask_sharpen       (GImage       *gimage);
void            gimage_mask_all           (GImage       *gimage);
void            gimage_mask_none          (GImage       *gimage);

void            gimage_mask_feather       (GImage       *gimage,
					   gdouble       feather_radius_x,
					   gdouble       feather_radius_y);

void            gimage_mask_border        (GImage       *gimage,
					   gint          border_radius_x,
					   gint          border_radius_y);

void            gimage_mask_grow          (GImage       *gimage,
					   gint           grow_pixels_x,
					   gint           grow_pixels_y);

void            gimage_mask_shrink        (GImage       *gimage,
					   gint          shrink_pixels_x,
					   gint          shrink_pixels_y,
					   gboolean      edge_lock);

void            gimage_mask_layer_alpha   (GImage       *gimage,
					   Layer        *layer);

void            gimage_mask_layer_mask    (GImage       *gimage,
					   Layer        *layer);

void            gimage_mask_load          (GImage       *gimage,
					   Channel      *channel);

Channel *       gimage_mask_save          (GImage       *gimage);

gboolean        gimage_mask_stroke        (GImage       *gimage,
					   GimpDrawable *drawable);

#endif  /*  __GIMAGE_MASK_H__  */
