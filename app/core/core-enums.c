
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "libgimpbase/gimpbase.h"
#include "core-enums.h"
#include "gimp-intl.h"

/* enumerations from "./core-enums.h" */
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
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpAddMaskType", values);
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
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_container_policy_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_CONTAINER_POLICY_STRONG, "GIMP_CONTAINER_POLICY_STRONG", "strong" },
    { GIMP_CONTAINER_POLICY_WEAK, "GIMP_CONTAINER_POLICY_WEAK", "weak" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_CONTAINER_POLICY_STRONG, "GIMP_CONTAINER_POLICY_STRONG", NULL },
    { GIMP_CONTAINER_POLICY_WEAK, "GIMP_CONTAINER_POLICY_WEAK", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpContainerPolicy", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_convert_dither_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_NO_DITHER, "GIMP_NO_DITHER", "no-dither" },
    { GIMP_FS_DITHER, "GIMP_FS_DITHER", "fs-dither" },
    { GIMP_FSLOWBLEED_DITHER, "GIMP_FSLOWBLEED_DITHER", "fslowbleed-dither" },
    { GIMP_FIXED_DITHER, "GIMP_FIXED_DITHER", "fixed-dither" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_NO_DITHER, N_("None"), NULL },
    { GIMP_FS_DITHER, N_("Floyd-Steinberg (normal)"), NULL },
    { GIMP_FSLOWBLEED_DITHER, N_("Floyd-Steinberg (reduced color bleeding)"), NULL },
    { GIMP_FIXED_DITHER, N_("Positioned"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpConvertDitherType", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_convert_palette_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_MAKE_PALETTE, "GIMP_MAKE_PALETTE", "make-palette" },
    { GIMP_WEB_PALETTE, "GIMP_WEB_PALETTE", "web-palette" },
    { GIMP_MONO_PALETTE, "GIMP_MONO_PALETTE", "mono-palette" },
    { GIMP_CUSTOM_PALETTE, "GIMP_CUSTOM_PALETTE", "custom-palette" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_MAKE_PALETTE, N_("Generate optimum palette"), NULL },
    { GIMP_WEB_PALETTE, N_("Use web-optimized palette"), NULL },
    { GIMP_MONO_PALETTE, N_("Use black and white (1-bit) palette"), NULL },
    { GIMP_CUSTOM_PALETTE, N_("Use custom palette"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpConvertPaletteType", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_gravity_type_get_type (void)
{
  static const GEnumValue values[] =
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

  static const GimpEnumDesc descs[] =
  {
    { GIMP_GRAVITY_NONE, "GIMP_GRAVITY_NONE", NULL },
    { GIMP_GRAVITY_NORTH_WEST, "GIMP_GRAVITY_NORTH_WEST", NULL },
    { GIMP_GRAVITY_NORTH, "GIMP_GRAVITY_NORTH", NULL },
    { GIMP_GRAVITY_NORTH_EAST, "GIMP_GRAVITY_NORTH_EAST", NULL },
    { GIMP_GRAVITY_WEST, "GIMP_GRAVITY_WEST", NULL },
    { GIMP_GRAVITY_CENTER, "GIMP_GRAVITY_CENTER", NULL },
    { GIMP_GRAVITY_EAST, "GIMP_GRAVITY_EAST", NULL },
    { GIMP_GRAVITY_SOUTH_WEST, "GIMP_GRAVITY_SOUTH_WEST", NULL },
    { GIMP_GRAVITY_SOUTH, "GIMP_GRAVITY_SOUTH", NULL },
    { GIMP_GRAVITY_SOUTH_EAST, "GIMP_GRAVITY_SOUTH_EAST", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpGravityType", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_fill_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_FOREGROUND_FILL, "GIMP_FOREGROUND_FILL", "foreground-fill" },
    { GIMP_BACKGROUND_FILL, "GIMP_BACKGROUND_FILL", "background-fill" },
    { GIMP_WHITE_FILL, "GIMP_WHITE_FILL", "white-fill" },
    { GIMP_TRANSPARENT_FILL, "GIMP_TRANSPARENT_FILL", "transparent-fill" },
    { GIMP_PATTERN_FILL, "GIMP_PATTERN_FILL", "pattern-fill" },
    { GIMP_NO_FILL, "GIMP_NO_FILL", "no-fill" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_FOREGROUND_FILL, N_("Foreground color"), NULL },
    { GIMP_BACKGROUND_FILL, N_("Background color"), NULL },
    { GIMP_WHITE_FILL, N_("White"), NULL },
    { GIMP_TRANSPARENT_FILL, N_("Transparency"), NULL },
    { GIMP_PATTERN_FILL, N_("Pattern"), NULL },
    { GIMP_NO_FILL, N_("None"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpFillType", values);
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
    { GIMP_GRADIENT_LINEAR, N_("Linear"), NULL },
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
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_stroke_method_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_STROKE_METHOD_LIBART, "GIMP_STROKE_METHOD_LIBART", "libart" },
    { GIMP_STROKE_METHOD_PAINT_CORE, "GIMP_STROKE_METHOD_PAINT_CORE", "paint-core" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_STROKE_METHOD_LIBART, N_("Stroke line"), NULL },
    { GIMP_STROKE_METHOD_PAINT_CORE, N_("Stroke with a paint tool"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpStrokeMethod", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_stroke_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_STROKE_STYLE_SOLID, "GIMP_STROKE_STYLE_SOLID", "solid" },
    { GIMP_STROKE_STYLE_PATTERN, "GIMP_STROKE_STYLE_PATTERN", "pattern" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_STROKE_STYLE_SOLID, N_("Solid"), NULL },
    { GIMP_STROKE_STYLE_PATTERN, N_("Pattern"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpStrokeStyle", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_join_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_JOIN_MITER, "GIMP_JOIN_MITER", "miter" },
    { GIMP_JOIN_ROUND, "GIMP_JOIN_ROUND", "round" },
    { GIMP_JOIN_BEVEL, "GIMP_JOIN_BEVEL", "bevel" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_JOIN_MITER, N_("Miter"), NULL },
    { GIMP_JOIN_ROUND, N_("Round"), NULL },
    { GIMP_JOIN_BEVEL, N_("Bevel"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpJoinStyle", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_cap_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_CAP_BUTT, "GIMP_CAP_BUTT", "butt" },
    { GIMP_CAP_ROUND, "GIMP_CAP_ROUND", "round" },
    { GIMP_CAP_SQUARE, "GIMP_CAP_SQUARE", "square" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_CAP_BUTT, N_("Butt"), NULL },
    { GIMP_CAP_ROUND, N_("Round"), NULL },
    { GIMP_CAP_SQUARE, N_("Square"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpCapStyle", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_dash_preset_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_DASH_CUSTOM, "GIMP_DASH_CUSTOM", "custom" },
    { GIMP_DASH_LINE, "GIMP_DASH_LINE", "line" },
    { GIMP_DASH_LONG_DASH, "GIMP_DASH_LONG_DASH", "long-dash" },
    { GIMP_DASH_MEDIUM_DASH, "GIMP_DASH_MEDIUM_DASH", "medium-dash" },
    { GIMP_DASH_SHORT_DASH, "GIMP_DASH_SHORT_DASH", "short-dash" },
    { GIMP_DASH_SPARSE_DOTS, "GIMP_DASH_SPARSE_DOTS", "sparse-dots" },
    { GIMP_DASH_NORMAL_DOTS, "GIMP_DASH_NORMAL_DOTS", "normal-dots" },
    { GIMP_DASH_DENSE_DOTS, "GIMP_DASH_DENSE_DOTS", "dense-dots" },
    { GIMP_DASH_STIPPLES, "GIMP_DASH_STIPPLES", "stipples" },
    { GIMP_DASH_DASH_DOT, "GIMP_DASH_DASH_DOT", "dash-dot" },
    { GIMP_DASH_DASH_DOT_DOT, "GIMP_DASH_DASH_DOT_DOT", "dash-dot-dot" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_DASH_CUSTOM, N_("Custom"), NULL },
    { GIMP_DASH_LINE, N_("Line"), NULL },
    { GIMP_DASH_LONG_DASH, N_("Long dashes"), NULL },
    { GIMP_DASH_MEDIUM_DASH, N_("Medium dashes"), NULL },
    { GIMP_DASH_SHORT_DASH, N_("Short dashes"), NULL },
    { GIMP_DASH_SPARSE_DOTS, N_("Sparse dots"), NULL },
    { GIMP_DASH_NORMAL_DOTS, N_("Normal dots"), NULL },
    { GIMP_DASH_DENSE_DOTS, N_("Dense dots"), NULL },
    { GIMP_DASH_STIPPLES, N_("Stipples"), NULL },
    { GIMP_DASH_DASH_DOT, N_("Dash dot..."), NULL },
    { GIMP_DASH_DASH_DOT_DOT, N_("Dash dot dot..."), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpDashPreset", values);
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
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_brush_generated_shape_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_BRUSH_GENERATED_CIRCLE, "GIMP_BRUSH_GENERATED_CIRCLE", "circle" },
    { GIMP_BRUSH_GENERATED_SQUARE, "GIMP_BRUSH_GENERATED_SQUARE", "square" },
    { GIMP_BRUSH_GENERATED_DIAMOND, "GIMP_BRUSH_GENERATED_DIAMOND", "diamond" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_BRUSH_GENERATED_CIRCLE, N_("Circle"), NULL },
    { GIMP_BRUSH_GENERATED_SQUARE, N_("Square"), NULL },
    { GIMP_BRUSH_GENERATED_DIAMOND, N_("Diamond"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpBrushGeneratedShape", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_orientation_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_ORIENTATION_HORIZONTAL, "GIMP_ORIENTATION_HORIZONTAL", "horizontal" },
    { GIMP_ORIENTATION_VERTICAL, "GIMP_ORIENTATION_VERTICAL", "vertical" },
    { GIMP_ORIENTATION_UNKNOWN, "GIMP_ORIENTATION_UNKNOWN", "unknown" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_ORIENTATION_HORIZONTAL, N_("Horizontal"), NULL },
    { GIMP_ORIENTATION_VERTICAL, N_("Vertical"), NULL },
    { GIMP_ORIENTATION_UNKNOWN, N_("Unknown"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpOrientationType", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_rotation_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_ROTATE_90, "GIMP_ROTATE_90", "90" },
    { GIMP_ROTATE_180, "GIMP_ROTATE_180", "180" },
    { GIMP_ROTATE_270, "GIMP_ROTATE_270", "270" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_ROTATE_90, "GIMP_ROTATE_90", NULL },
    { GIMP_ROTATE_180, "GIMP_ROTATE_180", NULL },
    { GIMP_ROTATE_270, "GIMP_ROTATE_270", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpRotationType", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_view_size_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_VIEW_SIZE_TINY, "GIMP_VIEW_SIZE_TINY", "tiny" },
    { GIMP_VIEW_SIZE_EXTRA_SMALL, "GIMP_VIEW_SIZE_EXTRA_SMALL", "extra-small" },
    { GIMP_VIEW_SIZE_SMALL, "GIMP_VIEW_SIZE_SMALL", "small" },
    { GIMP_VIEW_SIZE_MEDIUM, "GIMP_VIEW_SIZE_MEDIUM", "medium" },
    { GIMP_VIEW_SIZE_LARGE, "GIMP_VIEW_SIZE_LARGE", "large" },
    { GIMP_VIEW_SIZE_EXTRA_LARGE, "GIMP_VIEW_SIZE_EXTRA_LARGE", "extra-large" },
    { GIMP_VIEW_SIZE_HUGE, "GIMP_VIEW_SIZE_HUGE", "huge" },
    { GIMP_VIEW_SIZE_ENORMOUS, "GIMP_VIEW_SIZE_ENORMOUS", "enormous" },
    { GIMP_VIEW_SIZE_GIGANTIC, "GIMP_VIEW_SIZE_GIGANTIC", "gigantic" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_VIEW_SIZE_TINY, N_("Tiny"), NULL },
    { GIMP_VIEW_SIZE_EXTRA_SMALL, N_("Very small"), NULL },
    { GIMP_VIEW_SIZE_SMALL, N_("Small"), NULL },
    { GIMP_VIEW_SIZE_MEDIUM, N_("Medium"), NULL },
    { GIMP_VIEW_SIZE_LARGE, N_("Large"), NULL },
    { GIMP_VIEW_SIZE_EXTRA_LARGE, N_("Very large"), NULL },
    { GIMP_VIEW_SIZE_HUGE, N_("Huge"), NULL },
    { GIMP_VIEW_SIZE_ENORMOUS, N_("Enormous"), NULL },
    { GIMP_VIEW_SIZE_GIGANTIC, N_("Gigantic"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpViewSize", values);
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
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_selection_control_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_SELECTION_OFF, "GIMP_SELECTION_OFF", "off" },
    { GIMP_SELECTION_LAYER_OFF, "GIMP_SELECTION_LAYER_OFF", "layer-off" },
    { GIMP_SELECTION_ON, "GIMP_SELECTION_ON", "on" },
    { GIMP_SELECTION_PAUSE, "GIMP_SELECTION_PAUSE", "pause" },
    { GIMP_SELECTION_RESUME, "GIMP_SELECTION_RESUME", "resume" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_SELECTION_OFF, "GIMP_SELECTION_OFF", NULL },
    { GIMP_SELECTION_LAYER_OFF, "GIMP_SELECTION_LAYER_OFF", NULL },
    { GIMP_SELECTION_ON, "GIMP_SELECTION_ON", NULL },
    { GIMP_SELECTION_PAUSE, "GIMP_SELECTION_PAUSE", NULL },
    { GIMP_SELECTION_RESUME, "GIMP_SELECTION_RESUME", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpSelectionControl", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_thumbnail_size_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_THUMBNAIL_SIZE_NONE, "GIMP_THUMBNAIL_SIZE_NONE", "none" },
    { GIMP_THUMBNAIL_SIZE_NORMAL, "GIMP_THUMBNAIL_SIZE_NORMAL", "normal" },
    { GIMP_THUMBNAIL_SIZE_LARGE, "GIMP_THUMBNAIL_SIZE_LARGE", "large" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_THUMBNAIL_SIZE_NONE, N_("No thumbnails"), NULL },
    { GIMP_THUMBNAIL_SIZE_NORMAL, N_("Normal (128x128)"), NULL },
    { GIMP_THUMBNAIL_SIZE_LARGE, N_("Large (256x256)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpThumbnailSize", values);
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
    { GIMP_TRANSFORM_FORWARD, N_("Forward (traditional)"), NULL },
    { GIMP_TRANSFORM_BACKWARD, N_("Backward (corrective)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpTransformDirection", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_undo_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_UNDO_MODE_UNDO, "GIMP_UNDO_MODE_UNDO", "undo" },
    { GIMP_UNDO_MODE_REDO, "GIMP_UNDO_MODE_REDO", "redo" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_UNDO_MODE_UNDO, "GIMP_UNDO_MODE_UNDO", NULL },
    { GIMP_UNDO_MODE_REDO, "GIMP_UNDO_MODE_REDO", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpUndoMode", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_undo_event_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_UNDO_EVENT_UNDO_PUSHED, "GIMP_UNDO_EVENT_UNDO_PUSHED", "undo-pushed" },
    { GIMP_UNDO_EVENT_UNDO_EXPIRED, "GIMP_UNDO_EVENT_UNDO_EXPIRED", "undo-expired" },
    { GIMP_UNDO_EVENT_REDO_EXPIRED, "GIMP_UNDO_EVENT_REDO_EXPIRED", "redo-expired" },
    { GIMP_UNDO_EVENT_UNDO, "GIMP_UNDO_EVENT_UNDO", "undo" },
    { GIMP_UNDO_EVENT_REDO, "GIMP_UNDO_EVENT_REDO", "redo" },
    { GIMP_UNDO_EVENT_UNDO_FREE, "GIMP_UNDO_EVENT_UNDO_FREE", "undo-free" },
    { GIMP_UNDO_EVENT_UNDO_FREEZE, "GIMP_UNDO_EVENT_UNDO_FREEZE", "undo-freeze" },
    { GIMP_UNDO_EVENT_UNDO_THAW, "GIMP_UNDO_EVENT_UNDO_THAW", "undo-thaw" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_UNDO_EVENT_UNDO_PUSHED, "GIMP_UNDO_EVENT_UNDO_PUSHED", NULL },
    { GIMP_UNDO_EVENT_UNDO_EXPIRED, "GIMP_UNDO_EVENT_UNDO_EXPIRED", NULL },
    { GIMP_UNDO_EVENT_REDO_EXPIRED, "GIMP_UNDO_EVENT_REDO_EXPIRED", NULL },
    { GIMP_UNDO_EVENT_UNDO, "GIMP_UNDO_EVENT_UNDO", NULL },
    { GIMP_UNDO_EVENT_REDO, "GIMP_UNDO_EVENT_REDO", NULL },
    { GIMP_UNDO_EVENT_UNDO_FREE, "GIMP_UNDO_EVENT_UNDO_FREE", NULL },
    { GIMP_UNDO_EVENT_UNDO_FREEZE, "GIMP_UNDO_EVENT_UNDO_FREEZE", NULL },
    { GIMP_UNDO_EVENT_UNDO_THAW, "GIMP_UNDO_EVENT_UNDO_THAW", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpUndoEvent", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_undo_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_UNDO_GROUP_NONE, "GIMP_UNDO_GROUP_NONE", "group-none" },
    { GIMP_UNDO_GROUP_IMAGE_SCALE, "GIMP_UNDO_GROUP_IMAGE_SCALE", "group-image-scale" },
    { GIMP_UNDO_GROUP_IMAGE_RESIZE, "GIMP_UNDO_GROUP_IMAGE_RESIZE", "group-image-resize" },
    { GIMP_UNDO_GROUP_IMAGE_FLIP, "GIMP_UNDO_GROUP_IMAGE_FLIP", "group-image-flip" },
    { GIMP_UNDO_GROUP_IMAGE_ROTATE, "GIMP_UNDO_GROUP_IMAGE_ROTATE", "group-image-rotate" },
    { GIMP_UNDO_GROUP_IMAGE_CROP, "GIMP_UNDO_GROUP_IMAGE_CROP", "group-image-crop" },
    { GIMP_UNDO_GROUP_IMAGE_CONVERT, "GIMP_UNDO_GROUP_IMAGE_CONVERT", "group-image-convert" },
    { GIMP_UNDO_GROUP_IMAGE_ITEM_REMOVE, "GIMP_UNDO_GROUP_IMAGE_ITEM_REMOVE", "group-image-item-remove" },
    { GIMP_UNDO_GROUP_IMAGE_LAYERS_MERGE, "GIMP_UNDO_GROUP_IMAGE_LAYERS_MERGE", "group-image-layers-merge" },
    { GIMP_UNDO_GROUP_IMAGE_VECTORS_MERGE, "GIMP_UNDO_GROUP_IMAGE_VECTORS_MERGE", "group-image-vectors-merge" },
    { GIMP_UNDO_GROUP_IMAGE_QMASK, "GIMP_UNDO_GROUP_IMAGE_QMASK", "group-image-qmask" },
    { GIMP_UNDO_GROUP_IMAGE_GRID, "GIMP_UNDO_GROUP_IMAGE_GRID", "group-image-grid" },
    { GIMP_UNDO_GROUP_IMAGE_GUIDE, "GIMP_UNDO_GROUP_IMAGE_GUIDE", "group-image-guide" },
    { GIMP_UNDO_GROUP_DRAWABLE, "GIMP_UNDO_GROUP_DRAWABLE", "group-drawable" },
    { GIMP_UNDO_GROUP_DRAWABLE_MOD, "GIMP_UNDO_GROUP_DRAWABLE_MOD", "group-drawable-mod" },
    { GIMP_UNDO_GROUP_MASK, "GIMP_UNDO_GROUP_MASK", "group-mask" },
    { GIMP_UNDO_GROUP_ITEM_VISIBILITY, "GIMP_UNDO_GROUP_ITEM_VISIBILITY", "group-item-visibility" },
    { GIMP_UNDO_GROUP_ITEM_LINKED, "GIMP_UNDO_GROUP_ITEM_LINKED", "group-item-linked" },
    { GIMP_UNDO_GROUP_ITEM_PROPERTIES, "GIMP_UNDO_GROUP_ITEM_PROPERTIES", "group-item-properties" },
    { GIMP_UNDO_GROUP_ITEM_DISPLACE, "GIMP_UNDO_GROUP_ITEM_DISPLACE", "group-item-displace" },
    { GIMP_UNDO_GROUP_ITEM_SCALE, "GIMP_UNDO_GROUP_ITEM_SCALE", "group-item-scale" },
    { GIMP_UNDO_GROUP_ITEM_RESIZE, "GIMP_UNDO_GROUP_ITEM_RESIZE", "group-item-resize" },
    { GIMP_UNDO_GROUP_LAYER_ADD_MASK, "GIMP_UNDO_GROUP_LAYER_ADD_MASK", "group-layer-add-mask" },
    { GIMP_UNDO_GROUP_LAYER_APPLY_MASK, "GIMP_UNDO_GROUP_LAYER_APPLY_MASK", "group-layer-apply-mask" },
    { GIMP_UNDO_GROUP_FS_TO_LAYER, "GIMP_UNDO_GROUP_FS_TO_LAYER", "group-fs-to-layer" },
    { GIMP_UNDO_GROUP_FS_FLOAT, "GIMP_UNDO_GROUP_FS_FLOAT", "group-fs-float" },
    { GIMP_UNDO_GROUP_FS_ANCHOR, "GIMP_UNDO_GROUP_FS_ANCHOR", "group-fs-anchor" },
    { GIMP_UNDO_GROUP_FS_REMOVE, "GIMP_UNDO_GROUP_FS_REMOVE", "group-fs-remove" },
    { GIMP_UNDO_GROUP_EDIT_PASTE, "GIMP_UNDO_GROUP_EDIT_PASTE", "group-edit-paste" },
    { GIMP_UNDO_GROUP_EDIT_CUT, "GIMP_UNDO_GROUP_EDIT_CUT", "group-edit-cut" },
    { GIMP_UNDO_GROUP_TEXT, "GIMP_UNDO_GROUP_TEXT", "group-text" },
    { GIMP_UNDO_GROUP_TRANSFORM, "GIMP_UNDO_GROUP_TRANSFORM", "group-transform" },
    { GIMP_UNDO_GROUP_PAINT, "GIMP_UNDO_GROUP_PAINT", "group-paint" },
    { GIMP_UNDO_GROUP_PARASITE_ATTACH, "GIMP_UNDO_GROUP_PARASITE_ATTACH", "group-parasite-attach" },
    { GIMP_UNDO_GROUP_PARASITE_REMOVE, "GIMP_UNDO_GROUP_PARASITE_REMOVE", "group-parasite-remove" },
    { GIMP_UNDO_GROUP_VECTORS_IMPORT, "GIMP_UNDO_GROUP_VECTORS_IMPORT", "group-vectors-import" },
    { GIMP_UNDO_GROUP_MISC, "GIMP_UNDO_GROUP_MISC", "group-misc" },
    { GIMP_UNDO_IMAGE_TYPE, "GIMP_UNDO_IMAGE_TYPE", "image-type" },
    { GIMP_UNDO_IMAGE_SIZE, "GIMP_UNDO_IMAGE_SIZE", "image-size" },
    { GIMP_UNDO_IMAGE_RESOLUTION, "GIMP_UNDO_IMAGE_RESOLUTION", "image-resolution" },
    { GIMP_UNDO_IMAGE_GRID, "GIMP_UNDO_IMAGE_GRID", "image-grid" },
    { GIMP_UNDO_IMAGE_GUIDE, "GIMP_UNDO_IMAGE_GUIDE", "image-guide" },
    { GIMP_UNDO_IMAGE_COLORMAP, "GIMP_UNDO_IMAGE_COLORMAP", "image-colormap" },
    { GIMP_UNDO_DRAWABLE, "GIMP_UNDO_DRAWABLE", "drawable" },
    { GIMP_UNDO_DRAWABLE_MOD, "GIMP_UNDO_DRAWABLE_MOD", "drawable-mod" },
    { GIMP_UNDO_MASK, "GIMP_UNDO_MASK", "mask" },
    { GIMP_UNDO_ITEM_RENAME, "GIMP_UNDO_ITEM_RENAME", "item-rename" },
    { GIMP_UNDO_ITEM_DISPLACE, "GIMP_UNDO_ITEM_DISPLACE", "item-displace" },
    { GIMP_UNDO_ITEM_VISIBILITY, "GIMP_UNDO_ITEM_VISIBILITY", "item-visibility" },
    { GIMP_UNDO_ITEM_LINKED, "GIMP_UNDO_ITEM_LINKED", "item-linked" },
    { GIMP_UNDO_LAYER_ADD, "GIMP_UNDO_LAYER_ADD", "layer-add" },
    { GIMP_UNDO_LAYER_REMOVE, "GIMP_UNDO_LAYER_REMOVE", "layer-remove" },
    { GIMP_UNDO_LAYER_MASK_ADD, "GIMP_UNDO_LAYER_MASK_ADD", "layer-mask-add" },
    { GIMP_UNDO_LAYER_MASK_REMOVE, "GIMP_UNDO_LAYER_MASK_REMOVE", "layer-mask-remove" },
    { GIMP_UNDO_LAYER_REPOSITION, "GIMP_UNDO_LAYER_REPOSITION", "layer-reposition" },
    { GIMP_UNDO_LAYER_MODE, "GIMP_UNDO_LAYER_MODE", "layer-mode" },
    { GIMP_UNDO_LAYER_OPACITY, "GIMP_UNDO_LAYER_OPACITY", "layer-opacity" },
    { GIMP_UNDO_LAYER_PRESERVE_TRANS, "GIMP_UNDO_LAYER_PRESERVE_TRANS", "layer-preserve-trans" },
    { GIMP_UNDO_TEXT_LAYER, "GIMP_UNDO_TEXT_LAYER", "text-layer" },
    { GIMP_UNDO_TEXT_LAYER_MODIFIED, "GIMP_UNDO_TEXT_LAYER_MODIFIED", "text-layer-modified" },
    { GIMP_UNDO_CHANNEL_ADD, "GIMP_UNDO_CHANNEL_ADD", "channel-add" },
    { GIMP_UNDO_CHANNEL_REMOVE, "GIMP_UNDO_CHANNEL_REMOVE", "channel-remove" },
    { GIMP_UNDO_CHANNEL_REPOSITION, "GIMP_UNDO_CHANNEL_REPOSITION", "channel-reposition" },
    { GIMP_UNDO_CHANNEL_COLOR, "GIMP_UNDO_CHANNEL_COLOR", "channel-color" },
    { GIMP_UNDO_VECTORS_ADD, "GIMP_UNDO_VECTORS_ADD", "vectors-add" },
    { GIMP_UNDO_VECTORS_REMOVE, "GIMP_UNDO_VECTORS_REMOVE", "vectors-remove" },
    { GIMP_UNDO_VECTORS_MOD, "GIMP_UNDO_VECTORS_MOD", "vectors-mod" },
    { GIMP_UNDO_VECTORS_REPOSITION, "GIMP_UNDO_VECTORS_REPOSITION", "vectors-reposition" },
    { GIMP_UNDO_FS_TO_LAYER, "GIMP_UNDO_FS_TO_LAYER", "fs-to-layer" },
    { GIMP_UNDO_FS_RIGOR, "GIMP_UNDO_FS_RIGOR", "fs-rigor" },
    { GIMP_UNDO_FS_RELAX, "GIMP_UNDO_FS_RELAX", "fs-relax" },
    { GIMP_UNDO_TRANSFORM, "GIMP_UNDO_TRANSFORM", "transform" },
    { GIMP_UNDO_PAINT, "GIMP_UNDO_PAINT", "paint" },
    { GIMP_UNDO_INK, "GIMP_UNDO_INK", "ink" },
    { GIMP_UNDO_PARASITE_ATTACH, "GIMP_UNDO_PARASITE_ATTACH", "parasite-attach" },
    { GIMP_UNDO_PARASITE_REMOVE, "GIMP_UNDO_PARASITE_REMOVE", "parasite-remove" },
    { GIMP_UNDO_CANT, "GIMP_UNDO_CANT", "cant" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_UNDO_GROUP_NONE, N_("<<invalid>>"), NULL },
    { GIMP_UNDO_GROUP_IMAGE_SCALE, N_("Scale image"), NULL },
    { GIMP_UNDO_GROUP_IMAGE_RESIZE, N_("Resize image"), NULL },
    { GIMP_UNDO_GROUP_IMAGE_FLIP, N_("Flip image"), NULL },
    { GIMP_UNDO_GROUP_IMAGE_ROTATE, N_("Rotate image"), NULL },
    { GIMP_UNDO_GROUP_IMAGE_CROP, N_("Crop image"), NULL },
    { GIMP_UNDO_GROUP_IMAGE_CONVERT, N_("Convert image"), NULL },
    { GIMP_UNDO_GROUP_IMAGE_ITEM_REMOVE, N_("Remove item"), NULL },
    { GIMP_UNDO_GROUP_IMAGE_LAYERS_MERGE, N_("Merge layers"), NULL },
    { GIMP_UNDO_GROUP_IMAGE_VECTORS_MERGE, N_("Merge vectors"), NULL },
    { GIMP_UNDO_GROUP_IMAGE_QMASK, N_("Quick Mask"), NULL },
    { GIMP_UNDO_GROUP_IMAGE_GRID, N_("Grid"), NULL },
    { GIMP_UNDO_GROUP_IMAGE_GUIDE, N_("Guide"), NULL },
    { GIMP_UNDO_GROUP_DRAWABLE, N_("Drawable"), NULL },
    { GIMP_UNDO_GROUP_DRAWABLE_MOD, N_("Drawable mod"), NULL },
    { GIMP_UNDO_GROUP_MASK, N_("Selection mask"), NULL },
    { GIMP_UNDO_GROUP_ITEM_VISIBILITY, N_("Item visibility"), NULL },
    { GIMP_UNDO_GROUP_ITEM_LINKED, N_("Linked item"), NULL },
    { GIMP_UNDO_GROUP_ITEM_PROPERTIES, N_("Item properties"), NULL },
    { GIMP_UNDO_GROUP_ITEM_DISPLACE, N_("Move item"), NULL },
    { GIMP_UNDO_GROUP_ITEM_SCALE, N_("Scale item"), NULL },
    { GIMP_UNDO_GROUP_ITEM_RESIZE, N_("Resize item"), NULL },
    { GIMP_UNDO_GROUP_LAYER_ADD_MASK, N_("Add layer mask"), NULL },
    { GIMP_UNDO_GROUP_LAYER_APPLY_MASK, N_("Apply layer mask"), NULL },
    { GIMP_UNDO_GROUP_FS_TO_LAYER, N_("Floating selection to layer"), NULL },
    { GIMP_UNDO_GROUP_FS_FLOAT, N_("Float selection"), NULL },
    { GIMP_UNDO_GROUP_FS_ANCHOR, N_("Anchor floating selection"), NULL },
    { GIMP_UNDO_GROUP_FS_REMOVE, N_("Remove floating selection"), NULL },
    { GIMP_UNDO_GROUP_EDIT_PASTE, N_("Paste"), NULL },
    { GIMP_UNDO_GROUP_EDIT_CUT, N_("Cut"), NULL },
    { GIMP_UNDO_GROUP_TEXT, N_("Text"), NULL },
    { GIMP_UNDO_GROUP_TRANSFORM, N_("Transform"), NULL },
    { GIMP_UNDO_GROUP_PAINT, N_("Paint"), NULL },
    { GIMP_UNDO_GROUP_PARASITE_ATTACH, N_("Attach parasite"), NULL },
    { GIMP_UNDO_GROUP_PARASITE_REMOVE, N_("Remove parasite"), NULL },
    { GIMP_UNDO_GROUP_VECTORS_IMPORT, N_("Import paths"), NULL },
    { GIMP_UNDO_GROUP_MISC, N_("Plug-In"), NULL },
    { GIMP_UNDO_IMAGE_TYPE, N_("Image type"), NULL },
    { GIMP_UNDO_IMAGE_SIZE, N_("Image size"), NULL },
    { GIMP_UNDO_IMAGE_RESOLUTION, N_("Resolution change"), NULL },
    { GIMP_UNDO_IMAGE_GRID, N_("Grid"), NULL },
    { GIMP_UNDO_IMAGE_GUIDE, N_("Guide"), NULL },
    { GIMP_UNDO_IMAGE_COLORMAP, N_("Change indexed palette"), NULL },
    { GIMP_UNDO_DRAWABLE, N_("Drawable"), NULL },
    { GIMP_UNDO_DRAWABLE_MOD, N_("Drawable mod"), NULL },
    { GIMP_UNDO_MASK, N_("Selection mask"), NULL },
    { GIMP_UNDO_ITEM_RENAME, N_("Rename item"), NULL },
    { GIMP_UNDO_ITEM_DISPLACE, N_("Move item"), NULL },
    { GIMP_UNDO_ITEM_VISIBILITY, N_("Item visibility"), NULL },
    { GIMP_UNDO_ITEM_LINKED, N_("Set item linked"), NULL },
    { GIMP_UNDO_LAYER_ADD, N_("New layer"), NULL },
    { GIMP_UNDO_LAYER_REMOVE, N_("Delete layer"), NULL },
    { GIMP_UNDO_LAYER_MASK_ADD, N_("Add layer mask"), NULL },
    { GIMP_UNDO_LAYER_MASK_REMOVE, N_("Delete layer mask"), NULL },
    { GIMP_UNDO_LAYER_REPOSITION, N_("Reposition layer"), NULL },
    { GIMP_UNDO_LAYER_MODE, N_("Set layer mode"), NULL },
    { GIMP_UNDO_LAYER_OPACITY, N_("Set layer opacity"), NULL },
    { GIMP_UNDO_LAYER_PRESERVE_TRANS, N_("Set preserve trans"), NULL },
    { GIMP_UNDO_TEXT_LAYER, N_("Text"), NULL },
    { GIMP_UNDO_TEXT_LAYER_MODIFIED, N_("Text modified"), NULL },
    { GIMP_UNDO_CHANNEL_ADD, N_("New channel"), NULL },
    { GIMP_UNDO_CHANNEL_REMOVE, N_("Delete channel"), NULL },
    { GIMP_UNDO_CHANNEL_REPOSITION, N_("Reposition channel"), NULL },
    { GIMP_UNDO_CHANNEL_COLOR, N_("Channel color"), NULL },
    { GIMP_UNDO_VECTORS_ADD, N_("New vectors"), NULL },
    { GIMP_UNDO_VECTORS_REMOVE, N_("Delete vectors"), NULL },
    { GIMP_UNDO_VECTORS_MOD, N_("Vectors mod"), NULL },
    { GIMP_UNDO_VECTORS_REPOSITION, N_("Reposition vectors"), NULL },
    { GIMP_UNDO_FS_TO_LAYER, N_("FS to layer"), NULL },
    { GIMP_UNDO_FS_RIGOR, N_("FS rigor"), NULL },
    { GIMP_UNDO_FS_RELAX, N_("FS relax"), NULL },
    { GIMP_UNDO_TRANSFORM, N_("Transform"), NULL },
    { GIMP_UNDO_PAINT, N_("Paint"), NULL },
    { GIMP_UNDO_INK, N_("Ink"), NULL },
    { GIMP_UNDO_PARASITE_ATTACH, N_("Attach parasite"), NULL },
    { GIMP_UNDO_PARASITE_REMOVE, N_("Remove parasite"), NULL },
    { GIMP_UNDO_CANT, N_("EEK: can't undo"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpUndoType", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_dirty_mask_get_type (void)
{
  static const GFlagsValue values[] =
  {
    { GIMP_DIRTY_NONE, "GIMP_DIRTY_NONE", "none" },
    { GIMP_DIRTY_IMAGE, "GIMP_DIRTY_IMAGE", "image" },
    { GIMP_DIRTY_IMAGE_SIZE, "GIMP_DIRTY_IMAGE_SIZE", "image-size" },
    { GIMP_DIRTY_IMAGE_META, "GIMP_DIRTY_IMAGE_META", "image-meta" },
    { GIMP_DIRTY_IMAGE_STRUCTURE, "GIMP_DIRTY_IMAGE_STRUCTURE", "image-structure" },
    { GIMP_DIRTY_ITEM, "GIMP_DIRTY_ITEM", "item" },
    { GIMP_DIRTY_ITEM_META, "GIMP_DIRTY_ITEM_META", "item-meta" },
    { GIMP_DIRTY_DRAWABLE, "GIMP_DIRTY_DRAWABLE", "drawable" },
    { GIMP_DIRTY_VECTORS, "GIMP_DIRTY_VECTORS", "vectors" },
    { GIMP_DIRTY_SELECTION, "GIMP_DIRTY_SELECTION", "selection" },
    { GIMP_DIRTY_ALL, "GIMP_DIRTY_ALL", "all" },
    { 0, NULL, NULL }
  };

  static const GimpFlagsDesc descs[] =
  {
    { GIMP_DIRTY_NONE, "GIMP_DIRTY_NONE", NULL },
    { GIMP_DIRTY_IMAGE, "GIMP_DIRTY_IMAGE", NULL },
    { GIMP_DIRTY_IMAGE_SIZE, "GIMP_DIRTY_IMAGE_SIZE", NULL },
    { GIMP_DIRTY_IMAGE_META, "GIMP_DIRTY_IMAGE_META", NULL },
    { GIMP_DIRTY_IMAGE_STRUCTURE, "GIMP_DIRTY_IMAGE_STRUCTURE", NULL },
    { GIMP_DIRTY_ITEM, "GIMP_DIRTY_ITEM", NULL },
    { GIMP_DIRTY_ITEM_META, "GIMP_DIRTY_ITEM_META", NULL },
    { GIMP_DIRTY_DRAWABLE, "GIMP_DIRTY_DRAWABLE", NULL },
    { GIMP_DIRTY_VECTORS, "GIMP_DIRTY_VECTORS", NULL },
    { GIMP_DIRTY_SELECTION, "GIMP_DIRTY_SELECTION", NULL },
    { GIMP_DIRTY_ALL, "GIMP_DIRTY_ALL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_flags_register_static ("GimpDirtyMask", values);
      gimp_flags_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

