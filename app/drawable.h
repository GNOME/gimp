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
#ifndef __DRAWABLE_H__
#define __DRAWABLE_H__

#include "gimpdrawable.h"

int drawable_ID (GimpDrawable *);
void drawable_fill (GimpDrawable *drawable, GimpFillType fill_type);
void drawable_update (GimpDrawable *drawable, int x, int y, int w, int h);
void drawable_apply_image (GimpDrawable *, int, int, int, int, 
			   TileManager *, int);


#define drawable_merge_shadow gimp_drawable_merge_shadow
#define drawable_mask_bounds gimp_drawable_mask_bounds
#define drawable_invalidate_preview gimp_drawable_invalidate_preview
#define drawable_dirty gimp_drawable_dirty
#define drawable_clean gimp_drawable_clean
#define drawable_type gimp_drawable_type
#define drawable_has_alpha gimp_drawable_has_alpha
#define drawable_type_with_alpha gimp_drawable_type_with_alpha
#define drawable_color gimp_drawable_color
#define drawable_gray gimp_drawable_gray
#define drawable_indexed gimp_drawable_indexed
#define drawable_data gimp_drawable_data
#define drawable_shadow gimp_drawable_shadow
#define drawable_bytes gimp_drawable_bytes
#define drawable_width gimp_drawable_width
#define drawable_height gimp_drawable_height
#define drawable_visible gimp_drawable_visible	
#define drawable_offsets gimp_drawable_offsets
#define drawable_cmap gimp_drawable_cmap
#define drawable_get_name gimp_drawable_get_name		
#define drawable_set_name gimp_drawable_set_name		

#define drawable_get_ID gimp_drawable_get_ID
#define drawable_deallocate gimp_drawable_deallocate	
#define drawable_gimage gimp_drawable_gimage

#endif /* __DRAWABLE_H__ */
