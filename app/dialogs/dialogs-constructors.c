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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimptoolinfo.h"

#include "vectors/gimpvectors.h"

#include "config/gimpdisplayconfig.h"

#include "widgets/gimpbrusheditor.h"
#include "widgets/gimpbrushfactoryview.h"
#include "widgets/gimpbufferview.h"
#include "widgets/gimpcoloreditor.h"
#include "widgets/gimpcolormapeditor.h"
#include "widgets/gimpcontainergridview.h"
#include "widgets/gimpcontainertreeview.h"
#include "widgets/gimpdataeditor.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimperrorconsole.h"
#include "widgets/gimpimagedock.h"
#include "widgets/gimpimageview.h"
#include "widgets/gimpitemtreeview.h"
#include "widgets/gimpdevicestatus.h"
#include "widgets/gimpdockable.h"
#include "widgets/gimpdockbook.h"
#include "widgets/gimpdocumentview.h"
#include "widgets/gimpgradienteditor.h"
#include "widgets/gimppaletteeditor.h"
#include "widgets/gimppreview.h"
#include "widgets/gimppreviewrenderer.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimpselectioneditor.h"
#include "widgets/gimptemplateview.h"
#include "widgets/gimptoolbox.h"
#include "widgets/gimptoolbox-color-area.h"
#include "widgets/gimpundoeditor.h"
#include "widgets/gimpvectorstreeview.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-filter-dialog.h"
#include "display/gimpnavigationview.h"

#include "about-dialog.h"
#include "channels-commands.h"
#include "dialogs.h"
#include "dialogs-constructors.h"
#include "file-commands.h"
#include "file-new-dialog.h"
#include "layers-commands.h"
#include "module-browser.h"
#include "preferences-dialog.h"
#include "templates-commands.h"
#include "tips-dialog.h"
#include "tool-options-dialog.h"
#include "vectors-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void dialogs_indexed_palette_selected      (GimpColormapEditor *editor,
                                                   GimpDockable       *dockable);

static GtkWidget * dialogs_viewable_preview_func  (GimpDockable       *dockable,
                                                   GimpContext        *context,
                                                   GtkIconSize         size,
                                                   gpointer            data);
static GtkWidget * dialogs_tool_options_preview_func (GimpDockable    *dockable,
                                                   GimpContext        *context,
                                                   GtkIconSize         size,
                                                   gpointer            data);

static void   dialogs_set_view_context_func         (GimpDockable     *dockable,
                                                     GimpContext      *context);
static void   dialogs_set_editor_context_func       (GimpDockable     *dockable,
                                                     GimpContext      *context);
static void   dialogs_set_color_editor_context_func (GimpDockable     *dockable,
                                                     GimpContext      *context);
static void   dialogs_set_image_item_context_func   (GimpDockable     *dockable,
                                                     GimpContext      *context);
static void   dialogs_set_image_editor_context_func (GimpDockable     *dockable,
                                                     GimpContext      *context);
static void   dialogs_set_navigation_context_func   (GimpDockable     *dockable,
                                                     GimpContext      *context);

static GtkWidget * dialogs_dockable_new (GtkWidget                  *widget,
					 const gchar                *name,
					 const gchar                *blurb,
                                         const gchar                *stock_id,
					 GimpDockableGetPreviewFunc  get_preview_func,
                                         gpointer                    get_preview_data,
					 GimpDockableSetContextFunc  set_context_func);

static void dialogs_image_item_view_image_changed  (GimpContext         *context,
                                                    GimpImage           *gimage,
                                                    GimpContainerView   *view);
static void dialogs_image_editor_image_changed     (GimpContext         *context,
                                                    GimpImage           *gimage,
                                                    GimpImageEditor     *editor);
static void dialogs_navigation_display_changed     (GimpContext         *context,
                                                    GimpDisplay         *gdisp,
                                                    GimpNavigationView  *view);


/**********************/
/*  toplevel dialogs  */
/**********************/

GtkWidget *
dialogs_file_new_new (GimpDialogFactory *factory,
                      GimpContext       *context,
                      gint               preview_size)
{
  return file_new_dialog_new (context->gimp);
}

