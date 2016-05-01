/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpfilterstack.c
 * Copyright (C) 2008-2013 Michael Natterer <mitch@gimp.org>
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "gimpfilter.h"
#include "gimpfilterstack.h"


/*  local function prototypes  */

static void   gimp_filter_stack_constructed (GObject         *object);
static void   gimp_filter_stack_finalize    (GObject         *object);

static void   gimp_filter_stack_add         (GimpContainer   *container,
                                             GimpObject      *object);
static void   gimp_filter_stack_remove      (GimpContainer   *container,
                                             GimpObject      *object);
static void   gimp_filter_stack_reorder     (GimpContainer   *container,
                                             GimpObject      *object,
                                             gint             new_index);

static void   gimp_filter_stack_add_node    (GimpFilterStack *stack,
                                             GimpFilter      *filter);
static void   gimp_filter_stack_remove_node (GimpFilterStack *stack,
                                             GimpFilter      *filter);


G_DEFINE_TYPE (GimpFilterStack, gimp_filter_stack, GIMP_TYPE_LIST);

#define parent_class gimp_filter_stack_parent_class


static void
gimp_filter_stack_class_init (GimpFilterStackClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GimpContainerClass *container_class = GIMP_CONTAINER_CLASS (klass);

  object_class->constructed = gimp_filter_stack_constructed;
  object_class->finalize    = gimp_filter_stack_finalize;

  container_class->add      = gimp_filter_stack_add;
  container_class->remove   = gimp_filter_stack_remove;
  container_class->reorder  = gimp_filter_stack_reorder;
}

static void
gimp_filter_stack_init (GimpFilterStack *stack)
{
}

