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

#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmenufactory.h"

#include "dialogs.h"
#include "dialogs-constructors.h"

#include "gimp-intl.h"


GimpDialogFactory *global_dialog_factory  = NULL;
GimpDialogFactory *global_dock_factory    = NULL;
GimpDialogFactory *global_toolbox_factory = NULL;


static const GimpDialogFactoryEntry toplevel_entries[] =
{
  /*  foreign toplevels without constructor  */
  { "gimp-brightness-contrast-tool-dialog", NULL, NULL, NULL, NULL,
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-color-picker-tool-dialog", NULL, NULL, NULL, NULL,
    NULL, 0, TRUE,  TRUE,  TRUE, FALSE },
  { "gimp-colorize-tool-dialog", NULL, NULL, NULL, NULL,
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-crop-tool-dialog", NULL, NULL, NULL, NULL,
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-curves-tool-dialog", NULL, NULL, NULL, NULL,
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-color-balance-tool-dialog", NULL, NULL, NULL, NULL,
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-hue-saturation-tool-dialog", NULL, NULL, NULL, NULL,
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-levels-tool-dialog", NULL, NULL, NULL, NULL,
    NULL, 0, TRUE,  TRUE,  TRUE, FALSE },
  { "gimp-measure-tool-dialog", NULL, NULL, NULL, NULL,
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-posterize-tool-dialog", NULL, NULL, NULL, NULL,
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-rotate-tool-dialog", NULL, NULL, NULL, NULL,
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-scale-tool-dialog", NULL, NULL, NULL, NULL,
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-shear-tool-dialog", NULL, NULL, NULL, NULL,
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-text-tool-dialog", NULL, NULL, NULL, NULL,
    NULL, 0, TRUE,  TRUE,  TRUE,  FALSE },
  { "gimp-threshold-tool-dialog", NULL, NULL, NULL, NULL,
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-perspective-tool-dialog", NULL, NULL, NULL, NULL,
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },

  { "gimp-toolbox-color-dialog", NULL, NULL, NULL, NULL,
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-gradient-editor-color-dialog", NULL, NULL, NULL, NULL,
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-palette-editor-color-dialog", NULL, NULL, NULL, NULL,
    NULL, 0, TRUE,  TRUE,  FALSE, FALSE },

  { "gimp-colormap-editor-color-dialog", NULL, NULL, NULL, NULL,
    NULL, 0, FALSE, TRUE,  FALSE, FALSE },

  /*  ordinary toplevels  */
  { "gimp-image-new-dialog", NULL, NULL, NULL, NULL,
    dialogs_image_new_new, 0,
    FALSE, TRUE,  FALSE, FALSE },
  { "gimp-file-open-dialog", NULL, NULL, NULL, NULL,
    dialogs_file_open_new, 0,
    TRUE,  TRUE,  TRUE,  FALSE },
  { "gimp-file-open-location-dialog", NULL, NULL, NULL, NULL,
    dialogs_file_open_location_new, 0,
    FALSE, TRUE,  FALSE, FALSE },
  { "gimp-file-save-dialog", NULL, NULL, NULL, NULL,
    dialogs_file_save_new, 0,
    FALSE, TRUE,  TRUE,  FALSE },

  /*  singleton toplevels  */
  { "gimp-preferences-dialog", NULL, NULL, NULL, NULL,
    dialogs_preferences_get, 0,
    TRUE,  TRUE,  FALSE, FALSE },
  { "gimp-module-dialog", NULL, NULL, NULL, NULL,
    dialogs_module_get, 0,
    TRUE,  TRUE,  TRUE,  FALSE },
  { "gimp-tips-dialog", NULL, NULL, NULL, NULL,
    dialogs_tips_get, 0,
    TRUE,  FALSE, FALSE, FALSE },
  { "gimp-about-dialog", NULL, NULL, NULL, NULL,
    dialogs_about_get, 0,
    TRUE,  FALSE, FALSE, FALSE },
  { "gimp-error-dialog", NULL, NULL, NULL, NULL,
    dialogs_error_get, 0,
    TRUE,  FALSE, FALSE, FALSE },
  { "gimp-quit-dialog", NULL, NULL, NULL, NULL,
    dialogs_quit_get, 0,
    TRUE,  FALSE, FALSE, FALSE }
};

