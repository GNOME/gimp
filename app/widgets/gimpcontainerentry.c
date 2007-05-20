/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainerentry.c
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

#include <string.h>

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpviewable.h"

#include "gimpcellrendererviewable.h"
#include "gimpcontainerentry.h"
#include "gimpcontainerview.h"
#include "gimpviewrenderer.h"


#define gimp_container_entry_get_model(entry) \
  gtk_entry_completion_get_model (gtk_entry_get_completion (GTK_ENTRY (entry)))


static void     gimp_container_entry_view_iface_init (GimpContainerViewInterface *iface);

static void     gimp_container_entry_set_context  (GimpContainerView      *view,
                                                   GimpContext            *context);
static gpointer gimp_container_entry_insert_item  (GimpContainerView      *view,
                                                   GimpViewable           *viewable,
                                                   gint                    index);
static void     gimp_container_entry_remove_item  (GimpContainerView      *view,
                                                   GimpViewable           *viewable,
                                                   gpointer                insert_data);
static void     gimp_container_entry_reorder_item (GimpContainerView      *view,
                                                   GimpViewable           *viewable,
                                                   gint                    new_index,
                                                   gpointer                insert_data);
static void     gimp_container_entry_rename_item  (GimpContainerView      *view,
                                                   GimpViewable           *viewable,
                                                   gpointer                insert_data);
static gboolean  gimp_container_entry_select_item (GimpContainerView      *view,
                                                   GimpViewable           *viewable,
                                                   gpointer                insert_data);
static void     gimp_container_entry_clear_items  (GimpContainerView      *view);
static void    gimp_container_entry_set_view_size (GimpContainerView      *view);

static void     gimp_container_entry_changed      (GtkEntry               *entry,
                                                   GimpContainerView      *view);
static void   gimp_container_entry_match_selected (GtkEntryCompletion     *widget,
                                                   GtkTreeModel           *model,
                                                   GtkTreeIter            *iter,
                                                   GimpContainerView      *view);
static void gimp_container_entry_renderer_update  (GimpViewRenderer       *renderer,
                                                   GimpContainerView      *view);


G_DEFINE_TYPE_WITH_CODE (GimpContainerEntry, gimp_container_entry,
                         GTK_TYPE_ENTRY,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONTAINER_VIEW,
                                                gimp_container_entry_view_iface_init))

#define parent_class gimp_container_entry_parent_class

static GimpContainerViewInterface *parent_view_iface = NULL;


static void
gimp_container_entry_class_init (GimpContainerEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_container_view_set_property;
  object_class->get_property = gimp_container_view_get_property;

  gimp_container_view_install_properties (object_class);
}

static void
gimp_container_entry_view_iface_init (GimpContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  if (! parent_view_iface)
    parent_view_iface = g_type_default_interface_peek (GIMP_TYPE_CONTAINER_VIEW);

  iface->set_context   = gimp_container_entry_set_context;
  iface->insert_item   = gimp_container_entry_insert_item;
  iface->remove_item   = gimp_container_entry_remove_item;
  iface->reorder_item  = gimp_container_entry_reorder_item;
  iface->rename_item   = gimp_container_entry_rename_item;
  iface->select_item   = gimp_container_entry_select_item;
  iface->clear_items   = gimp_container_entry_clear_items;
  iface->set_view_size = gimp_container_entry_set_view_size;

  iface->insert_data_free = (GDestroyNotify) gtk_tree_iter_free;
}

static void
gimp_container_entry_init (GimpContainerEntry *entry)
{
  GtkEntryCompletion *completion;
  GtkListStore       *store;
  GtkCellRenderer    *cell;

  completion = g_object_new (GTK_TYPE_ENTRY_COMPLETION,
                             "inline-completion",  TRUE,
                             "popup-single-match", FALSE,
                             "popup-set-width",    FALSE,
                             NULL);

  store = gtk_list_store_new (GIMP_CONTAINER_ENTRY_NUM_COLUMNS,
                              GIMP_TYPE_VIEW_RENDERER,
                              G_TYPE_STRING);

  gtk_entry_completion_set_model (completion, GTK_TREE_MODEL (store));
  g_object_unref (store);

  gtk_entry_set_completion (GTK_ENTRY (entry), completion);

  g_signal_connect (completion, "match-selected",
                    G_CALLBACK (gimp_container_entry_match_selected),
                    entry);

  g_object_unref (completion);

  /*  FIXME: This can be done better with GTK+ 2.6.  */

  cell = gimp_cell_renderer_viewable_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (completion), cell, FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (completion), cell,
                                  "renderer",
                                  GIMP_CONTAINER_ENTRY_COLUMN_RENDERER,
                                  NULL);

  gtk_entry_completion_set_text_column (completion,
                                        GIMP_CONTAINER_ENTRY_COLUMN_NAME);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (gimp_container_entry_changed),
                    entry);
}

