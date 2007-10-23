/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainertreeview.c
 * Copyright (C) 2003-2004 Michael Natterer <mitch@gimp.org>
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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpviewable.h"

#include "gimpcellrendererviewable.h"
#include "gimpcontainertreeview.h"
#include "gimpcontainertreeview-dnd.h"
#include "gimpcontainerview.h"
#include "gimpdnd.h"
#include "gimpviewrenderer.h"
#include "gimpwidgets-utils.h"


enum
{
  COLUMN_RENDERER,
  COLUMN_NAME,
  COLUMN_NAME_ATTRIBUTES,
  NUM_COLUMNS
};


static void  gimp_container_tree_view_view_iface_init (GimpContainerViewInterface *iface);

static GObject *gimp_container_tree_view_constructor  (GType                   type,
                                                       guint                   n_params,
                                                       GObjectConstructParam  *params);

static void    gimp_container_tree_view_unrealize     (GtkWidget              *widget);
static void    gimp_container_tree_view_unmap         (GtkWidget              *widget);
static gboolean  gimp_container_tree_view_popup_menu  (GtkWidget              *widget);

static void    gimp_container_tree_view_set_container (GimpContainerView      *view,
                                                       GimpContainer          *container);
static void    gimp_container_tree_view_set_context   (GimpContainerView      *view,
                                                       GimpContext            *context);
static gpointer gimp_container_tree_view_insert_item  (GimpContainerView      *view,
                                                       GimpViewable           *viewable,
                                                       gint                    index);
static void     gimp_container_tree_view_remove_item  (GimpContainerView      *view,
                                                       GimpViewable           *viewable,
                                                       gpointer                insert_data);
static void     gimp_container_tree_view_reorder_item (GimpContainerView      *view,
                                                       GimpViewable           *viewable,
                                                       gint                    new_index,
                                                       gpointer                insert_data);
static void     gimp_container_tree_view_rename_item  (GimpContainerView      *view,
                                                       GimpViewable           *viewable,
                                                       gpointer                insert_data);
static gboolean  gimp_container_tree_view_select_item (GimpContainerView      *view,
                                                       GimpViewable           *viewable,
                                                       gpointer                insert_data);
static void     gimp_container_tree_view_clear_items  (GimpContainerView      *view);
static void    gimp_container_tree_view_set_view_size (GimpContainerView      *view);

static void gimp_container_tree_view_name_canceled    (GtkCellRendererText    *cell,
                                                       GimpContainerTreeView  *tree_view);

static void gimp_container_tree_view_selection_changed (GtkTreeSelection      *sel,
                                                        GimpContainerTreeView *tree_view);
static gboolean gimp_container_tree_view_button_press (GtkWidget              *widget,
                                                       GdkEventButton         *bevent,
                                                       GimpContainerTreeView  *tree_view);
static void gimp_container_tree_view_renderer_update  (GimpViewRenderer       *renderer,
                                                       GimpContainerTreeView  *tree_view);

static GimpViewable * gimp_container_tree_view_drag_viewable (GtkWidget       *widget,
                                                              GimpContext    **context,
                                                              gpointer         data);
static GdkPixbuf    * gimp_container_tree_view_drag_pixbuf   (GtkWidget       *widget,
                                                              gpointer         data);


G_DEFINE_TYPE_WITH_CODE (GimpContainerTreeView, gimp_container_tree_view,
                         GIMP_TYPE_CONTAINER_BOX,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONTAINER_VIEW,
                                                gimp_container_tree_view_view_iface_init))

#define parent_class gimp_container_tree_view_parent_class

static GimpContainerViewInterface *parent_view_iface = NULL;


