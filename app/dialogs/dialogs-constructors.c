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
#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-colormap.h"
#include "core/gimplayer.h"

#include "vectors/gimpvectors.h"

#include "config/gimpdisplayconfig.h"

#include "widgets/gimpbrusheditor.h"
#include "widgets/gimpbrushfactoryview.h"
#include "widgets/gimpbufferview.h"
#include "widgets/gimpcoloreditor.h"
#include "widgets/gimpcolormapeditor.h"
#include "widgets/gimpcontainergridview.h"
#include "widgets/gimpcontainertreeview.h"
#include "widgets/gimpcontainerview-utils.h"
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
#include "widgets/gimpimagedock.h"
#include "widgets/gimpimageview.h"
#include "widgets/gimpitemtreeview.h"
#include "widgets/gimppaletteeditor.h"
#include "widgets/gimppatternfactoryview.h"
#include "widgets/gimpselectioneditor.h"
#include "widgets/gimptemplateview.h"
#include "widgets/gimptoolbox.h"
#include "widgets/gimptooloptionseditor.h"
#include "widgets/gimptoolview.h"
#include "widgets/gimpundoeditor.h"
#include "widgets/gimpvectorstreeview.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpnavigationeditor.h"

#include "actions/channels-commands.h"
#include "actions/edit-commands.h"
#include "actions/file-commands.h"
#include "actions/layers-commands.h"
#include "actions/vectors-commands.h"

#include "about-dialog.h"
#include "dialogs.h"
#include "dialogs-constructors.h"
#include "file-open-dialog.h"
#include "file-open-location-dialog.h"
#include "file-save-dialog.h"
#include "image-new-dialog.h"
#include "module-dialog.h"
#include "preferences-dialog.h"
#include "tips-dialog.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static GtkWidget * dialogs_dockable_new        (GtkWidget          *widget,
                                                const gchar        *name,
                                                const gchar        *blurb,
                                                const gchar        *stock_id,
                                                const gchar        *help_id);

static void   dialogs_indexed_palette_selected (GimpColormapEditor *editor,
                                                GdkModifierType     state,
                                                GimpDockable       *dockable);


/**********************/
/*  toplevel dialogs  */
/**********************/

GtkWidget *
dialogs_image_new_new (GimpDialogFactory *factory,
                       GimpContext       *context,
                       gint               preview_size)
{
  return image_new_dialog_new (context->gimp);
}

GtkWidget *
dialogs_file_open_new (GimpDialogFactory *factory,
                       GimpContext       *context,
                       gint               preview_size)
{
  return file_open_dialog_new (context->gimp);
}

GtkWidget *
dialogs_file_open_location_new (GimpDialogFactory *factory,
                                GimpContext       *context,
                                gint               preview_size)
{
  return file_open_location_dialog_new (context->gimp);
}

GtkWidget *
dialogs_file_save_new (GimpDialogFactory *factory,
                       GimpContext       *context,
                       gint               preview_size)
{
  return file_save_dialog_new (context->gimp);
}

GtkWidget *
dialogs_preferences_get (GimpDialogFactory *factory,
			 GimpContext       *context,
                         gint               preview_size)
{
  return preferences_dialog_create (context->gimp);
}

GtkWidget *
dialogs_module_get (GimpDialogFactory *factory,
                    GimpContext       *context,
                    gint               preview_size)
{
  return module_dialog_new (context->gimp);
}

GtkWidget *
dialogs_tips_get (GimpDialogFactory *factory,
		  GimpContext       *context,
                  gint               preview_size)
{
  return tips_dialog_create (context->gimp);
}

GtkWidget *
dialogs_about_get (GimpDialogFactory *factory,
		   GimpContext       *context,
                   gint               preview_size)
{
  return about_dialog_create ();
}

GtkWidget *
dialogs_error_get (GimpDialogFactory *factory,
                   GimpContext       *context,
                   gint               preview_size)
{
  return gimp_error_dialog_new (_("GIMP Message"), GIMP_STOCK_WARNING);
}

/***********/
/*  docks  */
/***********/

