/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbrushfactoryview.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpbrush.h"
#include "core/gimpbrushgenerated.h"
#include "core/gimpdatafactory.h"

#include "gimpcontainerview.h"
#include "gimpbrushfactoryview.h"
#include "gimpviewrenderer.h"

#include "gimp-intl.h"


static void   gimp_brush_factory_view_destroy         (GtkObject            *object);

static void   gimp_brush_factory_view_select_item     (GimpContainerEditor  *editor,
                                                       GimpViewable         *viewable);

static void   gimp_brush_factory_view_spacing_changed (GimpBrush            *brush,
                                                       GimpBrushFactoryView *view);
static void   gimp_brush_factory_view_spacing_update  (GtkAdjustment        *adjustment,
                                                       GimpBrushFactoryView *view);


G_DEFINE_TYPE (GimpBrushFactoryView, gimp_brush_factory_view,
               GIMP_TYPE_DATA_FACTORY_VIEW)

#define parent_class gimp_brush_factory_view_parent_class


static void
gimp_brush_factory_view_class_init (GimpBrushFactoryViewClass *klass)
{
  GtkObjectClass           *object_class = GTK_OBJECT_CLASS (klass);
  GimpContainerEditorClass *editor_class = GIMP_CONTAINER_EDITOR_CLASS (klass);

  object_class->destroy     = gimp_brush_factory_view_destroy;

  editor_class->select_item = gimp_brush_factory_view_select_item;
}

static void
gimp_brush_factory_view_init (GimpBrushFactoryView *view)
{
  GtkWidget *table;

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_widget_show (table);

  view->spacing_adjustment =
    GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                                          _("Spacing:"), -1, -1,
                                          0.0, 1.0, 200.0, 1.0, 10.0, 1,
                                          FALSE, 1.0, 5000.0,
                                          _("Percentage of width of brush"),
                                          NULL));

  view->spacing_scale = GIMP_SCALE_ENTRY_SCALE (view->spacing_adjustment);

  g_signal_connect (view->spacing_adjustment, "value-changed",
                    G_CALLBACK (gimp_brush_factory_view_spacing_update),
                    view);

  view->spacing_changed_handler_id = 0;
}

static void
gimp_brush_factory_view_destroy (GtkObject *object)
{
  GimpBrushFactoryView *view   = GIMP_BRUSH_FACTORY_VIEW (object);
  GimpContainerEditor  *editor = GIMP_CONTAINER_EDITOR (object);

  if (view->spacing_changed_handler_id)
    {
      GimpContainer *container;

      container = gimp_container_view_get_container (editor->view);

      gimp_container_remove_handler (container,
                                     view->spacing_changed_handler_id);

      view->spacing_changed_handler_id = 0;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget *
gimp_brush_factory_view_new (GimpViewType     view_type,
                             GimpDataFactory *factory,
                             GimpContext     *context,
                             gboolean         change_brush_spacing,
                             gint             view_size,
                             gint             view_border_width,
                             GimpMenuFactory *menu_factory)
{
  GimpBrushFactoryView *factory_view;
  GimpContainerEditor  *editor;

  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= GIMP_VIEW_MAX_BORDER_WIDTH,
                        NULL);

  factory_view = g_object_new (GIMP_TYPE_BRUSH_FACTORY_VIEW, NULL);

  factory_view->change_brush_spacing = change_brush_spacing;

  if (! gimp_data_factory_view_construct (GIMP_DATA_FACTORY_VIEW (factory_view),
                                          view_type,
                                          factory,
                                          context,
                                          view_size, view_border_width,
                                          menu_factory, "<Brushes>",
                                          "/brushes-popup",
                                          "brushes"))
    {
      g_object_unref (factory_view);
      return NULL;
    }

  editor = GIMP_CONTAINER_EDITOR (factory_view);

  /*  eek  */
  gtk_box_pack_end (GTK_BOX (editor->view),
                    factory_view->spacing_scale->parent,
                    FALSE, FALSE, 0);

  factory_view->spacing_changed_handler_id =
    gimp_container_add_handler (factory->container, "spacing-changed",
                                G_CALLBACK (gimp_brush_factory_view_spacing_changed),
                                factory_view);

  return GTK_WIDGET (factory_view);
}

static void
gimp_brush_factory_view_select_item (GimpContainerEditor *editor,
                                     GimpViewable        *viewable)
{
  GimpBrushFactoryView *view = GIMP_BRUSH_FACTORY_VIEW (editor);
  GimpContainer        *container;
  gboolean              spacing_sensitive = FALSE;

  if (GIMP_CONTAINER_EDITOR_CLASS (parent_class)->select_item)
    GIMP_CONTAINER_EDITOR_CLASS (parent_class)->select_item (editor, viewable);

  container = gimp_container_view_get_container (editor->view);

  if (viewable && gimp_container_have (container, GIMP_OBJECT (viewable)))
    {
      GimpBrush *brush = GIMP_BRUSH (viewable);

      spacing_sensitive = TRUE;

      g_signal_handlers_block_by_func (view->spacing_adjustment,
                                       gimp_brush_factory_view_spacing_update,
                                       view);

      gtk_adjustment_set_value (view->spacing_adjustment,
                                gimp_brush_get_spacing (brush));

      g_signal_handlers_unblock_by_func (view->spacing_adjustment,
                                         gimp_brush_factory_view_spacing_update,
                                         view);
    }

  gtk_widget_set_sensitive (view->spacing_scale, spacing_sensitive);
}

static void
gimp_brush_factory_view_spacing_changed (GimpBrush            *brush,
                                         GimpBrushFactoryView *view)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (view);
  GimpContext         *context;

  context = gimp_container_view_get_context (editor->view);

  if (brush == gimp_context_get_brush (context))
    {
      g_signal_handlers_block_by_func (view->spacing_adjustment,
                                       gimp_brush_factory_view_spacing_update,
                                       view);

      gtk_adjustment_set_value (view->spacing_adjustment,
                                gimp_brush_get_spacing (brush));

      g_signal_handlers_unblock_by_func (view->spacing_adjustment,
                                         gimp_brush_factory_view_spacing_update,
                                         view);
    }
}

static void
gimp_brush_factory_view_spacing_update (GtkAdjustment        *adjustment,
                                        GimpBrushFactoryView *view)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (view);
  GimpContext         *context;
  GimpBrush           *brush;

  context = gimp_container_view_get_context (editor->view);

  brush = gimp_context_get_brush (context);

  if (brush && view->change_brush_spacing)
    {
      g_signal_handlers_block_by_func (brush,
                                       gimp_brush_factory_view_spacing_changed,
                                       view);

      gimp_brush_set_spacing (brush, adjustment->value);

      g_signal_handlers_unblock_by_func (brush,
                                         gimp_brush_factory_view_spacing_changed,
                                         view);
    }
}
