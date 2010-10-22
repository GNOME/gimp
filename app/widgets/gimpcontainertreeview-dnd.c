/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainertreeview-dnd.c
 * Copyright (C) 2003-2009 Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpviewable.h"

#include "gimpcontainertreestore.h"
#include "gimpcontainertreeview.h"
#include "gimpcontainertreeview-dnd.h"
#include "gimpcontainertreeview-private.h"
#include "gimpcontainerview.h"
#include "gimpdnd.h"
#include "gimpviewrenderer.h"
#include "gimpselectiondata.h"


static gboolean
gimp_container_tree_view_drop_status (GimpContainerTreeView    *tree_view,
                                      GdkDragContext           *context,
                                      gint                      x,
                                      gint                      y,
                                      guint                     time,
                                      GtkTreePath             **return_path,
                                      GdkAtom                  *return_atom,
                                      GimpDndType              *return_src_type,
                                      GimpViewable            **return_src,
                                      GimpViewable            **return_dest,
                                      GtkTreeViewDropPosition  *return_pos)
{
  GimpViewable            *src_viewable  = NULL;
  GimpViewable            *dest_viewable = NULL;
  GtkTreePath             *drop_path     = NULL;
  GtkTargetList           *target_list;
  GdkAtom                  target_atom;
  GimpDndType              src_type;
  GtkTreeViewDropPosition  drop_pos      = GTK_TREE_VIEW_DROP_BEFORE;
  GdkDragAction            drag_action   = 0;

  if (! gimp_container_view_get_container (GIMP_CONTAINER_VIEW (tree_view)) ||
      ! gimp_container_view_get_reorderable (GIMP_CONTAINER_VIEW (tree_view)))
    goto drop_impossible;

  target_list = gtk_drag_dest_get_target_list (GTK_WIDGET (tree_view->view));
  target_atom = gtk_drag_dest_find_target (GTK_WIDGET (tree_view->view),
                                           context, target_list);
  if (! gtk_target_list_find (target_list, target_atom, &src_type))
    goto drop_impossible;

  switch (src_type)
    {
    case GIMP_DND_TYPE_URI_LIST:
    case GIMP_DND_TYPE_TEXT_PLAIN:
    case GIMP_DND_TYPE_NETSCAPE_URL:
    case GIMP_DND_TYPE_COLOR:
    case GIMP_DND_TYPE_SVG:
    case GIMP_DND_TYPE_SVG_XML:
    case GIMP_DND_TYPE_COMPONENT:
    case GIMP_DND_TYPE_PIXBUF:
      break;

    default:
      {
        GtkWidget *src_widget = gtk_drag_get_source_widget (context);

        if (! src_widget)
          goto drop_impossible;

        src_viewable = gimp_dnd_get_drag_data (src_widget);

        if (! GIMP_IS_VIEWABLE (src_viewable))
          goto drop_impossible;
      }
      break;
    }

  if (gtk_tree_view_get_path_at_pos (tree_view->view, x, y,
                                     &drop_path, NULL, NULL, NULL))
    {
      GimpViewRenderer *renderer;
      GtkTreeIter       iter;
      GdkRectangle      cell_area;

      gtk_tree_model_get_iter (tree_view->model, &iter, drop_path);

      gtk_tree_model_get (tree_view->model, &iter,
                          GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);

      dest_viewable = renderer->viewable;

      g_object_unref (renderer);

      gtk_tree_view_get_cell_area (tree_view->view, drop_path, NULL, &cell_area);

      if (gimp_viewable_get_children (dest_viewable))
        {
          if (gtk_tree_view_row_expanded (tree_view->view, drop_path))
            {
              if (y >= (cell_area.y + cell_area.height / 2))
                drop_pos = GTK_TREE_VIEW_DROP_INTO_OR_AFTER;
              else
                drop_pos = GTK_TREE_VIEW_DROP_BEFORE;
            }
          else
            {
              if (y >= (cell_area.y + 2 * (cell_area.height / 3)))
                drop_pos = GTK_TREE_VIEW_DROP_AFTER;
              else if (y <= (cell_area.y + cell_area.height / 3))
                drop_pos = GTK_TREE_VIEW_DROP_BEFORE;
              else
                drop_pos = GTK_TREE_VIEW_DROP_INTO_OR_AFTER;
            }
        }
      else
        {
          if (y >= (cell_area.y + cell_area.height / 2))
            drop_pos = GTK_TREE_VIEW_DROP_AFTER;
          else
            drop_pos = GTK_TREE_VIEW_DROP_BEFORE;
        }
    }
  else
    {
      GtkTreeIter iter;
      gint        n_children;

      n_children = gtk_tree_model_iter_n_children (tree_view->model, NULL);

      if (n_children > 0 &&
          gtk_tree_model_iter_nth_child (tree_view->model, &iter,
                                         NULL, n_children - 1))
        {
          GimpViewRenderer *renderer;

          gtk_tree_model_get (tree_view->model, &iter,
                              GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER,
                              &renderer,
                              -1);

          drop_path     = gtk_tree_model_get_path (tree_view->model, &iter);
          dest_viewable = renderer->viewable;
          drop_pos      = GTK_TREE_VIEW_DROP_AFTER;

          g_object_unref (renderer);
        }
    }

  if (dest_viewable || tree_view->priv->dnd_drop_to_empty)
    {
      if (GIMP_CONTAINER_TREE_VIEW_GET_CLASS (tree_view)->drop_possible (tree_view,
                                                                         src_type,
                                                                         src_viewable,
                                                                         dest_viewable,
                                                                         drop_path,
                                                                         drop_pos,
                                                                         &drop_pos,
                                                                         &drag_action))
        {
          gdk_drag_status (context, drag_action, time);

          if (return_path)
            *return_path = drop_path;
          else
            gtk_tree_path_free (drop_path);

          if (return_atom)
            *return_atom = target_atom;

          if (return_src)
            *return_src = src_viewable;

          if (return_dest)
            *return_dest = dest_viewable;

          if (return_pos)
            *return_pos = drop_pos;

          return TRUE;
        }

      gtk_tree_path_free (drop_path);
    }

 drop_impossible:

  gdk_drag_status (context, 0, time);

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

  adj = gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (tree_view->view));

