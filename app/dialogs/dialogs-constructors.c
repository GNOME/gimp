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

#include "apptypes.h"
#include "widgets/widgets-types.h"

#include "core/gimpbrushgenerated.h"
#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpgradient.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"

#include "widgets/gimpcontainerlistview.h"
#include "widgets/gimpcontainergridview.h"
#include "widgets/gimpdatafactoryview.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpimagedock.h"
#include "widgets/gimpdockable.h"
#include "widgets/gimpdockbook.h"
#include "widgets/gimpdrawablelistview.h"
#include "widgets/gimplistitem.h"
#include "widgets/gimppreview.h"

#include "tools/gimptoolinfo.h"
#include "tools/tool_manager.h"
#include "tools/tool_options_dialog.h"

#include "about-dialog.h"
#include "brush-editor.h"
#include "brush-select.h"
#include "channels-commands.h"
#include "color-area.h"
#include "colormap-dialog.h"
#include "devices.h"
#include "dialogs-constructors.h"
#include "docindex.h"
#include "errorconsole.h"
#include "gdisplay.h"
#include "gradient-editor.h"
#include "gradient-select.h"
#include "layers-commands.h"
#include "lc_dialog.h"
#include "palette-editor.h"
#include "pattern-select.h"
#include "preferences-dialog.h"
#include "tips-dialog.h"
#include "toolbox.h"

#include "context_manager.h"
#include "gdisplay.h"
#include "gimprc.h"
#include "module_db.h"
#include "undo_history.h"

#ifdef DISPLAY_FILTERS
#include "gdisplay_color_ui.h"
#endif /* DISPLAY_FILTERS */

#include "libgimp/gimpintl.h"


static void dialogs_indexed_palette_selected     (GimpColormapDialog *dialog,
						  GimpContext        *context);

static GtkWidget * dialogs_brush_tab_func        (GimpDockable       *dockable,
						  GimpDockbook       *dockbook,
						  gint                size);
static GtkWidget * dialogs_pattern_tab_func      (GimpDockable       *dockable,
						  GimpDockbook       *dockbook,
						  gint                size);
static GtkWidget * dialogs_gradient_tab_func     (GimpDockable       *dockable,
						  GimpDockbook       *dockbook,
						  gint                size);
static GtkWidget * dialogs_palette_tab_func      (GimpDockable       *dockable,
						  GimpDockbook       *dockbook,
						  gint                size);
static GtkWidget * dialogs_tool_tab_func         (GimpDockable       *dockable,
						  GimpDockbook       *dockbook,
						  gint                size);

static void   dialogs_set_view_context_func      (GimpDockable       *dockable,
						  GimpContext        *context);
static void   dialogs_set_data_context_func      (GimpDockable       *dockable,
						  GimpContext        *context);
static void   dialogs_set_drawable_context_func  (GimpDockable       *dockable,
						  GimpContext        *context);

static GtkWidget * dialogs_dockable_new (GtkWidget                  *widget,
					 const gchar                *name,
					 const gchar                *short_name,
					 GimpDockableGetTabFunc      get_tab_func,
					 GimpDockableSetContextFunc  set_context_func);

static void dialogs_drawable_view_image_changed (GimpContext          *context,
						 GimpImage            *gimage,
						 GimpDrawableListView *view);

static gchar * dialogs_tool_name_func           (GtkWidget *widget);


/*  public functions  */

GtkWidget *
dialogs_toolbox_get (GimpDialogFactory *factory,
		     GimpContext       *context)
{
  return toolbox_create ();
}

GtkWidget *
dialogs_lc_get (GimpDialogFactory *factory,
		GimpContext       *context)
{
  GDisplay *gdisp;

  gdisp = gimp_context_get_display (context);

  return lc_dialog_create (gdisp ? gdisp->gimage : NULL);
}

GtkWidget *
dialogs_tool_options_get (GimpDialogFactory *factory,
			  GimpContext       *context)
{
  return tool_options_dialog_create ();
}

GtkWidget *
dialogs_device_status_get (GimpDialogFactory *factory,
			   GimpContext       *context)
{
  return device_status_create ();
}

GtkWidget *
dialogs_brush_select_get (GimpDialogFactory *factory,
			  GimpContext       *context)
{
  return brush_dialog_create ();
}