GtkWidget *
dialogs_preferences_get (GimpDialogFactory *factory,
			 GimpContext       *context,
                         gint               preview_size)
{
  return preferences_dialog_create (context->gimp);
}

GtkWidget *
dialogs_module_browser_get (GimpDialogFactory *factory,
			    GimpContext       *context,
                            gint               preview_size)
{
  return module_browser_new (context->gimp);
}

GtkWidget *
dialogs_display_filters_get (GimpDialogFactory *factory,
			     GimpContext       *context,
                             gint               preview_size)
{
  GimpDisplay *gdisp;

  gdisp = gimp_context_get_display (context);

  if (gdisp)
    {
      GimpDisplayShell *shell;

      shell = GIMP_DISPLAY_SHELL (gdisp->shell);

      if (! shell->filters_dialog)
        gimp_display_shell_filter_dialog_new (shell);

      return shell->filters_dialog;
    }

  return NULL;
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

  view = tool_options_dialog_create (context->gimp);

  g_object_add_weak_pointer (G_OBJECT (view), (gpointer *) &view);

  return dialogs_dockable_new (view,
                               _("Tool Options"), _("Tool Options"),
                               GIMP_STOCK_TOOL_OPTIONS,
                               dialogs_tool_options_preview_func, NULL,
                               NULL);
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

  g_object_add_weak_pointer (G_OBJECT (view), (gpointer *) &view);

  return dialogs_dockable_new (view,
                               _("Devices"), _("Device Status"),
                               GIMP_STOCK_DEVICE_STATUS,
                               NULL, NULL,
                               NULL);
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

  g_object_add_weak_pointer (G_OBJECT (view), (gpointer *) &view);

  return dialogs_dockable_new (view,
                               _("Errors"), _("Error Console"),
                               GIMP_STOCK_WARNING,
                               NULL, NULL,
                               NULL);
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
			       _("Images"), _("Image List"),
                               GIMP_STOCK_IMAGES,
			       NULL, NULL,
			       dialogs_set_editor_context_func);
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
			       _("Brushes"), _("Brush List"),
                               GIMP_STOCK_TOOL_PAINTBRUSH,
			       dialogs_viewable_preview_func, "brush",
			       dialogs_set_editor_context_func);
}

GtkWidget *
dialogs_pattern_list_view_new (GimpDialogFactory *factory,
			       GimpContext       *context,
                               gint               preview_size)
{
  GtkWidget *view;

  view = gimp_data_factory_view_new (GIMP_VIEW_TYPE_LIST,
				     context->gimp->pattern_factory,
				     NULL,
				     context,
				     preview_size, 1,
				     factory->menu_factory, "<Patterns>");

  return dialogs_dockable_new (view,
			       _("Patterns"), _("Pattern List"),
                               GIMP_STOCK_TOOL_BUCKET_FILL,
			       dialogs_viewable_preview_func, "pattern",
			       dialogs_set_editor_context_func);
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
                                     factory->menu_factory, "<Gradients>");

  return dialogs_dockable_new (view,
			       _("Gradients"), _("Gradient List"),
                               GIMP_STOCK_TOOL_BLEND,
			       dialogs_viewable_preview_func, "gradient",
			       dialogs_set_editor_context_func);
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
                                     factory->menu_factory, "<Palettes>");

  return dialogs_dockable_new (view,
			       _("Palettes"), _("Palette List"),
                               GTK_STOCK_SELECT_COLOR,
			       dialogs_viewable_preview_func, "palette",
			       dialogs_set_editor_context_func);
}

GtkWidget *
dialogs_font_list_view_new (GimpDialogFactory *factory,
                            GimpContext       *context,
                            gint               preview_size)
{
  GtkWidget *view;

  view = gimp_container_tree_view_new (context->gimp->fonts,
				       context,
				       preview_size, 1,
                                       FALSE);

  return dialogs_dockable_new (view,
			       _("Fonts"), _("Font List"),
                               GTK_STOCK_SELECT_FONT,
			       dialogs_viewable_preview_func, "font",
			       dialogs_set_view_context_func);
}

