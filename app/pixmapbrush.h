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
#ifndef __PIXMAPBRUSH_H__
#define __PIXMAPBRUSH_H__

#include "tools.h"
#include "paint_core.h"
#include "procedural_db.h"
#include "gimpbrush.h"
#include "gimpbrushpixmap.h"
#include "temp_buf.h"

void *        pixmap_paint_func  (PaintCore *, GimpDrawable *, int);
Tool *        tools_new_pixmapbrush   (void);
void          tools_free_pixmapbrush  (Tool *);

void          paint_line_pixmap_mask(GImage *dest, GimpDrawable *drawable,
				     GimpBrushPixmap *brush,
				     unsigned char * d,
				     int            x,
				     int            offset_x,
				     int            y,
				     int            offset_y,
				     int            bytes,
				     int            width);			     

void           color_area_with_pixmap (GImage *dest,
				       GimpDrawable *drawable,
				       TempBuf *area,
				       GimpBrush *brush);
/*  Procedure definition and marshalling function  */
/* extern ProcRecord pixmapbrush_proc; */
/* extern ProcRecord pixmapbrush_extended_proc; */
/* extern ProcRecord pixmapbrush_extended_gradient_proc; */
#endif  /*  __PIXMAPBRUSH_H__  */