static const GimpDialogFactoryEntry dock_entries[] =
{
  /*  singleton dockables  */
  { "gimp-tool-options",
    N_("Tool Options"), NULL, GIMP_STOCK_TOOL_OPTIONS,
    GIMP_HELP_TOOL_OPTIONS_DIALOG,
    dialogs_tool_options_get, 0,
    TRUE,  FALSE, FALSE, TRUE },
  { "gimp-device-status",
    N_("Devices"), N_("Device Status"), GIMP_STOCK_DEVICE_STATUS,
    GIMP_HELP_DEVICE_STATUS_DIALOG,
    dialogs_device_status_get, 0,
    TRUE,  FALSE, FALSE, TRUE },
  { "gimp-error-console",
    N_("Errors"), N_("Error Console"), GIMP_STOCK_WARNING,
    GIMP_HELP_ERRORS_DIALOG,
    dialogs_error_console_get, 0,
    TRUE,  FALSE, FALSE, TRUE },

  /*  list views  */
  { "gimp-image-list",
    N_("Images"), NULL, GIMP_STOCK_IMAGES,
    GIMP_HELP_IMAGE_DIALOG,
    dialogs_image_list_view_new, GIMP_VIEW_SIZE_MEDIUM,
    FALSE, FALSE, FALSE, TRUE },
  { "gimp-brush-list",
    N_("Brushes"), NULL, GIMP_STOCK_BRUSH,
    GIMP_HELP_BRUSH_DIALOG,
    dialogs_brush_list_view_new, GIMP_VIEW_SIZE_MEDIUM,
    FALSE, FALSE, FALSE, TRUE },
  { "gimp-pattern-list",
    N_("Patterns"), NULL, GIMP_STOCK_PATTERN,
    GIMP_HELP_PATTERN_DIALOG,
    dialogs_pattern_list_view_new, GIMP_VIEW_SIZE_MEDIUM,
    FALSE, FALSE, FALSE, TRUE },
  { "gimp-gradient-list",
    N_("Gradients"), NULL, GIMP_STOCK_GRADIENT,
    GIMP_HELP_GRADIENT_DIALOG,
    dialogs_gradient_list_view_new, GIMP_VIEW_SIZE_MEDIUM,
    FALSE, FALSE, FALSE, TRUE },
  { "gimp-palette-list",
    N_("Palettes"), NULL, GIMP_STOCK_PALETTE,
    GIMP_HELP_PALETTE_DIALOG,
    dialogs_palette_list_view_new, GIMP_VIEW_SIZE_MEDIUM,
    FALSE, FALSE, FALSE, TRUE },
  { "gimp-font-list",
    N_("Fonts"), NULL, GIMP_STOCK_FONT,
    GIMP_HELP_FONT_DIALOG,
    dialogs_font_list_view_new, GIMP_VIEW_SIZE_MEDIUM,
    FALSE, FALSE, FALSE, TRUE },
  { "gimp-tool-list",
    N_("Tools"), NULL, GIMP_STOCK_TOOLS,
    GIMP_HELP_TOOLS_DIALOG,
    dialogs_tool_list_view_new, GIMP_VIEW_SIZE_SMALL,
    FALSE, FALSE, FALSE, TRUE },
  { "gimp-buffer-list",
    N_("Buffers"), NULL, GIMP_STOCK_BUFFER,
    GIMP_HELP_BUFFER_DIALOG,
    dialogs_buffer_list_view_new, GIMP_VIEW_SIZE_MEDIUM,
    FALSE, FALSE, FALSE, TRUE },
  { "gimp-document-list",
    N_("History"), N_("Document History"), GTK_STOCK_OPEN,
    GIMP_HELP_DOCUMENT_DIALOG,
    dialogs_document_list_new, GIMP_VIEW_SIZE_LARGE,
    FALSE, FALSE, FALSE, TRUE },
  { "gimp-template-list",
    N_("Templates"), N_("Image Templates"), GIMP_STOCK_TEMPLATE,
    GIMP_HELP_TEMPLATE_DIALOG,
    dialogs_template_list_new, GIMP_VIEW_SIZE_SMALL,
    FALSE, FALSE, FALSE, TRUE },

  /*  grid views  */
  { "gimp-image-grid",
    N_("Images"), NULL, GIMP_STOCK_IMAGES,
    GIMP_HELP_IMAGE_DIALOG,
    dialogs_image_grid_view_new, GIMP_VIEW_SIZE_MEDIUM,
    FALSE, FALSE, FALSE, TRUE },
  { "gimp-brush-grid",
    N_("Brushes"), NULL, GIMP_STOCK_BRUSH,
    GIMP_HELP_BRUSH_DIALOG,
    dialogs_brush_grid_view_new, GIMP_VIEW_SIZE_MEDIUM,
    FALSE, FALSE, FALSE, TRUE },
  { "gimp-pattern-grid",
    N_("Patterns"), NULL, GIMP_STOCK_PATTERN,
    GIMP_HELP_PATTERN_DIALOG,
    dialogs_pattern_grid_view_new, GIMP_VIEW_SIZE_MEDIUM,
    FALSE, FALSE, FALSE, TRUE },
  { "gimp-gradient-grid",
    N_("Gradients"), NULL, GIMP_STOCK_GRADIENT,
    GIMP_HELP_GRADIENT_DIALOG,
    dialogs_gradient_grid_view_new, GIMP_VIEW_SIZE_MEDIUM,
    FALSE, FALSE, FALSE, TRUE },
  { "gimp-palette-grid",
    N_("Palettes"), NULL, GIMP_STOCK_PALETTE,
    GIMP_HELP_PALETTE_DIALOG,
    dialogs_palette_grid_view_new, GIMP_VIEW_SIZE_MEDIUM,
    FALSE, FALSE, FALSE, TRUE },
  { "gimp-font-grid",
    N_("Fonts"), NULL, GIMP_STOCK_FONT,
    GIMP_HELP_FONT_DIALOG,
    dialogs_font_grid_view_new, GIMP_VIEW_SIZE_MEDIUM,
    FALSE, FALSE, FALSE, TRUE },
  { "gimp-tool-grid",
    N_("Tools"), NULL, GIMP_STOCK_TOOLS,
    GIMP_HELP_TOOLS_DIALOG,
    dialogs_tool_grid_view_new, GIMP_VIEW_SIZE_SMALL,
    FALSE, FALSE, FALSE, TRUE },
  { "gimp-buffer-grid",
    N_("Buffers"), NULL, GIMP_STOCK_BUFFER,
    GIMP_HELP_BUFFER_DIALOG,
    dialogs_buffer_grid_view_new, GIMP_VIEW_SIZE_MEDIUM,
    FALSE, FALSE, FALSE, TRUE },
  { "gimp-document-grid",
    N_("History"), N_("Document History"), GTK_STOCK_OPEN,
    GIMP_HELP_DOCUMENT_DIALOG,
    dialogs_document_grid_new, GIMP_VIEW_SIZE_LARGE,
    FALSE, FALSE, FALSE, TRUE },

  /*  image related  */
  { "gimp-layer-list",
    N_("Layers"), NULL, GIMP_STOCK_LAYERS,
    GIMP_HELP_LAYER_DIALOG,
    dialogs_layer_list_view_new, 0,
    FALSE, FALSE, FALSE, TRUE },
  { "gimp-channel-list",
    N_("Channels"), NULL, GIMP_STOCK_CHANNELS,
    GIMP_HELP_CHANNEL_DIALOG,
    dialogs_channel_list_view_new, 0,
    FALSE, FALSE, FALSE, TRUE },
  { "gimp-vectors-list",
    N_("Paths"), NULL, GIMP_STOCK_PATHS,
    GIMP_HELP_PATH_DIALOG,
    dialogs_vectors_list_view_new, 0,
    FALSE, FALSE, FALSE, TRUE },
  { "gimp-indexed-palette",
    N_("Colormap"), N_("Indexed Palette"), GIMP_STOCK_INDEXED_PALETTE,
    GIMP_HELP_INDEXED_PALETTE_DIALOG,
    dialogs_colormap_editor_new, 0,
    FALSE, FALSE, FALSE, TRUE },
  { "gimp-histogram-editor",
    N_("Histogram"), NULL, GIMP_STOCK_HISTOGRAM,
    GIMP_HELP_HISTOGRAM_DIALOG,
    dialogs_histogram_editor_new, 0,
    FALSE, FALSE, FALSE, TRUE },
  { "gimp-selection-editor",
    N_("Selection"), N_("Selection Editor"), GIMP_STOCK_TOOL_RECT_SELECT,
    GIMP_HELP_SELECTION_DIALOG,
    dialogs_selection_editor_new, 0,
    FALSE, FALSE, FALSE, TRUE },
  { "gimp-undo-history",
    N_("Undo"), N_("Undo History"), GIMP_STOCK_UNDO_HISTORY,
    GIMP_HELP_UNDO_DIALOG,
    dialogs_undo_editor_new, 0,
    FALSE, FALSE, FALSE, TRUE },

  /*  display related  */
  { "gimp-navigation-view",
    N_("Navigation"), N_("Display Navigation"), GIMP_STOCK_NAVIGATION,
    GIMP_HELP_NAVIGATION_DIALOG,
    dialogs_navigation_editor_new, 0,
    FALSE, FALSE, FALSE, TRUE },

  /*  editors  */
  { "gimp-color-editor",
    N_("FG/BG"), N_("FG/BG Color"), GIMP_STOCK_DEFAULT_COLORS,
    GIMP_HELP_COLOR_DIALOG,
    dialogs_color_editor_new, 0,
    FALSE, FALSE, FALSE, TRUE },

  /*  singleton editors  */
  { "gimp-brush-editor",
    N_("Brush Editor"), NULL, GIMP_STOCK_BRUSH,
    GIMP_HELP_BRUSH_EDITOR_DIALOG,
    dialogs_brush_editor_get, 0,
    TRUE,  FALSE, FALSE, TRUE },
  { "gimp-gradient-editor",
    N_("Gradient Editor"), NULL, GIMP_STOCK_GRADIENT,
    GIMP_HELP_GRADIENT_EDITOR_DIALOG,
    dialogs_gradient_editor_get, 0,
    TRUE,  FALSE, FALSE, TRUE },
  { "gimp-palette-editor",
    N_("Palette Editor"), NULL, GIMP_STOCK_PALETTE,
    GIMP_HELP_PALETTE_EDITOR_DIALOG,
    dialogs_palette_editor_get, 0,
    TRUE,  FALSE, FALSE, TRUE }
};


