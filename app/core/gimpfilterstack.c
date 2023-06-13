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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "gimpfilter.h"
#include "gimpfilterstack.h"


/*  local function prototypes  */

static void   gimp_filter_stack_constructed      (GObject         *object);
static void   gimp_filter_stack_finalize         (GObject         *object);

static void   gimp_filter_stack_add              (GimpContainer   *container,
                                                  GimpObject      *object);
static void   gimp_filter_stack_remove           (GimpContainer   *container,
                                                  GimpObject      *object);
static void   gimp_filter_stack_reorder          (GimpContainer   *container,
                                                  GimpObject      *object,
                                                  gint             new_index);

static void   gimp_filter_stack_add_node         (GimpFilterStack *stack,
                                                  GimpFilter      *filter);
static void   gimp_filter_stack_remove_node      (GimpFilterStack *stack,
                                                  GimpFilter      *filter);
static void   gimp_filter_stack_update_last_node (GimpFilterStack *stack);

static void   gimp_filter_stack_filter_active    (GimpFilter      *filter,
                                                  GimpFilterStack *stack);


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

  gimp_assert (g_type_is_a (gimp_container_get_children_type (container),
                            GIMP_TYPE_FILTER));

  gimp_container_add_handler (container, "active-changed",
                              G_CALLBACK (gimp_filter_stack_filter_active),
                              container);
}

static void
gimp_filter_stack_finalize (GObject *object)
{
  GimpFilterStack *stack = GIMP_FILTER_STACK (object);

  g_clear_object (&stack->graph);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_filter_stack_add (GimpContainer *container,
                       GimpObject    *object)
{
  GimpFilterStack *stack  = GIMP_FILTER_STACK (container);
  GimpFilter      *filter = GIMP_FILTER (object);

  GIMP_CONTAINER_CLASS (parent_class)->add (container, object);

  if (gimp_filter_get_active (filter))
    {
      if (stack->graph)
        {
          gegl_node_add_child (stack->graph, gimp_filter_get_node (filter));
          gimp_filter_stack_add_node (stack, filter);
        }

      gimp_filter_stack_update_last_node (stack);
    }
}

static void
gimp_filter_stack_remove (GimpContainer *container,
                          GimpObject    *object)
{
  GimpFilterStack *stack  = GIMP_FILTER_STACK (container);
  GimpFilter      *filter = GIMP_FILTER (object);

  if (stack->graph && gimp_filter_get_active (filter))
    {
      gimp_filter_stack_remove_node (stack, filter);
      gegl_node_remove_child (stack->graph, gimp_filter_get_node (filter));
    }

  GIMP_CONTAINER_CLASS (parent_class)->remove (container, object);

  if (gimp_filter_get_active (filter))
    {
      gimp_filter_set_is_last_node (filter, FALSE);
      gimp_filter_stack_update_last_node (stack);
    }
}

static void
gimp_filter_stack_reorder (GimpContainer *container,
                           GimpObject    *object,
                           gint           new_index)
{
  GimpFilterStack *stack  = GIMP_FILTER_STACK (container);
  GimpFilter      *filter = GIMP_FILTER (object);

  if (stack->graph && gimp_filter_get_active (filter))
    gimp_filter_stack_remove_node (stack, filter);

  GIMP_CONTAINER_CLASS (parent_class)->reorder (container, object, new_index);

  if (gimp_filter_get_active (filter))
    {
      gimp_filter_stack_update_last_node (stack);

      if (stack->graph)
        gimp_filter_stack_add_node (stack, filter);
    }
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
  GeglNode *previous;
  GeglNode *output;

  g_return_val_if_fail (GIMP_IS_FILTER_STACK (stack), NULL);

  if (stack->graph)
    return stack->graph;

  stack->graph = gegl_node_new ();

  previous = gegl_node_get_input_proxy (stack->graph, "input");

  for (list = GIMP_LIST (stack)->queue->tail;
       list;
       list = g_list_previous (list))
    {
      GimpFilter *filter = list->data;
      GeglNode   *node;

      if (! gimp_filter_get_active (filter))
        continue;

      node = gimp_filter_get_node (filter);

      gegl_node_add_child (stack->graph, node);

      gegl_node_link (previous, node);

      previous = node;
    }

  output = gegl_node_get_output_proxy (stack->graph, "output");

  gegl_node_link (previous, output);

  return stack->graph;
}


/*  private functions  */

static void
gimp_filter_stack_add_node (GimpFilterStack *stack,
                            GimpFilter      *filter)
{
  GeglNode *node;
  GeglNode *node_above = NULL;
  GeglNode *node_below = NULL;
  GList    *iter;

  node = gimp_filter_get_node (filter);


  iter = g_list_find (GIMP_LIST (stack)->queue->head, filter);

  while ((iter = g_list_previous (iter)))
    {
      GimpFilter *filter_above = iter->data;

      if (gimp_filter_get_active (filter_above))
        {
          node_above = gimp_filter_get_node (filter_above);

          break;
        }
    }

  if (! node_above)
    node_above = gegl_node_get_output_proxy (stack->graph, "output");

  node_below = gegl_node_get_producer (node_above, "input", NULL);

  gegl_node_link (node_below, node);
  gegl_node_link (node, node_above);
}

static void
gimp_filter_stack_remove_node (GimpFilterStack *stack,
                               GimpFilter      *filter)
{
  GeglNode *node;
  GeglNode *node_above = NULL;
  GeglNode *node_below = NULL;
  GList    *iter;

  node = gimp_filter_get_node (filter);

  iter = g_list_find (GIMP_LIST (stack)->queue->head, filter);

  while ((iter = g_list_previous (iter)))
    {
      GimpFilter *filter_above = iter->data;

      if (gimp_filter_get_active (filter_above))
        {
          node_above = gimp_filter_get_node (filter_above);

          break;
        }
    }

  if (! node_above)
    node_above = gegl_node_get_output_proxy (stack->graph, "output");

  node_below = gegl_node_get_producer (node, "input", NULL);

  gegl_node_disconnect (node, "input");

  gegl_node_link (node_below, node_above);
}

static void
gimp_filter_stack_update_last_node (GimpFilterStack *stack)
{
  GList    *list;
  gboolean  found_last = FALSE;

  for (list = GIMP_LIST (stack)->queue->tail;
       list;
       list = g_list_previous (list))
    {
      GimpFilter *filter = list->data;

      if (! found_last && gimp_filter_get_active (filter))
        {
          gimp_filter_set_is_last_node (filter, TRUE);
          found_last = TRUE;
        }
      else
        {
          gimp_filter_set_is_last_node (filter, FALSE);
        }
    }
}

static void
gimp_filter_stack_filter_active (GimpFilter      *filter,
                                 GimpFilterStack *stack)
{
  if (stack->graph)
    {
      if (gimp_filter_get_active (filter))
        {
          gegl_node_add_child (stack->graph, gimp_filter_get_node (filter));
          gimp_filter_stack_add_node (stack, filter);
        }
      else
        {
          gimp_filter_stack_remove_node (stack, filter);
          gegl_node_remove_child (stack->graph, gimp_filter_get_node (filter));
        }
    }

  gimp_filter_stack_update_last_node (stack);

  if (! gimp_filter_get_active (filter))
    gimp_filter_set_is_last_node (filter, FALSE);
}