GtkWidget *
dialogs_toolbox_get (GimpDialogFactory *factory,
		     GimpContext       *context,
                     gint               preview_size)
{
  /*  we pass "global_dock_factory", _not_ "global_toolbox_factory" to
   *  the toolbox constructor, because the toolbox_factory has no
   *  dockables registered
   */
  return gimp_toolbox_new (global_dock_factory, context->gimp);
}

GtkWidget *
dialogs_dock_new (GimpDialogFactory *factory,
		  GimpContext       *context,
                  gint               preview_size)
{
  return gimp_image_dock_new (factory,
                              context->gimp->images,
                              context->gimp->displays);
}


/***************/
/*  dockables  */
/***************/

GtkWidget *
dialogs_tool_options_get (GimpDialogFactory *factory,
			  GimpContext       *context,
                          gint               preview_size)
{
  static GtkWidget *view = NULL;

  if (view)
    return NULL;

  view = gimp_tool_options_editor_new (context->gimp, factory->menu_factory);

  g_object_add_weak_pointer (G_OBJECT (view), (gpointer) &view);

  return dialogs_dockable_new (view,
                               _("Tool Options"), NULL,
                               GIMP_STOCK_TOOL_OPTIONS,
                               GIMP_HELP_TOOL_OPTIONS_DIALOG);
}

GtkWidget *
dialogs_device_status_get (GimpDialogFactory *factory,
                           GimpContext       *context,
                           gint               preview_size)
{
  static GtkWidget *view = NULL;

  if (view)
    return NULL;

  view = gimp_device_status_new (context->gimp);

  g_object_add_weak_pointer (G_OBJECT (view), (gpointer) &view);

  return dialogs_dockable_new (view,
                               _("Devices"), _("Device Status"),
                               GIMP_STOCK_DEVICE_STATUS,
                               GIMP_HELP_DEVICE_STATUS_DIALOG);
}

GtkWidget *
dialogs_error_console_get (GimpDialogFactory *factory,
			   GimpContext       *context,
                           gint               preview_size)
{
  static GtkWidget *view = NULL;

  if (view)
    return NULL;

  view = gimp_error_console_new (context->gimp, factory->menu_factory);

  g_object_add_weak_pointer (G_OBJECT (view), (gpointer) &view);

  return dialogs_dockable_new (view,
                               _("Errors"), _("Error Console"),
                               GIMP_STOCK_WARNING,
                               GIMP_HELP_ERRORS_DIALOG);
}


/*****  list views  *****/

GtkWidget *
dialogs_image_list_view_new (GimpDialogFactory *factory,
			     GimpContext       *context,
                             gint               preview_size)
{
  GtkWidget *view;

  view = gimp_image_view_new (GIMP_VIEW_TYPE_LIST,
                              context->gimp->images,
                              context,
                              preview_size, 1,
                              factory->menu_factory);

  return dialogs_dockable_new (view,
			       _("Images"), NULL,
                               GIMP_STOCK_IMAGES,
                               GIMP_HELP_IMAGE_DIALOG);
}

GtkWidget *
dialogs_brush_list_view_new (GimpDialogFactory *factory,
			     GimpContext       *context,
                             gint               preview_size)
{
  GtkWidget *view;

  view = gimp_brush_factory_view_new (GIMP_VIEW_TYPE_LIST,
				      context->gimp->brush_factory,
				      dialogs_edit_brush_func,
				      context,
				      TRUE,
				      preview_size, 1,
                                      factory->menu_factory);

  return dialogs_dockable_new (view,
			       _("Brushes"), NULL,
                               GIMP_STOCK_BRUSH,
                               GIMP_HELP_BRUSH_DIALOG);
}

GtkWidget *
dialogs_pattern_list_view_new (GimpDialogFactory *factory,
			       GimpContext       *context,
                               gint               preview_size)
{
  GtkWidget *view;

  view = gimp_pattern_factory_view_new (GIMP_VIEW_TYPE_LIST,
                                        context->gimp->pattern_factory,
                                        NULL,
                                        context,
                                        preview_size, 1,
                                        factory->menu_factory);

  return dialogs_dockable_new (view,
			       _("Patterns"), NULL,
                               GIMP_STOCK_PATTERN,
                               GIMP_HELP_PATTERN_DIALOG);
}

