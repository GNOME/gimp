/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * operations-types.h
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

#ifndef __OPERATIONS_TYPES_H__
#define __OPERATIONS_TYPES_H__


#include <gegl-types.h>

#include "gegl/ligma-gegl-types.h"

#include "operations-enums.h"


/*  operations  */

typedef struct _LigmaOperationPointFilter        LigmaOperationPointFilter;
typedef struct _LigmaOperationLayerMode          LigmaOperationLayerMode;


/*  operation config objects  */

typedef struct _LigmaOperationSettings           LigmaOperationSettings;

typedef struct _LigmaBrightnessContrastConfig    LigmaBrightnessContrastConfig;
typedef struct _LigmaCageConfig                  LigmaCageConfig;
typedef struct _LigmaColorBalanceConfig          LigmaColorBalanceConfig;
typedef struct _LigmaColorizeConfig              LigmaColorizeConfig;
typedef struct _LigmaCurvesConfig                LigmaCurvesConfig;
typedef struct _LigmaDesaturateConfig            LigmaDesaturateConfig;
typedef struct _LigmaHueSaturationConfig         LigmaHueSaturationConfig;
typedef struct _LigmaLevelsConfig                LigmaLevelsConfig;
typedef struct _LigmaPosterizeConfig             LigmaPosterizeConfig;
typedef struct _LigmaThresholdConfig             LigmaThresholdConfig;


/*  non-object types  */

typedef struct _LigmaCagePoint                   LigmaCagePoint;


/*  functions  */

typedef gboolean (* LigmaLayerModeFunc)      (GeglOperation          *operation,
                                             void                   *in,
                                             void                   *aux,
                                             void                   *mask,
                                             void                   *out,
                                             glong                   samples,
                                             const GeglRectangle    *roi,
                                             gint                    level);

typedef  void    (* LigmaLayerModeBlendFunc) (GeglOperation          *operation,
                                             const gfloat           *in,
                                             const gfloat           *layer,
                                             gfloat                 *out,
                                             gint                    samples);


#endif /* __OPERATIONS_TYPES_H__ */
