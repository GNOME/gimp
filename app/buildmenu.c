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

GtkWidget *
build_menu (MenuItem            *items,
	    GtkAcceleratorTable *table)
{
  GtkWidget *menu;
  GtkWidget *menu_item;

  menu = gtk_menu_new ();
  gtk_menu_set_accelerator_table (GTK_MENU (menu), table);

  while (items->label)
    {
      if (items->label[0] == '-')
	{
	  menu_item = gtk_menu_item_new ();
	  gtk_container_add (GTK_CONTAINER (menu), menu_item);
	}
      else
	{
	  menu_item = gtk_menu_item_new_with_label (items->label);
	  gtk_container_add (GTK_CONTAINER (menu), menu_item);

	  if (items->accelerator_key && table)
	    gtk_widget_install_accelerator (menu_item,
					    table,
					    "activate",
					    items->accelerator_key,
					    items->accelerator_mods);
	}

      if (items->callback)
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
			    (GtkSignalFunc) items->callback,
			    items->user_data);

      if (items->subitems)
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), build_menu (items->subitems, table));

      gtk_widget_show (menu_item);
      items->widget = menu_item;

      items++;
    }

  return menu;
}
