
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#undef GIMP_DISABLE_DEPRECATED
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
    { GIMP_ADD_WHITE_MASK, NC_("add-mask-type", "_White (full opacity)"), NULL },
    { GIMP_ADD_BLACK_MASK, NC_("add-mask-type", "_Black (full transparency)"), NULL },
    { GIMP_ADD_ALPHA_MASK, NC_("add-mask-type", "Layer's _alpha channel"), NULL },
    { GIMP_ADD_ALPHA_TRANSFER_MASK, NC_("add-mask-type", "_Transfer layer's alpha channel"), NULL },
    { GIMP_ADD_SELECTION_MASK, NC_("add-mask-type", "_Selection"), NULL },
    { GIMP_ADD_COPY_MASK, NC_("add-mask-type", "_Grayscale copy of layer"), NULL },
    { GIMP_ADD_CHANNEL_MASK, NC_("add-mask-type", "C_hannel"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpAddMaskType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "add-mask-type");
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
    { GIMP_FG_BG_RGB_MODE, NC_("blend-mode", "FG to BG (RGB)"), NULL },
    { GIMP_FG_BG_HSV_MODE, NC_("blend-mode", "FG to BG (HSV)"), NULL },
    { GIMP_FG_TRANSPARENT_MODE, NC_("blend-mode", "FG to transparent"), NULL },
    { GIMP_CUSTOM_MODE, NC_("blend-mode", "Custom gradient"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpBlendMode", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "blend-mode");
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
    { GIMP_FG_BUCKET_FILL, NC_("bucket-fill-mode", "FG color fill"), NULL },
    { GIMP_BG_BUCKET_FILL, NC_("bucket-fill-mode", "BG color fill"), NULL },
    { GIMP_PATTERN_BUCKET_FILL, NC_("bucket-fill-mode", "Pattern fill"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpBucketFillMode", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "bucket-fill-mode");
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
    { GIMP_CHANNEL_OP_ADD, NC_("channel-ops", "Add to the current selection"), NULL },
    { GIMP_CHANNEL_OP_SUBTRACT, NC_("channel-ops", "Subtract from the current selection"), NULL },
    { GIMP_CHANNEL_OP_REPLACE, NC_("channel-ops", "Replace the current selection"), NULL },
    { GIMP_CHANNEL_OP_INTERSECT, NC_("channel-ops", "Intersect with the current selection"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpChannelOps", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "channel-ops");
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
    { GIMP_RED_CHANNEL, NC_("channel-type", "Red"), NULL },
    { GIMP_GREEN_CHANNEL, NC_("channel-type", "Green"), NULL },
    { GIMP_BLUE_CHANNEL, NC_("channel-type", "Blue"), NULL },
    { GIMP_GRAY_CHANNEL, NC_("channel-type", "Gray"), NULL },
    { GIMP_INDEXED_CHANNEL, NC_("channel-type", "Indexed"), NULL },
    { GIMP_ALPHA_CHANNEL, NC_("channel-type", "Alpha"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpChannelType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "channel-type");
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
    { GIMP_CHECK_SIZE_SMALL_CHECKS, NC_("check-size", "Small"), NULL },
    { GIMP_CHECK_SIZE_MEDIUM_CHECKS, NC_("check-size", "Medium"), NULL },
    { GIMP_CHECK_SIZE_LARGE_CHECKS, NC_("check-size", "Large"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpCheckSize", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "check-size");
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
    { GIMP_CHECK_TYPE_LIGHT_CHECKS, NC_("check-type", "Light checks"), NULL },
    { GIMP_CHECK_TYPE_GRAY_CHECKS, NC_("check-type", "Mid-tone checks"), NULL },
    { GIMP_CHECK_TYPE_DARK_CHECKS, NC_("check-type", "Dark checks"), NULL },
    { GIMP_CHECK_TYPE_WHITE_ONLY, NC_("check-type", "White only"), NULL },
    { GIMP_CHECK_TYPE_GRAY_ONLY, NC_("check-type", "Gray only"), NULL },
    { GIMP_CHECK_TYPE_BLACK_ONLY, NC_("check-type", "Black only"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpCheckType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "check-type");
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
    { GIMP_IMAGE_CLONE, NC_("clone-type", "Image"), NULL },
    { GIMP_PATTERN_CLONE, NC_("clone-type", "Pattern"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpCloneType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "clone-type");
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
    { GIMP_DESATURATE_LIGHTNESS, NC_("desaturate-mode", "Lightness"), NULL },
    { GIMP_DESATURATE_LUMINOSITY, NC_("desaturate-mode", "Luminosity"), NULL },
    { GIMP_DESATURATE_AVERAGE, NC_("desaturate-mode", "Average"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpDesaturateMode", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "desaturate-mode");
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
    { GIMP_DODGE, NC_("dodge-burn-type", "Dodge"), NULL },
    { GIMP_BURN, NC_("dodge-burn-type", "Burn"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpDodgeBurnType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "dodge-burn-type");
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

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpForegroundExtractMode", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "foreground-extract-mode");
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
    { GIMP_GRADIENT_LINEAR, NC_("gradient-type", "Linear"), NULL },
    { GIMP_GRADIENT_BILINEAR, NC_("gradient-type", "Bi-linear"), NULL },
    { GIMP_GRADIENT_RADIAL, NC_("gradient-type", "Radial"), NULL },
    { GIMP_GRADIENT_SQUARE, NC_("gradient-type", "Square"), NULL },
    { GIMP_GRADIENT_CONICAL_SYMMETRIC, NC_("gradient-type", "Conical (sym)"), NULL },
    { GIMP_GRADIENT_CONICAL_ASYMMETRIC, NC_("gradient-type", "Conical (asym)"), NULL },
    { GIMP_GRADIENT_SHAPEBURST_ANGULAR, NC_("gradient-type", "Shaped (angular)"), NULL },
    { GIMP_GRADIENT_SHAPEBURST_SPHERICAL, NC_("gradient-type", "Shaped (spherical)"), NULL },
    { GIMP_GRADIENT_SHAPEBURST_DIMPLED, NC_("gradient-type", "Shaped (dimpled)"), NULL },
    { GIMP_GRADIENT_SPIRAL_CLOCKWISE, NC_("gradient-type", "Spiral (cw)"), NULL },
    { GIMP_GRADIENT_SPIRAL_ANTICLOCKWISE, NC_("gradient-type", "Spiral (ccw)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpGradientType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "gradient-type");
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
    { GIMP_GRID_DOTS, NC_("grid-style", "Intersections (dots)"), NULL },
    { GIMP_GRID_INTERSECTIONS, NC_("grid-style", "Intersections (crosshairs)"), NULL },
    { GIMP_GRID_ON_OFF_DASH, NC_("grid-style", "Dashed"), NULL },
    { GIMP_GRID_DOUBLE_DASH, NC_("grid-style", "Double dashed"), NULL },
    { GIMP_GRID_SOLID, NC_("grid-style", "Solid"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpGridStyle", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "grid-style");
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
    { GIMP_ICON_TYPE_STOCK_ID, NC_("icon-type", "Stock ID"), NULL },
    { GIMP_ICON_TYPE_INLINE_PIXBUF, NC_("icon-type", "Inline pixbuf"), NULL },
    { GIMP_ICON_TYPE_IMAGE_FILE, NC_("icon-type", "Image file"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpIconType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "icon-type");
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
    { GIMP_RGB, NC_("image-base-type", "RGB color"), NULL },
    { GIMP_GRAY, NC_("image-base-type", "Grayscale"), NULL },
    { GIMP_INDEXED, NC_("image-base-type", "Indexed color"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpImageBaseType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "image-base-type");
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
    { GIMP_RGB_IMAGE, NC_("image-type", "RGB"), NULL },
    { GIMP_RGBA_IMAGE, NC_("image-type", "RGB-alpha"), NULL },
    { GIMP_GRAY_IMAGE, NC_("image-type", "Grayscale"), NULL },
    { GIMP_GRAYA_IMAGE, NC_("image-type", "Grayscale-alpha"), NULL },
    { GIMP_INDEXED_IMAGE, NC_("image-type", "Indexed"), NULL },
    { GIMP_INDEXEDA_IMAGE, NC_("image-type", "Indexed-alpha"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpImageType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "image-type");
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
    { GIMP_INTERPOLATION_NONE, NC_("interpolation-type", "None"), NULL },
    { GIMP_INTERPOLATION_LINEAR, NC_("interpolation-type", "Linear"), NULL },
    { GIMP_INTERPOLATION_CUBIC, NC_("interpolation-type", "Cubic"), NULL },
    { GIMP_INTERPOLATION_LANCZOS, NC_("interpolation-type", "Sinc (Lanczos3)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpInterpolationType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "interpolation-type");
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
    { GIMP_PAINT_CONSTANT, NC_("paint-application-mode", "Constant"), NULL },
    { GIMP_PAINT_INCREMENTAL, NC_("paint-application-mode", "Incremental"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpPaintApplicationMode", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "paint-application-mode");
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
    { GIMP_REPEAT_NONE, NC_("repeat-mode", "None"), NULL },
    { GIMP_REPEAT_SAWTOOTH, NC_("repeat-mode", "Sawtooth wave"), NULL },
    { GIMP_REPEAT_TRIANGULAR, NC_("repeat-mode", "Triangular wave"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpRepeatMode", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "repeat-mode");
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
    { GIMP_RUN_INTERACTIVE, NC_("run-mode", "Run interactively"), NULL },
    { GIMP_RUN_NONINTERACTIVE, NC_("run-mode", "Run non-interactively"), NULL },
    { GIMP_RUN_WITH_LAST_VALS, NC_("run-mode", "Run with last used values"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpRunMode", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "run-mode");
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
    { GIMP_PIXELS, NC_("size-type", "Pixels"), NULL },
    { GIMP_POINTS, NC_("size-type", "Points"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpSizeType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "size-type");
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
    { GIMP_SHADOWS, NC_("transfer-mode", "Shadows"), NULL },
    { GIMP_MIDTONES, NC_("transfer-mode", "Midtones"), NULL },
    { GIMP_HIGHLIGHTS, NC_("transfer-mode", "Highlights"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpTransferMode", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "transfer-mode");
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
    { GIMP_TRANSFORM_FORWARD, NC_("transform-direction", "Normal (Forward)"), NULL },
    { GIMP_TRANSFORM_BACKWARD, NC_("transform-direction", "Corrective (Backward)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpTransformDirection", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "transform-direction");
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
    { GIMP_TRANSFORM_RESIZE_ADJUST, NC_("transform-resize", "Adjust"), NULL },
    { GIMP_TRANSFORM_RESIZE_CLIP, NC_("transform-resize", "Clip"), NULL },
    { GIMP_TRANSFORM_RESIZE_CROP, NC_("transform-resize", "Crop to result"), NULL },
    { GIMP_TRANSFORM_RESIZE_CROP_WITH_ASPECT, NC_("transform-resize", "Crop with aspect"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpTransformResize", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "transform-resize");
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
    { GIMP_PDB_ITEM, "GIMP_PDB_ITEM", "item" },
    { GIMP_PDB_DISPLAY, "GIMP_PDB_DISPLAY", "display" },
    { GIMP_PDB_IMAGE, "GIMP_PDB_IMAGE", "image" },
    { GIMP_PDB_LAYER, "GIMP_PDB_LAYER", "layer" },
    { GIMP_PDB_CHANNEL, "GIMP_PDB_CHANNEL", "channel" },
    { GIMP_PDB_DRAWABLE, "GIMP_PDB_DRAWABLE", "drawable" },
    { GIMP_PDB_SELECTION, "GIMP_PDB_SELECTION", "selection" },
    { GIMP_PDB_COLORARRAY, "GIMP_PDB_COLORARRAY", "colorarray" },
    { GIMP_PDB_VECTORS, "GIMP_PDB_VECTORS", "vectors" },
    { GIMP_PDB_PARASITE, "GIMP_PDB_PARASITE", "parasite" },
    { GIMP_PDB_STATUS, "GIMP_PDB_STATUS", "status" },
    { GIMP_PDB_END, "GIMP_PDB_END", "end" },
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
    { GIMP_PDB_ITEM, "GIMP_PDB_ITEM", NULL },
    { GIMP_PDB_DISPLAY, "GIMP_PDB_DISPLAY", NULL },
    { GIMP_PDB_IMAGE, "GIMP_PDB_IMAGE", NULL },
    { GIMP_PDB_LAYER, "GIMP_PDB_LAYER", NULL },
    { GIMP_PDB_CHANNEL, "GIMP_PDB_CHANNEL", NULL },
    { GIMP_PDB_DRAWABLE, "GIMP_PDB_DRAWABLE", NULL },
    { GIMP_PDB_SELECTION, "GIMP_PDB_SELECTION", NULL },
    { GIMP_PDB_COLORARRAY, "GIMP_PDB_COLORARRAY", NULL },
    { GIMP_PDB_VECTORS, "GIMP_PDB_VECTORS", NULL },
    { GIMP_PDB_PARASITE, "GIMP_PDB_PARASITE", NULL },
    { GIMP_PDB_STATUS, "GIMP_PDB_STATUS", NULL },
    { GIMP_PDB_END, "GIMP_PDB_END", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpPDBArgType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "pdb-arg-type");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_pdb_error_handler_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_PDB_ERROR_HANDLER_INTERNAL, "GIMP_PDB_ERROR_HANDLER_INTERNAL", "internal" },
    { GIMP_PDB_ERROR_HANDLER_PLUGIN, "GIMP_PDB_ERROR_HANDLER_PLUGIN", "plugin" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_PDB_ERROR_HANDLER_INTERNAL, "GIMP_PDB_ERROR_HANDLER_INTERNAL", NULL },
    { GIMP_PDB_ERROR_HANDLER_PLUGIN, "GIMP_PDB_ERROR_HANDLER_PLUGIN", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpPDBErrorHandler", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "pdb-error-handler");
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
    { GIMP_INTERNAL, NC_("pdb-proc-type", "Internal GIMP procedure"), NULL },
    { GIMP_PLUGIN, NC_("pdb-proc-type", "GIMP Plug-In"), NULL },
    { GIMP_EXTENSION, NC_("pdb-proc-type", "GIMP Extension"), NULL },
    { GIMP_TEMPORARY, NC_("pdb-proc-type", "Temporary Procedure"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpPDBProcType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "pdb-proc-type");
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

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpPDBStatusType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "pdb-status-type");
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

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpMessageHandlerType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "message-handler-type");
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

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpStackTraceMode", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "stack-trace-mode");
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

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpProgressCommand", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "progress-command");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_text_direction_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_TEXT_DIRECTION_LTR, "GIMP_TEXT_DIRECTION_LTR", "ltr" },
    { GIMP_TEXT_DIRECTION_RTL, "GIMP_TEXT_DIRECTION_RTL", "rtl" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_TEXT_DIRECTION_LTR, NC_("text-direction", "From left to right"), NULL },
    { GIMP_TEXT_DIRECTION_RTL, NC_("text-direction", "From right to left"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpTextDirection", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "text-direction");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_text_hint_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_TEXT_HINT_STYLE_NONE, "GIMP_TEXT_HINT_STYLE_NONE", "none" },
    { GIMP_TEXT_HINT_STYLE_SLIGHT, "GIMP_TEXT_HINT_STYLE_SLIGHT", "slight" },
    { GIMP_TEXT_HINT_STYLE_MEDIUM, "GIMP_TEXT_HINT_STYLE_MEDIUM", "medium" },
    { GIMP_TEXT_HINT_STYLE_FULL, "GIMP_TEXT_HINT_STYLE_FULL", "full" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_TEXT_HINT_STYLE_NONE, NC_("text-hint-style", "None"), NULL },
    { GIMP_TEXT_HINT_STYLE_SLIGHT, NC_("text-hint-style", "Slight"), NULL },
    { GIMP_TEXT_HINT_STYLE_MEDIUM, NC_("text-hint-style", "Medium"), NULL },
    { GIMP_TEXT_HINT_STYLE_FULL, NC_("text-hint-style", "Full"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpTextHintStyle", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "text-hint-style");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_text_justification_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_TEXT_JUSTIFY_LEFT, "GIMP_TEXT_JUSTIFY_LEFT", "left" },
    { GIMP_TEXT_JUSTIFY_RIGHT, "GIMP_TEXT_JUSTIFY_RIGHT", "right" },
    { GIMP_TEXT_JUSTIFY_CENTER, "GIMP_TEXT_JUSTIFY_CENTER", "center" },
    { GIMP_TEXT_JUSTIFY_FILL, "GIMP_TEXT_JUSTIFY_FILL", "fill" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_TEXT_JUSTIFY_LEFT, NC_("text-justification", "Left justified"), NULL },
    { GIMP_TEXT_JUSTIFY_RIGHT, NC_("text-justification", "Right justified"), NULL },
    { GIMP_TEXT_JUSTIFY_CENTER, NC_("text-justification", "Centered"), NULL },
    { GIMP_TEXT_JUSTIFY_FILL, NC_("text-justification", "Filled"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpTextJustification", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "text-justification");
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
    { GIMP_USER_DIRECTORY_DOWNLOAD, "GIMP_USER_DIRECTORY_DOWNLOAD", "download" },
    { GIMP_USER_DIRECTORY_MUSIC, "GIMP_USER_DIRECTORY_MUSIC", "music" },
    { GIMP_USER_DIRECTORY_PICTURES, "GIMP_USER_DIRECTORY_PICTURES", "pictures" },
    { GIMP_USER_DIRECTORY_PUBLIC_SHARE, "GIMP_USER_DIRECTORY_PUBLIC_SHARE", "public-share" },
    { GIMP_USER_DIRECTORY_TEMPLATES, "GIMP_USER_DIRECTORY_TEMPLATES", "templates" },
    { GIMP_USER_DIRECTORY_VIDEOS, "GIMP_USER_DIRECTORY_VIDEOS", "videos" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_USER_DIRECTORY_DESKTOP, "GIMP_USER_DIRECTORY_DESKTOP", NULL },
    { GIMP_USER_DIRECTORY_DOCUMENTS, "GIMP_USER_DIRECTORY_DOCUMENTS", NULL },
    { GIMP_USER_DIRECTORY_DOWNLOAD, "GIMP_USER_DIRECTORY_DOWNLOAD", NULL },
    { GIMP_USER_DIRECTORY_MUSIC, "GIMP_USER_DIRECTORY_MUSIC", NULL },
    { GIMP_USER_DIRECTORY_PICTURES, "GIMP_USER_DIRECTORY_PICTURES", NULL },
    { GIMP_USER_DIRECTORY_PUBLIC_SHARE, "GIMP_USER_DIRECTORY_PUBLIC_SHARE", NULL },
    { GIMP_USER_DIRECTORY_TEMPLATES, "GIMP_USER_DIRECTORY_TEMPLATES", NULL },
    { GIMP_USER_DIRECTORY_VIDEOS, "GIMP_USER_DIRECTORY_VIDEOS", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpUserDirectory", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "user-directory");
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

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpVectorsStrokeType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "vectors-stroke-type");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

