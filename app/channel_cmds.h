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
#ifndef __CHANNEL_CMDS_H__
#define __CHANNEL_CMDS_H__

#include "procedural_db.h"

extern ProcRecord channel_new_proc;
extern ProcRecord channel_copy_proc;
extern ProcRecord channel_delete_proc;
extern ProcRecord channel_get_name_proc;
extern ProcRecord channel_set_name_proc;
extern ProcRecord channel_get_visible_proc;
extern ProcRecord channel_set_visible_proc;
extern ProcRecord channel_get_show_masked_proc;
extern ProcRecord channel_set_show_masked_proc;
extern ProcRecord channel_get_opacity_proc;
extern ProcRecord channel_set_opacity_proc;
extern ProcRecord channel_get_color_proc;
extern ProcRecord channel_set_color_proc;

#endif /* __CHANNEL_CMDS_H__ */
