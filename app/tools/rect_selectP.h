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
#ifndef __RECT_SELECTP_H__
#define __RECT_SELECTP_H__

#include "draw_core.h"

typedef struct _RectSelect RectSelect, EllipseSelect;

struct _RectSelect
{
  DrawCore   *core;       /*  Core select object  */

  SelectOps   op;         /*  selection operation (SELECTION_ADD etc.)  */

  gint        current_x;  /*  these values are updated on every motion event  */
  gint        current_y;  /*  (enables immediate cursor updating on modifier
			   *   key events).  */

  gint        x, y;        /*  upper left hand coordinate  */
  gint        w, h;        /*  width and height  */
  gint        center;      /*  is the selection being created from the
			    *  center out?  */

  gint        fixed_size;
  gdouble     fixed_width;
  gdouble     fixed_height;
  guint       context_id;   /*  for the statusbar  */
};

#endif  /*  __RECT_SELECTP_H__  */
