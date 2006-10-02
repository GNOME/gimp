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
#include "core/gimpimage.h"
#include "core/gimpimage-colormap.h"

#include "config/gimpguiconfig.h"

#include "widgets/gimpbrusheditor.h"
#include "widgets/gimpbrushfactoryview.h"
#include "widgets/gimpbufferview.h"
#include "widgets/gimpchanneltreeview.h"
#include "widgets/gimpcoloreditor.h"
#include "widgets/gimpcolormapeditor.h"
#include "widgets/gimpcontainergridview.h"
#include "widgets/gimpcontainertreeview.h"
#include "widgets/gimpcontainerview-utils.h"
#include "widgets/gimpcursorview.h"
#include "widgets/gimpdataeditor.h"
#include "widgets/gimpdevicestatus.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdockable.h"
#include "widgets/gimpdocumentview.h"
#include "widgets/gimperrorconsole.h"
#include "widgets/gimperrordialog.h"
#include "widgets/gimpfontview.h"
#include "widgets/gimpgradienteditor.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimphistogrameditor.h"
#include "widgets/gimpimageview.h"
#include "widgets/gimplayertreeview.h"
#include "widgets/gimpmenudock.h"
#include "widgets/gimppaletteeditor.h"
#include "widgets/gimppatternfactoryview.h"
#include "widgets/gimpsamplepointeditor.h"
#include "widgets/gimpselectioneditor.h"
#include "widgets/gimptemplateview.h"
#include "widgets/gimptoolbox.h"
#include "widgets/gimptooloptionseditor.h"
#include "widgets/gimptoolview.h"
#include "widgets/gimpundoeditor.h"
#include "widgets/gimpvectorstreeview.h"

#include "display/gimpnavigationeditor.h"

#include "about-dialog.h"
#include "dialogs.h"
#include "dialogs-constructors.h"
#include "file-open-dialog.h"
#include "file-open-location-dialog.h"
#include "file-save-dialog.h"
#include "image-new-dialog.h"
#include "keyboard-shortcuts-dialog.h"
#include "module-dialog.h"
#include "palette-import-dialog.h"
#include "preferences-dialog.h"
#include "quit-dialog.h"
#include "tips-dialog.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   dialogs_indexed_palette_selected (GimpColormapEditor *editor,
                                                GdkModifierType     state,
                                                gpointer            data);


/**********************/
/*  toplevel dialogs  */
/**********************/

GtkWidget *
dialogs_image_new_new (GimpDialogFactory *factory,
                       GimpContext       *context,
                       gint               view_size)
{
  return image_new_dialog_new (context);
}

GtkWidget *
dialogs_file_open_new (GimpDialogFactory *factory,
                       GimpContext       *context,
                       gint               view_size)
{
  return file_open_dialog_new (context->gimp);
}

GtkWidget *
dialogs_file_open_location_new (GimpDialogFactory *factory,
                                GimpContext       *context,
                                gint               view_size)
{
  return file_open_location_dialog_new (context->gimp);
}

GtkWidget *
dialogs_file_save_new (GimpDialogFactory *factory,
                       GimpContext       *context,
                       gint               view_size)
{
  return file_save_dialog_new (context->gimp);
}

GtkWidget *
dialogs_preferences_get (GimpDialogFactory *factory,
                         GimpContext       *context,
                         gint               view_size)
{
  return preferences_dialog_create (context->gimp);
}

GtkWidget *
dialogs_keyboard_shortcuts_get (GimpDialogFactory *factory,
                                GimpContext       *context,
                                gint               view_size)
{
  return keyboard_shortcuts_dialog_new (context->gimp);
}

GtkWidget *
dialogs_module_get (GimpDialogFactory *factory,
                    GimpContext       *context,
                    gint               view_size)
{
  return module_dialog_new (context->gimp);
}

GtkWidget *
dialogs_palette_import_get (GimpDialogFactory *factory,
                            GimpContext       *context,
                            gint               view_size)
{
  return palette_import_dialog_new (context);
}

