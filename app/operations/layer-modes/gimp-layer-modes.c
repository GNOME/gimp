/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-layer-modes.c
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
 *                    Øyvind Kolås <pippin@gimp.org>
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

#include <glib-object.h>

#include "../operations-types.h"

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

#include "gimpoperationantierase.h"
#include "gimpoperationbehind.h"
#include "gimpoperationcolorerase.h"
#include "gimpoperationdissolve.h"
#include "gimpoperationerase.h"
#include "gimpoperationnormal.h"
#include "gimpoperationreplace.h"

#include "gimp-layer-modes.h"


typedef struct _GimpLayerModeInfo GimpLayerModeInfo;

struct _GimpLayerModeInfo
{
  GimpLayerMode           layer_mode;
  const gchar            *op_name;
  GimpLayerModeFunc       function;
  GimpLayerModeFlags      flags;
  GimpLayerModeContext    context;
  GimpLayerCompositeMode  paint_composite_mode;
  GimpLayerCompositeMode  composite_mode;
  GimpLayerColorSpace     composite_space;
  GimpLayerColorSpace     blend_space;
};


/*  static variables  */

static const GimpLayerModeInfo layer_mode_infos[] =
{
  { GIMP_LAYER_MODE_NORMAL_LEGACY,

    .op_name              = "gimp:normal",
    .function             = gimp_operation_normal_process,
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_DISSOLVE,

    .op_name              = "gimp:dissolve",
    .function             = gimp_operation_dissolve_process,
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_OVER
  },

  { GIMP_LAYER_MODE_BEHIND_LEGACY,

    .op_name              = "gimp:behind",
    .function             = gimp_operation_behind_process,
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_PAINT |
                            GIMP_LAYER_MODE_CONTEXT_FADE,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_MULTIPLY_LEGACY,

    .op_name              = "gimp:multiply-legacy",
    .function             = gimp_operation_multiply_legacy_process,
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_SCREEN_LEGACY,

    .op_name              = "gimp:screen-legacy",
    .function             = gimp_operation_screen_legacy_process,
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_OVERLAY_LEGACY,

    .op_name              = "gimp:softlight-legacy",
    .function             = gimp_operation_softlight_legacy_process,
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_DIFFERENCE_LEGACY,

    .op_name              = "gimp:difference-legacy",
    .function             = gimp_operation_difference_legacy_process,
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_ADDITION_LEGACY,

    .op_name              = "gimp:addition-legacy",
    .function             = gimp_operation_addition_legacy_process,
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_SUBTRACT_LEGACY,

    .op_name              = "gimp:subtract-legacy",
    .function             = gimp_operation_subtract_legacy_process,
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_DARKEN_ONLY_LEGACY,

    .op_name              = "gimp:darken-only-legacy",
    .function             = gimp_operation_darken_only_legacy_process,
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_LIGHTEN_ONLY_LEGACY,

    .op_name              = "gimp:lighten-only-legacy",
    .function             = gimp_operation_lighten_only_legacy_process,
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_HSV_HUE_LEGACY,

    .op_name              = "gimp:hsv-hue-legacy",
    .function             = gimp_operation_hsv_hue_legacy_process,
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_HSV_SATURATION_LEGACY,

    .op_name              = "gimp:hsv-saturation-legacy",
    .function             = gimp_operation_hsv_saturation_legacy_process,
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_HSV_COLOR_LEGACY,

    .op_name              = "gimp:hsv-color-legacy",
    .function             = gimp_operation_hsv_color_legacy_process,
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_HSV_VALUE_LEGACY,

    .op_name              = "gimp:hsv-value-legacy",
    .function             = gimp_operation_hsv_value_legacy_process,
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_DIVIDE_LEGACY,

    .op_name              = "gimp:divide-legacy",
    .function             = gimp_operation_divide_legacy_process,
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_DODGE_LEGACY,

    .op_name              = "gimp:dodge-legacy",
    .function             = gimp_operation_dodge_legacy_process,
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_BURN_LEGACY,

    .op_name              = "gimp:burn-legacy",
    .function             = gimp_operation_burn_legacy_process,
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_HARDLIGHT_LEGACY,

    .op_name              = "gimp:hardlight-legacy",
    .function             = gimp_operation_hardlight_legacy_process,
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_SOFTLIGHT_LEGACY,

    .op_name              = "gimp:softlight-legacy",
    .function             = gimp_operation_softlight_legacy_process,
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_GRAIN_EXTRACT_LEGACY,

    .op_name              = "gimp:grain-extract-legacy",
    .function             = gimp_operation_grain_extract_legacy_process,
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_GRAIN_MERGE_LEGACY,

    .op_name              = "gimp:grain-merge-legacy",
    .function             = gimp_operation_grain_merge_legacy_process,
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_COLOR_ERASE,

    .op_name              = "gimp:color-erase",
    .function             = gimp_operation_color_erase_process,
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE  |
                            GIMP_LAYER_MODE_FLAG_SUBTRACTIVE,
    .context              = GIMP_LAYER_MODE_CONTEXT_PAINT |
                            GIMP_LAYER_MODE_CONTEXT_FADE,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_OVERLAY,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_LCH_HUE,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_LAB
  },

  { GIMP_LAYER_MODE_LCH_CHROMA,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_LAB
  },

  { GIMP_LAYER_MODE_LCH_COLOR,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_LAB
  },

  { GIMP_LAYER_MODE_LCH_LIGHTNESS,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_LAB
  },

  { GIMP_LAYER_MODE_NORMAL,

    .op_name              = "gimp:normal",
    .function             = gimp_operation_normal_process,
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { GIMP_LAYER_MODE_BEHIND,

    .op_name              = "gimp:behind",
    .function             = gimp_operation_behind_process,
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_PAINT |
                            GIMP_LAYER_MODE_CONTEXT_FADE,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { GIMP_LAYER_MODE_MULTIPLY,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { GIMP_LAYER_MODE_SCREEN,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_DIFFERENCE,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_ADDITION,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { GIMP_LAYER_MODE_SUBTRACT,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { GIMP_LAYER_MODE_DARKEN_ONLY,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR
    /* no blend_space: reuse composite space, no conversion thus fewer copies */
  },

  { GIMP_LAYER_MODE_LIGHTEN_ONLY,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR
    /* no blend_space: reuse composite space, no conversion thus fewer copies */
  },

  { GIMP_LAYER_MODE_HSV_HUE,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_HSV_SATURATION,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_HSV_COLOR,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_HSV_VALUE,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_DIVIDE,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { GIMP_LAYER_MODE_DODGE,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_BURN,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_HARDLIGHT,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_SOFTLIGHT,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_GRAIN_EXTRACT,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_GRAIN_MERGE,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_VIVID_LIGHT,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_PIN_LIGHT,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_LINEAR_LIGHT,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_HARD_MIX,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_EXCLUSION,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_LINEAR_BURN,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_LUMA_DARKEN_ONLY,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_LUMA_LIGHTEN_ONLY,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_LUMINANCE,

    .op_name              = "gimp:layer-mode",
    .function             = gimp_operation_layer_mode_process_pixels,
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { GIMP_LAYER_MODE_ERASE,

    .op_name              = "gimp:erase",
    .function             = gimp_operation_erase_process,
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_SUBTRACTIVE,
    .context              = GIMP_LAYER_MODE_CONTEXT_FADE,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_ATOP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { GIMP_LAYER_MODE_REPLACE,

    .op_name              = "gimp:replace",
    .function             = gimp_operation_replace_process,
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_FADE,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { GIMP_LAYER_MODE_ANTI_ERASE,

    .op_name              = "gimp:anti-erase",
    .function             = gimp_operation_anti_erase_process,
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_FADE,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_SRC_OVER,
    .composite_mode       = GIMP_LAYER_COMPOSITE_SRC_OVER
  }
};

static const GimpLayerMode layer_mode_group_default[] =
{
  GIMP_LAYER_MODE_NORMAL,
  GIMP_LAYER_MODE_REPLACE,
  GIMP_LAYER_MODE_DISSOLVE,
  GIMP_LAYER_MODE_BEHIND,
  GIMP_LAYER_MODE_COLOR_ERASE,
  GIMP_LAYER_MODE_ERASE,
  GIMP_LAYER_MODE_ANTI_ERASE,

  GIMP_LAYER_MODE_SEPARATOR,

  GIMP_LAYER_MODE_LIGHTEN_ONLY,
  GIMP_LAYER_MODE_LUMA_LIGHTEN_ONLY,
  GIMP_LAYER_MODE_SCREEN,
  GIMP_LAYER_MODE_DODGE,
  GIMP_LAYER_MODE_ADDITION,

  GIMP_LAYER_MODE_SEPARATOR,

  GIMP_LAYER_MODE_DARKEN_ONLY,
  GIMP_LAYER_MODE_LUMA_DARKEN_ONLY,
  GIMP_LAYER_MODE_MULTIPLY,
  GIMP_LAYER_MODE_BURN,

  GIMP_LAYER_MODE_SEPARATOR,

  GIMP_LAYER_MODE_OVERLAY,
  GIMP_LAYER_MODE_SOFTLIGHT,
  GIMP_LAYER_MODE_HARDLIGHT,
  GIMP_LAYER_MODE_VIVID_LIGHT,
  GIMP_LAYER_MODE_PIN_LIGHT,
  GIMP_LAYER_MODE_LINEAR_LIGHT,
  GIMP_LAYER_MODE_HARD_MIX,

  GIMP_LAYER_MODE_SEPARATOR,

  GIMP_LAYER_MODE_DIFFERENCE,
  GIMP_LAYER_MODE_SUBTRACT,
  GIMP_LAYER_MODE_GRAIN_EXTRACT,
  GIMP_LAYER_MODE_GRAIN_MERGE,
  GIMP_LAYER_MODE_DIVIDE,

  GIMP_LAYER_MODE_SEPARATOR,

  GIMP_LAYER_MODE_HSV_HUE,
  GIMP_LAYER_MODE_HSV_SATURATION,
  GIMP_LAYER_MODE_HSV_COLOR,
  GIMP_LAYER_MODE_HSV_VALUE,

  GIMP_LAYER_MODE_SEPARATOR,

  GIMP_LAYER_MODE_LCH_HUE,
  GIMP_LAYER_MODE_LCH_CHROMA,
  GIMP_LAYER_MODE_LCH_COLOR,
  GIMP_LAYER_MODE_LCH_LIGHTNESS,
  GIMP_LAYER_MODE_LUMINANCE,

  GIMP_LAYER_MODE_SEPARATOR,

  GIMP_LAYER_MODE_EXCLUSION,
  GIMP_LAYER_MODE_LINEAR_BURN
};

static const GimpLayerMode layer_mode_group_legacy[] =
{
  GIMP_LAYER_MODE_NORMAL_LEGACY,
  GIMP_LAYER_MODE_DISSOLVE,
  GIMP_LAYER_MODE_BEHIND_LEGACY,

  GIMP_LAYER_MODE_SEPARATOR,

  GIMP_LAYER_MODE_LIGHTEN_ONLY_LEGACY,
  GIMP_LAYER_MODE_SCREEN_LEGACY,
  GIMP_LAYER_MODE_DODGE_LEGACY,
  GIMP_LAYER_MODE_ADDITION_LEGACY,

  GIMP_LAYER_MODE_SEPARATOR,

  GIMP_LAYER_MODE_DARKEN_ONLY_LEGACY,
  GIMP_LAYER_MODE_MULTIPLY_LEGACY,
  GIMP_LAYER_MODE_BURN_LEGACY,

  GIMP_LAYER_MODE_SEPARATOR,

  GIMP_LAYER_MODE_OVERLAY,
  GIMP_LAYER_MODE_SOFTLIGHT_LEGACY,
  GIMP_LAYER_MODE_HARDLIGHT_LEGACY,

  GIMP_LAYER_MODE_SEPARATOR,

  GIMP_LAYER_MODE_DIFFERENCE_LEGACY,
  GIMP_LAYER_MODE_SUBTRACT_LEGACY,
  GIMP_LAYER_MODE_GRAIN_EXTRACT_LEGACY,
  GIMP_LAYER_MODE_GRAIN_MERGE_LEGACY,
  GIMP_LAYER_MODE_DIVIDE_LEGACY,

  GIMP_LAYER_MODE_SEPARATOR,

  GIMP_LAYER_MODE_HSV_HUE_LEGACY,
  GIMP_LAYER_MODE_HSV_SATURATION_LEGACY,
  GIMP_LAYER_MODE_HSV_COLOR_LEGACY,
  GIMP_LAYER_MODE_HSV_VALUE_LEGACY
};

static const GimpLayerMode layer_mode_groups[][2] =
{
  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_NORMAL,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = GIMP_LAYER_MODE_NORMAL_LEGACY
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_DISSOLVE,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = GIMP_LAYER_MODE_DISSOLVE
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_BEHIND,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = GIMP_LAYER_MODE_BEHIND_LEGACY
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_MULTIPLY,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = GIMP_LAYER_MODE_MULTIPLY_LEGACY
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_SCREEN,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = GIMP_LAYER_MODE_SCREEN_LEGACY
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_OVERLAY,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_DIFFERENCE,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = GIMP_LAYER_MODE_DIFFERENCE_LEGACY
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_ADDITION,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = GIMP_LAYER_MODE_ADDITION_LEGACY
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_SUBTRACT,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = GIMP_LAYER_MODE_SUBTRACT_LEGACY
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_DARKEN_ONLY,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = GIMP_LAYER_MODE_DARKEN_ONLY_LEGACY
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_LIGHTEN_ONLY,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = GIMP_LAYER_MODE_LIGHTEN_ONLY_LEGACY
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_HSV_HUE,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = GIMP_LAYER_MODE_HSV_HUE_LEGACY
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_HSV_SATURATION,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = GIMP_LAYER_MODE_HSV_SATURATION_LEGACY
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_HSV_COLOR,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = GIMP_LAYER_MODE_HSV_COLOR_LEGACY
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_HSV_VALUE,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = GIMP_LAYER_MODE_HSV_VALUE_LEGACY
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_DIVIDE,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = GIMP_LAYER_MODE_DIVIDE_LEGACY
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_DODGE,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = GIMP_LAYER_MODE_DODGE_LEGACY
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_BURN,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = GIMP_LAYER_MODE_BURN_LEGACY
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_HARDLIGHT,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = GIMP_LAYER_MODE_HARDLIGHT_LEGACY
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_SOFTLIGHT,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = GIMP_LAYER_MODE_SOFTLIGHT_LEGACY
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_GRAIN_EXTRACT,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = GIMP_LAYER_MODE_GRAIN_EXTRACT_LEGACY
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_GRAIN_MERGE,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = GIMP_LAYER_MODE_GRAIN_MERGE_LEGACY
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_COLOR_ERASE,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = -1,
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_VIVID_LIGHT,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_PIN_LIGHT,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_LINEAR_LIGHT,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_HARD_MIX,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_EXCLUSION,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_LINEAR_BURN,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_LUMA_DARKEN_ONLY,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_LUMA_LIGHTEN_ONLY,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_LUMINANCE,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_ERASE,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_REPLACE,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_ANTI_ERASE,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = -1
  }
};


/*  public functions  */

void
gimp_layer_modes_init (void)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS (layer_mode_infos); i++)
    {
      g_assert ((GimpLayerMode) i == layer_mode_infos[i].layer_mode);
    }
}

static const GimpLayerModeInfo *
gimp_layer_mode_info (GimpLayerMode mode)
{
  g_return_val_if_fail (mode < G_N_ELEMENTS (layer_mode_infos),
                        &layer_mode_infos[0]);

  return &layer_mode_infos[mode];
}

gboolean
gimp_layer_mode_is_legacy (GimpLayerMode mode)
{
  const GimpLayerModeInfo *info = gimp_layer_mode_info (mode);

  if (! info)
    return FALSE;

  return (info->flags & GIMP_LAYER_MODE_FLAG_LEGACY) != 0;
}

gboolean
gimp_layer_mode_is_blend_space_mutable (GimpLayerMode mode)
{
  const GimpLayerModeInfo *info = gimp_layer_mode_info (mode);

  if (! info)
    return FALSE;

  return (info->flags & GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE) == 0;
}

gboolean
gimp_layer_mode_is_composite_space_mutable (GimpLayerMode mode)
{
  const GimpLayerModeInfo *info = gimp_layer_mode_info (mode);

  if (! info)
    return FALSE;

  return (info->flags & GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE) == 0;
}

gboolean
gimp_layer_mode_is_composite_mode_mutable (GimpLayerMode mode)
{
  const GimpLayerModeInfo *info = gimp_layer_mode_info (mode);

  if (! info)
    return FALSE;

  return (info->flags & GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE) == 0;
}

gboolean
gimp_layer_mode_is_subtractive (GimpLayerMode mode)
{
  const GimpLayerModeInfo *info = gimp_layer_mode_info (mode);

  if (! info)
    return FALSE;

  return (info->flags & GIMP_LAYER_MODE_FLAG_SUBTRACTIVE) != 0;
}

GimpLayerColorSpace
gimp_layer_mode_get_blend_space (GimpLayerMode mode)
{
  const GimpLayerModeInfo *info = gimp_layer_mode_info (mode);

  if (! info)
    return GIMP_LAYER_COLOR_SPACE_RGB_LINEAR;

  return info->blend_space;
}

GimpLayerColorSpace
gimp_layer_mode_get_composite_space (GimpLayerMode mode)
{
  const GimpLayerModeInfo *info = gimp_layer_mode_info (mode);

  if (! info)
    return GIMP_LAYER_COLOR_SPACE_RGB_LINEAR;

  return info->composite_space;
}

GimpLayerCompositeMode
gimp_layer_mode_get_composite_mode (GimpLayerMode mode)
{
  const GimpLayerModeInfo *info = gimp_layer_mode_info (mode);

  if (! info)
    return GIMP_LAYER_COMPOSITE_SRC_OVER;

  return info->composite_mode;
}

GimpLayerCompositeMode
gimp_layer_mode_get_paint_composite_mode (GimpLayerMode mode)
{
  const GimpLayerModeInfo *info = gimp_layer_mode_info (mode);

  if (! info)
    return GIMP_LAYER_COMPOSITE_SRC_OVER;

  return info->paint_composite_mode;
}

const gchar *
gimp_layer_mode_get_operation (GimpLayerMode mode)
{
  const GimpLayerModeInfo *info = gimp_layer_mode_info (mode);

  if (! info)
    return "gimp:layer-mode";

  return info->op_name;
}

GimpLayerModeFunc
gimp_layer_mode_get_function (GimpLayerMode mode)
{
  const GimpLayerModeInfo *info = gimp_layer_mode_info (mode);

  if (! info)
    return gimp_operation_layer_mode_process_pixels;

  return info->function;
}

GimpLayerModeContext
gimp_layer_mode_get_context (GimpLayerMode mode)
{
  const GimpLayerModeInfo *info = gimp_layer_mode_info (mode);

  if (! info)
    return 0;

  return info->context;
}

GimpLayerMode *
gimp_layer_mode_get_context_array (GimpLayerMode         mode,
                                   GimpLayerModeContext  context,
                                   gint                 *n_modes)
{
  GimpLayerModeGroup   group;
  const GimpLayerMode *group_modes;
  gint                 n_group_modes;
  GimpLayerMode       *array;
  gint                 i;

  group = gimp_layer_mode_get_group (mode);

  group_modes = gimp_layer_mode_get_group_array (group, &n_group_modes);

  array = g_new0 (GimpLayerMode, n_group_modes);
  *n_modes = 0;

  for (i = 0; i < n_group_modes; i++)
    {
      if (group_modes[i] != GIMP_LAYER_MODE_SEPARATOR &&
          (gimp_layer_mode_get_context (group_modes[i]) & context))
        {
          array[*n_modes] = group_modes[i];
          (*n_modes)++;
        }
    }

  return array;
}

static gboolean
is_mode_in_array (const GimpLayerMode *modes,
                  gint                 n_modes,
                  GimpLayerMode        mode)
{
  gint i;

  for (i = 0; i < n_modes; i++)
    {
      if (modes[i] == mode)
        return TRUE;
    }

  return FALSE;
}

GimpLayerModeGroup
gimp_layer_mode_get_group (GimpLayerMode mode)
{
  if (is_mode_in_array (layer_mode_group_default,
                        G_N_ELEMENTS (layer_mode_group_default), mode))
    {
      return GIMP_LAYER_MODE_GROUP_DEFAULT;
    }
  else if (is_mode_in_array (layer_mode_group_legacy,
                             G_N_ELEMENTS (layer_mode_group_legacy), mode))
    {
      return GIMP_LAYER_MODE_GROUP_LEGACY;
    }

  return GIMP_LAYER_MODE_GROUP_DEFAULT;
}

const GimpLayerMode *
gimp_layer_mode_get_group_array (GimpLayerModeGroup  group,
                                 gint               *n_modes)
{
  g_return_val_if_fail (n_modes != NULL, NULL);

  switch (group)
    {
    case GIMP_LAYER_MODE_GROUP_DEFAULT:
      *n_modes = G_N_ELEMENTS (layer_mode_group_default);
      return layer_mode_group_default;

    case GIMP_LAYER_MODE_GROUP_LEGACY:
      *n_modes = G_N_ELEMENTS (layer_mode_group_legacy);
      return layer_mode_group_legacy;

    default:
      g_return_val_if_reached (NULL);
    }
}

gboolean
gimp_layer_mode_get_for_group (GimpLayerMode       old_mode,
                               GimpLayerModeGroup  new_group,
                               GimpLayerMode      *new_mode)
{
  gint i;

  g_return_val_if_fail (new_mode != NULL, FALSE);

  for (i = 0; i < G_N_ELEMENTS (layer_mode_groups); i++)
    {
      if (is_mode_in_array (layer_mode_groups[i], 2, old_mode))
        {
          *new_mode = layer_mode_groups[i][new_group];

          if (*new_mode != -1)
            return TRUE;

          return FALSE;
        }
    }

  *new_mode = -1;

  return FALSE;
}

const Babl *
gimp_layer_mode_get_format (GimpLayerMode        mode,
                            GimpLayerColorSpace  composite_space,
                            GimpLayerColorSpace  blend_space,
                            const Babl          *preferred_format)
{
  /* for now, all modes perform i/o in the composite space. */
  (void) mode;
  (void) blend_space;

  if (composite_space == GIMP_LAYER_COLOR_SPACE_AUTO)
    composite_space = gimp_layer_mode_get_composite_space (mode);

  switch (composite_space)
    {
    case GIMP_LAYER_COLOR_SPACE_AUTO:
      /* compositing is color-space agnostic.  try to return the preferred
       * format, and fall back to linear.
       */
      if (preferred_format == babl_format ("RGBA float") ||
          preferred_format == babl_format ("R'G'B'A float"))
        {
          return preferred_format;
        }
      else
        {
          return babl_format ("RGBA float");
        }

    case GIMP_LAYER_COLOR_SPACE_RGB_LINEAR:
      return babl_format ("RGBA float");

    case GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL:
      return babl_format ("R'G'B'A float");

    case GIMP_LAYER_COLOR_SPACE_LAB:
      return babl_format ("CIE Lab alpha float");
    }

  g_return_val_if_reached (babl_format ("RGBA float"));
}