GtkWidget *
dialogs_pattern_select_get (GimpDialogFactory *factory,
			    GimpContext       *context)
{
  return pattern_dialog_create ();
}

GtkWidget *
dialogs_gradient_select_get (GimpDialogFactory *factory,
			     GimpContext       *context)
{
  return gradient_dialog_create ();
}

GtkWidget *
dialogs_palette_get (GimpDialogFactory *factory,
		     GimpContext       *context)
{
  return palette_dialog_create ();
}

GtkWidget *
dialogs_error_console_get (GimpDialogFactory *factory,
			   GimpContext       *context)
{
  return error_console_create ();
}

GtkWidget *
dialogs_document_index_get (GimpDialogFactory *factory,
			    GimpContext       *context)
{
  return document_index_create ();
}

GtkWidget *
dialogs_preferences_get (GimpDialogFactory *factory,
			 GimpContext       *context)
{
  return preferences_dialog_create ();
}

GtkWidget *
dialogs_input_devices_get (GimpDialogFactory *factory,
			   GimpContext       *context)
{
  return input_dialog_create ();
}

GtkWidget *
dialogs_module_browser_get (GimpDialogFactory *factory,
			    GimpContext       *context)
{
  return module_db_browser_new ();
}

GtkWidget *
dialogs_indexed_palette_get (GimpDialogFactory *factory,
			     GimpContext       *context)
{
  GimpColormapDialog *cmap_dlg;

  cmap_dlg = gimp_colormap_dialog_create (image_context);

  gtk_signal_connect (GTK_OBJECT (cmap_dlg), "selected",
		      GTK_SIGNAL_FUNC (dialogs_indexed_palette_selected),
		      factory->context);

  return GTK_WIDGET (cmap_dlg);
}

GtkWidget *
dialogs_undo_history_get (GimpDialogFactory *factory,
			  GimpContext       *context)
{
  GDisplay  *gdisp;
  GimpImage *gimage;

  gdisp = gimp_context_get_display (context);

  if (! gdisp)
    return NULL;

  gimage = gdisp->gimage;

  if (! gimage->undo_history)
    gimage->undo_history = undo_history_new (gimage);

  return gimage->undo_history;
}

GtkWidget *
dialogs_display_filters_get (GimpDialogFactory *factory,
			     GimpContext       *context)
{
#ifdef DISPLAY_FILTERS
  GDisplay *gdisp;

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
		  GimpContext       *context)
{
  return tips_dialog_create ();
}

GtkWidget *
dialogs_about_get (GimpDialogFactory *factory,
		   GimpContext       *context)
{
  return about_dialog_create ();
}


GtkWidget *
dialogs_dock_new (GimpDialogFactory *factory,
		  GimpContext       *context)
{
  return gimp_image_dock_new (factory, image_context);
}


/*  list views  */

GtkWidget *
dialogs_image_list_view_new (GimpDialogFactory *factory,
			     GimpContext       *context)
{
  GtkWidget *view;

  view = gimp_container_list_view_new (image_context,
				       context,
				       32,
				       5, 3);

  return dialogs_dockable_new (view,
			       "Image List", "Images",
			       NULL,
			       dialogs_set_view_context_func);
}

GtkWidget *
dialogs_brush_list_view_new (GimpDialogFactory *factory,
			     GimpContext       *context)
{
  GtkWidget *view;

  view = gimp_data_factory_view_new (GIMP_VIEW_TYPE_LIST,
				     global_brush_factory,
				     dialogs_edit_brush_func,
				     context,
				     32,
				     5, 3);

  return dialogs_dockable_new (view,
			       "Brush List", "Brushes",
			       dialogs_brush_tab_func,
			       dialogs_set_data_context_func);
}

GtkWidget *
dialogs_pattern_list_view_new (GimpDialogFactory *factory,
			       GimpContext       *context)
{
  GtkWidget *view;

  view = gimp_data_factory_view_new (GIMP_VIEW_TYPE_LIST,
				     global_pattern_factory,
				     NULL,
				     context,
				     32,
				     5, 3);

  return dialogs_dockable_new (view,
			       "Pattern List", "Patterns",
			       dialogs_pattern_tab_func,
			       dialogs_set_data_context_func);
}