static void
gimp_container_tree_view_class_init (GimpContainerTreeViewClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructor = gimp_container_tree_view_constructor;

  widget_class->unrealize   = gimp_container_tree_view_unrealize;
  widget_class->unmap       = gimp_container_tree_view_unmap;
  widget_class->popup_menu  = gimp_container_tree_view_popup_menu;

  klass->drop_possible      = gimp_container_tree_view_real_drop_possible;
  klass->drop_viewable      = gimp_container_tree_view_real_drop_viewable;
  klass->drop_color         = NULL;
  klass->drop_uri_list      = NULL;
  klass->drop_svg           = NULL;
  klass->drop_component     = NULL;
  klass->drop_pixbuf        = NULL;
}

static void
gimp_container_tree_view_view_iface_init (GimpContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  iface->set_container = gimp_container_tree_view_set_container;
  iface->set_context   = gimp_container_tree_view_set_context;
  iface->insert_item   = gimp_container_tree_view_insert_item;
  iface->remove_item   = gimp_container_tree_view_remove_item;
  iface->reorder_item  = gimp_container_tree_view_reorder_item;
  iface->rename_item   = gimp_container_tree_view_rename_item;
  iface->select_item   = gimp_container_tree_view_select_item;
  iface->clear_items   = gimp_container_tree_view_clear_items;
  iface->set_view_size = gimp_container_tree_view_set_view_size;

  iface->insert_data_free = (GDestroyNotify) gtk_tree_iter_free;
}

static void
gimp_container_tree_view_init (GimpContainerTreeView *tree_view)
{
  GimpContainerBox *box = GIMP_CONTAINER_BOX (tree_view);

  tree_view->n_model_columns = NUM_COLUMNS;

  tree_view->model_columns[COLUMN_RENDERER]        = GIMP_TYPE_VIEW_RENDERER;
  tree_view->model_columns[COLUMN_NAME]            = G_TYPE_STRING;
  tree_view->model_columns[COLUMN_NAME_ATTRIBUTES] = PANGO_TYPE_ATTR_LIST;

  tree_view->model_column_renderer        = COLUMN_RENDERER;
  tree_view->model_column_name            = COLUMN_NAME;
  tree_view->model_column_name_attributes = COLUMN_NAME_ATTRIBUTES;

  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (box->scrolled_win),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (box->scrolled_win),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
}

static GObject *
gimp_container_tree_view_constructor (GType                  type,
                                      guint                  n_params,
                                      GObjectConstructParam *params)
{
  GimpContainerTreeView *tree_view;
  GimpContainerView     *view;
  GimpContainerBox      *box;
  GtkListStore          *list;
  GObject               *object;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  tree_view = GIMP_CONTAINER_TREE_VIEW (object);
  view      = GIMP_CONTAINER_VIEW (object);
  box       = GIMP_CONTAINER_BOX (object);

  list = gtk_list_store_newv (tree_view->n_model_columns,
                              tree_view->model_columns);
  tree_view->model = GTK_TREE_MODEL (list);

  tree_view->view = g_object_new (GTK_TYPE_TREE_VIEW,
                                  "model",           list,
                                  "search-column",   COLUMN_NAME,
                                  "enable-search",   FALSE,
                                  "headers-visible", FALSE,
                                  NULL);
  g_object_unref (list);

  gtk_container_add (GTK_CONTAINER (box->scrolled_win),
                     GTK_WIDGET (tree_view->view));
  gtk_widget_show (GTK_WIDGET (tree_view->view));

  gimp_container_view_set_dnd_widget (view, GTK_WIDGET (tree_view->view));

  tree_view->main_column = gtk_tree_view_column_new ();
  gtk_tree_view_insert_column (tree_view->view, tree_view->main_column, 0);

  tree_view->renderer_cell = gimp_cell_renderer_viewable_new ();
  gtk_tree_view_column_pack_start (tree_view->main_column,
                                   tree_view->renderer_cell,
                                   FALSE);

  gtk_tree_view_column_set_attributes (tree_view->main_column,
                                       tree_view->renderer_cell,
                                       "renderer", COLUMN_RENDERER,
                                       NULL);

  tree_view->name_cell = gtk_cell_renderer_text_new ();
  g_object_set (tree_view->name_cell, "xalign", 0.0, NULL);
  gtk_tree_view_column_pack_end (tree_view->main_column,
                                 tree_view->name_cell,
                                 FALSE);

  gtk_tree_view_column_set_attributes (tree_view->main_column,
                                       tree_view->name_cell,
                                       "text",       COLUMN_NAME,
                                       "attributes", COLUMN_NAME_ATTRIBUTES,
                                       NULL);

  g_signal_connect (tree_view->name_cell, "editing-canceled",
                    G_CALLBACK (gimp_container_tree_view_name_canceled),
                    tree_view);

  tree_view->renderer_cells = g_list_prepend (tree_view->renderer_cells,
                                              tree_view->renderer_cell);

  tree_view->selection = gtk_tree_view_get_selection (tree_view->view);

  g_signal_connect (tree_view->selection, "changed",
                    G_CALLBACK (gimp_container_tree_view_selection_changed),
                    tree_view);

  g_signal_connect (tree_view->view, "drag-leave",
                    G_CALLBACK (gimp_container_tree_view_drag_leave),
                    tree_view);
  g_signal_connect (tree_view->view, "drag-motion",
                    G_CALLBACK (gimp_container_tree_view_drag_motion),
                    tree_view);
  g_signal_connect (tree_view->view, "drag-drop",
                    G_CALLBACK (gimp_container_tree_view_drag_drop),
                    tree_view);
  g_signal_connect (tree_view->view, "drag-data-received",
                    G_CALLBACK (gimp_container_tree_view_drag_data_received),
                    tree_view);

  return object;
}

