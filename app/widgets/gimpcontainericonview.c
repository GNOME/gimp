/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacontainericonview.c
 * Copyright (C) 2010 Michael Natterer <mitch@ligma.org>
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
#include <gtk/gtk.h>

#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmaviewable.h"

#include "ligmacellrendererviewable.h"
#include "ligmacontainertreestore.h"
#include "ligmacontainericonview.h"
#include "ligmacontainerview.h"
#include "ligmadnd.h"
#include "ligmaviewrenderer.h"
#include "ligmawidgets-utils.h"


struct _LigmaContainerIconViewPrivate
{
  LigmaViewRenderer *dnd_renderer;
};


static void          ligma_container_icon_view_view_iface_init   (LigmaContainerViewInterface  *iface);

static void          ligma_container_icon_view_constructed       (GObject                     *object);
static void          ligma_container_icon_view_finalize          (GObject                     *object);

static void          ligma_container_icon_view_unmap             (GtkWidget                   *widget);
static gboolean      ligma_container_icon_view_popup_menu        (GtkWidget                   *widget);

static void          ligma_container_icon_view_set_container     (LigmaContainerView           *view,
                                                                 LigmaContainer               *container);
static void          ligma_container_icon_view_set_context       (LigmaContainerView           *view,
                                                                 LigmaContext                 *context);
static void          ligma_container_icon_view_set_selection_mode(LigmaContainerView           *view,
                                                                 GtkSelectionMode             mode);

static gpointer      ligma_container_icon_view_insert_item       (LigmaContainerView           *view,
                                                                 LigmaViewable                *viewable,
                                                                 gpointer                     parent_insert_data,
                                                                 gint                         index);
static void          ligma_container_icon_view_remove_item       (LigmaContainerView           *view,
                                                                 LigmaViewable                *viewable,
                                                                 gpointer                     insert_data);
static void          ligma_container_icon_view_reorder_item      (LigmaContainerView           *view,
                                                                 LigmaViewable                *viewable,
                                                                 gint                         new_index,
                                                                 gpointer                     insert_data);
static void          ligma_container_icon_view_rename_item       (LigmaContainerView           *view,
                                                                 LigmaViewable                *viewable,
                                                                 gpointer                     insert_data);
static gboolean      ligma_container_icon_view_select_items      (LigmaContainerView           *view,
                                                                 GList                       *items,
                                                                 GList                       *paths);
static void          ligma_container_icon_view_clear_items       (LigmaContainerView           *view);
static void          ligma_container_icon_view_set_view_size     (LigmaContainerView           *view);

static void          ligma_container_icon_view_selection_changed (GtkIconView                 *view,
                                                                 LigmaContainerIconView       *icon_view);
static void          ligma_container_icon_view_item_activated    (GtkIconView                 *view,
                                                                 GtkTreePath                 *path,
                                                                 LigmaContainerIconView       *icon_view);
static gboolean      ligma_container_icon_view_button_press      (GtkWidget                   *widget,
                                                                 GdkEventButton              *bevent,
                                                                 LigmaContainerIconView       *icon_view);
static gboolean      ligma_container_icon_view_tooltip           (GtkWidget                   *widget,
                                                                 gint                         x,
                                                                 gint                         y,
                                                                 gboolean                     keyboard_tip,
                                                                 GtkTooltip                  *tooltip,
                                                                 LigmaContainerIconView       *icon_view);

static LigmaViewable * ligma_container_icon_view_drag_viewable    (GtkWidget    *widget,
                                                                 LigmaContext **context,
                                                                 gpointer      data);
static GdkPixbuf    * ligma_container_icon_view_drag_pixbuf        (GtkWidget *widget,
                                                                   gpointer   data);
static gboolean      ligma_container_icon_view_get_selected_single (LigmaContainerIconView  *icon_view,
                                                                   GtkTreeIter            *iter);
static gint          ligma_container_icon_view_get_selected        (LigmaContainerView      *view,
                                                                   GList                 **items,
                                                                   GList                 **paths);


