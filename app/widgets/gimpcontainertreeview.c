/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainertreeview.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#include "widgets-types.h"

#include "base/temp-buf.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpviewable.h"

#include "gimpcellrendererviewable.h"
#include "gimpcontainertreeview.h"
#include "gimpdnd.h"
#include "gimppreview.h"
#include "gimppreviewrenderer.h"


enum
{
  COLUMN_VIEWABLE,
  COLUMN_RENDERER,
  COLUMN_NAME,
  NUM_COLUMNS
};


static void     gimp_container_tree_view_class_init   (GimpContainerTreeViewClass *klass);
static void     gimp_container_tree_view_init         (GimpContainerTreeView      *view);

static void    gimp_container_tree_view_set_container (GimpContainerView      *view,
                                                       GimpContainer          *container);

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
static void     gimp_container_tree_view_select_item  (GimpContainerView      *view,
						       GimpViewable           *viewable,
						       gpointer                insert_data);
static void     gimp_container_tree_view_clear_items  (GimpContainerView      *view);
static void gimp_container_tree_view_set_preview_size (GimpContainerView      *view);

static void gimp_container_tree_view_selection_changed (GtkTreeSelection      *sel,
                                                        GimpContainerTreeView *tree_view);
static gboolean gimp_container_tree_view_button_press (GtkWidget              *widget,
						       GdkEventButton         *bevent,
						       GimpContainerTreeView  *tree_view);

static void gimp_container_tree_view_renderer_update  (GimpPreviewRenderer    *renderer,
                                                       GimpContainerTreeView  *tree_view);
static void gimp_container_tree_view_name_changed     (GimpObject             *object,
                                                       GimpContainerTreeView  *tree_view);

static GimpViewable * gimp_container_tree_view_drag_viewable (GtkWidget       *widget,
                                                              gpointer         data);


static GimpContainerViewClass *parent_class = NULL;


GType
gimp_container_tree_view_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpContainerTreeViewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_container_tree_view_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpContainerTreeView),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_container_tree_view_init,
      };

      view_type = g_type_register_static (GIMP_TYPE_CONTAINER_VIEW,
                                          "GimpContainerTreeView",
                                          &view_info, 0);
    }

  return view_type;
}

static void
gimp_container_tree_view_class_init (GimpContainerTreeViewClass *klass)
{
  GimpContainerViewClass *container_view_class;

  container_view_class = GIMP_CONTAINER_VIEW_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  container_view_class->set_container    = gimp_container_tree_view_set_container;
  container_view_class->insert_item      = gimp_container_tree_view_insert_item;
  container_view_class->remove_item      = gimp_container_tree_view_remove_item;
  container_view_class->reorder_item     = gimp_container_tree_view_reorder_item;
  container_view_class->select_item      = gimp_container_tree_view_select_item;
  container_view_class->clear_items      = gimp_container_tree_view_clear_items;
  container_view_class->set_preview_size = gimp_container_tree_view_set_preview_size;

  container_view_class->insert_data_free = (GDestroyNotify) g_free;
}

