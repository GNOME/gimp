
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "core-enums.h"
#include "libgimp/gimpintl.h"

/* enumerations from "./core-enums.h" */

static const GEnumValue gimp_orientation_type_enum_values[] =
{
  { GIMP_HORIZONTAL, N_("Horizontal"), "horizontal" },
  { GIMP_VERTICAL, N_("Vertical"), "vertical" },
  { GIMP_UNKNOWN, N_("Unknown"), "unknown" },
  { 0, NULL, NULL }
};

GType
gimp_orientation_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpOrientationType", gimp_orientation_type_enum_values);

  return enum_type;
}


static const GEnumValue gimp_blend_mode_enum_values[] =
{
  { GIMP_FG_BG_RGB_MODE, N_("FG to BG (RGB)"), "fg-bg-rgb-mode" },
  { GIMP_FG_BG_HSV_MODE, N_("FG to BG (HSV)"), "fg-bg-hsv-mode" },
  { GIMP_FG_TRANSPARENT_MODE, N_("FG to Transparent"), "fg-transparent-mode" },
  { GIMP_CUSTOM_MODE, N_("Custom Gradient"), "custom-mode" },
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


static const GEnumValue gimp_bucket_fill_mode_enum_values[] =
{
  { GIMP_FG_BUCKET_FILL, N_("FG Color Fill"), "fg-bucket-fill" },
  { GIMP_BG_BUCKET_FILL, N_("BG Color Fill"), "bg-bucket-fill" },
  { GIMP_PATTERN_BUCKET_FILL, N_("Pattern Fill"), "pattern-bucket-fill" },
  { 0, NULL, NULL }
};

GType
gimp_bucket_fill_mode_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpBucketFillMode", gimp_bucket_fill_mode_enum_values);

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


static const GEnumValue gimp_convert_dither_type_enum_values[] =
{
  { GIMP_NO_DITHER, N_("No Color Dithering"), "no-dither" },
  { GIMP_FS_DITHER, N_("Floyd-Steinberg Color Dithering (Normal)"), "fs-dither" },
  { GIMP_FSLOWBLEED_DITHER, N_("Floyd-Steinberg Color Dithering (Reduced Color Bleeding)"), "fslowbleed-dither" },
  { GIMP_FIXED_DITHER, N_("Positioned Color Dithering"), "fixed-dither" },
  { 0, NULL, NULL }
};

GType
gimp_convert_dither_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpConvertDitherType", gimp_convert_dither_type_enum_values);

  return enum_type;
}


static const GEnumValue gimp_gravity_type_enum_values[] =
{
  { GIMP_GRAVITY_NONE, "GIMP_GRAVITY_NONE", "none" },
  { GIMP_GRAVITY_NORTH_WEST, "GIMP_GRAVITY_NORTH_WEST", "north-west" },
  { GIMP_GRAVITY_NORTH, "GIMP_GRAVITY_NORTH", "north" },
  { GIMP_GRAVITY_NORTH_EAST, "GIMP_GRAVITY_NORTH_EAST", "north-east" },
  { GIMP_GRAVITY_WEST, "GIMP_GRAVITY_WEST", "west" },
  { GIMP_GRAVITY_CENTER, "GIMP_GRAVITY_CENTER", "center" },
  { GIMP_GRAVITY_EAST, "GIMP_GRAVITY_EAST", "east" },
  { GIMP_GRAVITY_SOUTH_WEST, "GIMP_GRAVITY_SOUTH_WEST", "south-west" },
  { GIMP_GRAVITY_SOUTH, "GIMP_GRAVITY_SOUTH", "south" },
  { GIMP_GRAVITY_SOUTH_EAST, "GIMP_GRAVITY_SOUTH_EAST", "south-east" },
  { 0, NULL, NULL }
};

GType
gimp_gravity_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpGravityType", gimp_gravity_type_enum_values);

  return enum_type;
}


static const GEnumValue gimp_fill_type_enum_values[] =
{
  { GIMP_FOREGROUND_FILL, N_("Foreground"), "foreground-fill" },
  { GIMP_BACKGROUND_FILL, N_("Background"), "background-fill" },
  { GIMP_WHITE_FILL, N_("White"), "white-fill" },
  { GIMP_TRANSPARENT_FILL, N_("Transparent"), "transparent-fill" },
  { GIMP_NO_FILL, N_("None"), "no-fill" },
  { 0, NULL, NULL }
};

GType
gimp_fill_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpFillType", gimp_fill_type_enum_values);

  return enum_type;
}


