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


typedef struct _DialogEntry DialogEntry;

struct _DialogEntry
{
  const gchar       *identifier;
  GimpDialogNewFunc  new_func;
  gboolean           singleton;
  gboolean           session_managed;
};


GimpDialogFactory *global_dialog_factory     = NULL;
GimpDialogFactory *global_dock_factory       = NULL;
GimpDialogFactory *global_image_dock_factory = NULL;


static const DialogEntry toplevel_entries[] =
{
  { "gimp:toolbox",                dialogs_toolbox_get,         TRUE, TRUE },
  { "gimp:lc-dialog",              dialogs_lc_get,              TRUE, TRUE },
  { "gimp:tool-options-dialog",    dialogs_tool_options_get,    TRUE, TRUE },
  { "gimp:device-status-dialog",   dialogs_device_status_get,   TRUE, TRUE },
  { "gimp:brush-select-dialog",    dialogs_brush_select_get,    TRUE, TRUE },
  { "gimp:pattern-select-dialog",  dialogs_pattern_select_get,  TRUE, TRUE },
  { "gimp:gradient-select-dialog", dialogs_gradient_select_get, TRUE, TRUE },
  { "gimp:palette-dialog",         dialogs_palette_get,         TRUE, TRUE },
  { "gimp:error-console-dialog",   dialogs_error_console_get,   TRUE, TRUE },
  { "gimp:document-index-dialog",  dialogs_document_index_get,  TRUE, TRUE }
};
static const gint n_toplevel_entries = (sizeof (toplevel_entries) /
					sizeof (toplevel_entries[0]));

static const DialogEntry dock_entries[] =
{
  { "gimp:image-list",    dialogs_image_list_view_new,    FALSE, FALSE },
  { "gimp:brush-list",    dialogs_brush_list_view_new,    FALSE, FALSE },
  { "gimp:pattern-list",  dialogs_pattern_list_view_new,  FALSE, FALSE },
  { "gimp:gradient-list", dialogs_gradient_list_view_new, FALSE, FALSE },
  { "gimp:palette-list",  dialogs_palette_list_view_new,  FALSE, FALSE },
  { "gimp:tool-list",     dialogs_tool_list_view_new,     FALSE, FALSE },
  { "gimp:image-grid",    dialogs_image_grid_view_new,    FALSE, FALSE },
  { "gimp:brush-grid",    dialogs_brush_grid_view_new,    FALSE, FALSE },
  { "gimp:pattern-grid",  dialogs_pattern_grid_view_new,  FALSE, FALSE },
  { "gimp:gradient-grid", dialogs_gradient_grid_view_new, FALSE, FALSE },
  { "gimp:palette-grid",  dialogs_palette_grid_view_new,  FALSE, FALSE },
  { "gimp:tool-grid",     dialogs_tool_grid_view_new,     FALSE, FALSE }
};
static const gint n_dock_entries = (sizeof (dock_entries) /
				    sizeof (dock_entries[0]));

/*
static const DialogEntry image_dock_entries[] =
{
};
static const gint n_image_dock_entries = (sizeof (image_dock_entries) /
					  sizeof (image_dock_entries[0]));
*/


/*  public functions  */

void
dialogs_init (void)
{
  gint i;

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

  for (i = 0; i < n_toplevel_entries; i++)
    gimp_dialog_factory_register (global_dialog_factory,
				  toplevel_entries[i].identifier,
				  toplevel_entries[i].new_func,
				  toplevel_entries[i].singleton,
				  toplevel_entries[i].session_managed);

  for (i = 0; i < n_dock_entries; i++)
    gimp_dialog_factory_register (global_dock_factory,
				  dock_entries[i].identifier,
				  dock_entries[i].new_func,
				  dock_entries[i].singleton,
				  dock_entries[i].session_managed);

  /*
  for (i = 0; i < n_image_dock_entries; i++)
    gimp_dialog_factory_register (global_image_dock_factory,
				  image_dock_entries[i].identifier,
				  image_dock_entries[i].new_func,
				  image_dock_entries[i].singleton,
				  image_dock_entries[i].session_managed);
  */
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
