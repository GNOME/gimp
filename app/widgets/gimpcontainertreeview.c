/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacontainertreeview.c
 * Copyright (C) 2003-2010 Michael Natterer <mitch@ligma.org>
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

#include <string.h>

#include <gegl.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmaviewable.h"
#include "core/ligma-utils.h"

#include "ligmacellrendererbutton.h"
#include "ligmacellrendererviewable.h"
#include "ligmacontainertreestore.h"
#include "ligmacontainertreeview.h"
#include "ligmacontainertreeview-dnd.h"
#include "ligmacontainertreeview.h"
#include "ligmacontainertreeview-private.h"
#include "ligmacontainerview.h"
#include "ligmadnd.h"
#include "ligmaviewrenderer.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


enum
{
  EDIT_NAME,
  LAST_SIGNAL
};


static void          ligma_container_tree_view_view_iface_init   (LigmaContainerViewInterface  *iface);

static void          ligma_container_tree_view_constructed       (GObject                     *object);
static void          ligma_container_tree_view_finalize          (GObject                     *object);

static void          ligma_container_tree_view_style_updated     (GtkWidget                   *widget);
static void          ligma_container_tree_view_unmap             (GtkWidget                   *widget);
static gboolean      ligma_container_tree_view_popup_menu        (GtkWidget                   *widget);

static void          ligma_container_tree_view_set_container     (LigmaContainerView           *view,
                                                                 LigmaContainer               *container);
static void          ligma_container_tree_view_set_context       (LigmaContainerView           *view,
                                                                 LigmaContext                 *context);
static void          ligma_container_tree_view_set_selection_mode(LigmaContainerView           *view,
                                                                 GtkSelectionMode             mode);

static gpointer      ligma_container_tree_view_insert_item       (LigmaContainerView           *view,
                                                                 LigmaViewable                *viewable,
                                                                 gpointer                     parent_insert_data,
                                                                 gint                         index);
static void          ligma_container_tree_view_remove_item       (LigmaContainerView           *view,
                                                                 LigmaViewable                *viewable,
                                                                 gpointer                     insert_data);
static void          ligma_container_tree_view_reorder_item      (LigmaContainerView           *view,
                                                                 LigmaViewable                *viewable,
                                                                 gint                         new_index,
                                                                 gpointer                     insert_data);
static void          ligma_container_tree_view_rename_item       (LigmaContainerView           *view,
                                                                 LigmaViewable                *viewable,
                                                                 gpointer                     insert_data);
static void          ligma_container_tree_view_expand_item       (LigmaContainerView           *view,
                                                                 LigmaViewable                *viewable,
                                                                 gpointer                     insert_data);
static gboolean      ligma_container_tree_view_select_items      (LigmaContainerView           *view,
                                                                 GList                       *viewables,
                                                                 GList                       *paths);
static void          ligma_container_tree_view_clear_items       (LigmaContainerView           *view);
static void          ligma_container_tree_view_set_view_size     (LigmaContainerView           *view);

static void          ligma_container_tree_view_real_edit_name    (LigmaContainerTreeView       *tree_view);

static void     ligma_container_tree_view_selection_label_notify (GtkLabel                    *label,
                                                                 GParamSpec                  *pspec,
                                                                 LigmaItemTreeView            *view);

static gboolean      ligma_container_tree_view_edit_focus_out    (GtkWidget                   *widget,
                                                                 GdkEvent                    *event,
                                                                 gpointer                     user_data);
static void          ligma_container_tree_view_name_started      (GtkCellRendererText         *cell,
                                                                 GtkCellEditable             *editable,
                                                                 const gchar                 *path_str,
                                                                 LigmaContainerTreeView       *tree_view);
static void          ligma_container_tree_view_name_canceled     (GtkCellRendererText         *cell,
                                                                 LigmaContainerTreeView       *tree_view);

static void          ligma_container_tree_view_selection_changed (GtkTreeSelection            *sel,
                                                                 LigmaContainerTreeView       *tree_view);
static gboolean      ligma_container_tree_view_button            (GtkWidget                   *widget,
                                                                 GdkEventButton              *bevent,
                                                                 LigmaContainerTreeView       *tree_view);
static gboolean      ligma_container_tree_view_scroll            (GtkWidget                   *widget,
                                                                 GdkEventScroll              *event,
                                                                 LigmaContainerTreeView       *tree_view);
static gboolean      ligma_container_tree_view_tooltip           (GtkWidget                   *widget,
                                                                 gint                         x,
                                                                 gint                         y,
                                                                 gboolean                     keyboard_tip,
                                                                 GtkTooltip                  *tooltip,
                                                                 LigmaContainerTreeView       *tree_view);
static LigmaViewable *ligma_container_tree_view_drag_viewable     (GtkWidget                   *widget,
                                                                 LigmaContext                **context,
                                                                 gpointer                     data);
static GList       * ligma_container_tree_view_drag_viewable_list (GtkWidget    *widget,
                                                                   LigmaContext **context,
                                                                   gpointer      data);
static GdkPixbuf    *ligma_container_tree_view_drag_pixbuf       (GtkWidget                   *widget,
                                                                 gpointer                     data);
static void          ligma_container_tree_view_zoom_gesture_begin  (GtkGestureZoom            *gesture,
                                                                   GdkEventSequence          *sequence,
                                                                   LigmaContainerTreeView     *tree_view);
static void          ligma_container_tree_view_zoom_gesture_update (GtkGestureZoom            *gesture,
                                                                   GdkEventSequence          *sequence,
                                                                   LigmaContainerTreeView     *tree_view);

static gboolean      ligma_container_tree_view_get_selected_single (LigmaContainerTreeView  *tree_view,
                                                                   GtkTreeIter            *iter);
static gint          ligma_container_tree_view_get_selected        (LigmaContainerView    *view,
                                                                   GList               **items,
                                                                   GList               **paths);
static void          ligma_container_tree_view_row_expanded        (GtkTreeView               *tree_view,
                                                                   GtkTreeIter               *iter,
                                                                   GtkTreePath               *path,
                                                                   LigmaContainerTreeView     *view);
static void          ligma_container_tree_view_expand_rows         (GtkTreeModel             *model,
                                                                   GtkTreeView              *view,
                                                                   GtkTreeIter              *parent);

static void          ligma_container_tree_view_monitor_changed     (LigmaContainerTreeView    *view);

static gboolean      ligma_container_tree_view_search_path_foreach (GtkTreeModel             *model,
                                                                   GtkTreePath              *path,
                                                                   GtkTreeIter              *iter,
                                                                   gpointer                  data);
static GtkTreePath * ligma_container_tree_view_get_path            (LigmaContainerTreeView    *tree_view,
                                                                   LigmaViewable             *viewable);



G_DEFINE_TYPE_WITH_CODE (LigmaContainerTreeView, ligma_container_tree_view,
                         LIGMA_TYPE_CONTAINER_BOX,
                         G_ADD_PRIVATE (LigmaContainerTreeView)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONTAINER_VIEW,
                                                ligma_container_tree_view_view_iface_init))

#define parent_class ligma_container_tree_view_parent_class

static LigmaContainerViewInterface *parent_view_iface = NULL;

static guint tree_view_signals[LAST_SIGNAL] = { 0 };


