/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gegl-types.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_GEGL_TYPES_H__
#define __GIMP_GEGL_TYPES_H__


#include "core/core-types.h"


/*  operations  */

typedef struct _GimpOperationColorBalance    GimpOperationColorBalance;
typedef struct _GimpOperationColorize        GimpOperationColorize;
typedef struct _GimpOperationCurves          GimpOperationCurves;
typedef struct _GimpOperationDesaturate      GimpOperationDesaturate;
typedef struct _GimpOperationHueSaturation   GimpOperationHueSaturation;
typedef struct _GimpOperationLevels          GimpOperationLevels;
typedef struct _GimpOperationPointFilter     GimpOperationPointFilter;
typedef struct _GimpOperationPosterize       GimpOperationPosterize;
typedef struct _GimpOperationThreshold       GimpOperationThreshold;
typedef struct _GimpOperationTileSink        GimpOperationTileSink;
typedef struct _GimpOperationTileSource      GimpOperationTileSource;


/*  operation config objects  */

typedef struct _GimpBrightnessContrastConfig GimpBrightnessContrastConfig;
typedef struct _GimpColorBalanceConfig       GimpColorBalanceConfig;
typedef struct _GimpColorizeConfig           GimpColorizeConfig;
typedef struct _GimpCurvesConfig             GimpCurvesConfig;
typedef struct _GimpDesaturateConfig         GimpDesaturateConfig;
typedef struct _GimpHueSaturationConfig      GimpHueSaturationConfig;
typedef struct _GimpLevelsConfig             GimpLevelsConfig;
typedef struct _GimpPosterizeConfig          GimpPosterizeConfig;
typedef struct _GimpThresholdConfig          GimpThresholdConfig;


#endif /* __GIMP_GEGL_TYPES_H__ */
