/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimplayerstack.h
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

#ifndef __GIMP_LAYER_STACK_H__
#define __GIMP_LAYER_STACK_H__

#include "gimpdrawablestack.h"


#define GIMP_TYPE_LAYER_STACK            (gimp_layer_stack_get_type ())
#define GIMP_LAYER_STACK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_LAYER_STACK, GimpLayerStack))
#define GIMP_LAYER_STACK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_LAYER_STACK, GimpLayerStackClass))
#define GIMP_IS_LAYER_STACK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_LAYER_STACK))
#define GIMP_IS_LAYER_STACK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_LAYER_STACK))


typedef struct _GimpLayerStackClass GimpLayerStackClass;

struct _GimpLayerStack
{
  GimpDrawableStack  parent_instance;
};

struct _GimpLayerStackClass
{
  GimpDrawableStackClass  parent_class;
};


GType           gimp_layer_stack_get_type  (void) G_GNUC_CONST;
GimpContainer * gimp_layer_stack_new       (GType layer_type);


#endif  /*  __GIMP_LAYER_STACK_H__  */