/*  public functions  */

void
dialogs_init (Gimp            *gimp,
              GimpMenuFactory *menu_factory)
{
  gint i;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_MENU_FACTORY (menu_factory));

  global_dialog_factory = gimp_dialog_factory_new ("toplevel",
						   gimp_get_user_context (gimp),
						   NULL,
						   NULL);

  global_toolbox_factory = gimp_dialog_factory_new ("toolbox",
                                                    gimp_get_user_context (gimp),
                                                    menu_factory,
                                                    dialogs_toolbox_get);
  gimp_dialog_factory_set_constructor (global_toolbox_factory,
                                       dialogs_dockable_constructor);

  global_dock_factory = gimp_dialog_factory_new ("dock",
						 gimp_get_user_context (gimp),
						 menu_factory,
						 dialogs_dock_new);
  gimp_dialog_factory_set_constructor (global_dock_factory,
                                       dialogs_dockable_constructor);

  for (i = 0; i < G_N_ELEMENTS (toplevel_entries); i++)
    gimp_dialog_factory_register_entry (global_dialog_factory,
					toplevel_entries[i].identifier,
					gettext (toplevel_entries[i].name),
					gettext (toplevel_entries[i].blurb),
					toplevel_entries[i].stock_id,
					toplevel_entries[i].help_id,
					toplevel_entries[i].new_func,
					toplevel_entries[i].preview_size,
					toplevel_entries[i].singleton,
					toplevel_entries[i].session_managed,
					toplevel_entries[i].remember_size,
					toplevel_entries[i].remember_if_open);

  for (i = 0; i < G_N_ELEMENTS (dock_entries); i++)
    gimp_dialog_factory_register_entry (global_dock_factory,
					dock_entries[i].identifier,
					gettext (dock_entries[i].name),
					gettext (dock_entries[i].blurb),
					dock_entries[i].stock_id,
					dock_entries[i].help_id,
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
  g_return_if_fail (GIMP_IS_GIMP (gimp));

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

GtkWidget *
dialogs_get_toolbox (void)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_DIALOG_FACTORY (global_toolbox_factory), NULL);

  for (list = global_toolbox_factory->open_dialogs;
       list;
       list = g_list_next (list))
    {
      if (GTK_WIDGET_TOPLEVEL (list->data))
        return list->data;
    }

  return NULL;
}
