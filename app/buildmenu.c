/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
#include "appenv.h"
#include "buildmenu.h"
#include "interface.h"

#include "config.h"
#include "libgimp/gimpintl.h"

GtkWidget *
build_menu (MenuItem            *items,
	    GtkAccelGroup       *accel_group)
{
  GtkWidget *menu;
  GtkWidget *menu_item;

  menu = gtk_menu_new ();
  gtk_menu_set_accel_group (GTK_MENU (menu), accel_group);

  while (items->label)
    {
      if (items->label[0] == '-')
	{
	  menu_item = gtk_menu_item_new ();
	  gtk_container_add (GTK_CONTAINER (menu), menu_item);
	}
      else
	{
	  menu_item = gtk_menu_item_new_with_label (gettext(items->label));
	  gtk_container_add (GTK_CONTAINER (menu), menu_item);

	  if (items->accelerator_key && accel_group)
	    gtk_widget_add_accelerator (menu_item,
					"activate",
					accel_group,
					items->accelerator_key,
					items->accelerator_mods,
					GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);
	}

      if (items->callback)
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
			    (GtkSignalFunc) items->callback,
			    items->user_data);

      if (items->subitems)
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), build_menu (items->subitems, accel_group));

      gtk_widget_show (menu_item);
      items->widget = menu_item;

      items++;
    }

  return menu;
}
