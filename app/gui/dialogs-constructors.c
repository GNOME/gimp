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

#include "tools/tool_manager.h"
#include "tools/tool_options_dialog.h"

#include "widgets/gimpcontainerlistview.h"
#include "widgets/gimpcontainergridview.h"
#include "widgets/gimpdatafactoryview.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpimagedock.h"
#include "widgets/gimpdockable.h"
#include "widgets/gimpdockbook.h"
#include "widgets/gimpdrawablelistview.h"
#include "widgets/gimppreview.h"

#include "about-dialog.h"
#include "brush-editor.h"
#include "brush-select.h"
#include "color-area.h"
#include "colormap-dialog.h"
#include "devices.h"
#include "dialogs-constructors.h"
#include "docindex.h"
#include "errorconsole.h"
#include "gdisplay.h"
#include "gradient-editor.h"
#include "gradient-select.h"
#include "lc_dialog.h"
#include "palette-editor.h"
#include "pattern-select.h"
#include "preferences-dialog.h"
#include "tips-dialog.h"
#include "toolbox.h"

#include "context_manager.h"
#include "gdisplay.h"
#include "gimpbrush.h"
#include "gimpbrushgenerated.h"
#include "gimpchannel.h"
#include "gimpcontainer.h"
#include "gimpcontext.h"
#include "gimpdatafactory.h"
#include "gimpgradient.h"
#include "gimpimage.h"
#include "gimplayer.h"
#include "gimprc.h"
#include "module_db.h"
#include "undo_history.h"

#ifdef DISPLAY_FILTERS
#include "gdisplay_color_ui.h"
#endif /* DISPLAY_FILTERS */

#include "libgimp/gimpintl.h"


static void dialogs_indexed_palette_selected (GimpColormapDialog     *dialog,
					      GimpContext            *context);

static GtkWidget * dialogs_brush_tab_func    (GimpDockable           *dockable,
					      GimpDockbook           *dockbook,
					      gint                    size);
static GtkWidget * dialogs_pattern_tab_func  (GimpDockable           *dockable,
					      GimpDockbook           *dockbook,
					      gint                    size);
static GtkWidget * dialogs_gradient_tab_func (GimpDockable           *dockable,
					      GimpDockbook           *dockbook,
					      gint                    size);
static GtkWidget * dialogs_palette_tab_func  (GimpDockable           *dockable,
					      GimpDockbook           *dockbook,
					      gint                    size);

static GtkWidget * dialogs_dockable_new      (GtkWidget              *widget,
					      const gchar            *name,
					      const gchar            *short_name,
					      GimpDockableGetTabFunc  get_tab_func);

static void dialogs_drawable_view_image_changed (GimpContext          *context,
						 GimpImage            *gimage,
						 GimpDrawableListView *view);


/*  public functions  */

GtkWidget *
dialogs_toolbox_get (GimpDialogFactory *factory)
{
  return toolbox_create ();
}

GtkWidget *
dialogs_lc_get (GimpDialogFactory *factory)
{
  GDisplay *gdisp;

  gdisp = gimp_context_get_display (factory->context);

  return lc_dialog_create (gdisp ? gdisp->gimage : NULL);
}

GtkWidget *
dialogs_tool_options_get (GimpDialogFactory *factory)
{
  return tool_options_dialog_create ();
}

GtkWidget *
dialogs_device_status_get (GimpDialogFactory *factory)
{
  return device_status_create ();
}

GtkWidget *
dialogs_brush_select_get (GimpDialogFactory *factory)
{
  return brush_dialog_create ();
}

GtkWidget *
dialogs_pattern_select_get (GimpDialogFactory *factory)
{
  return pattern_dialog_create ();
}

GtkWidget *
dialogs_gradient_select_get (GimpDialogFactory *factory)
{
  return gradient_dialog_create ();
}

GtkWidget *
dialogs_palette_get (GimpDialogFactory *factory)
{
  return palette_dialog_create ();
}

GtkWidget *
dialogs_error_console_get (GimpDialogFactory *factory)
{
  return error_console_create ();
}

GtkWidget *
dialogs_document_index_get (GimpDialogFactory *factory)
{
  return document_index_create ();
}

GtkWidget *
dialogs_preferences_get (GimpDialogFactory *factory)
{
  return preferences_dialog_create ();
}

GtkWidget *
dialogs_input_devices_get (GimpDialogFactory *factory)
{
  return input_dialog_create ();
}

GtkWidget *
dialogs_module_browser_get (GimpDialogFactory *factory)
{
  return module_db_browser_new ();
}