static void
ligma_container_tree_view_class_init (LigmaContainerTreeViewClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkBindingSet  *binding_set;

  object_class->constructed   = ligma_container_tree_view_constructed;
  object_class->finalize      = ligma_container_tree_view_finalize;

  widget_class->style_updated = ligma_container_tree_view_style_updated;
  widget_class->unmap         = ligma_container_tree_view_unmap;
  widget_class->popup_menu    = ligma_container_tree_view_popup_menu;

  klass->edit_name            = ligma_container_tree_view_real_edit_name;
  klass->drop_possible        = ligma_container_tree_view_real_drop_possible;
  klass->drop_viewables       = ligma_container_tree_view_real_drop_viewables;
  klass->drop_color           = NULL;
  klass->drop_uri_list        = NULL;
  klass->drop_svg             = NULL;
  klass->drop_component       = NULL;
  klass->drop_pixbuf          = NULL;

  tree_view_signals[EDIT_NAME] =
    g_signal_new ("edit-name",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (LigmaContainerTreeViewClass, edit_name),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  binding_set = gtk_binding_set_by_class (klass);

  gtk_binding_entry_add_signal (binding_set, GDK_KEY_F2, 0,
                                "edit-name", 0);
}

static void
ligma_container_tree_view_view_iface_init (LigmaContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  if (! parent_view_iface)
    parent_view_iface = g_type_default_interface_peek (LIGMA_TYPE_CONTAINER_VIEW);

  iface->set_container      = ligma_container_tree_view_set_container;
  iface->set_context        = ligma_container_tree_view_set_context;
  iface->set_selection_mode = ligma_container_tree_view_set_selection_mode;
  iface->insert_item        = ligma_container_tree_view_insert_item;
  iface->remove_item        = ligma_container_tree_view_remove_item;
  iface->reorder_item       = ligma_container_tree_view_reorder_item;
  iface->rename_item        = ligma_container_tree_view_rename_item;
  iface->expand_item        = ligma_container_tree_view_expand_item;
  iface->select_items       = ligma_container_tree_view_select_items;
  iface->clear_items        = ligma_container_tree_view_clear_items;
  iface->set_view_size      = ligma_container_tree_view_set_view_size;
  iface->get_selected       = ligma_container_tree_view_get_selected;

  iface->insert_data_free = (GDestroyNotify) gtk_tree_iter_free;
}

static void
ligma_container_tree_view_init (LigmaContainerTreeView *tree_view)
{
  LigmaContainerBox *box = LIGMA_CONTAINER_BOX (tree_view);

  tree_view->priv = ligma_container_tree_view_get_instance_private (tree_view);

  ligma_container_tree_store_columns_init (tree_view->model_columns,
                                          &tree_view->n_model_columns);

  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (box->scrolled_win),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (box->scrolled_win),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  ligma_widget_track_monitor (GTK_WIDGET (tree_view),
                             G_CALLBACK (ligma_container_tree_view_monitor_changed),
                             NULL, NULL);
}

static void
ligma_container_tree_view_constructed (GObject *object)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (object);
  LigmaContainerView     *view      = LIGMA_CONTAINER_VIEW (object);
  LigmaContainerBox      *box       = LIGMA_CONTAINER_BOX (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  tree_view->model = ligma_container_tree_store_new (view,
                                                    tree_view->n_model_columns,
                                                    tree_view->model_columns);

  tree_view->view = g_object_new (GTK_TYPE_TREE_VIEW,
                                  "model",           tree_view->model,
                                  "search-column",   LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME,
                                  "enable-search",   FALSE,
                                  "headers-visible", FALSE,
                                  "has-tooltip",     TRUE,
                                  "show-expanders",  LIGMA_CONTAINER_VIEW_GET_IFACE (view)->model_is_tree,
                                  NULL);

  gtk_container_add (GTK_CONTAINER (box->scrolled_win),
                     GTK_WIDGET (tree_view->view));
  gtk_widget_show (GTK_WIDGET (tree_view->view));

  ligma_container_view_set_dnd_widget (view, GTK_WIDGET (tree_view->view));

  tree_view->main_column = gtk_tree_view_column_new ();
  tree_view->priv->multi_selection_label = gtk_label_new (NULL);
  gtk_label_set_selectable (GTK_LABEL (tree_view->priv->multi_selection_label), TRUE);
  gtk_tree_view_column_set_widget (tree_view->main_column,
                                   tree_view->priv->multi_selection_label);
  g_signal_connect (tree_view->priv->multi_selection_label, "notify::label",
                    G_CALLBACK (ligma_container_tree_view_selection_label_notify),
                    tree_view);
  g_signal_connect (tree_view->priv->multi_selection_label, "notify::selection-bound",
                    G_CALLBACK (ligma_container_tree_view_selection_label_notify),
                    tree_view);
  gtk_widget_show (tree_view->priv->multi_selection_label);
  gtk_tree_view_insert_column (tree_view->view, tree_view->main_column, 0);

  gtk_tree_view_set_expander_column (tree_view->view, tree_view->main_column);
  gtk_tree_view_set_enable_tree_lines (tree_view->view, TRUE);

  tree_view->renderer_cell = ligma_cell_renderer_viewable_new ();
  gtk_tree_view_column_pack_start (tree_view->main_column,
                                   tree_view->renderer_cell,
                                   FALSE);

  gtk_tree_view_column_set_attributes (tree_view->main_column,
                                       tree_view->renderer_cell,
                                       "renderer", LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER,
                                       NULL);

  tree_view->priv->name_cell = gtk_cell_renderer_text_new ();
  g_object_set (tree_view->priv->name_cell, "xalign", 0.0, NULL);
  gtk_tree_view_column_pack_end (tree_view->main_column,
                                 tree_view->priv->name_cell,
                                 FALSE);

  gtk_tree_view_column_set_attributes (tree_view->main_column,
                                       tree_view->priv->name_cell,
                                       "text",       LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME,
                                       "attributes", LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME_ATTRIBUTES,
                                       "sensitive",  LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME_SENSITIVE,
                                       NULL);

  g_signal_connect (tree_view->priv->name_cell, "editing-started",
                    G_CALLBACK (ligma_container_tree_view_name_started),
                    tree_view);
  g_signal_connect (tree_view->priv->name_cell, "editing-canceled",
                    G_CALLBACK (ligma_container_tree_view_name_canceled),
                    tree_view);

  ligma_container_tree_view_add_renderer_cell (tree_view,
                                              tree_view->renderer_cell,
                                              LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER);

  tree_view->priv->selection = gtk_tree_view_get_selection (tree_view->view);

  g_signal_connect (tree_view->priv->selection, "changed",
                    G_CALLBACK (ligma_container_tree_view_selection_changed),
                    tree_view);

  g_signal_connect (tree_view->view, "drag-failed",
                    G_CALLBACK (ligma_container_tree_view_drag_failed),
                    tree_view);
  g_signal_connect (tree_view->view, "drag-leave",
                    G_CALLBACK (ligma_container_tree_view_drag_leave),
                    tree_view);
  g_signal_connect (tree_view->view, "drag-motion",
                    G_CALLBACK (ligma_container_tree_view_drag_motion),
                    tree_view);
  g_signal_connect (tree_view->view, "drag-drop",
                    G_CALLBACK (ligma_container_tree_view_drag_drop),
                    tree_view);
  g_signal_connect (tree_view->view, "drag-data-received",
                    G_CALLBACK (ligma_container_tree_view_drag_data_received),
                    tree_view);
  g_signal_connect (tree_view->view, "scroll-event",
                    G_CALLBACK (ligma_container_tree_view_scroll),
                    tree_view);

  tree_view->priv->zoom_gesture = gtk_gesture_zoom_new (GTK_WIDGET (tree_view->view));
  gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (tree_view->priv->zoom_gesture),
                                              GTK_PHASE_CAPTURE);

  /* The default signal handler needs to run first to setup scale delta */
  g_signal_connect_after (tree_view->priv->zoom_gesture, "begin",
                          G_CALLBACK (ligma_container_tree_view_zoom_gesture_begin),
                          tree_view);
  g_signal_connect_after (tree_view->priv->zoom_gesture, "update",
                          G_CALLBACK (ligma_container_tree_view_zoom_gesture_update),
                          tree_view);

  /* connect_after so external code can connect to "query-tooltip" too
   * and override the default tip
   */
  g_signal_connect_after (tree_view->view, "query-tooltip",
                          G_CALLBACK (ligma_container_tree_view_tooltip),
                          tree_view);
}

static void
ligma_container_tree_view_finalize (GObject *object)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (object);

  g_clear_object (&tree_view->model);

  if (tree_view->priv->toggle_cells)
    {
      g_list_free (tree_view->priv->toggle_cells);
      tree_view->priv->toggle_cells = NULL;
    }

  g_list_free (tree_view->priv->renderer_cells);

  if (tree_view->priv->editable_cells)
    {
      g_list_free (tree_view->priv->editable_cells);
      tree_view->priv->editable_cells = NULL;
    }

  g_clear_pointer (&tree_view->priv->editing_path, g_free);
  g_clear_object (&tree_view->priv->zoom_gesture);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
ligma_container_tree_view_style_updated_foreach (GtkTreeModel *model,
                                                GtkTreePath  *path,
                                                GtkTreeIter  *iter,
                                                gpointer      data)
{
  LigmaViewRenderer *renderer;

  renderer = ligma_container_tree_store_get_renderer (LIGMA_CONTAINER_TREE_STORE (model), iter);

  if (renderer)
    {
      ligma_view_renderer_invalidate (renderer);
      g_object_unref (renderer);
    }

  return FALSE;
}