#ifdef SCROLL_DEBUG
  g_print ("scroll_timeout: scrolling by %d\n", SCROLL_STEP);
#endif

  if (tree_view->priv->scroll_dir == GDK_SCROLL_UP)
    new_value = gtk_adjustment_get_value (adj) - SCROLL_STEP;
  else
    new_value = gtk_adjustment_get_value (adj) + SCROLL_STEP;

  new_value = CLAMP (new_value,
                     gtk_adjustment_get_lower (adj),
                     gtk_adjustment_get_upper (adj) -
                     gtk_adjustment_get_page_size (adj));

  gtk_adjustment_set_value (adj, new_value);

  if (tree_view->priv->scroll_timeout_id)
    {
      g_source_remove (tree_view->priv->scroll_timeout_id);

      tree_view->priv->scroll_timeout_id =
        g_timeout_add (tree_view->priv->scroll_timeout_interval,
                       gimp_container_tree_view_scroll_timeout,
                       tree_view);
    }

  return FALSE;
}

void
gimp_container_tree_view_drag_failed (GtkWidget             *widget,
                                      GdkDragContext        *context,
                                      GtkDragResult          result,
                                      GimpContainerTreeView *tree_view)
{
  if (tree_view->priv->scroll_timeout_id)
    {
      g_source_remove (tree_view->priv->scroll_timeout_id);
      tree_view->priv->scroll_timeout_id = 0;
    }

  gtk_tree_view_set_drag_dest_row (tree_view->view, NULL, 0);
}

