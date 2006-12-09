/* GIMP - The GNU Image Manipulation Program
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


#define FOREIGN(id,singleton,remember_size) \
  { id, NULL, NULL, NULL, NULL, \
    NULL, 0, singleton,  TRUE, remember_size, FALSE }

#define TOPLEVEL(id,new_func,singleton,session_managed,remember_size) \
  { id, NULL, NULL, NULL, NULL, \
    new_func, 0, singleton, session_managed, remember_size, FALSE }


static const GimpDialogFactoryEntry toplevel_entries[] =
{
  /*  foreign toplevels without constructor  */
  FOREIGN ("gimp-brightness-contrast-tool-dialog", TRUE,  FALSE),
  FOREIGN ("gimp-color-picker-tool-dialog",        TRUE,  TRUE),
  FOREIGN ("gimp-colorize-tool-dialog",            TRUE,  FALSE),
  FOREIGN ("gimp-crop-tool-dialog",                TRUE,  FALSE),
  FOREIGN ("gimp-curves-tool-dialog",              TRUE,  TRUE),
  FOREIGN ("gimp-color-balance-tool-dialog",       TRUE,  FALSE),
  FOREIGN ("gimp-hue-saturation-tool-dialog",      TRUE,  FALSE),
  FOREIGN ("gimp-levels-tool-dialog",              TRUE,  TRUE),
  FOREIGN ("gimp-measure-tool-dialog",             TRUE,  FALSE),
  FOREIGN ("gimp-posterize-tool-dialog",           TRUE,  FALSE),
  FOREIGN ("gimp-rotate-tool-dialog",              TRUE,  FALSE),
  FOREIGN ("gimp-scale-tool-dialog",               TRUE,  FALSE),
  FOREIGN ("gimp-shear-tool-dialog",               TRUE,  FALSE),
  FOREIGN ("gimp-text-tool-dialog",                TRUE,  TRUE),
  FOREIGN ("gimp-threshold-tool-dialog",           TRUE,  FALSE),
  FOREIGN ("gimp-perspective-tool-dialog",         TRUE,  FALSE),

  FOREIGN ("gimp-toolbox-color-dialog",            TRUE,  FALSE),
  FOREIGN ("gimp-gradient-editor-color-dialog",    TRUE,  FALSE),
  FOREIGN ("gimp-palette-editor-color-dialog",     TRUE,  FALSE),
  FOREIGN ("gimp-colormap-editor-color-dialog",    TRUE,  FALSE),

  FOREIGN ("gimp-controller-editor-dialog",        FALSE, TRUE),
  FOREIGN ("gimp-controller-action-dialog",        FALSE, TRUE),

  /*  ordinary toplevels  */
  TOPLEVEL ("gimp-image-new-dialog",
            dialogs_image_new_new,          FALSE, TRUE, FALSE),
  TOPLEVEL ("gimp-file-open-dialog",
            dialogs_file_open_new,          TRUE,  TRUE, TRUE),
  TOPLEVEL ("gimp-file-open-location-dialog",
            dialogs_file_open_location_new, FALSE, TRUE, FALSE),
  TOPLEVEL ("gimp-file-save-dialog",
            dialogs_file_save_new,          FALSE, TRUE, TRUE),

  /*  singleton toplevels  */
  TOPLEVEL ("gimp-preferences-dialog",
            dialogs_preferences_get,        TRUE, TRUE,  FALSE),
  TOPLEVEL ("gimp-keyboard-shortcuts-dialog",
            dialogs_keyboard_shortcuts_get, TRUE, TRUE,  TRUE),
  TOPLEVEL ("gimp-module-dialog",
            dialogs_module_get,             TRUE, TRUE,  TRUE),
  TOPLEVEL ("gimp-palette-import-dialog",
            dialogs_palette_import_get,     TRUE, TRUE,  TRUE),
  TOPLEVEL ("gimp-tips-dialog",
            dialogs_tips_get,               TRUE, FALSE, FALSE),
  TOPLEVEL ("gimp-about-dialog",
            dialogs_about_get,              TRUE, FALSE, FALSE),
  TOPLEVEL ("gimp-error-dialog",
            dialogs_error_get,              TRUE, FALSE, FALSE),
  TOPLEVEL ("gimp-close-all-dialog",
            dialogs_close_all_get,          TRUE, FALSE, FALSE),
  TOPLEVEL ("gimp-quit-dialog",
            dialogs_quit_get,               TRUE, FALSE, FALSE)
};