GtkWidget *
dialogs_gradient_list_view_new (GimpDialogFactory *factory,
				GimpContext       *context,
                                gint               preview_size)
{
  GtkWidget *view;

  view = gimp_data_factory_view_new (GIMP_VIEW_TYPE_LIST,
                                     context->gimp->gradient_factory,
                                     dialogs_edit_gradient_func,
                                     context,
                                     preview_size, 1,
                                     factory->menu_factory, "<Gradients>",
                                     "/gradients-popup",
                                     "gradients");

  return dialogs_dockable_new (view,
			       _("Gradients"), NULL,
                               GIMP_STOCK_GRADIENT,
                               GIMP_HELP_GRADIENT_DIALOG);
}

GtkWidget *
dialogs_palette_list_view_new (GimpDialogFactory *factory,
			       GimpContext       *context,
                               gint               preview_size)
{
  GtkWidget *view;

  view = gimp_data_factory_view_new (GIMP_VIEW_TYPE_LIST,
                                     context->gimp->palette_factory,
                                     dialogs_edit_palette_func,
                                     context,
                                     preview_size, 1,
                                     factory->menu_factory, "<Palettes>",
                                     "/palettes-popup",
                                     "palettes");

  return dialogs_dockable_new (view,
			       _("Palettes"), NULL,
                               GIMP_STOCK_PALETTE,
                               GIMP_HELP_PALETTE_DIALOG);
}

GtkWidget *
dialogs_font_list_view_new (GimpDialogFactory *factory,
                            GimpContext       *context,
                            gint               preview_size)
{
  GtkWidget *view;

  view = gimp_font_view_new (GIMP_VIEW_TYPE_LIST,
                             context->gimp->fonts,
                             context,
                             preview_size, 1,
                             factory->menu_factory);

  return dialogs_dockable_new (view,
			       _("Fonts"), NULL,
                               GIMP_STOCK_FONT,
                               GIMP_HELP_FONT_DIALOG);
}

GtkWidget *
dialogs_tool_list_view_new (GimpDialogFactory *factory,
			    GimpContext       *context,
                            gint               preview_size)
{
  GtkWidget *view;

  view = gimp_tool_view_new (GIMP_VIEW_TYPE_LIST,
                             context->gimp->tool_info_list,
                             context,
                             preview_size, 0,
                             factory->menu_factory);

  return dialogs_dockable_new (view,
			       _("Tools"), NULL,
                               GIMP_STOCK_TOOLS,
                               GIMP_HELP_TOOLS_DIALOG);
}

GtkWidget *
dialogs_buffer_list_view_new (GimpDialogFactory *factory,
			      GimpContext       *context,
                              gint               preview_size)
{
  GtkWidget *view;

  view = gimp_buffer_view_new (GIMP_VIEW_TYPE_LIST,
			       context->gimp->named_buffers,
			       context,
			       preview_size, 1,
                               factory->menu_factory);

  return dialogs_dockable_new (view,
			       _("Buffers"), NULL,
                               GIMP_STOCK_BUFFER,
                               GIMP_HELP_BUFFER_DIALOG);
}

GtkWidget *
dialogs_document_list_new (GimpDialogFactory *factory,
                           GimpContext       *context,
                           gint               preview_size)
{
  GtkWidget *view;

  view = gimp_document_view_new (GIMP_VIEW_TYPE_LIST,
                                 context->gimp->documents,
                                 context,
                                 preview_size, 1,
                                 factory->menu_factory);

  return dialogs_dockable_new (view,
			       _("History"), _("Document History"),
                               GTK_STOCK_OPEN,
                               GIMP_HELP_DOCUMENT_DIALOG);
}

GtkWidget *
dialogs_template_list_new (GimpDialogFactory *factory,
                           GimpContext       *context,
                           gint               preview_size)
{
  GtkWidget *view;

  view = gimp_template_view_new (GIMP_VIEW_TYPE_LIST,
                                 context->gimp->templates,
                                 context,
                                 preview_size, 0,
                                 factory->menu_factory);

  return dialogs_dockable_new (view,
			       _("Templates"), _("Image Templates"),
                               GIMP_STOCK_TEMPLATE,
                               GIMP_HELP_TEMPLATE_DIALOG);
}