G_DEFINE_TYPE_WITH_CODE (LigmaContainerIconView, ligma_container_icon_view,
                         LIGMA_TYPE_CONTAINER_BOX,
                         G_ADD_PRIVATE (LigmaContainerIconView)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONTAINER_VIEW,
                                                ligma_container_icon_view_view_iface_init))

#define parent_class ligma_container_icon_view_parent_class

static LigmaContainerViewInterface *parent_view_iface = NULL;


static void
ligma_container_icon_view_class_init (LigmaContainerIconViewClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = ligma_container_icon_view_constructed;
  object_class->finalize    = ligma_container_icon_view_finalize;

  widget_class->unmap       = ligma_container_icon_view_unmap;
  widget_class->popup_menu  = ligma_container_icon_view_popup_menu;
}

static void
ligma_container_icon_view_view_iface_init (LigmaContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  if (! parent_view_iface)
    parent_view_iface = g_type_default_interface_peek (LIGMA_TYPE_CONTAINER_VIEW);

  iface->set_container      = ligma_container_icon_view_set_container;
  iface->set_context        = ligma_container_icon_view_set_context;
  iface->set_selection_mode = ligma_container_icon_view_set_selection_mode;
  iface->insert_item        = ligma_container_icon_view_insert_item;
  iface->remove_item        = ligma_container_icon_view_remove_item;
  iface->reorder_item       = ligma_container_icon_view_reorder_item;
  iface->rename_item        = ligma_container_icon_view_rename_item;
  iface->select_items       = ligma_container_icon_view_select_items;
  iface->clear_items        = ligma_container_icon_view_clear_items;
  iface->set_view_size      = ligma_container_icon_view_set_view_size;
  iface->get_selected       = ligma_container_icon_view_get_selected;

  iface->insert_data_free = (GDestroyNotify) gtk_tree_iter_free;
}

static void
ligma_container_icon_view_init (LigmaContainerIconView *icon_view)
{
  LigmaContainerBox *box = LIGMA_CONTAINER_BOX (icon_view);

  icon_view->priv = ligma_container_icon_view_get_instance_private (icon_view);

  ligma_container_tree_store_columns_init (icon_view->model_columns,
                                          &icon_view->n_model_columns);

  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (box->scrolled_win),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (box->scrolled_win),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
}

static void
ligma_container_icon_view_constructed (GObject *object)
{
  LigmaContainerIconView *icon_view = LIGMA_CONTAINER_ICON_VIEW (object);
  LigmaContainerView     *view      = LIGMA_CONTAINER_VIEW (object);
  LigmaContainerBox      *box       = LIGMA_CONTAINER_BOX (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  icon_view->model = ligma_container_tree_store_new (view,
                                                    icon_view->n_model_columns,
                                                    icon_view->model_columns);

  icon_view->view = g_object_new (GTK_TYPE_ICON_VIEW,
                                  "model",          icon_view->model,
                                  "row-spacing",    0,
                                  "column-spacing", 0,
                                  "margin",         0,
                                  "item-padding",   1,
                                  "has-tooltip",    TRUE,
                                  NULL);
  g_object_unref (icon_view->model);

  gtk_container_add (GTK_CONTAINER (box->scrolled_win),
                     GTK_WIDGET (icon_view->view));
  gtk_widget_show (GTK_WIDGET (icon_view->view));

  ligma_container_view_set_dnd_widget (view, GTK_WIDGET (icon_view->view));

  icon_view->renderer_cell = ligma_cell_renderer_viewable_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (icon_view->view),
                              icon_view->renderer_cell,
                              FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (icon_view->view),
                                  icon_view->renderer_cell,
                                  "renderer", LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER,
                                  NULL);

  ligma_container_tree_store_add_renderer_cell (LIGMA_CONTAINER_TREE_STORE (icon_view->model),
                                               icon_view->renderer_cell, -1);

  g_signal_connect (icon_view->view, "selection-changed",
                    G_CALLBACK (ligma_container_icon_view_selection_changed),
                    icon_view);
  g_signal_connect (icon_view->view, "item-activated",
                    G_CALLBACK (ligma_container_icon_view_item_activated),
                    icon_view);
  g_signal_connect (icon_view->view, "query-tooltip",
                    G_CALLBACK (ligma_container_icon_view_tooltip),
                    icon_view);
}

