/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoperationsplit.h
 * Copyright (C) 2008 Michael Natterer <mitch@ligma.org>
 *               2017 Ell
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

#ifndef __LIGMA_OPERATION_SPLIT_H__
#define __LIGMA_OPERATION_SPLIT_H__


#include "ligmaoperationlayermode.h"


#define LIGMA_TYPE_OPERATION_SPLIT            (ligma_operation_split_get_type ())
#define LIGMA_OPERATION_SPLIT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_OPERATION_SPLIT, LigmaOperationSplit))
#define LIGMA_OPERATION_SPLIT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LIGMA_TYPE_OPERATION_SPLIT, LigmaOperationSplitClass))
#define LIGMA_IS_OPERATION_SPLIT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_OPERATION_SPLIT))
#define LIGMA_IS_OPERATION_SPLIT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LIGMA_TYPE_OPERATION_SPLIT))
#define LIGMA_OPERATION_SPLIT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LIGMA_TYPE_OPERATION_SPLIT, LigmaOperationSplitClass))


typedef struct _LigmaOperationSplit      LigmaOperationSplit;
typedef struct _LigmaOperationSplitClass LigmaOperationSplitClass;

struct _LigmaOperationSplit
{
  LigmaOperationLayerMode  parent_instance;
};

struct _LigmaOperationSplitClass
{
  LigmaOperationLayerModeClass  parent_class;
};


GType   ligma_operation_split_get_type (void) G_GNUC_CONST;


#endif /* __LIGMA_OPERATION_SPLIT_H__ */