GtkWidget *
dialogs_tool_list_view_new (GimpDialogFactory *factory,
			    GimpContext       *context,
                            gint               preview_size)
{
  GtkWidget *view;

  view = gimp_container_tree_view_new (context->gimp->tool_info_list,
				       context,
				       preview_size, 0,
                                       FALSE);

  return dialogs_dockable_new (view,
			       _("Tools"), _("Tool List"),
                               GTK_STOCK_MISSING_IMAGE,
			       dialogs_viewable_preview_func, "tool",
			       dialogs_set_view_context_func);
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
			       _("Buffers"), _("Buffer List"),
                               GTK_STOCK_PASTE,
                               NULL, NULL,
			       dialogs_set_editor_context_func);
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
                                 file_file_open_dialog,
                                 factory->menu_factory);

  return dialogs_dockable_new (view,
			       _("History"), _("Document History List"),
                               GTK_STOCK_OPEN,
                               NULL, NULL,
			       dialogs_set_editor_context_func);
}

GtkWidget *
dialogs_template_list_new (GimpDialogFactory *factory,
                           GimpContext       *context,
                           gint               preview_size)
{
  GtkWidget        *view;
  GimpTemplateView *template_view;

  view = gimp_template_view_new (GIMP_VIEW_TYPE_LIST,
                                 context->gimp->templates,
                                 context,
                                 preview_size, 0,
                                 factory->menu_factory);

  template_view = GIMP_TEMPLATE_VIEW (view);

  template_view->new_template_func  = templates_new_template_dialog;
  template_view->edit_template_func = templates_edit_template_dialog;
  template_view->create_image_func  = templates_file_new_dialog;

  return dialogs_dockable_new (view,
			       _("Templates"), _("List of Templates"),
                               GIMP_STOCK_TEMPLATE,
			       NULL, NULL,
			       dialogs_set_editor_context_func);
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
			       _("Images"), _("Image Grid"),
                               GIMP_STOCK_IMAGES,
			       NULL, NULL,
			       dialogs_set_editor_context_func);
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
			       _("Brushes"), _("Brush Grid"),
                               GIMP_STOCK_TOOL_PAINTBRUSH,
			       dialogs_viewable_preview_func, "brush",
			       dialogs_set_editor_context_func);
}

GtkWidget *
dialogs_pattern_grid_view_new (GimpDialogFactory *factory,
			       GimpContext       *context,
                               gint               preview_size)
{
  GtkWidget *view;

  view = gimp_data_factory_view_new (GIMP_VIEW_TYPE_GRID,
				     context->gimp->pattern_factory,
				     NULL,
				     context,
				     preview_size, 1,
				     factory->menu_factory, "<Patterns>");

  return dialogs_dockable_new (view,
			       _("Patterns"), _("Pattern Grid"),
                               GIMP_STOCK_TOOL_BUCKET_FILL,
			       dialogs_viewable_preview_func, "pattern",
			       dialogs_set_editor_context_func);
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
                                     factory->menu_factory, "<Gradients>");

  return dialogs_dockable_new (view,
			       _("Gradients"), _("Gradient Grid"),
                               GIMP_STOCK_TOOL_BLEND,
			       dialogs_viewable_preview_func, "gradient",
			       dialogs_set_editor_context_func);
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
                                     factory->menu_factory, "<Palettes>");

  return dialogs_dockable_new (view,
			       _("Palettes"), _("Palette Grid"),
                               GTK_STOCK_SELECT_COLOR,
			       dialogs_viewable_preview_func, "palette",
			       dialogs_set_editor_context_func);
}

GtkWidget *
dialogs_font_grid_view_new (GimpDialogFactory *factory,
                            GimpContext       *context,
                            gint               preview_size)
{
  GtkWidget *view;

  view = gimp_container_grid_view_new (context->gimp->fonts,
				       context,
				       preview_size, 1,
                                       FALSE);

  return dialogs_dockable_new (view,
			       _("Fonts"), _("Font Grid"),
                               GTK_STOCK_SELECT_FONT,
			       dialogs_viewable_preview_func, "font",
			       dialogs_set_view_context_func);
}

