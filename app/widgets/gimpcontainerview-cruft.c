/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainerview-cruft.c
 * Copyright (C) 2001-2025 Michael Natterer <mitch@gimp.org>
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

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimptreehandler.h"

#include "gimpcontainerview.h"
#include "gimpcontainerview-cruft.h"
#include "gimpcontainerview-private.h"


/*  local function prototypes  */

static void   gimp_container_view_clear_items      (GimpContainerView  *view);

static void   gimp_container_view_add_container    (GimpContainerView  *view,
                                                    GimpContainer      *container);
static void   gimp_container_view_add_foreach      (GimpViewable       *viewable,
                                                    GimpContainerView  *view);
static void   gimp_container_view_add              (GimpContainerView  *view,
                                                    GimpViewable       *viewable,
                                                    GimpContainer      *container);

static void   gimp_container_view_remove_container (GimpContainerView  *view,
                                                    GimpContainer      *container);
static void   gimp_container_view_remove_foreach   (GimpViewable       *viewable,
                                                    GimpContainerView  *view);
static void   gimp_container_view_remove           (GimpContainerView  *view,
                                                    GimpViewable       *viewable,
                                                    GimpContainer      *container);

static void   gimp_container_view_reorder          (GimpContainerView  *view,
                                                    GimpViewable       *viewable,
                                                    gint                old_index,
                                                    gint                new_index,
                                                    GimpContainer      *container);

static void   gimp_container_view_freeze           (GimpContainerView  *view,
                                                    GimpContainer      *container);
static void   gimp_container_view_thaw             (GimpContainerView  *view,
                                                    GimpContainer      *container);
static void   gimp_container_view_name_changed     (GimpViewable       *viewable,
                                                    GimpContainerView  *view);
static void   gimp_container_view_expanded_changed (GimpViewable       *viewable,
                                                    GimpContainerView  *view);


/*  public functions  */

void
_gimp_container_view_connect_cruft (GimpContainerView *view)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  if (! gimp_container_frozen (private->container))
    gimp_container_view_add_container (view, private->container);

  g_signal_connect_object (private->container, "freeze",
                           G_CALLBACK (gimp_container_view_freeze),
                           view,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (private->container, "thaw",
                           G_CALLBACK (gimp_container_view_thaw),
                           view,
                           G_CONNECT_SWAPPED);
}

void
_gimp_container_view_disconnect_cruft (GimpContainerView *view)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  g_signal_handlers_disconnect_by_func (private->container,
                                        gimp_container_view_freeze,
                                        view);
  g_signal_handlers_disconnect_by_func (private->container,
                                        gimp_container_view_thaw,
                                        view);

  if (! gimp_container_frozen (private->container))
    gimp_container_view_remove_container (view, private->container);
}

void
_gimp_container_view_real_clear_items (GimpContainerView *view)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  g_hash_table_remove_all (private->item_hash);
}

gpointer
_gimp_container_view_lookup (GimpContainerView *view,
                             GimpViewable      *viewable)
{
  GimpContainerViewPrivate *private;

  g_return_val_if_fail (GIMP_IS_CONTAINER_VIEW (view), NULL);
  g_return_val_if_fail (viewable == NULL || GIMP_IS_VIEWABLE (viewable), NULL);

  /*  we handle the NULL viewable here as a workaround for bug #149906 */
  if (! viewable)
    return NULL;

  private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  return g_hash_table_lookup (private->item_hash, viewable);
}

gboolean
_gimp_container_view_contains (GimpContainerView *view,
                               GList             *viewables)
{
  GimpContainerViewPrivate *private;
  GList                    *iter;

  g_return_val_if_fail (GIMP_IS_CONTAINER_VIEW (view), FALSE);
  g_return_val_if_fail (viewables, FALSE);

  private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  for (iter = viewables; iter; iter = iter->next)
    {
      if (! g_hash_table_contains (private->item_hash, iter->data))
        return FALSE;
    }

  return TRUE;
}


/*  private functions  */

static void
gimp_container_view_clear_items (GimpContainerView *view)
{
  GIMP_CONTAINER_VIEW_GET_IFACE (view)->clear_items (view);
}

