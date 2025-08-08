/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpdrawablestack.c
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
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

#include "gimpdrawable.h"
#include "gimpdrawablestack.h"
#include "gimpmarshal.h"


enum
{
  UPDATE,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void   gimp_drawable_stack_constructed      (GObject           *object);

static void   gimp_drawable_stack_add              (GimpContainer     *container,
                                                    GimpObject        *object);
static void   gimp_drawable_stack_remove           (GimpContainer     *container,
                                                    GimpObject        *object);
static void   gimp_drawable_stack_reorder          (GimpContainer     *container,
                                                    GimpObject        *object,
                                                    gint               old_index,
                                                    gint               new_index);

static void   gimp_drawable_stack_drawable_update  (GimpItem          *item,
                                                    gint               x,
                                                    gint               y,
                                                    gint               width,
                                                    gint               height,
                                                    GimpDrawableStack *stack);
static void   gimp_drawable_stack_drawable_active  (GimpItem          *item,
                                                    GimpDrawableStack *stack);


G_DEFINE_TYPE (GimpDrawableStack, gimp_drawable_stack, GIMP_TYPE_ITEM_STACK)

#define parent_class gimp_drawable_stack_parent_class

static guint stack_signals[LAST_SIGNAL] = { 0 };


static void
gimp_drawable_stack_class_init (GimpDrawableStackClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GimpContainerClass *container_class = GIMP_CONTAINER_CLASS (klass);

  stack_signals[UPDATE] =
    g_signal_new ("update",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDrawableStackClass, update),
                  NULL, NULL,
                  gimp_marshal_VOID__INT_INT_INT_INT,
                  G_TYPE_NONE, 4,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT);

  object_class->constructed = gimp_drawable_stack_constructed;

  container_class->add      = gimp_drawable_stack_add;
  container_class->remove   = gimp_drawable_stack_remove;
  container_class->reorder  = gimp_drawable_stack_reorder;
}

static void
gimp_drawable_stack_init (GimpDrawableStack *stack)
{
}

static void
gimp_drawable_stack_constructed (GObject *object)
{
  GimpContainer *container = GIMP_CONTAINER (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (g_type_is_a (gimp_container_get_child_type (container),
                            GIMP_TYPE_DRAWABLE));

  gimp_container_add_handler (container, "update",
                              G_CALLBACK (gimp_drawable_stack_drawable_update),
                              container);
  gimp_container_add_handler (container, "active-changed",
                              G_CALLBACK (gimp_drawable_stack_drawable_active),
                              container);
}

static void
gimp_drawable_stack_add (GimpContainer *container,
                         GimpObject    *object)
{
  GimpDrawableStack *stack = GIMP_DRAWABLE_STACK (container);

  GIMP_CONTAINER_CLASS (parent_class)->add (container, object);

  if (gimp_filter_get_active (GIMP_FILTER (object)))
    gimp_drawable_stack_drawable_active (GIMP_ITEM (object), stack);
}

static void
gimp_drawable_stack_remove (GimpContainer *container,
                            GimpObject    *object)
{
  GimpDrawableStack *stack = GIMP_DRAWABLE_STACK (container);

  GIMP_CONTAINER_CLASS (parent_class)->remove (container, object);

  if (gimp_filter_get_active (GIMP_FILTER (object)))
    gimp_drawable_stack_drawable_active (GIMP_ITEM (object), stack);
}

static void
gimp_drawable_stack_reorder (GimpContainer *container,
                             GimpObject    *object,
                             gint           old_index,
                             gint           new_index)
{
  GimpDrawableStack *stack  = GIMP_DRAWABLE_STACK (container);

  GIMP_CONTAINER_CLASS (parent_class)->reorder (container, object,
                                                old_index, new_index);

  if (gimp_filter_get_active (GIMP_FILTER (object)))
    gimp_drawable_stack_drawable_active (GIMP_ITEM (object), stack);
}


/*  public functions  */

GimpContainer *
gimp_drawable_stack_new (GType drawable_type)
{
  g_return_val_if_fail (g_type_is_a (drawable_type, GIMP_TYPE_DRAWABLE), NULL);

  return g_object_new (GIMP_TYPE_DRAWABLE_STACK,
                       "name",       g_type_name (drawable_type),
                       "child-type", drawable_type,
                       "policy",     GIMP_CONTAINER_POLICY_STRONG,
                       NULL);
}


/*  protected functions  */

void
gimp_drawable_stack_update (GimpDrawableStack *stack,
                            gint               x,
                            gint               y,
                            gint               width,
                            gint               height)
{
  g_return_if_fail (GIMP_IS_DRAWABLE_STACK (stack));

  g_signal_emit (stack, stack_signals[UPDATE], 0,
                 x, y, width, height);
}


/*  private functions  */

static void
gimp_drawable_stack_drawable_update (GimpItem          *item,
                                     gint               x,
                                     gint               y,
                                     gint               width,
                                     gint               height,
                                     GimpDrawableStack *stack)
{
  if (gimp_filter_get_active (GIMP_FILTER (item)))
    {
      gint offset_x;
      gint offset_y;

      gimp_item_get_offset (item, &offset_x, &offset_y);

      gimp_drawable_stack_update (stack,
                                  x + offset_x, y + offset_y,
                                  width, height);
    }
}

static void
gimp_drawable_stack_drawable_active (GimpItem          *item,
                                     GimpDrawableStack *stack)
{
  GeglRectangle bounding_box;

  bounding_box = gimp_drawable_get_bounding_box (GIMP_DRAWABLE (item));

  bounding_box.x += gimp_item_get_offset_x (item);
  bounding_box.y += gimp_item_get_offset_y (item);

  gimp_drawable_stack_update (stack,
                              bounding_box.x,     bounding_box.y,
                              bounding_box.width, bounding_box.height);
}
