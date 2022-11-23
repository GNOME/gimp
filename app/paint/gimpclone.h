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

#ifndef __LIGMA_CLONE_H__
#define __LIGMA_CLONE_H__


#include "ligmasourcecore.h"


#define LIGMA_TYPE_CLONE            (ligma_clone_get_type ())
#define LIGMA_CLONE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CLONE, LigmaClone))
#define LIGMA_CLONE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CLONE, LigmaCloneClass))
#define LIGMA_IS_CLONE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CLONE))
#define LIGMA_IS_CLONE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CLONE))
#define LIGMA_CLONE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CLONE, LigmaCloneClass))


typedef struct _LigmaCloneClass LigmaCloneClass;

struct _LigmaClone
{
  LigmaSourceCore  parent_instance;
};

struct _LigmaCloneClass
{
  LigmaSourceCoreClass  parent_class;
};


void    ligma_clone_register (Ligma                      *ligma,
                             LigmaPaintRegisterCallback  callback);

GType   ligma_clone_get_type (void) G_GNUC_CONST;


#endif  /*  __LIGMA_CLONE_H__  */
