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
#ifndef __DODGEBURN_H__
#define __DODGEBURN_H__

#include "paint_core.h"
#include "tools.h"

typedef enum
{
  DODGE,
  BURN 
} DodgeBurnType;

typedef enum
{
  DODGEBURN_HIGHLIGHTS,
  DODGEBURN_MIDTONES,
  DODGEBURN_SHADOWS 
} DodgeBurnMode;

void *        dodgeburn_paint_func  (PaintCore *, GimpDrawable *, int);
gboolean      dodgeburn_non_gui     (GimpDrawable *, double, DodgeBurnType, DodgeBurnMode, int, double *);
gboolean      dodgeburn_non_gui_default (GimpDrawable *, int, double *);
Tool *        tools_new_dodgeburn   (void);
void          tools_free_dodgeburn  (Tool *);

#endif  /*  __DODGEBURN_H__  */
