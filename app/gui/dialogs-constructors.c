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

#include "apptypes.h"

#include "tools/tool_manager.h"

#include "widgets/gimpcontainerlistview.h"
#include "widgets/gimpcontainergridview.h"
#include "widgets/gimpdatafactoryview.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdock.h"
#include "widgets/gimpdockable.h"
#include "widgets/gimpdockbook.h"
#include "widgets/gimppreview.h"

#include "context_manager.h"
#include "gradient_editor.h"
#include "gimpcontainer.h"
#include "gimpcontext.h"
#include "gimpdatafactory.h"
#include "gimpimage.h"
#include "gimprc.h"

#include "libgimp/gimpintl.h"


static GtkWidget * dialogs_brush_tab_func    (GimpDockable *dockable,
					      GimpDockbook *dockbook,
					      gint          size);
static GtkWidget * dialogs_pattern_tab_func  (GimpDockable *dockable,
					      GimpDockbook *dockbook,
					      gint          size);
static GtkWidget * dialogs_gradient_tab_func (GimpDockable *dockable,
					      GimpDockbook *dockbook,
					      gint          size);
static GtkWidget * dialogs_palette_tab_func  (GimpDockable *dockable,
					      GimpDockbook *dockbook,
					      gint          size);

static GtkWidget * dialogs_dockable_new (GtkWidget              *widget,
					 const gchar            *name,
					 const gchar            *short_name,
					 GimpDockableGetTabFunc  get_tab_func);


/*  public functions  */

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
				     NULL,
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
				     NULL,
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
				     NULL,
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
				     NULL,
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
				     NULL,
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
				     NULL,
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


/*  private functions  */

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

GtkWidget *
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