static void
ligma_container_tree_view_style_updated (GtkWidget *widget)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (widget);

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  if (tree_view->model)
    gtk_tree_model_foreach (tree_view->model,
                            ligma_container_tree_view_style_updated_foreach,
                            NULL);
}

static void
ligma_container_tree_view_unmap (GtkWidget *widget)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (widget);

  if (tree_view->priv->scroll_timeout_id)
    {
      g_source_remove (tree_view->priv->scroll_timeout_id);
      tree_view->priv->scroll_timeout_id = 0;
    }

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static gboolean
ligma_container_tree_view_popup_menu (GtkWidget *widget)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (widget);
  GtkTreeIter            iter;
  GtkTreePath           *path;
  GdkRectangle           rect;

  if (!ligma_container_tree_view_get_selected_single (tree_view, &iter))
    return FALSE;

  path = gtk_tree_model_get_path (tree_view->model, &iter);
  gtk_tree_view_get_cell_area (tree_view->view, path,
                               tree_view->main_column, &rect);
  gtk_tree_view_convert_bin_window_to_widget_coords (tree_view->view,
                                                     rect.x, rect.y,
                                                     &rect.x, &rect.y);
  gtk_tree_path_free (path);

  return ligma_editor_popup_menu_at_rect (LIGMA_EDITOR (widget),
                                         gtk_tree_view_get_bin_window (tree_view->view),
                                         &rect, GDK_GRAVITY_CENTER, GDK_GRAVITY_NORTH_WEST,
                                         NULL);
}

GtkWidget *
ligma_container_tree_view_new (LigmaContainer *container,
                              LigmaContext   *context,
                              gint           view_size,
                              gint           view_border_width)
{
  LigmaContainerTreeView *tree_view;
  LigmaContainerView     *view;

  g_return_val_if_fail (container == NULL || LIGMA_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (context == NULL || LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= LIGMA_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= LIGMA_VIEW_MAX_BORDER_WIDTH,
                        NULL);

  tree_view = g_object_new (LIGMA_TYPE_CONTAINER_TREE_VIEW, NULL);

  view = LIGMA_CONTAINER_VIEW (tree_view);

  ligma_container_view_set_view_size (view, view_size, view_border_width);

  if (container)
    ligma_container_view_set_container (view, container);

  if (context)
    ligma_container_view_set_context (view, context);

  return GTK_WIDGET (tree_view);
}

GtkCellRenderer *
ligma_container_tree_view_get_name_cell (LigmaContainerTreeView *tree_view)
{
  g_return_val_if_fail (LIGMA_IS_CONTAINER_TREE_VIEW (tree_view), NULL);

  return tree_view->priv->name_cell;
}

void
ligma_container_tree_view_set_main_column_title (LigmaContainerTreeView *tree_view,
                                                const gchar           *title)
{
  g_return_if_fail (LIGMA_IS_CONTAINER_TREE_VIEW (tree_view));

  gtk_tree_view_column_set_title (tree_view->main_column,
                                  title);
}

void
ligma_container_tree_view_add_toggle_cell (LigmaContainerTreeView *tree_view,
                                          GtkCellRenderer       *cell)
{
  g_return_if_fail (LIGMA_IS_CONTAINER_TREE_VIEW (tree_view));
  g_return_if_fail (LIGMA_IS_CELL_RENDERER_TOGGLE (cell) ||
                    LIGMA_IS_CELL_RENDERER_BUTTON (cell));

  tree_view->priv->toggle_cells = g_list_prepend (tree_view->priv->toggle_cells,
                                                  cell);
}

void
ligma_container_tree_view_add_renderer_cell (LigmaContainerTreeView *tree_view,
                                            GtkCellRenderer       *cell,
                                            gint                   column_number)
{
  g_return_if_fail (LIGMA_IS_CONTAINER_TREE_VIEW (tree_view));
  g_return_if_fail (LIGMA_IS_CELL_RENDERER_VIEWABLE (cell));

  tree_view->priv->renderer_cells = g_list_prepend (tree_view->priv->renderer_cells,
                                                    cell);

  ligma_container_tree_store_add_renderer_cell (LIGMA_CONTAINER_TREE_STORE (tree_view->model),
                                               cell, column_number);
}

void
ligma_container_tree_view_set_dnd_drop_to_empty (LigmaContainerTreeView *tree_view,
                                                gboolean               dnd_drop_to_empty)
{
  g_return_if_fail (LIGMA_IS_CONTAINER_TREE_VIEW (tree_view));

  tree_view->priv->dnd_drop_to_empty = dnd_drop_to_empty;
}

void
ligma_container_tree_view_connect_name_edited (LigmaContainerTreeView *tree_view,
                                              GCallback              callback,
                                              gpointer               data)
{
  g_return_if_fail (LIGMA_IS_CONTAINER_TREE_VIEW (tree_view));
  g_return_if_fail (callback != NULL);

  g_object_set (tree_view->priv->name_cell,
                "mode",     GTK_CELL_RENDERER_MODE_EDITABLE,
                "editable", TRUE,
                NULL);

  if (! g_list_find (tree_view->priv->editable_cells, tree_view->priv->name_cell))
    tree_view->priv->editable_cells = g_list_prepend (tree_view->priv->editable_cells,
                                                      tree_view->priv->name_cell);

  g_signal_connect (tree_view->priv->name_cell, "edited",
                    callback,
                    data);
}

gboolean
ligma_container_tree_view_name_edited (GtkCellRendererText   *cell,
                                      const gchar           *path_str,
                                      const gchar           *new_name,
                                      LigmaContainerTreeView *tree_view)
{
  GtkTreePath *path;
  GtkTreeIter  iter;
  gboolean     changed = FALSE;

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      LigmaViewRenderer *renderer;
      LigmaObject       *object;
      const gchar      *old_name;

      renderer = ligma_container_tree_store_get_renderer (LIGMA_CONTAINER_TREE_STORE (tree_view->model), &iter);

      object = LIGMA_OBJECT (renderer->viewable);

      old_name = ligma_object_get_name (object);

      if (! old_name) old_name = "";
      if (! new_name) new_name = "";

      if (strcmp (old_name, new_name))
        {
          ligma_object_set_name (object, new_name);

          changed = TRUE;
        }
      else
        {
          gchar *name = ligma_viewable_get_description (renderer->viewable,
                                                       NULL);

          gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), &iter,
                              LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME, name,
                              -1);
          g_free (name);
        }

      g_object_unref (renderer);
    }

  gtk_tree_path_free (path);

  return changed;
}


/*  LigmaContainerView methods  */

static void
ligma_container_tree_view_set_container (LigmaContainerView *view,
                                        LigmaContainer     *container)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (view);
  LigmaContainer         *old_container;

  old_container = ligma_container_view_get_container (view);

  if (old_container)
    {
      tree_view->priv->dnd_renderer = NULL;

      g_signal_handlers_disconnect_by_func (tree_view->view,
                                            ligma_container_tree_view_row_expanded,
                                            tree_view);
      if (! container)
        {
          if (ligma_dnd_viewable_list_source_remove (GTK_WIDGET (tree_view->view),
                                                    ligma_container_get_children_type (old_container)))
            {
              if (LIGMA_VIEWABLE_CLASS (g_type_class_peek (ligma_container_get_children_type (old_container)))->get_size)
                ligma_dnd_pixbuf_source_remove (GTK_WIDGET (tree_view->view));

              gtk_drag_source_unset (GTK_WIDGET (tree_view->view));
            }

          g_signal_handlers_disconnect_by_func (tree_view->view,
                                                ligma_container_tree_view_button,
                                                tree_view);
        }
    }
  else if (container)
    {
      if (ligma_dnd_drag_source_set_by_type (GTK_WIDGET (tree_view->view),
                                            GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                                            ligma_container_get_children_type (container),
                                            GDK_ACTION_COPY))
        {
          ligma_dnd_viewable_list_source_add (GTK_WIDGET (tree_view->view),
                                             ligma_container_get_children_type (container),
                                             ligma_container_tree_view_drag_viewable_list,
                                             tree_view);
          ligma_dnd_viewable_source_add (GTK_WIDGET (tree_view->view),
                                        ligma_container_get_children_type (container),
                                        ligma_container_tree_view_drag_viewable,
                                        tree_view);

          if (LIGMA_VIEWABLE_CLASS (g_type_class_peek (ligma_container_get_children_type (container)))->get_size)
            ligma_dnd_pixbuf_source_add (GTK_WIDGET (tree_view->view),
                                        ligma_container_tree_view_drag_pixbuf,
                                        tree_view);
        }

      /*  connect button_press_event after DND so we can keep the list from
       *  selecting the item on button2
       */
      g_signal_connect (tree_view->view, "button-press-event",
                        G_CALLBACK (ligma_container_tree_view_button),
                        tree_view);
      g_signal_connect (tree_view->view, "button-release-event",
                        G_CALLBACK (ligma_container_tree_view_button),
                        tree_view);
    }

  parent_view_iface->set_container (view, container);

  if (container)
    {
      ligma_container_tree_view_expand_rows (tree_view->model,
                                            tree_view->view,
                                            NULL);

      g_signal_connect (tree_view->view,
                        "row-collapsed",
                        G_CALLBACK (ligma_container_tree_view_row_expanded),
                        tree_view);
      g_signal_connect (tree_view->view,
                        "row-expanded",
                        G_CALLBACK (ligma_container_tree_view_row_expanded),
                        tree_view);
    }

  gtk_tree_view_columns_autosize (tree_view->view);
}

