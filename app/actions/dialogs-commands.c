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

#include "widgets/gimpdock.h"
#include "widgets/gimpdockbook.h"

#include "dialogs-commands.h"
#include "dialogs-constructors.h"
#include "gimpdialogfactory.h"


void
dialogs_add_tab_cmd_callback (GtkWidget *widget,
			      gpointer   data,
			      guint      action)
{
  GimpDockbook *dockbook;

  dockbook = (GimpDockbook *) gtk_item_factory_popup_data_from_widget (widget);

  if (dockbook && action)
    {
      GimpDockable *dockable;

      dockable = gimp_dialog_factory_dialog_new (dockbook->dock->factory,
						 GUINT_TO_POINTER (action));

      if (dockable)
	gimp_dockbook_add (dockbook, dockable, -1);
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
      g_print ("remove\n");
    }
}
