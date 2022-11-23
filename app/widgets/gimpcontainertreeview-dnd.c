/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacontainertreeview-dnd.c
 * Copyright (C) 2003-2009 Michael Natterer <mitch@ligma.org>
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

#include "core/ligmacontainer.h"
#include "core/ligmaviewable.h"

#include "ligmacontainertreestore.h"
#include "ligmacontainertreeview.h"
#include "ligmacontainertreeview-dnd.h"
#include "ligmacontainertreeview-private.h"
#include "ligmacontainerview.h"
#include "ligmadnd.h"
#include "ligmaviewrenderer.h"
#include "ligmaselectiondata.h"


static gint
ligma_container_tree_view_viewable_sort (LigmaViewable          *v1,
                                        LigmaViewable          *v2,
                                        LigmaContainerTreeView *tree_view)
{
  LigmaContainerView *view        = LIGMA_CONTAINER_VIEW (tree_view);
  LigmaViewable      *parent1;
  LigmaViewable      *parent2;
  LigmaContainer     *container1  = NULL;
  LigmaContainer     *container2  = NULL;
  LigmaContainer     *container   = ligma_container_view_get_container (view);
  gint               index1      = -1;
  gint               index2      = -1;
  gint               depth1;
  gint               depth2;

  parent1 = ligma_viewable_get_parent (v1);
  parent2 = ligma_viewable_get_parent (v2);

  if (parent1)
    container1 = ligma_viewable_get_children (parent1);
  else if (ligma_container_have (container, LIGMA_OBJECT (v1)))
    container1 = container;

  if (parent2)
    container2 = ligma_viewable_get_children (parent2);
  else if (ligma_container_have (container, LIGMA_OBJECT (v2)))
    container2 = container;

  g_return_val_if_fail (container1 && container2, 0);

  if (container1 == container2)
    {
      index1 = ligma_container_get_child_index (container1, LIGMA_OBJECT (v1));
      index2 = ligma_container_get_child_index (container2, LIGMA_OBJECT (v2));

      return index1 < index2 ? -1 : (index1 > index2 ? 1 : 0);
    }

  depth1 = ligma_viewable_get_depth (v1);
  depth2 = ligma_viewable_get_depth (v2);

  if (depth1 == depth2)
    {
      return ligma_container_tree_view_viewable_sort (parent1, parent2, tree_view);
    }
  else if (depth1 > depth2)
    {
      depth1 = ligma_viewable_get_depth (parent1);
      while (depth1 > depth2)
        {
          parent1 = ligma_viewable_get_parent (parent1);
          depth1 = ligma_viewable_get_depth (parent1);
        }
      return ligma_container_tree_view_viewable_sort (parent1, v2, tree_view);
    }
  else /* if (depth1 < depth2) */
    {
      depth2 = ligma_viewable_get_depth (parent2);
      while (depth1 < depth2)
        {
          parent2 = ligma_viewable_get_parent (parent2);
          depth2 = ligma_viewable_get_depth (parent2);
        }
      return ligma_container_tree_view_viewable_sort (v1, parent2, tree_view);
    }
}

/**
 * ligma_container_tree_view_drop_status:
 * @tree_view:
 * @context:
 * @x:
 * @y:
 * @time:
 * @return_path: the #GtkTreePath of the drop position if the drop is
 *               possible.
 * @return_atom:
 * @return_src_type: the type of drag'n drop.
 * @return_src: allocated #GList of #LigmaViewable being dragged.
 * @return_dest: the #LigmaViewable you are dropping on.
 * @return_pos: the drop position (before, after or into @return_dest).
 *
 * Check whether the current drag can be dropped into @tree_view at
 * position (@x, @y). If so, the various return value information will
 * be optionally filled.
 * Note: if @return_src is not %NULL, hence is filled, it must be freed
 * with g_list_free().
 *
 * Returns: %TRUE is the drop is possible, %FALSE otherwise.
 */
