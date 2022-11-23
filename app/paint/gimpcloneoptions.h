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

#ifndef __LIGMA_CLONE_OPTIONS_H__
#define __LIGMA_CLONE_OPTIONS_H__


#include "ligmasourceoptions.h"


#define LIGMA_TYPE_CLONE_OPTIONS            (ligma_clone_options_get_type ())
#define LIGMA_CLONE_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CLONE_OPTIONS, LigmaCloneOptions))
#define LIGMA_CLONE_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CLONE_OPTIONS, LigmaCloneOptionsClass))
#define LIGMA_IS_CLONE_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CLONE_OPTIONS))
#define LIGMA_IS_CLONE_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CLONE_OPTIONS))
#define LIGMA_CLONE_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CLONE_OPTIONS, LigmaCloneOptionsClass))


typedef struct _LigmaCloneOptionsClass LigmaCloneOptionsClass;

struct _LigmaCloneOptions
{
  LigmaSourceOptions  parent_instance;

  LigmaCloneType      clone_type;
};

struct _LigmaCloneOptionsClass
{
  LigmaSourceOptionsClass  parent_class;
};


GType   ligma_clone_options_get_type (void) G_GNUC_CONST;


#endif  /*  __LIGMA_CLONE_OPTIONS_H__  */
