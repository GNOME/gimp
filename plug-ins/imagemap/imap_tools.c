/*
 * This is a plug-in for the GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-1999 Maurits Rijk  lpeek.mrijk@consunet.nl
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
 *
 */

#include "config.h"
#include "imap_circle.h"
#include "imap_edit_area_info.h"
#include "imap_main.h"
#include "libgimp/stdplugins-intl.h"
#include "imap_menu.h"
#include "imap_misc.h"
#include "imap_polygon.h"
#include "imap_popup.h"
#include "imap_rectangle.h"
#include "imap_tools.h"

#include "arrow.xpm"
#include "rectangle.xpm"
#include "circle.xpm"
#include "polygon.xpm"
#include "delete.xpm"
#include "edit.xpm"

static gboolean _callback_lock;
static Tools_t _tools;

static void
tools_command(GtkWidget *widget, gpointer data)
{
   CommandFactory_t *factory = (CommandFactory_t*) data;
   Command_t *command = (*factory)();
   command_execute(command);
}

void 
arrow_on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
   if (event->button == 1) {
      if (event->type == GDK_2BUTTON_PRESS)
	 edit_shape((gint) event->x, (gint) event->y);
      else
	 select_shape(widget, event);
   } else {
      do_popup_menu(event);
   }
}

static void
arrow_clicked(GtkWidget *widget, gpointer data)
{
   if (_callback_lock) {
      _callback_lock = FALSE;
   } else {
      set_arrow_func();
      menu_select_arrow();
      popup_select_arrow();
   }
}

static void
rectangle_clicked(GtkWidget *widget, gpointer data)
{
   if (_callback_lock) {
      _callback_lock = FALSE;
   } else {
      set_rectangle_func();
      menu_select_rectangle();
      popup_select_rectangle();
   }
}

static void
circle_clicked(GtkWidget *widget, gpointer data)
{
   if (_callback_lock) {
      _callback_lock = FALSE;
   } else {
      set_circle_func();
      menu_select_circle();
      popup_select_circle();
   }
}

static void
polygon_clicked(GtkWidget *widget, gpointer data)
{
   if (_callback_lock) {
      _callback_lock = FALSE;
   } else {
      set_polygon_func();
      menu_select_polygon();
      popup_select_polygon();
   }
}

Tools_t*
make_tools(GtkWidget *window)
{
   GtkWidget *handlebox;
   GtkWidget *toolbar;

   _tools.container = handlebox = gtk_handle_box_new();
   toolbar = gtk_toolbar_new(GTK_ORIENTATION_VERTICAL, GTK_TOOLBAR_ICONS);
   gtk_container_set_border_width(GTK_CONTAINER(toolbar), 5);
   gtk_toolbar_set_space_size(GTK_TOOLBAR(toolbar), 5);
   gtk_container_add(GTK_CONTAINER(handlebox), toolbar);

   _tools.arrow = make_toolbar_radio_icon(toolbar, window, NULL, arrow_xpm, 
					  "Select", _("Select existing area"), 
					  arrow_clicked, NULL);
   _tools.rectangle = make_toolbar_radio_icon(toolbar, window, _tools.arrow, 
					      rectangle_xpm, "Rectangle", 
					      _("Define Rectangle area"), 
					      rectangle_clicked, NULL);
   _tools.circle = make_toolbar_radio_icon(toolbar, window, _tools.rectangle, 
					   circle_xpm, "Circle",
					   _("Define Circle/Oval area"), 
					   circle_clicked, NULL);
   _tools.polygon = make_toolbar_radio_icon(toolbar, window, _tools.circle, 
					    polygon_xpm, "Polygon",
					    _("Define Polygon area"), 
					    polygon_clicked, NULL);
   gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
   _tools.edit = make_toolbar_icon(toolbar, window, edit_xpm, "Edit",
				   _("Edit selected area info"), tools_command,
				   &_tools.cmd_edit);
   gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
   _tools.delete = make_toolbar_icon(toolbar, window, delete_xpm, "Delete",
				     _("Delete selected area"), tools_command,
				     &_tools.cmd_delete);

   gtk_widget_show(toolbar);
   gtk_widget_show(handlebox);

   tools_set_sensitive(FALSE);
   set_arrow_func();

   return &_tools;
}

static void
tools_select(GtkWidget *widget)
{
   _callback_lock = TRUE;
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
   gtk_widget_grab_focus(widget);
}

void
tools_select_arrow(void)
{
   tools_select(_tools.arrow);
}

void
tools_select_rectangle(void)
{
   tools_select(_tools.rectangle);
}

void
tools_select_circle(void)
{
   tools_select(_tools.circle);
}

void
tools_select_polygon(void)
{
   tools_select(_tools.polygon);
}

void
tools_set_sensitive(gboolean sensitive)
{
   gtk_widget_set_sensitive(_tools.edit, sensitive);
   gtk_widget_set_sensitive(_tools.delete, sensitive);
}