static void
gimp_container_tree_view_init (GimpContainerTreeView *tree_view)
{
  GtkTreeViewColumn *column;
  GtkCellRenderer   *cell;

  tree_view->scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (tree_view->scrolled_win),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (tree_view->scrolled_win), 
                                  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (tree_view), tree_view->scrolled_win,
		      TRUE, TRUE, 0);
  gtk_widget_show (tree_view->scrolled_win);

  tree_view->list = gtk_list_store_new (NUM_COLUMNS,
                                        GIMP_TYPE_VIEWABLE,
                                        GIMP_TYPE_PREVIEW_RENDERER,
                                        G_TYPE_STRING);
  tree_view->view = GTK_TREE_VIEW
    (gtk_tree_view_new_with_model (GTK_TREE_MODEL (tree_view->list)));
  g_object_unref (tree_view->list);

  GIMP_CONTAINER_VIEW (tree_view)->dnd_widget = GTK_WIDGET (tree_view->view);

  gtk_tree_view_set_headers_visible (tree_view->view, FALSE);

  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (tree_view->view, column);

  cell = gimp_cell_renderer_viewable_new ();
  gtk_tree_view_column_pack_start (column, cell, FALSE);
  gtk_tree_view_column_set_attributes (column, cell,
                                       "renderer", COLUMN_RENDERER,
                                       NULL);

  gtk_tree_view_insert_column_with_attributes (tree_view->view,
                                               1, NULL,
                                               gtk_cell_renderer_text_new (),
                                               "text", COLUMN_NAME,
                                               NULL);

  gtk_container_add (GTK_CONTAINER (tree_view->scrolled_win),
                     GTK_WIDGET (tree_view->view));
  gtk_widget_show (GTK_WIDGET (tree_view->view));

  GTK_WIDGET_UNSET_FLAGS (GTK_SCROLLED_WINDOW
			  (tree_view->scrolled_win)->vscrollbar,
                          GTK_CAN_FOCUS);

  tree_view->selection = gtk_tree_view_get_selection (tree_view->view);

  g_signal_connect (tree_view->selection, "changed",
                    G_CALLBACK (gimp_container_tree_view_selection_changed),
                    tree_view);
}