GtkWidget *
dialogs_tips_get (GimpDialogFactory *factory,
                  GimpContext       *context,
                  gint               view_size)
{
  return tips_dialog_create (context->gimp);
}

GtkWidget *
dialogs_about_get (GimpDialogFactory *factory,
                   GimpContext       *context,
                   gint               view_size)
{
  return about_dialog_create (context);
}

GtkWidget *
dialogs_error_get (GimpDialogFactory *factory,
                   GimpContext       *context,
                   gint               view_size)
{
  return gimp_error_dialog_new (_("GIMP Message"));
}

GtkWidget *
dialogs_close_all_get (GimpDialogFactory *factory,
                       GimpContext       *context,
                       gint               view_size)
{
  return close_all_dialog_new (context->gimp);
}

GtkWidget *
dialogs_quit_get (GimpDialogFactory *factory,
                  GimpContext       *context,
                  gint               view_size)
{
  return quit_dialog_new (context->gimp);
}


/***********/
/*  docks  */
/***********/

GtkWidget *
dialogs_toolbox_get (GimpDialogFactory *factory,
                     GimpContext       *context,
                     gint               view_size)
{
  /*  we pass "global_dock_factory", _not_ "global_toolbox_factory" to
   *  the toolbox constructor, because the global_toolbox_factory has no
   *  dockables registered
   */
  return gimp_toolbox_new (global_dock_factory, context);
}

GtkWidget *
dialogs_dock_new (GimpDialogFactory *factory,
                  GimpContext       *context,
                  gint               view_size)
{
  return gimp_menu_dock_new (factory,
                             context->gimp->images,
                             context->gimp->displays);
}


/***************/
/*  dockables  */
/***************/

/*****  the dockable constructor  *****/

GtkWidget *
dialogs_dockable_constructor (GimpDialogFactory      *factory,
                              GimpDialogFactoryEntry *entry,
                              GimpContext            *context,
                              gint                    view_size)
{
  GtkWidget *dockable = NULL;
  GtkWidget *widget;

  widget = entry->new_func (factory, context, view_size);

  if (widget)
    {
      dockable = gimp_dockable_new (entry->name, entry->blurb,
                                    entry->stock_id, entry->help_id);
      gtk_container_add (GTK_CONTAINER (dockable), widget);
      gtk_widget_show (widget);

      /* EEK */
      g_object_set_data (G_OBJECT (dockable), "gimp-dialog-identifier",
                         entry->identifier);
    }

  return dockable;
}


/*****  singleton dialogs  *****/

GtkWidget *
dialogs_tool_options_new (GimpDialogFactory *factory,
                          GimpContext       *context,
                          gint               view_size)
{
  return gimp_tool_options_editor_new (context->gimp,
                                       factory->menu_factory);
}

GtkWidget *
dialogs_device_status_new (GimpDialogFactory *factory,
                           GimpContext       *context,
                           gint               view_size)
{
  return gimp_device_status_new (context->gimp);
}

GtkWidget *
dialogs_error_console_new (GimpDialogFactory *factory,
                           GimpContext       *context,
                           gint               view_size)
{
  return gimp_error_console_new (context->gimp,
                                 factory->menu_factory);
}

GtkWidget *
dialogs_cursor_view_new (GimpDialogFactory *factory,
                         GimpContext       *context,
                         gint               view_size)
{
  return gimp_cursor_view_new (factory->menu_factory);
}


/*****  list views  *****/

GtkWidget *
dialogs_image_list_view_new (GimpDialogFactory *factory,
                             GimpContext       *context,
                             gint               view_size)
{
  return gimp_image_view_new (GIMP_VIEW_TYPE_LIST,
                              context->gimp->images,
                              context,
                              view_size, 1,
                              factory->menu_factory);
}

