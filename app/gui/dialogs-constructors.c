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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimpbrushgenerated.h"
#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpgradient.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimppalette.h"
#include "core/gimptoolinfo.h"

#include "vectors/gimpvectors.h"

#include "widgets/gimpbrusheditor.h"
#include "widgets/gimpbrushfactoryview.h"
#include "widgets/gimpbufferview.h"
#include "widgets/gimpcontainerlistview.h"
#include "widgets/gimpcontainergridview.h"
#include "widgets/gimpdataeditor.h"
#include "widgets/gimpdatafactoryview.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpimagedock.h"
#include "widgets/gimpitemfactory.h"
#include "widgets/gimpitemlistview.h"
#include "widgets/gimpdockable.h"
#include "widgets/gimpdockbook.h"
#include "widgets/gimpdocumentview.h"
#include "widgets/gimpgradienteditor.h"
#include "widgets/gimplayerlistview.h"
#include "widgets/gimplistitem.h"
#include "widgets/gimppaletteeditor.h"
#include "widgets/gimppreview.h"
#include "widgets/gimpvectorslistview.h"

#include "about-dialog.h"
#include "brush-select.h"
#include "brushes-commands.h"
#include "buffers-commands.h"
#include "channels-commands.h"
#include "color-area.h"
#include "colormap-dialog.h"
#include "device-status-dialog.h"
#include "dialogs.h"
#include "dialogs-constructors.h"
#include "documents-commands.h"
#include "error-console-dialog.h"
#include "gradient-select.h"
#include "gradients-commands.h"
#include "layers-commands.h"
#include "menus.h"
#include "module-browser.h"
#include "palette-select.h"
#include "palettes-commands.h"
#include "paths-dialog.h"
#include "pattern-select.h"
#include "patterns-commands.h"
#include "preferences-dialog.h"
#include "tips-dialog.h"
#include "tool-options-dialog.h"
#include "toolbox.h"
#include "vectors-commands.h"

#include "gimprc.h"
#include "undo_history.h"

#ifdef DISPLAY_FILTERS
#include "gdisplay_color_ui.h"
#endif /* DISPLAY_FILTERS */

#include "libgimp/gimpintl.h"


/*  local function prototypes  */

static void dialogs_indexed_palette_selected      (GimpColormapDialog *dialog,
                                                   GimpDockable       *dockable);

static GtkWidget * dialogs_brush_tab_func         (GimpDockable       *dockable,
                                                   GimpDockbook       *dockbook,
                                                   gint                size);
static GtkWidget * dialogs_pattern_tab_func       (GimpDockable       *dockable,
                                                   GimpDockbook       *dockbook,
                                                   gint                size);
static GtkWidget * dialogs_gradient_tab_func      (GimpDockable       *dockable,
                                                   GimpDockbook       *dockbook,
                                                   gint                size);
static GtkWidget * dialogs_palette_tab_func       (GimpDockable       *dockable,
                                                   GimpDockbook       *dockbook,
                                                   gint                size);
static GtkWidget * dialogs_tool_tab_func          (GimpDockable       *dockable,
                                                   GimpDockbook       *dockbook,
                                                   gint                size);
static GtkWidget * dialogs_tool_options_tab_func  (GimpDockable       *dockable,
                                                   GimpDockbook       *dockbook,
                                                   gint                size);

static void   dialogs_set_view_context_func       (GimpDockable       *dockable,
                                                   GimpContext        *context);
static void   dialogs_set_editor_context_func     (GimpDockable       *dockable,
                                                   GimpContext        *context);
static void   dialogs_set_image_item_context_func (GimpDockable       *dockable,
                                                   GimpContext        *context);
static void   dialogs_set_path_context_func       (GimpDockable       *dockable,
                                                   GimpContext        *context);
static void   dialogs_set_indexed_palette_context_func (GimpDockable *dockable,
							GimpContext  *context);

static GtkWidget * dialogs_dockable_new (GtkWidget                  *widget,
					 const gchar                *name,
					 const gchar                *short_name,
					 GimpDockableGetTabFunc      get_tab_func,
					 GimpDockableSetContextFunc  set_context_func);

static void dialogs_image_item_view_image_changed (GimpContext        *context,
                                                   GimpImage          *gimage,
                                                   GimpItemListView   *view);
