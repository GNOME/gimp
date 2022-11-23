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

#ifndef  __LIGMA_MYBRUSH_CORE_H__
#define  __LIGMA_MYBRUSH_CORE_H__


#include "ligmapaintcore.h"


#define LIGMA_TYPE_MYBRUSH_CORE            (ligma_mybrush_core_get_type ())
#define LIGMA_MYBRUSH_CORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_MYBRUSH_CORE, LigmaMybrushCore))
#define LIGMA_MYBRUSH_CORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_MYBRUSH_CORE, LigmaMybrushCoreClass))
#define LIGMA_IS_MYBRUSH_CORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_MYBRUSH_CORE))
#define LIGMA_IS_MYBRUSH_CORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_MYBRUSH_CORE))
#define LIGMA_MYBRUSH_CORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_MYBRUSH_CORE, LigmaMybrushCoreClass))


typedef struct _LigmaMybrushCorePrivate LigmaMybrushCorePrivate;
typedef struct _LigmaMybrushCoreClass   LigmaMybrushCoreClass;

struct _LigmaMybrushCore
{
  LigmaPaintCore           parent_instance;

  LigmaMybrushCorePrivate *private;
};

struct _LigmaMybrushCoreClass
{
  LigmaPaintCoreClass  parent_class;
};


void    ligma_mybrush_core_register (Ligma                      *ligma,
                                    LigmaPaintRegisterCallback  callback);

GType   ligma_mybrush_core_get_type (void) G_GNUC_CONST;


#endif  /*  __LIGMA_MYBRUSH_CORE_H__  */
