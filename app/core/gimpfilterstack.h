/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmafilterstack.h
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

#ifndef __LIGMA_FILTER_STACK_H__
#define __LIGMA_FILTER_STACK_H__

#include "ligmalist.h"


#define LIGMA_TYPE_FILTER_STACK            (ligma_filter_stack_get_type ())
#define LIGMA_FILTER_STACK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_FILTER_STACK, LigmaFilterStack))
#define LIGMA_FILTER_STACK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_FILTER_STACK, LigmaFilterStackClass))
#define LIGMA_IS_FILTER_STACK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_FILTER_STACK))
#define LIGMA_IS_FILTER_STACK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_FILTER_STACK))


typedef struct _LigmaFilterStackClass LigmaFilterStackClass;

struct _LigmaFilterStack
{
  LigmaList  parent_instance;

  GeglNode *graph;
};

struct _LigmaFilterStackClass
{
  LigmaListClass  parent_class;
};


GType           ligma_filter_stack_get_type  (void) G_GNUC_CONST;
LigmaContainer * ligma_filter_stack_new       (GType            filter_type);

GeglNode *      ligma_filter_stack_get_graph (LigmaFilterStack *stack);


#endif  /*  __LIGMA_FILTER_STACK_H__  */
