/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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
#ifndef __PAINT_OPTIONS_H__
#define __PAINT_OPTIONS_H__

#include "buildmenu.h"
#include "tools.h"
#include "tool_options.h"


/*  the paint options structures  */
typedef struct _PaintOptions PaintOptions;
struct _PaintOptions
{
  ToolOptions  tool_options;

  /*  vbox for the common paint options  */
  GtkWidget   *paint_vbox;

  /*  a widget to be shown if we are in global mode  */
  GtkWidget   *global;

  /*  options used by all paint tools  */
  GtkObject   *opacity_w;

  GtkWidget   *paint_mode_w;
};


/*  paint tool options functions  */
PaintOptions *paint_options_new    (ToolType              tool_type,
				    ToolOptionsResetFunc  reset_func);

void          paint_options_reset  (PaintOptions         *options);

/*  to be used by "derived" paint options only  */
void          paint_options_init   (PaintOptions         *options,
				    ToolType              tool_type,
				    ToolOptionsResetFunc  reset_func);


/*  functions for the global paint options  */

/*  switch between global and per-tool paint options  */
void    paint_options_set_global     (gboolean  global);


/*  a utility function which returns a paint mode menu  */
GtkWidget *paint_mode_menu_new (MenuItemCallback  callback,
				gpointer          user_data);

#endif  /*  __PAINT_OPTIONS_H__  */
