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
#ifndef __RECT_SELECT_H__
#define __RECT_SELECT_H__

#include "gimpimageF.h"
#include "tools.h"
#include "channel.h"

typedef enum
{
  SELECTION_ADD       = ADD,
  SELECTION_SUB       = SUB,
  SELECTION_REPLACE   = REPLACE,
  SELECTION_INTERSECT = INTERSECT,
  SELECTION_MOVE_MASK,
  SELECTION_MOVE
} SelectOps;

/*  rect select action functions  */
void   rect_select_button_press   (Tool *, GdkEventButton *, gpointer);
void   rect_select_button_release (Tool *, GdkEventButton *, gpointer);
void   rect_select_motion         (Tool *, GdkEventMotion *, gpointer);
void   rect_select_cursor_update  (Tool *, GdkEventMotion *, gpointer);
void   rect_select_control        (Tool *, ToolAction,       gpointer);

/*  rect select functions  */
void   rect_select_draw           (Tool *);
void   rect_select                (GimpImage *, int, int, int, int, int, int,
				   double);

Tool * tools_new_rect_select      (void);
void   tools_free_rect_select     (Tool *);

#endif  /*  __RECT_SELECT_H__  */
