/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * brush-editor.h
 * Copyright 1998 Jay Cox <jaycox@earthlink.net>
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

#ifndef  __BRUSH_EDITOR_H__
#define  __BRUSH_EDITOR_H__


typedef struct _BrushEditor BrushEditor;


BrushEditor * brush_editor_new       (Gimp        *gimp);

void          brush_editor_set_brush (BrushEditor *brush_editor,
				      GimpBrush   *brush);
void          brush_editor_free      (BrushEditor *brush_editor);


#endif  /*  __BRUSH_EDITOR_H__  */
