/* The GIMP -- an image manipulation program
 * Copyright (C) 1999 Andy Thomas alt@picnic.demon.co.uk
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
#ifndef  __PATH_TRANSFORM_H__
#define  __PATH_TRANSFORM_H__

#include "draw_core.h" 
#include "gdisplayF.h" 
#include "path.h"

#include "libgimp/gimpmatrix.h"

PathUndo*  path_transform_start_undo    (GimpImage   *gimage);
void       path_transform_free_undo     (PathUndo    *pundo);
void       path_transform_do_undo       (GimpImage   *gimage, 
					 PathUndo    *pundo);

void       path_transform_current_path  (GimpImage   *gimage,  
					 GimpMatrix3  transform,
					 gboolean     forpreview);
void       path_transform_draw_current  (GDisplay    *gimage, 
					 DrawCore    *core, 
					 GimpMatrix3  transform);

void       path_transform_flip_horz     (GimpImage   *gimage);
void       path_transform_flip_vert     (GimpImage   *gimage);
void       path_transform_xy            (GimpImage   *gimage, gint x, gint y);

#endif  /*  __PATH_TRANSFORM_H__  */




