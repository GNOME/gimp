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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __LAYER_CMDS_H__
#define __LAYER_CMDS_H__

#include "procedural_db.h"

extern ProcRecord layer_new_proc;
extern ProcRecord layer_copy_proc;
extern ProcRecord layer_create_mask_proc;
extern ProcRecord layer_scale_proc;
extern ProcRecord layer_resize_proc;
extern ProcRecord layer_delete_proc;
extern ProcRecord layer_translate_proc;
extern ProcRecord layer_add_alpha_proc;
extern ProcRecord layer_get_name_proc;
extern ProcRecord layer_set_name_proc;
extern ProcRecord layer_get_visible_proc;
extern ProcRecord layer_set_visible_proc;
extern ProcRecord layer_get_preserve_trans_proc;
extern ProcRecord layer_set_preserve_trans_proc;
extern ProcRecord layer_get_apply_mask_proc;
extern ProcRecord layer_set_apply_mask_proc;
extern ProcRecord layer_get_show_mask_proc;
extern ProcRecord layer_set_show_mask_proc;
extern ProcRecord layer_get_edit_mask_proc;
extern ProcRecord layer_set_edit_mask_proc;
extern ProcRecord layer_get_opacity_proc;
extern ProcRecord layer_set_opacity_proc;
extern ProcRecord layer_get_mode_proc;
extern ProcRecord layer_set_mode_proc;
extern ProcRecord layer_set_offsets_proc;
extern ProcRecord layer_mask_proc;
extern ProcRecord layer_is_floating_sel_proc;

#endif /* __LAYER_CMDS_H__ */
