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
#ifndef __CLONE_H__
#define __CLONE_H__

#include "paint_core.h"
#include "tools.h"

typedef enum
{
  IMAGE_CLONE,
  PATTERN_CLONE
} CloneType;

void *        clone_paint_func  (PaintCore *, GimpDrawable *, int);
gboolean      clone_non_gui     (GimpDrawable *, GimpDrawable *, CloneType,
    				 double, double, int, double *);
gboolean      clone_non_gui_default (GimpDrawable *, int, double *);
Tool *        tools_new_clone   (void);
void          tools_free_clone  (Tool *);

#endif  /*  __CLONE_H__  */
