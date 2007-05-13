
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "gimpbasetypes.h"
#include "libgimp/libgimp-intl.h"

/* enumerations from "./gimpbaseenums.h" */
GType
gimp_add_mask_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_ADD_WHITE_MASK, "GIMP_ADD_WHITE_MASK", "white-mask" },
    { GIMP_ADD_BLACK_MASK, "GIMP_ADD_BLACK_MASK", "black-mask" },
    { GIMP_ADD_ALPHA_MASK, "GIMP_ADD_ALPHA_MASK", "alpha-mask" },
    { GIMP_ADD_ALPHA_TRANSFER_MASK, "GIMP_ADD_ALPHA_TRANSFER_MASK", "alpha-transfer-mask" },
    { GIMP_ADD_SELECTION_MASK, "GIMP_ADD_SELECTION_MASK", "selection-mask" },
    { GIMP_ADD_COPY_MASK, "GIMP_ADD_COPY_MASK", "copy-mask" },
    { GIMP_ADD_CHANNEL_MASK, "GIMP_ADD_CHANNEL_MASK", "channel-mask" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_ADD_WHITE_MASK, N_("_White (full opacity)"), NULL },
    { GIMP_ADD_BLACK_MASK, N_("_Black (full transparency)"), NULL },
    { GIMP_ADD_ALPHA_MASK, N_("Layer's _alpha channel"), NULL },
    { GIMP_ADD_ALPHA_TRANSFER_MASK, N_("_Transfer layer's alpha channel"), NULL },
    { GIMP_ADD_SELECTION_MASK, N_("_Selection"), NULL },
    { GIMP_ADD_COPY_MASK, N_("_Grayscale copy of layer"), NULL },
    { GIMP_ADD_CHANNEL_MASK, N_("C_hannel"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpAddMaskType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_blend_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_FG_BG_RGB_MODE, "GIMP_FG_BG_RGB_MODE", "fg-bg-rgb-mode" },
    { GIMP_FG_BG_HSV_MODE, "GIMP_FG_BG_HSV_MODE", "fg-bg-hsv-mode" },
    { GIMP_FG_TRANSPARENT_MODE, "GIMP_FG_TRANSPARENT_MODE", "fg-transparent-mode" },
    { GIMP_CUSTOM_MODE, "GIMP_CUSTOM_MODE", "custom-mode" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_FG_BG_RGB_MODE, N_("FG to BG (RGB)"), NULL },
    { GIMP_FG_BG_HSV_MODE, N_("FG to BG (HSV)"), NULL },
    { GIMP_FG_TRANSPARENT_MODE, N_("FG to transparent"), NULL },
    { GIMP_CUSTOM_MODE, N_("Custom gradient"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpBlendMode", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_bucket_fill_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_FG_BUCKET_FILL, "GIMP_FG_BUCKET_FILL", "fg-bucket-fill" },
    { GIMP_BG_BUCKET_FILL, "GIMP_BG_BUCKET_FILL", "bg-bucket-fill" },
    { GIMP_PATTERN_BUCKET_FILL, "GIMP_PATTERN_BUCKET_FILL", "pattern-bucket-fill" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_FG_BUCKET_FILL, N_("FG color fill"), NULL },
    { GIMP_BG_BUCKET_FILL, N_("BG color fill"), NULL },
    { GIMP_PATTERN_BUCKET_FILL, N_("Pattern fill"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpBucketFillMode", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_channel_ops_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_CHANNEL_OP_ADD, "GIMP_CHANNEL_OP_ADD", "add" },
    { GIMP_CHANNEL_OP_SUBTRACT, "GIMP_CHANNEL_OP_SUBTRACT", "subtract" },
    { GIMP_CHANNEL_OP_REPLACE, "GIMP_CHANNEL_OP_REPLACE", "replace" },
    { GIMP_CHANNEL_OP_INTERSECT, "GIMP_CHANNEL_OP_INTERSECT", "intersect" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_CHANNEL_OP_ADD, N_("Add to the current selection"), NULL },
    { GIMP_CHANNEL_OP_SUBTRACT, N_("Subtract from the current selection"), NULL },
    { GIMP_CHANNEL_OP_REPLACE, N_("Replace the current selection"), NULL },
    { GIMP_CHANNEL_OP_INTERSECT, N_("Intersect with the current selection"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpChannelOps", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_channel_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_RED_CHANNEL, "GIMP_RED_CHANNEL", "red-channel" },
    { GIMP_GREEN_CHANNEL, "GIMP_GREEN_CHANNEL", "green-channel" },
    { GIMP_BLUE_CHANNEL, "GIMP_BLUE_CHANNEL", "blue-channel" },
    { GIMP_GRAY_CHANNEL, "GIMP_GRAY_CHANNEL", "gray-channel" },
    { GIMP_INDEXED_CHANNEL, "GIMP_INDEXED_CHANNEL", "indexed-channel" },
    { GIMP_ALPHA_CHANNEL, "GIMP_ALPHA_CHANNEL", "alpha-channel" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_RED_CHANNEL, N_("Red"), NULL },
    { GIMP_GREEN_CHANNEL, N_("Green"), NULL },
    { GIMP_BLUE_CHANNEL, N_("Blue"), NULL },
    { GIMP_GRAY_CHANNEL, N_("Gray"), NULL },
    { GIMP_INDEXED_CHANNEL, N_("Indexed"), NULL },
    { GIMP_ALPHA_CHANNEL, N_("Alpha"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpChannelType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_check_size_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_CHECK_SIZE_SMALL_CHECKS, "GIMP_CHECK_SIZE_SMALL_CHECKS", "small-checks" },
    { GIMP_CHECK_SIZE_MEDIUM_CHECKS, "GIMP_CHECK_SIZE_MEDIUM_CHECKS", "medium-checks" },
    { GIMP_CHECK_SIZE_LARGE_CHECKS, "GIMP_CHECK_SIZE_LARGE_CHECKS", "large-checks" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_CHECK_SIZE_SMALL_CHECKS, N_("Small"), NULL },
    { GIMP_CHECK_SIZE_MEDIUM_CHECKS, N_("Medium"), NULL },
    { GIMP_CHECK_SIZE_LARGE_CHECKS, N_("Large"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpCheckSize", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_check_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_CHECK_TYPE_LIGHT_CHECKS, "GIMP_CHECK_TYPE_LIGHT_CHECKS", "light-checks" },
    { GIMP_CHECK_TYPE_GRAY_CHECKS, "GIMP_CHECK_TYPE_GRAY_CHECKS", "gray-checks" },
    { GIMP_CHECK_TYPE_DARK_CHECKS, "GIMP_CHECK_TYPE_DARK_CHECKS", "dark-checks" },
    { GIMP_CHECK_TYPE_WHITE_ONLY, "GIMP_CHECK_TYPE_WHITE_ONLY", "white-only" },
    { GIMP_CHECK_TYPE_GRAY_ONLY, "GIMP_CHECK_TYPE_GRAY_ONLY", "gray-only" },
    { GIMP_CHECK_TYPE_BLACK_ONLY, "GIMP_CHECK_TYPE_BLACK_ONLY", "black-only" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_CHECK_TYPE_LIGHT_CHECKS, N_("Light checks"), NULL },
    { GIMP_CHECK_TYPE_GRAY_CHECKS, N_("Mid-tone checks"), NULL },
    { GIMP_CHECK_TYPE_DARK_CHECKS, N_("Dark checks"), NULL },
    { GIMP_CHECK_TYPE_WHITE_ONLY, N_("White only"), NULL },
    { GIMP_CHECK_TYPE_GRAY_ONLY, N_("Gray only"), NULL },
    { GIMP_CHECK_TYPE_BLACK_ONLY, N_("Black only"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpCheckType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_clone_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_IMAGE_CLONE, "GIMP_IMAGE_CLONE", "image-clone" },
    { GIMP_PATTERN_CLONE, "GIMP_PATTERN_CLONE", "pattern-clone" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_IMAGE_CLONE, N_("Image"), NULL },
    { GIMP_PATTERN_CLONE, N_("Pattern"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpCloneType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_desaturate_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_DESATURATE_LIGHTNESS, "GIMP_DESATURATE_LIGHTNESS", "lightness" },
    { GIMP_DESATURATE_LUMINOSITY, "GIMP_DESATURATE_LUMINOSITY", "luminosity" },
    { GIMP_DESATURATE_AVERAGE, "GIMP_DESATURATE_AVERAGE", "average" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_DESATURATE_LIGHTNESS, N_("Lightness"), NULL },
    { GIMP_DESATURATE_LUMINOSITY, N_("Luminosity"), NULL },
    { GIMP_DESATURATE_AVERAGE, N_("Average"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpDesaturateMode", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_dodge_burn_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_DODGE, "GIMP_DODGE", "dodge" },
    { GIMP_BURN, "GIMP_BURN", "burn" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_DODGE, N_("Dodge"), NULL },
    { GIMP_BURN, N_("Burn"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpDodgeBurnType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_foreground_extract_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_FOREGROUND_EXTRACT_SIOX, "GIMP_FOREGROUND_EXTRACT_SIOX", "siox" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_FOREGROUND_EXTRACT_SIOX, "GIMP_FOREGROUND_EXTRACT_SIOX", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpForegroundExtractMode", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_gradient_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_GRADIENT_LINEAR, "GIMP_GRADIENT_LINEAR", "linear" },
    { GIMP_GRADIENT_BILINEAR, "GIMP_GRADIENT_BILINEAR", "bilinear" },
    { GIMP_GRADIENT_RADIAL, "GIMP_GRADIENT_RADIAL", "radial" },
    { GIMP_GRADIENT_SQUARE, "GIMP_GRADIENT_SQUARE", "square" },
    { GIMP_GRADIENT_CONICAL_SYMMETRIC, "GIMP_GRADIENT_CONICAL_SYMMETRIC", "conical-symmetric" },
    { GIMP_GRADIENT_CONICAL_ASYMMETRIC, "GIMP_GRADIENT_CONICAL_ASYMMETRIC", "conical-asymmetric" },
    { GIMP_GRADIENT_SHAPEBURST_ANGULAR, "GIMP_GRADIENT_SHAPEBURST_ANGULAR", "shapeburst-angular" },
    { GIMP_GRADIENT_SHAPEBURST_SPHERICAL, "GIMP_GRADIENT_SHAPEBURST_SPHERICAL", "shapeburst-spherical" },
    { GIMP_GRADIENT_SHAPEBURST_DIMPLED, "GIMP_GRADIENT_SHAPEBURST_DIMPLED", "shapeburst-dimpled" },
    { GIMP_GRADIENT_SPIRAL_CLOCKWISE, "GIMP_GRADIENT_SPIRAL_CLOCKWISE", "spiral-clockwise" },
    { GIMP_GRADIENT_SPIRAL_ANTICLOCKWISE, "GIMP_GRADIENT_SPIRAL_ANTICLOCKWISE", "spiral-anticlockwise" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_GRADIENT_LINEAR, N_("gradient|Linear"), NULL },
    { GIMP_GRADIENT_BILINEAR, N_("Bi-linear"), NULL },
    { GIMP_GRADIENT_RADIAL, N_("Radial"), NULL },
    { GIMP_GRADIENT_SQUARE, N_("Square"), NULL },
    { GIMP_GRADIENT_CONICAL_SYMMETRIC, N_("Conical (sym)"), NULL },
    { GIMP_GRADIENT_CONICAL_ASYMMETRIC, N_("Conical (asym)"), NULL },
    { GIMP_GRADIENT_SHAPEBURST_ANGULAR, N_("Shaped (angular)"), NULL },
    { GIMP_GRADIENT_SHAPEBURST_SPHERICAL, N_("Shaped (spherical)"), NULL },
    { GIMP_GRADIENT_SHAPEBURST_DIMPLED, N_("Shaped (dimpled)"), NULL },
    { GIMP_GRADIENT_SPIRAL_CLOCKWISE, N_("Spiral (cw)"), NULL },
    { GIMP_GRADIENT_SPIRAL_ANTICLOCKWISE, N_("Spiral (ccw)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpGradientType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_grid_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_GRID_DOTS, "GIMP_GRID_DOTS", "dots" },
    { GIMP_GRID_INTERSECTIONS, "GIMP_GRID_INTERSECTIONS", "intersections" },
    { GIMP_GRID_ON_OFF_DASH, "GIMP_GRID_ON_OFF_DASH", "on-off-dash" },
    { GIMP_GRID_DOUBLE_DASH, "GIMP_GRID_DOUBLE_DASH", "double-dash" },
    { GIMP_GRID_SOLID, "GIMP_GRID_SOLID", "solid" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_GRID_DOTS, N_("Intersections (dots)"), NULL },
    { GIMP_GRID_INTERSECTIONS, N_("Intersections (crosshairs)"), NULL },
    { GIMP_GRID_ON_OFF_DASH, N_("Dashed"), NULL },
    { GIMP_GRID_DOUBLE_DASH, N_("Double dashed"), NULL },
    { GIMP_GRID_SOLID, N_("Solid"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpGridStyle", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_icon_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_ICON_TYPE_STOCK_ID, "GIMP_ICON_TYPE_STOCK_ID", "stock-id" },
    { GIMP_ICON_TYPE_INLINE_PIXBUF, "GIMP_ICON_TYPE_INLINE_PIXBUF", "inline-pixbuf" },
    { GIMP_ICON_TYPE_IMAGE_FILE, "GIMP_ICON_TYPE_IMAGE_FILE", "image-file" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_ICON_TYPE_STOCK_ID, N_("Stock ID"), NULL },
    { GIMP_ICON_TYPE_INLINE_PIXBUF, N_("Inline pixbuf"), NULL },
    { GIMP_ICON_TYPE_IMAGE_FILE, N_("Image file"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpIconType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_image_base_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_RGB, "GIMP_RGB", "rgb" },
    { GIMP_GRAY, "GIMP_GRAY", "gray" },
    { GIMP_INDEXED, "GIMP_INDEXED", "indexed" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_RGB, N_("RGB color"), NULL },
    { GIMP_GRAY, N_("Grayscale"), NULL },
    { GIMP_INDEXED, N_("Indexed color"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpImageBaseType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_image_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_RGB_IMAGE, "GIMP_RGB_IMAGE", "rgb-image" },
    { GIMP_RGBA_IMAGE, "GIMP_RGBA_IMAGE", "rgba-image" },
    { GIMP_GRAY_IMAGE, "GIMP_GRAY_IMAGE", "gray-image" },
    { GIMP_GRAYA_IMAGE, "GIMP_GRAYA_IMAGE", "graya-image" },
    { GIMP_INDEXED_IMAGE, "GIMP_INDEXED_IMAGE", "indexed-image" },
    { GIMP_INDEXEDA_IMAGE, "GIMP_INDEXEDA_IMAGE", "indexeda-image" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_RGB_IMAGE, N_("RGB"), NULL },
    { GIMP_RGBA_IMAGE, N_("RGB-alpha"), NULL },
    { GIMP_GRAY_IMAGE, N_("Grayscale"), NULL },
    { GIMP_GRAYA_IMAGE, N_("Grayscale-alpha"), NULL },
    { GIMP_INDEXED_IMAGE, N_("Indexed"), NULL },
    { GIMP_INDEXEDA_IMAGE, N_("Indexed-alpha"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpImageType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
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
    { GIMP_INTERPOLATION_LANCZOS, "GIMP_INTERPOLATION_LANCZOS", "lanczos" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_INTERPOLATION_NONE, N_("interpolation|None"), NULL },
    { GIMP_INTERPOLATION_LINEAR, N_("interpolation|Linear"), NULL },
    { GIMP_INTERPOLATION_CUBIC, N_("Cubic"), NULL },
    { GIMP_INTERPOLATION_LANCZOS, N_("Sinc (Lanczos3)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpInterpolationType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_paint_application_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_PAINT_CONSTANT, "GIMP_PAINT_CONSTANT", "constant" },
    { GIMP_PAINT_INCREMENTAL, "GIMP_PAINT_INCREMENTAL", "incremental" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_PAINT_CONSTANT, N_("Constant"), NULL },
    { GIMP_PAINT_INCREMENTAL, N_("Incremental"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpPaintApplicationMode", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_repeat_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_REPEAT_NONE, "GIMP_REPEAT_NONE", "none" },
    { GIMP_REPEAT_SAWTOOTH, "GIMP_REPEAT_SAWTOOTH", "sawtooth" },
    { GIMP_REPEAT_TRIANGULAR, "GIMP_REPEAT_TRIANGULAR", "triangular" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_REPEAT_NONE, N_("None"), NULL },
    { GIMP_REPEAT_SAWTOOTH, N_("Sawtooth wave"), NULL },
    { GIMP_REPEAT_TRIANGULAR, N_("Triangular wave"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpRepeatMode", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_run_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_RUN_INTERACTIVE, "GIMP_RUN_INTERACTIVE", "interactive" },
    { GIMP_RUN_NONINTERACTIVE, "GIMP_RUN_NONINTERACTIVE", "noninteractive" },
    { GIMP_RUN_WITH_LAST_VALS, "GIMP_RUN_WITH_LAST_VALS", "with-last-vals" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_RUN_INTERACTIVE, N_("Run interactively"), NULL },
    { GIMP_RUN_NONINTERACTIVE, N_("Run non-interactively"), NULL },
    { GIMP_RUN_WITH_LAST_VALS, N_("Run with last used values"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpRunMode", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_size_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_PIXELS, "GIMP_PIXELS", "pixels" },
    { GIMP_POINTS, "GIMP_POINTS", "points" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_PIXELS, N_("Pixels"), NULL },
    { GIMP_POINTS, N_("Points"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpSizeType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
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
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_transform_direction_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_TRANSFORM_FORWARD, "GIMP_TRANSFORM_FORWARD", "forward" },
    { GIMP_TRANSFORM_BACKWARD, "GIMP_TRANSFORM_BACKWARD", "backward" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_TRANSFORM_FORWARD, N_("Normal (Forward)"), NULL },
    { GIMP_TRANSFORM_BACKWARD, N_("Corrective (Backward)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpTransformDirection", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_transform_resize_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_TRANSFORM_RESIZE_ADJUST, "GIMP_TRANSFORM_RESIZE_ADJUST", "adjust" },
    { GIMP_TRANSFORM_RESIZE_CLIP, "GIMP_TRANSFORM_RESIZE_CLIP", "clip" },
    { GIMP_TRANSFORM_RESIZE_CROP, "GIMP_TRANSFORM_RESIZE_CROP", "crop" },
    { GIMP_TRANSFORM_RESIZE_CROP_WITH_ASPECT, "GIMP_TRANSFORM_RESIZE_CROP_WITH_ASPECT", "crop-with-aspect" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_TRANSFORM_RESIZE_ADJUST, N_("Adjust"), NULL },
    { GIMP_TRANSFORM_RESIZE_CLIP, N_("Clip"), NULL },
    { GIMP_TRANSFORM_RESIZE_CROP, N_("Crop to result"), NULL },
    { GIMP_TRANSFORM_RESIZE_CROP_WITH_ASPECT, N_("Crop with aspect"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpTransformResize", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_pdb_arg_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_PDB_INT32, "GIMP_PDB_INT32", "int32" },
    { GIMP_PDB_INT16, "GIMP_PDB_INT16", "int16" },
    { GIMP_PDB_INT8, "GIMP_PDB_INT8", "int8" },
    { GIMP_PDB_FLOAT, "GIMP_PDB_FLOAT", "float" },
    { GIMP_PDB_STRING, "GIMP_PDB_STRING", "string" },
    { GIMP_PDB_INT32ARRAY, "GIMP_PDB_INT32ARRAY", "int32array" },
    { GIMP_PDB_INT16ARRAY, "GIMP_PDB_INT16ARRAY", "int16array" },
    { GIMP_PDB_INT8ARRAY, "GIMP_PDB_INT8ARRAY", "int8array" },
    { GIMP_PDB_FLOATARRAY, "GIMP_PDB_FLOATARRAY", "floatarray" },
    { GIMP_PDB_STRINGARRAY, "GIMP_PDB_STRINGARRAY", "stringarray" },
    { GIMP_PDB_COLOR, "GIMP_PDB_COLOR", "color" },
    { GIMP_PDB_REGION, "GIMP_PDB_REGION", "region" },
    { GIMP_PDB_DISPLAY, "GIMP_PDB_DISPLAY", "display" },
    { GIMP_PDB_IMAGE, "GIMP_PDB_IMAGE", "image" },
    { GIMP_PDB_LAYER, "GIMP_PDB_LAYER", "layer" },
    { GIMP_PDB_CHANNEL, "GIMP_PDB_CHANNEL", "channel" },
    { GIMP_PDB_DRAWABLE, "GIMP_PDB_DRAWABLE", "drawable" },
    { GIMP_PDB_SELECTION, "GIMP_PDB_SELECTION", "selection" },
    { GIMP_PDB_BOUNDARY, "GIMP_PDB_BOUNDARY", "boundary" },
    { GIMP_PDB_VECTORS, "GIMP_PDB_VECTORS", "vectors" },
    { GIMP_PDB_PARASITE, "GIMP_PDB_PARASITE", "parasite" },
    { GIMP_PDB_STATUS, "GIMP_PDB_STATUS", "status" },
    { GIMP_PDB_END, "GIMP_PDB_END", "end" },
    { GIMP_PDB_PATH, "GIMP_PDB_PATH", "path" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_PDB_INT32, "GIMP_PDB_INT32", NULL },
    { GIMP_PDB_INT16, "GIMP_PDB_INT16", NULL },
    { GIMP_PDB_INT8, "GIMP_PDB_INT8", NULL },
    { GIMP_PDB_FLOAT, "GIMP_PDB_FLOAT", NULL },
    { GIMP_PDB_STRING, "GIMP_PDB_STRING", NULL },
    { GIMP_PDB_INT32ARRAY, "GIMP_PDB_INT32ARRAY", NULL },
    { GIMP_PDB_INT16ARRAY, "GIMP_PDB_INT16ARRAY", NULL },
    { GIMP_PDB_INT8ARRAY, "GIMP_PDB_INT8ARRAY", NULL },
    { GIMP_PDB_FLOATARRAY, "GIMP_PDB_FLOATARRAY", NULL },
    { GIMP_PDB_STRINGARRAY, "GIMP_PDB_STRINGARRAY", NULL },
    { GIMP_PDB_COLOR, "GIMP_PDB_COLOR", NULL },
    { GIMP_PDB_REGION, "GIMP_PDB_REGION", NULL },
    { GIMP_PDB_DISPLAY, "GIMP_PDB_DISPLAY", NULL },
    { GIMP_PDB_IMAGE, "GIMP_PDB_IMAGE", NULL },
    { GIMP_PDB_LAYER, "GIMP_PDB_LAYER", NULL },
    { GIMP_PDB_CHANNEL, "GIMP_PDB_CHANNEL", NULL },
    { GIMP_PDB_DRAWABLE, "GIMP_PDB_DRAWABLE", NULL },
    { GIMP_PDB_SELECTION, "GIMP_PDB_SELECTION", NULL },
    { GIMP_PDB_BOUNDARY, "GIMP_PDB_BOUNDARY", NULL },
    { GIMP_PDB_VECTORS, "GIMP_PDB_VECTORS", NULL },
    { GIMP_PDB_PARASITE, "GIMP_PDB_PARASITE", NULL },
    { GIMP_PDB_STATUS, "GIMP_PDB_STATUS", NULL },
    { GIMP_PDB_END, "GIMP_PDB_END", NULL },
    { GIMP_PDB_PATH, "GIMP_PDB_PATH", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpPDBArgType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_pdb_proc_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_INTERNAL, "GIMP_INTERNAL", "internal" },
    { GIMP_PLUGIN, "GIMP_PLUGIN", "plugin" },
    { GIMP_EXTENSION, "GIMP_EXTENSION", "extension" },
    { GIMP_TEMPORARY, "GIMP_TEMPORARY", "temporary" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_INTERNAL, N_("Internal GIMP procedure"), NULL },
    { GIMP_PLUGIN, N_("GIMP Plug-In"), NULL },
    { GIMP_EXTENSION, N_("GIMP Extension"), NULL },
    { GIMP_TEMPORARY, N_("Temporary Procedure"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpPDBProcType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_pdb_status_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_PDB_EXECUTION_ERROR, "GIMP_PDB_EXECUTION_ERROR", "execution-error" },
    { GIMP_PDB_CALLING_ERROR, "GIMP_PDB_CALLING_ERROR", "calling-error" },
    { GIMP_PDB_PASS_THROUGH, "GIMP_PDB_PASS_THROUGH", "pass-through" },
    { GIMP_PDB_SUCCESS, "GIMP_PDB_SUCCESS", "success" },
    { GIMP_PDB_CANCEL, "GIMP_PDB_CANCEL", "cancel" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_PDB_EXECUTION_ERROR, "GIMP_PDB_EXECUTION_ERROR", NULL },
    { GIMP_PDB_CALLING_ERROR, "GIMP_PDB_CALLING_ERROR", NULL },
    { GIMP_PDB_PASS_THROUGH, "GIMP_PDB_PASS_THROUGH", NULL },
    { GIMP_PDB_SUCCESS, "GIMP_PDB_SUCCESS", NULL },
    { GIMP_PDB_CANCEL, "GIMP_PDB_CANCEL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpPDBStatusType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_message_handler_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_MESSAGE_BOX, "GIMP_MESSAGE_BOX", "message-box" },
    { GIMP_CONSOLE, "GIMP_CONSOLE", "console" },
    { GIMP_ERROR_CONSOLE, "GIMP_ERROR_CONSOLE", "error-console" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_MESSAGE_BOX, "GIMP_MESSAGE_BOX", NULL },
    { GIMP_CONSOLE, "GIMP_CONSOLE", NULL },
    { GIMP_ERROR_CONSOLE, "GIMP_ERROR_CONSOLE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpMessageHandlerType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_stack_trace_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_STACK_TRACE_NEVER, "GIMP_STACK_TRACE_NEVER", "never" },
    { GIMP_STACK_TRACE_QUERY, "GIMP_STACK_TRACE_QUERY", "query" },
    { GIMP_STACK_TRACE_ALWAYS, "GIMP_STACK_TRACE_ALWAYS", "always" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_STACK_TRACE_NEVER, "GIMP_STACK_TRACE_NEVER", NULL },
    { GIMP_STACK_TRACE_QUERY, "GIMP_STACK_TRACE_QUERY", NULL },
    { GIMP_STACK_TRACE_ALWAYS, "GIMP_STACK_TRACE_ALWAYS", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpStackTraceMode", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_progress_command_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_PROGRESS_COMMAND_START, "GIMP_PROGRESS_COMMAND_START", "start" },
    { GIMP_PROGRESS_COMMAND_END, "GIMP_PROGRESS_COMMAND_END", "end" },
    { GIMP_PROGRESS_COMMAND_SET_TEXT, "GIMP_PROGRESS_COMMAND_SET_TEXT", "set-text" },
    { GIMP_PROGRESS_COMMAND_SET_VALUE, "GIMP_PROGRESS_COMMAND_SET_VALUE", "set-value" },
    { GIMP_PROGRESS_COMMAND_PULSE, "GIMP_PROGRESS_COMMAND_PULSE", "pulse" },
    { GIMP_PROGRESS_COMMAND_GET_WINDOW, "GIMP_PROGRESS_COMMAND_GET_WINDOW", "get-window" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_PROGRESS_COMMAND_START, "GIMP_PROGRESS_COMMAND_START", NULL },
    { GIMP_PROGRESS_COMMAND_END, "GIMP_PROGRESS_COMMAND_END", NULL },
    { GIMP_PROGRESS_COMMAND_SET_TEXT, "GIMP_PROGRESS_COMMAND_SET_TEXT", NULL },
    { GIMP_PROGRESS_COMMAND_SET_VALUE, "GIMP_PROGRESS_COMMAND_SET_VALUE", NULL },
    { GIMP_PROGRESS_COMMAND_PULSE, "GIMP_PROGRESS_COMMAND_PULSE", NULL },
    { GIMP_PROGRESS_COMMAND_GET_WINDOW, "GIMP_PROGRESS_COMMAND_GET_WINDOW", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpProgressCommand", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_user_directory_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_USER_DIRECTORY_DESKTOP, "GIMP_USER_DIRECTORY_DESKTOP", "desktop" },
    { GIMP_USER_DIRECTORY_DOCUMENTS, "GIMP_USER_DIRECTORY_DOCUMENTS", "documents" },
    { GIMP_USER_DIRECTORY_MUSIC, "GIMP_USER_DIRECTORY_MUSIC", "music" },
    { GIMP_USER_DIRECTORY_PICTURES, "GIMP_USER_DIRECTORY_PICTURES", "pictures" },
    { GIMP_USER_DIRECTORY_TEMPLATES, "GIMP_USER_DIRECTORY_TEMPLATES", "templates" },
    { GIMP_USER_DIRECTORY_VIDEOS, "GIMP_USER_DIRECTORY_VIDEOS", "videos" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_USER_DIRECTORY_DESKTOP, "GIMP_USER_DIRECTORY_DESKTOP", NULL },
    { GIMP_USER_DIRECTORY_DOCUMENTS, "GIMP_USER_DIRECTORY_DOCUMENTS", NULL },
    { GIMP_USER_DIRECTORY_MUSIC, "GIMP_USER_DIRECTORY_MUSIC", NULL },
    { GIMP_USER_DIRECTORY_PICTURES, "GIMP_USER_DIRECTORY_PICTURES", NULL },
    { GIMP_USER_DIRECTORY_TEMPLATES, "GIMP_USER_DIRECTORY_TEMPLATES", NULL },
    { GIMP_USER_DIRECTORY_VIDEOS, "GIMP_USER_DIRECTORY_VIDEOS", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpUserDirectory", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_vectors_stroke_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_VECTORS_STROKE_TYPE_BEZIER, "GIMP_VECTORS_STROKE_TYPE_BEZIER", "bezier" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_VECTORS_STROKE_TYPE_BEZIER, "GIMP_VECTORS_STROKE_TYPE_BEZIER", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpVectorsStrokeType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

