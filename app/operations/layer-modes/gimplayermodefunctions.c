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

#include "../operations-types.h"

#include "gimplayermodefunctions.h"

#include "operations/layer-modes-legacy/gimpoperationadditionlegacy.h"
#include "operations/layer-modes-legacy/gimpoperationburnlegacy.h"
#include "operations/layer-modes-legacy/gimpoperationdarkenonlylegacy.h"
#include "operations/layer-modes-legacy/gimpoperationdifferencelegacy.h"
#include "operations/layer-modes-legacy/gimpoperationdividelegacy.h"
#include "operations/layer-modes-legacy/gimpoperationdodgelegacy.h"
#include "operations/layer-modes-legacy/gimpoperationgrainextractlegacy.h"
#include "operations/layer-modes-legacy/gimpoperationgrainmergelegacy.h"
#include "operations/layer-modes-legacy/gimpoperationhardlightlegacy.h"
#include "operations/layer-modes-legacy/gimpoperationhsvcolorlegacy.h"
#include "operations/layer-modes-legacy/gimpoperationhsvhuelegacy.h"
#include "operations/layer-modes-legacy/gimpoperationhsvsaturationlegacy.h"
#include "operations/layer-modes-legacy/gimpoperationhsvvaluelegacy.h"
#include "operations/layer-modes-legacy/gimpoperationlightenonlylegacy.h"
#include "operations/layer-modes-legacy/gimpoperationmultiplylegacy.h"
#include "operations/layer-modes-legacy/gimpoperationscreenlegacy.h"
#include "operations/layer-modes-legacy/gimpoperationsoftlightlegacy.h"
#include "operations/layer-modes-legacy/gimpoperationsubtractlegacy.h"

#include "gimpoperationaddition.h"
#include "gimpoperationantierase.h"
#include "gimpoperationbehind.h"
#include "gimpoperationburn.h"
#include "gimpoperationcolorerase.h"
#include "gimpoperationdarkenonly.h"
#include "gimpoperationdifference.h"
#include "gimpoperationdissolve.h"
#include "gimpoperationdivide.h"
#include "gimpoperationdodge.h"
#include "gimpoperationerase.h"
#include "gimpoperationgrainextract.h"
#include "gimpoperationgrainmerge.h"
#include "gimpoperationhardlight.h"
#include "gimpoperationhsvcolor.h"
#include "gimpoperationhsvhue.h"
#include "gimpoperationhsvsaturation.h"
#include "gimpoperationhsvvalue.h"
#include "gimpoperationlchchroma.h"
#include "gimpoperationlchcolor.h"
#include "gimpoperationlchhue.h"
#include "gimpoperationlchlightness.h"
#include "gimpoperationlightenonly.h"
#include "gimpoperationmultiply.h"
#include "gimpoperationnormal.h"
#include "gimpoperationoverlay.h"
#include "gimpoperationreplace.h"
#include "gimpoperationscreen.h"
#include "gimpoperationsoftlight.h"
#include "gimpoperationsubtract.h"


