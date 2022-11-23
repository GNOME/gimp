/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmawidgets/ligmawidgets.h"

#include "dialogs-types.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"

#include "config/ligmaguiconfig.h"

#include "widgets/ligmabrusheditor.h"
#include "widgets/ligmabrushfactoryview.h"
#include "widgets/ligmabufferview.h"
#include "widgets/ligmachanneltreeview.h"
#include "widgets/ligmacoloreditor.h"
#include "widgets/ligmacolormapeditor.h"
#include "widgets/ligmacriticaldialog.h"
#include "widgets/ligmadashboard.h"
#include "widgets/ligmadevicestatus.h"
#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmadockwindow.h"
#include "widgets/ligmadocumentview.h"
#include "widgets/ligmadynamicseditor.h"
#include "widgets/ligmadynamicsfactoryview.h"
#include "widgets/ligmaerrorconsole.h"
#include "widgets/ligmaerrordialog.h"
#include "widgets/ligmafontfactoryview.h"
#include "widgets/ligmagradienteditor.h"
#include "widgets/ligmahistogrameditor.h"
#include "widgets/ligmaimageview.h"
#include "widgets/ligmalayertreeview.h"
#include "widgets/ligmamenudock.h"
#include "widgets/ligmapaletteeditor.h"
#include "widgets/ligmapatternfactoryview.h"
#include "widgets/ligmasamplepointeditor.h"
#include "widgets/ligmaselectioneditor.h"
#include "widgets/ligmasymmetryeditor.h"
#include "widgets/ligmatemplateview.h"
#include "widgets/ligmatoolbox.h"
#include "widgets/ligmatooloptionseditor.h"
#include "widgets/ligmatoolpresetfactoryview.h"
#include "widgets/ligmatoolpreseteditor.h"
#include "widgets/ligmaundoeditor.h"
#include "widgets/ligmavectorstreeview.h"

#include "display/ligmacursorview.h"
#include "display/ligmanavigationeditor.h"

#include "about-dialog.h"
#include "action-search-dialog.h"
#include "dialogs.h"
#include "dialogs-constructors.h"
#include "extensions-dialog.h"
#include "file-open-dialog.h"
#include "file-open-location-dialog.h"
#include "file-save-dialog.h"
#include "image-new-dialog.h"
#include "input-devices-dialog.h"
#include "keyboard-shortcuts-dialog.h"
#include "module-dialog.h"
#include "palette-import-dialog.h"
#include "preferences-dialog.h"
#include "quit-dialog.h"
#include "tips-dialog.h"
#include "welcome-dialog.h"

#include "ligma-intl.h"


/**********************/
/*  toplevel dialogs  */
/**********************/

GtkWidget *
dialogs_image_new_new (LigmaDialogFactory *factory,
                       LigmaContext       *context,
                       LigmaUIManager     *ui_manager,
                       gint               view_size)
{
  return image_new_dialog_new (context);
}

GtkWidget *
dialogs_file_open_new (LigmaDialogFactory *factory,
                       LigmaContext       *context,
                       LigmaUIManager     *ui_manager,
                       gint               view_size)
{
  return file_open_dialog_new (context->ligma);
}

GtkWidget *
dialogs_file_open_location_new (LigmaDialogFactory *factory,
                                LigmaContext       *context,
                                LigmaUIManager     *ui_manager,
                                gint               view_size)
{
  return file_open_location_dialog_new (context->ligma);
}

GtkWidget *
dialogs_file_save_new (LigmaDialogFactory *factory,
                       LigmaContext       *context,
                       LigmaUIManager     *ui_manager,
                       gint               view_size)
{
  return file_save_dialog_new (context->ligma, FALSE);
}

GtkWidget *
dialogs_file_export_new (LigmaDialogFactory *factory,
                         LigmaContext       *context,
                         LigmaUIManager     *ui_manager,
                         gint               view_size)
{
  return file_save_dialog_new (context->ligma, TRUE);
}

GtkWidget *
dialogs_preferences_get (LigmaDialogFactory *factory,
                         LigmaContext       *context,
                         LigmaUIManager     *ui_manager,
                         gint               view_size)
{
  return preferences_dialog_create (context->ligma);
}

GtkWidget *
dialogs_extensions_get (LigmaDialogFactory *factory,
                        LigmaContext       *context,
                        LigmaUIManager     *ui_manager,
                        gint               view_size)
{
  return extensions_dialog_new (context->ligma);
}