GtkWidget *
dialogs_indexed_palette_get (GimpDialogFactory *factory)
{
  GimpColormapDialog *cmap_dlg;

  cmap_dlg = gimp_colormap_dialog_create (image_context);

  gtk_signal_connect (GTK_OBJECT (cmap_dlg), "selected",
		      GTK_SIGNAL_FUNC (dialogs_indexed_palette_selected),
		      factory->context);

  return GTK_WIDGET (cmap_dlg);
}

GtkWidget *
dialogs_undo_history_get (GimpDialogFactory *factory)
{
  GDisplay  *gdisp;
  GimpImage *gimage;

  gdisp = gimp_context_get_display (factory->context);

  if (! gdisp)
    return NULL;

  gimage = gdisp->gimage;

  if (! gimage->undo_history)
    gimage->undo_history = undo_history_new (gimage);

  return gimage->undo_history;
}

GtkWidget *
dialogs_display_filters_get (GimpDialogFactory *factory)
{
#ifdef DISPLAY_FILTERS
  GDisplay *gdisp;

  gdisp = gimp_context_get_display (factory->context);

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
dialogs_tips_get (GimpDialogFactory *factory)
{
  return tips_dialog_create ();
}

GtkWidget *
dialogs_about_get (GimpDialogFactory *factory)
{
  return about_dialog_create ();
}


GtkWidget *
dialogs_dock_new (GimpDialogFactory *factory)
{
  return gimp_image_dock_new (factory, image_context);
}


GtkWidget *
dialogs_image_list_view_new (GimpDialogFactory *factory)
{
  GtkWidget *view;

  view = gimp_container_list_view_new (image_context,
				       factory->context,
				       32,
				       5, 3);

  return dialogs_dockable_new (view,
			       "Image List", "Images",
			       NULL);
}

GtkWidget *
dialogs_brush_list_view_new (GimpDialogFactory *factory)
{
  GtkWidget *view;

  view = gimp_data_factory_view_new (GIMP_VIEW_TYPE_LIST,
				     global_brush_factory,
				     dialogs_edit_brush_func,
				     factory->context,
				     32,
				     5, 3);

  return dialogs_dockable_new (view,
			       "Brush List", "Brushes",
			       dialogs_brush_tab_func);
}

GtkWidget *
dialogs_pattern_list_view_new (GimpDialogFactory *factory)
{
  GtkWidget *view;

  view = gimp_data_factory_view_new (GIMP_VIEW_TYPE_LIST,
				     global_pattern_factory,
				     NULL,
				     factory->context,
				     32,
				     5, 3);

  return dialogs_dockable_new (view,
			       "Pattern List", "Patterns",
			       dialogs_pattern_tab_func);
}

GtkWidget *
dialogs_gradient_list_view_new (GimpDialogFactory *factory)
{
  GtkWidget *view;

  view = gimp_data_factory_view_new (GIMP_VIEW_TYPE_LIST,
				     global_gradient_factory,
				     dialogs_edit_gradient_func,
				     factory->context,
				     32,
				     5, 3);

  return dialogs_dockable_new (view,
			       "Gradient List", "Gradients",
			       dialogs_gradient_tab_func);
}

GtkWidget *
dialogs_palette_list_view_new (GimpDialogFactory *factory)
{
  GtkWidget *view;

  view = gimp_data_factory_view_new (GIMP_VIEW_TYPE_LIST,
				     global_palette_factory,
				     dialogs_edit_palette_func,
				     factory->context,
				     32,
				     5, 3);

  return dialogs_dockable_new (view,
			       "Palette List", "Palettes",
			       dialogs_palette_tab_func);
}

GtkWidget *
dialogs_tool_list_view_new (GimpDialogFactory *factory)
{
  GtkWidget *view;

  view = gimp_container_list_view_new (global_tool_info_list,
				       factory->context,
				       22,
				       5, 3);

  return dialogs_dockable_new (view,
			       "Tool List", "Tools",
			       NULL);
}


/*  grid views  */

GtkWidget *
dialogs_image_grid_view_new (GimpDialogFactory *factory)
{
  GtkWidget *view;

  view = gimp_container_grid_view_new (image_context,
				       factory->context,
				       32,
				       5, 3);

  return dialogs_dockable_new (view,
			       "Image Grid", "Images",
			       NULL);
}

GtkWidget *
dialogs_brush_grid_view_new (GimpDialogFactory *factory)
{
  GtkWidget *view;

  view = gimp_data_factory_view_new (GIMP_VIEW_TYPE_GRID,
				     global_brush_factory,
				     dialogs_edit_brush_func,
				     factory->context,
				     32,
				     5, 3);

  return dialogs_dockable_new (view,
			       "Brush Grid", "Brushes",
			       dialogs_brush_tab_func);
}

GtkWidget *
dialogs_pattern_grid_view_new (GimpDialogFactory *factory)
{
  GtkWidget *view;

  view = gimp_data_factory_view_new (GIMP_VIEW_TYPE_GRID,
				     global_pattern_factory,
				     NULL,
				     factory->context,
				     32,
				     5, 3);

  return dialogs_dockable_new (view,
			       "Pattern Grid", "Patterns",
			       dialogs_pattern_tab_func);
}

GtkWidget *
dialogs_gradient_grid_view_new (GimpDialogFactory *factory)
{
  GtkWidget *view;

  view = gimp_data_factory_view_new (GIMP_VIEW_TYPE_GRID,
				     global_gradient_factory,
				     dialogs_edit_gradient_func,
				     factory->context,
				     32,
				     5, 3);

  return dialogs_dockable_new (view,
			       "Gradient Grid", "Gradients",
			       dialogs_gradient_tab_func);
}

GtkWidget *
dialogs_palette_grid_view_new (GimpDialogFactory *factory)
{
  GtkWidget *view;

  view = gimp_data_factory_view_new (GIMP_VIEW_TYPE_GRID,
				     global_palette_factory,
				     dialogs_edit_palette_func,
				     factory->context,
				     32,
				     5, 3);

  return dialogs_dockable_new (view,
			       "Palette Grid", "Palettes",
			       dialogs_palette_tab_func);
}

GtkWidget *
dialogs_tool_grid_view_new (GimpDialogFactory *factory)
{
  GtkWidget *view;

  view = gimp_container_grid_view_new (global_tool_info_list,
				       factory->context,
				       22,
				       5, 3);

  return dialogs_dockable_new (view,
			       "Tool Grid", "Tools",
			       NULL);
}


/*  image related dialogs  */

GtkWidget *
dialogs_layer_list_view_new (GimpDialogFactory *factory)
{
  GimpImage *gimage;
  GtkWidget *view;

  gimage = gimp_context_get_image (factory->context);

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
     (GimpCopyDrawableFunc)    gimp_layer_copy);

  gtk_signal_connect_while_alive
    (GTK_OBJECT (factory->context), "image_changed",
     GTK_SIGNAL_FUNC (dialogs_drawable_view_image_changed),
     view,
     GTK_OBJECT (view));

  return dialogs_dockable_new (view,
			       "Layer List", "Layers",
			       NULL);
}

