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

#include "libgimp/gimpunit.h"

/*  typedefs  */
typedef void (* GimpQueryFunc) (GtkWidget *, gpointer, gpointer);


/*  widget constructors  */

GtkWidget * gimp_dialog_new                 (const gchar        *title,
					     const gchar        *wmclass_name,
					     GimpHelpFunc        help_func,
					     gchar              *help_data,
					     GtkWindowPosition   position,
					     gint                allow_shrink,
					     gint                allow_grow,
					     gint                auto_shrink,

					     /* specify action area buttons
					      * as va_list:
					      *  gchar          *label,
					      *  GtkSignalFunc   callback,
					      *  gpointer        data,
					      *  GtkWidget     **widget_ptr,
					      *  gboolean        default_action,
					      *  gboolean        connect_delete,
					      */

					     ...);

GtkWidget * gimp_dialog_newv                (const gchar        *title,
					     const gchar        *wmclass_name,
					     GimpHelpFunc        help_func,
					     gchar              *help_data,
					     GtkWindowPosition   position,
					     gint                allow_shrink,
					     gint                allow_grow,
					     gint                auto_shrink,
					     va_list             args);

void        gimp_dialog_create_action_area  (GtkDialog          *dialog,

					     /* specify action area buttons
					      * as va_list:
					      *  gchar          *label,
					      *  GtkSignalFunc   callback,
					      *  gpointer        data,
					      *  GtkWidget     **widget_ptr,
					      *  gboolean        default_action,
					      *  gboolean        connect_delete,
					      */

					     ...);

void        gimp_dialog_create_action_areav (GtkDialog          *dialog,
					     va_list             args);


GtkWidget * gimp_option_menu_new (GtkSignalFunc       menu_item_callback,
				  gpointer            initial,  /* user_data */

				  /* specify menu items as va_list:
				   *  gchar          *label,
				   *  gpointer        data,
				   *  gpointer        user_data,
				   */

				  ...);

GtkWidget * gimp_radio_group_new (GtkSignalFunc       radio_button_callback,
				  gpointer            initial,  /* user_data */

				  /* specify radio buttons as va_list:
				   *  gchar          *label,
				   *  gpointer        data,
				   *  gpointer        user_data,
				   */

				  ...);

GtkWidget * gimp_spin_button_new (/* return value: */
				  GtkObject         **adjustment,

				  gfloat              value,
				  gfloat              lower,
				  gfloat              upper,
				  gfloat              step_increment,
				  gfloat              page_increment,
				  gfloat              page_size,
				  gfloat              climb_rate,
				  guint               digits);

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
				   GUnit          unit,
				   gdouble        resolution,
				   gboolean       dot_for_dot,
				   GtkObject     *object,
				   gchar         *signal,
				   GimpQueryFunc  callback,
				   gpointer       data);

/*  a simple message box  */

GtkWidget * gimp_message_box      (gchar        *message,
				   GtkCallback   callback,
				   gpointer      data);

/*  helper functions  */

/*  add aligned label & widget to a two-column table  */
void gimp_table_attach_aligned (GtkTable  *table,
				gint       row,
				gchar     *text,
				gfloat     xalign,
				gfloat     yalign,
				GtkWidget *widget,
				gboolean   left_adjust);

#endif /* __GIMP_UI_H__ */
