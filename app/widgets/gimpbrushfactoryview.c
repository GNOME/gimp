/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmabrushfactoryview.c
 * Copyright (C) 2001 Michael Natterer <mitch@ligma.org>
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

#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmabrush.h"
#include "core/ligmabrushgenerated.h"
#include "core/ligmadatafactory.h"

#include "ligmabrushfactoryview.h"
#include "ligmacontainerview.h"
#include "ligmaeditor.h"
#include "ligmamenufactory.h"
#include "ligmaviewrenderer.h"

#include "ligma-intl.h"

enum
{
  SPACING_CHANGED,
  LAST_SIGNAL
};


static void   ligma_brush_factory_view_dispose         (GObject              *object);

static void   ligma_brush_factory_view_select_item     (LigmaContainerEditor  *editor,
                                                       LigmaViewable         *viewable);

static void   ligma_brush_factory_view_spacing_changed (LigmaBrush            *brush,
                                                       LigmaBrushFactoryView *view);
static void   ligma_brush_factory_view_spacing_update  (GtkAdjustment        *adjustment,
                                                       LigmaBrushFactoryView *view);


G_DEFINE_TYPE (LigmaBrushFactoryView, ligma_brush_factory_view,
               LIGMA_TYPE_DATA_FACTORY_VIEW)

#define parent_class ligma_brush_factory_view_parent_class

static guint ligma_brush_factory_view_signals[LAST_SIGNAL] = { 0 };


