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

#include "gimpobject.h"

#define GIMP_TYPE_TOOL_OPTIONS            (gimp_tool_options_get_type ())
#define GIMP_TOOL_OPTIONS(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_TOOL_OPTIONS, GimpToolOptions))
#define GIMP_IS_TOOL_OPTIONS(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_TOOL_OPTIONS))
#define GIMP_TOOL_OPTIONS_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_OPTIONS, GimpToolOptionsClass))
#define GIMP_IS_TOOL_OPTIONS_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_OPTIONS))

/*  the tool options structures  */

struct _ToolOptions
{
  GimpObject	       parent_instance;
  GtkWidget            *main_vbox;
  gchar                *title;

  ToolOptionsResetFunc  reset_func;
};

struct _ToolOptionsClass
{
  GimpObjectClass   parent_class;
};

typedef struct _ToolOptionsClass ToolOptionsClass;

/*  create a dummy tool options structure
 *  (to be used by tools without options)
 */
ToolOptions * tool_options_new  (const gchar *title);

/*  initialize an already allocated ToolOptions structure
 *  (to be used by derived tool options only)
 */
void          tool_options_init (ToolOptions          *options,
				 const gchar                *title,
				 ToolOptionsResetFunc  reset_func);


#endif  /*  __TOOL_OPTIONS_H__  */