static gboolean
ligma_container_tree_view_drop_status (LigmaContainerTreeView    *tree_view,
                                      GdkDragContext           *context,
                                      gint                      x,
                                      gint                      y,
                                      guint                     time,
                                      GtkTreePath             **return_path,
                                      GdkAtom                  *return_atom,
                                      LigmaDndType              *return_src_type,
                                      GList                   **return_src,
                                      LigmaViewable            **return_dest,
                                      GtkTreeViewDropPosition  *return_pos)
{
  GList                   *src_viewables = NULL;
  LigmaViewable            *dest_viewable = NULL;
  GtkTreePath             *drop_path     = NULL;
  GtkTargetList           *target_list;
  GdkAtom                  target_atom;
  LigmaDndType              src_type;
  GtkTreeViewDropPosition  drop_pos      = GTK_TREE_VIEW_DROP_BEFORE;
  GdkDragAction            drag_action   = 0;

  if (! ligma_container_view_get_container (LIGMA_CONTAINER_VIEW (tree_view)) ||
      ! ligma_container_view_get_reorderable (LIGMA_CONTAINER_VIEW (tree_view)))
    goto drop_impossible;

  target_list = gtk_drag_dest_get_target_list (GTK_WIDGET (tree_view->view));
  target_atom = gtk_drag_dest_find_target (GTK_WIDGET (tree_view->view),
                                           context, target_list);
  if (! gtk_target_list_find (target_list, target_atom, &src_type))
    goto drop_impossible;

  switch (src_type)
    {
    case LIGMA_DND_TYPE_URI_LIST:
    case LIGMA_DND_TYPE_TEXT_PLAIN:
    case LIGMA_DND_TYPE_NETSCAPE_URL:
    case LIGMA_DND_TYPE_COLOR:
    case LIGMA_DND_TYPE_SVG:
    case LIGMA_DND_TYPE_SVG_XML:
    case LIGMA_DND_TYPE_COMPONENT:
    case LIGMA_DND_TYPE_PIXBUF:
      break;

    case LIGMA_DND_TYPE_XDS:
    case LIGMA_DND_TYPE_IMAGE:
    case LIGMA_DND_TYPE_LAYER:
    case LIGMA_DND_TYPE_CHANNEL:
    case LIGMA_DND_TYPE_LAYER_MASK:
    case LIGMA_DND_TYPE_VECTORS:
    case LIGMA_DND_TYPE_BRUSH:
    case LIGMA_DND_TYPE_PATTERN:
    case LIGMA_DND_TYPE_GRADIENT:
    case LIGMA_DND_TYPE_PALETTE:
    case LIGMA_DND_TYPE_FONT:
    case LIGMA_DND_TYPE_BUFFER:
    case LIGMA_DND_TYPE_IMAGEFILE:
    case LIGMA_DND_TYPE_TEMPLATE:
    case LIGMA_DND_TYPE_TOOL_ITEM:
    case LIGMA_DND_TYPE_NOTEBOOK_TAB:
      /* Various LigmaViewable drag data. */
      {
        GtkWidget    *src_widget = gtk_drag_get_source_widget (context);
        LigmaViewable *src_viewable  = NULL;

        if (! src_widget)
          goto drop_impossible;

        src_viewable = ligma_dnd_get_drag_viewable (src_widget);

        if (! LIGMA_IS_VIEWABLE (src_viewable))
          goto drop_impossible;

        src_viewables = g_list_prepend (src_viewables, src_viewable);
      }
      break;

    case LIGMA_DND_TYPE_CHANNEL_LIST:
    case LIGMA_DND_TYPE_LAYER_LIST:
    case LIGMA_DND_TYPE_VECTORS_LIST:
      /* Various LigmaViewable list (GList) drag data. */
      {
        GtkWidget *src_widget = gtk_drag_get_source_widget (context);
        GList     *iter;

        if (! src_widget)
          goto drop_impossible;

        src_viewables = ligma_dnd_get_drag_list (src_widget);

        if (! src_viewables)
          goto drop_impossible;

        for (iter = src_viewables; iter; iter = iter->next)
          if (! LIGMA_IS_VIEWABLE (iter->data))
            {
              g_warning ("%s: contents of the viewable list has the wrong type '%s'.",
                         G_STRFUNC, G_OBJECT_TYPE_NAME (iter->data));
              goto drop_impossible;
            }
      }
      break;

    default:
      goto drop_impossible;
      break;
    }

  gtk_tree_view_convert_widget_to_bin_window_coords (tree_view->view, x, y, &x, &y);
  if (gtk_tree_view_get_path_at_pos (tree_view->view, x, y,
                                     &drop_path, NULL, NULL, NULL))
    {
      LigmaViewRenderer *renderer;
      GtkTreeIter       iter;
      GdkRectangle      cell_area;

      gtk_tree_model_get_iter (tree_view->model, &iter, drop_path);

      renderer = ligma_container_tree_store_get_renderer (LIGMA_CONTAINER_TREE_STORE (tree_view->model),
                                                         &iter);

      dest_viewable = renderer->viewable;

      g_object_unref (renderer);

      gtk_tree_view_get_cell_area (tree_view->view, drop_path, NULL, &cell_area);

      if (ligma_viewable_get_children (dest_viewable))
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
          LigmaViewRenderer *renderer;

          renderer = ligma_container_tree_store_get_renderer (LIGMA_CONTAINER_TREE_STORE (tree_view->model),
                                                             &iter);

          drop_path     = gtk_tree_model_get_path (tree_view->model, &iter);
          dest_viewable = renderer->viewable;
          drop_pos      = GTK_TREE_VIEW_DROP_AFTER;

          g_object_unref (renderer);
        }
    }

  if (dest_viewable || tree_view->priv->dnd_drop_to_empty)
    {
      if (LIGMA_CONTAINER_TREE_VIEW_GET_CLASS (tree_view)->drop_possible (tree_view,
                                                                         src_type,
                                                                         src_viewables,
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
            {
              src_viewables = g_list_sort_with_data (src_viewables,
                                                     (GCompareDataFunc) ligma_container_tree_view_viewable_sort,
                                                     tree_view);
              *return_src = src_viewables;
            }
          else
            {
              g_list_free (src_viewables);
            }

          if (return_dest)
            *return_dest = dest_viewable;

          if (return_pos)
            *return_pos = drop_pos;

          return TRUE;
        }
    }

 drop_impossible:

  g_list_free (src_viewables);
  gtk_tree_path_free (drop_path);
  gdk_drag_status (context, 0, time);

  return FALSE;
}