GtkWidget *
dialogs_gradient_list_view_new (GimpDialogFactory *factory,
				GimpContext       *context)
{
  GtkWidget *view;

  view = gimp_data_factory_view_new (GIMP_VIEW_TYPE_LIST,
				     global_gradient_factory,
				     dialogs_edit_gradient_func,
				     context,
				     32 / 2,
				     5, 3);

  return dialogs_dockable_new (view,
			       "Gradient List", "Gradients",
			       dialogs_gradient_tab_func,
			       dialogs_set_data_context_func);
}

GtkWidget *
dialogs_palette_list_view_new (GimpDialogFactory *factory,
			       GimpContext       *context)
{
  GtkWidget *view;

  view = gimp_data_factory_view_new (GIMP_VIEW_TYPE_LIST,
				     global_palette_factory,
				     dialogs_edit_palette_func,
				     context,
				     32,
				     5, 3);

  return dialogs_dockable_new (view,
			       "Palette List", "Palettes",
			       dialogs_palette_tab_func,
			       dialogs_set_data_context_func);
}

GtkWidget *
dialogs_tool_list_view_new (GimpDialogFactory *factory,
			    GimpContext       *context)
{
  GtkWidget *view;

  view = gimp_container_list_view_new (NULL,
				       context,
				       22,
				       5, 3);

  gimp_container_view_set_name_func (GIMP_CONTAINER_VIEW (view),
				     dialogs_tool_name_func);
  gimp_container_view_set_container (GIMP_CONTAINER_VIEW (view),
				     global_tool_info_list);

  return dialogs_dockable_new (view,
			       "Tool List", "Tools",
			       dialogs_tool_tab_func,
			       dialogs_set_view_context_func);
}


/*  grid views  */

GtkWidget *
dialogs_image_grid_view_new (GimpDialogFactory *factory,
			     GimpContext       *context)
{
  GtkWidget *view;

  view = gimp_container_grid_view_new (image_context,
				       context,
				       32,
				       5, 3);

  return dialogs_dockable_new (view,
			       "Image Grid", "Images",
			       NULL,
			       dialogs_set_view_context_func);
}

GtkWidget *
dialogs_brush_grid_view_new (GimpDialogFactory *factory,
			     GimpContext       *context)
{
  GtkWidget *view;

  view = gimp_data_factory_view_new (GIMP_VIEW_TYPE_GRID,
				     global_brush_factory,
				     dialogs_edit_brush_func,
				     context,
				     32,
				     5, 3);

  return dialogs_dockable_new (view,
			       "Brush Grid", "Brushes",
			       dialogs_brush_tab_func,
			       dialogs_set_data_context_func);
}

GtkWidget *
dialogs_pattern_grid_view_new (GimpDialogFactory *factory,
			       GimpContext       *context)
{
  GtkWidget *view;

  view = gimp_data_factory_view_new (GIMP_VIEW_TYPE_GRID,
				     global_pattern_factory,
				     NULL,
				     context,
				     32,
				     5, 3);

  return dialogs_dockable_new (view,
			       "Pattern Grid", "Patterns",
			       dialogs_pattern_tab_func,
			       dialogs_set_data_context_func);
}

GtkWidget *
dialogs_gradient_grid_view_new (GimpDialogFactory *factory,
				GimpContext       *context)
{
  GtkWidget *view;

  view = gimp_data_factory_view_new (GIMP_VIEW_TYPE_GRID,
				     global_gradient_factory,
				     dialogs_edit_gradient_func,
				     context,
				     32,
				     5, 3);

  return dialogs_dockable_new (view,
			       "Gradient Grid", "Gradients",
			       dialogs_gradient_tab_func,
			       dialogs_set_data_context_func);
}

GtkWidget *
dialogs_palette_grid_view_new (GimpDialogFactory *factory,
			       GimpContext       *context)
{
  GtkWidget *view;

  view = gimp_data_factory_view_new (GIMP_VIEW_TYPE_GRID,
				     global_palette_factory,
				     dialogs_edit_palette_func,
				     context,
				     32,
				     5, 3);

  return dialogs_dockable_new (view,
			       "Palette Grid", "Palettes",
			       dialogs_palette_tab_func,
			       dialogs_set_data_context_func);
}

