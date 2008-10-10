/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpdrawablestack.c
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
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

#include <gegl.h>

#include "core-types.h"

#include "gimpdrawable.h"
#include "gimpdrawablestack.h"


#ifdef __GNUC__
#warning FIXME: gegl_node_add_child() needs to be public
#endif
GeglNode * gegl_node_add_child (GeglNode *self,
                                GeglNode *child);

#ifdef __GNUC__
#warning FIXME: gegl_node_remove_child() needs to be public
#endif
GeglNode * gegl_node_remove_child (GeglNode *self,
                                   GeglNode *child);


/*  local function prototypes  */

static void   gimp_drawable_stack_finalize    (GObject           *object);

static void   gimp_drawable_stack_remove      (GimpContainer     *container,
                                               GimpObject        *object);
static void   gimp_drawable_stack_reorder     (GimpContainer     *container,
                                               GimpObject        *object,
                                               gint               new_index);

static void   gimp_drawable_stack_add_node    (GimpDrawableStack *stack,
                                               GimpDrawable      *drawable);
static void   gimp_drawable_stack_remove_node (GimpDrawableStack *stack,
                                               GimpDrawable      *drawable);


G_DEFINE_TYPE (GimpDrawableStack, gimp_drawable_stack, GIMP_TYPE_LIST)

#define parent_class gimp_drawable_stack_parent_class


static void
gimp_drawable_stack_class_init (GimpDrawableStackClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GimpContainerClass *container_class = GIMP_CONTAINER_CLASS (klass);

  object_class->finalize   = gimp_drawable_stack_finalize;

  container_class->remove  = gimp_drawable_stack_remove;
  container_class->reorder = gimp_drawable_stack_reorder;
}

static void
gimp_drawable_stack_init (GimpDrawableStack *list)
{
}