static void
ligma_brush_factory_view_class_init (LigmaBrushFactoryViewClass *klass)
{
  GObjectClass             *object_class = G_OBJECT_CLASS (klass);
  LigmaContainerEditorClass *editor_class = LIGMA_CONTAINER_EDITOR_CLASS (klass);

  object_class->dispose     = ligma_brush_factory_view_dispose;

  editor_class->select_item = ligma_brush_factory_view_select_item;

  /**
   * LigmaBrushFactoryView::spacing-changed:
   * @view: the #LigmaBrushFactoryView.
   *
   * Emitted when the spacing changed.
   */
  ligma_brush_factory_view_signals[SPACING_CHANGED] =
    g_signal_new ("spacing-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaBrushFactoryViewClass, spacing_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);
}

static void
ligma_brush_factory_view_init (LigmaBrushFactoryView *view)
{
  view->spacing_adjustment = gtk_adjustment_new (0.0, 1.0, 5000.0,
                                                 1.0, 10.0, 0.0);

  view->spacing_scale = ligma_spin_scale_new (view->spacing_adjustment,
                                             _("Spacing"), 1);
  ligma_spin_scale_set_scale_limits (LIGMA_SPIN_SCALE (view->spacing_scale),
                                   1.0, 200.0);
  ligma_help_set_help_data (view->spacing_scale,
                           _("Percentage of width of brush"),
                           NULL);

  g_signal_connect (view->spacing_adjustment, "value-changed",
                    G_CALLBACK (ligma_brush_factory_view_spacing_update),
                    view);
}

static void
ligma_brush_factory_view_dispose (GObject *object)
{
  LigmaBrushFactoryView *view   = LIGMA_BRUSH_FACTORY_VIEW (object);
  LigmaContainerEditor  *editor = LIGMA_CONTAINER_EDITOR (object);

  if (view->spacing_changed_handler_id)
    {
      LigmaDataFactory *factory;
      LigmaContainer   *container;

      factory   = ligma_data_factory_view_get_data_factory (LIGMA_DATA_FACTORY_VIEW (editor));
      container = ligma_data_factory_get_container (factory);

      ligma_container_remove_handler (container,
                                     view->spacing_changed_handler_id);

      view->spacing_changed_handler_id = 0;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

GtkWidget *
ligma_brush_factory_view_new (LigmaViewType     view_type,
                             LigmaDataFactory *factory,
                             LigmaContext     *context,
                             gboolean         change_brush_spacing,
                             gint             view_size,
                             gint             view_border_width,
                             LigmaMenuFactory *menu_factory)
{
  LigmaBrushFactoryView *factory_view;
  LigmaContainerEditor  *editor;

  g_return_val_if_fail (LIGMA_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= LIGMA_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= LIGMA_VIEW_MAX_BORDER_WIDTH,
                        NULL);
  g_return_val_if_fail (menu_factory == NULL ||
                        LIGMA_IS_MENU_FACTORY (menu_factory), NULL);

  factory_view = g_object_new (LIGMA_TYPE_BRUSH_FACTORY_VIEW,
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

  editor = LIGMA_CONTAINER_EDITOR (factory_view);

  ligma_editor_add_action_button (LIGMA_EDITOR (editor->view),
                                 "brushes", "brushes-open-as-image",
                                 NULL);

  gtk_box_pack_end (GTK_BOX (editor->view), factory_view->spacing_scale,
                    FALSE, FALSE, 0);
  gtk_widget_show (factory_view->spacing_scale);

  factory_view->spacing_changed_handler_id =
    ligma_container_add_handler (ligma_data_factory_get_container (factory), "spacing-changed",
                                G_CALLBACK (ligma_brush_factory_view_spacing_changed),
                                factory_view);

  return GTK_WIDGET (factory_view);
}

static void
ligma_brush_factory_view_select_item (LigmaContainerEditor *editor,
                                     LigmaViewable        *viewable)
{
  LigmaBrushFactoryView *view = LIGMA_BRUSH_FACTORY_VIEW (editor);
  LigmaContainer        *container;
  gboolean              spacing_sensitive = FALSE;

  if (LIGMA_CONTAINER_EDITOR_CLASS (parent_class)->select_item)
    LIGMA_CONTAINER_EDITOR_CLASS (parent_class)->select_item (editor, viewable);

  container = ligma_container_view_get_container (editor->view);

  if (viewable && ligma_container_have (container, LIGMA_OBJECT (viewable)))
    {
      LigmaBrush *brush = LIGMA_BRUSH (viewable);

      spacing_sensitive = TRUE;

      g_signal_handlers_block_by_func (view->spacing_adjustment,
                                       ligma_brush_factory_view_spacing_update,
                                       view);

      gtk_adjustment_set_value (view->spacing_adjustment,
                                ligma_brush_get_spacing (brush));

      g_signal_handlers_unblock_by_func (view->spacing_adjustment,
                                         ligma_brush_factory_view_spacing_update,
                                         view);
    }

  gtk_widget_set_sensitive (view->spacing_scale, spacing_sensitive);
}

static void
ligma_brush_factory_view_spacing_changed (LigmaBrush            *brush,
                                         LigmaBrushFactoryView *view)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (view);
  LigmaContext         *context;

  context = ligma_container_view_get_context (editor->view);

  if (brush == ligma_context_get_brush (context))
    {
      g_signal_handlers_block_by_func (view->spacing_adjustment,
                                       ligma_brush_factory_view_spacing_update,
                                       view);

      gtk_adjustment_set_value (view->spacing_adjustment,
                                ligma_brush_get_spacing (brush));

      g_signal_handlers_unblock_by_func (view->spacing_adjustment,
                                         ligma_brush_factory_view_spacing_update,
                                         view);
    }
}

static void
ligma_brush_factory_view_spacing_update (GtkAdjustment        *adjustment,
                                        LigmaBrushFactoryView *view)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (view);
  LigmaContext         *context;
  LigmaBrush           *brush;

  context = ligma_container_view_get_context (editor->view);

  brush = ligma_context_get_brush (context);

  if (brush && view->change_brush_spacing)
    {
      g_signal_handlers_block_by_func (brush,
                                       ligma_brush_factory_view_spacing_changed,
                                       view);

      ligma_brush_set_spacing (brush, gtk_adjustment_get_value (adjustment));

      g_signal_handlers_unblock_by_func (brush,
                                         ligma_brush_factory_view_spacing_changed,
                                         view);
    }
  g_signal_emit (view, ligma_brush_factory_view_signals[SPACING_CHANGED], 0);
}
