/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmalayerstack.c
 * Copyright (C) 2017 Ell
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

#include "ligmalayer.h"
#include "ligmalayerstack.h"


/*  local function prototypes  */

static void   ligma_layer_stack_constructed             (GObject       *object);

static void   ligma_layer_stack_add                     (LigmaContainer *container,
                                                        LigmaObject    *object);
static void   ligma_layer_stack_remove                  (LigmaContainer *container,
                                                        LigmaObject    *object);
static void   ligma_layer_stack_reorder                 (LigmaContainer *container,
                                                        LigmaObject    *object,
                                                        gint           new_index);

static void   ligma_layer_stack_layer_active            (LigmaLayer      *layer,
                                                        LigmaLayerStack *stack);
static void   ligma_layer_stack_layer_excludes_backdrop (LigmaLayer      *layer,
                                                        LigmaLayerStack *stack);

static void   ligma_layer_stack_update_backdrop         (LigmaLayerStack *stack,
                                                        LigmaLayer      *layer,
                                                        gboolean        ignore_active,
                                                        gboolean        ignore_excludes_backdrop);
static void   ligma_layer_stack_update_range            (LigmaLayerStack *stack,
                                                        gint            first,
                                                        gint            last);


G_DEFINE_TYPE (LigmaLayerStack, ligma_layer_stack, LIGMA_TYPE_DRAWABLE_STACK)

#define parent_class ligma_layer_stack_parent_class


static void
ligma_layer_stack_class_init (LigmaLayerStackClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  LigmaContainerClass *container_class = LIGMA_CONTAINER_CLASS (klass);

  object_class->constructed = ligma_layer_stack_constructed;

  container_class->add      = ligma_layer_stack_add;
  container_class->remove   = ligma_layer_stack_remove;
  container_class->reorder  = ligma_layer_stack_reorder;
}

static void
ligma_layer_stack_init (LigmaLayerStack *stack)
{
}

static void
ligma_layer_stack_constructed (GObject *object)
{
  LigmaContainer *container = LIGMA_CONTAINER (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (g_type_is_a (ligma_container_get_children_type (container),
                            LIGMA_TYPE_LAYER));

  ligma_container_add_handler (container, "active-changed",
                              G_CALLBACK (ligma_layer_stack_layer_active),
                              container);
  ligma_container_add_handler (container, "excludes-backdrop-changed",
                              G_CALLBACK (ligma_layer_stack_layer_excludes_backdrop),
                              container);
}

static void
ligma_layer_stack_add (LigmaContainer *container,
                      LigmaObject    *object)
{
  LigmaLayerStack *stack = LIGMA_LAYER_STACK (container);

  LIGMA_CONTAINER_CLASS (parent_class)->add (container, object);

  ligma_layer_stack_update_backdrop (stack, LIGMA_LAYER (object), FALSE, FALSE);
}

static void
ligma_layer_stack_remove (LigmaContainer *container,
                         LigmaObject    *object)
{
  LigmaLayerStack *stack = LIGMA_LAYER_STACK (container);
  gboolean        update_backdrop;
  gint            index;

  update_backdrop = ligma_filter_get_active (LIGMA_FILTER (object)) &&
                    ligma_layer_get_excludes_backdrop (LIGMA_LAYER (object));

  if (update_backdrop)
    index = ligma_container_get_child_index (container, object);

  LIGMA_CONTAINER_CLASS (parent_class)->remove (container, object);

  if (update_backdrop)
    ligma_layer_stack_update_range (stack, index, -1);
}

static void
ligma_layer_stack_reorder (LigmaContainer *container,
                          LigmaObject    *object,
                          gint           new_index)
{
  LigmaLayerStack *stack = LIGMA_LAYER_STACK (container);
  gboolean        update_backdrop;
  gint            index;

  update_backdrop = ligma_filter_get_active (LIGMA_FILTER (object)) &&
                    ligma_layer_get_excludes_backdrop (LIGMA_LAYER (object));

  if (update_backdrop)
    index = ligma_container_get_child_index (container, object);

  LIGMA_CONTAINER_CLASS (parent_class)->reorder (container, object, new_index);

  if (update_backdrop)
    ligma_layer_stack_update_range (stack, index, new_index);
}


/*  public functions  */

LigmaContainer *
ligma_layer_stack_new (GType layer_type)
{
  g_return_val_if_fail (g_type_is_a (layer_type, LIGMA_TYPE_LAYER), NULL);

  return g_object_new (LIGMA_TYPE_LAYER_STACK,
                       "name",          g_type_name (layer_type),
                       "children-type", layer_type,
                       "policy",        LIGMA_CONTAINER_POLICY_STRONG,
                       NULL);
}


/*  private functions  */

static void
ligma_layer_stack_layer_active (LigmaLayer      *layer,
                               LigmaLayerStack *stack)
{
  ligma_layer_stack_update_backdrop (stack, layer, TRUE, FALSE);
}

static void
ligma_layer_stack_layer_excludes_backdrop (LigmaLayer      *layer,
                                          LigmaLayerStack *stack)
{
  ligma_layer_stack_update_backdrop (stack, layer, FALSE, TRUE);
}

static void
ligma_layer_stack_update_backdrop (LigmaLayerStack *stack,
                                  LigmaLayer      *layer,
                                  gboolean        ignore_active,
                                  gboolean        ignore_excludes_backdrop)
{
  if ((ignore_active            || ligma_filter_get_active (LIGMA_FILTER (layer))) &&
      (ignore_excludes_backdrop || ligma_layer_get_excludes_backdrop (layer)))
    {
      gint index;

      index = ligma_container_get_child_index (LIGMA_CONTAINER (stack),
                                              LIGMA_OBJECT (layer));

      ligma_layer_stack_update_range (stack, index + 1, -1);
    }
}

static void
ligma_layer_stack_update_range (LigmaLayerStack *stack,
                               gint            first,
                               gint            last)
{
  GList *iter;

  g_return_if_fail (first >= 0 && last >= -1);

  /* if the range is reversed, flip first and last; note that last == -1 is
   * used to update all layers from first onward.
   */
  if (last >= 0 && last < first)
    {
      gint temp = first;

      first = last + 1;
      last  = temp + 1;
    }

  iter = ligma_item_stack_get_item_iter (LIGMA_ITEM_STACK (stack));

  for (iter = g_list_nth (iter, first);
       iter && first != last;
       iter = g_list_next (iter), first++)
    {
      LigmaItem *item = iter->data;

      if (ligma_filter_get_active (LIGMA_FILTER (item)))
        {
          GeglRectangle bounding_box;

          bounding_box = ligma_drawable_get_bounding_box (LIGMA_DRAWABLE (item));

          bounding_box.x += ligma_item_get_offset_x (item);
          bounding_box.y += ligma_item_get_offset_y (item);

          ligma_drawable_stack_update (LIGMA_DRAWABLE_STACK (stack),
                                      bounding_box.x,     bounding_box.y,
                                      bounding_box.width, bounding_box.height);
        }
    }
}
