/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainertreeview-dnd.c
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

#include "core/gimpcontainer.h"
#include "core/gimpviewable.h"

#include "gimpcontainertreeview.h"
#include "gimpcontainertreeview-dnd.h"
#include "gimpcontainerview.h"
#include "gimpdnd.h"
#include "gimppreviewrenderer.h"


static gboolean
gimp_container_tree_view_drop_status (GimpContainerTreeView    *tree_view,
                                      GdkDragContext           *context,
                                      gint                      x,
                                      gint                      y,
                                      guint                     time,
                                      GtkTreePath             **return_path,
                                      GimpViewable            **return_src,
                                      GimpViewable            **return_dest,
                                      GtkTreeViewDropPosition  *return_pos)
{
  GtkWidget    *src_widget;
  GimpViewable *src_viewable;
  GtkTreePath  *path;

  if (! gimp_container_view_get_reorderable (GIMP_CONTAINER_VIEW (tree_view)))
    return FALSE;

  src_widget = gtk_drag_get_source_widget (context);

  if (! src_widget)
    return FALSE;

  src_viewable = gimp_dnd_get_drag_data (src_widget);

  if (! GIMP_IS_VIEWABLE (src_viewable))
    return FALSE;

  if (gtk_tree_view_get_path_at_pos (tree_view->view, x, y,
                                     &path, NULL, NULL, NULL))
    {
      GimpPreviewRenderer     *renderer;
      GimpViewable            *dest_viewable;
      GtkTreeIter              iter;
      GdkRectangle             cell_area;
      GtkTreeViewDropPosition  drop_pos;
      GdkDragAction            drag_action;

      gtk_tree_model_get_iter (tree_view->model, &iter, path);

      gtk_tree_model_get (tree_view->model, &iter,
                          tree_view->model_column_renderer, &renderer,
                          -1);

      dest_viewable = renderer->viewable;

      gtk_tree_view_get_cell_area (tree_view->view, path, NULL, &cell_area);

      if (y >= (cell_area.y + cell_area.height / 2))
        drop_pos = GTK_TREE_VIEW_DROP_AFTER;
      else
        drop_pos = GTK_TREE_VIEW_DROP_BEFORE;

      g_object_unref (renderer);

      if (GIMP_CONTAINER_TREE_VIEW_GET_CLASS (tree_view)->drop_possible (tree_view,
                                                                         src_viewable,
                                                                         dest_viewable,
                                                                         drop_pos,
                                                                         &drag_action))
        {
          gdk_drag_status (context, drag_action, time);

          if (return_path)
            *return_path = path;
          else
            gtk_tree_path_free (path);

          if (return_src)
            *return_src = src_viewable;

          if (return_dest)
            *return_dest = dest_viewable;

          if (return_pos)
            *return_pos = drop_pos;

          return TRUE;
        }

      gdk_drag_status (context, 0, time);

      gtk_tree_path_free (path);
    }

  return FALSE;
}

#define SCROLL_DISTANCE 30
#define SCROLL_STEP     10
#define SCROLL_INTERVAL  5
/* #define SCROLL_DEBUG 1 */

static gboolean
gimp_container_tree_view_scroll_timeout (gpointer data)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (data);
  GtkAdjustment         *adj;
  gdouble                new_value;

  adj = gtk_tree_view_get_vadjustment (GTK_TREE_VIEW (tree_view->view));

#ifdef SCROLL_DEBUG
  g_print ("scroll_timeout: scrolling by %d\n", SCROLL_STEP);
#endif

  if (tree_view->scroll_dir == GDK_SCROLL_UP)
    new_value = adj->value - SCROLL_STEP;
  else
    new_value = adj->value + SCROLL_STEP;

  new_value = CLAMP (new_value, adj->lower, adj->upper - adj->page_size);

  gtk_adjustment_set_value (adj, new_value);

  if (tree_view->scroll_timeout_id)
    {
      g_source_remove (tree_view->scroll_timeout_id);

      tree_view->scroll_timeout_id =
        g_timeout_add (tree_view->scroll_timeout_interval,
                       gimp_container_tree_view_scroll_timeout,
                       tree_view);
    }

  return FALSE;
}

void
gimp_container_tree_view_drag_leave (GtkWidget             *widget,
                                     GdkDragContext        *context,
                                     guint                  time,
                                     GimpContainerTreeView *tree_view)
{
  if (tree_view->scroll_timeout_id)
    {
      g_source_remove (tree_view->scroll_timeout_id);
      tree_view->scroll_timeout_id = 0;
    }

  gtk_tree_view_unset_rows_drag_dest (tree_view->view);
}

