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
#ifndef __TRANSFORM_TOOL_H__
#define __TRANSFORM_TOOL_H__

/*  transform directions  */
#define TRANSFORM_TRADITIONAL 0
#define TRANSFORM_CORRECTIVE  1

/*  tool functions  */
Tool     * tools_new_transform_tool  (void);
void       tools_free_transform_tool (Tool *tool);

gboolean   transform_tool_smoothing  (void);
gboolean   transform_tool_showpath   (void);
gboolean   transform_tool_clip	     (void);
gint	   transform_tool_direction  (void);
gint	   transform_tool_grid_size  (void);
gboolean   transform_tool_show_grid  (void);

#endif  /*  __TRANSFORM_TOOL_H__  */