void
gimp_container_tree_view_drag_leave (GtkWidget             *widget,
                                     GdkDragContext        *context,
                                     guint                  time,
                                     GimpContainerTreeView *tree_view)
{
  if (tree_view->priv->scroll_timeout_id)
    {
      g_source_remove (tree_view->priv->scroll_timeout_id);
      tree_view->priv->scroll_timeout_id = 0;
    }

  gtk_tree_view_set_drag_dest_row (tree_view->view, NULL, 0);
}

gboolean
gimp_container_tree_view_drag_motion (GtkWidget             *widget,
                                      GdkDragContext        *context,
                                      gint                   x,
                                      gint                   y,
                                      guint                  time,
                                      GimpContainerTreeView *tree_view)
{
  GtkAllocation            allocation;
  GtkTreePath             *drop_path;
  GtkTreeViewDropPosition  drop_pos;

  gtk_widget_get_allocation (widget, &allocation);

  if (y < SCROLL_DISTANCE || y > (allocation.height - SCROLL_DISTANCE))
    {
      gint distance;

      if (y < SCROLL_DISTANCE)
        {
          tree_view->priv->scroll_dir = GDK_SCROLL_UP;
          distance = MIN (-y, -1);
        }
      else
        {
          tree_view->priv->scroll_dir = GDK_SCROLL_DOWN;
          distance = MAX (allocation.height - y, 1);
        }

      tree_view->priv->scroll_timeout_interval = SCROLL_INTERVAL * ABS (distance);

#ifdef SCROLL_DEBUG
      g_print ("drag_motion: scroll_distance = %d  scroll_interval = %d\n",
               distance, tree_view->priv->scroll_timeout_interval);
#endif

      if (! tree_view->priv->scroll_timeout_id)
        tree_view->priv->scroll_timeout_id =
          g_timeout_add (tree_view->priv->scroll_timeout_interval,
                         gimp_container_tree_view_scroll_timeout,
                         tree_view);
    }
  else if (tree_view->priv->scroll_timeout_id)
    {
      g_source_remove (tree_view->priv->scroll_timeout_id);
      tree_view->priv->scroll_timeout_id = 0;
    }

  if (gimp_container_tree_view_drop_status (tree_view,
                                            context, x, y, time,
                                            &drop_path, NULL, NULL, NULL, NULL,
                                            &drop_pos))
    {
      gtk_tree_view_set_drag_dest_row (tree_view->view, drop_path, drop_pos);
      gtk_tree_path_free (drop_path);
    }
  else
    {
      gtk_tree_view_set_drag_dest_row (tree_view->view, NULL, 0);
    }

  /*  always return TRUE so drag_leave() is called  */
  return TRUE;
}

gboolean
gimp_container_tree_view_drag_drop (GtkWidget             *widget,
                                    GdkDragContext        *context,
                                    gint                   x,
                                    gint                   y,
                                    guint                  time,
                                    GimpContainerTreeView *tree_view)
{
  GimpDndType              src_type;
  GimpViewable            *src_viewable;
  GimpViewable            *dest_viewable;
  GdkAtom                  target;
  GtkTreeViewDropPosition  drop_pos;

  if (tree_view->priv->scroll_timeout_id)
    {
      g_source_remove (tree_view->priv->scroll_timeout_id);
      tree_view->priv->scroll_timeout_id = 0;
    }

  if (gimp_container_tree_view_drop_status (tree_view,
                                            context, x, y, time,
                                            NULL, &target, &src_type,
                                            &src_viewable,
                                            &dest_viewable, &drop_pos))
    {
      GimpContainerTreeViewClass *tree_view_class;

      tree_view_class = GIMP_CONTAINER_TREE_VIEW_GET_CLASS (tree_view);

      if (src_viewable)
        {
          gboolean success = TRUE;

          /* XXX: Make GimpContainerTreeViewClass::drop_viewable()
           * return success?
           */
          tree_view_class->drop_viewable (tree_view, src_viewable,
                                          dest_viewable, drop_pos);

          gtk_drag_finish (context, success, FALSE, time);
        }
      else
        {
          gtk_drag_get_data (widget, context, target, time);
        }

      return TRUE;
    }

  return FALSE;
}