static void
gimp_container_tree_view_unrealize (GtkWidget *widget)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (widget);
  GtkTreeIter            iter;
  gboolean               iter_valid;

  for (iter_valid = gtk_tree_model_get_iter_first (tree_view->model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (tree_view->model, &iter))
    {
      GimpViewRenderer *renderer;

      gtk_tree_model_get (tree_view->model, &iter,
                          COLUMN_RENDERER, &renderer,
                          -1);

      gimp_view_renderer_unrealize (renderer);
      g_object_unref (renderer);
    }

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
gimp_container_tree_view_unmap (GtkWidget *widget)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (widget);

  if (tree_view->scroll_timeout_id)
    {
      g_source_remove (tree_view->scroll_timeout_id);
      tree_view->scroll_timeout_id = 0;
    }

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
gimp_container_tree_view_menu_position (GtkMenu  *menu,
                                        gint     *x,
                                        gint     *y,
                                        gpointer  data)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (data);
  GtkWidget             *widget    = GTK_WIDGET (tree_view->view);
  GtkTreeIter            selected_iter;

  gdk_window_get_origin (widget->window, x, y);

  if (GTK_WIDGET_NO_WINDOW (widget))
    {
      *x += widget->allocation.x;
      *y += widget->allocation.y;
    }

  if (gtk_tree_selection_get_selected (tree_view->selection, NULL,
                                       &selected_iter))
    {
      GtkTreePath  *path;
      GdkRectangle  cell_rect;
      gint          center;

      path = gtk_tree_model_get_path (tree_view->model, &selected_iter);
      gtk_tree_view_get_cell_area (tree_view->view, path,
                                   tree_view->main_column, &cell_rect);
      gtk_tree_path_free (path);

      center = cell_rect.y + cell_rect.height / 2;
      center = CLAMP (center, 0, widget->allocation.height);

      *x += widget->allocation.width / 2;
      *y += center;
    }
  else
    {
      *x += widget->style->xthickness;
      *y += widget->style->ythickness;
    }

  gimp_menu_position (menu, x, y);
}

static gboolean
gimp_container_tree_view_popup_menu (GtkWidget *widget)
{
  return gimp_editor_popup_menu (GIMP_EDITOR (widget),
                                 gimp_container_tree_view_menu_position,
                                 widget);
}