GtkWidget *
dialogs_tool_grid_view_new (GimpDialogFactory *factory,
			    GimpContext       *context,
                            gint               preview_size)
{
  GtkWidget *view;

  view = gimp_container_grid_view_new (context->gimp->tool_info_list,
				       context,
				       preview_size, 1,
                                       FALSE);

  return dialogs_dockable_new (view,
			       _("Tools"), _("Tool Grid"),
                               GTK_STOCK_MISSING_IMAGE,
			       dialogs_viewable_preview_func, "tool",
			       dialogs_set_view_context_func);
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
			       _("Buffers"), _("Buffer Grid"),
                               GTK_STOCK_PASTE,
			       NULL, NULL,
			       dialogs_set_editor_context_func);
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
                                 file_file_open_dialog,
                                 factory->menu_factory);

  return dialogs_dockable_new (view,
			       _("History"), _("Document History Grid"),
                               GTK_STOCK_OPEN,
                               NULL, NULL,
			       dialogs_set_editor_context_func);
}


/*****  image related dialogs  *****/

GtkWidget *
dialogs_layer_list_view_new (GimpDialogFactory *factory,
			     GimpContext       *context,
                             gint               preview_size)
{
  GimpImage *gimage;
  GtkWidget *view;
  GtkWidget *dockable;

  gimage = gimp_context_get_image (context);

  if (preview_size < 1)
    preview_size = context->gimp->config->layer_preview_size;

  view =
    gimp_item_tree_view_new (preview_size, 2,
                             gimage,
                             GIMP_TYPE_LAYER,
                             "active_layer_changed",
                             (GimpNewItemFunc)      layers_new_layer_query,
                             (GimpEditItemFunc)     layers_edit_layer_query,
                             (GimpActivateItemFunc) layers_edit_layer_query,
                             factory->menu_factory, "<Layers>");

  dockable = dialogs_dockable_new (view,
				   _("Layers"), _("Layer List"),
                                   GIMP_STOCK_LAYERS,
				   NULL, NULL,
				   dialogs_set_image_item_context_func);

  gimp_dockable_set_context (GIMP_DOCKABLE (dockable), context);

  return dockable;
}

GtkWidget *
dialogs_channel_list_view_new (GimpDialogFactory *factory,
			       GimpContext       *context,
                               gint               preview_size)
{
  GimpImage *gimage;
  GtkWidget *view;
  GtkWidget *dockable;

  gimage = gimp_context_get_image (context);

  if (preview_size < 1)
    preview_size = context->gimp->config->layer_preview_size;

  view =
    gimp_item_tree_view_new (preview_size, 1,
                             gimage,
                             GIMP_TYPE_CHANNEL,
                             "active_channel_changed",
                             (GimpNewItemFunc)      channels_new_channel_query,
                             (GimpEditItemFunc)     channels_edit_channel_query,
                             (GimpActivateItemFunc) channels_edit_channel_query,
                             factory->menu_factory, "<Channels>");

  dockable = dialogs_dockable_new (view,
				   _("Channels"), _("Channel List"),
                                   GIMP_STOCK_CHANNELS,
				   NULL, NULL,
				   dialogs_set_image_item_context_func);

  gimp_dockable_set_context (GIMP_DOCKABLE (dockable), context);

  return dockable;
}

GtkWidget *
dialogs_vectors_list_view_new (GimpDialogFactory *factory,
			       GimpContext       *context,
                               gint               preview_size)
{
  GimpImage           *gimage;
  GimpVectorsTreeView *vectors_view;
  GtkWidget           *view;
  GtkWidget           *dockable;

  gimage = gimp_context_get_image (context);

  if (preview_size < 1)
    preview_size = context->gimp->config->layer_preview_size;

  view =
    gimp_item_tree_view_new (preview_size, 1,
                             gimage,
                             GIMP_TYPE_VECTORS,
                             "active_vectors_changed",
                             (GimpNewItemFunc)      vectors_new_vectors_query,
                             (GimpEditItemFunc)     vectors_edit_vectors_query,
                             (GimpActivateItemFunc) vectors_vectors_tool,
                             factory->menu_factory, "<Vectors>");

  vectors_view = GIMP_VECTORS_TREE_VIEW (view);

  vectors_view->stroke_item_func          = vectors_stroke_vectors;
  vectors_view->selection_to_vectors_func = vectors_selection_to_vectors;

  dockable = dialogs_dockable_new (view,
				   _("Paths"), _("Path List"),
                                   GIMP_STOCK_PATHS,
				   NULL, NULL,
				   dialogs_set_image_item_context_func);

  gimp_dockable_set_context (GIMP_DOCKABLE (dockable), context);

  return dockable;
}