static void
ligma_container_tree_view_set_context (LigmaContainerView *view,
                                      LigmaContext       *context)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (view);

  if (tree_view->model)
    ligma_container_tree_store_set_context (LIGMA_CONTAINER_TREE_STORE (tree_view->model),
                                           context);

  parent_view_iface->set_context (view, context);
}

static void
ligma_container_tree_view_set_selection_mode (LigmaContainerView *view,
                                             GtkSelectionMode   mode)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (view);

  gtk_tree_selection_set_mode (tree_view->priv->selection, mode);

  parent_view_iface->set_selection_mode (view, mode);
}

static gpointer
ligma_container_tree_view_insert_item (LigmaContainerView *view,
                                      LigmaViewable      *viewable,
                                      gpointer           parent_insert_data,
                                      gint               index)
{
  LigmaContainerTreeView *tree_view   = LIGMA_CONTAINER_TREE_VIEW (view);
  GtkTreeIter           *parent_iter = parent_insert_data;
  GtkTreeIter           *iter;

  iter = ligma_container_tree_store_insert_item (LIGMA_CONTAINER_TREE_STORE (tree_view->model),
                                                viewable,
                                                parent_iter,
                                                index);

  if (parent_iter)
    ligma_container_tree_view_expand_item (view, viewable, parent_iter);

  return iter;
}

static void
ligma_container_tree_view_remove_item (LigmaContainerView *view,
                                      LigmaViewable      *viewable,
                                      gpointer           insert_data)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (view);
  GtkTreeIter           *iter      = (GtkTreeIter *) insert_data;

  ligma_container_tree_store_remove_item (LIGMA_CONTAINER_TREE_STORE (tree_view->model),
                                         viewable,
                                         iter);

  if (iter)
    gtk_tree_view_columns_autosize (tree_view->view);
}

static void
ligma_container_tree_view_reorder_item (LigmaContainerView *view,
                                       LigmaViewable      *viewable,
                                       gint               new_index,
                                       gpointer           insert_data)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (view);
  GtkTreeIter           *iter      = (GtkTreeIter *) insert_data;
  GtkTreeIter            parent_iter;
  gboolean               selected  = FALSE;

  if (iter)
    {
      GtkTreeIter selected_iter;

      selected = ligma_container_tree_view_get_selected_single (tree_view,
                                                               &selected_iter);

      if (selected)
        {
          LigmaViewRenderer *renderer;

          renderer = ligma_container_tree_store_get_renderer (LIGMA_CONTAINER_TREE_STORE (tree_view->model),
                                                             &selected_iter);

          if (renderer->viewable != viewable)
            selected = FALSE;

          g_object_unref (renderer);
        }
    }

  ligma_container_tree_store_reorder_item (LIGMA_CONTAINER_TREE_STORE (tree_view->model),
                                          viewable,
                                          new_index,
                                          iter);

  if (selected)
    ligma_container_view_select_item (view, viewable);

  if (gtk_tree_model_iter_parent (tree_view->model, &parent_iter, iter))
    ligma_container_tree_view_expand_item (view, viewable, &parent_iter);
}

static void
ligma_container_tree_view_rename_item (LigmaContainerView *view,
                                      LigmaViewable      *viewable,
                                      gpointer           insert_data)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (view);
  GtkTreeIter           *iter      = (GtkTreeIter *) insert_data;

  if (ligma_container_tree_store_rename_item (LIGMA_CONTAINER_TREE_STORE (tree_view->model),
                                             viewable,
                                             iter))
    {
      gtk_tree_view_columns_autosize (tree_view->view);
    }
}

static void
ligma_container_tree_view_expand_item (LigmaContainerView *view,
                                      LigmaViewable      *viewable,
                                      gpointer           insert_data)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (view);
  GtkTreeIter           *iter      = (GtkTreeIter *) insert_data;
  LigmaViewRenderer      *renderer;

  renderer = ligma_container_tree_store_get_renderer (LIGMA_CONTAINER_TREE_STORE (tree_view->model), iter);

  if (renderer)
    {
      GtkTreePath *path = gtk_tree_model_get_path (tree_view->model, iter);

      g_signal_handlers_block_by_func (tree_view,
                                       ligma_container_tree_view_row_expanded,
                                       view);

      if (ligma_viewable_get_expanded (renderer->viewable))
        gtk_tree_view_expand_row (tree_view->view, path, FALSE);
      else
        gtk_tree_view_collapse_row (tree_view->view, path);

      g_signal_handlers_unblock_by_func (tree_view,
                                         ligma_container_tree_view_row_expanded,
                                         view);

      gtk_tree_path_free (path);
      g_object_unref (renderer);
    }
}

static gboolean
ligma_container_tree_view_select_items (LigmaContainerView *view,
                                       GList             *items,
                                       GList             *paths)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (view);
  GList                 *item;
  GList                 *path;
  gboolean               free_paths      = FALSE;
  gboolean               scroll_to_first = TRUE;

  /* If @paths is not set, compute it ourselves. */
  if (g_list_length (items) != g_list_length (paths))
    {
      paths = NULL;
      for (item = items; item; item = item->next)
        {
          GtkTreePath *path;
          path = ligma_container_tree_view_get_path (tree_view, item->data);
          if (path != NULL)
            /* It may happen that some items have no paths when a tree
             * view has some filtering logics (for instance Palette or
             * Fonts dockables). Then an item which was selected at
             * first might become unselected during filtering and has to
             * be removed from selection.
             */
            paths = g_list_prepend (paths, path);
        }

      paths = g_list_reverse (paths);
      free_paths = TRUE;
    }

  g_signal_handlers_block_by_func (tree_view->priv->selection,
                                   ligma_container_tree_view_selection_changed,
                                   tree_view);
  gtk_tree_selection_unselect_all (tree_view->priv->selection);
  for (item = items, path = paths; item && path; item = item->next, path = path->next)
    {
      GtkTreePath *parent_path;

      /* Expand if necessary. */
      parent_path = gtk_tree_path_copy (path->data);
      if (gtk_tree_path_up (parent_path))
        gtk_tree_view_expand_to_path (tree_view->view, parent_path);
      gtk_tree_path_free (parent_path);

      /* Add to the selection. */
      gtk_tree_selection_select_path (tree_view->priv->selection, path->data);
    }
  g_signal_handlers_unblock_by_func (tree_view->priv->selection,
                                     ligma_container_tree_view_selection_changed,
                                     tree_view);

  if (paths)
    {
      GtkTreePath *first;
      GtkTreePath *last;

      /* Scroll to the top item if and only if none of the selected
       * items are already visible. Do nothing otherwise.
       */
      if (gtk_tree_view_get_visible_range (tree_view->view, &first, &last))
        {
          for (item = items, path = paths; item && path; item = item->next, path = path->next)
            {
              if (gtk_tree_path_compare (first, path->data) <= 0 &&
                  gtk_tree_path_compare (path->data, last) <= 0)
                {
                  scroll_to_first = FALSE;
                  break;
                }
            }

          gtk_tree_path_free (first);
          gtk_tree_path_free (last);
        }

      if (scroll_to_first)
        gtk_tree_view_scroll_to_cell (tree_view->view, paths->data,
                                      NULL, FALSE, 0.0, 0.0);
    }

  if (free_paths)
    g_list_free_full (paths, (GDestroyNotify) gtk_tree_path_free);

  if (g_list_length (items) > 1)
    {
      gchar *str;

      str = g_strdup_printf (ngettext ("%d item selected", "%d items selected",
                                       g_list_length (items)),
                             g_list_length (items));
      gtk_label_set_text (GTK_LABEL (tree_view->priv->multi_selection_label), str);
      g_free (str);
      gtk_widget_show (tree_view->priv->multi_selection_label);
    }
  else
    {
      gtk_widget_hide (tree_view->priv->multi_selection_label);
    }

  return TRUE;
}

