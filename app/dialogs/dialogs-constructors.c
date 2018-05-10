/* GIMP - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"

#include "config/gimpguiconfig.h"

#include "widgets/gimpbrusheditor.h"
#include "widgets/gimpbrushfactoryview.h"
#include "widgets/gimpbufferview.h"
#include "widgets/gimpchanneltreeview.h"
#include "widgets/gimpcoloreditor.h"
#include "widgets/gimpcolormapeditor.h"
#include "widgets/gimpcriticaldialog.h"
#include "widgets/gimpdashboard.h"
#include "widgets/gimpdevicestatus.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdockwindow.h"
#include "widgets/gimpdocumentview.h"
#include "widgets/gimpdynamicseditor.h"
#include "widgets/gimpdynamicsfactoryview.h"
#include "widgets/gimperrorconsole.h"
#include "widgets/gimperrordialog.h"
#include "widgets/gimpfontview.h"
#include "widgets/gimpgradienteditor.h"
#include "widgets/gimphistogrameditor.h"
#include "widgets/gimpimageview.h"
#include "widgets/gimplayertreeview.h"
#include "widgets/gimpmenudock.h"
#include "widgets/gimppaletteeditor.h"
#include "widgets/gimppatternfactoryview.h"
#include "widgets/gimpsamplepointeditor.h"
#include "widgets/gimpselectioneditor.h"
#include "widgets/gimpsymmetryeditor.h"
#include "widgets/gimptemplateview.h"
#include "widgets/gimptoolbox.h"
#include "widgets/gimptooloptionseditor.h"
#include "widgets/gimptoolpresetfactoryview.h"
#include "widgets/gimptoolpreseteditor.h"
#include "widgets/gimpundoeditor.h"
#include "widgets/gimpvectorstreeview.h"

#include "display/gimpcursorview.h"
#include "display/gimpnavigationeditor.h"

#include "about-dialog.h"
#include "action-search-dialog.h"
#include "dialogs.h"
#include "dialogs-constructors.h"
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

#include "gimp-intl.h"


/**********************/
/*  toplevel dialogs  */
/**********************/

GtkWidget *
dialogs_image_new_new (GimpDialogFactory *factory,
                       GimpContext       *context,
                       GimpUIManager     *ui_manager,
                       gint               view_size)
{
  return image_new_dialog_new (context);
}

GtkWidget *
dialogs_file_open_new (GimpDialogFactory *factory,
                       GimpContext       *context,
                       GimpUIManager     *ui_manager,
                       gint               view_size)
{
  return file_open_dialog_new (context->gimp);
}

GtkWidget *
dialogs_file_open_location_new (GimpDialogFactory *factory,
                                GimpContext       *context,
                                GimpUIManager     *ui_manager,
                                gint               view_size)
{
  return file_open_location_dialog_new (context->gimp);
}

GtkWidget *
dialogs_file_save_new (GimpDialogFactory *factory,
                       GimpContext       *context,
                       GimpUIManager     *ui_manager,
                       gint               view_size)
{
  return file_save_dialog_new (context->gimp, FALSE);
}

GtkWidget *
dialogs_file_export_new (GimpDialogFactory *factory,
                         GimpContext       *context,
                         GimpUIManager     *ui_manager,
                         gint               view_size)
{
  return file_save_dialog_new (context->gimp, TRUE);
}

GtkWidget *
dialogs_preferences_get (GimpDialogFactory *factory,
                         GimpContext       *context,
                         GimpUIManager     *ui_manager,
                         gint               view_size)
{
  return preferences_dialog_create (context->gimp);
}

GtkWidget *
dialogs_keyboard_shortcuts_get (GimpDialogFactory *factory,
                                GimpContext       *context,
                                GimpUIManager     *ui_manager,
                                gint               view_size)
{
  return keyboard_shortcuts_dialog_new (context->gimp);
}

GtkWidget *
dialogs_input_devices_get (GimpDialogFactory *factory,
                           GimpContext       *context,
                           GimpUIManager     *ui_manager,
                           gint               view_size)
{
  return input_devices_dialog_new (context->gimp);
}

