
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "core-enums.h"
#include"libgimp/gimpintl.h"

/* enumerations from "./core-enums.h" */

static const GEnumValue gimp_image_base_type_enum_values[] =
{
  { GIMP_RGB, "GIMP_RGB", "rgb" },
  { GIMP_GRAY, "GIMP_GRAY", "gray" },
  { GIMP_INDEXED, "GIMP_INDEXED", "indexed" },
  { 0, NULL, NULL }
};

GType
gimp_image_base_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpImageBaseType", gimp_image_base_type_enum_values);

  return enum_type;
}


static const GEnumValue gimp_preview_size_enum_values[] =
{
  { GIMP_PREVIEW_SIZE_NONE, N_("None"), "none" },
  { GIMP_PREVIEW_SIZE_TINY, N_("Tiny"), "tiny" },
  { GIMP_PREVIEW_SIZE_EXTRA_SMALL, N_("Very Small"), "extra-small" },
  { GIMP_PREVIEW_SIZE_SMALL, N_("Small"), "small" },
  { GIMP_PREVIEW_SIZE_MEDIUM, N_("Medium"), "medium" },
  { GIMP_PREVIEW_SIZE_LARGE, N_("Large"), "large" },
  { GIMP_PREVIEW_SIZE_EXTRA_LARGE, N_("Very Large"), "extra-large" },
  { GIMP_PREVIEW_SIZE_HUGE, N_("Huge"), "huge" },
  { GIMP_PREVIEW_SIZE_ENORMOUS, N_("Enormous"), "enormous" },
  { GIMP_PREVIEW_SIZE_GIGANTIC, N_("Gigantic"), "gigantic" },
  { 0, NULL, NULL }
};

GType
gimp_preview_size_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpPreviewSize", gimp_preview_size_enum_values);

  return enum_type;
}


static const GEnumValue gimp_selection_control_enum_values[] =
{
  { GIMP_SELECTION_OFF, "GIMP_SELECTION_OFF", "off" },
  { GIMP_SELECTION_LAYER_OFF, "GIMP_SELECTION_LAYER_OFF", "layer-off" },
  { GIMP_SELECTION_ON, "GIMP_SELECTION_ON", "on" },
  { GIMP_SELECTION_PAUSE, "GIMP_SELECTION_PAUSE", "pause" },
  { GIMP_SELECTION_RESUME, "GIMP_SELECTION_RESUME", "resume" },
  { 0, NULL, NULL }
};

GType
gimp_selection_control_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpSelectionControl", gimp_selection_control_enum_values);

  return enum_type;
}


static const GEnumValue gimp_channel_type_enum_values[] =
{
  { GIMP_RED_CHANNEL, "GIMP_RED_CHANNEL", "red-channel" },
  { GIMP_GREEN_CHANNEL, "GIMP_GREEN_CHANNEL", "green-channel" },
  { GIMP_BLUE_CHANNEL, "GIMP_BLUE_CHANNEL", "blue-channel" },
  { GIMP_GRAY_CHANNEL, "GIMP_GRAY_CHANNEL", "gray-channel" },
  { GIMP_INDEXED_CHANNEL, "GIMP_INDEXED_CHANNEL", "indexed-channel" },
  { GIMP_ALPHA_CHANNEL, "GIMP_ALPHA_CHANNEL", "alpha-channel" },
  { 0, NULL, NULL }
};

GType
gimp_channel_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpChannelType", gimp_channel_type_enum_values);

  return enum_type;
}


static const GEnumValue gimp_gradient_segment_type_enum_values[] =
{
  { GIMP_GRAD_LINEAR, "GIMP_GRAD_LINEAR", "linear" },
  { GIMP_GRAD_CURVED, "GIMP_GRAD_CURVED", "curved" },
  { GIMP_GRAD_SINE, "GIMP_GRAD_SINE", "sine" },
  { GIMP_GRAD_SPHERE_INCREASING, "GIMP_GRAD_SPHERE_INCREASING", "sphere-increasing" },
  { GIMP_GRAD_SPHERE_DECREASING, "GIMP_GRAD_SPHERE_DECREASING", "sphere-decreasing" },
  { 0, NULL, NULL }
};

GType
gimp_gradient_segment_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpGradientSegmentType", gimp_gradient_segment_type_enum_values);

  return enum_type;
}


static const GEnumValue gimp_gradient_segment_color_enum_values[] =
{
  { GIMP_GRAD_RGB, "GIMP_GRAD_RGB", "rgb" },
  { GIMP_GRAD_HSV_CCW, "GIMP_GRAD_HSV_CCW", "hsv-ccw" },
  { GIMP_GRAD_HSV_CW, "GIMP_GRAD_HSV_CW", "hsv-cw" },
  { 0, NULL, NULL }
};

GType
gimp_gradient_segment_color_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpGradientSegmentColor", gimp_gradient_segment_color_enum_values);

  return enum_type;
}


static const GEnumValue gimp_blend_mode_enum_values[] =
{
  { GIMP_FG_BG_RGB_MODE, "GIMP_FG_BG_RGB_MODE", "fg-bg-rgb-mode" },
  { GIMP_FG_BG_HSV_MODE, "GIMP_FG_BG_HSV_MODE", "fg-bg-hsv-mode" },
  { GIMP_FG_TRANS_MODE, "GIMP_FG_TRANS_MODE", "fg-trans-mode" },
  { GIMP_CUSTOM_MODE, "GIMP_CUSTOM_MODE", "custom-mode" },
  { 0, NULL, NULL }
};

GType
gimp_blend_mode_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpBlendMode", gimp_blend_mode_enum_values);

  return enum_type;
}


static const GEnumValue gimp_transform_direction_enum_values[] =
{
  { GIMP_TRANSFORM_FORWARD, "GIMP_TRANSFORM_FORWARD", "forward" },
  { GIMP_TRANSFORM_BACKWARD, "GIMP_TRANSFORM_BACKWARD", "backward" },
  { 0, NULL, NULL }
};

GType
gimp_transform_direction_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpTransformDirection", gimp_transform_direction_enum_values);

  return enum_type;
}


/* Generated data ends here */

