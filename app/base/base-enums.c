
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "base-enums.h"
#include "gimp-intl.h"

/* enumerations from "./base-enums.h" */
GType
gimp_check_size_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_SMALL_CHECKS, N_("Small"), "small-checks" },
    { GIMP_MEDIUM_CHECKS, N_("Medium"), "medium-checks" },
    { GIMP_LARGE_CHECKS, N_("Large"), "large-checks" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpCheckSize", values);

  return type;
}

GType
gimp_check_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_LIGHT_CHECKS, N_("Light Checks"), "light-checks" },
    { GIMP_GRAY_CHECKS, N_("Mid-Tone Checks"), "gray-checks" },
    { GIMP_DARK_CHECKS, N_("Dark Checks"), "dark-checks" },
    { GIMP_WHITE_ONLY, N_("White Only"), "white-only" },
    { GIMP_GRAY_ONLY, N_("Gray Only"), "gray-only" },
    { GIMP_BLACK_ONLY, N_("Black Only"), "black-only" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpCheckType", values);

  return type;
}

GType
gimp_curve_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_CURVE_SMOOTH, N_("Smooth"), "smooth" },
    { GIMP_CURVE_FREE, N_("Freehand"), "free" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpCurveType", values);

  return type;
}

GType
gimp_histogram_channel_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_HISTOGRAM_VALUE, N_("Value"), "value" },
    { GIMP_HISTOGRAM_RED, N_("Red"), "red" },
    { GIMP_HISTOGRAM_GREEN, N_("Green"), "green" },
    { GIMP_HISTOGRAM_BLUE, N_("Blue"), "blue" },
    { GIMP_HISTOGRAM_ALPHA, N_("Alpha"), "alpha" },
    { GIMP_HISTOGRAM_RGB, N_("RGB"), "rgb" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpHistogramChannel", values);

  return type;
}

GType
gimp_interpolation_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_INTERPOLATION_NONE, N_("None (Fastest)"), "none" },
    { GIMP_INTERPOLATION_LINEAR, N_("Linear"), "linear" },
    { GIMP_INTERPOLATION_CUBIC, N_("Cubic (Best)"), "cubic" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpInterpolationType", values);

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

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpLayerModeEffects", values);

  return type;
}

GType
gimp_transfer_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_SHADOWS, N_("Shadows"), "shadows" },
    { GIMP_MIDTONES, N_("Midtones"), "midtones" },
    { GIMP_HIGHLIGHTS, N_("Highlights"), "highlights" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpTransferMode", values);

  return type;
}


/* Generated data ends here */