GimpLayerModeFunction
get_layer_mode_function (GimpLayerMode  paint_mode,
                         gboolean       linear_mode)
{
  GimpLayerModeFunction func;

  switch (paint_mode)
    {
    case GIMP_LAYER_MODE_NORMAL:
    case GIMP_LAYER_MODE_NORMAL_NON_LINEAR:
      func = gimp_operation_normal_process_pixels;
      break;

    case GIMP_LAYER_MODE_DISSOLVE:
      func = gimp_operation_dissolve_process_pixels;
      break;

    case GIMP_LAYER_MODE_BEHIND:
    case GIMP_LAYER_MODE_BEHIND_LINEAR:
      func = gimp_operation_behind_process_pixels;
      break;

    case GIMP_LAYER_MODE_MULTIPLY_LEGACY:
      func = gimp_operation_multiply_legacy_process_pixels;
      break;

    case GIMP_LAYER_MODE_MULTIPLY:
    case GIMP_LAYER_MODE_MULTIPLY_LINEAR:
      func = gimp_operation_multiply_process_pixels;
      break;

    case GIMP_LAYER_MODE_SCREEN_LEGACY:
      func = gimp_operation_screen_legacy_process_pixels;
      break;

    case GIMP_LAYER_MODE_SCREEN:
      func = gimp_operation_screen_process_pixels;
      break;

    case GIMP_LAYER_MODE_OVERLAY_LEGACY:
      func = gimp_operation_softlight_legacy_process_pixels;
      break;

    case GIMP_LAYER_MODE_DIFFERENCE_LEGACY:
      func = gimp_operation_difference_legacy_process_pixels;
      break;

    case GIMP_LAYER_MODE_DIFFERENCE:
    case GIMP_LAYER_MODE_DIFFERENCE_LINEAR:
      func = gimp_operation_difference_process_pixels;
      break;

    case GIMP_LAYER_MODE_ADDITION_LEGACY:
      func = gimp_operation_addition_legacy_process_pixels;
      break;

    case GIMP_LAYER_MODE_ADDITION:
    case GIMP_LAYER_MODE_ADDITION_LINEAR:
      func = gimp_operation_addition_process_pixels;
      break;

    case GIMP_LAYER_MODE_SUBTRACT_LEGACY:
      func = gimp_operation_subtract_legacy_process_pixels;
      break;

    case GIMP_LAYER_MODE_SUBTRACT:
    case GIMP_LAYER_MODE_SUBTRACT_LINEAR:
      func = gimp_operation_subtract_process_pixels;
      break;

    case GIMP_LAYER_MODE_DARKEN_ONLY_LEGACY:
      func = gimp_operation_darken_only_legacy_process_pixels;
      break;

    case GIMP_LAYER_MODE_DARKEN_ONLY:
    case GIMP_LAYER_MODE_DARKEN_ONLY_LINEAR:
      func = gimp_operation_darken_only_process_pixels;
      break;

    case GIMP_LAYER_MODE_LIGHTEN_ONLY_LEGACY:
      func = gimp_operation_lighten_only_legacy_process_pixels;
      break;

    case GIMP_LAYER_MODE_LIGHTEN_ONLY:
    case GIMP_LAYER_MODE_LIGHTEN_ONLY_LINEAR:
      func = gimp_operation_lighten_only_process_pixels;
      break;

    case GIMP_LAYER_MODE_HSV_HUE:
      func = gimp_operation_hsv_hue_process_pixels;
      break;

    case GIMP_LAYER_MODE_HSV_HUE_LEGACY:
      func = gimp_operation_hsv_hue_legacy_process_pixels;
      break;

    case GIMP_LAYER_MODE_HSV_SATURATION:
      func = gimp_operation_hsv_saturation_process_pixels;
      break;

    case GIMP_LAYER_MODE_HSV_SATURATION_LEGACY:
      func = gimp_operation_hsv_saturation_legacy_process_pixels;
      break;

    case GIMP_LAYER_MODE_HSV_COLOR:
      func = gimp_operation_hsv_color_process_pixels;
      break;

    case GIMP_LAYER_MODE_HSV_COLOR_LEGACY:
      func = gimp_operation_hsv_color_legacy_process_pixels;
      break;

    case GIMP_LAYER_MODE_HSV_VALUE:
      func = gimp_operation_hsv_value_process_pixels;
      break;

    case GIMP_LAYER_MODE_HSV_VALUE_LEGACY:
      func = gimp_operation_hsv_value_legacy_process_pixels;
      break;

    case GIMP_LAYER_MODE_DIVIDE_LEGACY:
      func = gimp_operation_divide_legacy_process_pixels;
      break;

    case GIMP_LAYER_MODE_DIVIDE:
    case GIMP_LAYER_MODE_DIVIDE_LINEAR:
      func = gimp_operation_divide_process_pixels;
      break;

    case GIMP_LAYER_MODE_DODGE_LEGACY:
      func = gimp_operation_dodge_legacy_process_pixels;
      break;

    case GIMP_LAYER_MODE_DODGE:
    case GIMP_LAYER_MODE_DODGE_LINEAR:
      func = gimp_operation_dodge_process_pixels;
      break;

    case GIMP_LAYER_MODE_BURN_LEGACY:
      func = gimp_operation_burn_legacy_process_pixels;
      break;

    case GIMP_LAYER_MODE_BURN:
    case GIMP_LAYER_MODE_BURN_LINEAR:
      func = gimp_operation_burn_process_pixels;
      break;

    case GIMP_LAYER_MODE_HARDLIGHT_LEGACY:
      func = gimp_operation_hardlight_legacy_process_pixels;
      break;

    case GIMP_LAYER_MODE_HARDLIGHT:
      func = gimp_operation_hardlight_process_pixels;
      break;

    case GIMP_LAYER_MODE_SOFTLIGHT_LEGACY:
      func = gimp_operation_softlight_legacy_process_pixels;
      break;

    case GIMP_LAYER_MODE_SOFTLIGHT:
      func = gimp_operation_softlight_process_pixels;
      break;

    case GIMP_LAYER_MODE_GRAIN_EXTRACT_LEGACY:
      func = gimp_operation_grain_extract_legacy_process_pixels;
      break;

    case GIMP_LAYER_MODE_GRAIN_EXTRACT:
    case GIMP_LAYER_MODE_GRAIN_EXTRACT_LINEAR:
      func = gimp_operation_grain_extract_process_pixels;
      break;

    case GIMP_LAYER_MODE_GRAIN_MERGE_LEGACY:
      func = gimp_operation_grain_merge_legacy_process_pixels;
      break;

    case GIMP_LAYER_MODE_GRAIN_MERGE:
    case GIMP_LAYER_MODE_GRAIN_MERGE_LINEAR:
      func = gimp_operation_grain_merge_process_pixels;
      break;

    case GIMP_LAYER_MODE_COLOR_ERASE:
      func = gimp_operation_color_erase_process_pixels;
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
      func = gimp_operation_erase_process_pixels;
      break;

    case GIMP_LAYER_MODE_REPLACE:
      func = gimp_operation_replace_process_pixels;
      break;

    case GIMP_LAYER_MODE_ANTI_ERASE:
      func = gimp_operation_anti_erase_process_pixels;
      break;

    default:
      g_warning ("No direct function for layer mode (%d), using gimp:normal",
                 paint_mode);
      func = gimp_operation_normal_process_pixels;
      break;
    }

  return func;
}
