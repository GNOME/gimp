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
#ifndef __DRAWABLE_CMDS_H__
#define __DRAWABLE_CMDS_H__

#include "procedural_db.h"

extern ProcRecord drawable_merge_shadow_proc;
extern ProcRecord drawable_fill_proc;
extern ProcRecord drawable_update_proc;
extern ProcRecord drawable_mask_bounds_proc;
extern ProcRecord drawable_gimage_proc;
extern ProcRecord drawable_type_proc;
extern ProcRecord drawable_has_alpha_proc;
extern ProcRecord drawable_type_with_alpha_proc;
extern ProcRecord drawable_color_proc;
extern ProcRecord drawable_gray_proc;
extern ProcRecord drawable_indexed_proc;
extern ProcRecord drawable_bytes_proc;
extern ProcRecord drawable_width_proc;
extern ProcRecord drawable_height_proc;
extern ProcRecord drawable_offsets_proc;
extern ProcRecord drawable_layer_proc;
extern ProcRecord drawable_layer_mask_proc;
extern ProcRecord drawable_channel_proc;
extern ProcRecord drawable_set_pixel_proc;
extern ProcRecord drawable_get_pixel_proc;
extern ProcRecord gimp_drawable_find_parasite_proc;
extern ProcRecord gimp_drawable_attach_parasite_proc;
extern ProcRecord gimp_drawable_detach_parasite_proc;
extern ProcRecord drawable_set_image_proc;

#endif /* __DRAWABLE_CMDS_H__ */