static void
ligma_container_tree_view_clear_items (LigmaContainerView *view)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (view);

  g_signal_handlers_block_by_func (tree_view->priv->selection,
                                   ligma_container_tree_view_selection_changed,
                                   tree_view);

  /* temporarily unset the tree-view's model, so that name editing is stopped
   * now, before clearing the tree store.  otherwise, name editing would stop
   * when the corresponding item is removed from the store, leading us to
   * rename the wrong item.  see issue #3284.
   */
  gtk_tree_view_set_model (tree_view->view, NULL);

  ligma_container_tree_store_clear_items (LIGMA_CONTAINER_TREE_STORE (tree_view->model));

  gtk_tree_view_set_model (tree_view->view, tree_view->model);

  g_signal_handlers_unblock_by_func (tree_view->priv->selection,
                                     ligma_container_tree_view_selection_changed,
                                     tree_view);

  parent_view_iface->clear_items (view);
}

static void
ligma_container_tree_view_set_view_size (LigmaContainerView *view)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (view);
  GtkWidget             *tree_widget;
  GList                 *list;
  gint                   view_size;
  gint                   border_width;

  view_size = ligma_container_view_get_view_size (view, &border_width);

  if (tree_view->model)
    ligma_container_tree_store_set_view_size (LIGMA_CONTAINER_TREE_STORE (tree_view->model));

  tree_widget = GTK_WIDGET (tree_view->view);

  if (! tree_widget)
    return;

  for (list = tree_view->priv->toggle_cells; list; list = g_list_next (list))
    {
      gchar *icon_name;
      gint   icon_size;

      g_object_get (list->data, "icon-name", &icon_name, NULL);

      if (icon_name)
        {
          GtkStyleContext *style = gtk_widget_get_style_context (tree_widget);
          GtkBorder        border;

          gtk_style_context_save (style);
          gtk_style_context_add_class (style, GTK_STYLE_CLASS_BUTTON);
          gtk_style_context_get_border (style, 0, &border);
          gtk_style_context_restore (style);

          g_object_get (list->data, "icon-size", &icon_size, NULL);
          icon_size = MIN (icon_size, MAX (view_size - (border.left + border.right),
                                           view_size - (border.top + border.bottom)));
          g_object_set (list->data, "icon-size", icon_size, NULL);

          g_free (icon_name);
        }
    }

  gtk_tree_view_columns_autosize (tree_view->view);
}


/*  LigmaContainerTreeView methods  */

static void
ligma_container_tree_view_real_edit_name (LigmaContainerTreeView *tree_view)
{
  GtkTreeIter selected_iter;
  gboolean    success = FALSE;

  if (g_list_find (tree_view->priv->editable_cells,
                   tree_view->priv->name_cell) &&
      ligma_container_tree_view_get_selected_single (tree_view,
                                                    &selected_iter))
    {
      LigmaViewRenderer *renderer;

      renderer = ligma_container_tree_store_get_renderer (LIGMA_CONTAINER_TREE_STORE (tree_view->model),
                                                         &selected_iter);

      if (ligma_viewable_is_name_editable (renderer->viewable))
        {
          GtkTreePath *path;

          path = gtk_tree_model_get_path (tree_view->model, &selected_iter);

          gtk_tree_view_set_cursor_on_cell (tree_view->view, path,
                                            tree_view->main_column,
                                            tree_view->priv->name_cell,
                                            TRUE);

          gtk_tree_path_free (path);

          success = TRUE;
        }

      g_object_unref (renderer);
    }

  if (! success)
    gtk_widget_error_bell (GTK_WIDGET (tree_view));
}


/*  callbacks  */

static void
ligma_container_tree_view_selection_label_notify (GtkLabel         *label,
                                                 GParamSpec       *pspec,
                                                 LigmaItemTreeView *view)
{
  /* This is a weird trick to make the label follow the color scheme of
   * selected items in whatever theme is selected. It seems we cannot
   * link to the color of another widget whose theme we don't control.
   * Faking selection is the only way I found, though it is quite ugly
   * semantically.
   */
  gtk_label_select_region (label, 0, -1);
}

static gboolean
ligma_container_tree_view_edit_focus_out (GtkWidget *widget,
                                         GdkEvent  *event,
                                         gpointer   user_data)
{
  LigmaContainerTreeView *tree_view = user_data;

  /*  When focusing out of a tree view, we want its content to be
   *  updated as though it had been activated.
   */
  g_clear_pointer (&tree_view->priv->editing_path, g_free);
  g_signal_emit_by_name (widget, "activate", 0);

  return TRUE;
}

static void
ligma_container_tree_view_name_started (GtkCellRendererText   *cell,
                                       GtkCellEditable       *editable,
                                       const gchar           *path_str,
                                       LigmaContainerTreeView *tree_view)
{
  GtkTreePath *path;
  GtkTreeIter  iter;

  path = gtk_tree_path_new_from_string (path_str);

  tree_view->priv->editing_path = g_strdup (path_str);

  g_signal_connect (GTK_ENTRY (editable), "focus-out-event",
                    G_CALLBACK (ligma_container_tree_view_edit_focus_out),
                    tree_view);

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      LigmaViewRenderer *renderer;
      const gchar      *real_name;

      renderer = ligma_container_tree_store_get_renderer (LIGMA_CONTAINER_TREE_STORE (tree_view->model), &iter);

      real_name = ligma_object_get_name (renderer->viewable);
      ligma_view_renderer_remove_idle (renderer);

      g_object_unref (renderer);

      gtk_entry_set_text (GTK_ENTRY (editable), real_name);
    }

  gtk_tree_path_free (path);
}

static void
ligma_container_tree_view_name_canceled (GtkCellRendererText   *cell,
                                        LigmaContainerTreeView *tree_view)
{
  GtkTreeIter iter;

  if (ligma_container_tree_view_get_selected_single (tree_view, &iter))
    {
      LigmaViewRenderer *renderer;
      gchar            *name;

      renderer = ligma_container_tree_store_get_renderer (LIGMA_CONTAINER_TREE_STORE (tree_view->model), &iter);

      name = ligma_viewable_get_description (renderer->viewable, NULL);

      gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), &iter,
                          LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME, name,
                          -1);

      g_free (name);
      g_object_unref (renderer);
    }
}

static void
ligma_container_tree_view_selection_changed (GtkTreeSelection      *selection,
                                            LigmaContainerTreeView *tree_view)
{
  LigmaContainerView *view = LIGMA_CONTAINER_VIEW (tree_view);
  GList             *items;
  GList             *paths;

  ligma_container_tree_view_get_selected (view, &items, &paths);
  ligma_container_view_multi_selected (view, items, paths);
  g_list_free (items);
  g_list_free_full (paths, (GDestroyNotify) gtk_tree_path_free);
}

static GtkCellRenderer *
ligma_container_tree_view_find_click_cell (GtkWidget         *widget,
                                          GList             *cells,
                                          GtkTreeViewColumn *column,
                                          GdkRectangle      *column_area,
                                          gint               tree_x,
                                          gint               tree_y)
{
  GList    *list;
  gboolean  rtl = (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL);

  for (list = cells; list; list = g_list_next (list))
    {
      GtkCellRenderer *renderer = list->data;
      gint             start;
      gint             width;

      if (gtk_cell_renderer_get_visible (renderer) &&
          gtk_tree_view_column_cell_get_position (column, renderer,
                                                  &start, &width))
        {
          gint xpad, ypad;
          gint x;

          gtk_cell_renderer_get_padding (renderer, &xpad, &ypad);

          if (rtl)
            x = column_area->x + column_area->width - start - width;
          else
            x = start + column_area->x;

          if (tree_x >= x + xpad &&
              tree_x <  x + width - xpad)
            {
              return renderer;
            }
        }
    }

  return NULL;
}

