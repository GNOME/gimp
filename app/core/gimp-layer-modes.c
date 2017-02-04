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

#include "core-types.h"

#include "gimp-layer-modes.h"

#include "operations/layer-modes/gimpoperationlayermode.h"


/*  static variables  */

static const GimpLayerMode layer_mode_group_default[] =
{
  GIMP_LAYER_MODE_NORMAL,
  GIMP_LAYER_MODE_DISSOLVE,

  GIMP_LAYER_MODE_LIGHTEN_ONLY,
  GIMP_LAYER_MODE_LUMINANCE_LIGHTEN_ONLY,
  GIMP_LAYER_MODE_SCREEN,
  GIMP_LAYER_MODE_DODGE,
  GIMP_LAYER_MODE_ADDITION,

  GIMP_LAYER_MODE_DARKEN_ONLY,
  GIMP_LAYER_MODE_LUMINANCE_DARKEN_ONLY,
  GIMP_LAYER_MODE_MULTIPLY,
  GIMP_LAYER_MODE_BURN,

  GIMP_LAYER_MODE_OVERLAY,
  GIMP_LAYER_MODE_SOFTLIGHT,
  GIMP_LAYER_MODE_HARDLIGHT,
  GIMP_LAYER_MODE_VIVID_LIGHT,
  GIMP_LAYER_MODE_PIN_LIGHT,
  GIMP_LAYER_MODE_LINEAR_LIGHT,
  GIMP_LAYER_MODE_HARD_MIX,

  GIMP_LAYER_MODE_DIFFERENCE,
  GIMP_LAYER_MODE_SUBTRACT,
  GIMP_LAYER_MODE_GRAIN_EXTRACT,
  GIMP_LAYER_MODE_GRAIN_MERGE,
  GIMP_LAYER_MODE_DIVIDE,

  GIMP_LAYER_MODE_LCH_HUE,
  GIMP_LAYER_MODE_LCH_CHROMA,
  GIMP_LAYER_MODE_LCH_COLOR,
  GIMP_LAYER_MODE_LCH_LIGHTNESS,

  GIMP_LAYER_MODE_EXCLUSION,
  GIMP_LAYER_MODE_LINEAR_BURN
};

static const GimpLayerMode layer_mode_group_linear[] =
{
  GIMP_LAYER_MODE_NORMAL_LINEAR,
  GIMP_LAYER_MODE_DISSOLVE,

  GIMP_LAYER_MODE_LIGHTEN_ONLY,
  GIMP_LAYER_MODE_LUMINANCE_LIGHTEN_ONLY,
  GIMP_LAYER_MODE_SCREEN_LINEAR,
  GIMP_LAYER_MODE_DODGE_LINEAR,
  GIMP_LAYER_MODE_ADDITION_LINEAR,

  GIMP_LAYER_MODE_DARKEN_ONLY,
  GIMP_LAYER_MODE_LUMINANCE_DARKEN_ONLY,
  GIMP_LAYER_MODE_MULTIPLY_LINEAR,
  GIMP_LAYER_MODE_BURN_LINEAR,

  GIMP_LAYER_MODE_OVERLAY_LINEAR,
  GIMP_LAYER_MODE_SOFTLIGHT_LINEAR,
  GIMP_LAYER_MODE_HARDLIGHT_LINEAR,
  GIMP_LAYER_MODE_VIVID_LIGHT_LINEAR,
  GIMP_LAYER_MODE_PIN_LIGHT_LINEAR,
  GIMP_LAYER_MODE_LINEAR_LIGHT_LINEAR,
  GIMP_LAYER_MODE_HARD_MIX_LINEAR,

  GIMP_LAYER_MODE_DIFFERENCE_LINEAR,
  GIMP_LAYER_MODE_SUBTRACT_LINEAR,
  GIMP_LAYER_MODE_GRAIN_EXTRACT_LINEAR,
  GIMP_LAYER_MODE_GRAIN_MERGE_LINEAR,
  GIMP_LAYER_MODE_DIVIDE_LINEAR,

  GIMP_LAYER_MODE_EXCLUSION_LINEAR,
  GIMP_LAYER_MODE_LINEAR_BURN_LINEAR
};