GtkWidget *
dialogs_module_get (GimpDialogFactory *factory,
                    GimpContext       *context,
                    GimpUIManager     *ui_manager,
                    gint               view_size)
{
  return module_dialog_new (context->gimp);
}

GtkWidget *
dialogs_palette_import_get (GimpDialogFactory *factory,
                            GimpContext       *context,
                            GimpUIManager     *ui_manager,
                            gint               view_size)
{
  return palette_import_dialog_new (context);
}

GtkWidget *
dialogs_tips_get (GimpDialogFactory *factory,
                  GimpContext       *context,
                  GimpUIManager     *ui_manager,
                  gint               view_size)
{
  return tips_dialog_create (context->gimp);
}

GtkWidget *
dialogs_about_get (GimpDialogFactory *factory,
                   GimpContext       *context,
                   GimpUIManager     *ui_manager,
                   gint               view_size)
{
  return about_dialog_create ();
}

GtkWidget *
dialogs_action_search_get (GimpDialogFactory *factory,
                           GimpContext       *context,
                           GimpUIManager     *ui_manager,
                           gint               view_size)
{
  return action_search_dialog_create (context->gimp);
}

GtkWidget *
dialogs_error_get (GimpDialogFactory *factory,
                   GimpContext       *context,
                   GimpUIManager     *ui_manager,
                   gint               view_size)
{
  return gimp_error_dialog_new (_("GIMP Message"));
}

GtkWidget *
dialogs_critical_get (GimpDialogFactory *factory,
                      GimpContext       *context,
                      GimpUIManager     *ui_manager,
                      gint               view_size)
{
  return gimp_critical_dialog_new (_("GIMP Debug"));
}

GtkWidget *
dialogs_close_all_get (GimpDialogFactory *factory,
                       GimpContext       *context,
                       GimpUIManager     *ui_manager,
                       gint               view_size)
{
  return close_all_dialog_new (context->gimp);
}

GtkWidget *
dialogs_quit_get (GimpDialogFactory *factory,
                  GimpContext       *context,
                  GimpUIManager     *ui_manager,
                  gint               view_size)
{
  return quit_dialog_new (context->gimp);
}


/***********/
/*  docks  */
/***********/

GtkWidget *
dialogs_toolbox_new (GimpDialogFactory *factory,
                     GimpContext       *context,
                     GimpUIManager     *ui_manager,
                     gint               view_size)
{
  return gimp_toolbox_new (factory,
                           context,
                           ui_manager);
}

GtkWidget *
dialogs_toolbox_dock_window_new (GimpDialogFactory *factory,
                                 GimpContext       *context,
                                 GimpUIManager     *ui_manager,
                                 gint               view_size)
{
  static gint  role_serial = 1;
  GtkWidget   *dock;
  gchar       *role;

  role = g_strdup_printf ("gimp-toolbox-%d", role_serial++);
  dock = gimp_dock_window_new (role,
                               "<Toolbox>",
                               TRUE,
                               factory,
                               context);
  g_free (role);

  return dock;
}

GtkWidget *
dialogs_dock_new (GimpDialogFactory *factory,
                  GimpContext       *context,
                  GimpUIManager     *ui_manager,
                  gint               view_size)
{
  return gimp_menu_dock_new ();
}