GtkWidget *
dialogs_channel_list_view_new (GimpDialogFactory *factory)
{
  GimpImage *gimage;
  GtkWidget *view;

  gimage = gimp_context_get_image (factory->context);

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
     (GimpCopyDrawableFunc)    gimp_channel_copy);

  gtk_signal_connect_while_alive
    (GTK_OBJECT (factory->context), "image_changed",
     GTK_SIGNAL_FUNC (dialogs_drawable_view_image_changed),
     view,
     GTK_OBJECT (view));

  return dialogs_dockable_new (view,
			       "Channel List", "Channels",
			       NULL);
}


/*  editor dialogs  */

void
dialogs_edit_brush_func (GimpData *data)
{
  static BrushEditGeneratedWindow *brush_editor_dialog = NULL;

  GimpBrush *brush;

  brush = GIMP_BRUSH (data);

  if (GIMP_IS_BRUSH_GENERATED (brush))
    {
      if (! brush_editor_dialog)
	{
	  brush_editor_dialog = brush_edit_generated_new ();
	}

      brush_edit_generated_set_brush (brush_editor_dialog, brush);
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

  context = dockbook->dock->factory->context;

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

  context = dockbook->dock->factory->context;

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

  context = dockbook->dock->factory->context;

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

  context = dockbook->dock->factory->context;

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
dialogs_dockable_new (GtkWidget              *widget,
		      const gchar            *name,
		      const gchar            *short_name,
		      GimpDockableGetTabFunc  get_tab_func)

{
  GtkWidget *dockable;

  dockable = gimp_dockable_new (name,
				short_name,
				get_tab_func);
  gtk_container_add (GTK_CONTAINER (dockable), widget);
  gtk_widget_show (widget);

  return dockable;
}

static void
dialogs_drawable_view_image_changed (GimpContext          *context,
				     GimpImage            *gimage,
				     GimpDrawableListView *view)
{
  gimp_drawable_list_view_set_image (view, gimage);
}