GtkWidget *
gimp_container_entry_new (GimpContainer *container,
                          GimpContext   *context,
                          gint           view_size,
                          gint           view_border_width)
{
  GtkWidget         *entry;
  GimpContainerView *view;

  g_return_val_if_fail (container == NULL || GIMP_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (context == NULL || GIMP_IS_CONTEXT (context), NULL);

  entry = g_object_new (GIMP_TYPE_CONTAINER_ENTRY, NULL);

  view = GIMP_CONTAINER_VIEW (entry);

  gimp_container_view_set_view_size (view, view_size, view_border_width);

  if (container)
    gimp_container_view_set_container (view, container);

  if (context)
    gimp_container_view_set_context (view, context);

  return entry;
}

static void
gimp_container_entry_set (GimpContainerEntry *entry,
                          GtkTreeIter        *iter,
                          GimpViewable       *viewable)
{
  GimpContainerView *view  = GIMP_CONTAINER_VIEW (entry);
  GtkTreeModel      *model = gimp_container_entry_get_model (entry);
  GimpViewRenderer  *renderer;
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
                    G_CALLBACK (gimp_container_entry_renderer_update),
                    view);

  gtk_list_store_set (GTK_LIST_STORE (model), iter,
                      GIMP_CONTAINER_ENTRY_COLUMN_RENDERER,
                      renderer,
                      GIMP_CONTAINER_ENTRY_COLUMN_NAME,
                      gimp_object_get_name (GIMP_OBJECT (viewable)),
                      -1);

  g_object_unref (renderer);
}


/*  GimpContainerView methods  */

static void
gimp_container_entry_set_context (GimpContainerView *view,
                                  GimpContext       *context)
{
  GtkTreeModel *model = gimp_container_entry_get_model (view);

  parent_view_iface->set_context (view, context);

  if (model)
    {
      GtkTreeIter iter;
      gboolean    iter_valid;

      for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
           iter_valid;
           iter_valid = gtk_tree_model_iter_next (model, &iter))
        {
          GimpViewRenderer *renderer;

          gtk_tree_model_get (model, &iter,
                              GIMP_CONTAINER_ENTRY_COLUMN_RENDERER, &renderer,
                              -1);

          gimp_view_renderer_set_context (renderer, context);
          g_object_unref (renderer);
        }
    }
}

static gpointer
gimp_container_entry_insert_item (GimpContainerView *view,
                                  GimpViewable      *viewable,
                                  gint               index)
{
  GtkTreeModel *model = gimp_container_entry_get_model (view);
  GtkTreeIter   iter;

  if (index == -1)
    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  else
    gtk_list_store_insert (GTK_LIST_STORE (model), &iter, index);

  gimp_container_entry_set (GIMP_CONTAINER_ENTRY (view), &iter, viewable);

  return gtk_tree_iter_copy (&iter);
}

static void
gimp_container_entry_remove_item (GimpContainerView *view,
                                  GimpViewable      *viewable,
                                  gpointer           insert_data)
{
  GtkTreeModel *model = gimp_container_entry_get_model (view);
  GtkTreeIter  *iter  = insert_data;

  if (iter)
    gtk_list_store_remove (GTK_LIST_STORE (model), iter);
}