GtkWidget *
dialogs_tool_grid_view_new (GimpDialogFactory *factory,
			    GimpContext       *context)
{
  GtkWidget *view;

  view = gimp_container_grid_view_new (NULL,
				       context,
				       22,
				       5, 3);

  gimp_container_view_set_name_func (GIMP_CONTAINER_VIEW (view),
				     dialogs_tool_name_func);
  gimp_container_view_set_container (GIMP_CONTAINER_VIEW (view),
				     global_tool_info_list);

  return dialogs_dockable_new (view,
			       "Tool Grid", "Tools",
			       dialogs_tool_tab_func,
			       dialogs_set_view_context_func);
}


/*  image related dialogs  */

GtkWidget *
dialogs_layer_list_view_new (GimpDialogFactory *factory,
			     GimpContext       *context)
{
  GimpImage *gimage;
  GtkWidget *view;
  GtkWidget *dockable;

  gimage = gimp_context_get_image (context);

  view = gimp_drawable_list_view_new
    (gimage,
     GIMP_TYPE_LAYER,
     "active_layer_changed",
     (GimpGetContainerFunc)    gimp_image_get_layers,
     (GimpGetDrawableFunc)     gimp_image_get_active_layer,
     (GimpSetDrawableFunc)     gimp_image_set_active_layer,
     (GimpReorderDrawableFunc) gimp_image_position_layer,
     (GimpAddDrawableFunc)     gimp_image_add_layer,
     (GimpRemoveDrawableFunc)  gimp_image_remove_layer,
     (GimpCopyDrawableFunc)    gimp_layer_copy,
     (GimpNewDrawableFunc)     layers_new_layer_query,
     (GimpEditDrawableFunc)    layers_edit_layer_query,
     (GimpDrawableContextFunc) layers_show_context_menu);

  dockable = dialogs_dockable_new (view,
				   "Layer List", "Layers",
				   NULL,
				   dialogs_set_drawable_context_func);

  dialogs_set_drawable_context_func (GIMP_DOCKABLE (dockable), context);

  return dockable;
}

GtkWidget *
dialogs_channel_list_view_new (GimpDialogFactory *factory,
			       GimpContext       *context)
{
  GimpImage *gimage;
  GtkWidget *view;
  GtkWidget *dockable;

  gimage = gimp_context_get_image (context);

  view = gimp_drawable_list_view_new
    (gimage,
     GIMP_TYPE_CHANNEL,
     "active_channel_changed",
     (GimpGetContainerFunc)    gimp_image_get_channels,
     (GimpGetDrawableFunc)     gimp_image_get_active_channel,
     (GimpSetDrawableFunc)     gimp_image_set_active_channel,
     (GimpReorderDrawableFunc) gimp_image_position_channel,
     (GimpAddDrawableFunc)     gimp_image_add_channel,
     (GimpRemoveDrawableFunc)  gimp_image_remove_channel,
     (GimpCopyDrawableFunc)    gimp_channel_copy,
     (GimpNewDrawableFunc)     channels_new_channel_query,
     (GimpEditDrawableFunc)    channels_edit_channel_query,
     (GimpDrawableContextFunc) channels_show_context_menu);

  dockable = dialogs_dockable_new (view,
				   "Channel List", "Channels",
				   NULL,
				   dialogs_set_drawable_context_func);

  dialogs_set_drawable_context_func (GIMP_DOCKABLE (dockable), context);

  return dockable;
}


/*  editor dialogs  */

void
dialogs_edit_brush_func (GimpData *data)
{
  static BrushEditor *brush_editor_dialog = NULL;

  GimpBrush *brush;

  brush = GIMP_BRUSH (data);

  if (GIMP_IS_BRUSH_GENERATED (brush))
    {
      if (! brush_editor_dialog)
	{
	  brush_editor_dialog = brush_editor_new ();
	}

      brush_editor_set_brush (brush_editor_dialog, brush);
    }
  else
    {
      g_message (_("Sorry, this brush can't be edited."));
    }
}

void
dialogs_edit_gradient_func (GimpData *data)
{
  static GradientEditor *gradient_editor_dialog = NULL;

  GimpGradient *gradient;

  gradient = GIMP_GRADIENT (data);

  if (! gradient_editor_dialog)
    {
      gradient_editor_dialog = gradient_editor_new ();
    }

  gradient_editor_set_gradient (gradient_editor_dialog, gradient);
}

void
dialogs_edit_palette_func (GimpData *data)
{
  palette_dialog_edit_palette (data);
}


/*  private functions  */

