/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmafilterstack.c
 * Copyright (C) 2008-2013 Michael Natterer <mitch@ligma.org>
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

#include "ligmafilter.h"
#include "ligmafilterstack.h"


/*  local function prototypes  */

static void   ligma_filter_stack_constructed      (GObject         *object);
static void   ligma_filter_stack_finalize         (GObject         *object);

static void   ligma_filter_stack_add              (LigmaContainer   *container,
                                                  LigmaObject      *object);
static void   ligma_filter_stack_remove           (LigmaContainer   *container,
                                                  LigmaObject      *object);
static void   ligma_filter_stack_reorder          (LigmaContainer   *container,
                                                  LigmaObject      *object,
                                                  gint             new_index);

static void   ligma_filter_stack_add_node         (LigmaFilterStack *stack,
                                                  LigmaFilter      *filter);
static void   ligma_filter_stack_remove_node      (LigmaFilterStack *stack,
                                                  LigmaFilter      *filter);
static void   ligma_filter_stack_update_last_node (LigmaFilterStack *stack);

static void   ligma_filter_stack_filter_active    (LigmaFilter      *filter,
                                                  LigmaFilterStack *stack);


G_DEFINE_TYPE (LigmaFilterStack, ligma_filter_stack, LIGMA_TYPE_LIST);

#define parent_class ligma_filter_stack_parent_class


static void
ligma_filter_stack_class_init (LigmaFilterStackClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  LigmaContainerClass *container_class = LIGMA_CONTAINER_CLASS (klass);

  object_class->constructed = ligma_filter_stack_constructed;
  object_class->finalize    = ligma_filter_stack_finalize;

  container_class->add      = ligma_filter_stack_add;
  container_class->remove   = ligma_filter_stack_remove;
  container_class->reorder  = ligma_filter_stack_reorder;
}

static void
ligma_filter_stack_init (LigmaFilterStack *stack)
{
}

