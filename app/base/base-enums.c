
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "base-enums.h"
#include"libgimp/gimpintl.h"

/* enumerations from "./base-enums.h" */

static const GEnumValue gimp_interpolation_type_enum_values[] =
{
  { GIMP_INTERPOLATION_NONE, N_("None (Fastest)"), "none" },
  { GIMP_INTERPOLATION_LINEAR, N_("Linear"), "linear" },
  { GIMP_INTERPOLATION_CUBIC, N_("Cubic (Best)"), "cubic" },
  { 0, NULL, NULL }
};

GType
gimp_interpolation_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpInterpolationType", gimp_interpolation_type_enum_values);

  return enum_type;
}


static const GEnumValue gimp_layer_mode_effects_enum_values[] =
{
  { GIMP_NORMAL_MODE, NULL, "normal-mode" },
  { GIMP_DISSOLVE_MODE, NULL, "dissolve-mode" },
  { GIMP_BEHIND_MODE, NULL, "behind-mode" },
  { GIMP_MULTIPLY_MODE, NULL, "multiply-mode" },
  { GIMP_SCREEN_MODE, NULL, "screen-mode" },
  { GIMP_OVERLAY_MODE, NULL, "overlay-mode" },
  { GIMP_DIFFERENCE_MODE, NULL, "difference-mode" },
  { GIMP_ADDITION_MODE, NULL, "addition-mode" },
  { GIMP_SUBTRACT_MODE, NULL, "subtract-mode" },
  { GIMP_DARKEN_ONLY_MODE, NULL, "darken-only-mode" },
  { GIMP_LIGHTEN_ONLY_MODE, NULL, "lighten-only-mode" },
  { GIMP_HUE_MODE, NULL, "hue-mode" },
  { GIMP_SATURATION_MODE, NULL, "saturation-mode" },
  { GIMP_COLOR_MODE, NULL, "color-mode" },
  { GIMP_VALUE_MODE, NULL, "value-mode" },
  { GIMP_DIVIDE_MODE, NULL, "divide-mode" },
  { GIMP_DODGE_MODE, NULL, "dodge-mode" },
  { GIMP_BURN_MODE, NULL, "burn-mode" },
  { GIMP_HARDLIGHT_MODE, NULL, "hardlight-mode" },
  { GIMP_COLOR_ERASE_MODE, NULL, "color-erase-mode" },
  { 0, NULL, NULL }
};

GType
gimp_layer_mode_effects_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpLayerModeEffects", gimp_layer_mode_effects_enum_values);

  return enum_type;
}


static const GEnumValue gimp_check_size_enum_values[] =
{
  { GIMP_SMALL_CHECKS, NULL, "small-checks" },
  { GIMP_MEDIUM_CHECKS, NULL, "medium-checks" },
  { GIMP_LARGE_CHECKS, NULL, "large-checks" },
  { 0, NULL, NULL }
};

GType
gimp_check_size_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpCheckSize", gimp_check_size_enum_values);

  return enum_type;
}


static const GEnumValue gimp_check_type_enum_values[] =
{
  { GIMP_LIGHT_CHECKS, NULL, "light-checks" },
  { GIMP_GRAY_CHECKS, NULL, "gray-checks" },
  { GIMP_DARK_CHECKS, NULL, "dark-checks" },
  { GIMP_WHITE_ONLY, NULL, "white-only" },
  { GIMP_GRAY_ONLY, NULL, "gray-only" },
  { GIMP_BLACK_ONLY, NULL, "black-only" },
  { 0, NULL, NULL }
};

GType
gimp_check_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpCheckType", gimp_check_type_enum_values);

  return enum_type;
}


static const GEnumValue gimp_histogram_channel_enum_values[] =
{
  { GIMP_HISTOGRAM_VALUE, NULL, "value" },
  { GIMP_HISTOGRAM_RED, NULL, "red" },
  { GIMP_HISTOGRAM_GREEN, NULL, "green" },
  { GIMP_HISTOGRAM_BLUE, NULL, "blue" },
  { GIMP_HISTOGRAM_ALPHA, NULL, "alpha" },
  { 0, NULL, NULL }
};

GType
gimp_histogram_channel_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpHistogramChannel", gimp_histogram_channel_enum_values);

  return enum_type;
}


/* Generated data ends here */