static void
dialogs_indexed_palette_selected (GimpColormapDialog *dialog,
				  GimpContext        *context)
{
  GimpImage *gimage;
  GimpRGB    color;
  gint       index;

  gimage = gimp_colormap_dialog_image (dialog);
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

  gtk_signal_connect_object_while_alive
    (GTK_OBJECT (context),
     "brush_changed",
     GTK_SIGNAL_FUNC (gimp_preview_set_viewable),
     GTK_OBJECT (preview));

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

  gtk_signal_connect_object_while_alive
    (GTK_OBJECT (context),
     "pattern_changed",
     GTK_SIGNAL_FUNC (gimp_preview_set_viewable),
     GTK_OBJECT (preview));

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

  gtk_signal_connect_object_while_alive
    (GTK_OBJECT (context),
     "gradient_changed",
     GTK_SIGNAL_FUNC (gimp_preview_set_viewable),
     GTK_OBJECT (preview));

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

  gtk_signal_connect_object_while_alive
    (GTK_OBJECT (context),
     "palette_changed",
     GTK_SIGNAL_FUNC (gimp_preview_set_viewable),
     GTK_OBJECT (preview));

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
			   size, size, 1,
			   FALSE, FALSE, FALSE);

  gtk_signal_connect_object_while_alive
    (GTK_OBJECT (context),
     "tool_changed",
     GTK_SIGNAL_FUNC (gimp_preview_set_viewable),
     GTK_OBJECT (preview));

  return preview;
}

static void
dialogs_set_view_context_func (GimpDockable *dockable,
			       GimpContext  *context)
{
  GimpContainerView *view;

  view = (GimpContainerView *) gtk_object_get_data (GTK_OBJECT (dockable),
						    "gimp-dialogs-view");

  if (view)
    gimp_container_view_set_context (view, context);
}

static void
dialogs_set_data_context_func (GimpDockable *dockable,
			       GimpContext  *context)
{
  GimpDataFactoryView *view;

  view = (GimpDataFactoryView *) gtk_object_get_data (GTK_OBJECT (dockable),
						      "gimp-dialogs-view");

  if (view)
    {
      /* TODO: gimp_data_factory_view_set_context (view, context); */
    }
}

static void
dialogs_set_drawable_context_func (GimpDockable *dockable,
				   GimpContext  *context)
{
  GimpDrawableListView *view;

  view = (GimpDrawableListView *) gtk_object_get_data (GTK_OBJECT (dockable),
						       "gimp-dialogs-view");

  if (view)
    {
      GimpContext *old_context;

      old_context = (GimpContext *) gtk_object_get_data (GTK_OBJECT (view),
							 "gimp-dialogs-context");

      if (old_context)
	{
	  gtk_signal_disconnect_by_func (GTK_OBJECT (old_context),
					 GTK_SIGNAL_FUNC (dialogs_drawable_view_image_changed),
					 view);
	}

      if (context)
	{
	  gtk_signal_connect (GTK_OBJECT (context), "image_changed",
			      GTK_SIGNAL_FUNC (dialogs_drawable_view_image_changed),
			      view);

	  dialogs_drawable_view_image_changed (context,
					       gimp_context_get_image (context),
					       view);
	}
      else
	{
	  dialogs_drawable_view_image_changed (NULL, NULL, view);
	}

      gtk_object_set_data (GTK_OBJECT (view), "gimp-dialogs-context", context);
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

  gtk_object_set_data (GTK_OBJECT (dockable), "gimp-dialogs-view", widget);

  return dockable;
}

static void
dialogs_drawable_view_image_changed (GimpContext          *context,
				     GimpImage            *gimage,
				     GimpDrawableListView *view)
{
  gimp_drawable_list_view_set_image (view, gimage);
}

static gchar *
dialogs_tool_name_func (GtkWidget *widget)
{
  GimpToolInfo *tool_info = NULL;

  if (GIMP_IS_PREVIEW (widget))
    {
      tool_info = GIMP_TOOL_INFO (GIMP_PREVIEW (widget)->viewable);
    }
  else if (GIMP_IS_LIST_ITEM (widget))
    {
      tool_info = GIMP_TOOL_INFO (GIMP_PREVIEW (GIMP_LIST_ITEM (widget)->preview)->viewable);
    }

  if (tool_info)
    return g_strdup (tool_info->blurb);
  else
    return g_strdup ("EEK");
}
