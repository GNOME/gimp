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
#ifndef __GIMAGE_MASK_CMDS_H__
#define __GIMAGE_MASK_CMDS_H__

#include "procedural_db.h"

extern ProcRecord gimage_mask_bounds_proc;
extern ProcRecord gimage_mask_value_proc;
extern ProcRecord gimage_mask_is_empty_proc;
extern ProcRecord gimage_mask_translate_proc;
extern ProcRecord gimage_mask_float_proc;
extern ProcRecord gimage_mask_clear_proc;
extern ProcRecord gimage_mask_invert_proc;
extern ProcRecord gimage_mask_sharpen_proc;
extern ProcRecord gimage_mask_all_proc;
extern ProcRecord gimage_mask_none_proc;
extern ProcRecord gimage_mask_feather_proc;
extern ProcRecord gimage_mask_border_proc;
extern ProcRecord gimage_mask_grow_proc;
extern ProcRecord gimage_mask_shrink_proc;
extern ProcRecord gimage_mask_layer_alpha_proc;
extern ProcRecord gimage_mask_load_proc;
extern ProcRecord gimage_mask_save_proc;

#endif /* __GIMAGE_MASK_CMDS_H__ */