/*****  grid views  *****/

GtkWidget *
dialogs_image_grid_view_new (GimpDialogFactory *factory,
			     GimpContext       *context,
                             gint               preview_size)
{
  GtkWidget *view;

  view = gimp_image_view_new (GIMP_VIEW_TYPE_GRID,
                              context->gimp->images,
                              context,
                              preview_size, 1,
                              factory->menu_factory);

  return dialogs_dockable_new (view,
			       _("Images"), NULL,
                               GIMP_STOCK_IMAGES,
                               GIMP_HELP_IMAGE_DIALOG);
}

GtkWidget *
dialogs_brush_grid_view_new (GimpDialogFactory *factory,
			     GimpContext       *context,
                             gint               preview_size)
{
  GtkWidget *view;

  view = gimp_brush_factory_view_new (GIMP_VIEW_TYPE_GRID,
				      context->gimp->brush_factory,
				      dialogs_edit_brush_func,
				      context,
				      TRUE,
				      preview_size, 1,
                                      factory->menu_factory);

  return dialogs_dockable_new (view,
			       _("Brushes"), NULL,
                               GIMP_STOCK_BRUSH,
                               GIMP_HELP_BRUSH_DIALOG);
}

GtkWidget *
dialogs_pattern_grid_view_new (GimpDialogFactory *factory,
			       GimpContext       *context,
                               gint               preview_size)
{
  GtkWidget *view;

  view = gimp_pattern_factory_view_new (GIMP_VIEW_TYPE_GRID,
                                        context->gimp->pattern_factory,
                                        NULL,
                                        context,
                                        preview_size, 1,
                                        factory->menu_factory);

  return dialogs_dockable_new (view,
			       _("Patterns"), NULL,
                               GIMP_STOCK_PATTERN,
                               GIMP_HELP_PATTERN_DIALOG);
}

GtkWidget *
dialogs_gradient_grid_view_new (GimpDialogFactory *factory,
				GimpContext       *context,
                                gint               preview_size)
{
  GtkWidget *view;

  view = gimp_data_factory_view_new (GIMP_VIEW_TYPE_GRID,
                                     context->gimp->gradient_factory,
                                     dialogs_edit_gradient_func,
                                     context,
                                     preview_size, 1,
                                     factory->menu_factory, "<Gradients>",
                                     "/gradients-popup",
                                     "gradients");

  return dialogs_dockable_new (view,
			       _("Gradients"), NULL,
                               GIMP_STOCK_GRADIENT,
                               GIMP_HELP_GRADIENT_DIALOG);
}

GtkWidget *
dialogs_palette_grid_view_new (GimpDialogFactory *factory,
			       GimpContext       *context,
                               gint               preview_size)
{
  GtkWidget *view;

  view = gimp_data_factory_view_new (GIMP_VIEW_TYPE_GRID,
                                     context->gimp->palette_factory,
                                     dialogs_edit_palette_func,
                                     context,
                                     preview_size, 1,
                                     factory->menu_factory, "<Palettes>",
                                     "/palettes-popup",
                                     "palettes");

  return dialogs_dockable_new (view,
			       _("Palettes"), NULL,
                               GIMP_STOCK_PALETTE,
                               GIMP_HELP_PALETTE_DIALOG);
}

GtkWidget *
dialogs_font_grid_view_new (GimpDialogFactory *factory,
                            GimpContext       *context,
                            gint               preview_size)
{
  GtkWidget *view;

  view = gimp_font_view_new (GIMP_VIEW_TYPE_GRID,
                             context->gimp->fonts,
                             context,
                             preview_size, 1,
                             factory->menu_factory);

  return dialogs_dockable_new (view,
			       _("Fonts"), NULL,
                               GIMP_STOCK_FONT,
                               GIMP_HELP_FONT_DIALOG);
}

GtkWidget *
dialogs_tool_grid_view_new (GimpDialogFactory *factory,
			    GimpContext       *context,
                            gint               preview_size)
{
  GtkWidget *view;

  view = gimp_tool_view_new (GIMP_VIEW_TYPE_GRID,
                             context->gimp->tool_info_list,
                             context,
                             preview_size, 1,
                             factory->menu_factory);

  return dialogs_dockable_new (view,
			       _("Tools"), NULL,
                               GIMP_STOCK_TOOLS,
                               GIMP_HELP_TOOLS_DIALOG);
}

