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

#include "procedural_db.h"
#include "tools.h"

/*  rect select action functions  */
void          rect_select_button_press      (Tool *, GdkEventButton *, gpointer);
void          rect_select_button_release    (Tool *, GdkEventButton *, gpointer);
void          rect_select_motion            (Tool *, GdkEventMotion *, gpointer);
void          rect_select_cursor_update     (Tool *, GdkEventMotion *, gpointer);
void          rect_select_control           (Tool *, int, gpointer);

/*  rect select functions  */
void          rect_select_draw         (Tool *);
Tool *        tools_new_rect_select    (void);
void          tools_free_rect_select   (Tool *);

/*  Procedure definition and marshalling function  */
extern ProcRecord rect_select_proc;

#endif  /*  __RECT_SELECT_H__  */
