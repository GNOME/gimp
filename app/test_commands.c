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

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "apptypes.h"

#include "tools/tool_manager.h"

#include "widgets/gimpcontainerlistview.h"
#include "widgets/gimpcontainergridview.h"
#include "widgets/gimpdatafactoryview.h"
#include "widgets/gimpdock.h"
#include "widgets/gimpdockable.h"
#include "widgets/gimpdockbook.h"
#include "widgets/gimpdrawablelistview.h"
#include "widgets/gimppreview.h"

#include "context_manager.h"
#include "gradient_editor.h"
#include "gimpchannel.h"
#include "gimpcontainer.h"
#include "gimpcontext.h"
#include "gimpdatafactory.h"
#include "gimpimage.h"
#include "gimplayer.h"
#include "gimprc.h"

#include "libgimp/gimpintl.h"


#define return_if_no_display(gdisp) \
        gdisp = gdisplay_active (); \
        if (!gdisp) return


/*****  Container View Test Dialogs  *****/

static void
container_view_scale_callback (GtkAdjustment     *adj,
			       GimpContainerView *view)
{
  gimp_container_view_set_preview_size (view, ROUND (adj->value));
}

static void
brushes_callback (GtkWidget         *widget,
		  GimpContainerView *view)
{
  gimp_container_view_set_container (view, global_brush_factory->container);
}

static void
patterns_callback (GtkWidget         *widget,
		   GimpContainerView *view)
{
  gimp_container_view_set_container (view, global_pattern_factory->container);
}

static void
gradients_callback (GtkWidget         *widget,
		    GimpContainerView *view)
{
  gimp_container_view_set_container (view, global_gradient_factory->container);
}

static void
palettes_callback (GtkWidget         *widget,
		   GimpContainerView *view)
{
  gimp_container_view_set_container (view, global_palette_factory->container);
}

static void
images_callback (GtkWidget         *widget,
		 GimpContainerView *view)
{
  gimp_container_view_set_container (view, image_context);
}

/*
static void
null_callback (GtkWidget         *widget,
	       GimpContainerView *view)
{
  gimp_container_view_set_container (view, NULL);
}
*/

static void
container_view_new (gboolean       list,
		    gchar         *title,
		    GimpContainer *container,
		    GimpContext   *context,
		    gint           preview_size)
{
  GtkWidget *dialog;
  GtkWidget *view;
  GtkObject *adjustment;
  GtkWidget *scale;

  if (list)
    {
      view = gimp_container_list_view_new (container,
					   context,
					   preview_size,
					   5, 5);
    }
  else
    {
      view = gimp_container_grid_view_new (container,
					   context,
					   preview_size,
					   5, 5);
    }

  dialog = gimp_dialog_new (title, "test",
			    gimp_standard_help_func,
			    NULL,
			    GTK_WIN_POS_MOUSE,
			    FALSE, TRUE, TRUE,

			    "_delete_event_", gtk_widget_destroy,
			    NULL, 1, NULL, TRUE, TRUE,

			    NULL);

  gtk_widget_hide (GTK_DIALOG (dialog)->action_area);

  gtk_widget_hide (GTK_WIDGET (g_list_nth_data (gtk_container_children (GTK_CONTAINER (GTK_BIN (dialog)->child)), 0)));

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), view);
  gtk_widget_show (view);

  adjustment = gtk_adjustment_new (preview_size, 16, 256, 16, 16, 0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), scale,
		      FALSE, FALSE, 0);
  gtk_widget_show (scale);

  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (container_view_scale_callback),
		      view);

  gtk_widget_show (dialog);
}