GtkWidget *
dialogs_buffer_grid_view_new (GimpDialogFactory *factory,
			      GimpContext       *context,
                              gint               preview_size)
{
  GtkWidget *view;

  view = gimp_buffer_view_new (GIMP_VIEW_TYPE_GRID,
			       context->gimp->named_buffers,
			       context,
			       preview_size, 1,
                               factory->menu_factory);

  return dialogs_dockable_new (view,
			       _("Buffers"), NULL,
                               GIMP_STOCK_BUFFER,
                               GIMP_HELP_BUFFER_DIALOG);
}

GtkWidget *
dialogs_document_grid_new (GimpDialogFactory *factory,
                           GimpContext       *context,
                           gint               preview_size)
{
  GtkWidget *view;

  view = gimp_document_view_new (GIMP_VIEW_TYPE_GRID,
                                 context->gimp->documents,
                                 context,
                                 preview_size, 1,
                                 factory->menu_factory);

  return dialogs_dockable_new (view,
			       _("History"), _("Document History"),
                               GTK_STOCK_OPEN,
                               GIMP_HELP_DOCUMENT_DIALOG);
}


/*****  image related dialogs  *****/

GtkWidget *
dialogs_layer_list_view_new (GimpDialogFactory *factory,
			     GimpContext       *context,
                             gint               preview_size)
{
  GtkWidget *view;
  GtkWidget *dockable;

  if (preview_size < 1)
    preview_size = context->gimp->config->layer_preview_size;

  view =
    gimp_item_tree_view_new (preview_size, 2,
                             gimp_context_get_image (context),
                             GIMP_TYPE_LAYER,
                             "active_layer_changed",
                             (GimpEditItemFunc)     layers_edit_layer_query,
                             (GimpNewItemFunc)      layers_new_layer_query,
                             (GimpActivateItemFunc) layers_text_tool,
                             factory->menu_factory, "<Layers>",
                             "/layers-popup");

  dockable = dialogs_dockable_new (view,
				   _("Layers"), NULL,
                                   GIMP_STOCK_LAYERS,
                                   GIMP_HELP_LAYER_DIALOG);

  gimp_dockable_set_context (GIMP_DOCKABLE (dockable), context);

  return dockable;
}

GtkWidget *
dialogs_channel_list_view_new (GimpDialogFactory *factory,
			       GimpContext       *context,
                               gint               preview_size)
{
  GtkWidget *view;
  GtkWidget *dockable;

  if (preview_size < 1)
    preview_size = context->gimp->config->layer_preview_size;

  view =
    gimp_item_tree_view_new (preview_size, 1,
                             gimp_context_get_image (context),
                             GIMP_TYPE_CHANNEL,
                             "active_channel_changed",
                             (GimpEditItemFunc)     channels_edit_channel_query,
                             (GimpNewItemFunc)      channels_new_channel_query,
                             (GimpActivateItemFunc) channels_edit_channel_query,
                             factory->menu_factory, "<Channels>",
                             "/channels-popup");

  dockable = dialogs_dockable_new (view,
				   _("Channels"), NULL,
                                   GIMP_STOCK_CHANNELS,
                                   GIMP_HELP_CHANNEL_DIALOG);

  gimp_dockable_set_context (GIMP_DOCKABLE (dockable), context);

  return dockable;
}

GtkWidget *
dialogs_vectors_list_view_new (GimpDialogFactory *factory,
			       GimpContext       *context,
                               gint               preview_size)
{
  GtkWidget *view;
  GtkWidget *dockable;

  if (preview_size < 1)
    preview_size = context->gimp->config->layer_preview_size;

  view =
    gimp_item_tree_view_new (preview_size, 1,
                             gimp_context_get_image (context),
                             GIMP_TYPE_VECTORS,
                             "active_vectors_changed",
                             (GimpEditItemFunc)     vectors_edit_vectors_query,
                             (GimpNewItemFunc)      vectors_new_vectors_query,
                             (GimpActivateItemFunc) vectors_vectors_tool,
                             factory->menu_factory, "<Vectors>",
                             "/vectors-popup");

  dockable = dialogs_dockable_new (view,
				   _("Paths"), NULL,
                                   GIMP_STOCK_PATHS,
                                   GIMP_HELP_PATH_DIALOG);

  gimp_dockable_set_context (GIMP_DOCKABLE (dockable), context);

  return dockable;
}

