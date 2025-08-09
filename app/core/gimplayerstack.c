/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimplayerstack.c
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

#include "gimplayer.h"
#include "gimplayerstack.h"


/*  local function prototypes  */

static void   gimp_layer_stack_constructed             (GObject       *object);

static void   gimp_layer_stack_add                     (GimpContainer *container,
                                                        GimpObject    *object);
static void   gimp_layer_stack_remove                  (GimpContainer *container,
                                                        GimpObject    *object);
static void   gimp_layer_stack_reorder                 (GimpContainer *container,
                                                        GimpObject    *object,
                                                        gint           old_index,
                                                        gint           new_index);

static void   gimp_layer_stack_layer_active            (GimpLayer      *layer,
                                                        GimpLayerStack *stack);
static void   gimp_layer_stack_layer_excludes_backdrop (GimpLayer      *layer,
                                                        GimpLayerStack *stack);

static void   gimp_layer_stack_update_backdrop         (GimpLayerStack *stack,
                                                        GimpLayer      *layer,
                                                        gboolean        ignore_active,
                                                        gboolean        ignore_excludes_backdrop);
static void   gimp_layer_stack_update_range            (GimpLayerStack *stack,
                                                        gint            first,
                                                        gint            last);


G_DEFINE_TYPE (GimpLayerStack, gimp_layer_stack, GIMP_TYPE_DRAWABLE_STACK)

#define parent_class gimp_layer_stack_parent_class


static void
gimp_layer_stack_class_init (GimpLayerStackClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GimpContainerClass *container_class = GIMP_CONTAINER_CLASS (klass);

  object_class->constructed = gimp_layer_stack_constructed;

  container_class->add      = gimp_layer_stack_add;
  container_class->remove   = gimp_layer_stack_remove;
  container_class->reorder  = gimp_layer_stack_reorder;
}

static void
gimp_layer_stack_init (GimpLayerStack *stack)
{
}

static void
gimp_layer_stack_constructed (GObject *object)
{
  GimpContainer *container = GIMP_CONTAINER (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (g_type_is_a (gimp_container_get_child_type (container),
                            GIMP_TYPE_LAYER));

  gimp_container_add_handler (container, "active-changed",
                              G_CALLBACK (gimp_layer_stack_layer_active),
                              container);
  gimp_container_add_handler (container, "excludes-backdrop-changed",
                              G_CALLBACK (gimp_layer_stack_layer_excludes_backdrop),
                              container);
}

static void
gimp_layer_stack_add (GimpContainer *container,
                      GimpObject    *object)
{
  GimpLayerStack *stack = GIMP_LAYER_STACK (container);

  GIMP_CONTAINER_CLASS (parent_class)->add (container, object);

  gimp_layer_stack_update_backdrop (stack, GIMP_LAYER (object), FALSE, FALSE);
}

static void
gimp_layer_stack_remove (GimpContainer *container,
                         GimpObject    *object)
{
  GimpLayerStack *stack = GIMP_LAYER_STACK (container);
  gboolean        update_backdrop;
  gint            index;

  update_backdrop = gimp_filter_get_active (GIMP_FILTER (object)) &&
                    gimp_layer_get_excludes_backdrop (GIMP_LAYER (object));

  if (update_backdrop)
    index = gimp_container_get_child_index (container, object);

  GIMP_CONTAINER_CLASS (parent_class)->remove (container, object);

  if (update_backdrop)
    gimp_layer_stack_update_range (stack, index, -1);
}

static void
gimp_layer_stack_reorder (GimpContainer *container,
                          GimpObject    *object,
                          gint           old_index,
                          gint           new_index)
{
  GimpLayerStack *stack = GIMP_LAYER_STACK (container);
  gboolean        update_backdrop;

  update_backdrop = gimp_filter_get_active (GIMP_FILTER (object)) &&
                    gimp_layer_get_excludes_backdrop (GIMP_LAYER (object));

  GIMP_CONTAINER_CLASS (parent_class)->reorder (container, object,
                                                old_index, new_index);

  if (update_backdrop)
    gimp_layer_stack_update_range (stack, old_index, new_index);
}


/*  public functions  */

GimpContainer *
gimp_layer_stack_new (GType layer_type)
{
  g_return_val_if_fail (g_type_is_a (layer_type, GIMP_TYPE_LAYER), NULL);

  return g_object_new (GIMP_TYPE_LAYER_STACK,
                       "name",       g_type_name (layer_type),
                       "child-type", layer_type,
                       "policy",     GIMP_CONTAINER_POLICY_STRONG,
                       NULL);
}


/*  private functions  */

static void
gimp_layer_stack_layer_active (GimpLayer      *layer,
                               GimpLayerStack *stack)
{
  gimp_layer_stack_update_backdrop (stack, layer, TRUE, FALSE);
}

static void
gimp_layer_stack_layer_excludes_backdrop (GimpLayer      *layer,
                                          GimpLayerStack *stack)
{
  gimp_layer_stack_update_backdrop (stack, layer, FALSE, TRUE);
}

static void
gimp_layer_stack_update_backdrop (GimpLayerStack *stack,
                                  GimpLayer      *layer,
                                  gboolean        ignore_active,
                                  gboolean        ignore_excludes_backdrop)
{
  if ((ignore_active            || gimp_filter_get_active (GIMP_FILTER (layer))) &&
      (ignore_excludes_backdrop || gimp_layer_get_excludes_backdrop (layer)))
    {
      gint index;

      index = gimp_container_get_child_index (GIMP_CONTAINER (stack),
                                              GIMP_OBJECT (layer));

      gimp_layer_stack_update_range (stack, index + 1, -1);
    }
}

static void
gimp_layer_stack_update_range (GimpLayerStack *stack,
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

  iter = gimp_item_stack_get_item_iter (GIMP_ITEM_STACK (stack));

  for (iter = g_list_nth (iter, first);
       iter && first != last;
       iter = g_list_next (iter), first++)
    {
      GimpItem *item = iter->data;

      if (gimp_filter_get_active (GIMP_FILTER (item)))
        {
          GeglRectangle bounding_box;

          bounding_box = gimp_drawable_get_bounding_box (GIMP_DRAWABLE (item));

          bounding_box.x += gimp_item_get_offset_x (item);
          bounding_box.y += gimp_item_get_offset_y (item);

          gimp_drawable_stack_update (GIMP_DRAWABLE_STACK (stack),
                                      bounding_box.x,     bounding_box.y,
                                      bounding_box.width, bounding_box.height);
        }
    }
}
