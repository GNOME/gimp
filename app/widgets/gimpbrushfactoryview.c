/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbrushfactoryview.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpbrush.h"
#include "core/gimpbrushgenerated.h"
#include "core/gimpdatafactory.h"

#include "gimpbrushfactoryview.h"
#include "gimpcontainerview.h"
#include "gimpeditor.h"
#include "gimpmenufactory.h"
#include "gimpspinscale.h"
#include "gimpviewrenderer.h"

#include "gimp-intl.h"


static void   gimp_brush_factory_view_dispose         (GObject              *object);

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
  GObjectClass             *object_class = G_OBJECT_CLASS (klass);
  GimpContainerEditorClass *editor_class = GIMP_CONTAINER_EDITOR_CLASS (klass);

  object_class->dispose     = gimp_brush_factory_view_dispose;

  editor_class->select_item = gimp_brush_factory_view_select_item;
}

static void
gimp_brush_factory_view_init (GimpBrushFactoryView *view)
{
  view->spacing_adjustment =
    GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 1.0, 5000.0,
                                        1.0, 10.0, 0.0));

  view->spacing_scale = gimp_spin_scale_new (view->spacing_adjustment,
                                             _("Spacing"), 1);
  gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (view->spacing_scale),
                                   1.0, 200.0);
  gimp_help_set_help_data (view->spacing_scale,
                           _("Percentage of width of brush"),
                           NULL);

  g_signal_connect (view->spacing_adjustment, "value-changed",
                    G_CALLBACK (gimp_brush_factory_view_spacing_update),
                    view);
}

static void
gimp_brush_factory_view_dispose (GObject *object)
{
  GimpBrushFactoryView *view   = GIMP_BRUSH_FACTORY_VIEW (object);
  GimpContainerEditor  *editor = GIMP_CONTAINER_EDITOR (object);

  if (view->spacing_changed_handler_id)
    {
      GimpDataFactory *factory;
      GimpContainer   *container;

      factory   = gimp_data_factory_view_get_data_factory (GIMP_DATA_FACTORY_VIEW (editor));
      container = gimp_data_factory_get_container (factory);

      gimp_container_remove_handler (container,
                                     view->spacing_changed_handler_id);

      view->spacing_changed_handler_id = 0;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
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
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= GIMP_VIEW_MAX_BORDER_WIDTH,
                        NULL);
  g_return_val_if_fail (menu_factory == NULL ||
                        GIMP_IS_MENU_FACTORY (menu_factory), NULL);

  factory_view = g_object_new (GIMP_TYPE_BRUSH_FACTORY_VIEW,
                               "view-type",         view_type,
                               "data-factory",      factory,
                               "context",           context,
                               "view-size",         view_size,
                               "view-border-width", view_border_width,
                               "menu-factory",      menu_factory,
                               "menu-identifier",   "<Brushes>",
                               "ui-path",           "/brushes-popup",
                               "action-group",      "brushes",
                               NULL);

  factory_view->change_brush_spacing = change_brush_spacing;

  editor = GIMP_CONTAINER_EDITOR (factory_view);

  gimp_editor_add_action_button (GIMP_EDITOR (editor->view),
                                 "brushes", "brushes-open-as-image",
                                 NULL);

  gtk_box_pack_end (GTK_BOX (editor->view), factory_view->spacing_scale,
                    FALSE, FALSE, 0);
  gtk_widget_show (factory_view->spacing_scale);

  factory_view->spacing_changed_handler_id =
    gimp_container_add_handler (gimp_data_factory_get_container (factory), "spacing-changed",
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

      gimp_brush_set_spacing (brush, gtk_adjustment_get_value (adjustment));

      g_signal_handlers_unblock_by_func (brush,
                                         gimp_brush_factory_view_spacing_changed,
                                         view);
    }
}