static void
gimp_container_view_add_container (GimpContainerView *view,
                                   GimpContainer     *container)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  gimp_container_foreach (container,
                          (GFunc) gimp_container_view_add_foreach,
                          view);

  if (container == private->container)
    {
      GType              child_type;
      GimpViewableClass *viewable_class;

      child_type  = gimp_container_get_child_type (container);
      viewable_class = g_type_class_ref (child_type);

      private->name_changed_handler =
        gimp_tree_handler_connect (container,
                                   viewable_class->name_changed_signal,
                                   G_CALLBACK (gimp_container_view_name_changed),
                                   view);

      if (GIMP_CONTAINER_VIEW_GET_IFACE (view)->expand_item)
        {
          private->expanded_changed_handler =
            gimp_tree_handler_connect (container,
                                       "expanded-changed",
                                       G_CALLBACK (gimp_container_view_expanded_changed),
                                       view);
        }

      g_type_class_unref (viewable_class);
    }

  g_signal_connect_object (container, "add",
                           G_CALLBACK (gimp_container_view_add),
                           view,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (container, "remove",
                           G_CALLBACK (gimp_container_view_remove),
                           view,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (container, "reorder",
                           G_CALLBACK (gimp_container_view_reorder),
                           view,
                           G_CONNECT_SWAPPED);
}

static void
gimp_container_view_add_foreach (GimpViewable      *viewable,
                                 GimpContainerView *view)
{
  GimpContainerViewInterface *view_iface;
  GimpContainerViewPrivate   *private;
  GimpViewable               *parent;
  GimpContainer              *children;
  gpointer                    parent_insert_data = NULL;
  gpointer                    insert_data;

  view_iface = GIMP_CONTAINER_VIEW_GET_IFACE (view);
  private    = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  parent = gimp_viewable_get_parent (viewable);

  if (parent)
    parent_insert_data = g_hash_table_lookup (private->item_hash, parent);

  insert_data = view_iface->insert_item (view, viewable,
                                         parent_insert_data, -1);
  g_hash_table_insert (private->item_hash, viewable, insert_data);

  children = gimp_viewable_get_children (viewable);

  if (children)
    gimp_container_view_add_container (view, children);
}

static void
gimp_container_view_add (GimpContainerView *view,
                         GimpViewable      *viewable,
                         GimpContainer     *container)
{
  GimpContainerViewInterface *view_iface;
  GimpContainerViewPrivate   *private;
  GimpViewable               *parent;
  GimpContainer              *children;
  gpointer                    parent_insert_data = NULL;
  gpointer                    insert_data;
  gint                        index;

  view_iface = GIMP_CONTAINER_VIEW_GET_IFACE (view);
  private    = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  index = gimp_container_get_child_index (container,
                                          GIMP_OBJECT (viewable));

  parent = gimp_viewable_get_parent (viewable);

  if (parent)
    parent_insert_data = g_hash_table_lookup (private->item_hash, parent);

  insert_data = view_iface->insert_item (view, viewable,
                                         parent_insert_data, index);

  g_hash_table_insert (private->item_hash, viewable, insert_data);

  if (view_iface->insert_items_after)
    view_iface->insert_items_after (view);

  children = gimp_viewable_get_children (viewable);

  if (children)
    gimp_container_view_add_container (view, children);
}

static void
gimp_container_view_remove_container (GimpContainerView *view,
                                      GimpContainer     *container)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);

  g_object_ref (container);

  g_signal_handlers_disconnect_by_func (container,
                                        gimp_container_view_add,
                                        view);
  g_signal_handlers_disconnect_by_func (container,
                                        gimp_container_view_remove,
                                        view);
  g_signal_handlers_disconnect_by_func (container,
                                        gimp_container_view_reorder,
                                        view);

  if (container == private->container)
    {
      g_clear_pointer (&private->name_changed_handler,
                       gimp_tree_handler_disconnect);
      g_clear_pointer (&private->expanded_changed_handler,
                       gimp_tree_handler_disconnect);

      /* optimization: when the toplevel container gets removed, call
       * clear_items() which will get rid of all view widget stuff
       * *and* empty private->item_hash, so below call to
       * remove_foreach() will only disconnect all containers but not
       * remove all items individually (because they are gone from
       * item_hash).
       */
      gimp_container_view_clear_items (view);
    }

  gimp_container_foreach (container,
                          (GFunc) gimp_container_view_remove_foreach,
                          view);

  g_object_unref (container);
}

static void
gimp_container_view_remove_foreach (GimpViewable      *viewable,
                                    GimpContainerView *view)
{
  gimp_container_view_remove (view, viewable, NULL);
}

static void
gimp_container_view_remove (GimpContainerView *view,
                            GimpViewable      *viewable,
                            GimpContainer     *unused)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);
  GimpContainer            *children;
  gpointer                  insert_data;

  children = gimp_viewable_get_children (viewable);

  if (children)
    gimp_container_view_remove_container (view, children);

  insert_data = g_hash_table_lookup (private->item_hash, viewable);

  if (insert_data)
    {
      GIMP_CONTAINER_VIEW_GET_IFACE (view)->remove_item (view, viewable,
                                                         insert_data);

      g_hash_table_remove (private->item_hash, viewable);
    }
}

static void
gimp_container_view_reorder (GimpContainerView *view,
                             GimpViewable      *viewable,
                             gint               old_index,
                             gint               new_index,
                             GimpContainer     *container)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);
  gpointer                  insert_data;

  insert_data = g_hash_table_lookup (private->item_hash, viewable);

  if (insert_data)
    {
      GIMP_CONTAINER_VIEW_GET_IFACE (view)->reorder_item (view,
                                                          viewable,
                                                          new_index,
                                                          insert_data);
    }
}

static void
gimp_container_view_freeze (GimpContainerView *view,
                            GimpContainer     *container)
{
  gimp_container_view_remove_container (view, container);
}

static void
gimp_container_view_thaw (GimpContainerView *view,
                          GimpContainer     *container)
{
  gimp_container_view_add_container (view, container);
}

static void
gimp_container_view_name_changed (GimpViewable      *viewable,
                                  GimpContainerView *view)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);
  gpointer                  insert_data;

  insert_data = g_hash_table_lookup (private->item_hash, viewable);

  if (insert_data)
    {
      GIMP_CONTAINER_VIEW_GET_IFACE (view)->rename_item (view,
                                                         viewable,
                                                         insert_data);
    }
}

static void
gimp_container_view_expanded_changed (GimpViewable      *viewable,
                                      GimpContainerView *view)
{
  GimpContainerViewPrivate *private = GIMP_CONTAINER_VIEW_GET_PRIVATE (view);
  gpointer                  insert_data;

  insert_data = g_hash_table_lookup (private->item_hash, viewable);

  if (insert_data)
    {
      GIMP_CONTAINER_VIEW_GET_IFACE (view)->expand_item (view,
                                                         viewable,
                                                         insert_data);
    }
}
