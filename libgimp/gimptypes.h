/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimptypes.h
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_TYPES_H__
#define __GIMP_TYPES_H__

#include <libgimpbase/gimpbasetypes.h>

G_BEGIN_DECLS

/* For information look into the html documentation */


typedef struct _GimpPlugInInfo  GimpPlugInInfo;
typedef struct _GimpTile        GimpTile;
typedef struct _GimpDrawable    GimpDrawable;
typedef struct _GimpPixelRgn    GimpPixelRgn;
typedef struct _GimpParamDef    GimpParamDef;
typedef struct _GimpParamRegion GimpParamRegion;
typedef union  _GimpParamData   GimpParamData;
typedef struct _GimpParam       GimpParam;


#ifndef GIMP_DISABLE_DEPRECATED

/*  This is so ugly it hurts. C++ won't let us have enum GimpLayerMode
 *  in the API where we used to have enum GimpLayerModeEffects, so
 *  typedef and define around to make it happy:
 */
typedef GimpLayerMode GimpLayerModeEffects;

#define GIMP_NORMAL_MODE        GIMP_LAYER_MODE_NORMAL_LEGACY
#define GIMP_DISSOLVE_MODE      GIMP_LAYER_MODE_DISSOLVE
#define GIMP_BEHIND_MODE        GIMP_LAYER_MODE_BEHIND_LEGACY
#define GIMP_MULTIPLY_MODE      GIMP_LAYER_MODE_MULTIPLY_LEGACY
#define GIMP_SCREEN_MODE        GIMP_LAYER_MODE_SCREEN_LEGACY
#define GIMP_OVERLAY_MODE       GIMP_LAYER_MODE_OVERLAY_LEGACY
#define GIMP_DIFFERENCE_MODE    GIMP_LAYER_MODE_DIFFERENCE_LEGACY
#define GIMP_ADDITION_MODE      GIMP_LAYER_MODE_ADDITION_LEGACY
#define GIMP_SUBTRACT_MODE      GIMP_LAYER_MODE_SUBTRACT_LEGACY
#define GIMP_DARKEN_ONLY_MODE   GIMP_LAYER_MODE_DARKEN_ONLY_LEGACY
#define GIMP_LIGHTEN_ONLY_MODE  GIMP_LAYER_MODE_LIGHTEN_ONLY_LEGACY
#define GIMP_HUE_MODE           GIMP_LAYER_MODE_HSV_HUE_LEGACY
#define GIMP_SATURATION_MODE    GIMP_LAYER_MODE_HSV_SATURATION_LEGACY
#define GIMP_COLOR_MODE         GIMP_LAYER_MODE_HSL_COLOR_LEGACY
#define GIMP_VALUE_MODE         GIMP_LAYER_MODE_HSV_VALUE_LEGACY
#define GIMP_DIVIDE_MODE        GIMP_LAYER_MODE_DIVIDE_LEGACY
#define GIMP_DODGE_MODE         GIMP_LAYER_MODE_DODGE_LEGACY
#define GIMP_BURN_MODE          GIMP_LAYER_MODE_BURN_LEGACY
#define GIMP_HARDLIGHT_MODE     GIMP_LAYER_MODE_HARDLIGHT_LEGACY
#define GIMP_SOFTLIGHT_MODE     GIMP_LAYER_MODE_SOFTLIGHT_LEGACY
#define GIMP_GRAIN_EXTRACT_MODE GIMP_LAYER_MODE_GRAIN_EXTRACT_LEGACY
#define GIMP_GRAIN_MERGE_MODE   GIMP_LAYER_MODE_GRAIN_MERGE_LEGACY
#define GIMP_COLOR_ERASE_MODE   GIMP_LAYER_MODE_COLOR_ERASE_LEGACY

#define GIMP_NO_DITHER         GIMP_CONVERT_DITHER_NONE
#define GIMP_FS_DITHER         GIMP_CONVERT_DITHER_FS
#define GIMP_FSLOWBLEED_DITHER GIMP_CONVERT_DITHER_FS_LOWBLEED
#define GIMP_FIXED_DITHER      GIMP_CONVERT_DITHER_FIXED

#endif /* ! GIMP_DISABLE_DEPRECATED */

G_END_DECLS

#endif /* __GIMP_TYPES_H__ */