GtkWidget *
gimp_container_tree_view_new (GimpContainer *container,
                              GimpContext   *context,
                              gint           view_size,
                              gint           view_border_width)
{
  GimpContainerTreeView *tree_view;
  GimpContainerView     *view;

  g_return_val_if_fail (container == NULL || GIMP_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (context == NULL || GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= GIMP_VIEW_MAX_BORDER_WIDTH,
                        NULL);

  tree_view = g_object_new (GIMP_TYPE_CONTAINER_TREE_VIEW, NULL);

  view = GIMP_CONTAINER_VIEW (tree_view);

  gimp_container_view_set_view_size (view, view_size, view_border_width);

  if (container)
    gimp_container_view_set_container (view, container);

  if (context)
    gimp_container_view_set_context (view, context);

  return GTK_WIDGET (tree_view);
}

static void
gimp_container_tree_view_set (GimpContainerTreeView *tree_view,
                              GtkTreeIter           *iter,
                              GimpViewable          *viewable)
{
  GimpContainerView *view = GIMP_CONTAINER_VIEW (tree_view);
  GimpViewRenderer  *renderer;
  gchar             *name;
  gint               view_size;
  gint               border_width;

  view_size = gimp_container_view_get_view_size (view, &border_width);

  renderer = gimp_view_renderer_new (gimp_container_view_get_context (view),
                                     G_TYPE_FROM_INSTANCE (viewable),
                                     view_size, border_width,
                                     FALSE);
  gimp_view_renderer_set_viewable (renderer, viewable);
  gimp_view_renderer_remove_idle (renderer);

  g_signal_connect (renderer, "update",
                    G_CALLBACK (gimp_container_tree_view_renderer_update),
                    tree_view);

  name = gimp_viewable_get_description (viewable, NULL);

  gtk_list_store_set (GTK_LIST_STORE (tree_view->model), iter,
                      COLUMN_RENDERER, renderer,
                      COLUMN_NAME,     name,
                      -1);

  g_free (name);
  g_object_unref (renderer);
}

/*  GimpContainerView methods  */

static void
gimp_container_tree_view_set_container (GimpContainerView *view,
                                        GimpContainer     *container)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (view);
  GimpContainer         *old_container;

  old_container = gimp_container_view_get_container (view);

  if (old_container)
    {
      if (! container)
        {
          if (gimp_dnd_viewable_source_remove (GTK_WIDGET (tree_view->view),
                                               old_container->children_type))
            {
              if (GIMP_VIEWABLE_CLASS (g_type_class_peek (old_container->children_type))->get_size)
                gimp_dnd_pixbuf_source_remove (GTK_WIDGET (tree_view->view));

              gtk_drag_source_unset (GTK_WIDGET (tree_view->view));
            }

          g_signal_handlers_disconnect_by_func (tree_view->view,
                                                gimp_container_tree_view_button_press,
                                                tree_view);
        }
    }
  else if (container)
    {
      if (gimp_dnd_drag_source_set_by_type (GTK_WIDGET (tree_view->view),
                                            GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                                            container->children_type,
                                            GDK_ACTION_COPY))
        {
          gimp_dnd_viewable_source_add (GTK_WIDGET (tree_view->view),
                                        container->children_type,
                                        gimp_container_tree_view_drag_viewable,
                                        tree_view);

          if (GIMP_VIEWABLE_CLASS (g_type_class_peek (container->children_type))->get_size)
            gimp_dnd_pixbuf_source_add (GTK_WIDGET (tree_view->view),
                                        gimp_container_tree_view_drag_pixbuf,
                                        tree_view);
        }

      /*  connect button_press_event after DND so we can keep the list from
       *  selecting the item on button2
       */
      g_signal_connect (tree_view->view, "button-press-event",
                        G_CALLBACK (gimp_container_tree_view_button_press),
                        tree_view);
    }

  parent_view_iface->set_container (view, container);
}

