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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __EDIT_SELECTION_H__
#define __EDIT_SELECTION_H__

#include "tools.h"

typedef enum
{
  MaskTranslate,
  MaskToLayerTranslate,
  LayerTranslate,
  FloatingSelTranslate
} EditType;

/*  action functions  */
void   edit_selection_button_release (Tool *, GdkEventButton *, gpointer);
void   edit_selection_motion         (Tool *, GdkEventMotion *, gpointer);
void   edit_selection_control        (Tool *, int, gpointer);
void   edit_selection_cursor_update  (Tool *, GdkEventMotion *, gpointer);
void   edit_selection_draw           (Tool *);
void   edit_sel_arrow_keys_func      (Tool *, GdkEventKey *, gpointer);


void   init_edit_selection           (Tool *, gpointer, GdkEventButton *, EditType);

#endif  /*  __EDIT_SELECTION_H__  */