GtkWidget *
dialogs_keyboard_shortcuts_get (LigmaDialogFactory *factory,
                                LigmaContext       *context,
                                LigmaUIManager     *ui_manager,
                                gint               view_size)
{
  return keyboard_shortcuts_dialog_new (context->ligma);
}

GtkWidget *
dialogs_input_devices_get (LigmaDialogFactory *factory,
                           LigmaContext       *context,
                           LigmaUIManager     *ui_manager,
                           gint               view_size)
{
  return input_devices_dialog_new (context->ligma);
}

GtkWidget *
dialogs_module_get (LigmaDialogFactory *factory,
                    LigmaContext       *context,
                    LigmaUIManager     *ui_manager,
                    gint               view_size)
{
  return module_dialog_new (context->ligma);
}

GtkWidget *
dialogs_palette_import_get (LigmaDialogFactory *factory,
                            LigmaContext       *context,
                            LigmaUIManager     *ui_manager,
                            gint               view_size)
{
  return palette_import_dialog_new (context);
}

GtkWidget *
dialogs_tips_get (LigmaDialogFactory *factory,
                  LigmaContext       *context,
                  LigmaUIManager     *ui_manager,
                  gint               view_size)
{
  return tips_dialog_create (context->ligma);
}

GtkWidget *
dialogs_welcome_get (LigmaDialogFactory *factory,
                     LigmaContext       *context,
                     LigmaUIManager     *ui_manager,
                     gint               view_size)
{
  return welcome_dialog_create (context->ligma);
}

GtkWidget *
dialogs_about_get (LigmaDialogFactory *factory,
                   LigmaContext       *context,
                   LigmaUIManager     *ui_manager,
                   gint               view_size)
{
  return about_dialog_create (context->ligma->edit_config);
}

GtkWidget *
dialogs_action_search_get (LigmaDialogFactory *factory,
                           LigmaContext       *context,
                           LigmaUIManager     *ui_manager,
                           gint               view_size)
{
  return action_search_dialog_create (context->ligma);
}

GtkWidget *
dialogs_error_get (LigmaDialogFactory *factory,
                   LigmaContext       *context,
                   LigmaUIManager     *ui_manager,
                   gint               view_size)
{
  return ligma_error_dialog_new (_("LIGMA Message"));
}

GtkWidget *
dialogs_critical_get (LigmaDialogFactory *factory,
                      LigmaContext       *context,
                      LigmaUIManager     *ui_manager,
                      gint               view_size)
{
  return ligma_critical_dialog_new (_("LIGMA Debug"),
                                   context->ligma->config->last_known_release,
                                   context->ligma->config->last_release_timestamp);
}

GtkWidget *
dialogs_close_all_get (LigmaDialogFactory *factory,
                       LigmaContext       *context,
                       LigmaUIManager     *ui_manager,
                       gint               view_size)
{
  return close_all_dialog_new (context->ligma);
}

GtkWidget *
dialogs_quit_get (LigmaDialogFactory *factory,
                  LigmaContext       *context,
                  LigmaUIManager     *ui_manager,
                  gint               view_size)
{
  return quit_dialog_new (context->ligma);
}


/***********/
/*  docks  */
/***********/

GtkWidget *
dialogs_toolbox_new (LigmaDialogFactory *factory,
                     LigmaContext       *context,
                     LigmaUIManager     *ui_manager,
                     gint               view_size)
{
  return ligma_toolbox_new (factory,
                           context,
                           ui_manager);
}

GtkWidget *
dialogs_toolbox_dock_window_new (LigmaDialogFactory *factory,
                                 LigmaContext       *context,
                                 LigmaUIManager     *ui_manager,
                                 gint               view_size)
{
  static gint  role_serial = 1;
  GtkWidget   *dock;
  gchar       *role;

  role = g_strdup_printf ("ligma-toolbox-%d", role_serial++);
  dock = ligma_dock_window_new (role,
                               "<Toolbox>",
                               TRUE,
                               factory,
                               context);
  g_free (role);

  return dock;
}

GtkWidget *
dialogs_dock_new (LigmaDialogFactory *factory,
                  LigmaContext       *context,
                  LigmaUIManager     *ui_manager,
                  gint               view_size)
{
  return ligma_menu_dock_new ();
}