static gboolean
ligma_container_tree_view_button (GtkWidget             *widget,
                                 GdkEventButton        *bevent,
                                 LigmaContainerTreeView *tree_view)
{
  LigmaContainerView *container_view = LIGMA_CONTAINER_VIEW (tree_view);
  GtkTreeViewColumn *column;
  GtkTreePath       *path;

  tree_view->priv->dnd_renderer = NULL;

  if (bevent->type != GDK_BUTTON_RELEASE && ! gtk_widget_has_focus (widget))
    gtk_widget_grab_focus (widget);

  if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget),
                                     bevent->x, bevent->y,
                                     &path, &column, NULL, NULL))
    {
      LigmaViewRenderer         *renderer;
      GtkCellRenderer          *toggled_cell = NULL;
      LigmaCellRendererViewable *clicked_cell = NULL;
      GtkCellRenderer          *edit_cell    = NULL;
      GdkRectangle              column_area;
      GtkTreeIter               iter;
      gboolean                  handled = TRUE;
      gboolean                  multisel_mode;
      GdkModifierType           modifiers = (bevent->state & ligma_get_all_modifiers_mask ());

      multisel_mode = (gtk_tree_selection_get_mode (tree_view->priv->selection)
                       == GTK_SELECTION_MULTIPLE);

      if (! modifiers ||
          (modifiers & ~(ligma_get_extend_selection_mask () | ligma_get_modify_selection_mask ())))
        {
          /*  don't chain up for multi-selection handling if none of
           *  the participating modifiers is pressed, we implement
           *  button_press completely ourselves for a reason and don't
           *  want the default implementation mess up our state
           */
          multisel_mode = FALSE;
        }

      gtk_tree_model_get_iter (tree_view->model, &iter, path);

      renderer = ligma_container_tree_store_get_renderer (LIGMA_CONTAINER_TREE_STORE (tree_view->model), &iter);

      tree_view->priv->dnd_renderer = renderer;

      gtk_tree_view_get_cell_area (tree_view->view, path,
                                   column, &column_area);

      gtk_tree_view_column_cell_set_cell_data (column,
                                               tree_view->model,
                                               &iter,
                                               FALSE, FALSE);

      if (bevent->button == 1                                     &&
          gtk_tree_model_iter_has_child (tree_view->model, &iter) &&
          column == gtk_tree_view_get_expander_column (tree_view->view))
        {
          GList *cells;

          cells = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));

          if (! ligma_container_tree_view_find_click_cell (widget,
                                                          cells,
                                                          column, &column_area,
                                                          bevent->x, bevent->y))
            {
              if (bevent->state & ligma_get_extend_selection_mask ()                       &&
                  bevent->type == GDK_BUTTON_PRESS                                        &&
                  bevent->window == gtk_tree_view_get_bin_window (GTK_TREE_VIEW (widget)) &&
                  ! gtk_tree_view_is_blank_at_pos (GTK_TREE_VIEW (widget),
                                                   bevent->x, bevent->y, NULL, NULL,
                                                   NULL, NULL))
                {
                  /* When shift-clicking an expander, let's have a
                   * behavior similar to shift-clicking the visibility
                   * or link toggles, i.e. expanding the group alone or
                   * together will all groups of same depth.
                   */
                  GtkTreePath *first_path;
                  GtkTreeIter  parent;
                  gboolean     expand_all = TRUE;

                  /* Get the first item at same depth. */
                  if (gtk_tree_model_iter_parent (tree_view->model, &parent, &iter))
                    gtk_tree_model_iter_nth_child (tree_view->model, &iter, &parent, 0);
                  else
                    gtk_tree_model_get_iter_first (tree_view->model, &iter);
                  first_path = gtk_tree_model_get_path (tree_view->model, &iter);

                  /* Check expansion state of other items at same depth. */
                  do
                    {
                      GtkTreePath *path2 = gtk_tree_model_get_path (tree_view->model, &iter);

                      if (gtk_tree_path_compare (path, path2) != 0 &&
                          gtk_tree_view_row_expanded (GTK_TREE_VIEW (widget), path2))
                        expand_all = FALSE;

                      gtk_tree_path_free (path2);

                      if (! expand_all)
                        break;
                    }
                  while (gtk_tree_model_iter_next (tree_view->model, &iter));

                  /* Revert back to first item at same depth. */
                  gtk_tree_model_get_iter (tree_view->model, &iter, first_path);
                  gtk_tree_path_free (first_path);

                  /* Expand or collapse rows at this depth. */
                  do
                    {
                      GtkTreePath *path2 = gtk_tree_model_get_path (tree_view->model, &iter);

                      if (gtk_tree_path_compare (path, path2) == 0 || expand_all)
                        gtk_tree_view_expand_to_path (GTK_TREE_VIEW (widget), path2);
                      else
                        gtk_tree_view_collapse_row (GTK_TREE_VIEW (widget), path2);

                      gtk_tree_path_free (path2);
                    }
                  while (gtk_tree_model_iter_next (tree_view->model, &iter));

                  gtk_tree_view_scroll_to_cell (tree_view->view, path,
                                                NULL, FALSE, 0.0, 0.0);

                  handled = TRUE;
                }
              else
                {
                  /*  we didn't click on any cell, but we clicked on empty
                   *  space in the expander column of a row that has
                   *  children; let GtkTreeView process the button press
                   *  to maybe handle a click on an expander.
                   */
                  handled = FALSE;
                }

              g_list_free (cells);
              gtk_tree_path_free (path);
              g_object_unref (renderer);

              return handled;
            }

          g_list_free (cells);
        }

      toggled_cell =
        ligma_container_tree_view_find_click_cell (widget,
                                                  tree_view->priv->toggle_cells,
                                                  column, &column_area,
                                                  bevent->x, bevent->y);

      if (! toggled_cell)
        clicked_cell = (LigmaCellRendererViewable *)
          ligma_container_tree_view_find_click_cell (widget,
                                                    tree_view->priv->renderer_cells,
                                                    column, &column_area,
                                                    bevent->x, bevent->y);

      if (! toggled_cell && ! clicked_cell)
        {
          edit_cell =
            ligma_container_tree_view_find_click_cell (widget,
                                                      tree_view->priv->editable_cells,
                                                      column, &column_area,
                                                      bevent->x, bevent->y);

          if (edit_cell && bevent->type == GDK_BUTTON_RELEASE)
            {
              gchar *editing_path;

              editing_path = gtk_tree_path_to_string (path);
              if (tree_view->priv->editing_path &&
                  g_strcmp0 (tree_view->priv->editing_path, editing_path) == 0)
                {
                  /* Bail out when releasing over an edit cell we are
                   * already editing.
                   * The reason is that GDK_2BUTTON_PRESS happens before
                   * the last GDK_BUTTON_RELEASE in a double click. So
                   * if we were to proceed, the edit-in-progress would
                   * end up being canceled (by updating the selection)
                   * nearly immediately.
                   */
                  g_free (editing_path);
                  gtk_tree_path_free (path);
                  g_object_unref (renderer);
                  return handled;
                }
              g_free (editing_path);
            }
        }

      g_object_ref (tree_view);

      if (ligma_event_triggers_context_menu ((GdkEvent *) bevent, TRUE))
        {
          /* If the clicked item is not selected, it becomes the new
           * selection. Otherwise, we use the current selection. This
           * allows to not break multiple selection when right-clicking.
           */
          if (! ligma_container_view_is_item_selected (container_view, renderer->viewable))
            ligma_container_view_item_selected (container_view, renderer->viewable);
          /* Show the context menu. */
          if (ligma_container_view_get_container (container_view))
            ligma_editor_popup_menu_at_pointer (LIGMA_EDITOR (tree_view), (GdkEvent *) bevent);
        }
      else if (bevent->button == 1)
        {
          handled = TRUE;
          if (bevent->type == GDK_BUTTON_PRESS || bevent->type == GDK_BUTTON_RELEASE)
            {
              /*  don't select item if a toggle was clicked */
              if (! toggled_cell)
                {
                  gchar *path_str = gtk_tree_path_to_string (path);

                  handled = FALSE;
                  if (bevent->type == GDK_BUTTON_RELEASE && clicked_cell)
                    handled =
                      ligma_cell_renderer_viewable_pre_clicked (clicked_cell,
                                                               path_str,
                                                               bevent->state);

                  if (! handled && ! multisel_mode && ! modifiers)
                    {
                      if (! tree_view->priv->editing_path &&
                          (bevent->type == GDK_BUTTON_RELEASE ||
                           ! ligma_container_view_is_item_selected (container_view, renderer->viewable)))
                        /* If we click on currently selected item,
                         * handle simple click on release only for it
                         * to not change a multi-selection in case this
                         * click becomes a drag'n drop action.
                         */
                        handled =
                          ligma_container_view_item_selected (container_view,
                                                             renderer->viewable);
                    }
                  /* Multi selection will be handled by ligma_container_tree_view_selection_changed() */

                  g_free (path_str);
                }

              /*  a callback invoked by selecting the item may have
               *  destroyed us, so check if the container is still there
               */
              if (ligma_container_view_get_container (container_view))
                {
                  /*  another row may have been set by selecting  */
                  gtk_tree_view_column_cell_set_cell_data (column,
                                                           tree_view->model,
                                                           &iter,
                                                           FALSE, FALSE);

                  if (bevent->type == GDK_BUTTON_PRESS &&
                      (toggled_cell || clicked_cell))
                    {
                      gchar *path_str = gtk_tree_path_to_string (path);

                      if (toggled_cell)
                        {
                          if (LIGMA_IS_CELL_RENDERER_TOGGLE (toggled_cell))
                            {
                              ligma_cell_renderer_toggle_clicked (LIGMA_CELL_RENDERER_TOGGLE (toggled_cell),
                                                                 path_str,
                                                                 bevent->state);
                            }
                          else if (LIGMA_IS_CELL_RENDERER_BUTTON (toggled_cell))
                            {
                              ligma_cell_renderer_button_clicked (LIGMA_CELL_RENDERER_BUTTON (toggled_cell),
                                                                 path_str,
                                                                 bevent->state);
                            }
                        }
                      else if (clicked_cell)
                        {
                          ligma_cell_renderer_viewable_clicked (clicked_cell,
                                                               path_str,
                                                               bevent->state);
                        }

                      g_free (path_str);
                    }
                }
            }
          else if (bevent->type == GDK_2BUTTON_PRESS)
            {
              gboolean success = TRUE;

              /*  don't select item if a toggle was clicked */
              if (bevent->type == GDK_BUTTON_RELEASE && ! toggled_cell)
                success = ligma_container_view_item_selected (container_view,
                                                             renderer->viewable);

              if (success)
                {
                  if (edit_cell)
                    {
                      if (ligma_viewable_is_name_editable (renderer->viewable))
                        {
                          gtk_tree_view_set_cursor_on_cell (tree_view->view,
                                                            path,
                                                            column, edit_cell,
                                                            TRUE);
                        }
                      else
                        {
                          gtk_widget_error_bell (widget);
                        }
                    }
                  else if (! toggled_cell &&
                           ! (bevent->state & ligma_get_all_modifiers_mask ()))
                    {
                      /* Only activate if we're not in a toggled cell
                       * and no modifier keys are pressed
                       */
                      ligma_container_view_item_activated (container_view,
                                                          renderer->viewable);
                    }
                }
            }
        }
      else if (bevent->button == 2)
        {
          if (bevent->type == GDK_BUTTON_PRESS)
            {
              if (clicked_cell)
                {
                  gchar *path_str = gtk_tree_path_to_string (path);

                  ligma_cell_renderer_viewable_clicked (clicked_cell,
                                                       path_str,
                                                       bevent->state);

                  g_free (path_str);
                }
            }
        }

      g_object_unref (tree_view);

      gtk_tree_path_free (path);
      g_object_unref (renderer);

      return multisel_mode ? handled : (bevent->type == GDK_BUTTON_RELEASE ? FALSE : TRUE);
    }
  else
    {
      if (gdk_event_triggers_context_menu ((GdkEvent *) bevent))
        {
          ligma_editor_popup_menu_at_pointer (LIGMA_EDITOR (tree_view), (GdkEvent *) bevent);
        }

      return TRUE;
    }
}