static void
gimp_container_tree_view_set_context (GimpContainerView *view,
                                      GimpContext       *context)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (view);

  parent_view_iface->set_context (view, context);

  if (tree_view->model)
    {
      GtkTreeIter iter;
      gboolean    iter_valid;

      for (iter_valid = gtk_tree_model_get_iter_first (tree_view->model, &iter);
           iter_valid;
           iter_valid = gtk_tree_model_iter_next (tree_view->model, &iter))
        {
          GimpViewRenderer *renderer;

          gtk_tree_model_get (tree_view->model, &iter,
                              COLUMN_RENDERER, &renderer,
                              -1);

          gimp_view_renderer_set_context (renderer, context);
          g_object_unref (renderer);
        }
    }
}

static gpointer
gimp_container_tree_view_insert_item (GimpContainerView *view,
                                      GimpViewable      *viewable,
                                      gint               index)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (view);
  GtkTreeIter            iter;

  if (index == -1)
    gtk_list_store_append (GTK_LIST_STORE (tree_view->model), &iter);
  else
    gtk_list_store_insert (GTK_LIST_STORE (tree_view->model), &iter, index);

  gimp_container_tree_view_set (tree_view, &iter, viewable);

  return gtk_tree_iter_copy (&iter);
}

static void
gimp_container_tree_view_remove_item (GimpContainerView *view,
                                      GimpViewable      *viewable,
                                      gpointer           insert_data)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (view);
  GtkTreeIter           *iter      = (GtkTreeIter *) insert_data;

  if (iter)
    {
      gtk_list_store_remove (GTK_LIST_STORE (tree_view->model), iter);

      /*  If the store is empty after this remove, clear out renderers
       *  from all cells so they don't keep refing the viewables
       *  (see bug #149906).
       */
      if (! gtk_tree_model_iter_n_children (tree_view->model, NULL))
        {
          GList *list;

          for (list = tree_view->renderer_cells; list; list = list->next)
            g_object_set (list->data, "renderer", NULL, NULL);
        }
    }
}

static void
gimp_container_tree_view_reorder_item (GimpContainerView *view,
                                       GimpViewable      *viewable,
                                       gint               new_index,
                                       gpointer           insert_data)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (view);
  GtkTreeIter           *iter      = (GtkTreeIter *) insert_data;

  if (iter)
    {
      GimpContainer *container;
      GtkTreePath   *path;
      GtkTreeIter    selected_iter;
      gboolean       selected;

      container = gimp_container_view_get_container (view);

      selected = gtk_tree_selection_get_selected (tree_view->selection,
                                                  NULL, &selected_iter);

      if (selected)
        {
          GimpViewRenderer *renderer;

          gtk_tree_model_get (tree_view->model, &selected_iter,
                              COLUMN_RENDERER, &renderer,
                              -1);

          if (renderer->viewable != viewable)
            selected = FALSE;

          g_object_unref (renderer);
        }

      if (new_index == -1 || new_index == container->num_children - 1)
        {
          gtk_list_store_move_before (GTK_LIST_STORE (tree_view->model),
                                      iter, NULL);
        }
      else if (new_index == 0)
        {
          gtk_list_store_move_after (GTK_LIST_STORE (tree_view->model),
                                     iter, NULL);
        }
      else
        {
          GtkTreeIter place_iter;
          gint        old_index;

          path = gtk_tree_model_get_path (tree_view->model, iter);
          old_index = gtk_tree_path_get_indices (path)[0];
          gtk_tree_path_free (path);

          if (new_index != old_index)
            {
              path = gtk_tree_path_new_from_indices (new_index, -1);
              gtk_tree_model_get_iter (tree_view->model, &place_iter, path);
              gtk_tree_path_free (path);

              if (new_index > old_index)
                gtk_list_store_move_after (GTK_LIST_STORE (tree_view->model),
                                           iter, &place_iter);
              else
                gtk_list_store_move_before (GTK_LIST_STORE (tree_view->model),
                                            iter, &place_iter);
            }
        }

      if (selected)
        gimp_container_view_select_item (view, viewable);
    }
}

