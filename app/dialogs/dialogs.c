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

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpitemfactory.h"

#include "dialogs.h"
#include "dialogs-constructors.h"
#include "menus.h"


GimpDialogFactory *global_dialog_factory  = NULL;
GimpDialogFactory *global_dock_factory    = NULL;
GimpDialogFactory *global_toolbox_factory = NULL;


static const GimpDialogFactoryEntry toplevel_entries[] =
{
  /*  foreign toplevels without constructor  */
  { "gimp-file-open-dialog",
    NULL, 0, TRUE,  TRUE,  TRUE,  FALSE },
  { "gimp-file-save-dialog",
    NULL, 0, TRUE,  TRUE,  TRUE,  FALSE },

  { "gimp-brightness-contrast-tool-dialog",
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-color-picker-tool-dialog",
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-colorize-tool-dialog",
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-crop-tool-dialog",
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-curves-tool-dialog",
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-color-balance-tool-dialog",
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-histogram-tool-dialog",
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-hue-saturation-tool-dialog",
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-levels-tool-dialog",
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-measure-tool-dialog",
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-posterize-tool-dialog",
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-rotate-tool-dialog",
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-scale-tool-dialog",
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-shear-tool-dialog",
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-text-tool-dialog",
    NULL, 0, TRUE,  TRUE,  TRUE,  FALSE },
  { "gimp-threshold-tool-dialog",
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-perspective-tool-dialog",
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },

  { "gimp-toolbox-color-dialog",
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-gradient-editor-color-dialog",
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-palette-editor-color-dialog",
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },

  { "gimp-colormap-editor-color-dialog",
    NULL, 0, FALSE, TRUE,  FALSE, FALSE },

  /*  ordinary toplevels  */
  { "gimp-file-new-dialog",        dialogs_file_new_new,
    0, FALSE, TRUE,  FALSE, FALSE },

  /*  singleton toplevels  */
  { "gimp-preferences-dialog",     dialogs_preferences_get,
    0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-module-manager-dialog",  dialogs_module_browser_get,
    0, TRUE,  TRUE,  TRUE,  FALSE },
  { "gimp-tips-dialog",            dialogs_tips_get,
    0, TRUE,  FALSE, FALSE, FALSE },
  { "gimp-about-dialog",           dialogs_about_get,
    0, TRUE,  FALSE, FALSE, FALSE }
};

