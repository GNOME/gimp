/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoperationgrow.h
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

#ifndef __LIGMA_OPERATION_GROW_H__
#define __LIGMA_OPERATION_GROW_H__


#include <gegl-plugin.h>


#define LIGMA_TYPE_OPERATION_GROW            (ligma_operation_grow_get_type ())
#define LIGMA_OPERATION_GROW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_OPERATION_GROW, LigmaOperationGrow))
#define LIGMA_OPERATION_GROW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LIGMA_TYPE_OPERATION_GROW, LigmaOperationGrowClass))
#define LIGMA_IS_OPERATION_GROW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_OPERATION_GROW))
#define LIGMA_IS_OPERATION_GROW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LIGMA_TYPE_OPERATION_GROW))
#define LIGMA_OPERATION_GROW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LIGMA_TYPE_OPERATION_GROW, LigmaOperationGrowClass))


typedef struct _LigmaOperationGrow      LigmaOperationGrow;
typedef struct _LigmaOperationGrowClass LigmaOperationGrowClass;

struct _LigmaOperationGrow
{
  GeglOperationFilter  parent_instance;

  gint                 radius_x;
  gint                 radius_y;
};

struct _LigmaOperationGrowClass
{
  GeglOperationFilterClass  parent_class;
};


GType   ligma_operation_grow_get_type (void) G_GNUC_CONST;


#endif /* __LIGMA_OPERATION_GROW_H__ */
