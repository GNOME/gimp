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


/*  local function prototypes  */

static void   gimp_drawable_stack_finalize (GObject       *object);

static void   gimp_drawable_stack_add      (GimpContainer *container,
                                            GimpObject    *object);
static void   gimp_drawable_stack_remove   (GimpContainer *container,
                                            GimpObject    *object);
static void   gimp_drawable_stack_reorder  (GimpContainer *container,
                                            GimpObject    *object,
                                            gint           new_index);


G_DEFINE_TYPE (GimpDrawableStack, gimp_drawable_stack, GIMP_TYPE_LIST)

#define parent_class gimp_drawable_stack_parent_class


static void
gimp_drawable_stack_class_init (GimpDrawableStackClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GimpContainerClass *container_class = GIMP_CONTAINER_CLASS (klass);

  object_class->finalize   = gimp_drawable_stack_finalize;

  container_class->add     = gimp_drawable_stack_add;
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
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_drawable_stack_add (GimpContainer *container,
                         GimpObject    *object)
{
  GIMP_CONTAINER_CLASS (parent_class)->add (container, object);
}

static void
gimp_drawable_stack_remove (GimpContainer *container,
                            GimpObject    *object)
{
  GIMP_CONTAINER_CLASS (parent_class)->remove (container, object);
}

static void
gimp_drawable_stack_reorder (GimpContainer *container,
                             GimpObject    *object,
                             gint           new_index)
{
  GIMP_CONTAINER_CLASS (parent_class)->reorder (container, object, new_index);
}

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