static const GimpLayerMode layer_mode_group_perceptual[] =
{
  GIMP_LAYER_MODE_NORMAL,
  GIMP_LAYER_MODE_DISSOLVE,

  GIMP_LAYER_MODE_LIGHTEN_ONLY,
  GIMP_LAYER_MODE_LUMA_LIGHTEN_ONLY,
  GIMP_LAYER_MODE_SCREEN,
  GIMP_LAYER_MODE_DODGE,
  GIMP_LAYER_MODE_ADDITION,

  GIMP_LAYER_MODE_DARKEN_ONLY,
  GIMP_LAYER_MODE_LUMA_DARKEN_ONLY,
  GIMP_LAYER_MODE_MULTIPLY,
  GIMP_LAYER_MODE_BURN,

  GIMP_LAYER_MODE_OVERLAY,
  GIMP_LAYER_MODE_SOFTLIGHT,
  GIMP_LAYER_MODE_HARDLIGHT,
  GIMP_LAYER_MODE_VIVID_LIGHT,
  GIMP_LAYER_MODE_PIN_LIGHT,
  GIMP_LAYER_MODE_LINEAR_LIGHT,
  GIMP_LAYER_MODE_HARD_MIX,

  GIMP_LAYER_MODE_DIFFERENCE,
  GIMP_LAYER_MODE_SUBTRACT,
  GIMP_LAYER_MODE_GRAIN_EXTRACT,
  GIMP_LAYER_MODE_GRAIN_MERGE,
  GIMP_LAYER_MODE_DIVIDE,

  GIMP_LAYER_MODE_HSV_HUE,
  GIMP_LAYER_MODE_HSV_SATURATION,
  GIMP_LAYER_MODE_HSV_COLOR,
  GIMP_LAYER_MODE_HSV_VALUE,

  GIMP_LAYER_MODE_LCH_HUE,
  GIMP_LAYER_MODE_LCH_CHROMA,
  GIMP_LAYER_MODE_LCH_COLOR,
  GIMP_LAYER_MODE_LCH_LIGHTNESS,

  GIMP_LAYER_MODE_EXCLUSION,
  GIMP_LAYER_MODE_LINEAR_BURN
};

static const GimpLayerMode layer_mode_group_legacy[] =
{
  GIMP_LAYER_MODE_NORMAL,
  GIMP_LAYER_MODE_DISSOLVE,

  GIMP_LAYER_MODE_LIGHTEN_ONLY_LEGACY,
  GIMP_LAYER_MODE_SCREEN_LEGACY,
  GIMP_LAYER_MODE_DODGE_LEGACY,
  GIMP_LAYER_MODE_ADDITION_LEGACY,

  GIMP_LAYER_MODE_DARKEN_ONLY_LEGACY,
  GIMP_LAYER_MODE_MULTIPLY_LEGACY,
  GIMP_LAYER_MODE_BURN_LEGACY,

  GIMP_LAYER_MODE_SOFTLIGHT_LEGACY,
  GIMP_LAYER_MODE_HARDLIGHT_LEGACY,

  GIMP_LAYER_MODE_DIFFERENCE_LEGACY,
  GIMP_LAYER_MODE_SUBTRACT_LEGACY,
  GIMP_LAYER_MODE_GRAIN_EXTRACT_LEGACY,
  GIMP_LAYER_MODE_GRAIN_MERGE_LEGACY,
  GIMP_LAYER_MODE_DIVIDE_LEGACY,

  GIMP_LAYER_MODE_HSV_HUE_LEGACY,
  GIMP_LAYER_MODE_HSV_SATURATION_LEGACY,
  GIMP_LAYER_MODE_HSV_COLOR_LEGACY,
  GIMP_LAYER_MODE_HSV_VALUE_LEGACY
};

