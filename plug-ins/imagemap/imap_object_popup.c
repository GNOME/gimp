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

#include "imap_cmd_copy_object.h"
#include "imap_cmd_cut_object.h"
#include "imap_cmd_object_down.h"
#include "imap_cmd_object_up.h"
#include "imap_main.h"
#include "imap_object_popup.h"

/* Current object with popup menu */
static Object_t *_current_obj;

Object_t*
get_popup_object(void)
{
   return _current_obj;
}

static void
popup_edit_area_info(GtkWidget *widget, gpointer data)
{
   object_edit(_current_obj, TRUE);
}

static void
popup_delete_area(GtkWidget *widget, gpointer data)
{
   if (_current_obj->locked) {
      do_object_locked_dialog();
   } else {
      object_remove(_current_obj);
      redraw_preview();
   }
}

static void
popup_move_up(GtkWidget *widget, gpointer data)
{
   Command_t *command = object_up_command_new(_current_obj->list, 
					      _current_obj);
   command_execute(command);
}

static void
popup_move_down(GtkWidget *widget, gpointer data)
{
   Command_t *command = object_down_command_new(_current_obj->list,
						_current_obj);
   command_execute(command);
}

static void
popup_cut(GtkWidget *widget, gpointer data)
{
   if (_current_obj->locked) {
      do_object_locked_dialog();
   } else {
      Command_t *command = cut_object_command_new(_current_obj);
      command_execute(command);
   }
}

static void
popup_copy(GtkWidget *widget, gpointer data)
{
   Command_t *command = copy_object_command_new(_current_obj);
   command_execute(command);
}

ObjectPopup_t*
make_object_popup(void)
{
   ObjectPopup_t *popup = g_new(ObjectPopup_t, 1);
   GtkWidget *menu;

   popup->menu = menu = gtk_menu_new();
   make_item_with_label(menu, "Edit Area Info...", popup_edit_area_info, NULL);
   make_item_with_label(menu, "Delete Area", popup_delete_area, NULL);
   popup->up = make_item_with_label(menu, "Move Up", popup_move_up, NULL);
   popup->down = make_item_with_label(menu, "Move Down", popup_move_down, 
				      NULL);
   make_item_with_label(menu, "Cut", popup_cut, NULL);
   make_item_with_label(menu, "Copy", popup_copy, NULL);

   return popup;
}

GtkWidget*
object_popup_prepend_menu(ObjectPopup_t *popup, gchar *label, 
			  MenuCallback activate, gpointer data)
{
   return prepend_item_with_label(popup->menu, label, activate, data);
}

void
object_handle_popup(ObjectPopup_t *popup, Object_t *obj, GdkEventButton *event)
{
   int position = object_get_position_in_list(obj) + 1;

   _current_obj = popup->obj = obj;
   gtk_widget_set_sensitive(popup->up, (position > 1) ? TRUE : FALSE);
   gtk_widget_set_sensitive(popup->down, 
			    (position < g_list_length(obj->list->list)) 
			    ? TRUE : FALSE);

   gtk_menu_popup(GTK_MENU(popup->menu), NULL, NULL, NULL, NULL,
		  event->button, event->time);
}

void 
object_do_popup(Object_t *obj, GdkEventButton *event)
{
   static ObjectPopup_t *popup;

   if (!popup)
      popup = make_object_popup();
   object_handle_popup(popup, obj, event);
}
