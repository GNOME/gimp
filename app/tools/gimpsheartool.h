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
#ifndef __SHEAR_TOOL_H__
#define __SHEAR_TOOL_H__

#include "tools.h"
#include "transform_core.h"

TileManager * shear_tool_transform (Tool           *tool,
				    gpointer        gdisp_ptr,
				    TransformState  state);
TileManager * shear_tool_shear     (GimpImage      *gimage,
				    GimpDrawable   *drawable,
				    GDisplay       *gdisp,
				    TileManager    *float_tiles,
				    gboolean        interpolation,
				    GimpMatrix      matrix);

Tool * tools_new_shear_tool  (void);
void   tools_free_shear_tool (Tool *matrix);

#endif  /*  __SHEAR_TOOL_H__  */
