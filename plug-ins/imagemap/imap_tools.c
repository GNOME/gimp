/*
 * This is a plug-in for the GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2005 Maurits Rijk  m.rijk@chello.nl
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
#include "imap_tools.h"

#include "libgimp/stdplugins-intl.h"
#include "libgimpwidgets/gimpstock.h"

static Tools_t _tools;

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

extern GtkUIManager *ui_manager;

Tools_t*
make_tools(GtkWidget *window)
{
  GtkWidget *handlebox, *toolbar;

  _tools.container = handlebox = gtk_handle_box_new();
  toolbar = gtk_ui_manager_get_widget (ui_manager, "/Tools");
  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar), GTK_ORIENTATION_VERTICAL);
  gtk_container_set_border_width(GTK_CONTAINER(toolbar), 0);

  gtk_container_add(GTK_CONTAINER(handlebox), toolbar);
  gtk_widget_show(toolbar);
  gtk_widget_show(handlebox);

  return &_tools;
}
