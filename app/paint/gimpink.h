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

#ifndef  __LIGMA_INK_H__
#define  __LIGMA_INK_H__


#include "ligmapaintcore.h"
#include "ligmaink-blob.h"


#define LIGMA_TYPE_INK            (ligma_ink_get_type ())
#define LIGMA_INK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_INK, LigmaInk))
#define LIGMA_INK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_INK, LigmaInkClass))
#define LIGMA_IS_INK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_INK))
#define LIGMA_IS_INK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_INK))
#define LIGMA_INK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_INK, LigmaInkClass))


typedef struct _LigmaInkClass LigmaInkClass;

struct _LigmaInk
{
  LigmaPaintCore  parent_instance;

  GList         *start_blobs;  /*  starting blobs per stroke (for undo) */

  LigmaBlob      *cur_blob;     /*  current blob                         */
  GList         *last_blobs;   /*  blobs for last stroke positions      */
};

struct _LigmaInkClass
{
  LigmaPaintCoreClass  parent_class;
};


void    ligma_ink_register (Ligma                      *ligma,
                           LigmaPaintRegisterCallback  callback);

GType   ligma_ink_get_type (void) G_GNUC_CONST;


#endif  /*  __LIGMA_INK_H__  */
