/* LIGMA - The GNU Image Manipulation Program
 *
 * ligmaoperationcagecoefcalc.h
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

#ifndef __LIGMA_OPERATION_CAGE_COEF_CALC_H__
#define __LIGMA_OPERATION_CAGE_COEF_CALC_H__


#include <gegl-plugin.h>
#include <operation/gegl-operation-source.h>


enum
{
  LIGMA_OPERATION_CAGE_COEF_CALC_PROP_0,
  LIGMA_OPERATION_CAGE_COEF_CALC_PROP_CONFIG
};


#define LIGMA_TYPE_OPERATION_CAGE_COEF_CALC            (ligma_operation_cage_coef_calc_get_type ())
#define LIGMA_OPERATION_CAGE_COEF_CALC(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_OPERATION_CAGE_COEF_CALC, LigmaOperationCageCoefCalc))
#define LIGMA_OPERATION_CAGE_COEF_CALC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LIGMA_TYPE_OPERATION_CAGE_COEF_CALC, LigmaOperationCageCoefCalcClass))
#define LIGMA_IS_OPERATION_CAGE_COEF_CALC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_OPERATION_CAGE_COEF_CALC))
#define LIGMA_IS_OPERATION_CAGE_COEF_CALC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LIGMA_TYPE_OPERATION_CAGE_COEF_CALC))
#define LIGMA_OPERATION_CAGE_COEF_CALC_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LIGMA_TYPE_OPERATION_CAGE_COEF_CALC, LigmaOperationCageCoefCalcClass))


typedef struct _LigmaOperationCageCoefCalc      LigmaOperationCageCoefCalc;
typedef struct _LigmaOperationCageCoefCalcClass LigmaOperationCageCoefCalcClass;

struct _LigmaOperationCageCoefCalc
{
  GeglOperationSource  parent_instance;

  LigmaCageConfig      *config;
};

struct _LigmaOperationCageCoefCalcClass
{
  GeglOperationSourceClass  parent_class;
};


GType   ligma_operation_cage_coef_calc_get_type (void) G_GNUC_CONST;


#endif /* __LIGMA_OPERATION_CAGE_COEF_CALC_H__ */