GtkWidget *
dialogs_brush_list_view_new (GimpDialogFactory *factory,
                             GimpContext       *context,
                             gint               view_size)
{
  return gimp_brush_factory_view_new (GIMP_VIEW_TYPE_LIST,
                                      context->gimp->brush_factory,
                                      context,
                                      TRUE,
                                      view_size, 1,
                                      factory->menu_factory);
}

GtkWidget *
dialogs_pattern_list_view_new (GimpDialogFactory *factory,
                               GimpContext       *context,
                               gint               view_size)
{
  return gimp_pattern_factory_view_new (GIMP_VIEW_TYPE_LIST,
                                        context->gimp->pattern_factory,
                                        context,
                                        view_size, 1,
                                        factory->menu_factory);
}

GtkWidget *
dialogs_gradient_list_view_new (GimpDialogFactory *factory,
                                GimpContext       *context,
                                gint               view_size)
{
  return gimp_data_factory_view_new (GIMP_VIEW_TYPE_LIST,
                                     context->gimp->gradient_factory,
                                     context,
                                     view_size, 1,
                                     factory->menu_factory, "<Gradients>",
                                     "/gradients-popup",
                                     "gradients");
}

GtkWidget *
dialogs_palette_list_view_new (GimpDialogFactory *factory,
                               GimpContext       *context,
                               gint               view_size)
{
  return gimp_data_factory_view_new (GIMP_VIEW_TYPE_LIST,
                                     context->gimp->palette_factory,
                                     context,
                                     view_size, 1,
                                     factory->menu_factory, "<Palettes>",
                                     "/palettes-popup",
                                     "palettes");
}

GtkWidget *
dialogs_font_list_view_new (GimpDialogFactory *factory,
                            GimpContext       *context,
                            gint               view_size)
{
  return gimp_font_view_new (GIMP_VIEW_TYPE_LIST,
                             context->gimp->fonts,
                             context,
                             view_size, 1,
                             factory->menu_factory);
}

GtkWidget *
dialogs_tool_list_view_new (GimpDialogFactory *factory,
                            GimpContext       *context,
                            gint               view_size)
{
  return gimp_tool_view_new (GIMP_VIEW_TYPE_LIST,
                             context->gimp->tool_info_list,
                             context,
                             view_size, 0,
                             factory->menu_factory);
}

GtkWidget *
dialogs_buffer_list_view_new (GimpDialogFactory *factory,
                              GimpContext       *context,
                              gint               view_size)
{
  return gimp_buffer_view_new (GIMP_VIEW_TYPE_LIST,
                               context->gimp->named_buffers,
                               context,
                               view_size, 1,
                               factory->menu_factory);
}

GtkWidget *
dialogs_document_list_view_new (GimpDialogFactory *factory,
                                GimpContext       *context,
                                gint               view_size)
{
  return gimp_document_view_new (GIMP_VIEW_TYPE_LIST,
                                 context->gimp->documents,
                                 context,
                                 view_size, 0,
                                 factory->menu_factory);
}

GtkWidget *
dialogs_template_list_view_new (GimpDialogFactory *factory,
                                GimpContext       *context,
                                gint               view_size)
{
  return gimp_template_view_new (GIMP_VIEW_TYPE_LIST,
                                 context->gimp->templates,
                                 context,
                                 view_size, 0,
                                 factory->menu_factory);
}


/*****  grid views  *****/

GtkWidget *
dialogs_image_grid_view_new (GimpDialogFactory *factory,
                             GimpContext       *context,
                             gint               view_size)
{
  return gimp_image_view_new (GIMP_VIEW_TYPE_GRID,
                              context->gimp->images,
                              context,
                              view_size, 1,
                              factory->menu_factory);
}

GtkWidget *
dialogs_brush_grid_view_new (GimpDialogFactory *factory,
                             GimpContext       *context,
                             gint               view_size)
{
  return gimp_brush_factory_view_new (GIMP_VIEW_TYPE_GRID,
                                      context->gimp->brush_factory,
                                      context,
                                      TRUE,
                                      view_size, 1,
                                      factory->menu_factory);
}