void
gimp_container_tree_view_drag_data_received (GtkWidget             *widget,
                                             GdkDragContext        *context,
                                             gint                   x,
                                             gint                   y,
                                             GtkSelectionData      *selection_data,
                                             guint                  info,
                                             guint                  time,
                                             GimpContainerTreeView *tree_view)
{
  GimpViewable            *dest_viewable;
  GtkTreeViewDropPosition  drop_pos;
  gboolean                 success = FALSE;

  if (gimp_container_tree_view_drop_status (tree_view,
                                            context, x, y, time,
                                            NULL, NULL, NULL, NULL,
                                            &dest_viewable, &drop_pos))
    {
      GimpContainerTreeViewClass *tree_view_class;

      tree_view_class = GIMP_CONTAINER_TREE_VIEW_GET_CLASS (tree_view);

      switch (info)
        {
        case GIMP_DND_TYPE_URI_LIST:
        case GIMP_DND_TYPE_TEXT_PLAIN:
        case GIMP_DND_TYPE_NETSCAPE_URL:
          if (tree_view_class->drop_uri_list)
            {
              GList *uri_list;

              uri_list = gimp_selection_data_get_uri_list (selection_data);

              if (uri_list)
                {
                  tree_view_class->drop_uri_list (tree_view, uri_list,
                                                  dest_viewable, drop_pos);

                  g_list_free_full (uri_list, (GDestroyNotify) g_free);

                  success = TRUE;
                }
            }
          break;

        case GIMP_DND_TYPE_COLOR:
          if (tree_view_class->drop_color)
            {
              GimpRGB color;

              if (gimp_selection_data_get_color (selection_data, &color))
                {
                  tree_view_class->drop_color (tree_view, &color,
                                               dest_viewable, drop_pos);

                  success = TRUE;
                }
            }
          break;

        case GIMP_DND_TYPE_SVG:
        case GIMP_DND_TYPE_SVG_XML:
          if (tree_view_class->drop_svg)
            {
              const guchar *stream;
              gsize         stream_length;

              stream = gimp_selection_data_get_stream (selection_data,
                                                       &stream_length);

              if (stream)
                {
                  tree_view_class->drop_svg (tree_view,
                                             (const gchar *) stream,
                                             stream_length,
                                             dest_viewable, drop_pos);

                  success = TRUE;
                }
            }
          break;

        case GIMP_DND_TYPE_COMPONENT:
          if (tree_view_class->drop_component)
            {
              GimpImage       *image = NULL;
              GimpChannelType  component;

              if (tree_view->dnd_gimp)
                image = gimp_selection_data_get_component (selection_data,
                                                           tree_view->dnd_gimp,
                                                           &component);

              if (image)
                {
                  tree_view_class->drop_component (tree_view,
                                                   image, component,
                                                   dest_viewable, drop_pos);

                  success = TRUE;
                }
            }
          break;

        case GIMP_DND_TYPE_PIXBUF:
          if (tree_view_class->drop_pixbuf)
            {
              GdkPixbuf *pixbuf;

              pixbuf = gtk_selection_data_get_pixbuf (selection_data);

              if (pixbuf)
                {
                  tree_view_class->drop_pixbuf (tree_view,
                                                pixbuf,
                                                dest_viewable, drop_pos);
                  g_object_unref (pixbuf);

                  success = TRUE;
                }
            }
          break;

        default:
          break;
        }
    }

  gtk_drag_finish (context, success, FALSE, time);
}