static void
gimp_container_tree_view_rename_item (GimpContainerView *view,
                                      GimpViewable      *viewable,
                                      gpointer           insert_data)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (view);
  GtkTreeIter           *iter      = (GtkTreeIter *) insert_data;

  if (iter)
    {
      gchar *name = gimp_viewable_get_description (viewable, NULL);

      gtk_list_store_set (GTK_LIST_STORE (tree_view->model), iter,
                          COLUMN_NAME, name,
                          -1);

      g_free (name);
    }
}

static gboolean
gimp_container_tree_view_select_item (GimpContainerView *view,
                                      GimpViewable      *viewable,
                                      gpointer           insert_data)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (view);
  GtkTreeIter           *iter      = (GtkTreeIter *) insert_data;

  if (iter)
    {
      GtkTreePath *path;

      path = gtk_tree_model_get_path (tree_view->model, iter);

      g_signal_handlers_block_by_func (tree_view->selection,
                                       gimp_container_tree_view_selection_changed,
                                       tree_view);

      gtk_tree_view_set_cursor (tree_view->view, path, NULL, FALSE);

      g_signal_handlers_unblock_by_func (tree_view->selection,
                                         gimp_container_tree_view_selection_changed,
                                         tree_view);

      gtk_tree_view_scroll_to_cell (tree_view->view, path,
                                    NULL, FALSE, 0.0, 0.0);

      gtk_tree_path_free (path);
    }
  else
    {
      gtk_tree_selection_unselect_all (tree_view->selection);
    }

  return TRUE;
}

static void
gimp_container_tree_view_clear_items (GimpContainerView *view)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (view);

  gtk_list_store_clear (GTK_LIST_STORE (tree_view->model));

  /*  Clear out renderers from all cells so they don't keep refing the
   *  viewables (see bug #149906).
   */
  if (! gtk_tree_model_iter_n_children (tree_view->model, NULL))
    {
      GList *list;

      for (list = tree_view->renderer_cells; list; list = list->next)
        g_object_set (list->data, "renderer", NULL, NULL);
    }

  parent_view_iface->clear_items (view);
}

static void
gimp_container_tree_view_set_view_size (GimpContainerView *view)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (view);
  GtkWidget             *tree_widget;
  GList                 *list;
  gint                   view_size;
  gint                   border_width;

  view_size = gimp_container_view_get_view_size (view, &border_width);

  if (tree_view->model)
    {
      GtkTreeIter iter;
      gboolean    iter_valid;

      for (iter_valid = gtk_tree_model_get_iter_first (tree_view->model, &iter);
           iter_valid;
           iter_valid = gtk_tree_model_iter_next (tree_view->model, &iter))
        {
          GimpViewRenderer *renderer;

          gtk_tree_model_get (tree_view->model, &iter,
                              COLUMN_RENDERER, &renderer,
                              -1);

          gimp_view_renderer_set_size (renderer, view_size, border_width);
          g_object_unref (renderer);
        }
    }

  tree_widget = GTK_WIDGET (tree_view->view);

  if (! tree_widget)
    return;

  for (list = tree_view->toggle_cells; list; list = g_list_next (list))
    {
      gchar       *stock_id;
      GtkIconSize  icon_size;

      g_object_get (list->data, "stock-id", &stock_id, NULL);

      if (stock_id)
        {
          icon_size = gimp_get_icon_size (tree_widget,
                                          stock_id,
                                          GTK_ICON_SIZE_BUTTON,
                                          view_size -
                                          2 * tree_widget->style->xthickness,
                                          view_size -
                                          2 * tree_widget->style->ythickness);

          g_object_set (list->data, "stock-size", icon_size, NULL);

          g_free (stock_id);
        }
    }

  gtk_tree_view_columns_autosize (tree_view->view);
}


/*  callbacks  */

static void
gimp_container_tree_view_name_canceled (GtkCellRendererText   *cell,
                                        GimpContainerTreeView *tree_view)
{
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (tree_view->selection, NULL, &iter))
    {
      GimpViewRenderer *renderer;
      gchar            *name;

      gtk_tree_model_get (tree_view->model, &iter,
                          tree_view->model_column_renderer, &renderer,
                          -1);

      name = gimp_viewable_get_description (renderer->viewable, NULL);

      gtk_list_store_set (GTK_LIST_STORE (tree_view->model), &iter,
                          tree_view->model_column_name, name,
                          -1);

      g_free (name);
      g_object_unref (renderer);
    }
}