static void
data_factory_view_new (GimpViewType      view_type,
		       gchar            *title,
		       GimpDataFactory  *factory,
		       GimpDataEditFunc  edit_func,
		       GimpContext      *context,
		       gint              preview_size)
{
  GtkWidget *dialog;
  GtkWidget *view;
  GtkObject *adjustment;
  GtkWidget *scale;

  dialog = gimp_dialog_new (title, "test",
			    gimp_standard_help_func,
			    NULL,
			    GTK_WIN_POS_MOUSE,
			    FALSE, TRUE, TRUE,

			    "_delete_event_", gtk_widget_destroy,
			    NULL, 1, NULL, TRUE, TRUE,

			    NULL);

  gtk_widget_hide (GTK_DIALOG (dialog)->action_area);

  view = gimp_data_factory_view_new (view_type,
				     factory,
				     edit_func,
				     context,
				     preview_size,
				     5, 5);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), view);
  gtk_widget_show (view);

  adjustment = gtk_adjustment_new (preview_size, 16, 256, 16, 16, 0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), scale,
		      FALSE, FALSE, 0);
  gtk_widget_show (scale);

  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (container_view_scale_callback),
		      GIMP_DATA_FACTORY_VIEW (view)->view);

  gtk_widget_show (dialog);
}

static void
container_multi_view_new (gboolean       list,
			  gchar         *title,
			  GimpContainer *container,
			  GimpContext   *context,
			  gint           preview_size)
{
  GtkWidget *dialog;
  GtkWidget *view;
  GtkObject *adjustment;
  GtkWidget *scale;
  GtkWidget *preview;

  if (list)
    {
      view = gimp_container_list_view_new (container,
					   context,
					   preview_size,
					   5, 5);
    }
  else
    {
      view = gimp_container_grid_view_new (container,
					   context,
					   preview_size,
					   5, 5);
    }

  dialog = gimp_dialog_new (title, "test",
			    gimp_standard_help_func,
			    NULL,
			    GTK_WIN_POS_MOUSE,
			    FALSE, TRUE, TRUE,

			    "Images", images_callback,
			    view, NULL, NULL, FALSE, FALSE,

				/*
				  "NULL", null_callback,
				  view, NULL, NULL, FALSE, FALSE,
				*/

			    "_delete_event_", gtk_widget_destroy,
			    NULL, 1, NULL, FALSE, TRUE,

			    NULL);

  preview =
    gimp_preview_new_full (GIMP_VIEWABLE (gimp_context_get_brush (context)),
			   32, 32, 1,
			   FALSE, TRUE, FALSE);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), preview,
		      FALSE, FALSE, 0);
  gtk_widget_show (preview);

  gtk_signal_connect (GTK_OBJECT (preview), "clicked",
		      GTK_SIGNAL_FUNC (brushes_callback),
		      view);

  gtk_signal_connect_object_while_alive
    (GTK_OBJECT (context),
     "brush_changed",
     GTK_SIGNAL_FUNC (gimp_preview_set_viewable),
     GTK_OBJECT (preview));

  preview =
    gimp_preview_new_full (GIMP_VIEWABLE (gimp_context_get_pattern (context)),
			   32, 32, 1,
			   FALSE, TRUE, FALSE);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), preview,
		      FALSE, FALSE, 0);
  gtk_widget_show (preview);

  gtk_signal_connect (GTK_OBJECT (preview), "clicked",
		      GTK_SIGNAL_FUNC (patterns_callback),
		      view);

  gtk_signal_connect_object_while_alive
    (GTK_OBJECT (context),
     "pattern_changed",
     GTK_SIGNAL_FUNC (gimp_preview_set_viewable),
     GTK_OBJECT (preview));

  preview =
    gimp_preview_new_full (GIMP_VIEWABLE (gimp_context_get_gradient (context)),
			   32, 32, 1,
			   FALSE, TRUE, FALSE);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), preview,
		      FALSE, FALSE, 0);
  gtk_widget_show (preview);

  gtk_signal_connect (GTK_OBJECT (preview), "clicked",
		      GTK_SIGNAL_FUNC (gradients_callback),
		      view);

  gtk_signal_connect_object_while_alive
    (GTK_OBJECT (context),
     "gradient_changed",
     GTK_SIGNAL_FUNC (gimp_preview_set_viewable),
     GTK_OBJECT (preview));

  preview =
    gimp_preview_new_full (GIMP_VIEWABLE (gimp_context_get_palette (context)),
			   32, 32, 1,
			   FALSE, TRUE, FALSE);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), preview,
		      FALSE, FALSE, 0);
  gtk_widget_show (preview);

  gtk_signal_connect (GTK_OBJECT (preview), "clicked",
		      GTK_SIGNAL_FUNC (palettes_callback),
		      view);

  gtk_signal_connect_object_while_alive
    (GTK_OBJECT (context),
     "palette_changed",
     GTK_SIGNAL_FUNC (gimp_preview_set_viewable),
     GTK_OBJECT (preview));

  /*
  preview =
    gimp_preview_new_full (GIMP_VIEWABLE (gimp_context_get_image (context)),
			   32, 32, 1,
			   FALSE, TRUE, FALSE);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), preview,
		      FALSE, FALSE, 0);
  gtk_widget_show (preview);

  gtk_signal_connect (GTK_OBJECT (preview), "clicked",
		      GTK_SIGNAL_FUNC (images_callback),
		      view);

  gtk_signal_connect_object_while_alive
    (GTK_OBJECT (context),
     "pattern_changed",
     GTK_SIGNAL_FUNC (gimp_preview_set_viewable),
     GTK_OBJECT (preview));
  */

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), view);
  gtk_widget_show (view);

  adjustment = gtk_adjustment_new (preview_size, 16, 64, 4, 4, 0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), scale,
		      FALSE, FALSE, 0);
  gtk_widget_show (scale);

  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (container_view_scale_callback),
		      view);

  gtk_widget_show (dialog);
}

