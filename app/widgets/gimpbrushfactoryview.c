/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbrushfactoryview.c
 * Copyright (C) 2001 Michael Natterer
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

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpbrush.h"
#include "core/gimpbrushgenerated.h"
#include "core/gimpdatafactory.h"
#include "core/gimpmarshal.h"

#include "gimpcontainerview.h"
#include "gimpbrushfactoryview.h"

#include "libgimp/gimpintl.h"


static void   gimp_brush_factory_view_class_init (GimpBrushFactoryViewClass *klass);
static void   gimp_brush_factory_view_init       (GimpBrushFactoryView      *view);
static void   gimp_brush_factory_view_destroy    (GtkObject                *object);

static void   gimp_brush_factory_view_select_item     (GimpContainerEditor  *editor,
						       GimpViewable         *viewable);

static void   gimp_brush_factory_view_spacing_changed (GimpBrush            *brush,
						       GimpBrushFactoryView *view);
static void   gimp_brush_factory_view_spacing_update  (GtkAdjustment        *adjustment,
						       GimpBrushFactoryView *view);


static GimpDataFactoryViewClass *parent_class = NULL;


GtkType
gimp_brush_factory_view_get_type (void)
{
  static guint view_type = 0;

  if (! view_type)
    {
      GtkTypeInfo view_info =
      {
	"GimpBrushFactoryView",
	sizeof (GimpBrushFactoryView),
	sizeof (GimpBrushFactoryViewClass),
	(GtkClassInitFunc) gimp_brush_factory_view_class_init,
	(GtkObjectInitFunc) gimp_brush_factory_view_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      view_type = gtk_type_unique (GIMP_TYPE_DATA_FACTORY_VIEW, &view_info);
    }

  return view_type;
}

static void
gimp_brush_factory_view_class_init (GimpBrushFactoryViewClass *klass)
{
  GtkObjectClass           *object_class;
  GimpContainerEditorClass *editor_class;

  object_class = (GtkObjectClass *) klass;
  editor_class = (GimpContainerEditorClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_DATA_FACTORY_VIEW);

  object_class->destroy     = gimp_brush_factory_view_destroy;

  editor_class->select_item = gimp_brush_factory_view_select_item;
}

static void
gimp_brush_factory_view_init (GimpBrushFactoryView *view)
{
  GtkWidget *table;

  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_box_pack_end (GTK_BOX (view), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  view->spacing_adjustment =
    GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 1.0, 1000.0, 1.0, 1.0, 0.0));

  view->spacing_scale = gtk_hscale_new (view->spacing_adjustment);
  gtk_scale_set_value_pos (GTK_SCALE (view->spacing_scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (view->spacing_scale),
			       GTK_UPDATE_DELAYED);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Spacing:"), 1.0, 1.0,
			     view->spacing_scale, 1, FALSE);

  g_signal_connect (G_OBJECT (view->spacing_adjustment), "value_changed",
		    G_CALLBACK (gimp_brush_factory_view_spacing_update),
		    view);

  view->spacing_changed_handler_id = 0;
}

