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
#ifndef  __BRUSH_SELECT_H__
#define  __BRUSH_SELECT_H__

#include "buildmenu.h"

typedef struct _BrushSelect _BrushSelect, *BrushSelectP;

struct _BrushSelect {
  GtkWidget *shell;
  GtkWidget *frame;
  GtkWidget *preview;
  GtkWidget *brush_name;
  GtkWidget *brush_size;
  GtkWidget *options_box;
  GtkAdjustment *opacity_data;
  GtkAdjustment *spacing_data;
  GtkAdjustment *sbar_data;
  int width, height;
  int cell_width, cell_height;
  int scroll_offset;
  int redraw;
  /*  Brush preview  */
  GtkWidget *brush_popup;
  GtkWidget *brush_preview;
};

BrushSelectP  brush_select_new     (void);
void          brush_select_select  (BrushSelectP, int);
void          brush_select_free    (BrushSelectP);

/*  An interface to other dialogs which need to create a paint mode menu  */
GtkWidget *   create_paint_mode_menu (MenuItemCallback);

#endif  /*  __BRUSH_SELECT_H__  */