void
test_image_container_list_view_cmd_callback (GtkWidget *widget,
					     gpointer   client_data)
{
  container_view_new (TRUE, "Image List",
		      image_context,
		      gimp_context_get_user (),
		      64);
}

void
test_image_container_grid_view_cmd_callback (GtkWidget *widget,
					     gpointer   client_data)
{
  container_view_new (FALSE, "Image Grid",
		      image_context,
		      gimp_context_get_user (),
		      64);
}

void
test_brush_container_list_view_cmd_callback (GtkWidget *widget,
					     gpointer   client_data)
{
  data_factory_view_new (GIMP_VIEW_TYPE_LIST,
			 "Brush List",
			 global_brush_factory,
			 NULL,
			 gimp_context_get_user (),
			 24);
}

void
test_pattern_container_list_view_cmd_callback (GtkWidget *widget,
					       gpointer   client_data)
{
  data_factory_view_new (GIMP_VIEW_TYPE_LIST,
			 "Pattern List",
			 global_pattern_factory,
			 NULL,
			 gimp_context_get_user (),
			 24);
}

void
test_gradient_container_list_view_cmd_callback (GtkWidget *widget,
						gpointer   client_data)
{
  data_factory_view_new (GIMP_VIEW_TYPE_LIST,
			 "Gradient List",
			 global_gradient_factory,
			 NULL,
			 gimp_context_get_user (),
			 24);
}

void
test_palette_container_list_view_cmd_callback (GtkWidget *widget,
					       gpointer   client_data)
{
  data_factory_view_new (GIMP_VIEW_TYPE_LIST,
			 "Palette List",
			 global_palette_factory,
			 NULL,
			 gimp_context_get_user (),
			 24);
}

void
test_brush_container_grid_view_cmd_callback (GtkWidget *widget,
					     gpointer   client_data)
{
  data_factory_view_new (GIMP_VIEW_TYPE_GRID,
			 "Brush Grid",
			 global_brush_factory,
			 NULL,
			 gimp_context_get_user (),
			 32);
}

void
test_pattern_container_grid_view_cmd_callback (GtkWidget *widget,
					       gpointer   client_data)
{
  data_factory_view_new (GIMP_VIEW_TYPE_GRID,
			 "Pattern Grid",
			 global_pattern_factory,
			 NULL,
			 gimp_context_get_user (),
			 24);
}

void
test_gradient_container_grid_view_cmd_callback (GtkWidget *widget,
						gpointer   client_data)
{
  data_factory_view_new (GIMP_VIEW_TYPE_GRID,
			 "Gradient Grid",
			 global_gradient_factory,
			 NULL,
			 gimp_context_get_user (),
			 24);
}