/* We want to zoom on each 1/4 scroll events to roughly match zooming
 * behavior  of the main canvas. Each step of LIGMA_VIEW_SIZE_* is
 * approximately  of sqrt(2) = 1.4 relative change. The main canvas
 * on the other hand has zoom step equal to ZOOM_MIN_STEP=1.1
 * (see ligma_zoom_model_zoom_step).
 */
#define SCROLL_ZOOM_STEP_SIZE 0.25

static gboolean
ligma_container_tree_view_scroll (GtkWidget             *widget,
                                 GdkEventScroll        *event,
                                 LigmaContainerTreeView *tree_view)
{
  LigmaContainerView *view;
  gint               view_border_width;
  gint               view_size;

  if ((event->state & GDK_CONTROL_MASK) == 0)
    return FALSE;

  if (event->direction == GDK_SCROLL_UP)
    tree_view->priv->zoom_accumulated_scroll_delta -= SCROLL_ZOOM_STEP_SIZE;
  else if (event->direction == GDK_SCROLL_DOWN)
    tree_view->priv->zoom_accumulated_scroll_delta += SCROLL_ZOOM_STEP_SIZE;
  else if (event->direction == GDK_SCROLL_SMOOTH)
    tree_view->priv->zoom_accumulated_scroll_delta += event->delta_y * SCROLL_ZOOM_STEP_SIZE;
  else
    return FALSE;

  view      = LIGMA_CONTAINER_VIEW (tree_view);
  view_size = ligma_container_view_get_view_size (view, &view_border_width);

  if (tree_view->priv->zoom_accumulated_scroll_delta > 1)
    {
      tree_view->priv->zoom_accumulated_scroll_delta -= 1;
      view_size = ligma_view_size_get_smaller (view_size);
    }
  else if (tree_view->priv->zoom_accumulated_scroll_delta < -1)
    {
      tree_view->priv->zoom_accumulated_scroll_delta += 1;
      view_size = ligma_view_size_get_larger (view_size);
    }
  else
    return TRUE;

  ligma_container_view_set_view_size (view, view_size, view_border_width);
  return TRUE;
}

static gboolean
ligma_container_tree_view_tooltip (GtkWidget             *widget,
                                  gint                   x,
                                  gint                   y,
                                  gboolean               keyboard_tip,
                                  GtkTooltip            *tooltip,
                                  LigmaContainerTreeView *tree_view)
{
  LigmaViewRenderer *renderer;
  GtkTreeIter       iter;
  GtkTreePath      *path;
  gboolean          show_tip = FALSE;

  if (! gtk_tree_view_get_tooltip_context (GTK_TREE_VIEW (widget), &x, &y,
                                           keyboard_tip,
                                           NULL, &path, &iter))
    return FALSE;

  renderer = ligma_container_tree_store_get_renderer (LIGMA_CONTAINER_TREE_STORE (tree_view->model), &iter);

  if (renderer)
    {
      gchar *desc;
      gchar *tip;

      desc = ligma_viewable_get_description (renderer->viewable, &tip);

      if (tip)
        {
          gtk_tooltip_set_text (tooltip, tip);
          gtk_tree_view_set_tooltip_row (GTK_TREE_VIEW (widget), tooltip, path);

          show_tip = TRUE;

          g_free (tip);
        }

      g_free (desc);
      g_object_unref (renderer);
    }

  gtk_tree_path_free (path);

  return show_tip;
}

static LigmaViewable *
ligma_container_tree_view_drag_viewable (GtkWidget    *widget,
                                        LigmaContext **context,
                                        gpointer      data)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (data);

  if (context)
    *context = ligma_container_view_get_context (LIGMA_CONTAINER_VIEW (data));

  if (tree_view->priv->dnd_renderer)
    return tree_view->priv->dnd_renderer->viewable;

  return NULL;
}

static GList *
ligma_container_tree_view_drag_viewable_list (GtkWidget    *widget,
                                             LigmaContext **context,
                                             gpointer      data)
{
  GList *items = NULL;

  if (context)
    *context = ligma_container_view_get_context (LIGMA_CONTAINER_VIEW (data));

  ligma_container_tree_view_get_selected (LIGMA_CONTAINER_VIEW (data), &items, NULL);

  return items;
}

static GdkPixbuf *
ligma_container_tree_view_drag_pixbuf (GtkWidget *widget,
                                      gpointer   data)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (data);
  LigmaViewRenderer      *renderer  = tree_view->priv->dnd_renderer;
  gint                   width;
  gint                   height;

  if (renderer && ligma_viewable_get_size (renderer->viewable, &width, &height))
    return ligma_viewable_get_new_pixbuf (renderer->viewable,
                                         renderer->context,
                                         width, height);

  return NULL;
}