GtkWidget *
dialogs_indexed_palette_new (GimpDialogFactory *factory,
			     GimpContext       *context,
                             gint               preview_size)
{
  GtkWidget *view;
  GtkWidget *dockable;

  view = gimp_colormap_editor_new (gimp_context_get_image (context),
                                   factory->menu_factory);

  dockable = dialogs_dockable_new (view,
				   _("Colormap"), _("Indexed Palette"),
                                   GIMP_STOCK_INDEXED_PALETTE,
                                   GIMP_HELP_INDEXED_PALETTE_DIALOG);

  gimp_dockable_set_context (GIMP_DOCKABLE (dockable), context);

  g_signal_connect (view, "selected",
		    G_CALLBACK (dialogs_indexed_palette_selected),
		    dockable);

  return dockable;
}

GtkWidget *
dialogs_histogram_editor_new (GimpDialogFactory *factory,
                              GimpContext       *context,
                              gint               preview_size)
{
  GtkWidget *view;
  GtkWidget *dockable;

  view = gimp_histogram_editor_new (gimp_context_get_image (context));

  dockable = dialogs_dockable_new (view,
				   _("Histogram"), NULL,
                                   GIMP_STOCK_HISTOGRAM,
                                   GIMP_HELP_HISTOGRAM_DIALOG);

  gimp_dockable_set_context (GIMP_DOCKABLE (dockable), context);

  return dockable;
}

GtkWidget *
dialogs_selection_editor_new (GimpDialogFactory *factory,
                              GimpContext       *context,
                              gint               preview_size)
{
  GtkWidget *view;
  GtkWidget *dockable;

  view = gimp_selection_editor_new (gimp_context_get_image (context),
                                    factory->menu_factory);

  dockable = dialogs_dockable_new (view,
				   _("Selection"), _("Selection Editor"),
                                   GIMP_STOCK_TOOL_RECT_SELECT,
                                   GIMP_HELP_SELECTION_DIALOG);

  gimp_dockable_set_context (GIMP_DOCKABLE (dockable), context);

  return dockable;
}

GtkWidget *
dialogs_undo_history_new (GimpDialogFactory *factory,
                          GimpContext       *context,
                          gint               preview_size)
{
  GtkWidget *editor;
  GtkWidget *dockable;
  GimpImage *gimage;

  editor = gimp_undo_editor_new (context->gimp->config,
                                 factory->menu_factory);

  gimage = gimp_context_get_image (context);
  if (gimage)
    gimp_image_editor_set_image (GIMP_IMAGE_EDITOR (editor), gimage);

  dockable = dialogs_dockable_new (editor,
				   _("Undo"), _("Undo History"),
                                   GIMP_STOCK_UNDO_HISTORY,
                                   GIMP_HELP_UNDO_DIALOG);

  gimp_dockable_set_context (GIMP_DOCKABLE (dockable), context);

  return dockable;
}


/*****  display related dialogs  *****/

GtkWidget *
dialogs_navigation_view_new (GimpDialogFactory *factory,
                             GimpContext       *context,
                             gint               preview_size)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell = NULL;
  GtkWidget        *view;

  gdisp = gimp_context_get_display (context);

  if (gdisp)
    shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  view = gimp_navigation_editor_new (shell,
                                     GIMP_DISPLAY_CONFIG (context->gimp->config));

  return dialogs_dockable_new (view,
                               _("Navigation"), _("Display Navigation"),
                               GIMP_STOCK_NAVIGATION,
                               GIMP_HELP_NAVIGATION_DIALOG);
}


/*****  misc dockables  *****/