static void
ligma_filter_stack_constructed (GObject *object)
{
  LigmaContainer *container = LIGMA_CONTAINER (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (g_type_is_a (ligma_container_get_children_type (container),
                            LIGMA_TYPE_FILTER));

  ligma_container_add_handler (container, "active-changed",
                              G_CALLBACK (ligma_filter_stack_filter_active),
                              container);
}

static void
ligma_filter_stack_finalize (GObject *object)
{
  LigmaFilterStack *stack = LIGMA_FILTER_STACK (object);

  g_clear_object (&stack->graph);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_filter_stack_add (LigmaContainer *container,
                       LigmaObject    *object)
{
  LigmaFilterStack *stack  = LIGMA_FILTER_STACK (container);
  LigmaFilter      *filter = LIGMA_FILTER (object);

  LIGMA_CONTAINER_CLASS (parent_class)->add (container, object);

  if (ligma_filter_get_active (filter))
    {
      if (stack->graph)
        {
          gegl_node_add_child (stack->graph, ligma_filter_get_node (filter));
          ligma_filter_stack_add_node (stack, filter);
        }

      ligma_filter_stack_update_last_node (stack);
    }
}

static void
ligma_filter_stack_remove (LigmaContainer *container,
                          LigmaObject    *object)
{
  LigmaFilterStack *stack  = LIGMA_FILTER_STACK (container);
  LigmaFilter      *filter = LIGMA_FILTER (object);

  if (stack->graph && ligma_filter_get_active (filter))
    {
      ligma_filter_stack_remove_node (stack, filter);
      gegl_node_remove_child (stack->graph, ligma_filter_get_node (filter));
    }

  LIGMA_CONTAINER_CLASS (parent_class)->remove (container, object);

  if (ligma_filter_get_active (filter))
    {
      ligma_filter_set_is_last_node (filter, FALSE);
      ligma_filter_stack_update_last_node (stack);
    }
}

static void
ligma_filter_stack_reorder (LigmaContainer *container,
                           LigmaObject    *object,
                           gint           new_index)
{
  LigmaFilterStack *stack  = LIGMA_FILTER_STACK (container);
  LigmaFilter      *filter = LIGMA_FILTER (object);

  if (stack->graph && ligma_filter_get_active (filter))
    ligma_filter_stack_remove_node (stack, filter);

  LIGMA_CONTAINER_CLASS (parent_class)->reorder (container, object, new_index);

  if (ligma_filter_get_active (filter))
    {
      ligma_filter_stack_update_last_node (stack);

      if (stack->graph)
        ligma_filter_stack_add_node (stack, filter);
    }
}


/*  public functions  */

LigmaContainer *
ligma_filter_stack_new (GType filter_type)
{
  g_return_val_if_fail (g_type_is_a (filter_type, LIGMA_TYPE_FILTER), NULL);

  return g_object_new (LIGMA_TYPE_FILTER_STACK,
                       "name",          g_type_name (filter_type),
                       "children-type", filter_type,
                       "policy",        LIGMA_CONTAINER_POLICY_STRONG,
                       NULL);
}

GeglNode *
ligma_filter_stack_get_graph (LigmaFilterStack *stack)
{
  GList    *list;
  GeglNode *previous;
  GeglNode *output;

  g_return_val_if_fail (LIGMA_IS_FILTER_STACK (stack), NULL);

  if (stack->graph)
    return stack->graph;

  stack->graph = gegl_node_new ();

  previous = gegl_node_get_input_proxy (stack->graph, "input");

  for (list = LIGMA_LIST (stack)->queue->tail;
       list;
       list = g_list_previous (list))
    {
      LigmaFilter *filter = list->data;
      GeglNode   *node;

      if (! ligma_filter_get_active (filter))
        continue;

      node = ligma_filter_get_node (filter);

      gegl_node_add_child (stack->graph, node);

      gegl_node_connect_to (previous, "output",
                            node,     "input");

      previous = node;
    }

  output = gegl_node_get_output_proxy (stack->graph, "output");

  gegl_node_connect_to (previous, "output",
                        output,   "input");

  return stack->graph;
}


/*  private functions  */

static void
ligma_filter_stack_add_node (LigmaFilterStack *stack,
                            LigmaFilter      *filter)
{
  GeglNode *node;
  GeglNode *node_above = NULL;
  GeglNode *node_below = NULL;
  GList    *iter;

  node = ligma_filter_get_node (filter);


  iter = g_list_find (LIGMA_LIST (stack)->queue->head, filter);

  while ((iter = g_list_previous (iter)))
    {
      LigmaFilter *filter_above = iter->data;

      if (ligma_filter_get_active (filter_above))
        {
          node_above = ligma_filter_get_node (filter_above);

          break;
        }
    }

  if (! node_above)
    node_above = gegl_node_get_output_proxy (stack->graph, "output");

  node_below = gegl_node_get_producer (node_above, "input", NULL);

  gegl_node_connect_to (node_below, "output",
                        node,       "input");
  gegl_node_connect_to (node,       "output",
                        node_above, "input");
}

static void
ligma_filter_stack_remove_node (LigmaFilterStack *stack,
                               LigmaFilter      *filter)
{
  GeglNode *node;
  GeglNode *node_above = NULL;
  GeglNode *node_below = NULL;
  GList    *iter;

  node = ligma_filter_get_node (filter);

  iter = g_list_find (LIGMA_LIST (stack)->queue->head, filter);

  while ((iter = g_list_previous (iter)))
    {
      LigmaFilter *filter_above = iter->data;

      if (ligma_filter_get_active (filter_above))
        {
          node_above = ligma_filter_get_node (filter_above);

          break;
        }
    }

  if (! node_above)
    node_above = gegl_node_get_output_proxy (stack->graph, "output");

  node_below = gegl_node_get_producer (node, "input", NULL);

  gegl_node_disconnect (node, "input");

  gegl_node_connect_to (node_below, "output",
                        node_above, "input");
}

static void
ligma_filter_stack_update_last_node (LigmaFilterStack *stack)
{
  GList    *list;
  gboolean  found_last = FALSE;

  for (list = LIGMA_LIST (stack)->queue->tail;
       list;
       list = g_list_previous (list))
    {
      LigmaFilter *filter = list->data;

      if (! found_last && ligma_filter_get_active (filter))
        {
          ligma_filter_set_is_last_node (filter, TRUE);
          found_last = TRUE;
        }
      else
        {
          ligma_filter_set_is_last_node (filter, FALSE);
        }
    }
}

static void
ligma_filter_stack_filter_active (LigmaFilter      *filter,
                                 LigmaFilterStack *stack)
{
  if (stack->graph)
    {
      if (ligma_filter_get_active (filter))
        {
          gegl_node_add_child (stack->graph, ligma_filter_get_node (filter));
          ligma_filter_stack_add_node (stack, filter);
        }
      else
        {
          ligma_filter_stack_remove_node (stack, filter);
          gegl_node_remove_child (stack->graph, ligma_filter_get_node (filter));
        }
    }

  ligma_filter_stack_update_last_node (stack);

  if (! ligma_filter_get_active (filter))
    ligma_filter_set_is_last_node (filter, FALSE);
}