static void dialogs_path_view_image_changed       (GimpContext        *context,
                                                   GimpImage          *gimage,
                                                   GtkWidget          *view);
static void dialogs_indexed_palette_image_changed (GimpContext        *context,
						   GimpImage          *gimage,
						   GimpColormapDialog *ipal);


/**********************/
/*  toplevel dialogs  */
/**********************/

GtkWidget *
dialogs_toolbox_get (GimpDialogFactory *factory,
		     GimpContext       *context,
                     gint               preview_size)
{
  return toolbox_create (context->gimp);
}

GtkWidget *
dialogs_device_status_get (GimpDialogFactory *factory,
			   GimpContext       *context,
                           gint               preview_size)
{
  return device_status_dialog_create (context->gimp);
}

GtkWidget *
dialogs_brush_select_get (GimpDialogFactory *factory,
			  GimpContext       *context,
                          gint               preview_size)
{
  return brush_dialog_create (context->gimp);
}

GtkWidget *
dialogs_pattern_select_get (GimpDialogFactory *factory,
			    GimpContext       *context,
                            gint               preview_size)
{
  return pattern_dialog_create (context->gimp);
}

GtkWidget *
dialogs_gradient_select_get (GimpDialogFactory *factory,
			     GimpContext       *context,
                             gint               preview_size)
{
  return gradient_dialog_create (context->gimp);
}

GtkWidget *
dialogs_palette_select_get (GimpDialogFactory *factory,
			    GimpContext       *context,
                            gint               preview_size)
{
  return palette_dialog_create (context->gimp);
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
dialogs_undo_history_get (GimpDialogFactory *factory,
			  GimpContext       *context,
                          gint               preview_size)
{
  GimpImage *gimage;
  GtkWidget *undo_history;

  gimage = gimp_context_get_image (context);

  if (! gimage)
    return NULL;

  undo_history = g_object_get_data (G_OBJECT (gimage), "gimp-undo-history");

  if (! undo_history)
    {
      undo_history = undo_history_new (gimage);

      g_object_set_data (G_OBJECT (gimage), "gimp-undo-history", undo_history);
    }

  return undo_history;
}

GtkWidget *
dialogs_display_filters_get (GimpDialogFactory *factory,
			     GimpContext       *context,
                             gint               preview_size)
{
#ifdef DISPLAY_FILTERS
  GimpDisplay *gdisp;

  gdisp = gimp_context_get_display (context);

  if (! gdisp)
    gdisp = color_area_gdisp;

  if (! gdisp->cd_ui)
    gdisplay_color_ui_new (gdisp);

  return gdisp->cd_ui;
#else
  return NULL;
#endif /* DISPLAY_FILTERS */
}

GtkWidget *
dialogs_tips_get (GimpDialogFactory *factory,
		  GimpContext       *context,
                  gint               preview_size)
{
  return tips_dialog_create ();
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
dialogs_dock_new (GimpDialogFactory *factory,
		  GimpContext       *context,
                  gint               preview_size)
{
  return gimp_image_dock_new (factory, context->gimp->images);
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
                               "Tool Options", "Tool Options",
                               dialogs_tool_options_tab_func,
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

  view = error_console_create (context->gimp);

  g_object_add_weak_pointer (G_OBJECT (view), (gpointer *) &view);

  return dialogs_dockable_new (view,
                               "Error Console", "Errors",
                               NULL,
                               NULL);
}


/*****  list views  *****/

GtkWidget *
dialogs_image_list_view_new (GimpDialogFactory *factory,
			     GimpContext       *context,
                             gint               preview_size)
{
  GtkWidget *view;

  view = gimp_container_list_view_new (context->gimp->images,
				       context,
				       preview_size,
                                       FALSE,
				       5, 3);

  return dialogs_dockable_new (view,
			       "Image List", "Images",
			       NULL,
			       dialogs_set_view_context_func);
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
				      preview_size,
				      5, 3,
				      gimp_item_factory_from_path ("<Brushes>"));

  return dialogs_dockable_new (view,
			       "Brush List", "Brushes",
			       dialogs_brush_tab_func,
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
				     preview_size,
				     5, 3,
				     gimp_item_factory_from_path ("<Patterns>"));

  return dialogs_dockable_new (view,
			       "Pattern List", "Patterns",
			       dialogs_pattern_tab_func,
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
				     preview_size,
				     5, 3,
				     gimp_item_factory_from_path ("<Gradients>"));

  return dialogs_dockable_new (view,
			       "Gradient List", "Gradients",
			       dialogs_gradient_tab_func,
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
				     preview_size,
				     5, 3,
				     gimp_item_factory_from_path ("<Palettes>"));

  return dialogs_dockable_new (view,
			       "Palette List", "Palettes",
			       dialogs_palette_tab_func,
			       dialogs_set_editor_context_func);
}

GtkWidget *
dialogs_tool_list_view_new (GimpDialogFactory *factory,
			    GimpContext       *context,
                            gint               preview_size)
{
  GtkWidget *view;

  view = gimp_container_list_view_new (context->gimp->tool_info_list,
				       context,
				       preview_size,
                                       FALSE,
				       5, 3);

  return dialogs_dockable_new (view,
			       "Tool List", "Tools",
			       dialogs_tool_tab_func,
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
			       preview_size,
			       5, 3,
			       gimp_item_factory_from_path ("<Buffers>"));

  return dialogs_dockable_new (view,
			       "Buffer List", "Buffers",
			       NULL,
			       dialogs_set_editor_context_func);
}


/*****  grid views  *****/

GtkWidget *
dialogs_image_grid_view_new (GimpDialogFactory *factory,
			     GimpContext       *context,
                             gint               preview_size)
{
  GtkWidget *view;

  view = gimp_container_grid_view_new (context->gimp->images,
				       context,
				       preview_size,
                                       FALSE,
				       5, 3);

  return dialogs_dockable_new (view,
			       "Image Grid", "Images",
			       NULL,
			       dialogs_set_view_context_func);
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
				      preview_size,
				      5, 3,
				      gimp_item_factory_from_path ("<Brushes>"));

  return dialogs_dockable_new (view,
			       "Brush Grid", "Brushes",
			       dialogs_brush_tab_func,
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
				     preview_size,
				     5, 3,
				     gimp_item_factory_from_path ("<Patterns>"));

  return dialogs_dockable_new (view,
			       "Pattern Grid", "Patterns",
			       dialogs_pattern_tab_func,
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
				     preview_size,
				     5, 3,
				     gimp_item_factory_from_path ("<Gradients>"));

  return dialogs_dockable_new (view,
			       "Gradient Grid", "Gradients",
			       dialogs_gradient_tab_func,
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
				     preview_size,
				     5, 3,
				     gimp_item_factory_from_path ("<Gradients>"));

  return dialogs_dockable_new (view,
			       "Palette Grid", "Palettes",
			       dialogs_palette_tab_func,
			       dialogs_set_editor_context_func);
}