void
test_palette_container_grid_view_cmd_callback (GtkWidget *widget,
					       gpointer   client_data)
{
  data_factory_view_new (GIMP_VIEW_TYPE_GRID,
			 "Palette Grid",
			 global_palette_factory,
			 NULL,
			 gimp_context_get_user (),
			 24);
}

void
test_multi_container_list_view_cmd_callback (GtkWidget *widget,
					     gpointer   client_data)
{
  container_multi_view_new (TRUE, "Multi List",
			    global_brush_factory->container,
			    gimp_context_get_user (),
			    24);
}

void
test_multi_container_grid_view_cmd_callback (GtkWidget *widget,
					     gpointer   client_data)
{
  container_multi_view_new (FALSE, "Multi Grid",
			    global_brush_factory->container,
			    gimp_context_get_user (),
			    32);
}

static void
drawable_view_image_changed (GimpContext          *context,
			     GimpImage            *gimage,
			     GimpDrawableListView *view)
{
  gimp_drawable_list_view_set_image (view, gimage);
}

static void
drawable_view_new (gchar         *title,
		   gboolean       channels,
		   GimpContext   *context)
{
  GimpImage *gimage;
  GtkWidget *dialog;
  GtkWidget *view;

  gimage = gimp_context_get_image (context);

  if (channels)
    {
      view = gimp_drawable_list_view_new (gimage,
					  GIMP_TYPE_CHANNEL,
					  "active_channel_changed",
					  gimp_image_get_channels,
					  gimp_image_get_active_channel,
					  gimp_image_set_active_channel,
					  gimp_image_position_channel,
					  gimp_image_add_channel,
					  gimp_image_remove_channel,
					  gimp_channel_copy);
    }
  else
    {
      view = gimp_drawable_list_view_new (gimage,
					  GIMP_TYPE_LAYER,
					  "active_layer_changed",
					  gimp_image_get_layers,
					  gimp_image_get_active_layer,
					  gimp_image_set_active_layer,
					  gimp_image_position_layer,
					  gimp_image_add_layer,
					  gimp_image_remove_layer,
					  gimp_layer_copy);
    }

  gtk_list_set_selection_mode (GTK_LIST (GIMP_CONTAINER_LIST_VIEW (view)->gtk_list),
			       GTK_SELECTION_SINGLE);

  gtk_signal_connect (GTK_OBJECT (context), "image_changed",
		      GTK_SIGNAL_FUNC (drawable_view_image_changed),
		      view);

  dialog = gimp_dialog_new (title, "test",
			    gimp_standard_help_func,
			    NULL,
			    GTK_WIN_POS_MOUSE,
			    FALSE, TRUE, TRUE,

			    "_delete_event_", gtk_widget_destroy,
			    NULL, 1, NULL, TRUE, TRUE,

			    NULL);

  gtk_widget_hide (GTK_DIALOG (dialog)->action_area);

  gtk_widget_hide (GTK_WIDGET (g_list_nth_data (gtk_container_children (GTK_CONTAINER (GTK_BIN (dialog)->child)), 0)));

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), view);
  gtk_widget_show (view);

  gtk_widget_show (dialog);
}

void
test_layer_list_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  drawable_view_new ("Layer List", FALSE,
		     gimp_context_get_user ());
}

void
test_channel_list_cmd_callback (GtkWidget *widget,
				gpointer   client_data)
{
  drawable_view_new ("Channel List", TRUE,
		     gimp_context_get_user ());
}

