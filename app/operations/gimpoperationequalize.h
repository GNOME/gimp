/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoperationequalize.h
 * Copyright (C) 2012 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_OPERATION_EQUALIZE_H__
#define __LIGMA_OPERATION_EQUALIZE_H__


#include "ligmaoperationpointfilter.h"


#define LIGMA_TYPE_OPERATION_EQUALIZE            (ligma_operation_equalize_get_type ())
#define LIGMA_OPERATION_EQUALIZE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_OPERATION_EQUALIZE, LigmaOperationEqualize))
#define LIGMA_OPERATION_EQUALIZE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LIGMA_TYPE_OPERATION_EQUALIZE, LigmaOperationEqualizeClass))
#define LIGMA_IS_OPERATION_EQUALIZE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_OPERATION_EQUALIZE))
#define LIGMA_IS_OPERATION_EQUALIZE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LIGMA_TYPE_OPERATION_EQUALIZE))
#define LIGMA_OPERATION_EQUALIZE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LIGMA_TYPE_OPERATION_EQUALIZE, LigmaOperationEqualizeClass))


typedef struct _LigmaOperationEqualize      LigmaOperationEqualize;
typedef struct _LigmaOperationEqualizeClass LigmaOperationEqualizeClass;

struct _LigmaOperationEqualize
{
  LigmaOperationPointFilter  parent_instance;

  LigmaHistogram            *histogram;
  gdouble                  *values;
  gint                      n_bins;
};

struct _LigmaOperationEqualizeClass
{
  LigmaOperationPointFilterClass  parent_class;
};


GType   ligma_operation_equalize_get_type (void) G_GNUC_CONST;


#endif /* __LIGMA_OPERATION_EQUALIZE_H__ */