GtkWidget *
dialogs_dock_window_new (LigmaDialogFactory *factory,
                         LigmaContext       *context,
                         LigmaUIManager     *ui_manager,
                         gint               view_size)
{
  static gint  role_serial = 1;
  GtkWidget   *dock;
  gchar       *role;

  role = g_strdup_printf ("ligma-dock-%d", role_serial++);
  dock = ligma_dock_window_new (role,
                               "<Dock>",
                               FALSE,
                               factory,
                               context);
  g_free (role);

  return dock;
}


/***************/
/*  dockables  */
/***************/

/*****  singleton dialogs  *****/

GtkWidget *
dialogs_tool_options_new (LigmaDialogFactory *factory,
                          LigmaContext       *context,
                          LigmaUIManager     *ui_manager,
                          gint               view_size)
{
  return ligma_tool_options_editor_new (context->ligma,
                                       ligma_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_device_status_new (LigmaDialogFactory *factory,
                           LigmaContext       *context,
                           LigmaUIManager     *ui_manager,
                           gint               view_size)
{
  return ligma_device_status_new (context->ligma);
}

GtkWidget *
dialogs_error_console_new (LigmaDialogFactory *factory,
                           LigmaContext       *context,
                           LigmaUIManager     *ui_manager,
                           gint               view_size)
{
  return ligma_error_console_new (context->ligma,
                                 ligma_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_cursor_view_new (LigmaDialogFactory *factory,
                         LigmaContext       *context,
                         LigmaUIManager     *ui_manager,
                         gint               view_size)
{
  return ligma_cursor_view_new (context->ligma,
                               ligma_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_dashboard_new (LigmaDialogFactory *factory,
                       LigmaContext       *context,
                       LigmaUIManager     *ui_manager,
                       gint               view_size)
{
  return ligma_dashboard_new (context->ligma,
                             ligma_dialog_factory_get_menu_factory (factory));
}


/*****  list views  *****/

GtkWidget *
dialogs_image_list_view_new (LigmaDialogFactory *factory,
                             LigmaContext       *context,
                             LigmaUIManager     *ui_manager,
                             gint               view_size)
{
  return ligma_image_view_new (LIGMA_VIEW_TYPE_LIST,
                              context->ligma->images,
                              context,
                              view_size, 1,
                              ligma_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_brush_list_view_new (LigmaDialogFactory *factory,
                             LigmaContext       *context,
                             LigmaUIManager     *ui_manager,
                             gint               view_size)
{
  return ligma_brush_factory_view_new (LIGMA_VIEW_TYPE_LIST,
                                      context->ligma->brush_factory,
                                      context,
                                      TRUE,
                                      view_size, 1,
                                      ligma_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_dynamics_list_view_new (LigmaDialogFactory *factory,
                                LigmaContext       *context,
                                LigmaUIManager     *ui_manager,
                                gint               view_size)
{
  return ligma_dynamics_factory_view_new (LIGMA_VIEW_TYPE_LIST,
                                         context->ligma->dynamics_factory,
                                         context,
                                         view_size, 1,
                                         ligma_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_mypaint_brush_list_view_new (LigmaDialogFactory *factory,
                                     LigmaContext       *context,
                                     LigmaUIManager     *ui_manager,
                                     gint               view_size)
{
  return ligma_data_factory_view_new (LIGMA_VIEW_TYPE_LIST,
                                     context->ligma->mybrush_factory,
                                     context,
                                     view_size, 1,
                                     ligma_dialog_factory_get_menu_factory (factory),
                                     "<MyPaintBrushes>",
                                     "/mypaint-brushes-popup",
                                     "mypaint-brushes");
}

GtkWidget *
dialogs_pattern_list_view_new (LigmaDialogFactory *factory,
                               LigmaContext       *context,
                               LigmaUIManager     *ui_manager,
                               gint               view_size)
{
  return ligma_pattern_factory_view_new (LIGMA_VIEW_TYPE_LIST,
                                        context->ligma->pattern_factory,
                                        context,
                                        view_size, 1,
                                        ligma_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_gradient_list_view_new (LigmaDialogFactory *factory,
                                LigmaContext       *context,
                                LigmaUIManager     *ui_manager,
                                gint               view_size)
{
  return ligma_data_factory_view_new (LIGMA_VIEW_TYPE_LIST,
                                     context->ligma->gradient_factory,
                                     context,
                                     view_size, 1,
                                     ligma_dialog_factory_get_menu_factory (factory),
                                     "<Gradients>",
                                     "/gradients-popup",
                                     "gradients");
}

GtkWidget *
dialogs_palette_list_view_new (LigmaDialogFactory *factory,
                               LigmaContext       *context,
                               LigmaUIManager     *ui_manager,
                               gint               view_size)
{
  return ligma_data_factory_view_new (LIGMA_VIEW_TYPE_LIST,
                                     context->ligma->palette_factory,
                                     context,
                                     view_size, 1,
                                     ligma_dialog_factory_get_menu_factory (factory),
                                     "<Palettes>",
                                     "/palettes-popup",
                                     "palettes");
}

GtkWidget *
dialogs_font_list_view_new (LigmaDialogFactory *factory,
                            LigmaContext       *context,
                            LigmaUIManager     *ui_manager,
                            gint               view_size)
{
  return ligma_font_factory_view_new (LIGMA_VIEW_TYPE_LIST,
                                     context->ligma->font_factory,
                                     context,
                                     view_size, 1,
                                     ligma_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_buffer_list_view_new (LigmaDialogFactory *factory,
                              LigmaContext       *context,
                              LigmaUIManager     *ui_manager,
                              gint               view_size)
{
  return ligma_buffer_view_new (LIGMA_VIEW_TYPE_LIST,
                               context->ligma->named_buffers,
                               context,
                               view_size, 1,
                               ligma_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_tool_preset_list_view_new (LigmaDialogFactory *factory,
                                   LigmaContext       *context,
                                   LigmaUIManager     *ui_manager,
                                   gint               view_size)
{
  return ligma_tool_preset_factory_view_new (LIGMA_VIEW_TYPE_LIST,
                                            context->ligma->tool_preset_factory,
                                            context,
                                            view_size, 1,
                                            ligma_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_document_list_view_new (LigmaDialogFactory *factory,
                                LigmaContext       *context,
                                LigmaUIManager     *ui_manager,
                                gint               view_size)
{
  return ligma_document_view_new (LIGMA_VIEW_TYPE_LIST,
                                 context->ligma->documents,
                                 context,
                                 view_size, 0,
                                 ligma_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_template_list_view_new (LigmaDialogFactory *factory,
                                LigmaContext       *context,
                                LigmaUIManager     *ui_manager,
                                gint               view_size)
{
  return ligma_template_view_new (LIGMA_VIEW_TYPE_LIST,
                                 context->ligma->templates,
                                 context,
                                 view_size, 0,
                                 ligma_dialog_factory_get_menu_factory (factory));
}


/*****  grid views  *****/

GtkWidget *
dialogs_image_grid_view_new (LigmaDialogFactory *factory,
                             LigmaContext       *context,
                             LigmaUIManager     *ui_manager,
                             gint               view_size)
{
  return ligma_image_view_new (LIGMA_VIEW_TYPE_GRID,
                              context->ligma->images,
                              context,
                              view_size, 1,
                              ligma_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_brush_grid_view_new (LigmaDialogFactory *factory,
                             LigmaContext       *context,
                             LigmaUIManager     *ui_manager,
                             gint               view_size)
{
  return ligma_brush_factory_view_new (LIGMA_VIEW_TYPE_GRID,
                                      context->ligma->brush_factory,
                                      context,
                                      TRUE,
                                      view_size, 1,
                                      ligma_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_dynamics_grid_view_new (LigmaDialogFactory *factory,
                                LigmaContext       *context,
                                LigmaUIManager     *ui_manager,
                                gint               view_size)
{
  return ligma_dynamics_factory_view_new (LIGMA_VIEW_TYPE_GRID,
                                         context->ligma->dynamics_factory,
                                         context,
                                         view_size, 1,
                                         ligma_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_mypaint_brush_grid_view_new (LigmaDialogFactory *factory,
                                     LigmaContext       *context,
                                     LigmaUIManager     *ui_manager,
                                     gint               view_size)
{
  return ligma_data_factory_view_new (LIGMA_VIEW_TYPE_GRID,
                                     context->ligma->mybrush_factory,
                                     context,
                                     view_size, 1,
                                     ligma_dialog_factory_get_menu_factory (factory),
                                     "<MyPaintBrushes>",
                                     "/mypaint-brushes-popup",
                                     "mypaint-brushes");
}

GtkWidget *
dialogs_pattern_grid_view_new (LigmaDialogFactory *factory,
                               LigmaContext       *context,
                               LigmaUIManager     *ui_manager,
                               gint               view_size)
{
  return ligma_pattern_factory_view_new (LIGMA_VIEW_TYPE_GRID,
                                        context->ligma->pattern_factory,
                                        context,
                                        view_size, 1,
                                        ligma_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_gradient_grid_view_new (LigmaDialogFactory *factory,
                                LigmaContext       *context,
                                LigmaUIManager     *ui_manager,
                                gint               view_size)
{
  return ligma_data_factory_view_new (LIGMA_VIEW_TYPE_GRID,
                                     context->ligma->gradient_factory,
                                     context,
                                     view_size, 1,
                                     ligma_dialog_factory_get_menu_factory (factory),
                                     "<Gradients>",
                                     "/gradients-popup",
                                     "gradients");
}

GtkWidget *
dialogs_palette_grid_view_new (LigmaDialogFactory *factory,
                               LigmaContext       *context,
                               LigmaUIManager     *ui_manager,
                               gint               view_size)
{
  return ligma_data_factory_view_new (LIGMA_VIEW_TYPE_GRID,
                                     context->ligma->palette_factory,
                                     context,
                                     view_size, 1,
                                     ligma_dialog_factory_get_menu_factory (factory),
                                     "<Palettes>",
                                     "/palettes-popup",
                                     "palettes");
}

GtkWidget *
dialogs_font_grid_view_new (LigmaDialogFactory *factory,
                            LigmaContext       *context,
                            LigmaUIManager     *ui_manager,
                            gint               view_size)
{
  return ligma_font_factory_view_new (LIGMA_VIEW_TYPE_GRID,
                                     context->ligma->font_factory,
                                     context,
                                     view_size, 1,
                                     ligma_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_buffer_grid_view_new (LigmaDialogFactory *factory,
                              LigmaContext       *context,
                              LigmaUIManager     *ui_manager,
                              gint               view_size)
{
  return ligma_buffer_view_new (LIGMA_VIEW_TYPE_GRID,
                               context->ligma->named_buffers,
                               context,
                               view_size, 1,
                               ligma_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_tool_preset_grid_view_new (LigmaDialogFactory *factory,
                                   LigmaContext       *context,
                                   LigmaUIManager     *ui_manager,
                                   gint               view_size)
{
  return ligma_tool_preset_factory_view_new (LIGMA_VIEW_TYPE_GRID,
                                            context->ligma->tool_preset_factory,
                                            context,
                                            view_size, 1,
                                            ligma_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_document_grid_view_new (LigmaDialogFactory *factory,
                                LigmaContext       *context,
                                LigmaUIManager     *ui_manager,
                                gint               view_size)
{
  return ligma_document_view_new (LIGMA_VIEW_TYPE_GRID,
                                 context->ligma->documents,
                                 context,
                                 view_size, 0,
                                 ligma_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_template_grid_view_new (LigmaDialogFactory *factory,
                                LigmaContext       *context,
                                LigmaUIManager     *ui_manager,
                                gint               view_size)
{
  return ligma_template_view_new (LIGMA_VIEW_TYPE_GRID,
                                 context->ligma->templates,
                                 context,
                                 view_size, 0,
                                 ligma_dialog_factory_get_menu_factory (factory));
}


/*****  image related dialogs  *****/

GtkWidget *
dialogs_layer_list_view_new (LigmaDialogFactory *factory,
                             LigmaContext       *context,
                             LigmaUIManager     *ui_manager,
                             gint               view_size)
{
  if (view_size < 1)
    view_size = context->ligma->config->layer_preview_size;

  return ligma_item_tree_view_new (LIGMA_TYPE_LAYER_TREE_VIEW,
                                  view_size, 2, TRUE,
                                  ligma_context_get_image (context),
                                  ligma_dialog_factory_get_menu_factory (factory),
                                  "<Layers>",
                                  "/layers-popup");
}

GtkWidget *
dialogs_channel_list_view_new (LigmaDialogFactory *factory,
                               LigmaContext       *context,
                               LigmaUIManager     *ui_manager,
                               gint               view_size)
{
  if (view_size < 1)
    view_size = context->ligma->config->layer_preview_size;

  return ligma_item_tree_view_new (LIGMA_TYPE_CHANNEL_TREE_VIEW,
                                  view_size, 1, TRUE,
                                  ligma_context_get_image (context),
                                  ligma_dialog_factory_get_menu_factory (factory),
                                  "<Channels>",
                                  "/channels-popup");
}

GtkWidget *
dialogs_vectors_list_view_new (LigmaDialogFactory *factory,
                               LigmaContext       *context,
                               LigmaUIManager     *ui_manager,
                               gint               view_size)
{
  if (view_size < 1)
    view_size = context->ligma->config->layer_preview_size;

  return ligma_item_tree_view_new (LIGMA_TYPE_VECTORS_TREE_VIEW,
                                  view_size, 1, TRUE,
                                  ligma_context_get_image (context),
                                  ligma_dialog_factory_get_menu_factory (factory),
                                  "<Vectors>",
                                  "/vectors-popup");
}

GtkWidget *
dialogs_colormap_editor_new (LigmaDialogFactory *factory,
                             LigmaContext       *context,
                             LigmaUIManager     *ui_manager,
                             gint               view_size)
{
  return ligma_colormap_editor_new (ligma_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_histogram_editor_new (LigmaDialogFactory *factory,
                              LigmaContext       *context,
                              LigmaUIManager     *ui_manager,
                              gint               view_size)
{
  return ligma_histogram_editor_new ();
}

GtkWidget *
dialogs_selection_editor_new (LigmaDialogFactory *factory,
                              LigmaContext       *context,
                              LigmaUIManager     *ui_manager,
                              gint               view_size)
{
  return ligma_selection_editor_new (ligma_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_symmetry_editor_new (LigmaDialogFactory *factory,
                             LigmaContext       *context,
                             LigmaUIManager     *ui_manager,
                             gint               view_size)
{
  return ligma_symmetry_editor_new (ligma_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_undo_editor_new (LigmaDialogFactory *factory,
                         LigmaContext       *context,
                         LigmaUIManager     *ui_manager,
                         gint               view_size)
{
  return ligma_undo_editor_new (context->ligma->config,
                               ligma_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_sample_point_editor_new (LigmaDialogFactory *factory,
                                 LigmaContext       *context,
                                 LigmaUIManager     *ui_manager,
                                 gint               view_size)
{
  return ligma_sample_point_editor_new (ligma_dialog_factory_get_menu_factory (factory));
}


/*****  display related dialogs  *****/

GtkWidget *
dialogs_navigation_editor_new (LigmaDialogFactory *factory,
                               LigmaContext       *context,
                               LigmaUIManager     *ui_manager,
                               gint               view_size)
{
  return ligma_navigation_editor_new (ligma_dialog_factory_get_menu_factory (factory));
}


/*****  misc dockables  *****/

GtkWidget *
dialogs_color_editor_new (LigmaDialogFactory *factory,
                          LigmaContext       *context,
                          LigmaUIManager     *ui_manager,
                          gint               view_size)
{
  return ligma_color_editor_new (context);
}


/*************/
/*  editors  */
/*************/

GtkWidget *
dialogs_brush_editor_get (LigmaDialogFactory *factory,
                          LigmaContext       *context,
                          LigmaUIManager     *ui_manager,
                          gint               view_size)
{
  return ligma_brush_editor_new (context,
                                ligma_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_dynamics_editor_get (LigmaDialogFactory *factory,
                             LigmaContext       *context,
                             LigmaUIManager     *ui_manager,
                             gint               view_size)
{
  return ligma_dynamics_editor_new (context,
                                   ligma_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_gradient_editor_get (LigmaDialogFactory *factory,
                             LigmaContext       *context,
                             LigmaUIManager     *ui_manager,
                             gint               view_size)
{
  return ligma_gradient_editor_new (context,
                                   ligma_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_palette_editor_get (LigmaDialogFactory *factory,
                            LigmaContext       *context,
                            LigmaUIManager     *ui_manager,
                            gint               view_size)
{
  return ligma_palette_editor_new (context,
                                  ligma_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_tool_preset_editor_get (LigmaDialogFactory *factory,
                                LigmaContext       *context,
                                LigmaUIManager     *ui_manager,
                                gint               view_size)
{
  return ligma_tool_preset_editor_new (context,
                                      ligma_dialog_factory_get_menu_factory (factory));
}