GtkWidget *
dialogs_indexed_palette_new (GimpDialogFactory *factory,
			     GimpContext       *context,
                             gint               preview_size)
{
  GimpImage *gimage;
  GtkWidget *view;
  GtkWidget *dockable;

  gimage = gimp_context_get_image (context);

  view = gimp_colormap_editor_new (gimage, factory->menu_factory);

  dockable = dialogs_dockable_new (view,
				   _("Colormap"), _("Indexed Palette"),
                                   GIMP_STOCK_INDEXED_PALETTE,
				   NULL, NULL,
				   dialogs_set_image_editor_context_func);

  gimp_dockable_set_context (GIMP_DOCKABLE (dockable), context);

  g_signal_connect (view, "selected",
		    G_CALLBACK (dialogs_indexed_palette_selected),
		    dockable);

  return dockable;
}

GtkWidget *
dialogs_selection_editor_new (GimpDialogFactory *factory,
                              GimpContext       *context,
                              gint               preview_size)
{
  GimpImage           *gimage;
  GtkWidget           *view;
  GtkWidget           *dockable;
  GimpSelectionEditor *selection_editor;

  gimage = gimp_context_get_image (context);

  view = gimp_selection_editor_new (gimage);

  selection_editor = GIMP_SELECTION_EDITOR (view);

  selection_editor->selection_to_vectors_func = vectors_selection_to_vectors;

  dockable = dialogs_dockable_new (view,
				   _("Selection"), _("Selection Editor"),
                                   GIMP_STOCK_TOOL_RECT_SELECT,
				   NULL, NULL,
				   dialogs_set_image_editor_context_func);

  gimp_dockable_set_context (GIMP_DOCKABLE (dockable), context);

  return dockable;
}

GtkWidget *
dialogs_undo_history_new (GimpDialogFactory *factory,
                          GimpContext       *context,
                          gint               preview_size)
{
  GimpImage *gimage;
  GtkWidget *view;
  GtkWidget *dockable;

  gimage = gimp_context_get_image (context);

  view = gimp_undo_editor_new (gimage);

  dockable = dialogs_dockable_new (view,
				   _("Undo"), _("Undo History"),
                                   GIMP_STOCK_UNDO_HISTORY,
                                   NULL, NULL,
				   dialogs_set_image_editor_context_func);

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

  view = gimp_navigation_view_new (shell,
                                   GIMP_DISPLAY_CONFIG (context->gimp->config));

  return dialogs_dockable_new (view,
                               _("Navigation"), _("Display Navigation"),
                               GIMP_STOCK_NAVIGATION,
                               NULL, NULL,
                               dialogs_set_navigation_context_func);
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
                               NULL, NULL,
			       dialogs_set_color_editor_context_func);
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
                               _("Brush Editor"), _("Brush Editor"),
                               GIMP_STOCK_TOOL_PAINTBRUSH,
                               NULL, NULL,
                               NULL);
}

void
dialogs_edit_brush_func (GimpData *data)
{
  gimp_dialog_factory_dialog_raise (global_dock_factory,
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
			       _("Gradient Editor"), _("Gradient Editor"),
                               GIMP_STOCK_TOOL_BLEND,
			       NULL, NULL,
                               NULL);
}

void
dialogs_edit_gradient_func (GimpData *data)
{
  gimp_dialog_factory_dialog_raise (global_dock_factory,
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
			       _("Palette Editor"), _("Palette Editor"),
                               GTK_STOCK_SELECT_COLOR,
			       NULL, NULL,
                               NULL);
}

void
dialogs_edit_palette_func (GimpData *data)
{
  gimp_dialog_factory_dialog_raise (global_dock_factory,
				    "gimp-palette-editor",
                                    -1);

  gimp_data_editor_set_data (palette_editor, data);
}