#define ZOOM_GESTURE_STEP_SIZE 1.2

static void
ligma_container_tree_view_zoom_gesture_begin (GtkGestureZoom        *gesture,
                                             GdkEventSequence      *sequence,
                                             LigmaContainerTreeView *tree_view)
{
  tree_view->priv->zoom_gesture_last_set_value = gtk_gesture_zoom_get_scale_delta (gesture);
}

static void
ligma_container_tree_view_zoom_gesture_update (GtkGestureZoom        *gesture,
                                              GdkEventSequence      *sequence,
                                              LigmaContainerTreeView *tree_view)
{
  gdouble current_scale;
  gdouble last_set_value         = tree_view->priv->zoom_gesture_last_set_value;
  gdouble min_value_for_increase = last_set_value * ZOOM_GESTURE_STEP_SIZE;
  gdouble max_value_for_decrease = last_set_value / ZOOM_GESTURE_STEP_SIZE;
  LigmaContainerView *view;
  gint               view_border_width;
  gint               view_size;

  view      = LIGMA_CONTAINER_VIEW (tree_view);
  view_size = ligma_container_view_get_view_size (view, &view_border_width);

  current_scale = gtk_gesture_zoom_get_scale_delta (gesture);
  if (current_scale > min_value_for_increase)
    {
      last_set_value = min_value_for_increase;
      view_size      = ligma_view_size_get_larger (view_size);
    }
  else if (current_scale < max_value_for_decrease)
    {
      last_set_value = max_value_for_decrease;
      view_size      = ligma_view_size_get_smaller (view_size);
    }
  else
    return;

  tree_view->priv->zoom_gesture_last_set_value = last_set_value;
  ligma_container_view_set_view_size (view, view_size, view_border_width);
}

static gboolean
ligma_container_tree_view_get_selected_single (LigmaContainerTreeView  *tree_view,
                                              GtkTreeIter            *iter)
{
  GtkTreeSelection *selection;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view->view));

  if (gtk_tree_selection_count_selected_rows (selection) == 1)
    {
      GList    *selected_rows;

      selected_rows = gtk_tree_selection_get_selected_rows (selection, NULL);

      gtk_tree_model_get_iter (GTK_TREE_MODEL (tree_view->model), iter,
                               (GtkTreePath *) selected_rows->data);

      g_list_free_full (selected_rows, (GDestroyNotify) gtk_tree_path_free);

      return TRUE;
    }
  else
    {
      return FALSE;
    }
}

static gint
ligma_container_tree_view_get_selected (LigmaContainerView    *view,
                                       GList               **items,
                                       GList               **paths)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (view);
  GtkTreeSelection      *selection;
  gint                   selected_count;
  GList                 *selected_paths;
  GList                 *removed_paths = NULL;
  GList                 *current_row;
  GtkTreeIter            iter;
  LigmaViewRenderer      *renderer;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view->view));
  selected_count = gtk_tree_selection_count_selected_rows (selection);

  if (items == NULL)
    {
      if (paths)
        *paths = NULL;

      /* just provide selected count */
      return selected_count;
    }

  selected_paths = gtk_tree_selection_get_selected_rows (selection, NULL);
  *items         = NULL;

  for (current_row = selected_paths;
       current_row;
       current_row = g_list_next (current_row))
    {
      gtk_tree_model_get_iter (GTK_TREE_MODEL (tree_view->model), &iter,
                               (GtkTreePath *) current_row->data);

      renderer = ligma_container_tree_store_get_renderer (LIGMA_CONTAINER_TREE_STORE (tree_view->model),
                                                         &iter);

      if (renderer->viewable)
        *items = g_list_prepend (*items, renderer->viewable);
      else
        /* Remove from the selected_paths list but at the end, in order not
         * to break the for loop.
         */
        removed_paths = g_list_prepend (removed_paths, current_row);

      g_object_unref (renderer);
    }
  *items = g_list_reverse (*items);

  for (current_row = removed_paths; current_row; current_row = current_row->next)
    {
      GList *remove_list = current_row->data;

      selected_paths = g_list_remove_link (selected_paths, remove_list);
      gtk_tree_path_free (remove_list->data);
    }
  g_list_free_full (removed_paths, (GDestroyNotify) g_list_free);

  if (paths)
    *paths = selected_paths;
  else
    g_list_free_full (selected_paths, (GDestroyNotify) gtk_tree_path_free);

  return selected_count;
}

static void
ligma_container_tree_view_row_expanded (GtkTreeView           *tree_view,
                                       GtkTreeIter           *iter,
                                       GtkTreePath           *path,
                                       LigmaContainerTreeView *view)
{
  LigmaViewRenderer *renderer;

  renderer = ligma_container_tree_store_get_renderer (LIGMA_CONTAINER_TREE_STORE (view->model),
                                                     iter);
  if (renderer)
    {
      gboolean expanded = gtk_tree_view_row_expanded (tree_view, path);

      ligma_viewable_set_expanded (renderer->viewable,
                                  expanded);
      if (expanded)
        {
          g_signal_handlers_block_by_func (tree_view,
                                           ligma_container_tree_view_row_expanded,
                                           view);

          ligma_container_tree_view_expand_rows (view->model, tree_view, iter);

          g_signal_handlers_unblock_by_func (tree_view,
                                             ligma_container_tree_view_row_expanded,
                                             view);
        }

      g_object_unref (renderer);
    }
}

static void
ligma_container_tree_view_expand_rows (GtkTreeModel *model,
                                      GtkTreeView  *view,
                                      GtkTreeIter  *parent)
{
  GtkTreeIter iter;

  if (gtk_tree_model_iter_children (model, &iter, parent))
    do
      if (gtk_tree_model_iter_has_child (model, &iter))
        {
          LigmaViewRenderer *renderer;

          renderer = ligma_container_tree_store_get_renderer (LIGMA_CONTAINER_TREE_STORE (model),
                                                             &iter);
          if (renderer)
            {
              GtkTreePath *path = gtk_tree_model_get_path (model, &iter);

              if (ligma_viewable_get_expanded (renderer->viewable))
                gtk_tree_view_expand_row (view, path, FALSE);
              else
                gtk_tree_view_collapse_row (view, path);

              gtk_tree_path_free (path);
              g_object_unref (renderer);
            }

          ligma_container_tree_view_expand_rows (model, view, &iter);
        }
    while (gtk_tree_model_iter_next (model, &iter));
}

static gboolean
ligma_container_tree_view_monitor_changed_foreach (GtkTreeModel *model,
                                                  GtkTreePath  *path,
                                                  GtkTreeIter  *iter,
                                                  gpointer      data)
{
  LigmaViewRenderer *renderer;

  renderer = ligma_container_tree_store_get_renderer (LIGMA_CONTAINER_TREE_STORE (model),
                                                     iter);

  if (renderer)
    {
      ligma_view_renderer_free_color_transform (renderer);
      g_object_unref (renderer);
    }

  return FALSE;
}

static void
ligma_container_tree_view_monitor_changed (LigmaContainerTreeView *view)
{
  gtk_tree_model_foreach (view->model,
                          ligma_container_tree_view_monitor_changed_foreach,
                          NULL);
}

typedef struct
{
    LigmaViewable *viewable;
    GtkTreePath  *path;
} SearchData;

static gboolean
ligma_container_tree_view_search_path_foreach (GtkTreeModel *model,
                                              GtkTreePath  *path,
                                              GtkTreeIter  *iter,
                                              gpointer      data)
{
  SearchData       *search_data = data;
  LigmaViewRenderer *renderer;

  renderer = ligma_container_tree_store_get_renderer (LIGMA_CONTAINER_TREE_STORE (model),
                                                     iter);

  if (renderer->viewable == search_data->viewable)
    search_data->path = gtk_tree_path_copy (path);

  g_object_unref (renderer);

  return (search_data->path != NULL);
}

static GtkTreePath *
ligma_container_tree_view_get_path (LigmaContainerTreeView *tree_view,
                                   LigmaViewable          *viewable)
{
  SearchData search_data;

  search_data.viewable = viewable;
  search_data.path     = NULL;

  gtk_tree_model_foreach (GTK_TREE_MODEL (tree_view->model),
                          (GtkTreeModelForeachFunc) ligma_container_tree_view_search_path_foreach,
                          &search_data);

  return search_data.path;
}