GtkWidget *
dialogs_tool_grid_view_new (GimpDialogFactory *factory,
			    GimpContext       *context,
                            gint               preview_size)
{
  GtkWidget *view;

  view = gimp_container_grid_view_new (context->gimp->tool_info_list,
				       context,
				       preview_size,
                                       FALSE,
				       5, 3);

  return dialogs_dockable_new (view,
			       "Tool Grid", "Tools",
			       dialogs_tool_tab_func,
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
			       preview_size,
			       5, 3,
			       gimp_item_factory_from_path ("<Buffers>"));

  return dialogs_dockable_new (view,
			       "Buffer Grid", "Buffers",
			       NULL,
			       dialogs_set_editor_context_func);
}


/*****  image related dialogs  *****/

GtkWidget *
dialogs_layer_list_view_new (GimpDialogFactory *factory,
			     GimpContext       *context,
                             gint               preview_size)
{
  GimpImage         *gimage;
  GimpLayerListView *layer_view;
  GtkWidget         *view;
  GtkWidget         *dockable;

  gimage = gimp_context_get_image (context);

  view =
    gimp_item_list_view_new (preview_size,
                             gimage,
                             GIMP_TYPE_LAYER,
                             "active_layer_changed",
                             (GimpGetContainerFunc) gimp_image_get_layers,
                             (GimpGetItemFunc)      gimp_image_get_active_layer,
                             (GimpSetItemFunc)      gimp_image_set_active_layer,
                             (GimpReorderItemFunc)  gimp_image_position_layer,
                             (GimpAddItemFunc)      gimp_image_add_layer,
                             (GimpRemoveItemFunc)   layers_remove_layer,
                             (GimpCopyItemFunc)     gimp_layer_copy,
                             (GimpConvertItemFunc)  gimp_layer_new_from_drawable,
                             (GimpNewItemFunc)      layers_new_layer_query,
                             (GimpEditItemFunc)     layers_edit_layer_query,
                             (GimpActivateItemFunc) layers_edit_layer_query,
                             gimp_item_factory_from_path ("<Layers>"));

  layer_view = GIMP_LAYER_LIST_VIEW (view);

  layer_view->anchor_item_func = layers_anchor_layer;

  dockable = dialogs_dockable_new (view,
				   "Layer List", "Layers",
				   NULL,
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

  view =
    gimp_item_list_view_new (preview_size,
                             gimage,
                             GIMP_TYPE_CHANNEL,
                             "active_channel_changed",
                             (GimpGetContainerFunc) gimp_image_get_channels,
                             (GimpGetItemFunc)      gimp_image_get_active_channel,
                             (GimpSetItemFunc)      gimp_image_set_active_channel,
                             (GimpReorderItemFunc)  gimp_image_position_channel,
                             (GimpAddItemFunc)      gimp_image_add_channel,
                             (GimpRemoveItemFunc)   gimp_image_remove_channel,
                             (GimpCopyItemFunc)     gimp_channel_copy,
                             (GimpConvertItemFunc)  NULL,
                             (GimpNewItemFunc)      channels_new_channel_query,
                             (GimpEditItemFunc)     channels_edit_channel_query,
                             (GimpActivateItemFunc) channels_edit_channel_query,
                             gimp_item_factory_from_path ("<Channels>"));

  dockable = dialogs_dockable_new (view,
				   "Channel List", "Channels",
				   NULL,
				   dialogs_set_image_item_context_func);

  gimp_dockable_set_context (GIMP_DOCKABLE (dockable), context);

  return dockable;
}

static GimpVectors *
gimp_vectors_copy (const GimpVectors *vectors,
                   GType              new_type,
                   gboolean           add_alpha /* unused */)
{
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);
  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_VECTORS), NULL);

  return NULL;
}