#define SCROLL_DISTANCE 30
#define SCROLL_STEP     10
#define SCROLL_INTERVAL  5
/* #define SCROLL_DEBUG 1 */

static gboolean
ligma_container_tree_view_scroll_timeout (gpointer data)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (data);
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
                       ligma_container_tree_view_scroll_timeout,
                       tree_view);
    }

  return FALSE;
}

void
ligma_container_tree_view_drag_failed (GtkWidget             *widget,
                                      GdkDragContext        *context,
                                      GtkDragResult          result,
                                      LigmaContainerTreeView *tree_view)
{
  if (tree_view->priv->scroll_timeout_id)
    {
      g_source_remove (tree_view->priv->scroll_timeout_id);
      tree_view->priv->scroll_timeout_id = 0;
    }

  gtk_tree_view_set_drag_dest_row (tree_view->view, NULL, 0);
}

void
ligma_container_tree_view_drag_leave (GtkWidget             *widget,
                                     GdkDragContext        *context,
                                     guint                  time,
                                     LigmaContainerTreeView *tree_view)
{
  if (tree_view->priv->scroll_timeout_id)
    {
      g_source_remove (tree_view->priv->scroll_timeout_id);
      tree_view->priv->scroll_timeout_id = 0;
    }

  gtk_tree_view_set_drag_dest_row (tree_view->view, NULL, 0);
}