GtkWidget *
gimp_container_tree_view_new (GimpContainer *container,
                              GimpContext   *context,
			      gint           preview_size,
                              gboolean       reorderable,
			      gint           min_items_x,
			      gint           min_items_y)
{
  GimpContainerTreeView *tree_view;
  GimpContainerView     *view;
  gint                   window_border;

  g_return_val_if_fail (! container || GIMP_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (! context || GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (preview_size > 0 &&
                        preview_size <= GIMP_PREVIEW_MAX_SIZE, NULL);
  g_return_val_if_fail (min_items_x > 0 && min_items_x <= 64, NULL);
  g_return_val_if_fail (min_items_y > 0 && min_items_y <= 64, NULL);

  tree_view = g_object_new (GIMP_TYPE_CONTAINER_TREE_VIEW, NULL);

  view = GIMP_CONTAINER_VIEW (tree_view);

  view->preview_size = preview_size;
  view->reorderable  = reorderable ? TRUE : FALSE;

  window_border =
    GTK_SCROLLED_WINDOW (tree_view->scrolled_win)->vscrollbar->requisition.width +
    GTK_SCROLLED_WINDOW_GET_CLASS (tree_view->scrolled_win)->scrollbar_spacing +
    tree_view->scrolled_win->style->xthickness * 4;

  gtk_widget_set_size_request (GTK_WIDGET (tree_view),
                               (preview_size + 2) * min_items_x + window_border,
                               (preview_size + 6) * min_items_y + window_border);

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
  GimpContainerView   *view;
  GimpPreviewRenderer *renderer;
  gchar               *name;

  view = GIMP_CONTAINER_VIEW (tree_view);

  if (view->get_name_func)
    name = view->get_name_func (G_OBJECT (viewable), NULL);
  else
    name = g_strdup (gimp_object_get_name (GIMP_OBJECT (viewable)));

  renderer = gimp_preview_renderer_new (G_TYPE_FROM_INSTANCE (viewable),
                                        view->preview_size, 1, FALSE);
  gimp_preview_renderer_set_viewable (renderer, viewable);
  gimp_preview_renderer_remove_idle (renderer);

  g_signal_connect (renderer, "update",
                    G_CALLBACK (gimp_container_tree_view_renderer_update),
                    tree_view);

  gtk_list_store_set (tree_view->list, iter,
                      COLUMN_VIEWABLE, viewable,
                      COLUMN_RENDERER, renderer,
                      COLUMN_NAME,     name,
                      -1);

  g_object_unref (renderer);
  g_free (name);
}

/*  GimpContainerView methods  */

static void
gimp_container_tree_view_set_container (GimpContainerView *view,
                                        GimpContainer     *container)
{
  GimpContainerTreeView *tree_view;

  tree_view = GIMP_CONTAINER_TREE_VIEW (view);

  if (view->container)
    {
      gimp_container_remove_handler (view->container,
                                     tree_view->name_changed_handler_id);

      if (! container)
        {
          if (gimp_dnd_viewable_source_unset (GTK_WIDGET (tree_view->view),
                                              view->container->children_type))
            {
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
          gimp_dnd_viewable_source_set (GTK_WIDGET (tree_view->view),
                                        container->children_type,
                                        gimp_container_tree_view_drag_viewable,
                                        tree_view);
        }

      /*  connect button_press_event after DND so we can keep the list from
       *  selecting the item on button2
       */
      g_signal_connect (tree_view->view, "button_press_event",
                        G_CALLBACK (gimp_container_tree_view_button_press),
                        tree_view);
    }

  GIMP_CONTAINER_VIEW_CLASS (parent_class)->set_container (view, container);

  if (view->container)
    {
      GimpViewableClass *viewable_class;

      viewable_class = g_type_class_ref (container->children_type);

      tree_view->name_changed_handler_id =
        gimp_container_add_handler (view->container,
                                    viewable_class->name_changed_signal,
                                    G_CALLBACK (gimp_container_tree_view_name_changed),
                                    tree_view);

      g_type_class_unref (viewable_class);
    }
}

static gpointer
gimp_container_tree_view_insert_item (GimpContainerView *view,
				      GimpViewable      *viewable,
				      gint               index)
{
  GimpContainerTreeView *tree_view;
  GtkTreeIter           *iter;

  tree_view = GIMP_CONTAINER_TREE_VIEW (view);

  iter = g_new0 (GtkTreeIter, 1);

  if (index == -1)
    gtk_list_store_append (tree_view->list, iter);
  else
    gtk_list_store_insert (tree_view->list, iter, index);

  gimp_container_tree_view_set (tree_view, iter, viewable);

  return (gpointer) iter;
}

static void
gimp_container_tree_view_remove_item (GimpContainerView *view,
				      GimpViewable      *viewable,
				      gpointer           insert_data)
{
  GimpContainerTreeView *tree_view;
  GtkTreeIter           *iter;

  tree_view = GIMP_CONTAINER_TREE_VIEW (view);

  iter = (GtkTreeIter *) insert_data;

  if (iter)
    gtk_list_store_remove (tree_view->list, iter);
}

static void
gimp_container_tree_view_reorder_item (GimpContainerView *view,
				       GimpViewable      *viewable,
				       gint               new_index,
				       gpointer           insert_data)
{
  GimpContainerTreeView *tree_view;
  GtkTreeIter           *iter;

  tree_view = GIMP_CONTAINER_TREE_VIEW (view);

  iter = (GtkTreeIter *) insert_data;

  if (iter)
    {
      GtkTreeModel *model;
      GtkTreePath  *path;
      GtkTreeIter   selected_iter;
      gboolean      selected;

      model = GTK_TREE_MODEL (tree_view->list);

      selected = gtk_tree_selection_get_selected (tree_view->selection,
                                                  NULL, &selected_iter);

      if (selected)
        {
          GimpViewable *selected_viewable;

          gtk_tree_model_get (model, &selected_iter,
                              COLUMN_VIEWABLE, &selected_viewable,
                              -1);

          if (selected_viewable != viewable)
            selected = FALSE;

          g_object_unref (selected_viewable);
        }

      if (new_index == -1 || new_index == view->container->num_children - 1)
        {
          gtk_list_store_move_before (tree_view->list, iter, NULL);
        }
      else if (new_index == 0)
        {
          gtk_list_store_move_after (tree_view->list, iter, NULL);
        }
      else
        {
          GtkTreeIter place_iter;
          gint        old_index;

          path = gtk_tree_model_get_path (model, iter);
          old_index = *(gtk_tree_path_get_indices (path));
          gtk_tree_path_free (path);

          if (new_index != old_index)
            {
              path = gtk_tree_path_new_from_indices (new_index, -1);
              gtk_tree_model_get_iter (model, &place_iter, path);
              gtk_tree_path_free (path);

              if (new_index > old_index)
                gtk_list_store_move_after (tree_view->list, iter, &place_iter);
              else
                gtk_list_store_move_before (tree_view->list, iter, &place_iter);
            }
        }

      if (selected)
        {
          path = gtk_tree_model_get_path (model, iter);
          gtk_tree_view_scroll_to_cell (tree_view->view, path,
                                        NULL, FALSE, 0.0, 0.0);
          gtk_tree_path_free (path);
        }
    }
}

static void
gimp_container_tree_view_select_item (GimpContainerView *view,
				      GimpViewable      *viewable,
				      gpointer           insert_data)
{
  GimpContainerTreeView *tree_view;
  GtkTreeIter           *iter;

  tree_view = GIMP_CONTAINER_TREE_VIEW (view);

  iter = (GtkTreeIter *) insert_data;

  if (iter)
    {
      GtkTreePath *path;
      GtkTreeIter  selected_iter;

      if (gtk_tree_selection_get_selected (tree_view->selection, NULL,
                                           &selected_iter))
        {
          GimpViewable *selected_viewable;
          gboolean      equal;

          gtk_tree_model_get (GTK_TREE_MODEL (tree_view->list), &selected_iter,
                              COLUMN_VIEWABLE, &selected_viewable,
                              -1);

          equal = (selected_viewable == viewable);

          g_object_unref (selected_viewable);

          if (equal)
            return;
        }

      g_signal_handlers_block_by_func (tree_view->selection,
				       gimp_container_tree_view_selection_changed,
				       tree_view);

      gtk_tree_selection_select_iter (tree_view->selection, iter);

      g_signal_handlers_unblock_by_func (tree_view->selection,
					 gimp_container_tree_view_selection_changed,
					 tree_view);

      path = gtk_tree_model_get_path (GTK_TREE_MODEL (tree_view->list), iter);

      gtk_tree_view_scroll_to_cell (tree_view->view, path,
                                    NULL, FALSE, 0.0, 0.0);

      gtk_tree_path_free (path);
    }
  else
    {
      gtk_tree_selection_unselect_all (tree_view->selection);
    }
}

static void
gimp_container_tree_view_clear_items (GimpContainerView *view)
{
  GimpContainerTreeView *tree_view;

  tree_view = GIMP_CONTAINER_TREE_VIEW (view);

  gtk_list_store_clear (tree_view->list);

  GIMP_CONTAINER_VIEW_CLASS (parent_class)->clear_items (view);
}

static void
gimp_container_tree_view_set_preview_size (GimpContainerView *view)
{
  GimpContainerTreeView *tree_view;
  GtkTreeModel          *tree_model;
  GtkTreeIter            iter;
  gboolean               iter_valid;

  tree_view  = GIMP_CONTAINER_TREE_VIEW (view);
  tree_model = GTK_TREE_MODEL (tree_view->list);

  for (iter_valid = gtk_tree_model_get_iter_first (tree_model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (tree_model, &iter))
    {
      GimpPreviewRenderer *renderer;

      gtk_tree_model_get (GTK_TREE_MODEL (tree_view->list), &iter,
                          COLUMN_RENDERER, &renderer,
                          -1);

      gimp_preview_renderer_set_size (renderer, view->preview_size, 1);

      gtk_list_store_set (tree_view->list, &iter,
                          COLUMN_RENDERER, renderer,
                          -1);

      g_object_unref (renderer);
    }
}


/*  callbacks  */

static void
gimp_container_tree_view_selection_changed (GtkTreeSelection      *selection,
                                            GimpContainerTreeView *tree_view)
{
  GimpViewable *selected_viewable = NULL;
  GtkTreeIter   selected_iter;
  gboolean      selected;

  selected = gtk_tree_selection_get_selected (selection, NULL, &selected_iter);

  if (selected)
    gtk_tree_model_get (GTK_TREE_MODEL (tree_view->list), &selected_iter,
                        COLUMN_VIEWABLE, &selected_viewable,
                        -1);

  if (selected_viewable)
    {
      gimp_container_view_item_selected (GIMP_CONTAINER_VIEW (tree_view),
                                         selected_viewable);
      g_object_unref (selected_viewable);
    }
}

static gboolean
gimp_container_tree_view_button_press (GtkWidget             *widget,
                                       GdkEventButton        *bevent,
                                       GimpContainerTreeView *tree_view)
{
  GtkTreePath *path;
  gboolean     retval = FALSE;

  tree_view->dnd_viewable = NULL;

  if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget),
                                     bevent->x,
                                     bevent->y,
                                     &path, NULL, NULL, NULL))
    {
      GimpViewable *viewable;
      GtkTreeIter   iter;

      gtk_tree_model_get_iter (GTK_TREE_MODEL (tree_view->list), &iter, path);

      gtk_tree_path_free (path);

      gtk_tree_model_get (GTK_TREE_MODEL (tree_view->list), &iter,
                          COLUMN_VIEWABLE, &viewable,
                          -1);

      tree_view->dnd_viewable = viewable;

      switch (bevent->button)
        {
        case 1:
          if (bevent->type == GDK_2BUTTON_PRESS)
            {
              gimp_container_view_item_activated (GIMP_CONTAINER_VIEW (tree_view),
                                                  viewable);
              retval = TRUE;
            }
          break;

        case 2:
          retval = TRUE; /* we want to button2-drag without selecting */
          break;

        case 3:
          gimp_container_view_item_context (GIMP_CONTAINER_VIEW (tree_view),
                                            viewable);
          break;

        default:
          break;
        }

      g_object_unref (viewable);
    }

  return retval;
}

static void
gimp_container_tree_view_renderer_update (GimpPreviewRenderer   *renderer,
                                          GimpContainerTreeView *tree_view)
{
  GimpContainerView *view;
  GtkTreeIter       *iter;

  view = GIMP_CONTAINER_VIEW (tree_view);

  iter = g_hash_table_lookup (view->hash_table, renderer->viewable);

  if (iter)
    {
      GtkTreePath *path;

      path = gtk_tree_model_get_path (GTK_TREE_MODEL (tree_view->list), iter);

      gtk_tree_model_row_changed (GTK_TREE_MODEL (tree_view->list),
                                  path, iter);

      gtk_tree_path_free (path);
    }
}

static void
gimp_container_tree_view_name_changed (GimpObject            *object,
                                       GimpContainerTreeView *tree_view)
{
  GimpContainerView *view;
  GtkTreeIter       *iter;

  view = GIMP_CONTAINER_VIEW (tree_view);

  iter = g_hash_table_lookup (view->hash_table, object);

  if (iter)
    {
      gchar *name;

      if (view->get_name_func)
        name = view->get_name_func (G_OBJECT (object), NULL);
      else
        name = g_strdup (gimp_object_get_name (object));

      gtk_list_store_set (tree_view->list, iter,
                          COLUMN_NAME, name,
                          -1);

      g_free (name);
    }
}

static GimpViewable *
gimp_container_tree_view_drag_viewable (GtkWidget *widget,
                                        gpointer   data)
{
  GimpContainerTreeView *tree_view;

  tree_view = GIMP_CONTAINER_TREE_VIEW (data);

  return tree_view->dnd_viewable;
}