#define DOCKABLE(id,name,blurb,stock_id,help_id,\
                 new_func,view_size,singleton) \
  { id, name, blurb, stock_id, help_id, \
    new_func, view_size, singleton, FALSE, FALSE, TRUE }

#define LISTGRID(id,name,blurb,stock_id,help_id,\
                 view_size) \
  { "gimp-"#id"-list", name, blurb, stock_id, help_id, \
    dialogs_##id##_list_view_new, view_size, FALSE, FALSE, FALSE, TRUE }, \
  { "gimp-"#id"-grid", name, blurb, stock_id, help_id, \
    dialogs_##id##_grid_view_new, view_size, FALSE, FALSE, FALSE, TRUE }

static const GimpDialogFactoryEntry dock_entries[] =
{
  /*  singleton dockables  */
  DOCKABLE ("gimp-tool-options",
            N_("Tool Options"), NULL, GIMP_STOCK_TOOL_OPTIONS,
            GIMP_HELP_TOOL_OPTIONS_DIALOG,
            dialogs_tool_options_new, 0, TRUE),
  DOCKABLE ("gimp-device-status",
            N_("Devices"), N_("Device Status"), GIMP_STOCK_DEVICE_STATUS,
            GIMP_HELP_DEVICE_STATUS_DIALOG,
            dialogs_device_status_new, 0, TRUE),
  DOCKABLE ("gimp-error-console",
            N_("Errors"), N_("Error Console"), GIMP_STOCK_WARNING,
            GIMP_HELP_ERRORS_DIALOG,
            dialogs_error_console_new, 0, TRUE),
  DOCKABLE ("gimp-cursor-view",
            N_("Pointer"), N_("Pointer Information"), GIMP_STOCK_CURSOR,
            GIMP_HELP_POINTER_INFO_DIALOG,
            dialogs_cursor_view_new, 0, TRUE),

  /*  list & grid views  */
  LISTGRID (image, N_("Images"), NULL, GIMP_STOCK_IMAGES,
            GIMP_HELP_IMAGE_DIALOG, GIMP_VIEW_SIZE_MEDIUM),
  LISTGRID (brush, N_("Brushes"), NULL, GIMP_STOCK_BRUSH,
            GIMP_HELP_BRUSH_DIALOG, GIMP_VIEW_SIZE_MEDIUM),
  LISTGRID (pattern, N_("Patterns"), NULL, GIMP_STOCK_PATTERN,
            GIMP_HELP_PATTERN_DIALOG, GIMP_VIEW_SIZE_MEDIUM),
  LISTGRID (gradient, N_("Gradients"), NULL, GIMP_STOCK_GRADIENT,
            GIMP_HELP_GRADIENT_DIALOG, GIMP_VIEW_SIZE_MEDIUM),
  LISTGRID (palette, N_("Palettes"), NULL, GIMP_STOCK_PALETTE,
            GIMP_HELP_PALETTE_DIALOG, GIMP_VIEW_SIZE_MEDIUM),
  LISTGRID (font, N_("Fonts"), NULL, GIMP_STOCK_FONT,
            GIMP_HELP_FONT_DIALOG, GIMP_VIEW_SIZE_MEDIUM),
  LISTGRID (tool, N_("Tools"), NULL, GIMP_STOCK_TOOLS,
            GIMP_HELP_TOOLS_DIALOG, GIMP_VIEW_SIZE_SMALL),
  LISTGRID (buffer, N_("Buffers"), NULL, GIMP_STOCK_BUFFER,
            GIMP_HELP_BUFFER_DIALOG, GIMP_VIEW_SIZE_MEDIUM),
  LISTGRID (document, N_("History"), N_("Document History"), GTK_STOCK_OPEN,
            GIMP_HELP_DOCUMENT_DIALOG, GIMP_VIEW_SIZE_LARGE),
  LISTGRID (template, N_("Templates"), N_("Image Templates"), GIMP_STOCK_TEMPLATE,
            GIMP_HELP_TEMPLATE_DIALOG, GIMP_VIEW_SIZE_SMALL),

  /*  image related  */
  DOCKABLE ("gimp-layer-list",
            N_("Layers"), NULL, GIMP_STOCK_LAYERS,
            GIMP_HELP_LAYER_DIALOG,
            dialogs_layer_list_view_new, 0, FALSE),
  DOCKABLE ("gimp-channel-list",
            N_("Channels"), NULL, GIMP_STOCK_CHANNELS,
            GIMP_HELP_CHANNEL_DIALOG,
            dialogs_channel_list_view_new, 0, FALSE),
  DOCKABLE ("gimp-vectors-list",
            N_("Paths"), NULL, GIMP_STOCK_PATHS,
            GIMP_HELP_PATH_DIALOG,
            dialogs_vectors_list_view_new, 0, FALSE),
  DOCKABLE ("gimp-indexed-palette",
            N_("Colormap"), NULL, GIMP_STOCK_COLORMAP,
            GIMP_HELP_INDEXED_PALETTE_DIALOG,
            dialogs_colormap_editor_new, 0, FALSE),
  DOCKABLE ("gimp-histogram-editor",
            N_("Histogram"), NULL, GIMP_STOCK_HISTOGRAM,
            GIMP_HELP_HISTOGRAM_DIALOG,
            dialogs_histogram_editor_new, 0, FALSE),
  DOCKABLE ("gimp-selection-editor",
            N_("Selection"), N_("Selection Editor"), GIMP_STOCK_SELECTION,
            GIMP_HELP_SELECTION_DIALOG,
            dialogs_selection_editor_new, 0, FALSE),
  DOCKABLE ("gimp-undo-history",
            N_("Undo"), N_("Undo History"), GIMP_STOCK_UNDO_HISTORY,
            GIMP_HELP_UNDO_DIALOG,
            dialogs_undo_editor_new, 0, FALSE),
  DOCKABLE ("gimp-sample-point-editor",
            N_("Sample Points"), N_("Sample Points"), GIMP_STOCK_SAMPLE_POINT,
            GIMP_HELP_SAMPLE_POINT_DIALOG,
            dialogs_sample_point_editor_new, 0, FALSE),

  /*  display related  */
  DOCKABLE ("gimp-navigation-view",
            N_("Navigation"), N_("Display Navigation"), GIMP_STOCK_NAVIGATION,
            GIMP_HELP_NAVIGATION_DIALOG,
            dialogs_navigation_editor_new, 0, FALSE),

  /*  editors  */
  DOCKABLE ("gimp-color-editor",
            N_("FG/BG"), N_("FG/BG Color"), GIMP_STOCK_DEFAULT_COLORS,
            GIMP_HELP_COLOR_DIALOG,
            dialogs_color_editor_new, 0, FALSE),

  /*  singleton editors  */
  DOCKABLE ("gimp-brush-editor",
            N_("Brush Editor"), NULL, GIMP_STOCK_BRUSH,
            GIMP_HELP_BRUSH_EDITOR_DIALOG,
            dialogs_brush_editor_get, 0, TRUE),
  DOCKABLE ("gimp-gradient-editor",
            N_("Gradient Editor"), NULL, GIMP_STOCK_GRADIENT,
            GIMP_HELP_GRADIENT_EDITOR_DIALOG,
            dialogs_gradient_editor_get, 0, TRUE),
  DOCKABLE ("gimp-palette-editor",
            N_("Palette Editor"), NULL, GIMP_STOCK_PALETTE,
            GIMP_HELP_PALETTE_EDITOR_DIALOG,
            dialogs_palette_editor_get, 0, TRUE)
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
                                                   menu_factory,
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
                                        toplevel_entries[i].view_size,
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
                                        dock_entries[i].view_size,
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