GtkWidget *
dialogs_pattern_grid_view_new (GimpDialogFactory *factory,
                               GimpContext       *context,
                               gint               view_size)
{
  return gimp_pattern_factory_view_new (GIMP_VIEW_TYPE_GRID,
                                        context->gimp->pattern_factory,
                                        context,
                                        view_size, 1,
                                        factory->menu_factory);
}

GtkWidget *
dialogs_gradient_grid_view_new (GimpDialogFactory *factory,
                                GimpContext       *context,
                                gint               view_size)
{
  return gimp_data_factory_view_new (GIMP_VIEW_TYPE_GRID,
                                     context->gimp->gradient_factory,
                                     context,
                                     view_size, 1,
                                     factory->menu_factory, "<Gradients>",
                                     "/gradients-popup",
                                     "gradients");
}

GtkWidget *
dialogs_palette_grid_view_new (GimpDialogFactory *factory,
                               GimpContext       *context,
                               gint               view_size)
{
  return gimp_data_factory_view_new (GIMP_VIEW_TYPE_GRID,
                                     context->gimp->palette_factory,
                                     context,
                                     view_size, 1,
                                     factory->menu_factory, "<Palettes>",
                                     "/palettes-popup",
                                     "palettes");
}

GtkWidget *
dialogs_font_grid_view_new (GimpDialogFactory *factory,
                            GimpContext       *context,
                            gint               view_size)
{
  return gimp_font_view_new (GIMP_VIEW_TYPE_GRID,
                             context->gimp->fonts,
                             context,
                             view_size, 1,
                             factory->menu_factory);
}

GtkWidget *
dialogs_tool_grid_view_new (GimpDialogFactory *factory,
                            GimpContext       *context,
                            gint               view_size)
{
  return gimp_tool_view_new (GIMP_VIEW_TYPE_GRID,
                             context->gimp->tool_info_list,
                             context,
                             view_size, 1,
                             factory->menu_factory);
}

GtkWidget *
dialogs_buffer_grid_view_new (GimpDialogFactory *factory,
                              GimpContext       *context,
                              gint               view_size)
{
  return gimp_buffer_view_new (GIMP_VIEW_TYPE_GRID,
                               context->gimp->named_buffers,
                               context,
                               view_size, 1,
                               factory->menu_factory);
}

GtkWidget *
dialogs_document_grid_view_new (GimpDialogFactory *factory,
                                GimpContext       *context,
                                gint               view_size)
{
  return gimp_document_view_new (GIMP_VIEW_TYPE_GRID,
                                 context->gimp->documents,
                                 context,
                                 view_size, 0,
                                 factory->menu_factory);
}

GtkWidget *
dialogs_template_grid_view_new (GimpDialogFactory *factory,
                                GimpContext       *context,
                                gint               view_size)
{
  return gimp_template_view_new (GIMP_VIEW_TYPE_GRID,
                                 context->gimp->templates,
                                 context,
                                 view_size, 0,
                                 factory->menu_factory);
}


/*****  image related dialogs  *****/

GtkWidget *
dialogs_layer_list_view_new (GimpDialogFactory *factory,
                             GimpContext       *context,
                             gint               view_size)
{
  if (view_size < 1)
    view_size = context->gimp->config->layer_preview_size;

  return gimp_item_tree_view_new (GIMP_TYPE_LAYER_TREE_VIEW,
                                  view_size, 2,
                                  gimp_context_get_image (context),
                                  factory->menu_factory, "<Layers>",
                                  "/layers-popup");
}

GtkWidget *
dialogs_channel_list_view_new (GimpDialogFactory *factory,
                               GimpContext       *context,
                               gint               view_size)
{
  if (view_size < 1)
    view_size = context->gimp->config->layer_preview_size;

  return gimp_item_tree_view_new (GIMP_TYPE_CHANNEL_TREE_VIEW,
                                  view_size, 1,
                                  gimp_context_get_image (context),
                                  factory->menu_factory, "<Channels>",
                                  "/channels-popup");
}