GtkWidget *
dialogs_color_editor_new (GimpDialogFactory *factory,
                          GimpContext       *context,
                          gint               preview_size)
{
  GtkWidget *view;

  view = gimp_color_editor_new (context);

  return dialogs_dockable_new (view,
			       _("FG/BG"), _("FG/BG Color"),
                               GIMP_STOCK_DEFAULT_COLORS,
                               GIMP_HELP_COLOR_DIALOG);
}


/*****  editors  *****/

/*  the brush editor  */

static GimpDataEditor *brush_editor = NULL;

GtkWidget *
dialogs_brush_editor_get (GimpDialogFactory *factory,
			  GimpContext       *context,
                          gint               preview_size)
{
  brush_editor = gimp_brush_editor_new (context->gimp);

  return dialogs_dockable_new (GTK_WIDGET (brush_editor),
                               _("Brush Editor"), NULL,
                               GIMP_STOCK_BRUSH,
                               GIMP_HELP_BRUSH_EDITOR_DIALOG);
}

void
dialogs_edit_brush_func (GimpData  *data,
                         GtkWidget *parent)
{
  gimp_dialog_factory_dialog_raise (global_dock_factory,
                                    gtk_widget_get_screen (parent),
                                    "gimp-brush-editor",
                                    -1);

  gimp_data_editor_set_data (brush_editor, data);
}


/*  the gradient editor  */

static GimpDataEditor *gradient_editor = NULL;

GtkWidget *
dialogs_gradient_editor_get (GimpDialogFactory *factory,
			     GimpContext       *context,
                             gint               preview_size)
{
  gradient_editor = gimp_gradient_editor_new (context->gimp,
                                              factory->menu_factory);

  return dialogs_dockable_new (GTK_WIDGET (gradient_editor),
			       _("Gradient Editor"), NULL,
                               GIMP_STOCK_GRADIENT,
                               GIMP_HELP_GRADIENT_EDITOR_DIALOG);
}

void
dialogs_edit_gradient_func (GimpData  *data,
                            GtkWidget *parent)
{
  gimp_dialog_factory_dialog_raise (global_dock_factory,
                                    gtk_widget_get_screen (parent),
				    "gimp-gradient-editor",
                                    -1);

  gimp_data_editor_set_data (gradient_editor, data);
}


/*  the palette editor  */

static GimpDataEditor *palette_editor = NULL;

GtkWidget *
dialogs_palette_editor_get (GimpDialogFactory *factory,
			    GimpContext       *context,
                            gint               preview_size)
{
  palette_editor = gimp_palette_editor_new (context->gimp,
                                            factory->menu_factory);

  return dialogs_dockable_new (GTK_WIDGET (palette_editor),
			       _("Palette Editor"), NULL,
                               GIMP_STOCK_PALETTE,
                               GIMP_HELP_PALETTE_EDITOR_DIALOG);
}

void
dialogs_edit_palette_func (GimpData  *data,
                           GtkWidget *parent)
{
  gimp_dialog_factory_dialog_raise (global_dock_factory,
                                    gtk_widget_get_screen (parent),
				    "gimp-palette-editor",
                                    -1);

  gimp_data_editor_set_data (palette_editor, data);
}


/*  private functions  */

static GtkWidget *
dialogs_dockable_new (GtkWidget   *widget,
		      const gchar *name,
		      const gchar *blurb,
                      const gchar *stock_id,
                      const gchar *help_id)
{
  GtkWidget *dockable;

  dockable = gimp_dockable_new (name, blurb, stock_id, help_id);
  gtk_container_add (GTK_CONTAINER (dockable), widget);
  gtk_widget_show (widget);

  return dockable;
}

static void
dialogs_indexed_palette_selected (GimpColormapEditor *editor,
                                  GdkModifierType     state,
				  GimpDockable       *dockable)
{
  GimpImage *gimage = GIMP_IMAGE_EDITOR (editor)->gimage;

  if (gimage)
    {
      GimpRGB color;
      gint    index;

      index = gimp_colormap_editor_col_index (editor);

      gimp_image_get_colormap_entry (gimage, index, &color);

      if (state & GDK_CONTROL_MASK)
        gimp_context_set_background (dockable->context, &color);
      else
        gimp_context_set_foreground (dockable->context, &color);
    }
}
