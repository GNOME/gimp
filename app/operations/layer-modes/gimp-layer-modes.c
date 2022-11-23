/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligma-layer-modes.c
 * Copyright (C) 2017 Michael Natterer <mitch@ligma.org>
 *                    Øyvind Kolås <pippin@ligma.org>
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

#include "gegl/ligma-babl.h"

#include "ligmaoperationlayermode.h"
#include "ligmaoperationlayermode-blend.h"

#include "ligma-layer-modes.h"


typedef struct _LigmaLayerModeInfo LigmaLayerModeInfo;

struct _LigmaLayerModeInfo
{
  LigmaLayerMode             layer_mode;
  const gchar              *op_name;
  LigmaLayerModeBlendFunc    blend_function;
  LigmaLayerModeFlags        flags;
  LigmaLayerModeContext      context;
  LigmaLayerCompositeMode    paint_composite_mode;
  LigmaLayerCompositeMode    composite_mode;
  LigmaLayerColorSpace       composite_space;
  LigmaLayerColorSpace       blend_space;
};


/*  static variables  */

static const LigmaLayerModeInfo layer_mode_infos[] =
{
  { LIGMA_LAYER_MODE_NORMAL_LEGACY,

    .op_name              = "ligma:normal",
    .flags                = LIGMA_LAYER_MODE_FLAG_LEGACY                    |
                            LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE  |
                            LIGMA_LAYER_MODE_FLAG_TRIVIAL,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_DISSOLVE,

    .op_name              = "ligma:dissolve",
    .flags                = LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            LIGMA_LAYER_MODE_FLAG_TRIVIAL,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_UNION
  },

  { LIGMA_LAYER_MODE_BEHIND_LEGACY,

    .op_name              = "ligma:behind",
    .flags                = LIGMA_LAYER_MODE_FLAG_LEGACY                    |
                            LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_PAINT |
                            LIGMA_LAYER_MODE_CONTEXT_FILTER,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_MULTIPLY_LEGACY,

    .op_name              = "ligma:multiply-legacy",
    .flags                = LIGMA_LAYER_MODE_FLAG_LEGACY                    |
                            LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_SCREEN_LEGACY,

    .op_name              = "ligma:screen-legacy",
    .flags                = LIGMA_LAYER_MODE_FLAG_LEGACY                    |
                            LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_OVERLAY_LEGACY,

    .op_name              = "ligma:softlight-legacy",
    .flags                = LIGMA_LAYER_MODE_FLAG_LEGACY                    |
                            LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_DIFFERENCE_LEGACY,

    .op_name              = "ligma:difference-legacy",
    .flags                = LIGMA_LAYER_MODE_FLAG_LEGACY                    |
                            LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_ADDITION_LEGACY,

    .op_name              = "ligma:addition-legacy",
    .flags                = LIGMA_LAYER_MODE_FLAG_LEGACY                    |
                            LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_SUBTRACT_LEGACY,

    .op_name              = "ligma:subtract-legacy",
    .flags                = LIGMA_LAYER_MODE_FLAG_LEGACY                    |
                            LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_DARKEN_ONLY_LEGACY,

    .op_name              = "ligma:darken-only-legacy",
    .flags                = LIGMA_LAYER_MODE_FLAG_LEGACY                    |
                            LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_LIGHTEN_ONLY_LEGACY,

    .op_name              = "ligma:lighten-only-legacy",
    .flags                = LIGMA_LAYER_MODE_FLAG_LEGACY                    |
                            LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_HSV_HUE_LEGACY,

    .op_name              = "ligma:hsv-hue-legacy",
    .flags                = LIGMA_LAYER_MODE_FLAG_LEGACY                    |
                            LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_HSV_SATURATION_LEGACY,

    .op_name              = "ligma:hsv-saturation-legacy",
    .flags                = LIGMA_LAYER_MODE_FLAG_LEGACY                    |
                            LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_HSL_COLOR_LEGACY,

    .op_name              = "ligma:hsl-color-legacy",
    .flags                = LIGMA_LAYER_MODE_FLAG_LEGACY                    |
                            LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_HSV_VALUE_LEGACY,

    .op_name              = "ligma:hsv-value-legacy",
    .flags                = LIGMA_LAYER_MODE_FLAG_LEGACY                    |
                            LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_DIVIDE_LEGACY,

    .op_name              = "ligma:divide-legacy",
    .flags                = LIGMA_LAYER_MODE_FLAG_LEGACY                    |
                            LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_DODGE_LEGACY,

    .op_name              = "ligma:dodge-legacy",
    .flags                = LIGMA_LAYER_MODE_FLAG_LEGACY                    |
                            LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_BURN_LEGACY,

    .op_name              = "ligma:burn-legacy",
    .flags                = LIGMA_LAYER_MODE_FLAG_LEGACY                    |
                            LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_HARDLIGHT_LEGACY,

    .op_name              = "ligma:hardlight-legacy",
    .flags                = LIGMA_LAYER_MODE_FLAG_LEGACY                    |
                            LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_SOFTLIGHT_LEGACY,

    .op_name              = "ligma:softlight-legacy",
    .flags                = LIGMA_LAYER_MODE_FLAG_LEGACY                    |
                            LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_GRAIN_EXTRACT_LEGACY,

    .op_name              = "ligma:grain-extract-legacy",
    .flags                = LIGMA_LAYER_MODE_FLAG_LEGACY                    |
                            LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_GRAIN_MERGE_LEGACY,

    .op_name              = "ligma:grain-merge-legacy",
    .flags                = LIGMA_LAYER_MODE_FLAG_LEGACY                    |
                            LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_COLOR_ERASE_LEGACY,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_color_erase,
    .flags                = LIGMA_LAYER_MODE_FLAG_LEGACY                    |
                            LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE  |
                            LIGMA_LAYER_MODE_FLAG_SUBTRACTIVE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_PAINT |
                            LIGMA_LAYER_MODE_CONTEXT_FILTER,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_OVERLAY,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_overlay,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_LCH_HUE,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_lch_hue,
    .flags                = LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_LAB
  },

  { LIGMA_LAYER_MODE_LCH_CHROMA,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_lch_chroma,
    .flags                = LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_LAB
  },

  { LIGMA_LAYER_MODE_LCH_COLOR,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_lch_color,
    .flags                = LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_LAB
  },

  { LIGMA_LAYER_MODE_LCH_LIGHTNESS,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_lch_lightness,
    .flags                = LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_LAB
  },

  { LIGMA_LAYER_MODE_NORMAL,

    .op_name              = "ligma:normal",
    .flags                = LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE |
                            LIGMA_LAYER_MODE_FLAG_TRIVIAL,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { LIGMA_LAYER_MODE_BEHIND,

    .op_name              = "ligma:behind",
    .flags                = LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_PAINT |
                            LIGMA_LAYER_MODE_CONTEXT_FILTER,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { LIGMA_LAYER_MODE_MULTIPLY,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_multiply,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { LIGMA_LAYER_MODE_SCREEN,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_screen,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_DIFFERENCE,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_difference,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_ADDITION,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_addition,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { LIGMA_LAYER_MODE_SUBTRACT,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_subtract,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { LIGMA_LAYER_MODE_DARKEN_ONLY,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_darken_only,
    .flags                = LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR
    /* no blend_space: reuse composite space, no conversion thus fewer copies */
  },

  { LIGMA_LAYER_MODE_LIGHTEN_ONLY,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_lighten_only,
    .flags                = LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR
    /* no blend_space: reuse composite space, no conversion thus fewer copies */
  },

  { LIGMA_LAYER_MODE_HSV_HUE,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_hsv_hue,
    .flags                = LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_HSV_SATURATION,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_hsv_saturation,
    .flags                = LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_HSL_COLOR,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_hsl_color,
    .flags                = LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_HSV_VALUE,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_hsv_value,
    .flags                = LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_DIVIDE,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_divide,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { LIGMA_LAYER_MODE_DODGE,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_dodge,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_BURN,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_burn,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_HARDLIGHT,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_hardlight,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_SOFTLIGHT,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_softlight,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_GRAIN_EXTRACT,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_grain_extract,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_GRAIN_MERGE,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_grain_merge,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_VIVID_LIGHT,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_vivid_light,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_PIN_LIGHT,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_pin_light,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_LINEAR_LIGHT,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_linear_light,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_HARD_MIX,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_hard_mix,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_EXCLUSION,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_exclusion,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_LINEAR_BURN,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_linear_burn,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_LUMA_DARKEN_ONLY,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_luma_darken_only,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_LUMA_LIGHTEN_ONLY,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_luma_lighten_only,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { LIGMA_LAYER_MODE_LUMINANCE,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_luminance,
    .flags                = LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { LIGMA_LAYER_MODE_COLOR_ERASE,

    .op_name              = "ligma:layer-mode",
    .blend_function       = ligma_operation_layer_mode_blend_color_erase,
    .flags                = LIGMA_LAYER_MODE_FLAG_SUBTRACTIVE,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { LIGMA_LAYER_MODE_ERASE,

    .op_name              = "ligma:erase",
    .flags                = LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE |
                            LIGMA_LAYER_MODE_FLAG_SUBTRACTIVE           |
                            LIGMA_LAYER_MODE_FLAG_ALPHA_ONLY            |
                            LIGMA_LAYER_MODE_FLAG_TRIVIAL,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { LIGMA_LAYER_MODE_MERGE,

    .op_name              = "ligma:merge",
    .flags                = LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE |
                            LIGMA_LAYER_MODE_FLAG_TRIVIAL,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { LIGMA_LAYER_MODE_SPLIT,

    .op_name              = "ligma:split",
    .flags                = LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            LIGMA_LAYER_MODE_FLAG_SUBTRACTIVE               |
                            LIGMA_LAYER_MODE_FLAG_ALPHA_ONLY                |
                            LIGMA_LAYER_MODE_FLAG_TRIVIAL,
    .context              = LIGMA_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP
  },

  { LIGMA_LAYER_MODE_PASS_THROUGH,

    .op_name              = "ligma:pass-through",
    .flags                = LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE    |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE |
                            LIGMA_LAYER_MODE_FLAG_TRIVIAL,
    .context              = LIGMA_LAYER_MODE_CONTEXT_GROUP,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { LIGMA_LAYER_MODE_REPLACE,

    .op_name              = "ligma:replace",
    .flags                = LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE |
                            LIGMA_LAYER_MODE_FLAG_TRIVIAL,
    .context              = LIGMA_LAYER_MODE_CONTEXT_FILTER,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_space      = LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { LIGMA_LAYER_MODE_ANTI_ERASE,

    .op_name              = "ligma:anti-erase",
    .flags                = LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            LIGMA_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            LIGMA_LAYER_MODE_FLAG_ALPHA_ONLY,
    .paint_composite_mode = LIGMA_LAYER_COMPOSITE_UNION,
    .composite_mode       = LIGMA_LAYER_COMPOSITE_UNION
  }
};

static const LigmaLayerMode layer_mode_group_default[] =
{
  LIGMA_LAYER_MODE_PASS_THROUGH,

  LIGMA_LAYER_MODE_SEPARATOR,

  LIGMA_LAYER_MODE_REPLACE,

  LIGMA_LAYER_MODE_SEPARATOR,

  LIGMA_LAYER_MODE_NORMAL,
  LIGMA_LAYER_MODE_DISSOLVE,
  LIGMA_LAYER_MODE_BEHIND,
  LIGMA_LAYER_MODE_COLOR_ERASE,
  LIGMA_LAYER_MODE_ERASE,
  LIGMA_LAYER_MODE_ANTI_ERASE,
  LIGMA_LAYER_MODE_MERGE,
  LIGMA_LAYER_MODE_SPLIT,

  LIGMA_LAYER_MODE_SEPARATOR,

  LIGMA_LAYER_MODE_LIGHTEN_ONLY,
  LIGMA_LAYER_MODE_LUMA_LIGHTEN_ONLY,
  LIGMA_LAYER_MODE_SCREEN,
  LIGMA_LAYER_MODE_DODGE,
  LIGMA_LAYER_MODE_ADDITION,

  LIGMA_LAYER_MODE_SEPARATOR,

  LIGMA_LAYER_MODE_DARKEN_ONLY,
  LIGMA_LAYER_MODE_LUMA_DARKEN_ONLY,
  LIGMA_LAYER_MODE_MULTIPLY,
  LIGMA_LAYER_MODE_BURN,
  LIGMA_LAYER_MODE_LINEAR_BURN,

  LIGMA_LAYER_MODE_SEPARATOR,

  LIGMA_LAYER_MODE_OVERLAY,
  LIGMA_LAYER_MODE_SOFTLIGHT,
  LIGMA_LAYER_MODE_HARDLIGHT,
  LIGMA_LAYER_MODE_VIVID_LIGHT,
  LIGMA_LAYER_MODE_PIN_LIGHT,
  LIGMA_LAYER_MODE_LINEAR_LIGHT,
  LIGMA_LAYER_MODE_HARD_MIX,

  LIGMA_LAYER_MODE_SEPARATOR,

  LIGMA_LAYER_MODE_DIFFERENCE,
  LIGMA_LAYER_MODE_EXCLUSION,
  LIGMA_LAYER_MODE_SUBTRACT,
  LIGMA_LAYER_MODE_GRAIN_EXTRACT,
  LIGMA_LAYER_MODE_GRAIN_MERGE,
  LIGMA_LAYER_MODE_DIVIDE,

  LIGMA_LAYER_MODE_SEPARATOR,

  LIGMA_LAYER_MODE_HSV_HUE,
  LIGMA_LAYER_MODE_HSV_SATURATION,
  LIGMA_LAYER_MODE_HSL_COLOR,
  LIGMA_LAYER_MODE_HSV_VALUE,

  LIGMA_LAYER_MODE_SEPARATOR,

  LIGMA_LAYER_MODE_LCH_HUE,
  LIGMA_LAYER_MODE_LCH_CHROMA,
  LIGMA_LAYER_MODE_LCH_COLOR,
  LIGMA_LAYER_MODE_LCH_LIGHTNESS,
  LIGMA_LAYER_MODE_LUMINANCE
};

static const LigmaLayerMode layer_mode_group_legacy[] =
{
  LIGMA_LAYER_MODE_NORMAL_LEGACY,
  LIGMA_LAYER_MODE_DISSOLVE,
  LIGMA_LAYER_MODE_BEHIND_LEGACY,
  LIGMA_LAYER_MODE_COLOR_ERASE_LEGACY,

  LIGMA_LAYER_MODE_SEPARATOR,

  LIGMA_LAYER_MODE_LIGHTEN_ONLY_LEGACY,
  LIGMA_LAYER_MODE_SCREEN_LEGACY,
  LIGMA_LAYER_MODE_DODGE_LEGACY,
  LIGMA_LAYER_MODE_ADDITION_LEGACY,

  LIGMA_LAYER_MODE_SEPARATOR,

  LIGMA_LAYER_MODE_DARKEN_ONLY_LEGACY,
  LIGMA_LAYER_MODE_MULTIPLY_LEGACY,
  LIGMA_LAYER_MODE_BURN_LEGACY,

  LIGMA_LAYER_MODE_SEPARATOR,

  LIGMA_LAYER_MODE_OVERLAY,
  LIGMA_LAYER_MODE_SOFTLIGHT_LEGACY,
  LIGMA_LAYER_MODE_HARDLIGHT_LEGACY,

  LIGMA_LAYER_MODE_SEPARATOR,

  LIGMA_LAYER_MODE_DIFFERENCE_LEGACY,
  LIGMA_LAYER_MODE_SUBTRACT_LEGACY,
  LIGMA_LAYER_MODE_GRAIN_EXTRACT_LEGACY,
  LIGMA_LAYER_MODE_GRAIN_MERGE_LEGACY,
  LIGMA_LAYER_MODE_DIVIDE_LEGACY,

  LIGMA_LAYER_MODE_SEPARATOR,

  LIGMA_LAYER_MODE_HSV_HUE_LEGACY,
  LIGMA_LAYER_MODE_HSV_SATURATION_LEGACY,
  LIGMA_LAYER_MODE_HSL_COLOR_LEGACY,
  LIGMA_LAYER_MODE_HSV_VALUE_LEGACY
};

static const LigmaLayerMode layer_mode_groups[][2] =
{
  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_NORMAL,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = LIGMA_LAYER_MODE_NORMAL_LEGACY
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_DISSOLVE,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = LIGMA_LAYER_MODE_DISSOLVE
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_BEHIND,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = LIGMA_LAYER_MODE_BEHIND_LEGACY
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_MULTIPLY,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = LIGMA_LAYER_MODE_MULTIPLY_LEGACY
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_SCREEN,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = LIGMA_LAYER_MODE_SCREEN_LEGACY
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_OVERLAY,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_DIFFERENCE,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = LIGMA_LAYER_MODE_DIFFERENCE_LEGACY
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_ADDITION,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = LIGMA_LAYER_MODE_ADDITION_LEGACY
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_SUBTRACT,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = LIGMA_LAYER_MODE_SUBTRACT_LEGACY
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_DARKEN_ONLY,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = LIGMA_LAYER_MODE_DARKEN_ONLY_LEGACY
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_LIGHTEN_ONLY,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = LIGMA_LAYER_MODE_LIGHTEN_ONLY_LEGACY
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_HSV_HUE,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = LIGMA_LAYER_MODE_HSV_HUE_LEGACY
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_HSV_SATURATION,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = LIGMA_LAYER_MODE_HSV_SATURATION_LEGACY
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_HSL_COLOR,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = LIGMA_LAYER_MODE_HSL_COLOR_LEGACY
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_HSV_VALUE,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = LIGMA_LAYER_MODE_HSV_VALUE_LEGACY
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_DIVIDE,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = LIGMA_LAYER_MODE_DIVIDE_LEGACY
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_DODGE,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = LIGMA_LAYER_MODE_DODGE_LEGACY
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_BURN,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = LIGMA_LAYER_MODE_BURN_LEGACY
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_HARDLIGHT,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = LIGMA_LAYER_MODE_HARDLIGHT_LEGACY
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_SOFTLIGHT,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = LIGMA_LAYER_MODE_SOFTLIGHT_LEGACY
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_GRAIN_EXTRACT,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = LIGMA_LAYER_MODE_GRAIN_EXTRACT_LEGACY
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_GRAIN_MERGE,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = LIGMA_LAYER_MODE_GRAIN_MERGE_LEGACY
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_COLOR_ERASE,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = LIGMA_LAYER_MODE_COLOR_ERASE_LEGACY,
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_VIVID_LIGHT,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_PIN_LIGHT,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_LINEAR_LIGHT,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_HARD_MIX,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_EXCLUSION,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_LINEAR_BURN,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_LUMA_DARKEN_ONLY,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_LUMA_LIGHTEN_ONLY,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_LUMINANCE,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_ERASE,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_MERGE,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_SPLIT,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_PASS_THROUGH,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = -1,
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_REPLACE,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [LIGMA_LAYER_MODE_GROUP_DEFAULT] = LIGMA_LAYER_MODE_ANTI_ERASE,
    [LIGMA_LAYER_MODE_GROUP_LEGACY ] = -1
  }
};

static GeglOperation *ops[G_N_ELEMENTS (layer_mode_infos)] = { 0 };

/*  public functions  */

void
ligma_layer_modes_init (void)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS (layer_mode_infos); i++)
    {
      ligma_assert ((LigmaLayerMode) i == layer_mode_infos[i].layer_mode);
    }
}

void
ligma_layer_modes_exit (void)
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

static const LigmaLayerModeInfo *
ligma_layer_mode_info (LigmaLayerMode mode)
{
  g_return_val_if_fail (mode >= 0 && mode < G_N_ELEMENTS (layer_mode_infos),
                        &layer_mode_infos[0]);

  return &layer_mode_infos[mode];
}

gboolean
ligma_layer_mode_is_legacy (LigmaLayerMode mode)
{
  const LigmaLayerModeInfo *info = ligma_layer_mode_info (mode);

  if (! info)
    return FALSE;

  return (info->flags & LIGMA_LAYER_MODE_FLAG_LEGACY) != 0;
}

gboolean
ligma_layer_mode_is_blend_space_mutable (LigmaLayerMode mode)
{
  const LigmaLayerModeInfo *info = ligma_layer_mode_info (mode);

  if (! info)
    return FALSE;

  return (info->flags & LIGMA_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE) == 0;
}

gboolean
ligma_layer_mode_is_composite_space_mutable (LigmaLayerMode mode)
{
  const LigmaLayerModeInfo *info = ligma_layer_mode_info (mode);

  if (! info)
    return FALSE;

  return (info->flags & LIGMA_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE) == 0;
}

gboolean
ligma_layer_mode_is_composite_mode_mutable (LigmaLayerMode mode)
{
  const LigmaLayerModeInfo *info = ligma_layer_mode_info (mode);

  if (! info)
    return FALSE;

  return (info->flags & LIGMA_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE) == 0;
}

gboolean
ligma_layer_mode_is_subtractive (LigmaLayerMode mode)
{
  const LigmaLayerModeInfo *info = ligma_layer_mode_info (mode);

  if (! info)
    return FALSE;

  return (info->flags & LIGMA_LAYER_MODE_FLAG_SUBTRACTIVE) != 0;
}

gboolean
ligma_layer_mode_is_alpha_only (LigmaLayerMode mode)
{
  const LigmaLayerModeInfo *info = ligma_layer_mode_info (mode);

  if (! info)
    return FALSE;

  return (info->flags & LIGMA_LAYER_MODE_FLAG_ALPHA_ONLY) != 0;
}

gboolean
ligma_layer_mode_is_trivial (LigmaLayerMode mode)
{
  const LigmaLayerModeInfo *info = ligma_layer_mode_info (mode);

  if (! info)
    return FALSE;

  return (info->flags & LIGMA_LAYER_MODE_FLAG_TRIVIAL) != 0;
}

LigmaLayerColorSpace
ligma_layer_mode_get_blend_space (LigmaLayerMode mode)
{
  const LigmaLayerModeInfo *info = ligma_layer_mode_info (mode);

  if (! info)
    return LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR;

  return info->blend_space;
}

LigmaLayerColorSpace
ligma_layer_mode_get_composite_space (LigmaLayerMode mode)
{
  const LigmaLayerModeInfo *info = ligma_layer_mode_info (mode);

  if (! info)
    return LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR;

  return info->composite_space;
}

LigmaLayerCompositeMode
ligma_layer_mode_get_composite_mode (LigmaLayerMode mode)
{
  const LigmaLayerModeInfo *info = ligma_layer_mode_info (mode);

  if (! info)
    return LIGMA_LAYER_COMPOSITE_UNION;

  return info->composite_mode;
}

LigmaLayerCompositeMode
ligma_layer_mode_get_paint_composite_mode (LigmaLayerMode mode)
{
  const LigmaLayerModeInfo *info = ligma_layer_mode_info (mode);

  if (! info)
    return LIGMA_LAYER_COMPOSITE_UNION;

  return info->paint_composite_mode;
}

const gchar *
ligma_layer_mode_get_operation_name (LigmaLayerMode mode)
{
  const LigmaLayerModeInfo *info = ligma_layer_mode_info (mode);

  if (! info)
    return "ligma:layer-mode";

  return info->op_name;
}

/**
 * ligma_layer_mode_get_operation:
 * @mode:
 *
 * Returns: a #GeglOperation for @mode which may be reused and must not
 *          be freed.
 */
GeglOperation *
ligma_layer_mode_get_operation (LigmaLayerMode mode)
{
  const LigmaLayerModeInfo *info = ligma_layer_mode_info (mode);
  const gchar             *op_name;

  op_name = ligma_layer_mode_get_operation_name (mode);

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

      if (LIGMA_IS_OPERATION_LAYER_MODE (operation))
        {
          LigmaOperationLayerMode *layer_mode = LIGMA_OPERATION_LAYER_MODE (operation);

          layer_mode->layer_mode      = mode;
          layer_mode->function        = LIGMA_OPERATION_LAYER_MODE_GET_CLASS (operation)->process;
          layer_mode->blend_function  = ligma_layer_mode_get_blend_function (mode);
          layer_mode->blend_space     = ligma_layer_mode_get_blend_space (mode);
          layer_mode->composite_space = ligma_layer_mode_get_composite_space (mode);
          layer_mode->composite_mode  = ligma_layer_mode_get_paint_composite_mode (mode);
        }
    }

  return ops[mode];
}

LigmaLayerModeFunc
ligma_layer_mode_get_function (LigmaLayerMode mode)
{
  GeglOperation *operation;

  operation = ligma_layer_mode_get_operation (mode);

  return LIGMA_OPERATION_LAYER_MODE_GET_CLASS (operation)->process;
}

LigmaLayerModeBlendFunc
ligma_layer_mode_get_blend_function (LigmaLayerMode mode)
{
  const LigmaLayerModeInfo *info = ligma_layer_mode_info (mode);

  if (! info)
    return NULL;

  return info->blend_function;
}

LigmaLayerModeContext
ligma_layer_mode_get_context (LigmaLayerMode mode)
{
  const LigmaLayerModeInfo *info = ligma_layer_mode_info (mode);

  if (! info)
    return 0;

  return info->context;
}

LigmaLayerMode *
ligma_layer_mode_get_context_array (LigmaLayerMode         mode,
                                   LigmaLayerModeContext  context,
                                   gint                 *n_modes)
{
  LigmaLayerModeGroup   group;
  const LigmaLayerMode *group_modes;
  gint                 n_group_modes;
  LigmaLayerMode       *array;
  gint                 i;

  group = ligma_layer_mode_get_group (mode);

  group_modes = ligma_layer_mode_get_group_array (group, &n_group_modes);

  array = g_new0 (LigmaLayerMode, n_group_modes);
  *n_modes = 0;

  for (i = 0; i < n_group_modes; i++)
    {
      if (group_modes[i] != LIGMA_LAYER_MODE_SEPARATOR &&
          (ligma_layer_mode_get_context (group_modes[i]) & context))
        {
          array[*n_modes] = group_modes[i];
          (*n_modes)++;
        }
    }

  return array;
}

static gboolean
is_mode_in_array (const LigmaLayerMode *modes,
                  gint                 n_modes,
                  LigmaLayerMode        mode)
{
  gint i;

  for (i = 0; i < n_modes; i++)
    {
      if (modes[i] == mode)
        return TRUE;
    }

  return FALSE;
}

LigmaLayerModeGroup
ligma_layer_mode_get_group (LigmaLayerMode mode)
{
  if (is_mode_in_array (layer_mode_group_default,
                        G_N_ELEMENTS (layer_mode_group_default), mode))
    {
      return LIGMA_LAYER_MODE_GROUP_DEFAULT;
    }
  else if (is_mode_in_array (layer_mode_group_legacy,
                             G_N_ELEMENTS (layer_mode_group_legacy), mode))
    {
      return LIGMA_LAYER_MODE_GROUP_LEGACY;
    }

  return LIGMA_LAYER_MODE_GROUP_DEFAULT;
}

const LigmaLayerMode *
ligma_layer_mode_get_group_array (LigmaLayerModeGroup  group,
                                 gint               *n_modes)
{
  g_return_val_if_fail (n_modes != NULL, NULL);

  switch (group)
    {
    case LIGMA_LAYER_MODE_GROUP_DEFAULT:
      *n_modes = G_N_ELEMENTS (layer_mode_group_default);
      return layer_mode_group_default;

    case LIGMA_LAYER_MODE_GROUP_LEGACY:
      *n_modes = G_N_ELEMENTS (layer_mode_group_legacy);
      return layer_mode_group_legacy;

    default:
      g_return_val_if_reached (NULL);
    }
}

gboolean
ligma_layer_mode_get_for_group (LigmaLayerMode       old_mode,
                               LigmaLayerModeGroup  new_group,
                               LigmaLayerMode      *new_mode)
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
ligma_layer_mode_get_format (LigmaLayerMode           mode,
                            LigmaLayerColorSpace     blend_space,
                            LigmaLayerColorSpace     composite_space,
                            LigmaLayerCompositeMode  composite_mode,
                            const Babl             *preferred_format)
{
  LigmaLayerCompositeRegion composite_region;

  /* for now, all modes perform i/o in the composite space. */
  (void) mode;
  (void) blend_space;

  if (composite_space == LIGMA_LAYER_COLOR_SPACE_AUTO)
    composite_space = ligma_layer_mode_get_composite_space (mode);

  if (composite_mode == LIGMA_LAYER_COMPOSITE_AUTO)
    composite_mode = ligma_layer_mode_get_composite_mode (mode);

  composite_region = ligma_layer_mode_get_included_region (mode, composite_mode);

  if (ligma_layer_mode_is_alpha_only (mode))
    {
      if (composite_region != LIGMA_LAYER_COMPOSITE_REGION_UNION)
        {
          /* alpha-only layer modes don't combine colors in non-union composite
           * modes, hence we can disregard the composite space.
           */
          composite_space = LIGMA_LAYER_COLOR_SPACE_AUTO;
        }
    }
  else if (ligma_layer_mode_is_trivial (mode))
    {
      if (! (composite_region & LIGMA_LAYER_COMPOSITE_REGION_DESTINATION))
        {
          /* trivial layer modes don't combine colors when only the source
           * region is included, hence we can disregard the composite space.
           */
          composite_space = LIGMA_LAYER_COLOR_SPACE_AUTO;
        }
    }

  switch (composite_space)
    {
    case LIGMA_LAYER_COLOR_SPACE_AUTO:
      /* compositing is color-space agnostic.  return a format that has a fast
       * conversion path to/from the preferred format.
       */
      if (! preferred_format ||
          ligma_babl_format_get_trc (preferred_format) == LIGMA_TRC_LINEAR)
        return babl_format_with_space ("RGBA float", preferred_format);
      else
        return babl_format_with_space ("R'G'B'A float", preferred_format);

    case LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR:
      return babl_format_with_space ("RGBA float", preferred_format);

    case LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL:
      return babl_format_with_space ("R'G'B'A float", preferred_format);

    case LIGMA_LAYER_COLOR_SPACE_LAB:
      return babl_format_with_space ("CIE Lab alpha float", preferred_format);
    }

  g_return_val_if_reached (babl_format_with_space ("RGBA float", preferred_format));
}

LigmaLayerCompositeRegion
ligma_layer_mode_get_included_region (LigmaLayerMode          mode,
                                     LigmaLayerCompositeMode composite_mode)
{
  if (composite_mode == LIGMA_LAYER_COMPOSITE_AUTO)
    composite_mode = ligma_layer_mode_get_composite_mode (mode);

  switch (composite_mode)
    {
    case LIGMA_LAYER_COMPOSITE_UNION:
      return LIGMA_LAYER_COMPOSITE_REGION_UNION;

    case LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP:
      return LIGMA_LAYER_COMPOSITE_REGION_DESTINATION;

    case LIGMA_LAYER_COMPOSITE_CLIP_TO_LAYER:
      return LIGMA_LAYER_COMPOSITE_REGION_SOURCE;

    case LIGMA_LAYER_COMPOSITE_INTERSECTION:
      return LIGMA_LAYER_COMPOSITE_REGION_INTERSECTION;

    default:
      g_return_val_if_reached (LIGMA_LAYER_COMPOSITE_REGION_INTERSECTION);
    }
}