/*  private functions  */

static void
dialogs_indexed_palette_selected (GimpColormapEditor *editor,
				  GimpDockable       *dockable)
{
  GimpContext *context;

  context = (GimpContext *) g_object_get_data (G_OBJECT (editor),
					       "gimp-dialogs-context");

  if (context)
    {
      GimpImage *gimage;
      GimpRGB    color;
      gint       index;

      gimage = GIMP_IMAGE_EDITOR (editor)->gimage;
      index  = gimp_colormap_editor_col_index (editor);

      gimp_rgba_set_uchar (&color,
			   gimage->cmap[index * 3],
			   gimage->cmap[index * 3 + 1],
			   gimage->cmap[index * 3 + 2],
			   255);

      if (active_color == FOREGROUND)
	gimp_context_set_foreground (context, &color);
      else if (active_color == BACKGROUND)
	gimp_context_set_background (context, &color);
    }
}

static GtkWidget *
dialogs_viewable_preview_func (GimpDockable *dockable,
                               GimpContext  *context,
                               GtkIconSize   size,
                               gpointer      preview_data)
{
  GtkWidget   *preview;
  gint         width;
  gint         height;
  const gchar *prop_name;
  gboolean     is_tool;

  gtk_icon_size_lookup (size, &width, &height);

  prop_name = (const gchar *) preview_data;

  is_tool = (strcmp (prop_name, "tool") == 0);

  preview = gimp_prop_preview_new (G_OBJECT (context), prop_name, height);
  GIMP_PREVIEW (preview)->renderer->size = -1;
  gimp_preview_renderer_set_size_full (GIMP_PREVIEW (preview)->renderer,
                                       width, height,
                                       is_tool ? 0 : 1);

  return preview;
}

static void
dialogs_tool_options_tool_changed (GimpContext  *context,
                                   GimpToolInfo *tool_info,
                                   GtkLabel     *label)
{
  GtkImage *image;

  if ((image = g_object_get_data (G_OBJECT (label), "tool-icon")))
    {
      const gchar *stock_id;

      stock_id = gimp_viewable_get_stock_id (GIMP_VIEWABLE (tool_info));
      gtk_image_set_from_stock (image, stock_id, image->icon_size);
    }

  gtk_label_set_text (label, tool_info->blurb);

  gimp_help_set_help_data (GTK_WIDGET (label)->parent->parent,
                           tool_info->help,
                           tool_info->help_data);
}

static GtkWidget *
dialogs_tool_options_preview_func (GimpDockable *dockable,
                                   GimpContext  *context,
                                   GtkIconSize   size,
                                   gpointer      preview_data)
{
  GimpToolInfo *tool_info;
  GtkWidget    *hbox;
  GtkWidget    *image;
  GtkWidget    *label;
  gint          width;
  gint          height;
  const gchar  *stock_id;

  gtk_icon_size_lookup (size, &width, &height);

  tool_info = gimp_context_get_tool (context);

  hbox = gtk_hbox_new (FALSE, 2);

  stock_id = gimp_viewable_get_stock_id (GIMP_VIEWABLE (tool_info));
  image = gtk_image_new_from_stock (stock_id, size);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  label = gtk_label_new (tool_info->blurb);

  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  g_object_set_data (G_OBJECT (label), "tool-icon", image);

  g_signal_connect_object (context, "tool_changed",
			   G_CALLBACK (dialogs_tool_options_tool_changed),
			   label,
			   0);

  return hbox;
}

static void
dialogs_set_view_context_func (GimpDockable *dockable,
			       GimpContext  *context)
{
  GtkWidget *widget = GTK_BIN (dockable)->child;

  if (GIMP_IS_CONTAINER_VIEW (widget))
    gimp_container_view_set_context (GIMP_CONTAINER_VIEW (widget), context);
}

static void
dialogs_set_editor_context_func (GimpDockable *dockable,
				 GimpContext  *context)
{
  GtkWidget *widget = GTK_BIN (dockable)->child;

  if (GIMP_IS_CONTAINER_EDITOR (widget))
    gimp_container_view_set_context (GIMP_CONTAINER_EDITOR (widget)->view,
                                     context);
}

