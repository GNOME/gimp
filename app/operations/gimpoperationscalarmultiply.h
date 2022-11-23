/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoperationscalarmultiply.h
 * Copyright (C) 2014 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_OPERATION_SCALAR_MULTIPLY_H__
#define __LIGMA_OPERATION_SCALAR_MULTIPLY_H__


#include <gegl-plugin.h>


#define LIGMA_TYPE_OPERATION_SCALAR_MULTIPLY            (ligma_operation_scalar_multiply_get_type ())
#define LIGMA_OPERATION_SCALAR_MULTIPLY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_OPERATION_SCALAR_MULTIPLY, LigmaOperationScalarMultiply))
#define LIGMA_OPERATION_SCALAR_MULTIPLY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LIGMA_TYPE_OPERATION_SCALAR_MULTIPLY, LigmaOperationScalarMultiplyClass))
#define LIGMA_IS_OPERATION_SCALAR_MULTIPLY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_OPERATION_SCALAR_MULTIPLY))
#define LIGMA_IS_OPERATION_SCALAR_MULTIPLY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LIGMA_TYPE_OPERATION_SCALAR_MULTIPLY))
#define LIGMA_OPERATION_SCALAR_MULTIPLY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LIGMA_TYPE_OPERATION_SCALAR_MULTIPLY, LigmaOperationScalarMultiplyClass))


typedef struct _LigmaOperationScalarMultiply      LigmaOperationScalarMultiply;
typedef struct _LigmaOperationScalarMultiplyClass LigmaOperationScalarMultiplyClass;

struct _LigmaOperationScalarMultiply
{
  GeglOperationPointFilter  parent_instance;

  gint                      n_components;
  gdouble                   factor;
};

struct _LigmaOperationScalarMultiplyClass
{
  GeglOperationPointFilterClass  parent_class;
};


GType   ligma_operation_scalar_multiply_get_type (void) G_GNUC_CONST;


#endif /* __LIGMA_OPERATION_SCALAR_MULTIPLY_H__ */