static const GimpLayerMode layer_mode_groups[][4] =
{
  {
    GIMP_LAYER_MODE_NORMAL,
    GIMP_LAYER_MODE_NORMAL_LINEAR,
    GIMP_LAYER_MODE_NORMAL,
    GIMP_LAYER_MODE_NORMAL
  },

  {
    GIMP_LAYER_MODE_DISSOLVE,
    GIMP_LAYER_MODE_DISSOLVE,
    GIMP_LAYER_MODE_DISSOLVE,
    GIMP_LAYER_MODE_DISSOLVE
  },

  {
    GIMP_LAYER_MODE_BEHIND,
    GIMP_LAYER_MODE_BEHIND_LINEAR,
    GIMP_LAYER_MODE_BEHIND,
    GIMP_LAYER_MODE_BEHIND
  },

  {
    GIMP_LAYER_MODE_MULTIPLY,
    GIMP_LAYER_MODE_MULTIPLY_LINEAR,
    GIMP_LAYER_MODE_MULTIPLY,
    GIMP_LAYER_MODE_MULTIPLY_LEGACY
  },

  {
    GIMP_LAYER_MODE_SCREEN,
    GIMP_LAYER_MODE_SCREEN_LINEAR,
    GIMP_LAYER_MODE_SCREEN,
    GIMP_LAYER_MODE_SCREEN_LEGACY
  },

  {
    GIMP_LAYER_MODE_OVERLAY,
    GIMP_LAYER_MODE_OVERLAY_LINEAR,
    GIMP_LAYER_MODE_OVERLAY,
    -1
  },

  {
    GIMP_LAYER_MODE_DIFFERENCE,
    GIMP_LAYER_MODE_DIFFERENCE_LINEAR,
    GIMP_LAYER_MODE_DIFFERENCE,
    GIMP_LAYER_MODE_DIFFERENCE_LEGACY
  },

  {
    GIMP_LAYER_MODE_ADDITION,
    GIMP_LAYER_MODE_ADDITION_LINEAR,
    GIMP_LAYER_MODE_ADDITION,
    GIMP_LAYER_MODE_ADDITION_LEGACY
  },

  {
    GIMP_LAYER_MODE_SUBTRACT,
    GIMP_LAYER_MODE_SUBTRACT_LINEAR,
    GIMP_LAYER_MODE_SUBTRACT,
    GIMP_LAYER_MODE_SUBTRACT_LEGACY
  },

  {
    GIMP_LAYER_MODE_DARKEN_ONLY,
    GIMP_LAYER_MODE_DARKEN_ONLY,
    GIMP_LAYER_MODE_DARKEN_ONLY,
    GIMP_LAYER_MODE_DARKEN_ONLY_LEGACY
  },

  {
    GIMP_LAYER_MODE_LIGHTEN_ONLY,
    GIMP_LAYER_MODE_LIGHTEN_ONLY,
    GIMP_LAYER_MODE_LIGHTEN_ONLY,
    GIMP_LAYER_MODE_LIGHTEN_ONLY_LEGACY
  },

  {
    -1,
    -1,
    GIMP_LAYER_MODE_HSV_HUE,
    GIMP_LAYER_MODE_HSV_HUE_LEGACY
  },

  {
    -1,
    -1,
    GIMP_LAYER_MODE_HSV_SATURATION,
    GIMP_LAYER_MODE_HSV_SATURATION_LEGACY
  },

  {
    -1,
    -1,
    GIMP_LAYER_MODE_HSV_COLOR,
    GIMP_LAYER_MODE_HSV_COLOR_LEGACY
  },

  {
    -1,
    -1,
    GIMP_LAYER_MODE_HSV_VALUE,
    GIMP_LAYER_MODE_HSV_VALUE_LEGACY
  },

  {
    GIMP_LAYER_MODE_DIVIDE,
    GIMP_LAYER_MODE_DIVIDE_LINEAR,
    GIMP_LAYER_MODE_DIVIDE,
    GIMP_LAYER_MODE_DIVIDE_LEGACY
  },

  {
    GIMP_LAYER_MODE_DODGE,
    GIMP_LAYER_MODE_DODGE_LINEAR,
    GIMP_LAYER_MODE_DODGE,
    GIMP_LAYER_MODE_DODGE_LEGACY
  },

  {
    GIMP_LAYER_MODE_BURN,
    GIMP_LAYER_MODE_BURN_LINEAR,
    GIMP_LAYER_MODE_BURN,
    GIMP_LAYER_MODE_BURN_LEGACY
  },

  {
    GIMP_LAYER_MODE_HARDLIGHT,
    GIMP_LAYER_MODE_HARDLIGHT_LINEAR,
    GIMP_LAYER_MODE_HARDLIGHT,
    GIMP_LAYER_MODE_HARDLIGHT_LEGACY
  },

  {
    GIMP_LAYER_MODE_SOFTLIGHT,
    GIMP_LAYER_MODE_SOFTLIGHT_LINEAR,
    GIMP_LAYER_MODE_SOFTLIGHT,
    GIMP_LAYER_MODE_SOFTLIGHT_LEGACY
  },

  {
    GIMP_LAYER_MODE_GRAIN_EXTRACT,
    GIMP_LAYER_MODE_GRAIN_EXTRACT_LINEAR,
    GIMP_LAYER_MODE_GRAIN_EXTRACT,
    GIMP_LAYER_MODE_GRAIN_EXTRACT_LEGACY
  },

  {
    GIMP_LAYER_MODE_GRAIN_MERGE,
    GIMP_LAYER_MODE_GRAIN_MERGE_LINEAR,
    GIMP_LAYER_MODE_GRAIN_MERGE,
    GIMP_LAYER_MODE_GRAIN_MERGE_LEGACY
  },

  {
    GIMP_LAYER_MODE_COLOR_ERASE,
    -1,
    GIMP_LAYER_MODE_COLOR_ERASE,
    -1,
  },

  {
    GIMP_LAYER_MODE_VIVID_LIGHT,
    GIMP_LAYER_MODE_VIVID_LIGHT_LINEAR,
    GIMP_LAYER_MODE_VIVID_LIGHT,
    -1
  },

  {
    GIMP_LAYER_MODE_PIN_LIGHT,
    GIMP_LAYER_MODE_PIN_LIGHT_LINEAR,
    GIMP_LAYER_MODE_PIN_LIGHT,
    -1
  },

  {
    GIMP_LAYER_MODE_LINEAR_LIGHT,
    GIMP_LAYER_MODE_LINEAR_LIGHT_LINEAR,
    GIMP_LAYER_MODE_LINEAR_LIGHT,
    -1
  },

  {
    GIMP_LAYER_MODE_HARD_MIX,
    GIMP_LAYER_MODE_HARD_MIX_LINEAR,
    GIMP_LAYER_MODE_HARD_MIX,
    -1
  },

  {
    GIMP_LAYER_MODE_EXCLUSION,
    GIMP_LAYER_MODE_EXCLUSION_LINEAR,
    GIMP_LAYER_MODE_EXCLUSION,
    -1
  },

  {
    GIMP_LAYER_MODE_LINEAR_BURN,
    GIMP_LAYER_MODE_LINEAR_BURN_LINEAR,
    GIMP_LAYER_MODE_LINEAR_BURN,
    -1
  },

  {
    GIMP_LAYER_MODE_LUMINANCE_DARKEN_ONLY,
    GIMP_LAYER_MODE_LUMINANCE_DARKEN_ONLY,
    GIMP_LAYER_MODE_LUMA_DARKEN_ONLY,
    -1
  },

  {
    GIMP_LAYER_MODE_LUMINANCE_LIGHTEN_ONLY,
    GIMP_LAYER_MODE_LUMINANCE_LIGHTEN_ONLY,
    GIMP_LAYER_MODE_LUMA_LIGHTEN_ONLY,
    -1
  },

  {
    GIMP_LAYER_MODE_ERASE,
    GIMP_LAYER_MODE_ERASE,
    -1,
    -1
  },

  {
    GIMP_LAYER_MODE_REPLACE,
    GIMP_LAYER_MODE_REPLACE,
    -1,
    -1
  },

  {
    GIMP_LAYER_MODE_ANTI_ERASE,
    GIMP_LAYER_MODE_ANTI_ERASE,
    -1,
    -1
  },
};


