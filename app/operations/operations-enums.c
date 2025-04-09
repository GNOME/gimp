
/* Generated data (by gimp-mkenums) */

#include "stamp-operations-enums.h"
#include "config.h"
#include <gio/gio.h>
#include "libgimpbase/gimpbase.h"
#include "operations-enums.h"
#include "gimp-intl.h"

/* enumerations from "operations-enums.h" */
GType
gimp_layer_color_space_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_LAYER_COLOR_SPACE_AUTO, "GIMP_LAYER_COLOR_SPACE_AUTO", "auto" },
    { GIMP_LAYER_COLOR_SPACE_RGB_LINEAR, "GIMP_LAYER_COLOR_SPACE_RGB_LINEAR", "rgb-linear" },
    { GIMP_LAYER_COLOR_SPACE_RGB_NON_LINEAR, "GIMP_LAYER_COLOR_SPACE_RGB_NON_LINEAR", "rgb-non-linear" },
    { GIMP_LAYER_COLOR_SPACE_LAB, "GIMP_LAYER_COLOR_SPACE_LAB", "lab" },
    { GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL, "GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL", "rgb-perceptual" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_LAYER_COLOR_SPACE_AUTO, NC_("layer-color-space", "Auto"), NULL },
    { GIMP_LAYER_COLOR_SPACE_RGB_LINEAR, NC_("layer-color-space", "RGB (linear)"), NULL },
    { GIMP_LAYER_COLOR_SPACE_RGB_NON_LINEAR, NC_("layer-color-space", "RGB (from color profile)"), NULL },
    { GIMP_LAYER_COLOR_SPACE_LAB, NC_("layer-color-space", "LAB"), NULL },
    { GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL, NC_("layer-color-space", "RGB (perceptual)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpLayerColorSpace", values);
      gimp_type_set_translation_context (type, "layer-color-space");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_layer_composite_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_LAYER_COMPOSITE_AUTO, "GIMP_LAYER_COMPOSITE_AUTO", "auto" },
    { GIMP_LAYER_COMPOSITE_UNION, "GIMP_LAYER_COMPOSITE_UNION", "union" },
    { GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP, "GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP", "clip-to-backdrop" },
    { GIMP_LAYER_COMPOSITE_CLIP_TO_LAYER, "GIMP_LAYER_COMPOSITE_CLIP_TO_LAYER", "clip-to-layer" },
    { GIMP_LAYER_COMPOSITE_INTERSECTION, "GIMP_LAYER_COMPOSITE_INTERSECTION", "intersection" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_LAYER_COMPOSITE_AUTO, NC_("layer-composite-mode", "Auto"), NULL },
    { GIMP_LAYER_COMPOSITE_UNION, NC_("layer-composite-mode", "Union"), NULL },
    { GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP, NC_("layer-composite-mode", "Clip to backdrop"), NULL },
    { GIMP_LAYER_COMPOSITE_CLIP_TO_LAYER, NC_("layer-composite-mode", "Clip to layer"), NULL },
    { GIMP_LAYER_COMPOSITE_INTERSECTION, NC_("layer-composite-mode", "Intersection"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpLayerCompositeMode", values);
      gimp_type_set_translation_context (type, "layer-composite-mode");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_layer_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_LAYER_MODE_NORMAL_LEGACY, "GIMP_LAYER_MODE_NORMAL_LEGACY", "normal-legacy" },
    { GIMP_LAYER_MODE_DISSOLVE, "GIMP_LAYER_MODE_DISSOLVE", "dissolve" },
    { GIMP_LAYER_MODE_BEHIND_LEGACY, "GIMP_LAYER_MODE_BEHIND_LEGACY", "behind-legacy" },
    { GIMP_LAYER_MODE_MULTIPLY_LEGACY, "GIMP_LAYER_MODE_MULTIPLY_LEGACY", "multiply-legacy" },
    { GIMP_LAYER_MODE_SCREEN_LEGACY, "GIMP_LAYER_MODE_SCREEN_LEGACY", "screen-legacy" },
    { GIMP_LAYER_MODE_OVERLAY_LEGACY, "GIMP_LAYER_MODE_OVERLAY_LEGACY", "overlay-legacy" },
    { GIMP_LAYER_MODE_DIFFERENCE_LEGACY, "GIMP_LAYER_MODE_DIFFERENCE_LEGACY", "difference-legacy" },
    { GIMP_LAYER_MODE_ADDITION_LEGACY, "GIMP_LAYER_MODE_ADDITION_LEGACY", "addition-legacy" },
    { GIMP_LAYER_MODE_SUBTRACT_LEGACY, "GIMP_LAYER_MODE_SUBTRACT_LEGACY", "subtract-legacy" },
    { GIMP_LAYER_MODE_DARKEN_ONLY_LEGACY, "GIMP_LAYER_MODE_DARKEN_ONLY_LEGACY", "darken-only-legacy" },
    { GIMP_LAYER_MODE_LIGHTEN_ONLY_LEGACY, "GIMP_LAYER_MODE_LIGHTEN_ONLY_LEGACY", "lighten-only-legacy" },
    { GIMP_LAYER_MODE_HSV_HUE_LEGACY, "GIMP_LAYER_MODE_HSV_HUE_LEGACY", "hsv-hue-legacy" },
    { GIMP_LAYER_MODE_HSV_SATURATION_LEGACY, "GIMP_LAYER_MODE_HSV_SATURATION_LEGACY", "hsv-saturation-legacy" },
    { GIMP_LAYER_MODE_HSL_COLOR_LEGACY, "GIMP_LAYER_MODE_HSL_COLOR_LEGACY", "hsl-color-legacy" },
    { GIMP_LAYER_MODE_HSV_VALUE_LEGACY, "GIMP_LAYER_MODE_HSV_VALUE_LEGACY", "hsv-value-legacy" },
    { GIMP_LAYER_MODE_DIVIDE_LEGACY, "GIMP_LAYER_MODE_DIVIDE_LEGACY", "divide-legacy" },
    { GIMP_LAYER_MODE_DODGE_LEGACY, "GIMP_LAYER_MODE_DODGE_LEGACY", "dodge-legacy" },
    { GIMP_LAYER_MODE_BURN_LEGACY, "GIMP_LAYER_MODE_BURN_LEGACY", "burn-legacy" },
    { GIMP_LAYER_MODE_HARDLIGHT_LEGACY, "GIMP_LAYER_MODE_HARDLIGHT_LEGACY", "hardlight-legacy" },
    { GIMP_LAYER_MODE_SOFTLIGHT_LEGACY, "GIMP_LAYER_MODE_SOFTLIGHT_LEGACY", "softlight-legacy" },
    { GIMP_LAYER_MODE_GRAIN_EXTRACT_LEGACY, "GIMP_LAYER_MODE_GRAIN_EXTRACT_LEGACY", "grain-extract-legacy" },
    { GIMP_LAYER_MODE_GRAIN_MERGE_LEGACY, "GIMP_LAYER_MODE_GRAIN_MERGE_LEGACY", "grain-merge-legacy" },
    { GIMP_LAYER_MODE_COLOR_ERASE_LEGACY, "GIMP_LAYER_MODE_COLOR_ERASE_LEGACY", "color-erase-legacy" },
    { GIMP_LAYER_MODE_OVERLAY, "GIMP_LAYER_MODE_OVERLAY", "overlay" },
    { GIMP_LAYER_MODE_LCH_HUE, "GIMP_LAYER_MODE_LCH_HUE", "lch-hue" },
    { GIMP_LAYER_MODE_LCH_CHROMA, "GIMP_LAYER_MODE_LCH_CHROMA", "lch-chroma" },
    { GIMP_LAYER_MODE_LCH_COLOR, "GIMP_LAYER_MODE_LCH_COLOR", "lch-color" },
    { GIMP_LAYER_MODE_LCH_LIGHTNESS, "GIMP_LAYER_MODE_LCH_LIGHTNESS", "lch-lightness" },
    { GIMP_LAYER_MODE_NORMAL, "GIMP_LAYER_MODE_NORMAL", "normal" },
    { GIMP_LAYER_MODE_BEHIND, "GIMP_LAYER_MODE_BEHIND", "behind" },
    { GIMP_LAYER_MODE_MULTIPLY, "GIMP_LAYER_MODE_MULTIPLY", "multiply" },
    { GIMP_LAYER_MODE_SCREEN, "GIMP_LAYER_MODE_SCREEN", "screen" },
    { GIMP_LAYER_MODE_DIFFERENCE, "GIMP_LAYER_MODE_DIFFERENCE", "difference" },
    { GIMP_LAYER_MODE_ADDITION, "GIMP_LAYER_MODE_ADDITION", "addition" },
    { GIMP_LAYER_MODE_SUBTRACT, "GIMP_LAYER_MODE_SUBTRACT", "subtract" },
    { GIMP_LAYER_MODE_DARKEN_ONLY, "GIMP_LAYER_MODE_DARKEN_ONLY", "darken-only" },
    { GIMP_LAYER_MODE_LIGHTEN_ONLY, "GIMP_LAYER_MODE_LIGHTEN_ONLY", "lighten-only" },
    { GIMP_LAYER_MODE_HSV_HUE, "GIMP_LAYER_MODE_HSV_HUE", "hsv-hue" },
    { GIMP_LAYER_MODE_HSV_SATURATION, "GIMP_LAYER_MODE_HSV_SATURATION", "hsv-saturation" },
    { GIMP_LAYER_MODE_HSL_COLOR, "GIMP_LAYER_MODE_HSL_COLOR", "hsl-color" },
    { GIMP_LAYER_MODE_HSV_VALUE, "GIMP_LAYER_MODE_HSV_VALUE", "hsv-value" },
    { GIMP_LAYER_MODE_DIVIDE, "GIMP_LAYER_MODE_DIVIDE", "divide" },
    { GIMP_LAYER_MODE_DODGE, "GIMP_LAYER_MODE_DODGE", "dodge" },
    { GIMP_LAYER_MODE_BURN, "GIMP_LAYER_MODE_BURN", "burn" },
    { GIMP_LAYER_MODE_HARDLIGHT, "GIMP_LAYER_MODE_HARDLIGHT", "hardlight" },
    { GIMP_LAYER_MODE_SOFTLIGHT, "GIMP_LAYER_MODE_SOFTLIGHT", "softlight" },
    { GIMP_LAYER_MODE_GRAIN_EXTRACT, "GIMP_LAYER_MODE_GRAIN_EXTRACT", "grain-extract" },
    { GIMP_LAYER_MODE_GRAIN_MERGE, "GIMP_LAYER_MODE_GRAIN_MERGE", "grain-merge" },
    { GIMP_LAYER_MODE_VIVID_LIGHT, "GIMP_LAYER_MODE_VIVID_LIGHT", "vivid-light" },
    { GIMP_LAYER_MODE_PIN_LIGHT, "GIMP_LAYER_MODE_PIN_LIGHT", "pin-light" },
    { GIMP_LAYER_MODE_LINEAR_LIGHT, "GIMP_LAYER_MODE_LINEAR_LIGHT", "linear-light" },
    { GIMP_LAYER_MODE_HARD_MIX, "GIMP_LAYER_MODE_HARD_MIX", "hard-mix" },
    { GIMP_LAYER_MODE_EXCLUSION, "GIMP_LAYER_MODE_EXCLUSION", "exclusion" },
    { GIMP_LAYER_MODE_LINEAR_BURN, "GIMP_LAYER_MODE_LINEAR_BURN", "linear-burn" },
    { GIMP_LAYER_MODE_LUMA_DARKEN_ONLY, "GIMP_LAYER_MODE_LUMA_DARKEN_ONLY", "luma-darken-only" },
    { GIMP_LAYER_MODE_LUMA_LIGHTEN_ONLY, "GIMP_LAYER_MODE_LUMA_LIGHTEN_ONLY", "luma-lighten-only" },
    { GIMP_LAYER_MODE_LUMINANCE, "GIMP_LAYER_MODE_LUMINANCE", "luminance" },
    { GIMP_LAYER_MODE_COLOR_ERASE, "GIMP_LAYER_MODE_COLOR_ERASE", "color-erase" },
    { GIMP_LAYER_MODE_ERASE, "GIMP_LAYER_MODE_ERASE", "erase" },
    { GIMP_LAYER_MODE_MERGE, "GIMP_LAYER_MODE_MERGE", "merge" },
    { GIMP_LAYER_MODE_SPLIT, "GIMP_LAYER_MODE_SPLIT", "split" },
    { GIMP_LAYER_MODE_PASS_THROUGH, "GIMP_LAYER_MODE_PASS_THROUGH", "pass-through" },
    { GIMP_LAYER_MODE_REPLACE, "GIMP_LAYER_MODE_REPLACE", "replace" },
    { GIMP_LAYER_MODE_OVERWRITE, "GIMP_LAYER_MODE_OVERWRITE", "overwrite" },
    { GIMP_LAYER_MODE_ANTI_ERASE, "GIMP_LAYER_MODE_ANTI_ERASE", "anti-erase" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_LAYER_MODE_NORMAL_LEGACY, NC_("layer-mode", "Normal (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Normal (legacy)".
       Keep it short. */
    { GIMP_LAYER_MODE_NORMAL_LEGACY, NC_("layer-mode", "Normal (l)"), NULL },
    { GIMP_LAYER_MODE_DISSOLVE, NC_("layer-mode", "Dissolve"), NULL },
    { GIMP_LAYER_MODE_BEHIND_LEGACY, NC_("layer-mode", "Behind (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Behind (legacy)".
       Keep it short. */
    { GIMP_LAYER_MODE_BEHIND_LEGACY, NC_("layer-mode", "Behind (l)"), NULL },
    { GIMP_LAYER_MODE_MULTIPLY_LEGACY, NC_("layer-mode", "Multiply (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Multiply (legacy)".
       Keep it short. */
    { GIMP_LAYER_MODE_MULTIPLY_LEGACY, NC_("layer-mode", "Multiply (l)"), NULL },
    { GIMP_LAYER_MODE_SCREEN_LEGACY, NC_("layer-mode", "Screen (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Screen (legacy)".
       Keep it short. */
    { GIMP_LAYER_MODE_SCREEN_LEGACY, NC_("layer-mode", "Screen (l)"), NULL },
    { GIMP_LAYER_MODE_OVERLAY_LEGACY, NC_("layer-mode", "Old broken Overlay"), NULL },
    /* Translators: this is an abbreviated version of "Old broken Overlay".
       Keep it short. */
    { GIMP_LAYER_MODE_OVERLAY_LEGACY, NC_("layer-mode", "Old Overlay"), NULL },
    { GIMP_LAYER_MODE_DIFFERENCE_LEGACY, NC_("layer-mode", "Difference (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Difference (legacy)".
       Keep it short. */
    { GIMP_LAYER_MODE_DIFFERENCE_LEGACY, NC_("layer-mode", "Difference (l)"), NULL },
    { GIMP_LAYER_MODE_ADDITION_LEGACY, NC_("layer-mode", "Addition (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Addition (legacy)".
       Keep it short. */
    { GIMP_LAYER_MODE_ADDITION_LEGACY, NC_("layer-mode", "Addition (l)"), NULL },
    { GIMP_LAYER_MODE_SUBTRACT_LEGACY, NC_("layer-mode", "Subtract (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Subtract (legacy)".
       Keep it short. */
    { GIMP_LAYER_MODE_SUBTRACT_LEGACY, NC_("layer-mode", "Subtract (l)"), NULL },
    { GIMP_LAYER_MODE_DARKEN_ONLY_LEGACY, NC_("layer-mode", "Darken only (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Darken only (legacy)".
       Keep it short. */
    { GIMP_LAYER_MODE_DARKEN_ONLY_LEGACY, NC_("layer-mode", "Darken only (l)"), NULL },
    { GIMP_LAYER_MODE_LIGHTEN_ONLY_LEGACY, NC_("layer-mode", "Lighten only (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Lighten only (legacy)".
       Keep it short. */
    { GIMP_LAYER_MODE_LIGHTEN_ONLY_LEGACY, NC_("layer-mode", "Lighten only (l)"), NULL },
    { GIMP_LAYER_MODE_HSV_HUE_LEGACY, NC_("layer-mode", "HSV Hue (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "HSV Hue (legacy)".
       Keep it short. */
    { GIMP_LAYER_MODE_HSV_HUE_LEGACY, NC_("layer-mode", "HSV Hue (l)"), NULL },
    { GIMP_LAYER_MODE_HSV_SATURATION_LEGACY, NC_("layer-mode", "HSV Saturation (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "HSV Saturation (legacy)".
       Keep it short. */
    { GIMP_LAYER_MODE_HSV_SATURATION_LEGACY, NC_("layer-mode", "HSV Saturation (l)"), NULL },
    { GIMP_LAYER_MODE_HSL_COLOR_LEGACY, NC_("layer-mode", "HSL Color (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "HSL Color (legacy)".
       Keep it short. */
    { GIMP_LAYER_MODE_HSL_COLOR_LEGACY, NC_("layer-mode", "HSL Color (l)"), NULL },
    { GIMP_LAYER_MODE_HSV_VALUE_LEGACY, NC_("layer-mode", "HSV Value (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "HSV Value (legacy)".
       Keep it short. */
    { GIMP_LAYER_MODE_HSV_VALUE_LEGACY, NC_("layer-mode", "HSV Value (l)"), NULL },
    { GIMP_LAYER_MODE_DIVIDE_LEGACY, NC_("layer-mode", "Divide (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Divide (legacy)".
       Keep it short. */
    { GIMP_LAYER_MODE_DIVIDE_LEGACY, NC_("layer-mode", "Divide (l)"), NULL },
    { GIMP_LAYER_MODE_DODGE_LEGACY, NC_("layer-mode", "Dodge (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Dodge (legacy)".
       Keep it short. */
    { GIMP_LAYER_MODE_DODGE_LEGACY, NC_("layer-mode", "Dodge (l)"), NULL },
    { GIMP_LAYER_MODE_BURN_LEGACY, NC_("layer-mode", "Burn (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Burn (legacy)".
       Keep it short. */
    { GIMP_LAYER_MODE_BURN_LEGACY, NC_("layer-mode", "Burn (l)"), NULL },
    { GIMP_LAYER_MODE_HARDLIGHT_LEGACY, NC_("layer-mode", "Hard light (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Hard light (legacy)".
       Keep it short. */
    { GIMP_LAYER_MODE_HARDLIGHT_LEGACY, NC_("layer-mode", "Hard light (l)"), NULL },
    { GIMP_LAYER_MODE_SOFTLIGHT_LEGACY, NC_("layer-mode", "Soft light (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Soft light (legacy)".
       Keep it short. */
    { GIMP_LAYER_MODE_SOFTLIGHT_LEGACY, NC_("layer-mode", "Soft light (l)"), NULL },
    { GIMP_LAYER_MODE_GRAIN_EXTRACT_LEGACY, NC_("layer-mode", "Grain extract (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Grain extract (legacy)".
       Keep it short. */
    { GIMP_LAYER_MODE_GRAIN_EXTRACT_LEGACY, NC_("layer-mode", "Grain extract (l)"), NULL },
    { GIMP_LAYER_MODE_GRAIN_MERGE_LEGACY, NC_("layer-mode", "Grain merge (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Grain merge (legacy)".
       Keep it short. */
    { GIMP_LAYER_MODE_GRAIN_MERGE_LEGACY, NC_("layer-mode", "Grain merge (l)"), NULL },
    { GIMP_LAYER_MODE_COLOR_ERASE_LEGACY, NC_("layer-mode", "Color erase (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Color erase (legacy)".
       Keep it short. */
    { GIMP_LAYER_MODE_COLOR_ERASE_LEGACY, NC_("layer-mode", "Color erase (l)"), NULL },
    { GIMP_LAYER_MODE_OVERLAY, NC_("layer-mode", "Overlay"), NULL },
    { GIMP_LAYER_MODE_LCH_HUE, NC_("layer-mode", "LCh Hue"), NULL },
    { GIMP_LAYER_MODE_LCH_CHROMA, NC_("layer-mode", "LCh Chroma"), NULL },
    { GIMP_LAYER_MODE_LCH_COLOR, NC_("layer-mode", "LCh Color"), NULL },
    { GIMP_LAYER_MODE_LCH_LIGHTNESS, NC_("layer-mode", "LCh Lightness"), NULL },
    { GIMP_LAYER_MODE_NORMAL, NC_("layer-mode", "Normal"), NULL },
    { GIMP_LAYER_MODE_BEHIND, NC_("layer-mode", "Behind"), NULL },
    { GIMP_LAYER_MODE_MULTIPLY, NC_("layer-mode", "Multiply"), NULL },
    { GIMP_LAYER_MODE_SCREEN, NC_("layer-mode", "Screen"), NULL },
    { GIMP_LAYER_MODE_DIFFERENCE, NC_("layer-mode", "Difference"), NULL },
    { GIMP_LAYER_MODE_ADDITION, NC_("layer-mode", "Addition"), NULL },
    { GIMP_LAYER_MODE_SUBTRACT, NC_("layer-mode", "Subtract"), NULL },
    { GIMP_LAYER_MODE_DARKEN_ONLY, NC_("layer-mode", "Darken only"), NULL },
    { GIMP_LAYER_MODE_LIGHTEN_ONLY, NC_("layer-mode", "Lighten only"), NULL },
    { GIMP_LAYER_MODE_HSV_HUE, NC_("layer-mode", "HSV Hue"), NULL },
    { GIMP_LAYER_MODE_HSV_SATURATION, NC_("layer-mode", "HSV Saturation"), NULL },
    { GIMP_LAYER_MODE_HSL_COLOR, NC_("layer-mode", "HSL Color"), NULL },
    { GIMP_LAYER_MODE_HSV_VALUE, NC_("layer-mode", "HSV Value"), NULL },
    { GIMP_LAYER_MODE_DIVIDE, NC_("layer-mode", "Divide"), NULL },
    { GIMP_LAYER_MODE_DODGE, NC_("layer-mode", "Dodge"), NULL },
    { GIMP_LAYER_MODE_BURN, NC_("layer-mode", "Burn"), NULL },
    { GIMP_LAYER_MODE_HARDLIGHT, NC_("layer-mode", "Hard light"), NULL },
    { GIMP_LAYER_MODE_SOFTLIGHT, NC_("layer-mode", "Soft light"), NULL },
    { GIMP_LAYER_MODE_GRAIN_EXTRACT, NC_("layer-mode", "Grain extract"), NULL },
    { GIMP_LAYER_MODE_GRAIN_MERGE, NC_("layer-mode", "Grain merge"), NULL },
    { GIMP_LAYER_MODE_VIVID_LIGHT, NC_("layer-mode", "Vivid light"), NULL },
    { GIMP_LAYER_MODE_PIN_LIGHT, NC_("layer-mode", "Pin light"), NULL },
    { GIMP_LAYER_MODE_LINEAR_LIGHT, NC_("layer-mode", "Linear light"), NULL },
    { GIMP_LAYER_MODE_HARD_MIX, NC_("layer-mode", "Hard mix"), NULL },
    { GIMP_LAYER_MODE_EXCLUSION, NC_("layer-mode", "Exclusion"), NULL },
    { GIMP_LAYER_MODE_LINEAR_BURN, NC_("layer-mode", "Linear burn"), NULL },
    { GIMP_LAYER_MODE_LUMA_DARKEN_ONLY, NC_("layer-mode", "Luma/Luminance darken only"), NULL },
    /* Translators: this is an abbreviated version of "Luma/Luminance darken only".
       Keep it short. */
    { GIMP_LAYER_MODE_LUMA_DARKEN_ONLY, NC_("layer-mode", "Luma darken only"), NULL },
    { GIMP_LAYER_MODE_LUMA_LIGHTEN_ONLY, NC_("layer-mode", "Luma/Luminance lighten only"), NULL },
    /* Translators: this is an abbreviated version of "Luma/Luminance lighten only".
       Keep it short. */
    { GIMP_LAYER_MODE_LUMA_LIGHTEN_ONLY, NC_("layer-mode", "Luma lighten only"), NULL },
    { GIMP_LAYER_MODE_LUMINANCE, NC_("layer-mode", "Luminance"), NULL },
    { GIMP_LAYER_MODE_COLOR_ERASE, NC_("layer-mode", "Color erase"), NULL },
    { GIMP_LAYER_MODE_ERASE, NC_("layer-mode", "Erase"), NULL },
    { GIMP_LAYER_MODE_MERGE, NC_("layer-mode", "Merge"), NULL },
    { GIMP_LAYER_MODE_SPLIT, NC_("layer-mode", "Split"), NULL },
    { GIMP_LAYER_MODE_PASS_THROUGH, NC_("layer-mode", "Pass through"), NULL },
    { GIMP_LAYER_MODE_REPLACE, NC_("layer-mode", "Replace"), NULL },
    { GIMP_LAYER_MODE_OVERWRITE, NC_("layer-mode", "Overwrite"), NULL },
    { GIMP_LAYER_MODE_ANTI_ERASE, NC_("layer-mode", "Anti erase"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpLayerMode", values);
      gimp_type_set_translation_context (type, "layer-mode");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_layer_mode_group_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_LAYER_MODE_GROUP_DEFAULT, "GIMP_LAYER_MODE_GROUP_DEFAULT", "default" },
    { GIMP_LAYER_MODE_GROUP_LEGACY, "GIMP_LAYER_MODE_GROUP_LEGACY", "legacy" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_LAYER_MODE_GROUP_DEFAULT, NC_("layer-mode-group", "Default"), NULL },
    { GIMP_LAYER_MODE_GROUP_LEGACY, NC_("layer-mode-group", "Legacy"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpLayerModeGroup", values);
      gimp_type_set_translation_context (type, "layer-mode-group");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_layer_mode_context_get_type (void)
{
  static const GFlagsValue values[] =
  {
    { GIMP_LAYER_MODE_CONTEXT_LAYER, "GIMP_LAYER_MODE_CONTEXT_LAYER", "layer" },
    { GIMP_LAYER_MODE_CONTEXT_GROUP, "GIMP_LAYER_MODE_CONTEXT_GROUP", "group" },
    { GIMP_LAYER_MODE_CONTEXT_PAINT, "GIMP_LAYER_MODE_CONTEXT_PAINT", "paint" },
    { GIMP_LAYER_MODE_CONTEXT_FILTER, "GIMP_LAYER_MODE_CONTEXT_FILTER", "filter" },
    { GIMP_LAYER_MODE_CONTEXT_ALL, "GIMP_LAYER_MODE_CONTEXT_ALL", "all" },
    { 0, NULL, NULL }
  };

  static const GimpFlagsDesc descs[] =
  {
    { GIMP_LAYER_MODE_CONTEXT_LAYER, "GIMP_LAYER_MODE_CONTEXT_LAYER", NULL },
    { GIMP_LAYER_MODE_CONTEXT_GROUP, "GIMP_LAYER_MODE_CONTEXT_GROUP", NULL },
    { GIMP_LAYER_MODE_CONTEXT_PAINT, "GIMP_LAYER_MODE_CONTEXT_PAINT", NULL },
    { GIMP_LAYER_MODE_CONTEXT_FILTER, "GIMP_LAYER_MODE_CONTEXT_FILTER", NULL },
    { GIMP_LAYER_MODE_CONTEXT_ALL, "GIMP_LAYER_MODE_CONTEXT_ALL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_flags_register_static ("GimpLayerModeContext", values);
      gimp_type_set_translation_context (type, "layer-mode-context");
      gimp_flags_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