static void
dialogs_set_color_editor_context_func (GimpDockable *dockable,
                                       GimpContext  *context)
{
  GtkWidget *widget = GTK_BIN (dockable)->child;

  if (GIMP_IS_COLOR_EDITOR (widget))
    gimp_color_editor_set_context (GIMP_COLOR_EDITOR (widget), context);
}

static void
dialogs_set_image_item_context_func (GimpDockable *dockable,
                                     GimpContext  *context)
{
  GtkWidget *widget = GTK_BIN (dockable)->child;
  GimpImage *gimage = NULL;

  if (! GIMP_IS_CONTAINER_VIEW (widget))
    return;

  if (dockable->context)
    g_signal_handlers_disconnect_by_func (dockable->context,
                                          dialogs_image_item_view_image_changed,
                                          widget);

  if (context)
    {
      g_signal_connect (context, "image_changed",
                        G_CALLBACK (dialogs_image_item_view_image_changed),
                        widget);

      gimage = gimp_context_get_image (context);
    }

  dialogs_image_item_view_image_changed (context, gimage,
                                         GIMP_CONTAINER_VIEW (widget));
}

static void
dialogs_set_image_editor_context_func (GimpDockable *dockable,
                                       GimpContext  *context)
{
  GtkWidget *widget = GTK_BIN (dockable)->child;
  GimpImage *gimage = NULL;

  if (! GIMP_IS_IMAGE_EDITOR (widget))
    return;

  if (dockable->context)
    g_signal_handlers_disconnect_by_func (dockable->context,
                                          dialogs_image_editor_image_changed,
                                          widget);

  if (context)
    {
      g_signal_connect (context, "image_changed",
                        G_CALLBACK (dialogs_image_editor_image_changed),
                        widget);

      gimage = gimp_context_get_image (context);
    }

  dialogs_image_editor_image_changed (context, gimage,
                                      GIMP_IMAGE_EDITOR (widget));
}

static void
dialogs_set_navigation_context_func (GimpDockable *dockable,
                                     GimpContext  *context)
{
  GtkWidget   *widget = GTK_BIN (dockable)->child;
  GimpDisplay *gdisp = NULL;

  if (! GIMP_IS_NAVIGATION_VIEW (widget))
    return;

  if (dockable->context)
    g_signal_handlers_disconnect_by_func (dockable->context,
                                          dialogs_navigation_display_changed,
                                          widget);

  if (context)
    {
      g_signal_connect (context, "display_changed",
                        G_CALLBACK (dialogs_navigation_display_changed),
                        widget);

      gdisp = gimp_context_get_display (context);
    }

  dialogs_navigation_display_changed (context, gdisp,
                                      GIMP_NAVIGATION_VIEW (widget));
}

static GtkWidget *
dialogs_dockable_new (GtkWidget                  *widget,
		      const gchar                *name,
		      const gchar                *blurb,
                      const gchar                *stock_id,
		      GimpDockableGetPreviewFunc  get_preview_func,
                      gpointer                    get_preview_data,
		      GimpDockableSetContextFunc  set_context_func)
{
  GtkWidget *dockable;

  dockable = gimp_dockable_new (name, blurb, stock_id,
                                get_preview_func,
                                get_preview_data,
                                set_context_func);
  gtk_container_add (GTK_CONTAINER (dockable), widget);
  gtk_widget_show (widget);

  return dockable;
}

static void
dialogs_image_item_view_image_changed (GimpContext       *context,
                                       GimpImage         *gimage,
                                       GimpContainerView *view)
{
  gimp_item_tree_view_set_image (GIMP_ITEM_TREE_VIEW (view), gimage);
}

static void
dialogs_image_editor_image_changed (GimpContext     *context,
                                    GimpImage       *gimage,
                                    GimpImageEditor *editor)
{
  gimp_image_editor_set_image (editor, gimage);
}

static void
dialogs_navigation_display_changed (GimpContext        *context,
                                    GimpDisplay        *gdisp,
                                    GimpNavigationView *view)
{
  GimpDisplayShell *shell = NULL;

  if (gdisp)
    shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  gimp_navigation_view_set_shell (view, shell);
}
