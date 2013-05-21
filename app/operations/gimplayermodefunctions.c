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
#include <gegl-plugin.h>
#include "operations-types.h"

#include "gimplayermodefunctions.h"

#include "gimpoperationpointlayermode.h"
#include "gimpoperationnormalmode.h"
#include "gimpoperationdissolvemode.h"
#include "gimpoperationbehindmode.h"
#include "gimpoperationmultiplymode.h"
#include "gimpoperationscreenmode.h"
#include "gimpoperationoverlaymode.h"
#include "gimpoperationdifferencemode.h"
#include "gimpoperationadditionmode.h"
#include "gimpoperationsubtractmode.h"
#include "gimpoperationdarkenonlymode.h"
#include "gimpoperationlightenonlymode.h"
#include "gimpoperationhuemode.h"
#include "gimpoperationsaturationmode.h"
#include "gimpoperationcolormode.h"
#include "gimpoperationvaluemode.h"
#include "gimpoperationdividemode.h"
#include "gimpoperationdodgemode.h"
#include "gimpoperationburnmode.h"
#include "gimpoperationhardlightmode.h"
#include "gimpoperationsoftlightmode.h"
#include "gimpoperationgrainextractmode.h"
#include "gimpoperationgrainmergemode.h"
#include "gimpoperationcolorerasemode.h"
#include "gimpoperationerasemode.h"
#include "gimpoperationreplacemode.h"
#include "gimpoperationantierasemode.h"

GimpLayerModeFunction
get_layer_mode_function (GimpLayerModeEffects paint_mode)
{
  GimpLayerModeFunction func = gimp_operation_normal_mode_process_pixels;

  switch (paint_mode)
    {
      case GIMP_NORMAL_MODE:        func = gimp_operation_normal_mode_process_pixels; break;
      case GIMP_DISSOLVE_MODE:      func = gimp_operation_dissolve_mode_process_pixels; break;
      case GIMP_BEHIND_MODE:        func = gimp_operation_behind_mode_process_pixels; break;
      case GIMP_MULTIPLY_MODE:      func = gimp_operation_multiply_mode_process_pixels; break;
      case GIMP_SCREEN_MODE:        func = gimp_operation_screen_mode_process_pixels; break;
      case GIMP_OVERLAY_MODE:       func = gimp_operation_overlay_mode_process_pixels; break;
      case GIMP_DIFFERENCE_MODE:    func = gimp_operation_difference_mode_process_pixels; break;
      case GIMP_ADDITION_MODE:      func = gimp_operation_addition_mode_process_pixels; break;
      case GIMP_SUBTRACT_MODE:      func = gimp_operation_subtract_mode_process_pixels; break;
      case GIMP_DARKEN_ONLY_MODE:   func = gimp_operation_darken_only_mode_process_pixels; break;
      case GIMP_LIGHTEN_ONLY_MODE:  func = gimp_operation_lighten_only_mode_process_pixels; break;
      case GIMP_HUE_MODE:           func = gimp_operation_hue_mode_process_pixels; break;
      case GIMP_SATURATION_MODE:    func = gimp_operation_saturation_mode_process_pixels; break;
      case GIMP_COLOR_MODE:         func = gimp_operation_color_mode_process_pixels; break;
      case GIMP_VALUE_MODE:         func = gimp_operation_value_mode_process_pixels; break;
      case GIMP_DIVIDE_MODE:        func = gimp_operation_divide_mode_process_pixels; break;
      case GIMP_DODGE_MODE:         func = gimp_operation_dodge_mode_process_pixels; break;
      case GIMP_BURN_MODE:          func = gimp_operation_burn_mode_process_pixels; break;
      case GIMP_HARDLIGHT_MODE:     func = gimp_operation_hardlight_mode_process_pixels; break;
      case GIMP_SOFTLIGHT_MODE:     func = gimp_operation_softlight_mode_process_pixels; break;
      case GIMP_GRAIN_EXTRACT_MODE: func = gimp_operation_grain_extract_mode_process_pixels; break;
      case GIMP_GRAIN_MERGE_MODE:   func = gimp_operation_grain_merge_mode_process_pixels; break;
      case GIMP_COLOR_ERASE_MODE:   func = gimp_operation_color_erase_mode_process_pixels; break;
      case GIMP_ERASE_MODE:         func = gimp_operation_erase_mode_process_pixels; break;
      case GIMP_REPLACE_MODE:       func = gimp_operation_replace_mode_process_pixels; break;
      case GIMP_ANTI_ERASE_MODE:    func = gimp_operation_anti_erase_mode_process_pixels; break;
      default:
        g_warning ("No direct function for layer mode (%d), using gimp:normal-mode", paint_mode);
        func = gimp_operation_normal_mode_process_pixels;
        break;
    }

  return func;
}