GtkWidget *
dialogs_vectors_list_view_new (GimpDialogFactory *factory,
                               GimpContext       *context,
                               gint               view_size)
{
  if (view_size < 1)
    view_size = context->gimp->config->layer_preview_size;

  return gimp_item_tree_view_new (GIMP_TYPE_VECTORS_TREE_VIEW,
                                  view_size, 1,
                                  gimp_context_get_image (context),
                                  factory->menu_factory, "<Vectors>",
                                  "/vectors-popup");
}

GtkWidget *
dialogs_colormap_editor_new (GimpDialogFactory *factory,
                             GimpContext       *context,
                             gint               view_size)
{
  GtkWidget *view;

  view = gimp_colormap_editor_new (factory->menu_factory);

  g_signal_connect (view, "selected",
                    G_CALLBACK (dialogs_indexed_palette_selected),
                    NULL);

  return view;
}

GtkWidget *
dialogs_histogram_editor_new (GimpDialogFactory *factory,
                              GimpContext       *context,
                              gint               view_size)
{
  return gimp_histogram_editor_new ();
}

GtkWidget *
dialogs_selection_editor_new (GimpDialogFactory *factory,
                              GimpContext       *context,
                              gint               view_size)
{
  return gimp_selection_editor_new (factory->menu_factory);
}

GtkWidget *
dialogs_undo_editor_new (GimpDialogFactory *factory,
                         GimpContext       *context,
                         gint               view_size)
{
  return gimp_undo_editor_new (context->gimp->config,
                               factory->menu_factory);
}

GtkWidget *
dialogs_sample_point_editor_new (GimpDialogFactory *factory,
                                 GimpContext       *context,
                                 gint               view_size)
{
  return gimp_sample_point_editor_new (factory->menu_factory);
}


/*****  display related dialogs  *****/

GtkWidget *
dialogs_navigation_editor_new (GimpDialogFactory *factory,
                               GimpContext       *context,
                               gint               view_size)
{
  return gimp_navigation_editor_new (factory->menu_factory);
}


/*****  misc dockables  *****/

GtkWidget *
dialogs_color_editor_new (GimpDialogFactory *factory,
                          GimpContext       *context,
                          gint               view_size)
{
  return gimp_color_editor_new (context);
}


/*********************/
/*****  editors  *****/
/*********************/

GtkWidget *
dialogs_brush_editor_get (GimpDialogFactory *factory,
                          GimpContext       *context,
                          gint               view_size)
{
  return gimp_brush_editor_new (context,
                                factory->menu_factory);
}

GtkWidget *
dialogs_gradient_editor_get (GimpDialogFactory *factory,
                             GimpContext       *context,
                             gint               view_size)
{
  return gimp_gradient_editor_new (context,
                                   factory->menu_factory);
}

GtkWidget *
dialogs_palette_editor_get (GimpDialogFactory *factory,
                            GimpContext       *context,
                            gint               view_size)
{
  return gimp_palette_editor_new (context,
                                  factory->menu_factory);
}


/*  private functions  */

static void
dialogs_indexed_palette_selected (GimpColormapEditor *editor,
                                  GdkModifierType     state,
                                  gpointer            data)
{
  GimpImageEditor *image_editor = GIMP_IMAGE_EDITOR (editor);

  if (image_editor->image)
    {
      GimpRGB color;
      gint    index;

      index = gimp_colormap_editor_get_index (editor, NULL);

      gimp_image_get_colormap_entry (image_editor->image, index, &color);

      if (state & GDK_CONTROL_MASK)
        gimp_context_set_background (image_editor->context, &color);
      else
        gimp_context_set_foreground (image_editor->context, &color);
    }
}
