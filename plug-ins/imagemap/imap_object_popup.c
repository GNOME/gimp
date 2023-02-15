/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2005 Maurits Rijk  m.rijk@chello.nl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "config.h"

#include <gtk/gtk.h>

#include "imap_commands.h"
#include "imap_main.h"
#include "imap_menu.h"
#include "imap_object_popup.h"

#include "libgimp/stdplugins-intl.h"

void
object_handle_popup (ObjectPopup_t  *popup,
                     Object_t       *obj,
                     GdkEventButton *event,
                     GimpImap       *imap)
{
  /* int position = object_get_position_in_list(obj) + 1; */

#ifdef _TEMP_
   gtk_widget_set_sensitive(popup->up, (position > 1) ? TRUE : FALSE);
   gtk_widget_set_sensitive(popup->down,
                            (position < g_list_length(obj->list->list))
                            ? TRUE : FALSE);
#endif
   gtk_menu_attach_to_widget (GTK_MENU (popup->menu), GTK_WIDGET (imap->dlg), NULL);
   gtk_menu_popup_at_pointer (GTK_MENU (popup->menu), (GdkEvent *) event);
}

void
object_do_popup (Object_t       *obj,
                 GdkEventButton *event,
                 gpointer        data)
{
  static ObjectPopup_t *popup;
  GMenuModel           *model;
  GimpImap             *imap = GIMP_IMAP (data);

   if (! popup)
     {
       popup       = g_new (ObjectPopup_t, 1);
       model       = G_MENU_MODEL (gtk_builder_get_object
                                    (imap->builder, "imap-object-popup"));
       popup->menu = gtk_menu_new_from_model (model);
     }
   object_handle_popup (popup, obj, event, imap);
}