/*  public functions  */

gboolean
gimp_layer_mode_is_legacy (GimpLayerMode  mode)
{
  const GimpLayerModeInfo *info = gimp_layer_mode_info (mode);
  if (!info)
    return FALSE;
  return (info->flags & GIMP_LAYER_MODE_FLAG_LEGACY) != 0;
}

gboolean
gimp_layer_mode_wants_linear_data (GimpLayerMode  mode)
{
  const GimpLayerModeInfo *info = gimp_layer_mode_info (mode);
  if (!info)
    return FALSE;
  return (info->flags & GIMP_LAYER_MODE_FLAG_WANTS_LINEAR_DATA) != 0;
}

GimpLayerColorSpace
gimp_layer_mode_get_blend_space (GimpLayerMode  mode)
{
  const GimpLayerModeInfo *info = gimp_layer_mode_info (mode);
  if (!info)
    return GIMP_LAYER_COLOR_SPACE_RGB_LINEAR;
  return info->blend_space;
}

GimpLayerColorSpace
gimp_layer_mode_get_composite_space (GimpLayerMode  mode)
{
  const GimpLayerModeInfo *info = gimp_layer_mode_info (mode);
  if (!info)
    return GIMP_LAYER_COLOR_SPACE_RGB_LINEAR;
  return info->composite_space;
}