static void
gimp_container_entry_reorder_item (GimpContainerView *view,
                                   GimpViewable      *viewable,
                                   gint               new_index,
                                   gpointer           insert_data)
{
  GtkTreeModel  *model     = gimp_container_entry_get_model (view);
  GimpContainer *container = gimp_container_view_get_container (view);
  GtkTreeIter   *iter      = insert_data;

  if (!iter)
    return;

  if (new_index == -1 || new_index == container->num_children - 1)
    {
      gtk_list_store_move_before (GTK_LIST_STORE (model), iter, NULL);
    }
  else if (new_index == 0)
    {
      gtk_list_store_move_after (GTK_LIST_STORE (model), iter, NULL);
    }
  else
    {
      GtkTreePath *path;
      gint         old_index;

      path = gtk_tree_model_get_path (model, iter);
      old_index = gtk_tree_path_get_indices (path)[0];
      gtk_tree_path_free (path);

      if (new_index != old_index)
        {
          GtkTreeIter  place;

          path = gtk_tree_path_new_from_indices (new_index, -1);
          gtk_tree_model_get_iter (model, &place, path);
          gtk_tree_path_free (path);

          if (new_index > old_index)
            gtk_list_store_move_after (GTK_LIST_STORE (model), iter, &place);
          else
            gtk_list_store_move_before (GTK_LIST_STORE (model), iter, &place);
        }
    }
}

static void
gimp_container_entry_rename_item (GimpContainerView *view,
                                  GimpViewable      *viewable,
                                  gpointer           insert_data)
{
  GtkTreeModel *model = gimp_container_entry_get_model (view);
  GtkTreeIter  *iter  = insert_data;

  if (iter)
    gtk_list_store_set (GTK_LIST_STORE (model), iter,
                        GIMP_CONTAINER_ENTRY_COLUMN_NAME,
                        gimp_object_get_name (GIMP_OBJECT (viewable)),
                        -1);
}

static gboolean
gimp_container_entry_select_item (GimpContainerView *view,
                                  GimpViewable      *viewable,
                                  gpointer           insert_data)
{
  GtkEntry    *entry = GTK_ENTRY (view);
  GtkTreeIter *iter  = insert_data;

  g_signal_handlers_block_by_func (entry,
                                   gimp_container_entry_changed,
                                   view);

  gtk_entry_set_text (entry,
                      iter ?
                      gimp_object_get_name (GIMP_OBJECT (viewable)) : "");

  g_signal_handlers_unblock_by_func (entry,
                                     gimp_container_entry_changed,
                                     view);

  return TRUE;
}

static void
gimp_container_entry_clear_items (GimpContainerView *view)
{
  GtkTreeModel *model = gimp_container_entry_get_model (view);

  gtk_list_store_clear (GTK_LIST_STORE (model));

  parent_view_iface->clear_items (view);
}

static void
gimp_container_entry_set_view_size (GimpContainerView *view)
{
  GtkTreeModel *model = gimp_container_entry_get_model (view);
  GtkTreeIter   iter;
  gboolean      iter_valid;
  gint          view_size;
  gint          border_width;

  if (! model)
    return;

  view_size = gimp_container_view_get_view_size (view, &border_width);

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      GimpViewRenderer *renderer;

      gtk_tree_model_get (model, &iter,
                          GIMP_CONTAINER_ENTRY_COLUMN_RENDERER, &renderer,
                          -1);

      gimp_view_renderer_set_size (renderer, view_size, border_width);
      g_object_unref (renderer);
    }
}

static void
gimp_container_entry_changed (GtkEntry          *entry,
                              GimpContainerView *view)
{
  GimpContainer *container = gimp_container_view_get_container (view);
  GimpObject    *object;
  const gchar   *text;

  if (! container)
    return;

  text = gtk_entry_get_text (entry);

  object = gimp_container_get_child_by_name (container, text);

  if (object)
    gimp_container_view_item_selected (view, GIMP_VIEWABLE (object));
}

static void
gimp_container_entry_match_selected (GtkEntryCompletion *widget,
                                     GtkTreeModel       *model,
                                     GtkTreeIter        *iter,
                                     GimpContainerView  *view)
{
  GimpViewRenderer *renderer;

  gtk_tree_model_get (model, iter,
                      GIMP_CONTAINER_ENTRY_COLUMN_RENDERER, &renderer,
                      -1);

  gimp_container_view_item_selected (view, renderer->viewable);
  g_object_unref (renderer);
}

static void
gimp_container_entry_renderer_update (GimpViewRenderer  *renderer,
                                      GimpContainerView *view)
{
  GtkTreeIter *iter = gimp_container_view_lookup (view, renderer->viewable);

  if (iter)
    {
      GtkTreeModel *model = gimp_container_entry_get_model (view);
      GtkTreePath  *path  = gtk_tree_model_get_path (model, iter);

      gtk_tree_model_row_changed (model, path, iter);
      gtk_tree_path_free (path);
    }
}