static void
ligma_container_icon_view_finalize (GObject *object)
{
  //LigmaContainerIconView *icon_view = LIGMA_CONTAINER_ICON_VIEW (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_container_icon_view_unmap (GtkWidget *widget)
{
  //LigmaContainerIconView *icon_view = LIGMA_CONTAINER_ICON_VIEW (widget);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static gboolean
ligma_container_icon_view_popup_menu (GtkWidget *widget)
{
  LigmaContainerIconView *icon_view = LIGMA_CONTAINER_ICON_VIEW (widget);
  GtkTreeIter            iter;
  GtkTreePath           *path;
  GdkRectangle           rect;

  if (!ligma_container_icon_view_get_selected_single (icon_view, &iter))
    return FALSE;

  path = gtk_tree_model_get_path (icon_view->model, &iter);
  gtk_icon_view_get_cell_rect (icon_view->view, path, NULL, &rect);
  gtk_tree_path_free (path);

  return ligma_editor_popup_menu_at_rect (LIGMA_EDITOR (widget),
                                         gtk_widget_get_window (GTK_WIDGET (icon_view->view)),
                                         &rect, GDK_GRAVITY_CENTER, GDK_GRAVITY_NORTH_WEST,
                                         NULL);
}

GtkWidget *
ligma_container_icon_view_new (LigmaContainer *container,
                              LigmaContext   *context,
                              gint           view_size,
                              gint           view_border_width)
{
  LigmaContainerIconView *icon_view;
  LigmaContainerView     *view;

  g_return_val_if_fail (container == NULL || LIGMA_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (context == NULL || LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= LIGMA_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= LIGMA_VIEW_MAX_BORDER_WIDTH,
                        NULL);

  icon_view = g_object_new (LIGMA_TYPE_CONTAINER_ICON_VIEW, NULL);

  view = LIGMA_CONTAINER_VIEW (icon_view);

  ligma_container_view_set_view_size (view, view_size, 0 /* ignore border */);

  if (container)
    ligma_container_view_set_container (view, container);

  if (context)
    ligma_container_view_set_context (view, context);

  return GTK_WIDGET (icon_view);
}


/*  LigmaContainerView methods  */

static void
ligma_container_icon_view_set_container (LigmaContainerView *view,
                                        LigmaContainer     *container)
{
  LigmaContainerIconView *icon_view = LIGMA_CONTAINER_ICON_VIEW (view);
  LigmaContainer         *old_container;

  old_container = ligma_container_view_get_container (view);

  if (old_container)
    {
      if (! container)
        {
          if (ligma_dnd_viewable_source_remove (GTK_WIDGET (icon_view->view),
                                               ligma_container_get_children_type (old_container)))
            {
              if (LIGMA_VIEWABLE_CLASS (g_type_class_peek (ligma_container_get_children_type (old_container)))->get_size)
                ligma_dnd_pixbuf_source_remove (GTK_WIDGET (icon_view->view));

              gtk_drag_source_unset (GTK_WIDGET (icon_view->view));
            }

          g_signal_handlers_disconnect_by_func (icon_view->view,
                                                ligma_container_icon_view_button_press,
                                                icon_view);
        }
    }
  else if (container)
    {
      if (ligma_dnd_drag_source_set_by_type (GTK_WIDGET (icon_view->view),
                                            GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                                            ligma_container_get_children_type (container),
                                            GDK_ACTION_COPY))
        {
          ligma_dnd_viewable_source_add (GTK_WIDGET (icon_view->view),
                                        ligma_container_get_children_type (container),
                                        ligma_container_icon_view_drag_viewable,
                                        icon_view);

          if (LIGMA_VIEWABLE_CLASS (g_type_class_peek (ligma_container_get_children_type (container)))->get_size)
            ligma_dnd_pixbuf_source_add (GTK_WIDGET (icon_view->view),
                                        ligma_container_icon_view_drag_pixbuf,
                                        icon_view);
        }

      g_signal_connect (icon_view->view, "button-press-event",
                        G_CALLBACK (ligma_container_icon_view_button_press),
                        icon_view);
    }

  parent_view_iface->set_container (view, container);
}

static void
ligma_container_icon_view_set_context (LigmaContainerView *view,
                                      LigmaContext       *context)
{
  LigmaContainerIconView *icon_view = LIGMA_CONTAINER_ICON_VIEW (view);

  parent_view_iface->set_context (view, context);

  if (icon_view->model)
    ligma_container_tree_store_set_context (LIGMA_CONTAINER_TREE_STORE (icon_view->model),
                                           context);
}

static void
ligma_container_icon_view_set_selection_mode (LigmaContainerView *view,
                                             GtkSelectionMode   mode)
{
  LigmaContainerIconView *icon_view = LIGMA_CONTAINER_ICON_VIEW (view);

  gtk_icon_view_set_selection_mode (icon_view->view, mode);

  parent_view_iface->set_selection_mode (view, mode);
}

static gpointer
ligma_container_icon_view_insert_item (LigmaContainerView *view,
                                      LigmaViewable      *viewable,
                                      gpointer           parent_insert_data,
                                      gint               index)
{
  LigmaContainerIconView *icon_view = LIGMA_CONTAINER_ICON_VIEW (view);
  GtkTreeIter           *iter;

  iter = ligma_container_tree_store_insert_item (LIGMA_CONTAINER_TREE_STORE (icon_view->model),
                                                viewable,
                                                parent_insert_data,
                                                index);

  if (parent_insert_data)
    {
#if 0
      GtkTreePath *path = gtk_tree_model_get_path (icon_view->model, iter);

      gtk_icon_view_expand_to_path (icon_view->view, path);

      gtk_tree_path_free (path);
#endif
    }

  return iter;
}

static void
ligma_container_icon_view_remove_item (LigmaContainerView *view,
                                      LigmaViewable      *viewable,
                                      gpointer           insert_data)
{
  LigmaContainerIconView *icon_view = LIGMA_CONTAINER_ICON_VIEW (view);

  ligma_container_tree_store_remove_item (LIGMA_CONTAINER_TREE_STORE (icon_view->model),
                                         viewable,
                                         insert_data);
}

static void
ligma_container_icon_view_reorder_item (LigmaContainerView *view,
                                       LigmaViewable      *viewable,
                                       gint               new_index,
                                       gpointer           insert_data)
{
  LigmaContainerIconView *icon_view = LIGMA_CONTAINER_ICON_VIEW (view);
  GtkTreeIter           *iter      = (GtkTreeIter *) insert_data;
  gboolean               selected  = FALSE;

  if (iter)
    {
      GtkTreeIter selected_iter;

      selected = ligma_container_icon_view_get_selected_single (icon_view,
                                                               &selected_iter);

      if (selected)
        {
          LigmaViewRenderer *renderer;

          gtk_tree_model_get (icon_view->model, &selected_iter,
                              LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                              -1);

          if (renderer->viewable != viewable)
            selected = FALSE;

          g_object_unref (renderer);
        }
    }

  ligma_container_tree_store_reorder_item (LIGMA_CONTAINER_TREE_STORE (icon_view->model),
                                          viewable,
                                          new_index,
                                          iter);

  if (selected)
    ligma_container_view_select_item (view, viewable);
}

static void
ligma_container_icon_view_rename_item (LigmaContainerView *view,
                                      LigmaViewable      *viewable,
                                      gpointer           insert_data)
{
  LigmaContainerIconView *icon_view = LIGMA_CONTAINER_ICON_VIEW (view);
  GtkTreeIter           *iter      = (GtkTreeIter *) insert_data;

  ligma_container_tree_store_rename_item (LIGMA_CONTAINER_TREE_STORE (icon_view->model),
                                         viewable,
                                         iter);
}

static gboolean
ligma_container_icon_view_select_items (LigmaContainerView *view,
                                       GList             *viewables,
                                       GList             *paths)
{
  LigmaContainerIconView *icon_view = LIGMA_CONTAINER_ICON_VIEW (view);
  GList                 *list;

  if (viewables)
    {
      gboolean free_paths = FALSE;

      if (g_list_length (paths) != g_list_length (viewables))
        {
          free_paths = TRUE;
          paths = NULL;
          for (list = viewables; list; list = list->next)
            {
              GtkTreeIter *iter;
              GtkTreePath *path;

              iter = ligma_container_view_lookup (LIGMA_CONTAINER_VIEW (view),
                                                 list->data);
              if (! iter)
                /* This may happen when the LigmaContainerIconView only
                 * shows a subpart of the whole icons. We don't select
                 * what is not shown.
                 */
                continue;

              path = gtk_tree_model_get_path (icon_view->model, iter);

              paths = g_list_prepend (paths, path);
            }
          paths = g_list_reverse (paths);
        }

      g_signal_handlers_block_by_func (icon_view->view,
                                       ligma_container_icon_view_selection_changed,
                                       icon_view);

      gtk_icon_view_unselect_all (icon_view->view);

      for (list = paths; list; list = list->next)
        {
          gtk_icon_view_select_path (icon_view->view, list->data);
        }

      if (list)
        {
          gtk_icon_view_set_cursor (icon_view->view, list->data, NULL, FALSE);
          gtk_icon_view_scroll_to_path (icon_view->view, list->data, FALSE, 0.0, 0.0);
        }

      g_signal_handlers_unblock_by_func (icon_view->view,
                                         ligma_container_icon_view_selection_changed,
                                         icon_view);

      if (free_paths)
        g_list_free_full (paths, (GDestroyNotify) gtk_tree_path_free);
    }
  else
    {
      gtk_icon_view_unselect_all (icon_view->view);
    }

  return TRUE;
}

static void
ligma_container_icon_view_clear_items (LigmaContainerView *view)
{
  LigmaContainerIconView *icon_view = LIGMA_CONTAINER_ICON_VIEW (view);

  ligma_container_tree_store_clear_items (LIGMA_CONTAINER_TREE_STORE (icon_view->model));

  parent_view_iface->clear_items (view);
}

static void
ligma_container_icon_view_set_view_size (LigmaContainerView *view)
{
  LigmaContainerIconView *icon_view = LIGMA_CONTAINER_ICON_VIEW (view);

  if (icon_view->model)
    ligma_container_tree_store_set_view_size (LIGMA_CONTAINER_TREE_STORE (icon_view->model));

  if (icon_view->view)
    {
      gtk_icon_view_set_columns (icon_view->view, -1);
      gtk_icon_view_set_item_width (icon_view->view, -1);

      /* ugly workaround to force the icon view to invalidate all its
       * cached icon sizes
       */
      gtk_icon_view_set_item_orientation (icon_view->view,
                                          GTK_ORIENTATION_VERTICAL);
      gtk_icon_view_set_item_orientation (icon_view->view,
                                          GTK_ORIENTATION_HORIZONTAL);
    }
}


/*  callbacks  */

static void
ligma_container_icon_view_selection_changed (GtkIconView           *gtk_icon_view,
                                            LigmaContainerIconView *icon_view)
{
  LigmaContainerView *view = LIGMA_CONTAINER_VIEW (icon_view);
  GList             *items = NULL;
  GList             *paths;
  GList             *list;

  paths = gtk_icon_view_get_selected_items (icon_view->view);

  for (list = paths; list; list = list->next)
    {
      GtkTreeIter       iter;
      LigmaViewRenderer *renderer;

      gtk_tree_model_get_iter (GTK_TREE_MODEL (icon_view->model), &iter,
                               (GtkTreePath *) list->data);

      gtk_tree_model_get (icon_view->model, &iter,
                          LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);

      if (renderer->viewable)
        items = g_list_prepend (items, renderer->viewable);

      g_object_unref (renderer);
    }

  ligma_container_view_multi_selected (view, items, paths);

  g_list_free_full (paths, (GDestroyNotify) gtk_tree_path_free);
  g_list_free (items);
}

static void
ligma_container_icon_view_item_activated (GtkIconView           *view,
                                         GtkTreePath           *path,
                                         LigmaContainerIconView *icon_view)
{
  GtkTreeIter       iter;
  LigmaViewRenderer *renderer;

  gtk_tree_model_get_iter (icon_view->model, &iter, path);

  gtk_tree_model_get (icon_view->model, &iter,
                      LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                      -1);

  ligma_container_view_item_activated (LIGMA_CONTAINER_VIEW (icon_view),
                                      renderer->viewable);

  g_object_unref (renderer);
}

static gboolean
ligma_container_icon_view_button_press (GtkWidget             *widget,
                                       GdkEventButton        *bevent,
                                       LigmaContainerIconView *icon_view)
{
  LigmaContainerView *container_view = LIGMA_CONTAINER_VIEW (icon_view);
  GtkTreePath       *path;

  icon_view->priv->dnd_renderer = NULL;

  path = gtk_icon_view_get_path_at_pos (GTK_ICON_VIEW (widget),
                                        bevent->x, bevent->y);

  if (path)
    {
      LigmaViewRenderer *renderer;
      GtkTreeIter       iter;

      gtk_tree_model_get_iter (icon_view->model, &iter, path);

      gtk_tree_model_get (icon_view->model, &iter,
                          LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);

      icon_view->priv->dnd_renderer = renderer;

      if (gdk_event_triggers_context_menu ((GdkEvent *) bevent))
        {
          /* If the clicked item is not selected, it becomes the new
           * selection. Otherwise, we use the current selection. This
           * allows to not break multiple selection when right-clicking.
           */
          if (! ligma_container_view_is_item_selected (container_view, renderer->viewable))
            ligma_container_view_item_selected (container_view, renderer->viewable);
          /* Show the context menu. */
          if (ligma_container_view_get_container (container_view))
            ligma_editor_popup_menu_at_pointer (LIGMA_EDITOR (icon_view), (GdkEvent *) bevent);
        }

      g_object_unref (renderer);

      gtk_tree_path_free (path);
    }
  else
    {
      if (gdk_event_triggers_context_menu ((GdkEvent *) bevent))
        {
          ligma_editor_popup_menu_at_pointer (LIGMA_EDITOR (icon_view), (GdkEvent *) bevent);
        }

      return TRUE;
    }

  return FALSE;
}

static gboolean
ligma_container_icon_view_tooltip (GtkWidget             *widget,
                                  gint                   x,
                                  gint                   y,
                                  gboolean               keyboard_tip,
                                  GtkTooltip            *tooltip,
                                  LigmaContainerIconView *icon_view)
{
  LigmaViewRenderer *renderer;
  GtkTreeIter       iter;
  GtkTreePath      *path;
  gboolean          show_tip = FALSE;

  if (! gtk_icon_view_get_tooltip_context (GTK_ICON_VIEW (widget), &x, &y,
                                           keyboard_tip,
                                           NULL, &path, &iter))
    return FALSE;

  gtk_tree_model_get (icon_view->model, &iter,
                      LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                      -1);

  if (renderer)
    {
      gchar *desc;
      gchar *tip;

      desc = ligma_viewable_get_description (renderer->viewable, &tip);

      if (tip)
        {
          gtk_tooltip_set_text (tooltip, tip);
          gtk_icon_view_set_tooltip_cell (GTK_ICON_VIEW (widget), tooltip, path,
                                          icon_view->renderer_cell);

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
ligma_container_icon_view_drag_viewable (GtkWidget    *widget,
                                        LigmaContext **context,
                                        gpointer      data)
{
  LigmaContainerIconView *icon_view = LIGMA_CONTAINER_ICON_VIEW (data);

  if (context)
    *context = ligma_container_view_get_context (LIGMA_CONTAINER_VIEW (data));

  if (icon_view->priv->dnd_renderer)
    return icon_view->priv->dnd_renderer->viewable;

  return NULL;
}

static GdkPixbuf *
ligma_container_icon_view_drag_pixbuf (GtkWidget *widget,
                                      gpointer   data)
{
  LigmaContainerIconView *icon_view = LIGMA_CONTAINER_ICON_VIEW (data);
  LigmaViewRenderer      *renderer  = icon_view->priv->dnd_renderer;
  gint                   width;
  gint                   height;

  if (renderer && ligma_viewable_get_size (renderer->viewable, &width, &height))
    return ligma_viewable_get_new_pixbuf (renderer->viewable,
                                         renderer->context,
                                         width, height);

  return NULL;
}

static gboolean
ligma_container_icon_view_get_selected_single (LigmaContainerIconView  *icon_view,
                                              GtkTreeIter            *iter)
{
  GList    *selected_items;
  gboolean  retval;

  selected_items = gtk_icon_view_get_selected_items (icon_view->view);

  if (g_list_length (selected_items) == 1)
    {
      gtk_tree_model_get_iter (GTK_TREE_MODEL (icon_view->model), iter,
                               (GtkTreePath *) selected_items->data);

      retval = TRUE;
    }
  else
    {
      retval = FALSE;
    }

  g_list_free_full (selected_items, (GDestroyNotify) gtk_tree_path_free);

  return retval;
}

static gint
ligma_container_icon_view_get_selected (LigmaContainerView    *view,
                                       GList               **items,
                                       GList               **paths)
{
  LigmaContainerIconView *icon_view = LIGMA_CONTAINER_ICON_VIEW (view);
  GList                 *selected_paths;
  gint                   selected_count;
  LigmaContainer         *container;

  container = ligma_container_view_get_container (view);

  if (container)
    {
      const gchar *signal_name;
      LigmaContext *context;
      GType        children_type;

      context       = ligma_container_view_get_context (view);
      children_type = ligma_container_get_children_type (container);
      signal_name   = ligma_context_type_to_signal_name (children_type);

      /* As a special case, for containers tied to a context object, we
       * look up this object as being selected.
       * */
      if (signal_name && context)
        {
          LigmaObject  *object;

          object  = ligma_context_get_by_type (context, children_type);

          selected_count = object ? 1 : 0;
          if (items)
            {
              if (object)
                *items = g_list_prepend (NULL, object);
              else
                *items = NULL;
            }
          if (paths)
            *paths = NULL;

          return selected_count;
        }
    }

  selected_paths = gtk_icon_view_get_selected_items (icon_view->view);
  selected_count = g_list_length (selected_paths);

  if (items)
    {
      GList *removed_paths = NULL;
      GList *list;

      *items = NULL;

      for (list = selected_paths;
           list;
           list = g_list_next (list))
        {
          GtkTreeIter       iter;
          LigmaViewRenderer *renderer;

          gtk_tree_model_get_iter (GTK_TREE_MODEL (icon_view->model), &iter,
                                   (GtkTreePath *) list->data);

          gtk_tree_model_get (icon_view->model, &iter,
                              LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                              -1);

          if (renderer->viewable)
            *items = g_list_prepend (*items, renderer->viewable);
          else
            /* Remove from the selected_paths list but at the end, in order not
             * to break the for loop.
             */
            removed_paths = g_list_prepend (removed_paths, list);

          g_object_unref (renderer);
        }
      *items = g_list_reverse (*items);

      for (list = removed_paths; list; list = list->next)
        {
          GList *remove_list = list->data;

          selected_paths = g_list_remove_link (selected_paths, remove_list);
          gtk_tree_path_free (remove_list->data);
        }
      g_list_free_full (removed_paths, (GDestroyNotify) g_list_free);
    }

  if (paths)
    *paths = selected_paths;
  else
    g_list_free_full (selected_paths, (GDestroyNotify) gtk_tree_path_free);

  return selected_count;
}