GtkWidget *
dialogs_vectors_list_view_new (GimpDialogFactory *factory,
			       GimpContext       *context,
                               gint               preview_size)
{
  GimpImage           *gimage;
  GimpVectorsListView *vectors_view;
  GtkWidget           *view;
  GtkWidget           *dockable;

  gimage = gimp_context_get_image (context);

  view =
    gimp_item_list_view_new (preview_size,
                             gimage,
                             GIMP_TYPE_VECTORS,
                             "active_vectors_changed",
                             (GimpGetContainerFunc) gimp_image_get_vectors,
                             (GimpGetItemFunc)      gimp_image_get_active_vectors,
                             (GimpSetItemFunc)      gimp_image_set_active_vectors,
                             (GimpReorderItemFunc)  gimp_image_position_vectors,
                             (GimpAddItemFunc)      gimp_image_add_vectors,
                             (GimpRemoveItemFunc)   gimp_image_remove_vectors,
                             (GimpCopyItemFunc)     gimp_vectors_copy,
                             (GimpConvertItemFunc)  NULL,
                             (GimpNewItemFunc)      vectors_new_vectors_query,
                             (GimpEditItemFunc)     vectors_edit_vectors_query,
                             (GimpActivateItemFunc) vectors_vectors_tool,
                             gimp_item_factory_from_path ("<Vectors>"));

  vectors_view = GIMP_VECTORS_LIST_VIEW (view);

  vectors_view->stroke_item_func = vectors_stroke_vectors;

  dockable = dialogs_dockable_new (view,
				   "Paths List", "Paths",
				   NULL,
				   dialogs_set_image_item_context_func);

  gimp_dockable_set_context (GIMP_DOCKABLE (dockable), context);

  return dockable;
}