gboolean
gimp_container_tree_view_real_drop_possible (GimpContainerTreeView   *tree_view,
                                             GimpDndType              src_type,
                                             GimpViewable            *src_viewable,
                                             GimpViewable            *dest_viewable,
                                             GtkTreePath             *drop_path,
                                             GtkTreeViewDropPosition  drop_pos,
                                             GtkTreeViewDropPosition *return_drop_pos,
                                             GdkDragAction           *return_drag_action)
{
  GimpContainerView *view           = GIMP_CONTAINER_VIEW (tree_view);
  GimpContainer     *container      = gimp_container_view_get_container (view);
  GimpContainer     *src_container  = NULL;
  GimpContainer     *dest_container = NULL;
  gint               src_index      = -1;
  gint               dest_index     = -1;

  if (src_viewable)
    {
      GimpViewable *parent = gimp_viewable_get_parent (src_viewable);

      if (parent)
        src_container = gimp_viewable_get_children (parent);
      else if (gimp_container_have (container, GIMP_OBJECT (src_viewable)))
        src_container = container;

      if (src_container)
        src_index = gimp_container_get_child_index (src_container,
                                                    GIMP_OBJECT (src_viewable));
    }

  if (dest_viewable)
    {
      GimpViewable *parent;

      /*  dropping on the lower third of a group item drops into that group  */
      if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER &&
          gimp_viewable_get_children (dest_viewable))
        {
          parent = dest_viewable;
        }
      else
        {
          parent = gimp_viewable_get_parent (dest_viewable);
        }

      if (parent)
        dest_container = gimp_viewable_get_children (parent);
      else if (gimp_container_have (container, GIMP_OBJECT (dest_viewable)))
        dest_container = container;

      if (parent == dest_viewable)
        dest_index = 0;
      else
        dest_index = gimp_container_get_child_index (dest_container,
                                                     GIMP_OBJECT (dest_viewable));
    }

  if (src_viewable && g_type_is_a (G_TYPE_FROM_INSTANCE (src_viewable),
                                   gimp_container_get_children_type (container)))
    {
      if (src_viewable == dest_viewable)
        return FALSE;

      if (src_index == -1 || dest_index == -1)
        return FALSE;

      /*  don't allow dropping a parent node onto one of its descendants
       */
      if (gimp_viewable_is_ancestor (src_viewable, dest_viewable))
        return FALSE;
    }

  if (src_container == dest_container)
    {
      if (drop_pos == GTK_TREE_VIEW_DROP_BEFORE)
        {
          if (dest_index == (src_index + 1))
            return FALSE;
        }
      else if (drop_pos == GTK_TREE_VIEW_DROP_AFTER)
        {
          if (dest_index == (src_index - 1))
            return FALSE;
        }
    }

  if (return_drop_pos)
    *return_drop_pos = drop_pos;

  if (return_drag_action)
    {
      if (src_viewable && g_type_is_a (G_TYPE_FROM_INSTANCE (src_viewable),
                                       gimp_container_get_children_type (container)))
        *return_drag_action = GDK_ACTION_MOVE;
      else
        *return_drag_action = GDK_ACTION_COPY;
    }

  return TRUE;
}

void
gimp_container_tree_view_real_drop_viewable (GimpContainerTreeView   *tree_view,
                                             GimpViewable            *src_viewable,
                                             GimpViewable            *dest_viewable,
                                             GtkTreeViewDropPosition  drop_pos)
{
  GimpContainerView *view      = GIMP_CONTAINER_VIEW (tree_view);
  GimpContainer     *container = gimp_container_view_get_container (view);
  gint               src_index;
  gint               dest_index;

  src_index  = gimp_container_get_child_index (container,
                                               GIMP_OBJECT (src_viewable));
  dest_index = gimp_container_get_child_index (container,
                                               GIMP_OBJECT (dest_viewable));

  if (drop_pos == GTK_TREE_VIEW_DROP_AFTER && src_index > dest_index)
    {
      dest_index++;
    }
  else if (drop_pos == GTK_TREE_VIEW_DROP_BEFORE && src_index < dest_index)
    {
      dest_index--;
    }

  gimp_container_reorder (container, GIMP_OBJECT (src_viewable), dest_index);
}
