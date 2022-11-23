/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmalayerstack.h
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

#ifndef __LIGMA_LAYER_STACK_H__
#define __LIGMA_LAYER_STACK_H__

#include "ligmadrawablestack.h"


#define LIGMA_TYPE_LAYER_STACK            (ligma_layer_stack_get_type ())
#define LIGMA_LAYER_STACK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_LAYER_STACK, LigmaLayerStack))
#define LIGMA_LAYER_STACK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_LAYER_STACK, LigmaLayerStackClass))
#define LIGMA_IS_LAYER_STACK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_LAYER_STACK))
#define LIGMA_IS_LAYER_STACK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_LAYER_STACK))


typedef struct _LigmaLayerStackClass LigmaLayerStackClass;

struct _LigmaLayerStack
{
  LigmaDrawableStack  parent_instance;
};

struct _LigmaLayerStackClass
{
  LigmaDrawableStackClass  parent_class;
};


GType           ligma_layer_stack_get_type  (void) G_GNUC_CONST;
LigmaContainer * ligma_layer_stack_new       (GType layer_type);


#endif  /*  __LIGMA_LAYER_STACK_H__  */
