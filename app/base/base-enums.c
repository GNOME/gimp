
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "libgimpbase/gimpbase.h"
#include "base-enums.h"
#include "gimp-intl.h"

/* enumerations from "./base-enums.h" */
GType
gimp_curve_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_CURVE_SMOOTH, "GIMP_CURVE_SMOOTH", "smooth" },
    { GIMP_CURVE_FREE, "GIMP_CURVE_FREE", "free" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_CURVE_SMOOTH, NC_("curve-type", "Smooth"), NULL },
    { GIMP_CURVE_FREE, NC_("curve-type", "Freehand"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpCurveType", values);
      gimp_type_set_translation_context (type, "curve-type");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_histogram_channel_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_HISTOGRAM_VALUE, "GIMP_HISTOGRAM_VALUE", "value" },
    { GIMP_HISTOGRAM_RED, "GIMP_HISTOGRAM_RED", "red" },
    { GIMP_HISTOGRAM_GREEN, "GIMP_HISTOGRAM_GREEN", "green" },
    { GIMP_HISTOGRAM_BLUE, "GIMP_HISTOGRAM_BLUE", "blue" },
    { GIMP_HISTOGRAM_ALPHA, "GIMP_HISTOGRAM_ALPHA", "alpha" },
    { GIMP_HISTOGRAM_RGB, "GIMP_HISTOGRAM_RGB", "rgb" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_HISTOGRAM_VALUE, NC_("histogram-channel", "Value"), NULL },
    { GIMP_HISTOGRAM_RED, NC_("histogram-channel", "Red"), NULL },
    { GIMP_HISTOGRAM_GREEN, NC_("histogram-channel", "Green"), NULL },
    { GIMP_HISTOGRAM_BLUE, NC_("histogram-channel", "Blue"), NULL },
    { GIMP_HISTOGRAM_ALPHA, NC_("histogram-channel", "Alpha"), NULL },
    { GIMP_HISTOGRAM_RGB, NC_("histogram-channel", "RGB"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpHistogramChannel", values);
      gimp_type_set_translation_context (type, "histogram-channel");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_layer_mode_effects_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_NORMAL_MODE, "GIMP_NORMAL_MODE", "normal-mode" },
    { GIMP_DISSOLVE_MODE, "GIMP_DISSOLVE_MODE", "dissolve-mode" },
    { GIMP_BEHIND_MODE, "GIMP_BEHIND_MODE", "behind-mode" },
    { GIMP_MULTIPLY_MODE, "GIMP_MULTIPLY_MODE", "multiply-mode" },
    { GIMP_SCREEN_MODE, "GIMP_SCREEN_MODE", "screen-mode" },
    { GIMP_OVERLAY_MODE, "GIMP_OVERLAY_MODE", "overlay-mode" },
    { GIMP_DIFFERENCE_MODE, "GIMP_DIFFERENCE_MODE", "difference-mode" },
    { GIMP_ADDITION_MODE, "GIMP_ADDITION_MODE", "addition-mode" },
    { GIMP_SUBTRACT_MODE, "GIMP_SUBTRACT_MODE", "subtract-mode" },
    { GIMP_DARKEN_ONLY_MODE, "GIMP_DARKEN_ONLY_MODE", "darken-only-mode" },
    { GIMP_LIGHTEN_ONLY_MODE, "GIMP_LIGHTEN_ONLY_MODE", "lighten-only-mode" },
    { GIMP_HUE_MODE, "GIMP_HUE_MODE", "hue-mode" },
    { GIMP_SATURATION_MODE, "GIMP_SATURATION_MODE", "saturation-mode" },
    { GIMP_COLOR_MODE, "GIMP_COLOR_MODE", "color-mode" },
    { GIMP_VALUE_MODE, "GIMP_VALUE_MODE", "value-mode" },
    { GIMP_DIVIDE_MODE, "GIMP_DIVIDE_MODE", "divide-mode" },
    { GIMP_DODGE_MODE, "GIMP_DODGE_MODE", "dodge-mode" },
    { GIMP_BURN_MODE, "GIMP_BURN_MODE", "burn-mode" },
    { GIMP_HARDLIGHT_MODE, "GIMP_HARDLIGHT_MODE", "hardlight-mode" },
    { GIMP_SOFTLIGHT_MODE, "GIMP_SOFTLIGHT_MODE", "softlight-mode" },
    { GIMP_GRAIN_EXTRACT_MODE, "GIMP_GRAIN_EXTRACT_MODE", "grain-extract-mode" },
    { GIMP_GRAIN_MERGE_MODE, "GIMP_GRAIN_MERGE_MODE", "grain-merge-mode" },
    { GIMP_COLOR_ERASE_MODE, "GIMP_COLOR_ERASE_MODE", "color-erase-mode" },
    { GIMP_ERASE_MODE, "GIMP_ERASE_MODE", "erase-mode" },
    { GIMP_REPLACE_MODE, "GIMP_REPLACE_MODE", "replace-mode" },
    { GIMP_ANTI_ERASE_MODE, "GIMP_ANTI_ERASE_MODE", "anti-erase-mode" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_NORMAL_MODE, NC_("layer-mode-effects", "Normal"), NULL },
    { GIMP_DISSOLVE_MODE, NC_("layer-mode-effects", "Dissolve"), NULL },
    { GIMP_BEHIND_MODE, NC_("layer-mode-effects", "Behind"), NULL },
    { GIMP_MULTIPLY_MODE, NC_("layer-mode-effects", "Multiply"), NULL },
    { GIMP_SCREEN_MODE, NC_("layer-mode-effects", "Screen"), NULL },
    { GIMP_OVERLAY_MODE, NC_("layer-mode-effects", "Overlay"), NULL },
    { GIMP_DIFFERENCE_MODE, NC_("layer-mode-effects", "Difference"), NULL },
    { GIMP_ADDITION_MODE, NC_("layer-mode-effects", "Addition"), NULL },
    { GIMP_SUBTRACT_MODE, NC_("layer-mode-effects", "Subtract"), NULL },
    { GIMP_DARKEN_ONLY_MODE, NC_("layer-mode-effects", "Darken only"), NULL },
    { GIMP_LIGHTEN_ONLY_MODE, NC_("layer-mode-effects", "Lighten only"), NULL },
    { GIMP_HUE_MODE, NC_("layer-mode-effects", "Hue"), NULL },
    { GIMP_SATURATION_MODE, NC_("layer-mode-effects", "Saturation"), NULL },
    { GIMP_COLOR_MODE, NC_("layer-mode-effects", "Color"), NULL },
    { GIMP_VALUE_MODE, NC_("layer-mode-effects", "Value"), NULL },
    { GIMP_DIVIDE_MODE, NC_("layer-mode-effects", "Divide"), NULL },
    { GIMP_DODGE_MODE, NC_("layer-mode-effects", "Dodge"), NULL },
    { GIMP_BURN_MODE, NC_("layer-mode-effects", "Burn"), NULL },
    { GIMP_HARDLIGHT_MODE, NC_("layer-mode-effects", "Hard light"), NULL },
    { GIMP_SOFTLIGHT_MODE, NC_("layer-mode-effects", "Soft light"), NULL },
    { GIMP_GRAIN_EXTRACT_MODE, NC_("layer-mode-effects", "Grain extract"), NULL },
    { GIMP_GRAIN_MERGE_MODE, NC_("layer-mode-effects", "Grain merge"), NULL },
    { GIMP_COLOR_ERASE_MODE, NC_("layer-mode-effects", "Color erase"), NULL },
    { GIMP_ERASE_MODE, NC_("layer-mode-effects", "Erase"), NULL },
    { GIMP_REPLACE_MODE, NC_("layer-mode-effects", "Replace"), NULL },
    { GIMP_ANTI_ERASE_MODE, NC_("layer-mode-effects", "Anti erase"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpLayerModeEffects", values);
      gimp_type_set_translation_context (type, "layer-mode-effects");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_hue_range_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_ALL_HUES, "GIMP_ALL_HUES", "all-hues" },
    { GIMP_RED_HUES, "GIMP_RED_HUES", "red-hues" },
    { GIMP_YELLOW_HUES, "GIMP_YELLOW_HUES", "yellow-hues" },
    { GIMP_GREEN_HUES, "GIMP_GREEN_HUES", "green-hues" },
    { GIMP_CYAN_HUES, "GIMP_CYAN_HUES", "cyan-hues" },
    { GIMP_BLUE_HUES, "GIMP_BLUE_HUES", "blue-hues" },
    { GIMP_MAGENTA_HUES, "GIMP_MAGENTA_HUES", "magenta-hues" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_ALL_HUES, "GIMP_ALL_HUES", NULL },
    { GIMP_RED_HUES, "GIMP_RED_HUES", NULL },
    { GIMP_YELLOW_HUES, "GIMP_YELLOW_HUES", NULL },
    { GIMP_GREEN_HUES, "GIMP_GREEN_HUES", NULL },
    { GIMP_CYAN_HUES, "GIMP_CYAN_HUES", NULL },
    { GIMP_BLUE_HUES, "GIMP_BLUE_HUES", NULL },
    { GIMP_MAGENTA_HUES, "GIMP_MAGENTA_HUES", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpHueRange", values);
      gimp_type_set_translation_context (type, "hue-range");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

