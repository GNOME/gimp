/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmadrawablestack.h
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

#ifndef __LIGMA_DRAWABLE_STACK_H__
#define __LIGMA_DRAWABLE_STACK_H__

#include "ligmaitemstack.h"


#define LIGMA_TYPE_DRAWABLE_STACK            (ligma_drawable_stack_get_type ())
#define LIGMA_DRAWABLE_STACK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DRAWABLE_STACK, LigmaDrawableStack))
#define LIGMA_DRAWABLE_STACK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DRAWABLE_STACK, LigmaDrawableStackClass))
#define LIGMA_IS_DRAWABLE_STACK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DRAWABLE_STACK))
#define LIGMA_IS_DRAWABLE_STACK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DRAWABLE_STACK))


typedef struct _LigmaDrawableStackClass LigmaDrawableStackClass;

struct _LigmaDrawableStack
{
  LigmaItemStack  parent_instance;
};

struct _LigmaDrawableStackClass
{
  LigmaItemStackClass  parent_class;

  void (* update) (LigmaDrawableStack *stack,
                   gint               x,
                   gint               y,
                   gint               width,
                   gint               height);
};


GType           ligma_drawable_stack_get_type  (void) G_GNUC_CONST;
LigmaContainer * ligma_drawable_stack_new       (GType              drawable_type);


/*  protected  */

void            ligma_drawable_stack_update    (LigmaDrawableStack *stack,
                                               gint               x,
                                               gint               y,
                                               gint               width,
                                               gint               height);


#endif  /*  __LIGMA_DRAWABLE_STACK_H__  */