static void
gimp_container_tree_view_selection_changed (GtkTreeSelection      *selection,
                                            GimpContainerTreeView *tree_view)
{
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      GimpViewRenderer *renderer;

      gtk_tree_model_get (tree_view->model, &iter,
                          COLUMN_RENDERER, &renderer,
                          -1);

      gimp_container_view_item_selected (GIMP_CONTAINER_VIEW (tree_view),
                                         renderer->viewable);
      g_object_unref (renderer);
    }
}

static GtkCellRenderer *
gimp_container_tree_view_find_click_cell (GtkWidget         *widget,
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

      if (renderer->visible &&
          gtk_tree_view_column_cell_get_position (column, renderer,
                                                  &start, &width))
        {
          gint x;

          if (rtl)
            x = column_area->x + column_area->width - start - width;
          else
            x = start + column_area->x;

          if (tree_x >= x + renderer->xpad &&
              tree_x < x + width - renderer->xpad)
            {
              return renderer;
            }
        }
    }

  return NULL;
}

static gboolean
gimp_container_tree_view_button_press (GtkWidget             *widget,
                                       GdkEventButton        *bevent,
                                       GimpContainerTreeView *tree_view)
{
  GimpContainerView *container_view = GIMP_CONTAINER_VIEW (tree_view);
  GtkTreeViewColumn *column;
  GtkTreePath       *path;

  tree_view->dnd_renderer = NULL;

  if (! GTK_WIDGET_HAS_FOCUS (widget))
    gtk_widget_grab_focus (widget);

  if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget),
                                     bevent->x, bevent->y,
                                     &path, &column, NULL, NULL))
    {
      GimpViewRenderer         *renderer;
      GimpCellRendererToggle   *toggled_cell = NULL;
      GimpCellRendererViewable *clicked_cell = NULL;
      GtkCellRenderer          *edit_cell    = NULL;
      GdkRectangle              column_area;
      GtkTreeIter               iter;

      gtk_tree_model_get_iter (tree_view->model, &iter, path);

      gtk_tree_model_get (tree_view->model, &iter,
                          COLUMN_RENDERER, &renderer,
                          -1);

      tree_view->dnd_renderer = renderer;

      gtk_tree_view_get_background_area (tree_view->view, path,
                                         column, &column_area);

      gtk_tree_view_column_cell_set_cell_data (column,
                                               tree_view->model,
                                               &iter,
                                               FALSE, FALSE);

      toggled_cell = (GimpCellRendererToggle *)
        gimp_container_tree_view_find_click_cell (widget,
                                                  tree_view->toggle_cells,
                                                  column, &column_area,
                                                  bevent->x, bevent->y);

      if (! toggled_cell)
        clicked_cell = (GimpCellRendererViewable *)
          gimp_container_tree_view_find_click_cell (widget,
                                                    tree_view->renderer_cells,
                                                    column, &column_area,
                                                    bevent->x, bevent->y);

      if (! toggled_cell && ! clicked_cell)
        edit_cell =
          gimp_container_tree_view_find_click_cell (widget,
                                                    tree_view->editable_cells,
                                                    column, &column_area,
                                                    bevent->x, bevent->y);

      g_object_ref (tree_view);

      switch (bevent->button)
        {
        case 1:
          if (bevent->type == GDK_BUTTON_PRESS)
            {
              gboolean success = TRUE;

              /*  don't select item if a toggle was clicked */
              if (! toggled_cell)
                success = gimp_container_view_item_selected (container_view,
                                                             renderer->viewable);

              /*  a callback invoked by selecting the item may have
               *  destroyed us, so check if the container is still there
               */
              if (gimp_container_view_get_container (container_view))
                {
                  /*  another row may have been set by selecting  */
                  gtk_tree_view_column_cell_set_cell_data (column,
                                                           tree_view->model,
                                                           &iter,
                                                           FALSE, FALSE);

                  if (toggled_cell || clicked_cell)
                    {
                      gchar *path_str = gtk_tree_path_to_string (path);

                      if (toggled_cell)
                        {
                          gimp_cell_renderer_toggle_clicked (toggled_cell,
                                                             path_str,
                                                             bevent->state);
                        }
                      else if (clicked_cell)
                        {
                          gimp_cell_renderer_viewable_clicked (clicked_cell,
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
              if (! toggled_cell)
                success = gimp_container_view_item_selected (container_view,
                                                             renderer->viewable);

              if (success)
                {
                  if (edit_cell)
                    {
                      if (edit_cell == tree_view->name_cell)
                        {
                          const gchar *real_name;

                          real_name =
                            gimp_object_get_name (GIMP_OBJECT (renderer->viewable));

                          gtk_list_store_set (GTK_LIST_STORE (tree_view->model),
                                              &iter,
                                              tree_view->model_column_name,
                                              real_name,
                                              -1);
                        }

                      gtk_tree_view_set_cursor_on_cell (tree_view->view, path,
                                                        column, edit_cell, TRUE);
                    }
                  else if (! toggled_cell) /* ignore double click on toggles */
                    {
                      gimp_container_view_item_activated (container_view,
                                                          renderer->viewable);
                    }
                }
            }
          break;

        case 2:
          if (bevent->type == GDK_BUTTON_PRESS)
            {
              if (clicked_cell)
                {
                  gchar *path_str = gtk_tree_path_to_string (path);

                  gimp_cell_renderer_viewable_clicked (clicked_cell,
                                                       path_str,
                                                       bevent->state);

                  g_free (path_str);
                }
            }
          break;

        case 3:
          if (gimp_container_view_item_selected (container_view,
                                                 renderer->viewable))
            {
              if (gimp_container_view_get_container (container_view))
                gimp_container_view_item_context (container_view,
                                                  renderer->viewable);
            }
          break;

        default:
          break;
        }

      g_object_unref (tree_view);

      gtk_tree_path_free (path);
      g_object_unref (renderer);
    }
  else
    {
      switch (bevent->button)
        {
        case 3:
          gimp_editor_popup_menu (GIMP_EDITOR (tree_view), NULL, NULL);
          break;

        default:
          break;
        }
    }

  return TRUE;
}

static void
gimp_container_tree_view_renderer_update (GimpViewRenderer      *renderer,
                                          GimpContainerTreeView *tree_view)
{
  GimpContainerView *view = GIMP_CONTAINER_VIEW (tree_view);
  GtkTreeIter       *iter;

  iter = gimp_container_view_lookup (view, renderer->viewable);

  if (iter)
    {
      GtkTreePath *path;

      path = gtk_tree_model_get_path (tree_view->model, iter);
      gtk_tree_model_row_changed (tree_view->model, path, iter);
      gtk_tree_path_free (path);
    }
}

static GimpViewable *
gimp_container_tree_view_drag_viewable (GtkWidget    *widget,
                                        GimpContext **context,
                                        gpointer      data)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (data);

  if (context)
    *context = gimp_container_view_get_context (GIMP_CONTAINER_VIEW (data));

  if (tree_view->dnd_renderer)
    return tree_view->dnd_renderer->viewable;

  return NULL;
}

static GdkPixbuf *
gimp_container_tree_view_drag_pixbuf (GtkWidget *widget,
                                      gpointer   data)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (data);
  GimpViewRenderer      *renderer  = tree_view->dnd_renderer;
  gint                   width;
  gint                   height;

  if (renderer && gimp_viewable_get_size (renderer->viewable, &width, &height))
    return gimp_viewable_get_new_pixbuf (renderer->viewable,
                                         renderer->context,
                                         width, height);

  return NULL;
}