gboolean
ligma_container_tree_view_drag_motion (GtkWidget             *widget,
                                      GdkDragContext        *context,
                                      gint                   x,
                                      gint                   y,
                                      guint                  time,
                                      LigmaContainerTreeView *tree_view)
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
                         ligma_container_tree_view_scroll_timeout,
                         tree_view);
    }
  else if (tree_view->priv->scroll_timeout_id)
    {
      g_source_remove (tree_view->priv->scroll_timeout_id);
      tree_view->priv->scroll_timeout_id = 0;
    }

  if (ligma_container_tree_view_drop_status (tree_view,
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
ligma_container_tree_view_drag_drop (GtkWidget             *widget,
                                    GdkDragContext        *context,
                                    gint                   x,
                                    gint                   y,
                                    guint                  time,
                                    LigmaContainerTreeView *tree_view)
{
  LigmaDndType              src_type;
  GList                   *src_viewables;
  LigmaViewable            *dest_viewable;
  GdkAtom                  target;
  GtkTreeViewDropPosition  drop_pos;

  if (tree_view->priv->scroll_timeout_id)
    {
      g_source_remove (tree_view->priv->scroll_timeout_id);
      tree_view->priv->scroll_timeout_id = 0;
    }

  if (ligma_container_tree_view_drop_status (tree_view,
                                            context, x, y, time,
                                            NULL, &target, &src_type,
                                            &src_viewables,
                                            &dest_viewable, &drop_pos))
    {
      LigmaContainerTreeViewClass *tree_view_class;

      tree_view_class = LIGMA_CONTAINER_TREE_VIEW_GET_CLASS (tree_view);

      if (src_viewables)
        {
          gboolean success = TRUE;

          /* XXX: Make LigmaContainerTreeViewClass::drop_viewable()
           * return success?
           */
          tree_view_class->drop_viewables (tree_view, src_viewables,
                                           dest_viewable, drop_pos);

          gtk_drag_finish (context, success, FALSE, time);
          g_list_free (src_viewables);
        }
      else
        {
          /* Necessary for instance for dragging color components onto
           * item dialogs.
           */
          gtk_drag_get_data (widget, context, target, time);
        }

      return TRUE;
    }

  return FALSE;
}

void
ligma_container_tree_view_drag_data_received (GtkWidget             *widget,
                                             GdkDragContext        *context,
                                             gint                   x,
                                             gint                   y,
                                             GtkSelectionData      *selection_data,
                                             guint                  info,
                                             guint                  time,
                                             LigmaContainerTreeView *tree_view)
{
  LigmaViewable            *dest_viewable;
  GtkTreeViewDropPosition  drop_pos;
  gboolean                 success = FALSE;

  if (ligma_container_tree_view_drop_status (tree_view,
                                            context, x, y, time,
                                            NULL, NULL, NULL, NULL,
                                            &dest_viewable, &drop_pos))
    {
      LigmaContainerTreeViewClass *tree_view_class;

      tree_view_class = LIGMA_CONTAINER_TREE_VIEW_GET_CLASS (tree_view);

      switch (info)
        {
        case LIGMA_DND_TYPE_URI_LIST:
        case LIGMA_DND_TYPE_TEXT_PLAIN:
        case LIGMA_DND_TYPE_NETSCAPE_URL:
          if (tree_view_class->drop_uri_list)
            {
              GList *uri_list;

              uri_list = ligma_selection_data_get_uri_list (selection_data);

              if (uri_list)
                {
                  tree_view_class->drop_uri_list (tree_view, uri_list,
                                                  dest_viewable, drop_pos);

                  g_list_free_full (uri_list, (GDestroyNotify) g_free);

                  success = TRUE;
                }
            }
          break;

        case LIGMA_DND_TYPE_COLOR:
          if (tree_view_class->drop_color)
            {
              LigmaRGB color;

              if (ligma_selection_data_get_color (selection_data, &color))
                {
                  tree_view_class->drop_color (tree_view, &color,
                                               dest_viewable, drop_pos);

                  success = TRUE;
                }
            }
          break;

        case LIGMA_DND_TYPE_SVG:
        case LIGMA_DND_TYPE_SVG_XML:
          if (tree_view_class->drop_svg)
            {
              const guchar *stream;
              gsize         stream_length;

              stream = ligma_selection_data_get_stream (selection_data,
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

        case LIGMA_DND_TYPE_COMPONENT:
          if (tree_view_class->drop_component)
            {
              LigmaImage       *image = NULL;
              LigmaChannelType  component;

              if (tree_view->dnd_ligma)
                image = ligma_selection_data_get_component (selection_data,
                                                           tree_view->dnd_ligma,
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

        case LIGMA_DND_TYPE_PIXBUF:
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
ligma_container_tree_view_real_drop_possible (LigmaContainerTreeView   *tree_view,
                                             LigmaDndType              src_type,
                                             GList                   *src_viewables,
                                             LigmaViewable            *dest_viewable,
                                             GtkTreePath             *drop_path,
                                             GtkTreeViewDropPosition  drop_pos,
                                             GtkTreeViewDropPosition *return_drop_pos,
                                             GdkDragAction           *return_drag_action)
{
  LigmaContainerView *view           = LIGMA_CONTAINER_VIEW (tree_view);
  LigmaContainer     *container      = ligma_container_view_get_container (view);
  LigmaContainer     *src_container  = NULL;
  LigmaContainer     *dest_container = NULL;
  GList             *iter;
  gint               src_index      = -1;
  gint               dest_index     = -1;

  if (dest_viewable)
    {
      LigmaViewable *parent;

      /*  dropping on the lower third of a group item drops into that group  */
      if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER &&
          ligma_viewable_get_children (dest_viewable))
        {
          parent = dest_viewable;
        }
      else
        {
          parent = ligma_viewable_get_parent (dest_viewable);
        }

      if (parent)
        dest_container = ligma_viewable_get_children (parent);
      else if (ligma_container_have (container, LIGMA_OBJECT (dest_viewable)))
        dest_container = container;

      if (parent == dest_viewable)
        dest_index = 0;
      else
        dest_index = ligma_container_get_child_index (dest_container,
                                                     LIGMA_OBJECT (dest_viewable));
    }

  if (return_drag_action)
    {
      if (! src_viewables)
        *return_drag_action = GDK_ACTION_COPY;
      else
        *return_drag_action = GDK_ACTION_MOVE;
    }

  for (iter = src_viewables; iter; iter = iter->next)
    {
      LigmaViewable *src_viewable = iter->data;
      LigmaViewable *parent;

      parent = ligma_viewable_get_parent (src_viewable);

      if (parent)
        src_container = ligma_viewable_get_children (parent);
      else if (ligma_container_have (container, LIGMA_OBJECT (src_viewable)))
        src_container = container;

      if (src_container)
        src_index = ligma_container_get_child_index (src_container,
                                                    LIGMA_OBJECT (src_viewable));


      if (g_type_is_a (G_TYPE_FROM_INSTANCE (src_viewable),
                       ligma_container_get_children_type (container)))
        {
          /* The drop won't change a thing. This is not a fatal drop
           * failure, unless there is only one source viewable.
           * See also the XXX below.
           */
          if (src_viewable == dest_viewable && g_list_length (src_viewables) == 1)
            return FALSE;

          if (src_index == -1 || dest_index == -1)
            return FALSE;

          /*  don't allow dropping a parent node onto one of its descendants
          */
          if (ligma_viewable_is_ancestor (src_viewable, dest_viewable))
            return FALSE;
        }

      /* XXX only check these for list of 1 viewable for now.
       * Actually this drop failure would also happen for more than 1
       * viewable if all the sources are from the same src_container
       * with successive indexes.
       */
      if (src_container == dest_container && g_list_length (src_viewables) == 1)
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

      if (return_drag_action)
        {
          if (! g_type_is_a (G_TYPE_FROM_INSTANCE (src_viewable),
                             ligma_container_get_children_type (container)))
            *return_drag_action = GDK_ACTION_COPY;
        }
    }

  if (return_drop_pos)
    *return_drop_pos = drop_pos;

  return TRUE;
}

void
ligma_container_tree_view_real_drop_viewables (LigmaContainerTreeView   *tree_view,
                                              GList                   *src_viewables,
                                              LigmaViewable            *dest_viewable,
                                              GtkTreeViewDropPosition  drop_pos)
{
  LigmaContainerView *view       = LIGMA_CONTAINER_VIEW (tree_view);
  LigmaContainer     *src_container;
  LigmaContainer     *dest_container;
  GList             *iter;
  gint               dest_index = 0;

  g_return_if_fail (g_list_length (src_viewables) > 0);

  src_viewables = g_list_reverse (src_viewables);
  for (iter = src_viewables; iter; iter = iter->next)
    {
      LigmaViewable *src_viewable = iter->data;

      if ((drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER   ||
           drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE) &&
          ligma_viewable_get_children (dest_viewable))
        {
          dest_container = ligma_viewable_get_children (dest_viewable);
          dest_viewable  = NULL;
          drop_pos       = GTK_TREE_VIEW_DROP_BEFORE;
        }
      else if (ligma_viewable_get_parent (dest_viewable))
        {
          dest_container = ligma_viewable_get_children (ligma_viewable_get_parent (dest_viewable));
        }
      else
        {
          dest_container = ligma_container_view_get_container (view);
        }

      if (dest_viewable)
        {
          dest_index = ligma_container_get_child_index (dest_container,
                                                       LIGMA_OBJECT (dest_viewable));
        }

      if (ligma_viewable_get_parent (src_viewable))
        {
          src_container = ligma_viewable_get_children (
                                                      ligma_viewable_get_parent (src_viewable));
        }
      else
        {
          src_container = ligma_container_view_get_container (view);
        }

      if (src_container == dest_container)
        {
          gint src_index;

          src_index  = ligma_container_get_child_index (src_container,
                                                       LIGMA_OBJECT (src_viewable));

          switch (drop_pos)
            {
            case GTK_TREE_VIEW_DROP_AFTER:
            case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
              if (src_index > dest_index)
                dest_index++;
              break;

            case GTK_TREE_VIEW_DROP_BEFORE:
            case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
              if (src_index < dest_index)
                dest_index--;
              break;
            }

          ligma_container_reorder (src_container,
                                  LIGMA_OBJECT (src_viewable), dest_index);
        }
      else
        {
          switch (drop_pos)
            {
            case GTK_TREE_VIEW_DROP_AFTER:
            case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
              dest_index++;
              break;

            case GTK_TREE_VIEW_DROP_BEFORE:
            case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
              break;
            }

          g_object_ref (src_viewable);

          ligma_container_remove (src_container,  LIGMA_OBJECT (src_viewable));
          ligma_container_insert (dest_container, LIGMA_OBJECT (src_viewable),
                                 dest_index);

          g_object_unref (src_viewable);
        }
    }
}
