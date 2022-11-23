/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __LIGMA_PENCIL_H__
#define __LIGMA_PENCIL_H__


#include "ligmapaintbrush.h"


#define LIGMA_TYPE_PENCIL            (ligma_pencil_get_type ())
#define LIGMA_PENCIL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PENCIL, LigmaPencil))
#define LIGMA_PENCIL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PENCIL, LigmaPencilClass))
#define LIGMA_IS_PENCIL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PENCIL))
#define LIGMA_IS_PENCIL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PENCIL))
#define LIGMA_PENCIL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PENCIL, LigmaPencilClass))


typedef struct _LigmaPencilClass LigmaPencilClass;

struct _LigmaPencil
{
  LigmaPaintbrush  parent_instance;
};

struct _LigmaPencilClass
{
  LigmaPaintbrushClass  parent_class;
};


void    ligma_pencil_register (Ligma                      *ligma,
                              LigmaPaintRegisterCallback  callback);

GType   ligma_pencil_get_type (void) G_GNUC_CONST;


#endif  /*  __LIGMA_PENCIL_H__  */
