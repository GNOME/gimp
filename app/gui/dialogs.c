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

#include "widgets/widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"

#include "widgets/gimpdialogfactory.h"

#include "dialogs.h"
#include "dialogs-constructors.h"
#include "menus.h"


GimpDialogFactory *global_dialog_factory = NULL;
GimpDialogFactory *global_dock_factory   = NULL;


static const GimpDialogFactoryEntry toplevel_entries[] =
{
  { "gimp:toolbox",                dialogs_toolbox_get,         32, TRUE,  TRUE,  TRUE,  TRUE  },
  { "gimp:tool-options-dialog",    dialogs_tool_options_get,    32, TRUE,  TRUE,  FALSE, TRUE  },
  { "gimp:device-status-dialog",   dialogs_device_status_get,   32, TRUE,  TRUE,  FALSE, TRUE  },
  { "gimp:brush-select-dialog",    dialogs_brush_select_get,    32, TRUE,  TRUE,  FALSE, TRUE  },
  { "gimp:pattern-select-dialog",  dialogs_pattern_select_get,  32, TRUE,  TRUE,  FALSE, TRUE  },
  { "gimp:gradient-select-dialog", dialogs_gradient_select_get, 32, TRUE,  TRUE,  FALSE, TRUE  },
  { "gimp:palette-select-dialog",  dialogs_palette_select_get,  32, TRUE,  TRUE,  FALSE, TRUE  },
  { "gimp:preferences-dialog",     dialogs_preferences_get,     32, TRUE,  FALSE, FALSE, TRUE  },
  { "gimp:input-devices-dialog",   dialogs_input_devices_get,   32, TRUE,  FALSE, FALSE, TRUE  },
  { "gimp:module-browser-dialog",  dialogs_module_browser_get,  32, TRUE,  FALSE, FALSE, TRUE  },
  { "gimp:undo-history-dialog",    dialogs_undo_history_get,    32, FALSE, FALSE, FALSE, TRUE  },
  { "gimp:display-filters-dialog", dialogs_display_filters_get, 32, FALSE, FALSE, FALSE, TRUE  },
  { "gimp:tips-dialog",            dialogs_tips_get,            32, TRUE,  FALSE, FALSE, TRUE  },
  { "gimp:about-dialog",           dialogs_about_get,           32, TRUE,  FALSE, FALSE, TRUE  },

  { "gimp:brush-editor",           dialogs_brush_editor_get,    32, TRUE,  TRUE,  TRUE,  FALSE },
  { "gimp:gradient-editor",        dialogs_gradient_editor_get, 32, TRUE,  TRUE,  TRUE,  FALSE },
  { "gimp:palette-editor",         dialogs_palette_editor_get,  32, TRUE,  TRUE,  TRUE,  FALSE }
};

static const GimpDialogFactoryEntry dock_entries[] =
{
  { "gimp:image-list",       dialogs_image_list_view_new,    32, FALSE, FALSE, FALSE, TRUE },
  { "gimp:brush-list",       dialogs_brush_list_view_new,    32, FALSE, FALSE, FALSE, TRUE },
  { "gimp:pattern-list",     dialogs_pattern_list_view_new,  32, FALSE, FALSE, FALSE, TRUE },
  { "gimp:gradient-list",    dialogs_gradient_list_view_new, 16, FALSE, FALSE, FALSE, TRUE },
  { "gimp:palette-list",     dialogs_palette_list_view_new,  32, FALSE, FALSE, FALSE, TRUE },
  { "gimp:tool-list",        dialogs_tool_list_view_new,     24, FALSE, FALSE, FALSE, TRUE },
  { "gimp:buffer-list",      dialogs_buffer_list_view_new,   32, FALSE, FALSE, FALSE, TRUE },

  { "gimp:image-grid",       dialogs_image_grid_view_new,    32, FALSE, FALSE, FALSE, TRUE },
  { "gimp:brush-grid",       dialogs_brush_grid_view_new,    32, FALSE, FALSE, FALSE, TRUE },
  { "gimp:pattern-grid",     dialogs_pattern_grid_view_new,  32, FALSE, FALSE, FALSE, TRUE },
  { "gimp:gradient-grid",    dialogs_gradient_grid_view_new, 16, FALSE, FALSE, FALSE, TRUE },
  { "gimp:palette-grid",     dialogs_palette_grid_view_new,  32, FALSE, FALSE, FALSE, TRUE },
  { "gimp:tool-grid",        dialogs_tool_grid_view_new,     24, FALSE, FALSE, FALSE, TRUE },
  { "gimp:buffer-grid",      dialogs_buffer_grid_view_new,   32, FALSE, FALSE, FALSE, TRUE },

  { "gimp:layer-list",       dialogs_layer_list_view_new,    32, FALSE, FALSE, FALSE, TRUE },
  { "gimp:channel-list",     dialogs_channel_list_view_new,  32,FALSE, FALSE, FALSE, TRUE },
  { "gimp:path-list",        dialogs_path_list_view_new,     32, TRUE,  FALSE, FALSE, TRUE },
  { "gimp:indexed-palette",  dialogs_indexed_palette_new,    32, FALSE, FALSE, FALSE, TRUE },

  { "gimp:document-history", dialogs_document_history_new,   48, FALSE, FALSE, FALSE, TRUE },

  { "gimp:error-console",    dialogs_error_console_get,      32, TRUE,  FALSE, FALSE, TRUE }
};


/*  public functions  */

void
dialogs_init (Gimp *gimp)
{
  gint i;

  global_dialog_factory = gimp_dialog_factory_new ("toplevel",
						   gimp_get_user_context (gimp),
						   NULL,
						   NULL);

  global_dock_factory = gimp_dialog_factory_new ("dock",
						 gimp_get_user_context (gimp),
						 menus_get_dialogs_factory (),
						 dialogs_dock_new);

  for (i = 0; i < G_N_ELEMENTS (toplevel_entries); i++)
    gimp_dialog_factory_register_entry (global_dialog_factory,
					toplevel_entries[i].identifier,
					toplevel_entries[i].new_func,
					toplevel_entries[i].preview_size,
					toplevel_entries[i].singleton,
					toplevel_entries[i].session_managed,
					toplevel_entries[i].remember_size,
					toplevel_entries[i].remember_if_open);

  for (i = 0; i < G_N_ELEMENTS (dock_entries); i++)
    gimp_dialog_factory_register_entry (global_dock_factory,
					dock_entries[i].identifier,
					dock_entries[i].new_func,
					dock_entries[i].preview_size,
					dock_entries[i].singleton,
					dock_entries[i].session_managed,
					dock_entries[i].remember_size,
					dock_entries[i].remember_if_open);
}

void
dialogs_exit (Gimp *gimp)
{
  g_object_unref (G_OBJECT (global_dialog_factory));
  g_object_unref (G_OBJECT (global_dock_factory));

  global_dialog_factory = NULL;
  global_dock_factory   = NULL;
}
