/*
 * This is a plug-in for the GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2004 Maurits Rijk  m.rijk@chello.nl
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

#include <gtk/gtk.h>

#include "imap_circle.h"
#include "imap_edit_area_info.h"
#include "imap_main.h"
#include "imap_menu.h"
#include "imap_misc.h"
#include "imap_polygon.h"
#include "imap_popup.h"
#include "imap_rectangle.h"
#include "imap_stock.h"
#include "imap_tools.h"

#include "libgimp/stdplugins-intl.h"
#include "libgimpwidgets/gimpstock.h"

static gboolean _callback_lock;
static Tools_t _tools;

static void
tools_command(GtkWidget *widget, gpointer data)
{
   CommandFactory_t *factory = (CommandFactory_t*) data;
   Command_t *command = (*factory)();
   command_execute(command);
}

gboolean
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
   return FALSE;
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

#ifdef _NOT_READY_YET_
static void
fuzzy_select_clicked(GtkWidget *widget, gpointer data)
{
   if (_callback_lock) {
      _callback_lock = FALSE;
   } else {
      set_fuzzy_select_func();
      menu_select_fuzzy_select();
      popup_select_fuzzy_select();
   }
}
#endif

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

   toolbar = gtk_toolbar_new();
   gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
   _tools.container = handlebox = gtk_handle_box_new();
   gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar), GTK_ORIENTATION_VERTICAL);
   gtk_container_set_border_width(GTK_CONTAINER(toolbar), 0);

   gtk_container_add(GTK_CONTAINER(handlebox), toolbar);

   _tools.arrow = make_toolbar_radio_icon(toolbar, IMAP_STOCK_ARROW,
					  NULL, _("Select"),
                                          _("Select existing area"),
					  arrow_clicked, NULL);
#ifdef _NOT_READY_YET_
   _tools.fuzzy_select =
      make_toolbar_radio_icon(toolbar, GIMP_STOCK_TOOL_FUZZY_SELECT,
			      _tools.arrow, _("Fuzzy Select"),
			      _("Select contiguous regions"),
			      fuzzy_select_clicked, NULL);
#endif
   _tools.rectangle = make_toolbar_radio_icon(toolbar, IMAP_STOCK_RECTANGLE,
					      _tools.arrow,
					      _("Rectangle"),
					      _("Define Rectangle area"),
					      rectangle_clicked, NULL);
   _tools.circle = make_toolbar_radio_icon(toolbar, IMAP_STOCK_CIRCLE,
					   _tools.rectangle, _("Circle"),
					   _("Define Circle/Oval area"),
					   circle_clicked, NULL);
   _tools.polygon = make_toolbar_radio_icon(toolbar, IMAP_STOCK_POLYGON,
					    _tools.circle, _("Polygon"),
					    _("Define Polygon area"),
					    polygon_clicked, NULL);
   toolbar_add_space(toolbar);
   _tools.edit = make_toolbar_stock_icon(toolbar, GTK_STOCK_PROPERTIES,
                                   _("Edit"),
				   _("Edit selected area info"), tools_command,
				   &_tools.cmd_edit);
   toolbar_add_space(toolbar);
   _tools.delete = make_toolbar_stock_icon(toolbar, GTK_STOCK_DELETE,
                                     _("Delete"),
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
   gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (widget), TRUE);
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