GtkWidget *
dialogs_path_list_view_new (GimpDialogFactory *factory,
			    GimpContext       *context,
                            gint               preview_size)
{
  static GtkWidget *view = NULL;

  GtkWidget *dockable;

  if (view)
    return NULL;

  view = paths_dialog_create ();

  g_object_add_weak_pointer (G_OBJECT (view), (gpointer *) &view);

  dockable = dialogs_dockable_new (view,
				   "Old Path List", "Old Paths",
				   NULL,
				   dialogs_set_path_context_func);

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

  view = gimp_colormap_dialog_new (gimage);

  dockable = dialogs_dockable_new (view,
				   "Indexed Palette", "Colormap",
				   NULL,
				   dialogs_set_indexed_palette_context_func);

  dialogs_set_indexed_palette_context_func (GIMP_DOCKABLE (dockable), context);

  g_signal_connect (G_OBJECT (view), "selected",
		    G_CALLBACK (dialogs_indexed_palette_selected),
		    dockable);

  return dockable;
}


/*****  misc dockables  *****/

GtkWidget *
dialogs_document_history_new (GimpDialogFactory *factory,
			      GimpContext       *context,
                              gint               preview_size)
{
  GtkWidget *view;

  view = gimp_document_view_new (GIMP_VIEW_TYPE_LIST,
                                 context->gimp->documents,
                                 context,
                                 preview_size,
                                 5, 3,
                                 gimp_item_factory_from_path ("<Documents>"));

  return dialogs_dockable_new (view,
			       "Document History", "History",
			       NULL,
			       dialogs_set_editor_context_func);
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
                               "Brush Editor", "Brush Editor",
                               NULL, NULL);
}

