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

#include "libgimp/gimpunit.h"

typedef struct _SelectionOptions SelectionOptions;
struct _SelectionOptions
{
  int        antialias;
  int        antialias_d;
  GtkWidget *antialias_w;

  int        feather;
  int        feather_d;
  GtkWidget *feather_w;

  GtkWidget *feather_radius_box;

  double     feather_radius;
  double     feather_radius_d;
  GtkObject *feather_radius_w;

  int        extend; /* used by bezier selection */

  int        sample_merged;
  int        sample_merged_d;
  GtkWidget *sample_merged_w;

  int        fixed_size;
  int        fixed_size_d;
  GtkWidget *fixed_size_w;

  int        fixed_width;
  int        fixed_width_d;
  GtkObject *fixed_width_w;

  int        fixed_height;
  int        fixed_height_d;
  GtkObject *fixed_height_w;

  GUnit      fixed_unit;
  GUnit      fixed_unit_d;
  GtkWidget *fixed_unit_w;
};

SelectionOptions *create_selection_options  (ToolType,
					     ToolOptionsResetFunc);
void              reset_selection_options   (SelectionOptions *);

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