GtkWidget *
dialogs_dock_window_new (GimpDialogFactory *factory,
                         GimpContext       *context,
                         GimpUIManager     *ui_manager,
                         gint               view_size)
{
  static gint  role_serial = 1;
  GtkWidget   *dock;
  gchar       *role;

  role = g_strdup_printf ("gimp-dock-%d", role_serial++);
  dock = gimp_dock_window_new (role,
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
dialogs_tool_options_new (GimpDialogFactory *factory,
                          GimpContext       *context,
                          GimpUIManager     *ui_manager,
                          gint               view_size)
{
  return gimp_tool_options_editor_new (context->gimp,
                                       gimp_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_device_status_new (GimpDialogFactory *factory,
                           GimpContext       *context,
                           GimpUIManager     *ui_manager,
                           gint               view_size)
{
  return gimp_device_status_new (context->gimp);
}

GtkWidget *
dialogs_error_console_new (GimpDialogFactory *factory,
                           GimpContext       *context,
                           GimpUIManager     *ui_manager,
                           gint               view_size)
{
  return gimp_error_console_new (context->gimp,
                                 gimp_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_cursor_view_new (GimpDialogFactory *factory,
                         GimpContext       *context,
                         GimpUIManager     *ui_manager,
                         gint               view_size)
{
  return gimp_cursor_view_new (gimp_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_dashboard_new (GimpDialogFactory *factory,
                       GimpContext       *context,
                       GimpUIManager     *ui_manager,
                       gint               view_size)
{
  return gimp_dashboard_new (context->gimp,
                             gimp_dialog_factory_get_menu_factory (factory));
}


/*****  list views  *****/

GtkWidget *
dialogs_image_list_view_new (GimpDialogFactory *factory,
                             GimpContext       *context,
                             GimpUIManager     *ui_manager,
                             gint               view_size)
{
  return gimp_image_view_new (GIMP_VIEW_TYPE_LIST,
                              context->gimp->images,
                              context,
                              view_size, 1,
                              gimp_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_brush_list_view_new (GimpDialogFactory *factory,
                             GimpContext       *context,
                             GimpUIManager     *ui_manager,
                             gint               view_size)
{
  return gimp_brush_factory_view_new (GIMP_VIEW_TYPE_LIST,
                                      context->gimp->brush_factory,
                                      context,
                                      TRUE,
                                      view_size, 1,
                                      gimp_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_dynamics_list_view_new (GimpDialogFactory *factory,
                                GimpContext       *context,
                                GimpUIManager     *ui_manager,
                                gint               view_size)
{
  return gimp_dynamics_factory_view_new (GIMP_VIEW_TYPE_LIST,
                                         context->gimp->dynamics_factory,
                                         context,
                                         view_size, 1,
                                         gimp_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_mypaint_brush_list_view_new (GimpDialogFactory *factory,
                                     GimpContext       *context,
                                     GimpUIManager     *ui_manager,
                                     gint               view_size)
{
  return gimp_data_factory_view_new (GIMP_VIEW_TYPE_LIST,
                                     context->gimp->mybrush_factory,
                                     context,
                                     view_size, 1,
                                     gimp_dialog_factory_get_menu_factory (factory),
                                     "<MyPaintBrushes>",
                                     "/mypaint-brushes-popup",
                                     "mypaint-brushes");
}

GtkWidget *
dialogs_pattern_list_view_new (GimpDialogFactory *factory,
                               GimpContext       *context,
                               GimpUIManager     *ui_manager,
                               gint               view_size)
{
  return gimp_pattern_factory_view_new (GIMP_VIEW_TYPE_LIST,
                                        context->gimp->pattern_factory,
                                        context,
                                        view_size, 1,
                                        gimp_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_gradient_list_view_new (GimpDialogFactory *factory,
                                GimpContext       *context,
                                GimpUIManager     *ui_manager,
                                gint               view_size)
{
  return gimp_data_factory_view_new (GIMP_VIEW_TYPE_LIST,
                                     context->gimp->gradient_factory,
                                     context,
                                     view_size, 1,
                                     gimp_dialog_factory_get_menu_factory (factory),
                                     "<Gradients>",
                                     "/gradients-popup",
                                     "gradients");
}

GtkWidget *
dialogs_palette_list_view_new (GimpDialogFactory *factory,
                               GimpContext       *context,
                               GimpUIManager     *ui_manager,
                               gint               view_size)
{
  return gimp_data_factory_view_new (GIMP_VIEW_TYPE_LIST,
                                     context->gimp->palette_factory,
                                     context,
                                     view_size, 1,
                                     gimp_dialog_factory_get_menu_factory (factory),
                                     "<Palettes>",
                                     "/palettes-popup",
                                     "palettes");
}

GtkWidget *
dialogs_font_list_view_new (GimpDialogFactory *factory,
                            GimpContext       *context,
                            GimpUIManager     *ui_manager,
                            gint               view_size)
{
  return gimp_font_view_new (GIMP_VIEW_TYPE_LIST,
                             context->gimp->fonts,
                             context,
                             view_size, 1,
                             gimp_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_buffer_list_view_new (GimpDialogFactory *factory,
                              GimpContext       *context,
                              GimpUIManager     *ui_manager,
                              gint               view_size)
{
  return gimp_buffer_view_new (GIMP_VIEW_TYPE_LIST,
                               context->gimp->named_buffers,
                               context,
                               view_size, 1,
                               gimp_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_tool_preset_list_view_new (GimpDialogFactory *factory,
                                   GimpContext       *context,
                                   GimpUIManager     *ui_manager,
                                   gint               view_size)
{
  return gimp_tool_preset_factory_view_new (GIMP_VIEW_TYPE_LIST,
                                            context->gimp->tool_preset_factory,
                                            context,
                                            view_size, 1,
                                            gimp_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_document_list_view_new (GimpDialogFactory *factory,
                                GimpContext       *context,
                                GimpUIManager     *ui_manager,
                                gint               view_size)
{
  return gimp_document_view_new (GIMP_VIEW_TYPE_LIST,
                                 context->gimp->documents,
                                 context,
                                 view_size, 0,
                                 gimp_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_template_list_view_new (GimpDialogFactory *factory,
                                GimpContext       *context,
                                GimpUIManager     *ui_manager,
                                gint               view_size)
{
  return gimp_template_view_new (GIMP_VIEW_TYPE_LIST,
                                 context->gimp->templates,
                                 context,
                                 view_size, 0,
                                 gimp_dialog_factory_get_menu_factory (factory));
}


/*****  grid views  *****/

GtkWidget *
dialogs_image_grid_view_new (GimpDialogFactory *factory,
                             GimpContext       *context,
                             GimpUIManager     *ui_manager,
                             gint               view_size)
{
  return gimp_image_view_new (GIMP_VIEW_TYPE_GRID,
                              context->gimp->images,
                              context,
                              view_size, 1,
                              gimp_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_brush_grid_view_new (GimpDialogFactory *factory,
                             GimpContext       *context,
                             GimpUIManager     *ui_manager,
                             gint               view_size)
{
  return gimp_brush_factory_view_new (GIMP_VIEW_TYPE_GRID,
                                      context->gimp->brush_factory,
                                      context,
                                      TRUE,
                                      view_size, 1,
                                      gimp_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_dynamics_grid_view_new (GimpDialogFactory *factory,
                                GimpContext       *context,
                                GimpUIManager     *ui_manager,
                                gint               view_size)
{
  return gimp_dynamics_factory_view_new (GIMP_VIEW_TYPE_GRID,
                                         context->gimp->dynamics_factory,
                                         context,
                                         view_size, 1,
                                         gimp_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_mypaint_brush_grid_view_new (GimpDialogFactory *factory,
                                     GimpContext       *context,
                                     GimpUIManager     *ui_manager,
                                     gint               view_size)
{
  return gimp_data_factory_view_new (GIMP_VIEW_TYPE_GRID,
                                     context->gimp->mybrush_factory,
                                     context,
                                     view_size, 1,
                                     gimp_dialog_factory_get_menu_factory (factory),
                                     "<MyPaintBrushes>",
                                     "/mypaint-brushes-popup",
                                     "mypaint-brushes");
}

GtkWidget *
dialogs_pattern_grid_view_new (GimpDialogFactory *factory,
                               GimpContext       *context,
                               GimpUIManager     *ui_manager,
                               gint               view_size)
{
  return gimp_pattern_factory_view_new (GIMP_VIEW_TYPE_GRID,
                                        context->gimp->pattern_factory,
                                        context,
                                        view_size, 1,
                                        gimp_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_gradient_grid_view_new (GimpDialogFactory *factory,
                                GimpContext       *context,
                                GimpUIManager     *ui_manager,
                                gint               view_size)
{
  return gimp_data_factory_view_new (GIMP_VIEW_TYPE_GRID,
                                     context->gimp->gradient_factory,
                                     context,
                                     view_size, 1,
                                     gimp_dialog_factory_get_menu_factory (factory),
                                     "<Gradients>",
                                     "/gradients-popup",
                                     "gradients");
}

GtkWidget *
dialogs_palette_grid_view_new (GimpDialogFactory *factory,
                               GimpContext       *context,
                               GimpUIManager     *ui_manager,
                               gint               view_size)
{
  return gimp_data_factory_view_new (GIMP_VIEW_TYPE_GRID,
                                     context->gimp->palette_factory,
                                     context,
                                     view_size, 1,
                                     gimp_dialog_factory_get_menu_factory (factory),
                                     "<Palettes>",
                                     "/palettes-popup",
                                     "palettes");
}

GtkWidget *
dialogs_font_grid_view_new (GimpDialogFactory *factory,
                            GimpContext       *context,
                            GimpUIManager     *ui_manager,
                            gint               view_size)
{
  return gimp_font_view_new (GIMP_VIEW_TYPE_GRID,
                             context->gimp->fonts,
                             context,
                             view_size, 1,
                             gimp_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_buffer_grid_view_new (GimpDialogFactory *factory,
                              GimpContext       *context,
                              GimpUIManager     *ui_manager,
                              gint               view_size)
{
  return gimp_buffer_view_new (GIMP_VIEW_TYPE_GRID,
                               context->gimp->named_buffers,
                               context,
                               view_size, 1,
                               gimp_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_tool_preset_grid_view_new (GimpDialogFactory *factory,
                                   GimpContext       *context,
                                   GimpUIManager     *ui_manager,
                                   gint               view_size)
{
  return gimp_tool_preset_factory_view_new (GIMP_VIEW_TYPE_GRID,
                                            context->gimp->tool_preset_factory,
                                            context,
                                            view_size, 1,
                                            gimp_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_document_grid_view_new (GimpDialogFactory *factory,
                                GimpContext       *context,
                                GimpUIManager     *ui_manager,
                                gint               view_size)
{
  return gimp_document_view_new (GIMP_VIEW_TYPE_GRID,
                                 context->gimp->documents,
                                 context,
                                 view_size, 0,
                                 gimp_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_template_grid_view_new (GimpDialogFactory *factory,
                                GimpContext       *context,
                                GimpUIManager     *ui_manager,
                                gint               view_size)
{
  return gimp_template_view_new (GIMP_VIEW_TYPE_GRID,
                                 context->gimp->templates,
                                 context,
                                 view_size, 0,
                                 gimp_dialog_factory_get_menu_factory (factory));
}


/*****  image related dialogs  *****/

GtkWidget *
dialogs_layer_list_view_new (GimpDialogFactory *factory,
                             GimpContext       *context,
                             GimpUIManager     *ui_manager,
                             gint               view_size)
{
  if (view_size < 1)
    view_size = context->gimp->config->layer_preview_size;

  return gimp_item_tree_view_new (GIMP_TYPE_LAYER_TREE_VIEW,
                                  view_size, 2,
                                  gimp_context_get_image (context),
                                  gimp_dialog_factory_get_menu_factory (factory),
                                  "<Layers>",
                                  "/layers-popup");
}

GtkWidget *
dialogs_channel_list_view_new (GimpDialogFactory *factory,
                               GimpContext       *context,
                               GimpUIManager     *ui_manager,
                               gint               view_size)
{
  if (view_size < 1)
    view_size = context->gimp->config->layer_preview_size;

  return gimp_item_tree_view_new (GIMP_TYPE_CHANNEL_TREE_VIEW,
                                  view_size, 1,
                                  gimp_context_get_image (context),
                                  gimp_dialog_factory_get_menu_factory (factory),
                                  "<Channels>",
                                  "/channels-popup");
}

GtkWidget *
dialogs_vectors_list_view_new (GimpDialogFactory *factory,
                               GimpContext       *context,
                               GimpUIManager     *ui_manager,
                               gint               view_size)
{
  if (view_size < 1)
    view_size = context->gimp->config->layer_preview_size;

  return gimp_item_tree_view_new (GIMP_TYPE_VECTORS_TREE_VIEW,
                                  view_size, 1,
                                  gimp_context_get_image (context),
                                  gimp_dialog_factory_get_menu_factory (factory),
                                  "<Vectors>",
                                  "/vectors-popup");
}

GtkWidget *
dialogs_colormap_editor_new (GimpDialogFactory *factory,
                             GimpContext       *context,
                             GimpUIManager     *ui_manager,
                             gint               view_size)
{
  return gimp_colormap_editor_new (gimp_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_histogram_editor_new (GimpDialogFactory *factory,
                              GimpContext       *context,
                              GimpUIManager     *ui_manager,
                              gint               view_size)
{
  return gimp_histogram_editor_new ();
}

GtkWidget *
dialogs_selection_editor_new (GimpDialogFactory *factory,
                              GimpContext       *context,
                              GimpUIManager     *ui_manager,
                              gint               view_size)
{
  return gimp_selection_editor_new (gimp_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_symmetry_editor_new (GimpDialogFactory *factory,
                             GimpContext       *context,
                             GimpUIManager     *ui_manager,
                             gint               view_size)
{
  return gimp_symmetry_editor_new (gimp_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_undo_editor_new (GimpDialogFactory *factory,
                         GimpContext       *context,
                         GimpUIManager     *ui_manager,
                         gint               view_size)
{
  return gimp_undo_editor_new (context->gimp->config,
                               gimp_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_sample_point_editor_new (GimpDialogFactory *factory,
                                 GimpContext       *context,
                                 GimpUIManager     *ui_manager,
                                 gint               view_size)
{
  return gimp_sample_point_editor_new (gimp_dialog_factory_get_menu_factory (factory));
}


/*****  display related dialogs  *****/

GtkWidget *
dialogs_navigation_editor_new (GimpDialogFactory *factory,
                               GimpContext       *context,
                               GimpUIManager     *ui_manager,
                               gint               view_size)
{
  return gimp_navigation_editor_new (gimp_dialog_factory_get_menu_factory (factory));
}


/*****  misc dockables  *****/

GtkWidget *
dialogs_color_editor_new (GimpDialogFactory *factory,
                          GimpContext       *context,
                          GimpUIManager     *ui_manager,
                          gint               view_size)
{
  return gimp_color_editor_new (context);
}


/*************/
/*  editors  */
/*************/

GtkWidget *
dialogs_brush_editor_get (GimpDialogFactory *factory,
                          GimpContext       *context,
                          GimpUIManager     *ui_manager,
                          gint               view_size)
{
  return gimp_brush_editor_new (context,
                                gimp_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_dynamics_editor_get (GimpDialogFactory *factory,
                             GimpContext       *context,
                             GimpUIManager     *ui_manager,
                             gint               view_size)
{
  return gimp_dynamics_editor_new (context,
                                   gimp_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_gradient_editor_get (GimpDialogFactory *factory,
                             GimpContext       *context,
                             GimpUIManager     *ui_manager,
                             gint               view_size)
{
  return gimp_gradient_editor_new (context,
                                   gimp_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_palette_editor_get (GimpDialogFactory *factory,
                            GimpContext       *context,
                            GimpUIManager     *ui_manager,
                            gint               view_size)
{
  return gimp_palette_editor_new (context,
                                  gimp_dialog_factory_get_menu_factory (factory));
}

GtkWidget *
dialogs_tool_preset_editor_get (GimpDialogFactory *factory,
                                GimpContext       *context,
                                GimpUIManager     *ui_manager,
                                gint               view_size)
{
  return gimp_tool_preset_editor_new (context,
                                      gimp_dialog_factory_get_menu_factory (factory));
}
