/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimplayermodefunctions.c
 * Copyright (C) 2013 Daniel Sabo <DanielSabo@gmail.com>
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

#include "config.h"

#include <gegl.h>

#include "operations-types.h"

#include "gimplayermodefunctions.h"

#include "layer-modes/gimpoperationnormal.h"
#include "layer-modes/gimpoperationdissolve.h"
#include "gimpoperationbehindmode.h"
#include "layer-modes/gimpoperationmultiply.h"
#include "layer-modes-legacy/gimpoperationmultiplylegacy.h"
#include "layer-modes/gimpoperationscreen.h"
#include "layer-modes-legacy/gimpoperationscreenlegacy.h"
#include "layer-modes/gimpoperationoverlay.h"
#include "layer-modes/gimpoperationdifference.h"
#include "layer-modes-legacy/gimpoperationdifferencelegacy.h"
#include "layer-modes/gimpoperationaddition.h"
#include "layer-modes/gimpoperationsubtract.h"
#include "layer-modes-legacy/gimpoperationadditionlegacy.h"
#include "layer-modes-legacy/gimpoperationsubtractlegacy.h"
#include "layer-modes/gimpoperationdarkenonly.h"
#include "layer-modes-legacy/gimpoperationdarkenonlylegacy.h"
#include "layer-modes/gimpoperationlightenonly.h"
#include "layer-modes-legacy/gimpoperationlightenonlylegacy.h"
#include "gimpoperationhuemode.h"
#include "gimpoperationsaturationmode.h"
#include "gimpoperationcolormode.h"
#include "gimpoperationvaluemode.h"
#include "gimpoperationdividemode.h"
#include "layer-modes/gimpoperationdodge.h"
#include "layer-modes-legacy/gimpoperationdodgelegacy.h"
#include "gimpoperationburnmode.h"
#include "gimpoperationhardlightmode.h"
#include "gimpoperationsoftlightmode.h"
#include "gimpoperationgrainextractmode.h"
#include "gimpoperationgrainmergemode.h"
#include "gimpoperationcolorerasemode.h"
#include "layer-modes/gimpoperationlchhue.h"
#include "layer-modes/gimpoperationlchchroma.h"
#include "layer-modes/gimpoperationlchcolor.h"
#include "layer-modes/gimpoperationlchlightness.h"
#include "gimpoperationerasemode.h"
#include "gimpoperationreplacemode.h"
#include "gimpoperationantierasemode.h"


