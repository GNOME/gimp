/* GIMP - The GNU Image Manipulation Program
 *
 * gimpoperationcagecoefcalc.h
 * Copyright (C) 2010 Michael Muré <batolettre@gmail.com>
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

#pragma once

#include <gegl-plugin.h>
#include <operation/gegl-operation-source.h>


enum
{
  GIMP_OPERATION_CAGE_COEF_CALC_PROP_0,
  GIMP_OPERATION_CAGE_COEF_CALC_PROP_CONFIG
};


#define GIMP_TYPE_OPERATION_CAGE_COEF_CALC            (gimp_operation_cage_coef_calc_get_type ())
#define GIMP_OPERATION_CAGE_COEF_CALC(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPERATION_CAGE_COEF_CALC, GimpOperationCageCoefCalc))
#define GIMP_OPERATION_CAGE_COEF_CALC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_OPERATION_CAGE_COEF_CALC, GimpOperationCageCoefCalcClass))
#define GIMP_IS_OPERATION_CAGE_COEF_CALC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OPERATION_CAGE_COEF_CALC))
#define GIMP_IS_OPERATION_CAGE_COEF_CALC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_OPERATION_CAGE_COEF_CALC))
#define GIMP_OPERATION_CAGE_COEF_CALC_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_OPERATION_CAGE_COEF_CALC, GimpOperationCageCoefCalcClass))


typedef struct _GimpOperationCageCoefCalc      GimpOperationCageCoefCalc;
typedef struct _GimpOperationCageCoefCalcClass GimpOperationCageCoefCalcClass;

struct _GimpOperationCageCoefCalc
{
  GeglOperationSource  parent_instance;

  GimpCageConfig      *config;
};

struct _GimpOperationCageCoefCalcClass
{
  GeglOperationSourceClass  parent_class;
};


GType   gimp_operation_cage_coef_calc_get_type (void) G_GNUC_CONST;