static void
gimp_drawable_stack_finalize (GObject *object)
{
  GimpDrawableStack *stack = GIMP_DRAWABLE_STACK (object);

  if (stack->graph)
    {
      g_object_unref (stack->graph);
      stack->graph = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_drawable_stack_remove (GimpContainer *container,
                            GimpObject    *object)
{
  GimpDrawableStack *stack = GIMP_DRAWABLE_STACK (container);

  if (stack->graph)
    {
      gimp_drawable_stack_remove_node (stack, GIMP_DRAWABLE (object));

      gegl_node_remove_child (stack->graph,
                              gimp_drawable_get_node (GIMP_DRAWABLE (object)));
    }

  GIMP_CONTAINER_CLASS (parent_class)->remove (container, object);
}

static void
gimp_drawable_stack_reorder (GimpContainer *container,
                             GimpObject    *object,
                             gint           new_index)
{
  GimpDrawableStack *stack = GIMP_DRAWABLE_STACK (container);

  if (stack->graph)
    gimp_drawable_stack_remove_node (stack, GIMP_DRAWABLE (object));

  GIMP_CONTAINER_CLASS (parent_class)->reorder (container, object, new_index);

  if (stack->graph)
    gimp_drawable_stack_add_node (stack, GIMP_DRAWABLE (object));
}


/*  public functions  */

GimpContainer *
gimp_drawable_stack_new (GType drawable_type)
{
  g_return_val_if_fail (g_type_is_a (drawable_type, GIMP_TYPE_DRAWABLE), NULL);

  return g_object_new (GIMP_TYPE_DRAWABLE_STACK,
                       "name",          g_type_name (drawable_type),
                       "children-type", drawable_type,
                       "policy",        GIMP_CONTAINER_POLICY_STRONG,
                       "unique-names",  TRUE,
                       NULL);
}

GeglNode *
gimp_drawable_stack_get_graph (GimpDrawableStack *stack)
{
  GList    *list;
  GList    *reverse_list = NULL;
  GeglNode *previous     = NULL;
  GeglNode *output;

  g_return_val_if_fail (GIMP_IS_DRAWABLE_STACK (stack), NULL);

  if (stack->graph)
    return stack->graph;

  for (list = GIMP_LIST (stack)->list;
       list;
       list = g_list_next (list))
    {
      GimpDrawable *drawable = list->data;

      reverse_list = g_list_prepend (reverse_list, drawable);
    }

  stack->graph = gegl_node_new ();

  for (list = reverse_list; list; list = g_list_next (list))
    {
      GimpDrawable *drawable = list->data;
      GeglNode     *node     = gimp_drawable_get_node (drawable);

      gegl_node_add_child (stack->graph, node);

      if (previous)
        gegl_node_connect_to (previous, "output",
                              node,     "input");

      previous = node;
    }

  output = gegl_node_get_output_proxy (stack->graph, "output");

  if (previous)
    gegl_node_connect_to (previous, "output",
                          output,   "input");

  return stack->graph;
}


/*  private functions  */

static void
gimp_drawable_stack_add_node (GimpDrawableStack *stack,
                              GimpDrawable      *drawable)
{
  GimpDrawable *drawable_below;
  GeglNode     *node;
  gint          index;

  node = gimp_drawable_get_node (drawable);

  index = gimp_container_get_child_index (GIMP_CONTAINER (stack),
                                          GIMP_OBJECT (drawable));

  if (index == 0)
    {
      GeglNode *output;

      output = gegl_node_get_output_proxy (stack->graph, "output");

      gegl_node_connect_to (node,   "output",
                            output, "input");
    }
  else
    {
      GimpDrawable *drawable_above;
      GeglNode     *node_above;

      drawable_above = (GimpDrawable *)
        gimp_container_get_child_by_index (GIMP_CONTAINER (stack), index - 1);

      node_above = gimp_drawable_get_node (drawable_above);

      gegl_node_connect_to (node,       "output",
                            node_above, "input");
    }

  drawable_below = (GimpDrawable *)
    gimp_container_get_child_by_index (GIMP_CONTAINER (stack), index + 1);

  if (drawable_below)
    {
      GeglNode *node_below = gimp_drawable_get_node (drawable_below);

      gegl_node_connect_to (node_below, "output",
                            node,       "input");
    }
}

static void
gimp_drawable_stack_remove_node (GimpDrawableStack *stack,
                                 GimpDrawable      *drawable)
{
  GimpDrawable *drawable_below;
  GeglNode     *node;
  gint          index;

  node = gimp_drawable_get_node (GIMP_DRAWABLE (drawable));

  /*  bail out if the layer was newly added  */
  if (! gegl_node_get_consumers (node, "output", NULL, NULL))
    return;

  index = gimp_container_get_child_index (GIMP_CONTAINER (stack),
                                          GIMP_OBJECT (drawable));

  drawable_below = (GimpDrawable *)
    gimp_container_get_child_by_index (GIMP_CONTAINER (stack), index + 1);

  if (index == 0)
    {
      if (drawable_below)
        {
          GeglNode *node_below;
          GeglNode *output;

          node_below = gimp_drawable_get_node (drawable_below);

          output = gegl_node_get_output_proxy (stack->graph, "output");

          gegl_node_disconnect (node,       "input");
          gegl_node_connect_to (node_below, "output",
                                output,     "input");
        }
    }
  else
    {
      GimpDrawable *drawable_above;
      GeglNode     *node_above;

      drawable_above = (GimpDrawable *)
        gimp_container_get_child_by_index (GIMP_CONTAINER (stack), index - 1);

      node_above = gimp_drawable_get_node (drawable_above);

      if (drawable_below)
        {
          GeglNode *node_below;

          node_below = gimp_drawable_get_node (drawable_below);

          gegl_node_disconnect (node,       "input");
          gegl_node_connect_to (node_below, "output",
                                node_above, "input");
        }
      else
        {
          gegl_node_disconnect (node_above, "input");
        }
    }
}
