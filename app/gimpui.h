/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpui.h
 * Copyright (C) 1999 Michael Natterer <mitch@gimp.org>
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
#ifndef __GIMP_UI_H__
#define __GIMP_UI_H__

#include <gtk/gtk.h>

#include "gimphelp.h"

#include "libgimp/gimpdialog.h"
#include "libgimp/gimpunit.h"
#include "libgimp/gimpwidgets.h"

/*  typedefs  */
typedef void (* GimpQueryFunc) (GtkWidget *, gpointer, gpointer);

/*  some simple query dialogs
 *  if object != NULL then the query boxes will connect their cancel callback
 *  to the provided signal of this object
 *
 *  it's the caller's job to show the returned widgets
 */

GtkWidget * gimp_query_string_box (gchar         *title,
				   GimpHelpFunc   help_func,
				   gchar         *help_data,
				   gchar         *message,
				   gchar         *initial,
				   GtkObject     *object,
				   gchar         *signal,
				   GimpQueryFunc  callback,
				   gpointer       data);

GtkWidget * gimp_query_int_box    (gchar         *title,
				   GimpHelpFunc   help_func,
				   gchar         *help_data,
				   char          *message,
				   gint           initial,
				   gint           lower,
				   gint           upper,
				   GtkObject     *object,
				   gchar         *signal,
				   GimpQueryFunc  callback,
				   gpointer       data);

GtkWidget * gimp_query_double_box (gchar         *title,
				   GimpHelpFunc   help_func,
				   gchar         *help_data,
				   gchar         *message,
				   gdouble        initial,
				   gdouble        lower,
				   gdouble        upper,
				   gint           digits,
				   GtkObject     *object,
				   gchar         *signal,
				   GimpQueryFunc  callback,
				   gpointer       data);

GtkWidget * gimp_query_size_box   (gchar         *title,
				   GimpHelpFunc   help_func,
				   gchar         *help_data,
				   gchar         *message,
				   gdouble        initial,
				   gdouble        lower,
				   gdouble        upper,
				   gint           digits,
				   GimpUnit       unit,
				   gdouble        resolution,
				   gboolean       dot_for_dot,
				   GtkObject     *object,
				   gchar         *signal,
				   GimpQueryFunc  callback,
				   gpointer       data);

/*  a simple message box  */

void gimp_message_box             (gchar        *message,
				   GtkCallback   callback,
				   gpointer      data);

#endif /* __GIMP_UI_H__ */
