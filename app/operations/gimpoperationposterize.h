/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoperationposterize.h
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

#ifndef __LIGMA_OPERATION_POSTERIZE_H__
#define __LIGMA_OPERATION_POSTERIZE_H__


#include "ligmaoperationpointfilter.h"


#define LIGMA_TYPE_OPERATION_POSTERIZE            (ligma_operation_posterize_get_type ())
#define LIGMA_OPERATION_POSTERIZE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_OPERATION_POSTERIZE, LigmaOperationPosterize))
#define LIGMA_OPERATION_POSTERIZE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LIGMA_TYPE_OPERATION_POSTERIZE, LigmaOperationPosterizeClass))
#define LIGMA_IS_OPERATION_POSTERIZE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_OPERATION_POSTERIZE))
#define LIGMA_IS_OPERATION_POSTERIZE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LIGMA_TYPE_OPERATION_POSTERIZE))
#define LIGMA_OPERATION_POSTERIZE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LIGMA_TYPE_OPERATION_POSTERIZE, LigmaOperationPosterizeClass))


typedef struct _LigmaOperationPosterize      LigmaOperationPosterize;
typedef struct _LigmaOperationPosterizeClass LigmaOperationPosterizeClass;

struct _LigmaOperationPosterize
{
  LigmaOperationPointFilter  parent_instance;

  gint                      levels;
};

struct _LigmaOperationPosterizeClass
{
  LigmaOperationPointFilterClass  parent_class;
};


GType   ligma_operation_posterize_get_type (void) G_GNUC_CONST;


#endif /* __LIGMA_OPERATION_POSTERIZE_H__ */
