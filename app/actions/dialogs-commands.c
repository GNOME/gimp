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

#include "config.h"

#include <gtk/gtk.h>

#include "apptypes.h"
#include "widgets/widgets-types.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdockable.h"
#include "widgets/gimpdockbook.h"
#include "widgets/gimpimagedock.h"

#include "dialogs.h"
#include "dialogs-commands.h"


void
dialogs_create_toplevel_cmd_callback (GtkWidget *widget,
				      gpointer   data,
				      guint      action)
{
  if (action)
    {
      const gchar *identifier;

      identifier = g_quark_to_string ((GQuark) action);

      if (identifier)
	gimp_dialog_factory_dialog_new (global_dialog_factory, identifier);
    }
}

void
dialogs_create_dockable_cmd_callback (GtkWidget *widget,
				      gpointer   data,
				      guint      action)
{
  if (action)
    {
      const gchar *identifier;

      identifier = g_quark_to_string ((GQuark) action);

      if (identifier)
	gimp_dialog_factory_dialog_raise (global_dock_factory, identifier);
    }
}

void
dialogs_add_tab_cmd_callback (GtkWidget *widget,
			      gpointer   data,
			      guint      action)
{
  GimpDockbook *dockbook;

  dockbook = (GimpDockbook *) gtk_item_factory_popup_data_from_widget (widget);

  if (dockbook && action)
    {
      GtkWidget   *dockable;
      const gchar *identifier;

      identifier = g_quark_to_string ((GQuark) action);

      if (identifier)
	{
	  dockable = gimp_dialog_factory_dockable_new (dockbook->dock->factory,
						       dockbook->dock,
						       identifier);

	  /*  Maybe gimp_dialog_factory_dockable_new() returned an already
	   *  existing singleton dockable, so check if it already is
	   *  attached to a dockbook.
	   */
	  if (dockable && ! GIMP_DOCKABLE (dockable)->dockbook)
	    gimp_dockbook_add (dockbook, GIMP_DOCKABLE (dockable), -1);
	}
    }
}

void
dialogs_remove_tab_cmd_callback (GtkWidget *widget,
				 gpointer   data,
				 guint      action)
{
  GimpDockbook *dockbook;

  dockbook = (GimpDockbook *) gtk_item_factory_popup_data_from_widget (widget);

  if (dockbook)
    {
      GimpDockable *dockable;
      gint          page_num;

      page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (dockbook));

      dockable = (GimpDockable *)
	gtk_notebook_get_nth_page (GTK_NOTEBOOK (dockbook), page_num);

      if (dockable)
	gimp_dockbook_remove (dockbook, dockable);
    }
}

void
dialogs_toggle_image_menu_cmd_callback (GtkWidget *widget,
					gpointer   data,
					guint      action)
{
  GimpDockbook *dockbook;

  dockbook = (GimpDockbook *) gtk_item_factory_popup_data_from_widget (widget);

  if (dockbook)
    {
      gimp_image_dock_set_show_image_menu (GIMP_IMAGE_DOCK (dockbook->dock),
					   GTK_CHECK_MENU_ITEM (widget)->active);
    }
}

void
dialogs_toggle_auto_cmd_callback (GtkWidget *widget,
				  gpointer   data,
				  guint      action)
{
  GimpDockbook *dockbook;

  dockbook = (GimpDockbook *) gtk_item_factory_popup_data_from_widget (widget);

  if (dockbook)
    {
      gimp_image_dock_set_auto_follow_active (GIMP_IMAGE_DOCK (dockbook->dock),
					      GTK_CHECK_MENU_ITEM (widget)->active);
    }
}

void
dialogs_create_lc_cmd_callback (GtkWidget *widget,
				 gpointer   data,
				 guint      action)
{
  GtkWidget *dock;
  GtkWidget *dockbook;
  GtkWidget *dockable;

  dock = gimp_dialog_factory_dock_new (global_dock_factory);

  dockbook = gimp_dockbook_new ();

  gimp_dock_add_book (GIMP_DOCK (dock), GIMP_DOCKBOOK (dockbook), 0);

  dockable = gimp_dialog_factory_dialog_new (global_dock_factory,
					     "gimp:layer-list");
  if (dockable)
    gimp_dock_add (GIMP_DOCK (dock), GIMP_DOCKABLE (dockable), -1, -1);

  dockable = gimp_dialog_factory_dialog_new (global_dock_factory,
					     "gimp:channel-list");
  if (dockable)
    gimp_dock_add (GIMP_DOCK (dock), GIMP_DOCKABLE (dockable), -1, -1);

  dockable = gimp_dialog_factory_dialog_new (global_dock_factory,
					     "gimp:path-list");
  if (dockable)
    gimp_dock_add (GIMP_DOCK (dock), GIMP_DOCKABLE (dockable), -1, -1);

  gtk_widget_show (dock);
}
