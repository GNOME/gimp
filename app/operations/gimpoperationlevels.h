/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoperationlevels.h
 * Copyright (C) 2007 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_OPERATION_LEVELS_H__
#define __LIGMA_OPERATION_LEVELS_H__


#include "ligmaoperationpointfilter.h"


#define LIGMA_TYPE_OPERATION_LEVELS            (ligma_operation_levels_get_type ())
#define LIGMA_OPERATION_LEVELS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_OPERATION_LEVELS, LigmaOperationLevels))
#define LIGMA_OPERATION_LEVELS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LIGMA_TYPE_OPERATION_LEVELS, LigmaOperationLevelsClass))
#define LIGMA_IS_OPERATION_LEVELS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_OPERATION_LEVELS))
#define LIGMA_IS_OPERATION_LEVELS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LIGMA_TYPE_OPERATION_LEVELS))
#define LIGMA_OPERATION_LEVELS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LIGMA_TYPE_OPERATION_LEVELS, LigmaOperationLevelsClass))


typedef struct _LigmaOperationLevels      LigmaOperationLevels;
typedef struct _LigmaOperationLevelsClass LigmaOperationLevelsClass;

struct _LigmaOperationLevels
{
  LigmaOperationPointFilter  parent_instance;
};

struct _LigmaOperationLevelsClass
{
  LigmaOperationPointFilterClass  parent_class;
};


GType     ligma_operation_levels_get_type  (void) G_GNUC_CONST;

gdouble   ligma_operation_levels_map_input (LigmaLevelsConfig     *config,
                                           LigmaHistogramChannel  channel,
                                           gdouble               value);


#endif /* __LIGMA_OPERATION_LEVELS_H__ */