static const GEnumValue gimp_gradient_type_enum_values[] =
{
  { GIMP_LINEAR, N_("Linear"), "linear" },
  { GIMP_BILINEAR, N_("Bi-Linear"), "bilinear" },
  { GIMP_RADIAL, N_("Radial"), "radial" },
  { GIMP_SQUARE, N_("Square"), "square" },
  { GIMP_CONICAL_SYMMETRIC, N_("Conical (symmetric)"), "conical-symmetric" },
  { GIMP_CONICAL_ASYMMETRIC, N_("Conical (asymmetric)"), "conical-asymmetric" },
  { GIMP_SHAPEBURST_ANGULAR, N_("Shapeburst (angular)"), "shapeburst-angular" },
  { GIMP_SHAPEBURST_SPHERICAL, N_("Shapeburst (spherical)"), "shapeburst-spherical" },
  { GIMP_SHAPEBURST_DIMPLED, N_("Shapeburst (dimpled)"), "shapeburst-dimpled" },
  { GIMP_SPIRAL_CLOCKWISE, N_("Spiral (clockwise)"), "spiral-clockwise" },
  { GIMP_SPIRAL_ANTICLOCKWISE, N_("Spiral (anticlockwise)"), "spiral-anticlockwise" },
  { 0, NULL, NULL }
};

GType
gimp_gradient_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpGradientType", gimp_gradient_type_enum_values);

  return enum_type;
}


static const GEnumValue gimp_image_base_type_enum_values[] =
{
  { GIMP_RGB, N_("RGB"), "rgb" },
  { GIMP_GRAY, N_("Grayscale"), "gray" },
  { GIMP_INDEXED, N_("Indexed"), "indexed" },
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


static const GEnumValue gimp_image_type_enum_values[] =
{
  { GIMP_RGB_IMAGE, N_("RGB"), "rgb-image" },
  { GIMP_RGBA_IMAGE, N_("RGB-Alpha"), "rgba-image" },
  { GIMP_GRAY_IMAGE, N_("Grayscale"), "gray-image" },
  { GIMP_GRAYA_IMAGE, N_("Grayscale-Alpha"), "graya-image" },
  { GIMP_INDEXED_IMAGE, N_("Indexed"), "indexed-image" },
  { GIMP_INDEXEDA_IMAGE, N_("Indexed-Alpha"), "indexeda-image" },
  { 0, NULL, NULL }
};

GType
gimp_image_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpImageType", gimp_image_type_enum_values);

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


static const GEnumValue gimp_repeat_mode_enum_values[] =
{
  { GIMP_REPEAT_NONE, N_("None"), "none" },
  { GIMP_REPEAT_SAWTOOTH, N_("Sawtooth Wave"), "sawtooth" },
  { GIMP_REPEAT_TRIANGULAR, N_("Triangular Wave"), "triangular" },
  { 0, NULL, NULL }
};

GType
gimp_repeat_mode_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpRepeatMode", gimp_repeat_mode_enum_values);

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


static const GEnumValue gimp_thumbnail_size_enum_values[] =
{
  { GIMP_THUMBNAIL_SIZE_NONE, N_("No Thumbnails"), "none" },
  { GIMP_THUMBNAIL_SIZE_NORMAL, N_("Normal (128x128)"), "normal" },
  { GIMP_THUMBNAIL_SIZE_LARGE, N_("Large (256x256)"), "large" },
  { 0, NULL, NULL }
};

GType
gimp_thumbnail_size_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpThumbnailSize", gimp_thumbnail_size_enum_values);

  return enum_type;
}


static const GEnumValue gimp_transform_direction_enum_values[] =
{
  { GIMP_TRANSFORM_FORWARD, N_("Forward (Traditional)"), "forward" },
  { GIMP_TRANSFORM_BACKWARD, N_("Backward (Corrective)"), "backward" },
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


static const GEnumValue gimp_channel_ops_enum_values[] =
{
  { GIMP_CHANNEL_OP_ADD, N_("Add to the current selection"), "add" },
  { GIMP_CHANNEL_OP_SUBTRACT, N_("Subtract from the current selection"), "subtract" },
  { GIMP_CHANNEL_OP_REPLACE, N_("Replace the current selection"), "replace" },
  { GIMP_CHANNEL_OP_INTERSECT, N_("Intersect with the current selection"), "intersect" },
  { 0, NULL, NULL }
};

GType
gimp_channel_ops_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpChannelOps", gimp_channel_ops_enum_values);

  return enum_type;
}


static const GEnumValue gimp_convert_palette_type_enum_values[] =
{
  { GIMP_MAKE_PALETTE, "GIMP_MAKE_PALETTE", "make-palette" },
  { GIMP_REUSE_PALETTE, "GIMP_REUSE_PALETTE", "reuse-palette" },
  { GIMP_WEB_PALETTE, "GIMP_WEB_PALETTE", "web-palette" },
  { GIMP_MONO_PALETTE, "GIMP_MONO_PALETTE", "mono-palette" },
  { GIMP_CUSTOM_PALETTE, "GIMP_CUSTOM_PALETTE", "custom-palette" },
  { 0, NULL, NULL }
};

GType
gimp_convert_palette_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpConvertPaletteType", gimp_convert_palette_type_enum_values);

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


/* Generated data ends here */

