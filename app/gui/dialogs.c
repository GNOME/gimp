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

#include "widgets/gimpdialogfactory.h"

#include "dialogs.h"
#include "dialogs-constructors.h"
#include "menus.h"

#include "gimpcontext.h"


GimpDialogFactory *global_dialog_factory     = NULL;
GimpDialogFactory *global_dock_factory       = NULL;
GimpDialogFactory *global_image_dock_factory = NULL;


void
dialogs_init (void)
{
  global_dialog_factory =
    gimp_dialog_factory_new ("toplevel",
			     gimp_context_get_user (),
			     NULL);

  global_dock_factory =
    gimp_dialog_factory_new ("dock",
			     gimp_context_get_user (),
			     menus_get_dialogs_factory ());

  global_image_dock_factory =
    gimp_dialog_factory_new ("image-dock",
			     gimp_context_get_user (),
			     NULL);

  gimp_dialog_factory_register (global_dock_factory,
				"gimp:image-list",
				dialogs_image_list_view_new);
  gimp_dialog_factory_register (global_dock_factory,
				"gimp:brush-list",
				dialogs_brush_list_view_new);
  gimp_dialog_factory_register (global_dock_factory,
				"gimp:pattern-list",
				dialogs_pattern_list_view_new);
  gimp_dialog_factory_register (global_dock_factory,
				"gimp:gradient-list",
				dialogs_gradient_list_view_new);
  gimp_dialog_factory_register (global_dock_factory,
				"gimp:palette-list",
				dialogs_palette_list_view_new);
  gimp_dialog_factory_register (global_dock_factory,
				"gimp:tool-list",
				dialogs_tool_list_view_new);

  gimp_dialog_factory_register (global_dock_factory,
				"gimp:image-grid",
				dialogs_image_grid_view_new);
  gimp_dialog_factory_register (global_dock_factory,
				"gimp:brush-grid",
				dialogs_brush_grid_view_new);
  gimp_dialog_factory_register (global_dock_factory,
				"gimp:pattern-grid",
				dialogs_pattern_grid_view_new);
  gimp_dialog_factory_register (global_dock_factory,
				"gimp:gradient-grid",
				dialogs_gradient_grid_view_new);
  gimp_dialog_factory_register (global_dock_factory,
				"gimp:palette-grid",
				dialogs_palette_grid_view_new);
  gimp_dialog_factory_register (global_dock_factory,
				"gimp:tool-grid",
				dialogs_tool_grid_view_new);
}

void
dialogs_exit (void)
{
  gtk_object_unref (GTK_OBJECT (global_dialog_factory));
  gtk_object_unref (GTK_OBJECT (global_dock_factory));
  gtk_object_unref (GTK_OBJECT (global_image_dock_factory));

  global_dialog_factory     = NULL;
  global_dock_factory       = NULL;
  global_image_dock_factory = NULL;
}