GimpLayerCompositeMode
gimp_layer_mode_get_composite_mode (GimpLayerMode  mode)
{
  const GimpLayerModeInfo *info = gimp_layer_mode_info (mode);
  if (!info)
    return GIMP_LAYER_COMPOSITE_SRC_OVER;
  return info->composite_mode;
}

const gchar *
gimp_layer_mode_get_operation (GimpLayerMode  mode)
{
  const GimpLayerModeInfo *info = gimp_layer_mode_info (mode);
  if (!info)
    return "gimp:layer-mode";
  return info->op_name;
}

static gboolean
is_mode_in_array (const GimpLayerMode *modes,
                  gint                 n_modes,
                  GimpLayerMode        mode)
{
  gint i;

  for (i = 0; i < n_modes; i++)
    if (modes[i] == mode)
      return TRUE;

  return FALSE;
}

GimpLayerModeGroup
gimp_layer_mode_get_group (GimpLayerMode  mode)
{
  if (is_mode_in_array (layer_mode_group_default,
                        G_N_ELEMENTS (layer_mode_group_default), mode))
    {
      return GIMP_LAYER_MODE_GROUP_DEFAULT;
    }
  else if (is_mode_in_array (layer_mode_group_linear,
                             G_N_ELEMENTS (layer_mode_group_linear), mode))
    {
      return GIMP_LAYER_MODE_GROUP_LINEAR;
    }
  else if (is_mode_in_array (layer_mode_group_perceptual,
                             G_N_ELEMENTS (layer_mode_group_perceptual), mode))
    {
      return GIMP_LAYER_MODE_GROUP_PERCEPTUAL;
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

    case GIMP_LAYER_MODE_GROUP_LINEAR:
      *n_modes = G_N_ELEMENTS (layer_mode_group_linear);
      return layer_mode_group_linear;

    case GIMP_LAYER_MODE_GROUP_PERCEPTUAL:
      *n_modes = G_N_ELEMENTS (layer_mode_group_perceptual);
      return layer_mode_group_perceptual;

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
      if (is_mode_in_array (layer_mode_groups[i], 4, old_mode))
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