static GtkWidget *
test_brush_tab_func (GimpDockable *dockable,
		     gint          size)
{
  GimpContext *context;
  GtkWidget   *preview;

  context = gimp_context_get_user ();

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
test_pattern_tab_func (GimpDockable *dockable,
		       gint          size)
{
  GimpContext *context;
  GtkWidget   *preview;

  context = gimp_context_get_user ();

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
test_gradient_tab_func (GimpDockable *dockable,
			gint          size)
{
  GimpContext *context;
  GtkWidget   *preview;

  context = gimp_context_get_user ();

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
test_palette_tab_func (GimpDockable *dockable,
		       gint          size)
{
  GimpContext *context;
  GtkWidget   *preview;

  context = gimp_context_get_user ();

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

void
test_dock_new (GimpViewType  view_type,
	       GimpContext  *context)
{
  GtkWidget *dock;
  GtkWidget *dockbook;
  GtkWidget *dockable;
  GtkWidget *view;

  dock = gimp_dock_new ();

  dockbook = gimp_dockbook_new ();

  gimp_dock_add_book (GIMP_DOCK (dock), GIMP_DOCKBOOK (dockbook), 0);

  dockable = gimp_dockable_new (view_type == GIMP_VIEW_TYPE_LIST ?
				"Brush List" : "Brush Grid",
				"Brushes",
				test_brush_tab_func);
  view = gimp_data_factory_view_new (view_type,
				     global_brush_factory,
				     NULL,
				     context,
				     32,
				     5, 3);
  gtk_container_add (GTK_CONTAINER (dockable), view);
  gtk_widget_show (view);

  gimp_dock_add (GIMP_DOCK (dock), GIMP_DOCKABLE (dockable), -1, -1);

  dockable = gimp_dockable_new (view_type == GIMP_VIEW_TYPE_LIST ?
				"Pattern List" : "Pattern Grid",
				"Patterns",
				test_pattern_tab_func);
  view = gimp_data_factory_view_new (view_type,
				     global_pattern_factory,
				     NULL,
				     context,
				     32,
				     5, 3);
  gtk_container_add (GTK_CONTAINER (dockable), view);
  gtk_widget_show (view);

  gimp_dock_add (GIMP_DOCK (dock), GIMP_DOCKABLE (dockable), -1, -1);

  dockable = gimp_dockable_new (view_type == GIMP_VIEW_TYPE_LIST ?
				"Gradient List" : "Gradient Grid",
				"Gradients",
				test_gradient_tab_func);
  view = gimp_data_factory_view_new (view_type,
				     global_gradient_factory,
				     NULL,
				     context,
				     32,
				     5, 3);
  gtk_container_add (GTK_CONTAINER (dockable), view);
  gtk_widget_show (view);

  gimp_dock_add (GIMP_DOCK (dock), GIMP_DOCKABLE (dockable), -1, -1);

  dockable = gimp_dockable_new (view_type == GIMP_VIEW_TYPE_LIST ?
				"Palette List" : "Palette Grid",
				"Palettes",
				test_palette_tab_func);
  view = gimp_data_factory_view_new (view_type,
				     global_palette_factory,
				     NULL,
				     context,
				     32,
				     5, 3);
  gtk_container_add (GTK_CONTAINER (dockable), view);
  gtk_widget_show (view);

  gimp_dock_add (GIMP_DOCK (dock), GIMP_DOCKABLE (dockable), -1, -1);

  switch (view_type)
    {
    case GIMP_VIEW_TYPE_LIST:
      dockable = gimp_dockable_new ("Tool List", "Tools",
				    NULL);
      view = gimp_container_list_view_new (global_tool_info_list,
					   context,
					   22,
					   5, 3);
      break;

    case GIMP_VIEW_TYPE_GRID:
      dockable = gimp_dockable_new ("Tool Grid", "Tools",
				    NULL);
      view = gimp_container_grid_view_new (global_tool_info_list,
					   context,
					   22,
					   5, 3);
      break;

    default:
      g_assert_not_reached ();
    }

  gtk_container_add (GTK_CONTAINER (dockable), view);
  gtk_widget_show (view);

  gimp_dock_add (GIMP_DOCK (dock), GIMP_DOCKABLE (dockable), -1, -1);

  gtk_widget_show (dock);
}

void
test_list_dock_cmd_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  test_dock_new (GIMP_VIEW_TYPE_LIST,
		 gimp_context_get_user ());
}

void
test_grid_dock_cmd_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  test_dock_new (GIMP_VIEW_TYPE_GRID,
		 gimp_context_get_user ());
}