static void
gimp_filter_stack_constructed (GObject *object)
{
  GimpContainer *container = GIMP_CONTAINER (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (g_type_is_a (gimp_container_get_children_type (container),
                         GIMP_TYPE_FILTER));
}

static void
gimp_filter_stack_finalize (GObject *object)
{
  GimpFilterStack *stack = GIMP_FILTER_STACK (object);

  if (stack->graph)
    {
      g_object_unref (stack->graph);
      stack->graph = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_filter_stack_add (GimpContainer *container,
                       GimpObject    *object)
{
  GimpFilterStack *stack  = GIMP_FILTER_STACK (container);
  GimpFilter      *filter = GIMP_FILTER (object);
  gint             n_children;

  n_children = gimp_container_get_n_children (container);

  if (n_children == 0)
    gimp_filter_set_is_last_node (filter, TRUE);

  GIMP_CONTAINER_CLASS (parent_class)->add (container, object);

  if (stack->graph)
    {
      gegl_node_add_child (stack->graph, gimp_filter_get_node (filter));
      gimp_filter_stack_add_node (stack, filter);
    }
}

static void
gimp_filter_stack_remove (GimpContainer *container,
                          GimpObject    *object)
{
  GimpFilterStack *stack  = GIMP_FILTER_STACK (container);
  GimpFilter      *filter = GIMP_FILTER (object);
  gint             n_children;

  if (stack->graph)
    {
      gimp_filter_stack_remove_node (stack, filter);
      gegl_node_remove_child (stack->graph, gimp_filter_get_node (filter));
    }

  GIMP_CONTAINER_CLASS (parent_class)->remove (container, object);

  gimp_filter_set_is_last_node (filter, FALSE);

  n_children = gimp_container_get_n_children (container);

  if (n_children > 0)
    {
      GimpFilter *last_node = (GimpFilter *)
        gimp_container_get_child_by_index (container, n_children - 1);

      gimp_filter_set_is_last_node (last_node, TRUE);
    }
}

static void
gimp_filter_stack_reorder (GimpContainer *container,
                           GimpObject    *object,
                           gint           new_index)
{
  GimpFilterStack *stack  = GIMP_FILTER_STACK (container);
  GimpFilter      *filter = GIMP_FILTER (object);
  gint             n_children;
  gint             old_index;

  n_children = gimp_container_get_n_children (container);
  old_index  = gimp_container_get_child_index (container, object);

  if (stack->graph)
    gimp_filter_stack_remove_node (stack, filter);

  if (old_index == n_children -1)
    {
      gimp_filter_set_is_last_node (filter, FALSE);
    }
  else if (new_index == n_children - 1)
    {
      GimpFilter *last_node = (GimpFilter *)
        gimp_container_get_child_by_index (container, n_children - 1);

      gimp_filter_set_is_last_node (last_node, FALSE);
    }

  GIMP_CONTAINER_CLASS (parent_class)->reorder (container, object, new_index);

  if (new_index == n_children - 1)
    {
      gimp_filter_set_is_last_node (filter, TRUE);
    }
  else if (old_index == n_children - 1)
    {
      GimpFilter *last_node = (GimpFilter *)
        gimp_container_get_child_by_index (container, n_children - 1);

      gimp_filter_set_is_last_node (last_node, TRUE);
    }

  if (stack->graph)
    gimp_filter_stack_add_node (stack, filter);
}


/*  public functions  */

GimpContainer *
gimp_filter_stack_new (GType filter_type)
{
  g_return_val_if_fail (g_type_is_a (filter_type, GIMP_TYPE_FILTER), NULL);

  return g_object_new (GIMP_TYPE_FILTER_STACK,
                       "name",          g_type_name (filter_type),
                       "children-type", filter_type,
                       "policy",        GIMP_CONTAINER_POLICY_STRONG,
                       NULL);
}

GeglNode *
gimp_filter_stack_get_graph (GimpFilterStack *stack)
{
  GList    *list;
  GList    *reverse_list = NULL;
  GeglNode *first        = NULL;
  GeglNode *previous     = NULL;
  GeglNode *input;
  GeglNode *output;

  g_return_val_if_fail (GIMP_IS_FILTER_STACK (stack), NULL);

  if (stack->graph)
    return stack->graph;

  for (list = GIMP_LIST (stack)->queue->head;
       list;
       list = g_list_next (list))
    {
      GimpFilter *filter = list->data;

      reverse_list = g_list_prepend (reverse_list, filter);
    }

  stack->graph = gegl_node_new ();

  for (list = reverse_list; list; list = g_list_next (list))
    {
      GimpFilter *filter = list->data;
      GeglNode   *node   = gimp_filter_get_node (filter);

      if (! first)
        first = node;

      gegl_node_add_child (stack->graph, node);

      if (previous)
        gegl_node_connect_to (previous, "output",
                              node,     "input");

      previous = node;
    }

  g_list_free (reverse_list);

  input  = gegl_node_get_input_proxy  (stack->graph, "input");
  output = gegl_node_get_output_proxy (stack->graph, "output");

  if (first && previous)
    {
      gegl_node_connect_to (input, "output",
                            first, "input");
      gegl_node_connect_to (previous, "output",
                            output,   "input");
    }
  else
    {
      gegl_node_connect_to (input,  "output",
                            output, "input");
    }

  return stack->graph;
}


/*  private functions  */

static void
gimp_filter_stack_add_node (GimpFilterStack *stack,
                            GimpFilter      *filter)
{
  GimpFilter *filter_below;
  GeglNode   *node_above;
  GeglNode   *node_below;
  GeglNode   *node;
  gint        index;

  node = gimp_filter_get_node (filter);

  index = gimp_container_get_child_index (GIMP_CONTAINER (stack),
                                          GIMP_OBJECT (filter));

  if (index == 0)
    {
      node_above = gegl_node_get_output_proxy (stack->graph, "output");
    }
  else
    {
      GimpFilter *filter_above = (GimpFilter *)
        gimp_container_get_child_by_index (GIMP_CONTAINER (stack), index - 1);

      node_above = gimp_filter_get_node (filter_above);
    }

  gegl_node_connect_to (node,       "output",
                        node_above, "input");

  filter_below = (GimpFilter *)
    gimp_container_get_child_by_index (GIMP_CONTAINER (stack), index + 1);

  if (filter_below)
    {
      node_below = gimp_filter_get_node (filter_below);
    }
  else
    {
      node_below = gegl_node_get_input_proxy (stack->graph, "input");
    }

  gegl_node_connect_to (node_below, "output",
                        node,       "input");
}

static void
gimp_filter_stack_remove_node (GimpFilterStack *stack,
                               GimpFilter      *filter)
{
  GimpFilter *filter_below;
  GeglNode   *node_above;
  GeglNode   *node_below;
  GeglNode   *node;
  gint        index;

  node = gimp_filter_get_node (filter);

  gegl_node_disconnect (node, "input");

  index = gimp_container_get_child_index (GIMP_CONTAINER (stack),
                                          GIMP_OBJECT (filter));

  if (index == 0)
    {
      node_above = gegl_node_get_output_proxy (stack->graph, "output");
    }
  else
    {
      GimpFilter *filter_above = (GimpFilter *)
        gimp_container_get_child_by_index (GIMP_CONTAINER (stack), index - 1);

      node_above = gimp_filter_get_node (filter_above);
    }

  filter_below = (GimpFilter *)
    gimp_container_get_child_by_index (GIMP_CONTAINER (stack), index + 1);

  if (filter_below)
    {
      node_below = gimp_filter_get_node (filter_below);
    }
  else
    {
      node_below = gegl_node_get_input_proxy (stack->graph, "input");
    }

  gegl_node_connect_to (node_below, "output",
                        node_above, "input");
}
