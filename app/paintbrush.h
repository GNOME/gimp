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
#ifndef __PAINTBRUSH_H__
#define __PAINTBRUSH_H__

#include "tools.h"
#include "paint_core.h"

void *        paintbrush_paint_func (PaintCore *, GimpDrawable *, int);
gboolean      paintbrush_non_gui    (GimpDrawable *, int, double *, double,
   				     int, double);

Tool *        tools_new_paintbrush   (void);
void          tools_free_paintbrush  (Tool *);

#endif  /*  __PAINTBRUSH_H__  */