static const GimpDialogFactoryEntry dock_entries[] =
{
  /*  singleton dockables  */
  { "gimp-tool-options",     dialogs_tool_options_get,
    0, TRUE,  FALSE, FALSE, TRUE },
  { "gimp-device-status",    dialogs_device_status_get,
    0, TRUE,  FALSE, FALSE, TRUE  },
  { "gimp-error-console",    dialogs_error_console_get,
    0, TRUE,  FALSE, FALSE, TRUE },

  /*  list views  */
  { "gimp-image-list",       dialogs_image_list_view_new,
    GIMP_PREVIEW_SIZE_MEDIUM, FALSE, FALSE, FALSE, TRUE },
  { "gimp-brush-list",       dialogs_brush_list_view_new,
    GIMP_PREVIEW_SIZE_MEDIUM, FALSE, FALSE, FALSE, TRUE },
  { "gimp-pattern-list",     dialogs_pattern_list_view_new,
    GIMP_PREVIEW_SIZE_MEDIUM, FALSE, FALSE, FALSE, TRUE },
  { "gimp-gradient-list",    dialogs_gradient_list_view_new,
    GIMP_PREVIEW_SIZE_MEDIUM, FALSE, FALSE, FALSE, TRUE },
  { "gimp-palette-list",     dialogs_palette_list_view_new,
    GIMP_PREVIEW_SIZE_MEDIUM, FALSE, FALSE, FALSE, TRUE },
  { "gimp-font-list",        dialogs_font_list_view_new,
    GIMP_PREVIEW_SIZE_MEDIUM, FALSE, FALSE, FALSE, TRUE },
  { "gimp-tool-list",        dialogs_tool_list_view_new,
    GIMP_PREVIEW_SIZE_SMALL, FALSE, FALSE, FALSE, TRUE },
  { "gimp-buffer-list",      dialogs_buffer_list_view_new,
    GIMP_PREVIEW_SIZE_MEDIUM, FALSE, FALSE, FALSE, TRUE },
  { "gimp-document-list",    dialogs_document_list_new,
    GIMP_PREVIEW_SIZE_LARGE,  FALSE, FALSE, FALSE, TRUE },
  { "gimp-template-list",    dialogs_template_list_new,
    GIMP_PREVIEW_SIZE_SMALL,  FALSE, FALSE, FALSE, TRUE },

  /*  grid views  */
  { "gimp-image-grid",       dialogs_image_grid_view_new,
    GIMP_PREVIEW_SIZE_MEDIUM, FALSE, FALSE, FALSE, TRUE },
  { "gimp-brush-grid",       dialogs_brush_grid_view_new,
    GIMP_PREVIEW_SIZE_MEDIUM, FALSE, FALSE, FALSE, TRUE },
  { "gimp-pattern-grid",     dialogs_pattern_grid_view_new,
    GIMP_PREVIEW_SIZE_MEDIUM, FALSE, FALSE, FALSE, TRUE },
  { "gimp-gradient-grid",    dialogs_gradient_grid_view_new,
    GIMP_PREVIEW_SIZE_MEDIUM, FALSE, FALSE, FALSE, TRUE },
  { "gimp-palette-grid",     dialogs_palette_grid_view_new,
    GIMP_PREVIEW_SIZE_MEDIUM, FALSE, FALSE, FALSE, TRUE },
  { "gimp-font-grid",        dialogs_font_grid_view_new,
    GIMP_PREVIEW_SIZE_MEDIUM, FALSE, FALSE, FALSE, TRUE },
  { "gimp-tool-grid",        dialogs_tool_grid_view_new,
    GIMP_PREVIEW_SIZE_SMALL, FALSE, FALSE, FALSE, TRUE },
  { "gimp-buffer-grid",      dialogs_buffer_grid_view_new,
    GIMP_PREVIEW_SIZE_MEDIUM, FALSE, FALSE, FALSE, TRUE },
  { "gimp-document-grid",    dialogs_document_grid_new,
    GIMP_PREVIEW_SIZE_LARGE,  FALSE, FALSE, FALSE, TRUE },

  /*  image related  */
  { "gimp-layer-list",       dialogs_layer_list_view_new,
    0, FALSE, FALSE, FALSE, TRUE },
  { "gimp-channel-list",     dialogs_channel_list_view_new,
    0, FALSE, FALSE, FALSE, TRUE },
  { "gimp-vectors-list",     dialogs_vectors_list_view_new,
    0, FALSE, FALSE, FALSE, TRUE },
  { "gimp-indexed-palette",  dialogs_indexed_palette_new,
    0, FALSE, FALSE, FALSE, TRUE },
  { "gimp-histogram-editor", dialogs_histogram_editor_new,
    0, FALSE, FALSE, FALSE, TRUE },
  { "gimp-selection-editor", dialogs_selection_editor_new,
    0, FALSE, FALSE, FALSE, TRUE },
  { "gimp-undo-history",     dialogs_undo_history_new,
    0, FALSE, FALSE, FALSE, TRUE },

  /*  display related  */
  { "gimp-navigation-view",  dialogs_navigation_view_new,
    0, FALSE, FALSE, FALSE, TRUE },

  /*  editors  */
  { "gimp-color-editor",     dialogs_color_editor_new,
    0, FALSE, FALSE, FALSE, TRUE },

  /*  singleton editors  */
  { "gimp-brush-editor",     dialogs_brush_editor_get,
    0, TRUE,  FALSE, FALSE, TRUE },
  { "gimp-gradient-editor",  dialogs_gradient_editor_get,
    0, TRUE,  FALSE, FALSE, TRUE },
  { "gimp-palette-editor",   dialogs_palette_editor_get,
    0, TRUE,  FALSE, FALSE, TRUE },
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

  global_toolbox_factory = gimp_dialog_factory_new ("toolbox",
                                                    gimp_get_user_context (gimp),
                                                    global_menu_factory,
                                                    dialogs_toolbox_get);

  global_dock_factory = gimp_dialog_factory_new ("dock",
						 gimp_get_user_context (gimp),
						 global_menu_factory,
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
  if (global_dialog_factory)
    {
      g_object_unref (global_dialog_factory);
      global_dialog_factory = NULL;
    }

  /*  destroy the "global_toolbox_factory" _before_ destroying the
   *  "global_dock_factory" because the "global_toolbox_factory" owns
   *  dockables which were created by the "global_dock_factory".  This
   *  way they are properly removed from the "global_dock_factory", which
   *  would complain about stale entries otherwise.
   */
  if (global_toolbox_factory)
    {
      g_object_unref (global_toolbox_factory);
      global_toolbox_factory = NULL;
    }

  if (global_dock_factory)
    {
      g_object_unref (global_dock_factory);
      global_dock_factory = NULL;
    }
}
