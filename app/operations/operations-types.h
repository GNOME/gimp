/* GIMP - The GNU Image Manipulation Program
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

#pragma once

#include <gegl-types.h>

#include "gegl/gimp-gegl-types.h"

#include "operations-enums.h"


/*  operations  */

typedef struct _GimpOperationPointFilter        GimpOperationPointFilter;
typedef struct _GimpOperationLayerMode          GimpOperationLayerMode;


/*  operation config objects  */

typedef struct _GimpOperationSettings           GimpOperationSettings;

typedef struct _GimpBrightnessContrastConfig    GimpBrightnessContrastConfig;
typedef struct _GimpCageConfig                  GimpCageConfig;
typedef struct _GimpColorBalanceConfig          GimpColorBalanceConfig;
typedef struct _GimpColorizeConfig              GimpColorizeConfig;
typedef struct _GimpCurvesConfig                GimpCurvesConfig;
typedef struct _GimpDesaturateConfig            GimpDesaturateConfig;
typedef struct _GimpHueSaturationConfig         GimpHueSaturationConfig;
typedef struct _GimpLevelsConfig                GimpLevelsConfig;
typedef struct _GimpPosterizeConfig             GimpPosterizeConfig;
typedef struct _GimpThresholdConfig             GimpThresholdConfig;


/*  non-object types  */

typedef struct _GimpCagePoint                   GimpCagePoint;


/*  functions  */

typedef gboolean (* GimpLayerModeFunc)      (GeglOperation          *operation,
                                             void                   *in,
                                             void                   *aux,
                                             void                   *mask,
                                             void                   *out,
                                             glong                   samples,
                                             const GeglRectangle    *roi,
                                             gint                    level);

typedef  void    (* GimpLayerModeBlendFunc) (GeglOperation          *operation,
                                             const gfloat           *in,
                                             const gfloat           *layer,
                                             gfloat                 *out,
                                             gint                    samples);
