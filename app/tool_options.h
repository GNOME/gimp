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
#ifndef __TOOL_OPTIONS_H__
#define __TOOL_OPTIONS_H__

#include <gtk/gtk.h>

/*  tool options function declarations  */

typedef void  (* ToolOptionsResetFunc) (void);


/*  the tool options structures  */

typedef struct _ToolOptions ToolOptions;
struct _ToolOptions
{
  GtkWidget            *main_vbox;
  gchar                *title;

  ToolOptionsResetFunc  reset_func;
};


/*  create a dummy tool options structure
 *  (to be used by tools without options)
 */
ToolOptions * tool_options_new  (gchar *title);

/*  initialize an already allocated ToolOptions structure
 *  (to be used by derived tool options only)
 */
void          tool_options_init (ToolOptions          *options,
				 gchar                *title,
				 ToolOptionsResetFunc  reset_func);

#endif  /*  __TOOL_OPTIONS_H__  */
