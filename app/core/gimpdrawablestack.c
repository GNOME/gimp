/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmadrawablestack.c
 * Copyright (C) 2008 Michael Natterer <mitch@ligma.org>
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

#include "ligmadrawable.h"
#include "ligmadrawablestack.h"
#include "ligmamarshal.h"


enum
{
  UPDATE,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void   ligma_drawable_stack_constructed      (GObject           *object);

static void   ligma_drawable_stack_add              (LigmaContainer     *container,
                                                    LigmaObject        *object);
static void   ligma_drawable_stack_remove           (LigmaContainer     *container,
                                                    LigmaObject        *object);
static void   ligma_drawable_stack_reorder          (LigmaContainer     *container,
                                                    LigmaObject        *object,
                                                    gint               new_index);

static void   ligma_drawable_stack_drawable_update  (LigmaItem          *item,
                                                    gint               x,
                                                    gint               y,
                                                    gint               width,
                                                    gint               height,
                                                    LigmaDrawableStack *stack);
static void   ligma_drawable_stack_drawable_active  (LigmaItem          *item,
                                                    LigmaDrawableStack *stack);


G_DEFINE_TYPE (LigmaDrawableStack, ligma_drawable_stack, LIGMA_TYPE_ITEM_STACK)

#define parent_class ligma_drawable_stack_parent_class

static guint stack_signals[LAST_SIGNAL] = { 0 };


static void
ligma_drawable_stack_class_init (LigmaDrawableStackClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  LigmaContainerClass *container_class = LIGMA_CONTAINER_CLASS (klass);

  stack_signals[UPDATE] =
    g_signal_new ("update",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaDrawableStackClass, update),
                  NULL, NULL,
                  ligma_marshal_VOID__INT_INT_INT_INT,
                  G_TYPE_NONE, 4,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT);

  object_class->constructed = ligma_drawable_stack_constructed;

  container_class->add      = ligma_drawable_stack_add;
  container_class->remove   = ligma_drawable_stack_remove;
  container_class->reorder  = ligma_drawable_stack_reorder;
}

static void
ligma_drawable_stack_init (LigmaDrawableStack *stack)
{
}

static void
ligma_drawable_stack_constructed (GObject *object)
{
  LigmaContainer *container = LIGMA_CONTAINER (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (g_type_is_a (ligma_container_get_children_type (container),
                            LIGMA_TYPE_DRAWABLE));

  ligma_container_add_handler (container, "update",
                              G_CALLBACK (ligma_drawable_stack_drawable_update),
                              container);
  ligma_container_add_handler (container, "active-changed",
                              G_CALLBACK (ligma_drawable_stack_drawable_active),
                              container);
}

static void
ligma_drawable_stack_add (LigmaContainer *container,
                         LigmaObject    *object)
{
  LigmaDrawableStack *stack = LIGMA_DRAWABLE_STACK (container);

  LIGMA_CONTAINER_CLASS (parent_class)->add (container, object);

  if (ligma_filter_get_active (LIGMA_FILTER (object)))
    ligma_drawable_stack_drawable_active (LIGMA_ITEM (object), stack);
}

static void
ligma_drawable_stack_remove (LigmaContainer *container,
                            LigmaObject    *object)
{
  LigmaDrawableStack *stack = LIGMA_DRAWABLE_STACK (container);

  LIGMA_CONTAINER_CLASS (parent_class)->remove (container, object);

  if (ligma_filter_get_active (LIGMA_FILTER (object)))
    ligma_drawable_stack_drawable_active (LIGMA_ITEM (object), stack);
}

static void
ligma_drawable_stack_reorder (LigmaContainer *container,
                             LigmaObject    *object,
                             gint           new_index)
{
  LigmaDrawableStack *stack  = LIGMA_DRAWABLE_STACK (container);

  LIGMA_CONTAINER_CLASS (parent_class)->reorder (container, object, new_index);

  if (ligma_filter_get_active (LIGMA_FILTER (object)))
    ligma_drawable_stack_drawable_active (LIGMA_ITEM (object), stack);
}


/*  public functions  */

LigmaContainer *
ligma_drawable_stack_new (GType drawable_type)
{
  g_return_val_if_fail (g_type_is_a (drawable_type, LIGMA_TYPE_DRAWABLE), NULL);

  return g_object_new (LIGMA_TYPE_DRAWABLE_STACK,
                       "name",          g_type_name (drawable_type),
                       "children-type", drawable_type,
                       "policy",        LIGMA_CONTAINER_POLICY_STRONG,
                       NULL);
}


/*  protected functions  */

void
ligma_drawable_stack_update (LigmaDrawableStack *stack,
                            gint               x,
                            gint               y,
                            gint               width,
                            gint               height)
{
  g_return_if_fail (LIGMA_IS_DRAWABLE_STACK (stack));

  g_signal_emit (stack, stack_signals[UPDATE], 0,
                 x, y, width, height);
}


/*  private functions  */

static void
ligma_drawable_stack_drawable_update (LigmaItem          *item,
                                     gint               x,
                                     gint               y,
                                     gint               width,
                                     gint               height,
                                     LigmaDrawableStack *stack)
{
  if (ligma_filter_get_active (LIGMA_FILTER (item)))
    {
      gint offset_x;
      gint offset_y;

      ligma_item_get_offset (item, &offset_x, &offset_y);

      ligma_drawable_stack_update (stack,
                                  x + offset_x, y + offset_y,
                                  width, height);
    }
}

static void
ligma_drawable_stack_drawable_active (LigmaItem          *item,
                                     LigmaDrawableStack *stack)
{
  GeglRectangle bounding_box;

  bounding_box = ligma_drawable_get_bounding_box (LIGMA_DRAWABLE (item));

  bounding_box.x += ligma_item_get_offset_x (item);
  bounding_box.y += ligma_item_get_offset_y (item);

  ligma_drawable_stack_update (stack,
                              bounding_box.x,     bounding_box.y,
                              bounding_box.width, bounding_box.height);
}