static void
gimp_brush_factory_view_destroy (GtkObject *object)
{
  GimpBrushFactoryView *view;

  view = GIMP_BRUSH_FACTORY_VIEW (object);

  if (view->spacing_changed_handler_id)
    {
      gimp_container_remove_handler
	(GIMP_CONTAINER_EDITOR (view)->view->container,
	 view->spacing_changed_handler_id);

      view->spacing_changed_handler_id = 0;
    }			     

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget *
gimp_brush_factory_view_new (GimpViewType              view_type,
			     GimpDataFactory          *factory,
			     GimpDataEditFunc          edit_func,
			     GimpContext              *context,
			     gboolean                  change_brush_spacing,
			     gint                      preview_size,
			     gint                      min_items_x,
			     gint                      min_items_y,
			     GimpContainerContextFunc  context_func)
{
  GimpBrushFactoryView *factory_view;
  GimpContainerEditor  *editor;

  g_return_val_if_fail (factory != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (preview_size > 0 && preview_size <= 64, NULL);
  g_return_val_if_fail (min_items_x > 0 && min_items_x <= 64, NULL);
  g_return_val_if_fail (min_items_y > 0 && min_items_y <= 64, NULL);

  factory_view = gtk_type_new (GIMP_TYPE_BRUSH_FACTORY_VIEW);

  factory_view->change_brush_spacing = change_brush_spacing;

  if (! gimp_data_factory_view_construct (GIMP_DATA_FACTORY_VIEW (factory_view),
					  view_type,
					  factory,
					  edit_func,
					  context,
					  preview_size,
					  min_items_x,
					  min_items_y,
					  context_func))
    {
      g_object_unref (G_OBJECT (factory_view));
      return NULL;
    }

  editor = GIMP_CONTAINER_EDITOR (factory_view);

  gimp_container_add_handler
    (editor->view->container, "spacing_changed",
     G_CALLBACK (gimp_brush_factory_view_spacing_changed),
     factory_view);

  return GTK_WIDGET (factory_view);
}

static void
gimp_brush_factory_view_select_item (GimpContainerEditor *editor,
				     GimpViewable        *viewable)
{
  GimpBrushFactoryView *view;

  gboolean  edit_sensitive    = FALSE;
  gboolean  spacing_sensitive = FALSE;

  if (GIMP_CONTAINER_EDITOR_CLASS (parent_class)->select_item)
    GIMP_CONTAINER_EDITOR_CLASS (parent_class)->select_item (editor, viewable);

  view = GIMP_BRUSH_FACTORY_VIEW (editor);

  if (viewable &&
      gimp_container_have (GIMP_CONTAINER_EDITOR (view)->view->container,
			   GIMP_OBJECT (viewable)))
    {
      GimpBrush *brush;

      brush = GIMP_BRUSH (viewable);

      edit_sensitive    = GIMP_IS_BRUSH_GENERATED (brush);
      spacing_sensitive = TRUE;

      g_signal_handlers_block_by_func (G_OBJECT (view->spacing_adjustment),
				       gimp_brush_factory_view_spacing_update,
				       view);

      gtk_adjustment_set_value (view->spacing_adjustment,
				gimp_brush_get_spacing (brush));

      g_signal_handlers_unblock_by_func (G_OBJECT (view->spacing_adjustment),
					 gimp_brush_factory_view_spacing_update,
					 view);
    }

  gtk_widget_set_sensitive (GIMP_DATA_FACTORY_VIEW (view)->edit_button,
			    edit_sensitive);
  gtk_widget_set_sensitive (view->spacing_scale, spacing_sensitive);
}

static void
gimp_brush_factory_view_spacing_changed (GimpBrush            *brush,
					 GimpBrushFactoryView *view)
{
  if (brush == GIMP_CONTAINER_EDITOR (view)->view->context->brush)
    {
      g_signal_handlers_block_by_func (G_OBJECT (view->spacing_adjustment),
				       gimp_brush_factory_view_spacing_update,
				       view);

      gtk_adjustment_set_value (view->spacing_adjustment,
				gimp_brush_get_spacing (brush));

      g_signal_handlers_unblock_by_func (G_OBJECT (view->spacing_adjustment),
					 gimp_brush_factory_view_spacing_update,
					 view);
    }
}

static void
gimp_brush_factory_view_spacing_update (GtkAdjustment        *adjustment,
					GimpBrushFactoryView *view)
{
  GimpBrush *brush;

  brush = GIMP_CONTAINER_EDITOR (view)->view->context->brush;

  if (brush && view->change_brush_spacing)
    {
      g_signal_handlers_block_by_func (G_OBJECT (brush),
				       gimp_brush_factory_view_spacing_changed,
				       view);

      gimp_brush_set_spacing (brush, adjustment->value);

      g_signal_handlers_unblock_by_func (G_OBJECT (brush),
					 gimp_brush_factory_view_spacing_changed,
					 view);
    }
}
