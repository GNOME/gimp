/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpfilterstack.h
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

#pragma once

#include "gimplist.h"


#define GIMP_TYPE_FILTER_STACK            (gimp_filter_stack_get_type ())
#define GIMP_FILTER_STACK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_FILTER_STACK, GimpFilterStack))
#define GIMP_FILTER_STACK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_FILTER_STACK, GimpFilterStackClass))
#define GIMP_IS_FILTER_STACK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_FILTER_STACK))
#define GIMP_IS_FILTER_STACK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_FILTER_STACK))


typedef struct _GimpFilterStackClass GimpFilterStackClass;

struct _GimpFilterStack
{
  GimpList  parent_instance;

  GeglNode *graph;
};

struct _GimpFilterStackClass
{
  GimpListClass  parent_class;
};


GType           gimp_filter_stack_get_type  (void) G_GNUC_CONST;
GimpContainer * gimp_filter_stack_new       (GType            filter_type);

GeglNode *      gimp_filter_stack_get_graph (GimpFilterStack *stack);
