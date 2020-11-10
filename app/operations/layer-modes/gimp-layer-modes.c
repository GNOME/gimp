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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <glib-object.h>
#include <gegl.h>

#include "../operations-types.h"

#include "gegl/gimp-babl.h"

#include "gimpoperationlayermode.h"
#include "gimpoperationlayermode-blend.h"

#include "gimp-layer-modes.h"


typedef struct _GimpLayerModeInfo GimpLayerModeInfo;

struct _GimpLayerModeInfo
{
  GimpLayerMode             layer_mode;
  const gchar              *op_name;
  GimpLayerModeBlendFunc    blend_function;
  GimpLayerModeFlags        flags;
  GimpLayerModeContext      context;
  GimpLayerCompositeMode    paint_composite_mode;
  GimpLayerCompositeMode    composite_mode;
  GimpLayerColorSpace       composite_space;
  GimpLayerColorSpace       blend_space;
};


/*  static variables  */

static const GimpLayerModeInfo layer_mode_infos[] =
{
  { GIMP_LAYER_MODE_NORMAL_LEGACY,

    .op_name              = "gimp:normal",
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE  |
                            GIMP_LAYER_MODE_FLAG_TRIVIAL,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_UNION,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_DISSOLVE,

    .op_name              = "gimp:dissolve",
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_TRIVIAL,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_UNION
  },

  { GIMP_LAYER_MODE_BEHIND_LEGACY,

    .op_name              = "gimp:behind",
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_PAINT |
                            GIMP_LAYER_MODE_CONTEXT_FILTER,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_UNION,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_MULTIPLY_LEGACY,

    .op_name              = "gimp:multiply-legacy",
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_SCREEN_LEGACY,

    .op_name              = "gimp:screen-legacy",
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_OVERLAY_LEGACY,

    .op_name              = "gimp:softlight-legacy",
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_DIFFERENCE_LEGACY,

    .op_name              = "gimp:difference-legacy",
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_ADDITION_LEGACY,

    .op_name              = "gimp:addition-legacy",
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_SUBTRACT_LEGACY,

    .op_name              = "gimp:subtract-legacy",
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_DARKEN_ONLY_LEGACY,

    .op_name              = "gimp:darken-only-legacy",
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_LIGHTEN_ONLY_LEGACY,

    .op_name              = "gimp:lighten-only-legacy",
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_HSV_HUE_LEGACY,

    .op_name              = "gimp:hsv-hue-legacy",
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_HSV_SATURATION_LEGACY,

    .op_name              = "gimp:hsv-saturation-legacy",
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_HSL_COLOR_LEGACY,

    .op_name              = "gimp:hsl-color-legacy",
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_HSV_VALUE_LEGACY,

    .op_name              = "gimp:hsv-value-legacy",
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_DIVIDE_LEGACY,

    .op_name              = "gimp:divide-legacy",
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_DODGE_LEGACY,

    .op_name              = "gimp:dodge-legacy",
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_BURN_LEGACY,

    .op_name              = "gimp:burn-legacy",
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_HARDLIGHT_LEGACY,

    .op_name              = "gimp:hardlight-legacy",
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_SOFTLIGHT_LEGACY,

    .op_name              = "gimp:softlight-legacy",
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_GRAIN_EXTRACT_LEGACY,

    .op_name              = "gimp:grain-extract-legacy",
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_GRAIN_MERGE_LEGACY,

    .op_name              = "gimp:grain-merge-legacy",
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_COLOR_ERASE_LEGACY,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_color_erase,
    .flags                = GIMP_LAYER_MODE_FLAG_LEGACY                    |
                            GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE  |
                            GIMP_LAYER_MODE_FLAG_SUBTRACTIVE,
    .context              = GIMP_LAYER_MODE_CONTEXT_PAINT |
                            GIMP_LAYER_MODE_CONTEXT_FILTER,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_OVERLAY,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_overlay,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_LCH_HUE,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_lch_hue,
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_LAB
  },

  { GIMP_LAYER_MODE_LCH_CHROMA,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_lch_chroma,
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_LAB
  },

  { GIMP_LAYER_MODE_LCH_COLOR,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_lch_color,
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_LAB
  },

  { GIMP_LAYER_MODE_LCH_LIGHTNESS,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_lch_lightness,
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_LAB
  },

  { GIMP_LAYER_MODE_NORMAL,

    .op_name              = "gimp:normal",
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_TRIVIAL,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_UNION,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { GIMP_LAYER_MODE_BEHIND,

    .op_name              = "gimp:behind",
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_PAINT |
                            GIMP_LAYER_MODE_CONTEXT_FILTER,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_UNION,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { GIMP_LAYER_MODE_MULTIPLY,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_multiply,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { GIMP_LAYER_MODE_SCREEN,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_screen,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_DIFFERENCE,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_difference,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_ADDITION,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_addition,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { GIMP_LAYER_MODE_SUBTRACT,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_subtract,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { GIMP_LAYER_MODE_DARKEN_ONLY,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_darken_only,
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR
    /* no blend_space: reuse composite space, no conversion thus fewer copies */
  },

  { GIMP_LAYER_MODE_LIGHTEN_ONLY,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_lighten_only,
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR
    /* no blend_space: reuse composite space, no conversion thus fewer copies */
  },

  { GIMP_LAYER_MODE_HSV_HUE,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_hsv_hue,
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_HSV_SATURATION,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_hsv_saturation,
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_HSL_COLOR,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_hsl_color,
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_HSV_VALUE,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_hsv_value,
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_DIVIDE,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_divide,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { GIMP_LAYER_MODE_DODGE,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_dodge,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_BURN,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_burn,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_HARDLIGHT,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_hardlight,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_SOFTLIGHT,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_softlight,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_GRAIN_EXTRACT,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_grain_extract,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_GRAIN_MERGE,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_grain_merge,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_VIVID_LIGHT,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_vivid_light,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_PIN_LIGHT,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_pin_light,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_LINEAR_LIGHT,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_linear_light,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_HARD_MIX,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_hard_mix,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_EXCLUSION,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_exclusion,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_LINEAR_BURN,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_linear_burn,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_LUMA_DARKEN_ONLY,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_luma_darken_only,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_LUMA_LIGHTEN_ONLY,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_luma_lighten_only,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { GIMP_LAYER_MODE_LUMINANCE,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_luminance,
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { GIMP_LAYER_MODE_COLOR_ERASE,

    .op_name              = "gimp:layer-mode",
    .blend_function       = gimp_operation_layer_mode_blend_color_erase,
    .flags                = GIMP_LAYER_MODE_FLAG_SUBTRACTIVE,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { GIMP_LAYER_MODE_ERASE,

    .op_name              = "gimp:erase",
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_SUBTRACTIVE           |
                            GIMP_LAYER_MODE_FLAG_ALPHA_ONLY            |
                            GIMP_LAYER_MODE_FLAG_TRIVIAL,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { GIMP_LAYER_MODE_MERGE,

    .op_name              = "gimp:merge",
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_TRIVIAL,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_UNION,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { GIMP_LAYER_MODE_SPLIT,

    .op_name              = "gimp:split",
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_SUBTRACTIVE               |
                            GIMP_LAYER_MODE_FLAG_ALPHA_ONLY                |
                            GIMP_LAYER_MODE_FLAG_TRIVIAL,
    .context              = GIMP_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP
  },

  { GIMP_LAYER_MODE_PASS_THROUGH,

    .op_name              = "gimp:pass-through",
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE    |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_TRIVIAL,
    .context              = GIMP_LAYER_MODE_CONTEXT_GROUP,
    .composite_mode       = GIMP_LAYER_COMPOSITE_UNION,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { GIMP_LAYER_MODE_REPLACE,

    .op_name              = "gimp:replace",
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_TRIVIAL,
    .context              = GIMP_LAYER_MODE_CONTEXT_FILTER,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_UNION,
    .composite_space      = GIMP_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { GIMP_LAYER_MODE_ANTI_ERASE,

    .op_name              = "gimp:anti-erase",
    .flags                = GIMP_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            GIMP_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            GIMP_LAYER_MODE_FLAG_ALPHA_ONLY,
    .paint_composite_mode = GIMP_LAYER_COMPOSITE_UNION,
    .composite_mode       = GIMP_LAYER_COMPOSITE_UNION
  }
};

static const GimpLayerMode layer_mode_group_default[] =
{
  GIMP_LAYER_MODE_PASS_THROUGH,

  GIMP_LAYER_MODE_SEPARATOR,

  GIMP_LAYER_MODE_REPLACE,

  GIMP_LAYER_MODE_SEPARATOR,

  GIMP_LAYER_MODE_NORMAL,
  GIMP_LAYER_MODE_DISSOLVE,
  GIMP_LAYER_MODE_BEHIND,
  GIMP_LAYER_MODE_COLOR_ERASE,
  GIMP_LAYER_MODE_ERASE,
  GIMP_LAYER_MODE_ANTI_ERASE,
  GIMP_LAYER_MODE_MERGE,
  GIMP_LAYER_MODE_SPLIT,

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
  GIMP_LAYER_MODE_LINEAR_BURN,

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
  GIMP_LAYER_MODE_EXCLUSION,
  GIMP_LAYER_MODE_SUBTRACT,
  GIMP_LAYER_MODE_GRAIN_EXTRACT,
  GIMP_LAYER_MODE_GRAIN_MERGE,
  GIMP_LAYER_MODE_DIVIDE,

  GIMP_LAYER_MODE_SEPARATOR,

  GIMP_LAYER_MODE_HSV_HUE,
  GIMP_LAYER_MODE_HSV_SATURATION,
  GIMP_LAYER_MODE_HSL_COLOR,
  GIMP_LAYER_MODE_HSV_VALUE,

  GIMP_LAYER_MODE_SEPARATOR,

  GIMP_LAYER_MODE_LCH_HUE,
  GIMP_LAYER_MODE_LCH_CHROMA,
  GIMP_LAYER_MODE_LCH_COLOR,
  GIMP_LAYER_MODE_LCH_LIGHTNESS,
  GIMP_LAYER_MODE_LUMINANCE
};

static const GimpLayerMode layer_mode_group_legacy[] =
{
  GIMP_LAYER_MODE_NORMAL_LEGACY,
  GIMP_LAYER_MODE_DISSOLVE,
  GIMP_LAYER_MODE_BEHIND_LEGACY,
  GIMP_LAYER_MODE_COLOR_ERASE_LEGACY,

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
  GIMP_LAYER_MODE_HSL_COLOR_LEGACY,
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

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_HSL_COLOR,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = GIMP_LAYER_MODE_HSL_COLOR_LEGACY
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
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = GIMP_LAYER_MODE_COLOR_ERASE_LEGACY,
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

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_MERGE,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_SPLIT,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_PASS_THROUGH,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = -1,
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_REPLACE,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [GIMP_LAYER_MODE_GROUP_DEFAULT] = GIMP_LAYER_MODE_ANTI_ERASE,
    [GIMP_LAYER_MODE_GROUP_LEGACY ] = -1
  }
};

static GeglOperation *ops[G_N_ELEMENTS (layer_mode_infos)] = { 0 };

/*  public functions  */

void
gimp_layer_modes_init (void)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS (layer_mode_infos); i++)
    {
      gimp_assert ((GimpLayerMode) i == layer_mode_infos[i].layer_mode);
    }
}

void
gimp_layer_modes_exit (void)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS (layer_mode_infos); i++)
    {
      if (ops[i])
        {
          GeglNode *node;

          node = ops[i]->node;
          g_object_unref (node);
          ops[i] = NULL;
        }
    }
}

static const GimpLayerModeInfo *
gimp_layer_mode_info (GimpLayerMode mode)
{
  g_return_val_if_fail (mode >= 0 && mode < G_N_ELEMENTS (layer_mode_infos),
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

gboolean
gimp_layer_mode_is_alpha_only (GimpLayerMode mode)
{
  const GimpLayerModeInfo *info = gimp_layer_mode_info (mode);

  if (! info)
    return FALSE;

  return (info->flags & GIMP_LAYER_MODE_FLAG_ALPHA_ONLY) != 0;
}

gboolean
gimp_layer_mode_is_trivial (GimpLayerMode mode)
{
  const GimpLayerModeInfo *info = gimp_layer_mode_info (mode);

  if (! info)
    return FALSE;

  return (info->flags & GIMP_LAYER_MODE_FLAG_TRIVIAL) != 0;
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
    return GIMP_LAYER_COMPOSITE_UNION;

  return info->composite_mode;
}

GimpLayerCompositeMode
gimp_layer_mode_get_paint_composite_mode (GimpLayerMode mode)
{
  const GimpLayerModeInfo *info = gimp_layer_mode_info (mode);

  if (! info)
    return GIMP_LAYER_COMPOSITE_UNION;

  return info->paint_composite_mode;
}

const gchar *
gimp_layer_mode_get_operation_name (GimpLayerMode mode)
{
  const GimpLayerModeInfo *info = gimp_layer_mode_info (mode);

  if (! info)
    return "gimp:layer-mode";

  return info->op_name;
}

/**
 * gimp_layer_mode_get_operation:
 * @mode:
 *
 * Returns: a #GeglOperation for @mode which may be reused and must not
 *          be freed.
 */
GeglOperation *
gimp_layer_mode_get_operation (GimpLayerMode mode)
{
  const GimpLayerModeInfo *info = gimp_layer_mode_info (mode);
  const gchar             *op_name;

  op_name = gimp_layer_mode_get_operation_name (mode);

  if (! info)
    info = layer_mode_infos;

  mode = info - layer_mode_infos;

  if (! ops[mode])
    {
      GeglNode      *node;
      GeglOperation *operation;

      node = gegl_node_new_child (NULL,
                                  "operation", op_name,
                                  NULL);

      operation = gegl_node_get_gegl_operation (node);
      ops[mode] = operation;

      if (GIMP_IS_OPERATION_LAYER_MODE (operation))
        {
          GimpOperationLayerMode *layer_mode = GIMP_OPERATION_LAYER_MODE (operation);

          layer_mode->layer_mode      = mode;
          layer_mode->function        = GIMP_OPERATION_LAYER_MODE_GET_CLASS (operation)->process;
          layer_mode->blend_function  = gimp_layer_mode_get_blend_function (mode);
          layer_mode->blend_space     = gimp_layer_mode_get_blend_space (mode);
          layer_mode->composite_space = gimp_layer_mode_get_composite_space (mode);
          layer_mode->composite_mode  = gimp_layer_mode_get_paint_composite_mode (mode);
        }
    }

  return ops[mode];
}

GimpLayerModeFunc
gimp_layer_mode_get_function (GimpLayerMode mode)
{
  GeglOperation *operation;

  operation = gimp_layer_mode_get_operation (mode);

  return GIMP_OPERATION_LAYER_MODE_GET_CLASS (operation)->process;
}

GimpLayerModeBlendFunc
gimp_layer_mode_get_blend_function (GimpLayerMode mode)
{
  const GimpLayerModeInfo *info = gimp_layer_mode_info (mode);

  if (! info)
    return NULL;

  return info->blend_function;
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
gimp_layer_mode_get_format (GimpLayerMode           mode,
                            GimpLayerColorSpace     blend_space,
                            GimpLayerColorSpace     composite_space,
                            GimpLayerCompositeMode  composite_mode,
                            const Babl             *preferred_format)
{
  GimpLayerCompositeRegion composite_region;

  /* for now, all modes perform i/o in the composite space. */
  (void) mode;
  (void) blend_space;

  if (composite_space == GIMP_LAYER_COLOR_SPACE_AUTO)
    composite_space = gimp_layer_mode_get_composite_space (mode);

  if (composite_mode == GIMP_LAYER_COMPOSITE_AUTO)
    composite_mode = gimp_layer_mode_get_composite_mode (mode);

  composite_region = gimp_layer_mode_get_included_region (mode, composite_mode);

  if (gimp_layer_mode_is_alpha_only (mode))
    {
      if (composite_region != GIMP_LAYER_COMPOSITE_REGION_UNION)
        {
          /* alpha-only layer modes don't combine colors in non-union composite
           * modes, hence we can disregard the composite space.
           */
          composite_space = GIMP_LAYER_COLOR_SPACE_AUTO;
        }
    }
  else if (gimp_layer_mode_is_trivial (mode))
    {
      if (! (composite_region & GIMP_LAYER_COMPOSITE_REGION_DESTINATION))
        {
          /* trivial layer modes don't combine colors when only the source
           * region is included, hence we can disregard the composite space.
           */
          composite_space = GIMP_LAYER_COLOR_SPACE_AUTO;
        }
    }

  switch (composite_space)
    {
    case GIMP_LAYER_COLOR_SPACE_AUTO:
      /* compositing is color-space agnostic.  return a format that has a fast
       * conversion path to/from the preferred format.
       */
      if (! preferred_format ||
          gimp_babl_format_get_trc (preferred_format) == GIMP_TRC_LINEAR)
        return babl_format_with_space ("RGBA float", preferred_format);
      else
        return babl_format_with_space ("R'G'B'A float", preferred_format);

    case GIMP_LAYER_COLOR_SPACE_RGB_LINEAR:
      return babl_format_with_space ("RGBA float", preferred_format);

    case GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL:
      return babl_format_with_space ("R'G'B'A float", preferred_format);

    case GIMP_LAYER_COLOR_SPACE_LAB:
      return babl_format_with_space ("CIE Lab alpha float", preferred_format);
    }

  g_return_val_if_reached (babl_format_with_space ("RGBA float", preferred_format));
}

GimpLayerCompositeRegion
gimp_layer_mode_get_included_region (GimpLayerMode          mode,
                                     GimpLayerCompositeMode composite_mode)
{
  if (composite_mode == GIMP_LAYER_COMPOSITE_AUTO)
    composite_mode = gimp_layer_mode_get_composite_mode (mode);

  switch (composite_mode)
    {
    case GIMP_LAYER_COMPOSITE_UNION:
      return GIMP_LAYER_COMPOSITE_REGION_UNION;

    case GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP:
      return GIMP_LAYER_COMPOSITE_REGION_DESTINATION;

    case GIMP_LAYER_COMPOSITE_CLIP_TO_LAYER:
      return GIMP_LAYER_COMPOSITE_REGION_SOURCE;

    case GIMP_LAYER_COMPOSITE_INTERSECTION:
      return GIMP_LAYER_COMPOSITE_REGION_INTERSECTION;

    default:
      g_return_val_if_reached (GIMP_LAYER_COMPOSITE_REGION_INTERSECTION);
    }
}
