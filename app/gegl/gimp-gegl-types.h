/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-gegl-types.h
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_GEGL_TYPES_H__
#define __GIMP_GEGL_TYPES_H__

#include "core/core-types.h"

#include "gegl/gimp-gegl-enums.h"


/*  operations  */

typedef struct _GimpOperationTileSink           GimpOperationTileSink;
typedef struct _GimpOperationTileSource         GimpOperationTileSource;

typedef struct _GimpOperationCageCoefCalc       GimpOperationCageCoefCalc;
typedef struct _GimpOperationCageTransform      GimpOperationCageTransform;

typedef struct _GimpOperationPointFilter        GimpOperationPointFilter;
typedef struct _GimpOperationBrightnessContrast GimpOperationBrightnessContrast;
typedef struct _GimpOperationColorBalance       GimpOperationColorBalance;
typedef struct _GimpOperationColorize           GimpOperationColorize;
typedef struct _GimpOperationCurves             GimpOperationCurves;
typedef struct _GimpOperationDesaturate         GimpOperationDesaturate;
typedef struct _GimpOperationHueSaturation      GimpOperationHueSaturation;
typedef struct _GimpOperationLevels             GimpOperationLevels;
typedef struct _GimpOperationPosterize          GimpOperationPosterize;
typedef struct _GimpOperationThreshold          GimpOperationThreshold;

typedef struct _GimpOperationPointLayerMode     GimpOperationPointLayerMode;
typedef struct _GimpOperationDissolveMode       GimpOperationDissolveMode;
typedef struct _GimpOperationBehindMode         GimpOperationBehindMode;
typedef struct _GimpOperationMultiplyMode       GimpOperationMultiplyMode;
typedef struct _GimpOperationScreenMode         GimpOperationScreenMode;
typedef struct _GimpOperationOverlayMode        GimpOperationOverlayMode;
typedef struct _GimpOperationDifferenceMode     GimpOperationDifferenceMode;
typedef struct _GimpOperationAdditionMode       GimpOperationAdditionMode;
typedef struct _GimpOperationSubtractMode       GimpOperationSubtractMode;
typedef struct _GimpOperationDarkenOnlyMode     GimpOperationDarkenOnlyMode;
typedef struct _GimpOperationLightenOnlyMode    GimpOperationLightenOnlyMode;
typedef struct _GimpOperationHueMode            GimpOperationHueMode;
typedef struct _GimpOperationSaturationMode     GimpOperationSaturationMode;
typedef struct _GimpOperationColorMode          GimpOperationColorMode;
typedef struct _GimpOperationValueMode          GimpOperationValueMode;
typedef struct _GimpOperationDivideMode         GimpOperationDivideMode;
typedef struct _GimpOperationDodgeMode          GimpOperationDodgeMode;
typedef struct _GimpOperationBurnMode           GimpOperationBurnMode;
typedef struct _GimpOperationHardlightMode      GimpOperationHardlightMode;
typedef struct _GimpOperationSoftlightMode      GimpOperationSoftlightMode;
typedef struct _GimpOperationGrainExtractMode   GimpOperationGrainExtractMode;
typedef struct _GimpOperationGrainMergeMode     GimpOperationGrainMergeMode;
typedef struct _GimpOperationColorEraseMode     GimpOperationColorEraseMode;
typedef struct _GimpOperationEraseMode          GimpOperationEraseMode;
typedef struct _GimpOperationReplaceMode        GimpOperationReplaceMode;
typedef struct _GimpOperationAntiEraseMode      GimpOperationAntiEraseMode;


/*  operation config objects  */

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


/*  temporary stuff  */

typedef struct _GimpTileBackendTileManager      GimpTileBackendTileManager;


/*  non-object types  */

typedef struct _GimpCagePoint                 GimpCagePoint;


#endif /* __GIMP_GEGL_TYPES_H__ */
