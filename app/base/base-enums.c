
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
    { GIMP_CURVE_SMOOTH, N_("Smooth"), NULL },
    { GIMP_CURVE_FREE, N_("Freehand"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpCurveType", values);
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
    { GIMP_HISTOGRAM_VALUE, N_("Value"), NULL },
    { GIMP_HISTOGRAM_RED, N_("Red"), NULL },
    { GIMP_HISTOGRAM_GREEN, N_("Green"), NULL },
    { GIMP_HISTOGRAM_BLUE, N_("Blue"), NULL },
    { GIMP_HISTOGRAM_ALPHA, N_("Alpha"), NULL },
    { GIMP_HISTOGRAM_RGB, N_("RGB"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpHistogramChannel", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_interpolation_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_INTERPOLATION_NONE, "GIMP_INTERPOLATION_NONE", "none" },
    { GIMP_INTERPOLATION_LINEAR, "GIMP_INTERPOLATION_LINEAR", "linear" },
    { GIMP_INTERPOLATION_CUBIC, "GIMP_INTERPOLATION_CUBIC", "cubic" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_INTERPOLATION_NONE, N_("None (Fastest)"), NULL },
    { GIMP_INTERPOLATION_LINEAR, N_("Linear"), NULL },
    { GIMP_INTERPOLATION_CUBIC, N_("Cubic (Best)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpInterpolationType", values);
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
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_NORMAL_MODE, "GIMP_NORMAL_MODE", NULL },
    { GIMP_DISSOLVE_MODE, "GIMP_DISSOLVE_MODE", NULL },
    { GIMP_BEHIND_MODE, "GIMP_BEHIND_MODE", NULL },
    { GIMP_MULTIPLY_MODE, "GIMP_MULTIPLY_MODE", NULL },
    { GIMP_SCREEN_MODE, "GIMP_SCREEN_MODE", NULL },
    { GIMP_OVERLAY_MODE, "GIMP_OVERLAY_MODE", NULL },
    { GIMP_DIFFERENCE_MODE, "GIMP_DIFFERENCE_MODE", NULL },
    { GIMP_ADDITION_MODE, "GIMP_ADDITION_MODE", NULL },
    { GIMP_SUBTRACT_MODE, "GIMP_SUBTRACT_MODE", NULL },
    { GIMP_DARKEN_ONLY_MODE, "GIMP_DARKEN_ONLY_MODE", NULL },
    { GIMP_LIGHTEN_ONLY_MODE, "GIMP_LIGHTEN_ONLY_MODE", NULL },
    { GIMP_HUE_MODE, "GIMP_HUE_MODE", NULL },
    { GIMP_SATURATION_MODE, "GIMP_SATURATION_MODE", NULL },
    { GIMP_COLOR_MODE, "GIMP_COLOR_MODE", NULL },
    { GIMP_VALUE_MODE, "GIMP_VALUE_MODE", NULL },
    { GIMP_DIVIDE_MODE, "GIMP_DIVIDE_MODE", NULL },
    { GIMP_DODGE_MODE, "GIMP_DODGE_MODE", NULL },
    { GIMP_BURN_MODE, "GIMP_BURN_MODE", NULL },
    { GIMP_HARDLIGHT_MODE, "GIMP_HARDLIGHT_MODE", NULL },
    { GIMP_SOFTLIGHT_MODE, "GIMP_SOFTLIGHT_MODE", NULL },
    { GIMP_GRAIN_EXTRACT_MODE, "GIMP_GRAIN_EXTRACT_MODE", NULL },
    { GIMP_GRAIN_MERGE_MODE, "GIMP_GRAIN_MERGE_MODE", NULL },
    { GIMP_COLOR_ERASE_MODE, "GIMP_COLOR_ERASE_MODE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpLayerModeEffects", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_transfer_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_SHADOWS, "GIMP_SHADOWS", "shadows" },
    { GIMP_MIDTONES, "GIMP_MIDTONES", "midtones" },
    { GIMP_HIGHLIGHTS, "GIMP_HIGHLIGHTS", "highlights" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_SHADOWS, N_("Shadows"), NULL },
    { GIMP_MIDTONES, N_("Midtones"), NULL },
    { GIMP_HIGHLIGHTS, N_("Highlights"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpTransferMode", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