gboolean
gimp_container_tree_view_drag_motion (GtkWidget             *widget,
                                      GdkDragContext        *context,
                                      gint                   x,
                                      gint                   y,
                                      guint                  time,
                                      GimpContainerTreeView *tree_view)
{
  GtkTreePath             *path;
  GtkTreeViewDropPosition  drop_pos;

  if (y < SCROLL_DISTANCE || y > (widget->allocation.height - SCROLL_DISTANCE))
    {
      gint distance;

      if (y < SCROLL_DISTANCE)
        {
          tree_view->scroll_dir = GDK_SCROLL_UP;
          distance = MIN (-y, -1);
        }
      else
        {
          tree_view->scroll_dir = GDK_SCROLL_DOWN;
          distance = MAX (widget->allocation.height - y, 1);
        }

      tree_view->scroll_timeout_interval = SCROLL_INTERVAL * ABS (distance);

#ifdef SCROLL_DEBUG
      g_print ("drag_motion: scroll_distance = %d  scroll_interval = %d\n",
               distance, tree_view->scroll_timeout_interval);
#endif

      if (! tree_view->scroll_timeout_id)
        tree_view->scroll_timeout_id =
          g_timeout_add (tree_view->scroll_timeout_interval,
                         gimp_container_tree_view_scroll_timeout,
                         tree_view);
    }
  else if (tree_view->scroll_timeout_id)
    {
      g_source_remove (tree_view->scroll_timeout_id);
      tree_view->scroll_timeout_id = 0;
    }

  if (gimp_container_tree_view_drop_status (tree_view,
                                            context, x, y, time,
                                            &path, NULL, NULL, &drop_pos))
    {
      gtk_tree_view_set_drag_dest_row (tree_view->view, path, drop_pos);
      gtk_tree_path_free (path);

      return TRUE;
    }

  gtk_tree_view_unset_rows_drag_dest (tree_view->view);

  return FALSE;
}

gboolean
gimp_container_tree_view_drag_drop (GtkWidget             *widget,
                                    GdkDragContext        *context,
                                    gint                   x,
                                    gint                   y,
                                    guint                  time,
                                    GimpContainerTreeView *tree_view)
{
  GimpViewable            *src_viewable;
  GimpViewable            *dest_viewable;
  GtkTreeViewDropPosition  drop_pos;

  if (tree_view->scroll_timeout_id)
    {
      g_source_remove (tree_view->scroll_timeout_id);
      tree_view->scroll_timeout_id = 0;
    }

  if (gimp_container_tree_view_drop_status (tree_view,
                                            context, x, y, time,
                                            NULL, &src_viewable, &dest_viewable,
                                            &drop_pos))
    {
      GIMP_CONTAINER_TREE_VIEW_GET_CLASS (tree_view)->drop (tree_view,
                                                            src_viewable,
                                                            dest_viewable,
                                                            drop_pos);

      return TRUE;
    }

  return TRUE;
}

gboolean
gimp_container_tree_view_real_drop_possible (GimpContainerTreeView   *tree_view,
                                             GimpViewable            *src_viewable,
                                             GimpViewable            *dest_viewable,
                                             GtkTreeViewDropPosition  drop_pos,
                                             GdkDragAction           *drag_action)
{
  GimpContainerView *view      = GIMP_CONTAINER_VIEW (tree_view);
  GimpContainer     *container = gimp_container_view_get_container (view);
  GimpObject        *src_object;
  GimpObject        *dest_object;
  gint               src_index;
  gint               dest_index;

  if (src_viewable == dest_viewable)
    return FALSE;

  src_object  = GIMP_OBJECT (src_viewable);
  dest_object = GIMP_OBJECT (dest_viewable);

  src_index  = gimp_container_get_child_index (container, src_object);
  dest_index = gimp_container_get_child_index (container, dest_object);

  if (src_index == -1 || dest_index == -1)
    return FALSE;

  if (drop_pos == GTK_TREE_VIEW_DROP_BEFORE)
    {
      if (dest_index == (src_index + 1))
        return FALSE;
    }
  else
    {
      if (dest_index == (src_index - 1))
        return FALSE;
    }

  if (drag_action)
    *drag_action = GDK_ACTION_MOVE;

  return TRUE;
}

void
gimp_container_tree_view_real_drop (GimpContainerTreeView   *tree_view,
                                    GimpViewable            *src_viewable,
                                    GimpViewable            *dest_viewable,
                                    GtkTreeViewDropPosition  drop_pos)
{
  GimpContainerView *view      = GIMP_CONTAINER_VIEW (tree_view);
  GimpContainer     *container = gimp_container_view_get_container (view);
  GimpObject        *src_object;
  GimpObject        *dest_object;
  gint               src_index;
  gint               dest_index;

  src_object  = GIMP_OBJECT (src_viewable);
  dest_object = GIMP_OBJECT (dest_viewable);

  src_index  = gimp_container_get_child_index (container, src_object);
  dest_index = gimp_container_get_child_index (container, dest_object);

  if (drop_pos == GTK_TREE_VIEW_DROP_AFTER && src_index > dest_index)
    {
      dest_index++;
    }
  else if (drop_pos == GTK_TREE_VIEW_DROP_BEFORE && src_index < dest_index)
    {
      dest_index--;
    }

  gimp_container_reorder (container, src_object, dest_index);
}
