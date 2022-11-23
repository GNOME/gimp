
/* Generated data (by ligma-mkenums) */

#include "stamp-operations-enums.h"
#include "config.h"
#include <gio/gio.h>
#include "libligmabase/ligmabase.h"
#include "operations-enums.h"
#include "ligma-intl.h"

/* enumerations from "operations-enums.h" */
GType
ligma_layer_color_space_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_LAYER_COLOR_SPACE_AUTO, "LIGMA_LAYER_COLOR_SPACE_AUTO", "auto" },
    { LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR, "LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR", "rgb-linear" },
    { LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL, "LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL", "rgb-perceptual" },
    { LIGMA_LAYER_COLOR_SPACE_LAB, "LIGMA_LAYER_COLOR_SPACE_LAB", "lab" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_LAYER_COLOR_SPACE_AUTO, NC_("layer-color-space", "Auto"), NULL },
    { LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR, NC_("layer-color-space", "RGB (linear)"), NULL },
    { LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL, NC_("layer-color-space", "RGB (perceptual)"), NULL },
    { LIGMA_LAYER_COLOR_SPACE_LAB, NC_("layer-color-space", "LAB"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaLayerColorSpace", values);
      ligma_type_set_translation_context (type, "layer-color-space");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_layer_composite_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_LAYER_COMPOSITE_AUTO, "LIGMA_LAYER_COMPOSITE_AUTO", "auto" },
    { LIGMA_LAYER_COMPOSITE_UNION, "LIGMA_LAYER_COMPOSITE_UNION", "union" },
    { LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP, "LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP", "clip-to-backdrop" },
    { LIGMA_LAYER_COMPOSITE_CLIP_TO_LAYER, "LIGMA_LAYER_COMPOSITE_CLIP_TO_LAYER", "clip-to-layer" },
    { LIGMA_LAYER_COMPOSITE_INTERSECTION, "LIGMA_LAYER_COMPOSITE_INTERSECTION", "intersection" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_LAYER_COMPOSITE_AUTO, NC_("layer-composite-mode", "Auto"), NULL },
    { LIGMA_LAYER_COMPOSITE_UNION, NC_("layer-composite-mode", "Union"), NULL },
    { LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP, NC_("layer-composite-mode", "Clip to backdrop"), NULL },
    { LIGMA_LAYER_COMPOSITE_CLIP_TO_LAYER, NC_("layer-composite-mode", "Clip to layer"), NULL },
    { LIGMA_LAYER_COMPOSITE_INTERSECTION, NC_("layer-composite-mode", "Intersection"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaLayerCompositeMode", values);
      ligma_type_set_translation_context (type, "layer-composite-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_layer_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_LAYER_MODE_NORMAL_LEGACY, "LIGMA_LAYER_MODE_NORMAL_LEGACY", "normal-legacy" },
    { LIGMA_LAYER_MODE_DISSOLVE, "LIGMA_LAYER_MODE_DISSOLVE", "dissolve" },
    { LIGMA_LAYER_MODE_BEHIND_LEGACY, "LIGMA_LAYER_MODE_BEHIND_LEGACY", "behind-legacy" },
    { LIGMA_LAYER_MODE_MULTIPLY_LEGACY, "LIGMA_LAYER_MODE_MULTIPLY_LEGACY", "multiply-legacy" },
    { LIGMA_LAYER_MODE_SCREEN_LEGACY, "LIGMA_LAYER_MODE_SCREEN_LEGACY", "screen-legacy" },
    { LIGMA_LAYER_MODE_OVERLAY_LEGACY, "LIGMA_LAYER_MODE_OVERLAY_LEGACY", "overlay-legacy" },
    { LIGMA_LAYER_MODE_DIFFERENCE_LEGACY, "LIGMA_LAYER_MODE_DIFFERENCE_LEGACY", "difference-legacy" },
    { LIGMA_LAYER_MODE_ADDITION_LEGACY, "LIGMA_LAYER_MODE_ADDITION_LEGACY", "addition-legacy" },
    { LIGMA_LAYER_MODE_SUBTRACT_LEGACY, "LIGMA_LAYER_MODE_SUBTRACT_LEGACY", "subtract-legacy" },
    { LIGMA_LAYER_MODE_DARKEN_ONLY_LEGACY, "LIGMA_LAYER_MODE_DARKEN_ONLY_LEGACY", "darken-only-legacy" },
    { LIGMA_LAYER_MODE_LIGHTEN_ONLY_LEGACY, "LIGMA_LAYER_MODE_LIGHTEN_ONLY_LEGACY", "lighten-only-legacy" },
    { LIGMA_LAYER_MODE_HSV_HUE_LEGACY, "LIGMA_LAYER_MODE_HSV_HUE_LEGACY", "hsv-hue-legacy" },
    { LIGMA_LAYER_MODE_HSV_SATURATION_LEGACY, "LIGMA_LAYER_MODE_HSV_SATURATION_LEGACY", "hsv-saturation-legacy" },
    { LIGMA_LAYER_MODE_HSL_COLOR_LEGACY, "LIGMA_LAYER_MODE_HSL_COLOR_LEGACY", "hsl-color-legacy" },
    { LIGMA_LAYER_MODE_HSV_VALUE_LEGACY, "LIGMA_LAYER_MODE_HSV_VALUE_LEGACY", "hsv-value-legacy" },
    { LIGMA_LAYER_MODE_DIVIDE_LEGACY, "LIGMA_LAYER_MODE_DIVIDE_LEGACY", "divide-legacy" },
    { LIGMA_LAYER_MODE_DODGE_LEGACY, "LIGMA_LAYER_MODE_DODGE_LEGACY", "dodge-legacy" },
    { LIGMA_LAYER_MODE_BURN_LEGACY, "LIGMA_LAYER_MODE_BURN_LEGACY", "burn-legacy" },
    { LIGMA_LAYER_MODE_HARDLIGHT_LEGACY, "LIGMA_LAYER_MODE_HARDLIGHT_LEGACY", "hardlight-legacy" },
    { LIGMA_LAYER_MODE_SOFTLIGHT_LEGACY, "LIGMA_LAYER_MODE_SOFTLIGHT_LEGACY", "softlight-legacy" },
    { LIGMA_LAYER_MODE_GRAIN_EXTRACT_LEGACY, "LIGMA_LAYER_MODE_GRAIN_EXTRACT_LEGACY", "grain-extract-legacy" },
    { LIGMA_LAYER_MODE_GRAIN_MERGE_LEGACY, "LIGMA_LAYER_MODE_GRAIN_MERGE_LEGACY", "grain-merge-legacy" },
    { LIGMA_LAYER_MODE_COLOR_ERASE_LEGACY, "LIGMA_LAYER_MODE_COLOR_ERASE_LEGACY", "color-erase-legacy" },
    { LIGMA_LAYER_MODE_OVERLAY, "LIGMA_LAYER_MODE_OVERLAY", "overlay" },
    { LIGMA_LAYER_MODE_LCH_HUE, "LIGMA_LAYER_MODE_LCH_HUE", "lch-hue" },
    { LIGMA_LAYER_MODE_LCH_CHROMA, "LIGMA_LAYER_MODE_LCH_CHROMA", "lch-chroma" },
    { LIGMA_LAYER_MODE_LCH_COLOR, "LIGMA_LAYER_MODE_LCH_COLOR", "lch-color" },
    { LIGMA_LAYER_MODE_LCH_LIGHTNESS, "LIGMA_LAYER_MODE_LCH_LIGHTNESS", "lch-lightness" },
    { LIGMA_LAYER_MODE_NORMAL, "LIGMA_LAYER_MODE_NORMAL", "normal" },
    { LIGMA_LAYER_MODE_BEHIND, "LIGMA_LAYER_MODE_BEHIND", "behind" },
    { LIGMA_LAYER_MODE_MULTIPLY, "LIGMA_LAYER_MODE_MULTIPLY", "multiply" },
    { LIGMA_LAYER_MODE_SCREEN, "LIGMA_LAYER_MODE_SCREEN", "screen" },
    { LIGMA_LAYER_MODE_DIFFERENCE, "LIGMA_LAYER_MODE_DIFFERENCE", "difference" },
    { LIGMA_LAYER_MODE_ADDITION, "LIGMA_LAYER_MODE_ADDITION", "addition" },
    { LIGMA_LAYER_MODE_SUBTRACT, "LIGMA_LAYER_MODE_SUBTRACT", "subtract" },
    { LIGMA_LAYER_MODE_DARKEN_ONLY, "LIGMA_LAYER_MODE_DARKEN_ONLY", "darken-only" },
    { LIGMA_LAYER_MODE_LIGHTEN_ONLY, "LIGMA_LAYER_MODE_LIGHTEN_ONLY", "lighten-only" },
    { LIGMA_LAYER_MODE_HSV_HUE, "LIGMA_LAYER_MODE_HSV_HUE", "hsv-hue" },
    { LIGMA_LAYER_MODE_HSV_SATURATION, "LIGMA_LAYER_MODE_HSV_SATURATION", "hsv-saturation" },
    { LIGMA_LAYER_MODE_HSL_COLOR, "LIGMA_LAYER_MODE_HSL_COLOR", "hsl-color" },
    { LIGMA_LAYER_MODE_HSV_VALUE, "LIGMA_LAYER_MODE_HSV_VALUE", "hsv-value" },
    { LIGMA_LAYER_MODE_DIVIDE, "LIGMA_LAYER_MODE_DIVIDE", "divide" },
    { LIGMA_LAYER_MODE_DODGE, "LIGMA_LAYER_MODE_DODGE", "dodge" },
    { LIGMA_LAYER_MODE_BURN, "LIGMA_LAYER_MODE_BURN", "burn" },
    { LIGMA_LAYER_MODE_HARDLIGHT, "LIGMA_LAYER_MODE_HARDLIGHT", "hardlight" },
    { LIGMA_LAYER_MODE_SOFTLIGHT, "LIGMA_LAYER_MODE_SOFTLIGHT", "softlight" },
    { LIGMA_LAYER_MODE_GRAIN_EXTRACT, "LIGMA_LAYER_MODE_GRAIN_EXTRACT", "grain-extract" },
    { LIGMA_LAYER_MODE_GRAIN_MERGE, "LIGMA_LAYER_MODE_GRAIN_MERGE", "grain-merge" },
    { LIGMA_LAYER_MODE_VIVID_LIGHT, "LIGMA_LAYER_MODE_VIVID_LIGHT", "vivid-light" },
    { LIGMA_LAYER_MODE_PIN_LIGHT, "LIGMA_LAYER_MODE_PIN_LIGHT", "pin-light" },
    { LIGMA_LAYER_MODE_LINEAR_LIGHT, "LIGMA_LAYER_MODE_LINEAR_LIGHT", "linear-light" },
    { LIGMA_LAYER_MODE_HARD_MIX, "LIGMA_LAYER_MODE_HARD_MIX", "hard-mix" },
    { LIGMA_LAYER_MODE_EXCLUSION, "LIGMA_LAYER_MODE_EXCLUSION", "exclusion" },
    { LIGMA_LAYER_MODE_LINEAR_BURN, "LIGMA_LAYER_MODE_LINEAR_BURN", "linear-burn" },
    { LIGMA_LAYER_MODE_LUMA_DARKEN_ONLY, "LIGMA_LAYER_MODE_LUMA_DARKEN_ONLY", "luma-darken-only" },
    { LIGMA_LAYER_MODE_LUMA_LIGHTEN_ONLY, "LIGMA_LAYER_MODE_LUMA_LIGHTEN_ONLY", "luma-lighten-only" },
    { LIGMA_LAYER_MODE_LUMINANCE, "LIGMA_LAYER_MODE_LUMINANCE", "luminance" },
    { LIGMA_LAYER_MODE_COLOR_ERASE, "LIGMA_LAYER_MODE_COLOR_ERASE", "color-erase" },
    { LIGMA_LAYER_MODE_ERASE, "LIGMA_LAYER_MODE_ERASE", "erase" },
    { LIGMA_LAYER_MODE_MERGE, "LIGMA_LAYER_MODE_MERGE", "merge" },
    { LIGMA_LAYER_MODE_SPLIT, "LIGMA_LAYER_MODE_SPLIT", "split" },
    { LIGMA_LAYER_MODE_PASS_THROUGH, "LIGMA_LAYER_MODE_PASS_THROUGH", "pass-through" },
    { LIGMA_LAYER_MODE_REPLACE, "LIGMA_LAYER_MODE_REPLACE", "replace" },
    { LIGMA_LAYER_MODE_ANTI_ERASE, "LIGMA_LAYER_MODE_ANTI_ERASE", "anti-erase" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_LAYER_MODE_NORMAL_LEGACY, NC_("layer-mode", "Normal (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Normal (legacy)".
       Keep it short. */
    { LIGMA_LAYER_MODE_NORMAL_LEGACY, NC_("layer-mode", "Normal (l)"), NULL },
    { LIGMA_LAYER_MODE_DISSOLVE, NC_("layer-mode", "Dissolve"), NULL },
    { LIGMA_LAYER_MODE_BEHIND_LEGACY, NC_("layer-mode", "Behind (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Behind (legacy)".
       Keep it short. */
    { LIGMA_LAYER_MODE_BEHIND_LEGACY, NC_("layer-mode", "Behind (l)"), NULL },
    { LIGMA_LAYER_MODE_MULTIPLY_LEGACY, NC_("layer-mode", "Multiply (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Multiply (legacy)".
       Keep it short. */
    { LIGMA_LAYER_MODE_MULTIPLY_LEGACY, NC_("layer-mode", "Multiply (l)"), NULL },
    { LIGMA_LAYER_MODE_SCREEN_LEGACY, NC_("layer-mode", "Screen (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Screen (legacy)".
       Keep it short. */
    { LIGMA_LAYER_MODE_SCREEN_LEGACY, NC_("layer-mode", "Screen (l)"), NULL },
    { LIGMA_LAYER_MODE_OVERLAY_LEGACY, NC_("layer-mode", "Old broken Overlay"), NULL },
    /* Translators: this is an abbreviated version of "Old broken Overlay".
       Keep it short. */
    { LIGMA_LAYER_MODE_OVERLAY_LEGACY, NC_("layer-mode", "Old Overlay"), NULL },
    { LIGMA_LAYER_MODE_DIFFERENCE_LEGACY, NC_("layer-mode", "Difference (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Difference (legacy)".
       Keep it short. */
    { LIGMA_LAYER_MODE_DIFFERENCE_LEGACY, NC_("layer-mode", "Difference (l)"), NULL },
    { LIGMA_LAYER_MODE_ADDITION_LEGACY, NC_("layer-mode", "Addition (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Addition (legacy)".
       Keep it short. */
    { LIGMA_LAYER_MODE_ADDITION_LEGACY, NC_("layer-mode", "Addition (l)"), NULL },
    { LIGMA_LAYER_MODE_SUBTRACT_LEGACY, NC_("layer-mode", "Subtract (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Subtract (legacy)".
       Keep it short. */
    { LIGMA_LAYER_MODE_SUBTRACT_LEGACY, NC_("layer-mode", "Subtract (l)"), NULL },
    { LIGMA_LAYER_MODE_DARKEN_ONLY_LEGACY, NC_("layer-mode", "Darken only (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Darken only (legacy)".
       Keep it short. */
    { LIGMA_LAYER_MODE_DARKEN_ONLY_LEGACY, NC_("layer-mode", "Darken only (l)"), NULL },
    { LIGMA_LAYER_MODE_LIGHTEN_ONLY_LEGACY, NC_("layer-mode", "Lighten only (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Lighten only (legacy)".
       Keep it short. */
    { LIGMA_LAYER_MODE_LIGHTEN_ONLY_LEGACY, NC_("layer-mode", "Lighten only (l)"), NULL },
    { LIGMA_LAYER_MODE_HSV_HUE_LEGACY, NC_("layer-mode", "HSV Hue (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "HSV Hue (legacy)".
       Keep it short. */
    { LIGMA_LAYER_MODE_HSV_HUE_LEGACY, NC_("layer-mode", "HSV Hue (l)"), NULL },
    { LIGMA_LAYER_MODE_HSV_SATURATION_LEGACY, NC_("layer-mode", "HSV Saturation (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "HSV Saturation (legacy)".
       Keep it short. */
    { LIGMA_LAYER_MODE_HSV_SATURATION_LEGACY, NC_("layer-mode", "HSV Saturation (l)"), NULL },
    { LIGMA_LAYER_MODE_HSL_COLOR_LEGACY, NC_("layer-mode", "HSL Color (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "HSL Color (legacy)".
       Keep it short. */
    { LIGMA_LAYER_MODE_HSL_COLOR_LEGACY, NC_("layer-mode", "HSL Color (l)"), NULL },
    { LIGMA_LAYER_MODE_HSV_VALUE_LEGACY, NC_("layer-mode", "HSV Value (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "HSV Value (legacy)".
       Keep it short. */
    { LIGMA_LAYER_MODE_HSV_VALUE_LEGACY, NC_("layer-mode", "HSV Value (l)"), NULL },
    { LIGMA_LAYER_MODE_DIVIDE_LEGACY, NC_("layer-mode", "Divide (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Divide (legacy)".
       Keep it short. */
    { LIGMA_LAYER_MODE_DIVIDE_LEGACY, NC_("layer-mode", "Divide (l)"), NULL },
    { LIGMA_LAYER_MODE_DODGE_LEGACY, NC_("layer-mode", "Dodge (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Dodge (legacy)".
       Keep it short. */
    { LIGMA_LAYER_MODE_DODGE_LEGACY, NC_("layer-mode", "Dodge (l)"), NULL },
    { LIGMA_LAYER_MODE_BURN_LEGACY, NC_("layer-mode", "Burn (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Burn (legacy)".
       Keep it short. */
    { LIGMA_LAYER_MODE_BURN_LEGACY, NC_("layer-mode", "Burn (l)"), NULL },
    { LIGMA_LAYER_MODE_HARDLIGHT_LEGACY, NC_("layer-mode", "Hard light (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Hard light (legacy)".
       Keep it short. */
    { LIGMA_LAYER_MODE_HARDLIGHT_LEGACY, NC_("layer-mode", "Hard light (l)"), NULL },
    { LIGMA_LAYER_MODE_SOFTLIGHT_LEGACY, NC_("layer-mode", "Soft light (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Soft light (legacy)".
       Keep it short. */
    { LIGMA_LAYER_MODE_SOFTLIGHT_LEGACY, NC_("layer-mode", "Soft light (l)"), NULL },
    { LIGMA_LAYER_MODE_GRAIN_EXTRACT_LEGACY, NC_("layer-mode", "Grain extract (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Grain extract (legacy)".
       Keep it short. */
    { LIGMA_LAYER_MODE_GRAIN_EXTRACT_LEGACY, NC_("layer-mode", "Grain extract (l)"), NULL },
    { LIGMA_LAYER_MODE_GRAIN_MERGE_LEGACY, NC_("layer-mode", "Grain merge (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Grain merge (legacy)".
       Keep it short. */
    { LIGMA_LAYER_MODE_GRAIN_MERGE_LEGACY, NC_("layer-mode", "Grain merge (l)"), NULL },
    { LIGMA_LAYER_MODE_COLOR_ERASE_LEGACY, NC_("layer-mode", "Color erase (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Color erase (legacy)".
       Keep it short. */
    { LIGMA_LAYER_MODE_COLOR_ERASE_LEGACY, NC_("layer-mode", "Color erase (l)"), NULL },
    { LIGMA_LAYER_MODE_OVERLAY, NC_("layer-mode", "Overlay"), NULL },
    { LIGMA_LAYER_MODE_LCH_HUE, NC_("layer-mode", "LCh Hue"), NULL },
    { LIGMA_LAYER_MODE_LCH_CHROMA, NC_("layer-mode", "LCh Chroma"), NULL },
    { LIGMA_LAYER_MODE_LCH_COLOR, NC_("layer-mode", "LCh Color"), NULL },
    { LIGMA_LAYER_MODE_LCH_LIGHTNESS, NC_("layer-mode", "LCh Lightness"), NULL },
    { LIGMA_LAYER_MODE_NORMAL, NC_("layer-mode", "Normal"), NULL },
    { LIGMA_LAYER_MODE_BEHIND, NC_("layer-mode", "Behind"), NULL },
    { LIGMA_LAYER_MODE_MULTIPLY, NC_("layer-mode", "Multiply"), NULL },
    { LIGMA_LAYER_MODE_SCREEN, NC_("layer-mode", "Screen"), NULL },
    { LIGMA_LAYER_MODE_DIFFERENCE, NC_("layer-mode", "Difference"), NULL },
    { LIGMA_LAYER_MODE_ADDITION, NC_("layer-mode", "Addition"), NULL },
    { LIGMA_LAYER_MODE_SUBTRACT, NC_("layer-mode", "Subtract"), NULL },
    { LIGMA_LAYER_MODE_DARKEN_ONLY, NC_("layer-mode", "Darken only"), NULL },
    { LIGMA_LAYER_MODE_LIGHTEN_ONLY, NC_("layer-mode", "Lighten only"), NULL },
    { LIGMA_LAYER_MODE_HSV_HUE, NC_("layer-mode", "HSV Hue"), NULL },
    { LIGMA_LAYER_MODE_HSV_SATURATION, NC_("layer-mode", "HSV Saturation"), NULL },
    { LIGMA_LAYER_MODE_HSL_COLOR, NC_("layer-mode", "HSL Color"), NULL },
    { LIGMA_LAYER_MODE_HSV_VALUE, NC_("layer-mode", "HSV Value"), NULL },
    { LIGMA_LAYER_MODE_DIVIDE, NC_("layer-mode", "Divide"), NULL },
    { LIGMA_LAYER_MODE_DODGE, NC_("layer-mode", "Dodge"), NULL },
    { LIGMA_LAYER_MODE_BURN, NC_("layer-mode", "Burn"), NULL },
    { LIGMA_LAYER_MODE_HARDLIGHT, NC_("layer-mode", "Hard light"), NULL },
    { LIGMA_LAYER_MODE_SOFTLIGHT, NC_("layer-mode", "Soft light"), NULL },
    { LIGMA_LAYER_MODE_GRAIN_EXTRACT, NC_("layer-mode", "Grain extract"), NULL },
    { LIGMA_LAYER_MODE_GRAIN_MERGE, NC_("layer-mode", "Grain merge"), NULL },
    { LIGMA_LAYER_MODE_VIVID_LIGHT, NC_("layer-mode", "Vivid light"), NULL },
    { LIGMA_LAYER_MODE_PIN_LIGHT, NC_("layer-mode", "Pin light"), NULL },
    { LIGMA_LAYER_MODE_LINEAR_LIGHT, NC_("layer-mode", "Linear light"), NULL },
    { LIGMA_LAYER_MODE_HARD_MIX, NC_("layer-mode", "Hard mix"), NULL },
    { LIGMA_LAYER_MODE_EXCLUSION, NC_("layer-mode", "Exclusion"), NULL },
    { LIGMA_LAYER_MODE_LINEAR_BURN, NC_("layer-mode", "Linear burn"), NULL },
    { LIGMA_LAYER_MODE_LUMA_DARKEN_ONLY, NC_("layer-mode", "Luma/Luminance darken only"), NULL },
    /* Translators: this is an abbreviated version of "Luma/Luminance darken only".
       Keep it short. */
    { LIGMA_LAYER_MODE_LUMA_DARKEN_ONLY, NC_("layer-mode", "Luma darken only"), NULL },
    { LIGMA_LAYER_MODE_LUMA_LIGHTEN_ONLY, NC_("layer-mode", "Luma/Luminance lighten only"), NULL },
    /* Translators: this is an abbreviated version of "Luma/Luminance lighten only".
       Keep it short. */
    { LIGMA_LAYER_MODE_LUMA_LIGHTEN_ONLY, NC_("layer-mode", "Luma lighten only"), NULL },
    { LIGMA_LAYER_MODE_LUMINANCE, NC_("layer-mode", "Luminance"), NULL },
    { LIGMA_LAYER_MODE_COLOR_ERASE, NC_("layer-mode", "Color erase"), NULL },
    { LIGMA_LAYER_MODE_ERASE, NC_("layer-mode", "Erase"), NULL },
    { LIGMA_LAYER_MODE_MERGE, NC_("layer-mode", "Merge"), NULL },
    { LIGMA_LAYER_MODE_SPLIT, NC_("layer-mode", "Split"), NULL },
    { LIGMA_LAYER_MODE_PASS_THROUGH, NC_("layer-mode", "Pass through"), NULL },
    { LIGMA_LAYER_MODE_REPLACE, NC_("layer-mode", "Replace"), NULL },
    { LIGMA_LAYER_MODE_ANTI_ERASE, NC_("layer-mode", "Anti erase"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaLayerMode", values);
      ligma_type_set_translation_context (type, "layer-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_layer_mode_group_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_LAYER_MODE_GROUP_DEFAULT, "LIGMA_LAYER_MODE_GROUP_DEFAULT", "default" },
    { LIGMA_LAYER_MODE_GROUP_LEGACY, "LIGMA_LAYER_MODE_GROUP_LEGACY", "legacy" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_LAYER_MODE_GROUP_DEFAULT, NC_("layer-mode-group", "Default"), NULL },
    { LIGMA_LAYER_MODE_GROUP_LEGACY, NC_("layer-mode-group", "Legacy"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaLayerModeGroup", values);
      ligma_type_set_translation_context (type, "layer-mode-group");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_layer_mode_context_get_type (void)
{
  static const GFlagsValue values[] =
  {
    { LIGMA_LAYER_MODE_CONTEXT_LAYER, "LIGMA_LAYER_MODE_CONTEXT_LAYER", "layer" },
    { LIGMA_LAYER_MODE_CONTEXT_GROUP, "LIGMA_LAYER_MODE_CONTEXT_GROUP", "group" },
    { LIGMA_LAYER_MODE_CONTEXT_PAINT, "LIGMA_LAYER_MODE_CONTEXT_PAINT", "paint" },
    { LIGMA_LAYER_MODE_CONTEXT_FILTER, "LIGMA_LAYER_MODE_CONTEXT_FILTER", "filter" },
    { LIGMA_LAYER_MODE_CONTEXT_ALL, "LIGMA_LAYER_MODE_CONTEXT_ALL", "all" },
    { 0, NULL, NULL }
  };

  static const LigmaFlagsDesc descs[] =
  {
    { LIGMA_LAYER_MODE_CONTEXT_LAYER, "LIGMA_LAYER_MODE_CONTEXT_LAYER", NULL },
    { LIGMA_LAYER_MODE_CONTEXT_GROUP, "LIGMA_LAYER_MODE_CONTEXT_GROUP", NULL },
    { LIGMA_LAYER_MODE_CONTEXT_PAINT, "LIGMA_LAYER_MODE_CONTEXT_PAINT", NULL },
    { LIGMA_LAYER_MODE_CONTEXT_FILTER, "LIGMA_LAYER_MODE_CONTEXT_FILTER", NULL },
    { LIGMA_LAYER_MODE_CONTEXT_ALL, "LIGMA_LAYER_MODE_CONTEXT_ALL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_flags_register_static ("LigmaLayerModeContext", values);
      ligma_type_set_translation_context (type, "layer-mode-context");
      ligma_flags_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