GimpLayerModeFunction
get_layer_mode_function (GimpLayerMode  paint_mode,
                         gboolean       linear_mode)
{
  GimpLayerModeFunction func = gimp_operation_normal_process_pixels;

  switch (paint_mode)
    {
    case GIMP_LAYER_MODE_NORMAL:
      func = gimp_operation_normal_process_pixels;
      break;

    case GIMP_LAYER_MODE_DISSOLVE:
      func = gimp_operation_dissolve_process_pixels;
      break;

    case GIMP_LAYER_MODE_BEHIND:
      func = gimp_operation_behind_mode_process_pixels;
      break;

    case GIMP_LAYER_MODE_MULTIPLY_LEGACY:
      func = gimp_operation_multiply_legacy_process_pixels;
      break;

    case GIMP_LAYER_MODE_MULTIPLY:
      func = gimp_operation_multiply_process_pixels;
      break;

    case GIMP_LAYER_MODE_SCREEN_LEGACY:
      func = gimp_operation_screen_legacy_process_pixels;
      break;

    case GIMP_LAYER_MODE_SCREEN:
      func = gimp_operation_screen_process_pixels;
      break;

    case GIMP_LAYER_MODE_OVERLAY_LEGACY:
      func = gimp_operation_softlight_mode_process_pixels;
      break;

    case GIMP_LAYER_MODE_DIFFERENCE_LEGACY:
      func = gimp_operation_difference_legacy_process_pixels;
      break;

    case GIMP_LAYER_MODE_DIFFERENCE:
      func = gimp_operation_difference_process_pixels;
      break;

    case GIMP_LAYER_MODE_ADDITION:
      func = gimp_operation_addition_process_pixels;
      break;

    case GIMP_LAYER_MODE_ADDITION_LEGACY:
      func = gimp_operation_addition_legacy_process_pixels;
      break;

    case GIMP_LAYER_MODE_SUBTRACT:
      func = gimp_operation_subtract_process_pixels;
      break;

    case GIMP_LAYER_MODE_SUBTRACT_LEGACY:
      func = gimp_operation_subtract_legacy_process_pixels;
      break;

    case GIMP_LAYER_MODE_DARKEN_ONLY:
      func = gimp_operation_darken_only_process_pixels;
      break;

    case GIMP_LAYER_MODE_DARKEN_ONLY_LEGACY:
      func = gimp_operation_darken_only_legacy_process_pixels;
      break;

    case GIMP_LAYER_MODE_LIGHTEN_ONLY:
      func = gimp_operation_lighten_only_process_pixels;
      break;

    case GIMP_LAYER_MODE_LIGHTEN_ONLY_LEGACY:
      func = gimp_operation_lighten_only_legacy_process_pixels;
      break;

    case GIMP_LAYER_MODE_HSV_HUE_LEGACY:
      func = gimp_operation_hue_mode_process_pixels;
      break;

    case GIMP_LAYER_MODE_HSV_SATURATION_LEGACY:
      func = gimp_operation_saturation_mode_process_pixels;
      break;

    case GIMP_LAYER_MODE_HSV_COLOR_LEGACY:
      func = gimp_operation_color_mode_process_pixels;
      break;

    case GIMP_LAYER_MODE_HSV_VALUE_LEGACY:
      func = gimp_operation_value_mode_process_pixels;
      break;

    case GIMP_LAYER_MODE_DIVIDE_LEGACY:
      func = gimp_operation_divide_mode_process_pixels;
      break;

    case GIMP_LAYER_MODE_DODGE:
      func = gimp_operation_dodge_process_pixels;
      break;

    case GIMP_LAYER_MODE_DODGE_LEGACY:
      func = gimp_operation_dodge_legacy_process_pixels;
      break;

    case GIMP_LAYER_MODE_BURN_LEGACY:
      func = gimp_operation_burn_mode_process_pixels;
      break;

    case GIMP_LAYER_MODE_HARDLIGHT_LEGACY:
      func = gimp_operation_hardlight_mode_process_pixels;
      break;

    case GIMP_LAYER_MODE_SOFTLIGHT_LEGACY:
      func = gimp_operation_softlight_mode_process_pixels;
      break;

    case GIMP_LAYER_MODE_GRAIN_EXTRACT_LEGACY:
      func = gimp_operation_grain_extract_mode_process_pixels;
      break;

    case GIMP_LAYER_MODE_GRAIN_MERGE_LEGACY:
      func = gimp_operation_grain_merge_mode_process_pixels;
      break;

    case GIMP_LAYER_MODE_COLOR_ERASE:
      func = gimp_operation_color_erase_mode_process_pixels;
      break;

    case GIMP_LAYER_MODE_OVERLAY:
      func = gimp_operation_overlay_process_pixels;
      break;

    case GIMP_LAYER_MODE_LCH_HUE:
      func = linear_mode ?
             gimp_operation_lch_hue_process_pixels_linear :
             gimp_operation_lch_hue_process_pixels;
      break;

    case GIMP_LAYER_MODE_LCH_CHROMA:
      func = linear_mode ?
             gimp_operation_lch_chroma_process_pixels_linear :
             gimp_operation_lch_chroma_process_pixels;
      break;

    case GIMP_LAYER_MODE_LCH_COLOR:
      func = linear_mode ?
             gimp_operation_lch_color_process_pixels_linear :
             gimp_operation_lch_color_process_pixels;
      break;

    case GIMP_LAYER_MODE_LCH_LIGHTNESS:
      func = linear_mode ?
             gimp_operation_lch_lightness_process_pixels_linear :
             gimp_operation_lch_lightness_process_pixels;
      break;

    case GIMP_LAYER_MODE_ERASE:
      func = gimp_operation_erase_mode_process_pixels;
      break;

    case GIMP_LAYER_MODE_REPLACE:
      func = gimp_operation_replace_mode_process_pixels;
      break;

    case GIMP_LAYER_MODE_ANTI_ERASE:
      func = gimp_operation_anti_erase_mode_process_pixels;
      break;

    default:
      g_warning ("No direct function for layer mode (%d), using gimp:normal-mode", paint_mode);
      func = gimp_operation_normal_process_pixels;
      break;
    }

  return func;
}