void
dialogs_edit_brush_func (GimpData *data)
{
  gimp_dialog_factory_dialog_raise (global_dock_factory,
                                    "gimp:brush-editor",
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
  gradient_editor = gimp_gradient_editor_new (context->gimp);

  return dialogs_dockable_new (GTK_WIDGET (gradient_editor),
			       "Gradient Editor", "Gradient Editor",
			       NULL, NULL);
}

void
dialogs_edit_gradient_func (GimpData *data)
{
  gimp_dialog_factory_dialog_raise (global_dock_factory,
				    "gimp:gradient-editor",
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
  palette_editor = gimp_palette_editor_new (context->gimp);

  return dialogs_dockable_new (GTK_WIDGET (palette_editor),
			       "Palette Editor", "Palette Editor",
			       NULL, NULL);
}

void
dialogs_edit_palette_func (GimpData *data)
{
  gimp_dialog_factory_dialog_raise (global_dock_factory,
				    "gimp:palette-editor",
                                    -1);

  gimp_data_editor_set_data (palette_editor, data);
}


/*  private functions  */

static void
dialogs_indexed_palette_selected (GimpColormapDialog *dialog,
				  GimpDockable       *dockable)
{
  GimpContext *context;

  context = (GimpContext *) g_object_get_data (G_OBJECT (dialog),
					       "gimp-dialogs-context");

  if (context)
    {
      GimpImage   *gimage;
      GimpRGB      color;
      gint         index;

      gimage = gimp_colormap_dialog_get_image (dialog);
      index  = gimp_colormap_dialog_col_index (dialog);

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
dialogs_brush_tab_func (GimpDockable *dockable,
			GimpDockbook *dockbook,
			gint          size)
{
  GimpContext *context;
  GtkWidget   *preview;

  context = dockbook->dock->context;

  preview =
    gimp_preview_new_full (GIMP_VIEWABLE (gimp_context_get_brush (context)),
			   size, size, 1,
			   FALSE, FALSE, FALSE);

  g_signal_connect_object (G_OBJECT (context), "brush_changed",
			   G_CALLBACK (gimp_preview_set_viewable),
			   G_OBJECT (preview),
			   G_CONNECT_SWAPPED);

  return preview;
}

static GtkWidget *
dialogs_pattern_tab_func (GimpDockable *dockable,
			  GimpDockbook *dockbook,
			  gint          size)
{
  GimpContext *context;
  GtkWidget   *preview;

  context = dockbook->dock->context;

  preview =
    gimp_preview_new_full (GIMP_VIEWABLE (gimp_context_get_pattern (context)),
			   size, size, 1,
			   FALSE, FALSE, FALSE);

  g_signal_connect_object (G_OBJECT (context), "pattern_changed",
			   G_CALLBACK (gimp_preview_set_viewable),
			   G_OBJECT (preview),
			   G_CONNECT_SWAPPED);

  return preview;
}

static GtkWidget *
dialogs_gradient_tab_func (GimpDockable *dockable,
			   GimpDockbook *dockbook,
			   gint          size)
{
  GimpContext *context;
  GtkWidget   *preview;

  context = dockbook->dock->context;

  preview =
    gimp_preview_new_full (GIMP_VIEWABLE (gimp_context_get_gradient (context)),
			   size, size, 1,
			   FALSE, FALSE, FALSE);

  g_signal_connect_object (G_OBJECT (context), "gradient_changed",
			   G_CALLBACK (gimp_preview_set_viewable),
			   G_OBJECT (preview),
			   G_CONNECT_SWAPPED);

  return preview;
}

static GtkWidget *
dialogs_palette_tab_func (GimpDockable *dockable,
			  GimpDockbook *dockbook,
			  gint          size)
{
  GimpContext *context;
  GtkWidget   *preview;

  context = dockbook->dock->context;

  preview =
    gimp_preview_new_full (GIMP_VIEWABLE (gimp_context_get_palette (context)),
			   size, size, 1,
			   FALSE, FALSE, FALSE);

  g_signal_connect_object (G_OBJECT (context), "palette_changed",
			   G_CALLBACK (gimp_preview_set_viewable),
			   G_OBJECT (preview),
			   G_CONNECT_SWAPPED);

  return preview;
}

static GtkWidget *
dialogs_tool_tab_func (GimpDockable *dockable,
		       GimpDockbook *dockbook,
		       gint          size)
{
  GimpContext *context;
  GtkWidget   *preview;

  context = dockbook->dock->context;

  preview =
    gimp_preview_new_full (GIMP_VIEWABLE (gimp_context_get_tool (context)),
			   MIN (size, 22), MIN (size, 22), 1,
			   FALSE, FALSE, FALSE);

  g_signal_connect_object (G_OBJECT (context), "tool_changed",
			   G_CALLBACK (gimp_preview_set_viewable),
			   G_OBJECT (preview),
			   G_CONNECT_SWAPPED);

  return preview;
}

static void
dialogs_tool_options_tool_changed (GimpContext  *context,
                                   GimpToolInfo *tool_info,
                                   GtkLabel     *label)
{
  gchar *text;

  text = g_strdup_printf (_("%s Options"), tool_info->blurb);

  gtk_label_set_text (label, text);

  g_free (text);

  gimp_help_set_help_data (GTK_WIDGET (label)->parent->parent,
                           tool_info->help,
                           tool_info->help_data);
}

static GtkWidget *
dialogs_tool_options_tab_func (GimpDockable *dockable,
                               GimpDockbook *dockbook,
                               gint          size)
{
  GimpContext  *context;
  GimpToolInfo *tool_info;
  GtkWidget    *hbox;
  GtkWidget    *preview;
  GtkWidget    *label;
  gchar        *text;

  context = dockbook->dock->context;

  tool_info = gimp_context_get_tool (context);

  hbox = gtk_hbox_new (FALSE, 2);

  preview = gimp_preview_new_full (GIMP_VIEWABLE (tool_info),
                                   MIN (size, 22), MIN (size, 22), 0,
                                   FALSE, FALSE, FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), preview, FALSE, FALSE, 0);
  gtk_widget_show (preview);

  g_signal_connect_object (G_OBJECT (context), "tool_changed",
			   G_CALLBACK (gimp_preview_set_viewable),
			   G_OBJECT (preview),
			   G_CONNECT_SWAPPED);

  text = g_strdup_printf (_("%s Options"), tool_info->blurb);

  label = gtk_label_new (text);

  g_free (text);

  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  g_signal_connect_object (G_OBJECT (context), "tool_changed",
			   G_CALLBACK (dialogs_tool_options_tool_changed),
			   G_OBJECT (label),
			   0);

  return hbox;
}

static void
dialogs_set_view_context_func (GimpDockable *dockable,
			       GimpContext  *context)
{
  GimpContainerView *view;

  view = (GimpContainerView *) g_object_get_data (G_OBJECT (dockable),
						  "gimp-dialogs-view");

  if (view)
    gimp_container_view_set_context (view, context);
}

static void
dialogs_set_editor_context_func (GimpDockable *dockable,
				 GimpContext  *context)
{
  GimpContainerEditor *editor;

  editor = (GimpContainerEditor *) g_object_get_data (G_OBJECT (dockable),
						      "gimp-dialogs-view");

  if (editor)
    gimp_container_view_set_context (editor->view, context);
}

static void
dialogs_set_image_item_context_func (GimpDockable *dockable,
                                     GimpContext  *context)
{
  GimpItemListView *view;

  view = (GimpItemListView *) g_object_get_data (G_OBJECT (dockable),
                                                 "gimp-dialogs-view");

  if (view)
    {
      if (dockable->context)
	{
	  g_signal_handlers_disconnect_by_func (G_OBJECT (dockable->context),
						dialogs_image_item_view_image_changed,
						view);
	}

      if (context)
	{
	  g_signal_connect (G_OBJECT (context), "image_changed",
			    G_CALLBACK (dialogs_image_item_view_image_changed),
			    view);

	  dialogs_image_item_view_image_changed (context,
                                                 gimp_context_get_image (context),
                                                 view);
	}
      else
	{
	  dialogs_image_item_view_image_changed (NULL, NULL, view);
	}
    }
}

static void
dialogs_set_path_context_func (GimpDockable *dockable,
			       GimpContext  *context)
{
  GtkWidget *view;

  view = (GtkWidget *) g_object_get_data (G_OBJECT (dockable),
					  "gimp-dialogs-view");

  if (view)
    {
      if (dockable->context)
	{
	  g_signal_handlers_disconnect_by_func (G_OBJECT (dockable->context),
						dialogs_path_view_image_changed,
						view);
	}

      if (context)
	{
	  g_signal_connect (G_OBJECT (context), "image_changed",
			    G_CALLBACK (dialogs_path_view_image_changed),
			    view);

	  dialogs_path_view_image_changed (context,
					   gimp_context_get_image (context),
					   view);
	}
      else
	{
	  dialogs_path_view_image_changed (NULL, NULL, view);
	}
    }
}

static void
dialogs_set_indexed_palette_context_func (GimpDockable *dockable,
					  GimpContext  *context)
{
  GimpColormapDialog *view;

  view = (GimpColormapDialog *) g_object_get_data (G_OBJECT (dockable),
						   "gimp-dialogs-view");

  if (view)
    {
      if (dockable->context)
	{
	  g_signal_handlers_disconnect_by_func (G_OBJECT (dockable->context),
						dialogs_indexed_palette_image_changed,
						view);
	}

      if (context)
	{
	  g_signal_connect (G_OBJECT (context), "image_changed",
			    G_CALLBACK (dialogs_indexed_palette_image_changed),
			    view);

	  dialogs_indexed_palette_image_changed (context,
						 gimp_context_get_image (context),
						 view);
	}
      else
	{
	  dialogs_indexed_palette_image_changed (NULL, NULL, view);
	}
    }
}

static GtkWidget *
dialogs_dockable_new (GtkWidget                  *widget,
		      const gchar                *name,
		      const gchar                *short_name,
		      GimpDockableGetTabFunc      get_tab_func,
		      GimpDockableSetContextFunc  set_context_func)
{
  GtkWidget *dockable;

  dockable = gimp_dockable_new (name,
				short_name,
				get_tab_func,
				set_context_func);
  gtk_container_add (GTK_CONTAINER (dockable), widget);
  gtk_widget_show (widget);

  g_object_set_data (G_OBJECT (dockable), "gimp-dialogs-view", widget);

  return dockable;
}

static void
dialogs_image_item_view_image_changed (GimpContext      *context,
                                       GimpImage        *gimage,
                                       GimpItemListView *view)
{
  gimp_item_list_view_set_image (view, gimage);
}

static void
dialogs_path_view_image_changed (GimpContext *context,
				 GimpImage   *gimage,
				 GtkWidget   *widget)
{
  paths_dialog_update (gimage);
}

static void
dialogs_indexed_palette_image_changed (GimpContext        *context,
				       GimpImage          *gimage,
				       GimpColormapDialog *ipal)
{
  gimp_colormap_dialog_set_image (ipal, gimage);
}
