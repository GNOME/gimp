
/* Generated data (by gimp-mkenums) */

#include "stamp-gimpbaseenums.h"
#include "config.h"
#include <glib-object.h>
#undef GIMP_DISABLE_DEPRECATED
#include "gimpbasetypes.h"
#include "libgimp/libgimp-intl.h"
#include "gimpbaseenums.h"


/* enumerations from "gimpbaseenums.h" */
GType
gimp_add_mask_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_ADD_MASK_WHITE, "GIMP_ADD_MASK_WHITE", "white" },
    { GIMP_ADD_MASK_BLACK, "GIMP_ADD_MASK_BLACK", "black" },
    { GIMP_ADD_MASK_ALPHA, "GIMP_ADD_MASK_ALPHA", "alpha" },
    { GIMP_ADD_MASK_ALPHA_TRANSFER, "GIMP_ADD_MASK_ALPHA_TRANSFER", "alpha-transfer" },
    { GIMP_ADD_MASK_SELECTION, "GIMP_ADD_MASK_SELECTION", "selection" },
    { GIMP_ADD_MASK_COPY, "GIMP_ADD_MASK_COPY", "copy" },
    { GIMP_ADD_MASK_CHANNEL, "GIMP_ADD_MASK_CHANNEL", "channel" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_ADD_MASK_WHITE, NC_("add-mask-type", "_White (full opacity)"), NULL },
    { GIMP_ADD_MASK_BLACK, NC_("add-mask-type", "_Black (full transparency)"), NULL },
    { GIMP_ADD_MASK_ALPHA, NC_("add-mask-type", "Layer's _alpha channel"), NULL },
    { GIMP_ADD_MASK_ALPHA_TRANSFER, NC_("add-mask-type", "_Transfer layer's alpha channel"), NULL },
    { GIMP_ADD_MASK_SELECTION, NC_("add-mask-type", "_Selection"), NULL },
    { GIMP_ADD_MASK_COPY, NC_("add-mask-type", "_Grayscale copy of layer"), NULL },
    { GIMP_ADD_MASK_CHANNEL, NC_("add-mask-type", "C_hannel"), NULL },
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
    { GIMP_BRUSH_GENERATED_CIRCLE, NC_("brush-generated-shape", "Circle"), NULL },
    { GIMP_BRUSH_GENERATED_SQUARE, NC_("brush-generated-shape", "Square"), NULL },
    { GIMP_BRUSH_GENERATED_DIAMOND, NC_("brush-generated-shape", "Diamond"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpBrushGeneratedShape", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "brush-generated-shape");
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
    { GIMP_CAP_BUTT, NC_("cap-style", "Butt"), NULL },
    { GIMP_CAP_ROUND, NC_("cap-style", "Round"), NULL },
    { GIMP_CAP_SQUARE, NC_("cap-style", "Square"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpCapStyle", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "cap-style");
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
    { GIMP_CHANNEL_RED, "GIMP_CHANNEL_RED", "red" },
    { GIMP_CHANNEL_GREEN, "GIMP_CHANNEL_GREEN", "green" },
    { GIMP_CHANNEL_BLUE, "GIMP_CHANNEL_BLUE", "blue" },
    { GIMP_CHANNEL_GRAY, "GIMP_CHANNEL_GRAY", "gray" },
    { GIMP_CHANNEL_INDEXED, "GIMP_CHANNEL_INDEXED", "indexed" },
    { GIMP_CHANNEL_ALPHA, "GIMP_CHANNEL_ALPHA", "alpha" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_CHANNEL_RED, NC_("channel-type", "Red"), NULL },
    { GIMP_CHANNEL_GREEN, NC_("channel-type", "Green"), NULL },
    { GIMP_CHANNEL_BLUE, NC_("channel-type", "Blue"), NULL },
    { GIMP_CHANNEL_GRAY, NC_("channel-type", "Gray"), NULL },
    { GIMP_CHANNEL_INDEXED, NC_("channel-type", "Indexed"), NULL },
    { GIMP_CHANNEL_ALPHA, NC_("channel-type", "Alpha"), NULL },
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
    { GIMP_CHECK_TYPE_CUSTOM_CHECKS, "GIMP_CHECK_TYPE_CUSTOM_CHECKS", "custom-checks" },
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
    { GIMP_CHECK_TYPE_CUSTOM_CHECKS, NC_("check-type", "Custom checks"), NULL },
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
    { GIMP_CLONE_IMAGE, "GIMP_CLONE_IMAGE", "image" },
    { GIMP_CLONE_PATTERN, "GIMP_CLONE_PATTERN", "pattern" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_CLONE_IMAGE, NC_("clone-type", "Image"), NULL },
    { GIMP_CLONE_PATTERN, NC_("clone-type", "Pattern"), NULL },
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
gimp_color_tag_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_COLOR_TAG_NONE, "GIMP_COLOR_TAG_NONE", "none" },
    { GIMP_COLOR_TAG_BLUE, "GIMP_COLOR_TAG_BLUE", "blue" },
    { GIMP_COLOR_TAG_GREEN, "GIMP_COLOR_TAG_GREEN", "green" },
    { GIMP_COLOR_TAG_YELLOW, "GIMP_COLOR_TAG_YELLOW", "yellow" },
    { GIMP_COLOR_TAG_ORANGE, "GIMP_COLOR_TAG_ORANGE", "orange" },
    { GIMP_COLOR_TAG_BROWN, "GIMP_COLOR_TAG_BROWN", "brown" },
    { GIMP_COLOR_TAG_RED, "GIMP_COLOR_TAG_RED", "red" },
    { GIMP_COLOR_TAG_VIOLET, "GIMP_COLOR_TAG_VIOLET", "violet" },
    { GIMP_COLOR_TAG_GRAY, "GIMP_COLOR_TAG_GRAY", "gray" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_COLOR_TAG_NONE, NC_("color-tag", "None"), NULL },
    { GIMP_COLOR_TAG_BLUE, NC_("color-tag", "Blue"), NULL },
    { GIMP_COLOR_TAG_GREEN, NC_("color-tag", "Green"), NULL },
    { GIMP_COLOR_TAG_YELLOW, NC_("color-tag", "Yellow"), NULL },
    { GIMP_COLOR_TAG_ORANGE, NC_("color-tag", "Orange"), NULL },
    { GIMP_COLOR_TAG_BROWN, NC_("color-tag", "Brown"), NULL },
    { GIMP_COLOR_TAG_RED, NC_("color-tag", "Red"), NULL },
    { GIMP_COLOR_TAG_VIOLET, NC_("color-tag", "Violet"), NULL },
    { GIMP_COLOR_TAG_GRAY, NC_("color-tag", "Gray"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpColorTag", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "color-tag");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_component_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_COMPONENT_TYPE_U8, "GIMP_COMPONENT_TYPE_U8", "u8" },
    { GIMP_COMPONENT_TYPE_U16, "GIMP_COMPONENT_TYPE_U16", "u16" },
    { GIMP_COMPONENT_TYPE_U32, "GIMP_COMPONENT_TYPE_U32", "u32" },
    { GIMP_COMPONENT_TYPE_HALF, "GIMP_COMPONENT_TYPE_HALF", "half" },
    { GIMP_COMPONENT_TYPE_FLOAT, "GIMP_COMPONENT_TYPE_FLOAT", "float" },
    { GIMP_COMPONENT_TYPE_DOUBLE, "GIMP_COMPONENT_TYPE_DOUBLE", "double" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_COMPONENT_TYPE_U8, NC_("component-type", "8-bit integer"), NULL },
    { GIMP_COMPONENT_TYPE_U16, NC_("component-type", "16-bit integer"), NULL },
    { GIMP_COMPONENT_TYPE_U32, NC_("component-type", "32-bit integer"), NULL },
    { GIMP_COMPONENT_TYPE_HALF, NC_("component-type", "16-bit floating point"), NULL },
    { GIMP_COMPONENT_TYPE_FLOAT, NC_("component-type", "32-bit floating point"), NULL },
    { GIMP_COMPONENT_TYPE_DOUBLE, NC_("component-type", "64-bit floating point"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpComponentType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "component-type");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_convert_palette_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_CONVERT_PALETTE_GENERATE, "GIMP_CONVERT_PALETTE_GENERATE", "generate" },
    { GIMP_CONVERT_PALETTE_WEB, "GIMP_CONVERT_PALETTE_WEB", "web" },
    { GIMP_CONVERT_PALETTE_MONO, "GIMP_CONVERT_PALETTE_MONO", "mono" },
    { GIMP_CONVERT_PALETTE_CUSTOM, "GIMP_CONVERT_PALETTE_CUSTOM", "custom" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_CONVERT_PALETTE_GENERATE, NC_("convert-palette-type", "Generate optimum palette"), NULL },
    { GIMP_CONVERT_PALETTE_WEB, NC_("convert-palette-type", "Use web-optimized palette"), NULL },
    { GIMP_CONVERT_PALETTE_MONO, NC_("convert-palette-type", "Use black and white (1-bit) palette"), NULL },
    { GIMP_CONVERT_PALETTE_CUSTOM, NC_("convert-palette-type", "Use custom palette"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpConvertPaletteType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "convert-palette-type");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_convolve_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_CONVOLVE_BLUR, "GIMP_CONVOLVE_BLUR", "blur" },
    { GIMP_CONVOLVE_SHARPEN, "GIMP_CONVOLVE_SHARPEN", "sharpen" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_CONVOLVE_BLUR, NC_("convolve-type", "Blur"), NULL },
    { GIMP_CONVOLVE_SHARPEN, NC_("convolve-type", "Sharpen"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpConvolveType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "convolve-type");
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
    { GIMP_DESATURATE_LUMA, "GIMP_DESATURATE_LUMA", "luma" },
    { GIMP_DESATURATE_AVERAGE, "GIMP_DESATURATE_AVERAGE", "average" },
    { GIMP_DESATURATE_LUMINANCE, "GIMP_DESATURATE_LUMINANCE", "luminance" },
    { GIMP_DESATURATE_VALUE, "GIMP_DESATURATE_VALUE", "value" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_DESATURATE_LIGHTNESS, NC_("desaturate-mode", "Lightness (HSL)"), NULL },
    { GIMP_DESATURATE_LUMA, NC_("desaturate-mode", "Luma"), NULL },
    { GIMP_DESATURATE_AVERAGE, NC_("desaturate-mode", "Average (HSI Intensity)"), NULL },
    { GIMP_DESATURATE_LUMINANCE, NC_("desaturate-mode", "Luminance"), NULL },
    { GIMP_DESATURATE_VALUE, NC_("desaturate-mode", "Value (HSV)"), NULL },
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
    { GIMP_DODGE_BURN_TYPE_DODGE, "GIMP_DODGE_BURN_TYPE_DODGE", "dodge" },
    { GIMP_DODGE_BURN_TYPE_BURN, "GIMP_DODGE_BURN_TYPE_BURN", "burn" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_DODGE_BURN_TYPE_DODGE, NC_("dodge-burn-type", "Dodge"), NULL },
    { GIMP_DODGE_BURN_TYPE_BURN, NC_("dodge-burn-type", "Burn"), NULL },
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
gimp_fill_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_FILL_FOREGROUND, "GIMP_FILL_FOREGROUND", "foreground" },
    { GIMP_FILL_BACKGROUND, "GIMP_FILL_BACKGROUND", "background" },
    { GIMP_FILL_CIELAB_MIDDLE_GRAY, "GIMP_FILL_CIELAB_MIDDLE_GRAY", "cielab-middle-gray" },
    { GIMP_FILL_WHITE, "GIMP_FILL_WHITE", "white" },
    { GIMP_FILL_TRANSPARENT, "GIMP_FILL_TRANSPARENT", "transparent" },
    { GIMP_FILL_PATTERN, "GIMP_FILL_PATTERN", "pattern" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_FILL_FOREGROUND, NC_("fill-type", "Foreground color"), NULL },
    { GIMP_FILL_BACKGROUND, NC_("fill-type", "Background color"), NULL },
    { GIMP_FILL_CIELAB_MIDDLE_GRAY, NC_("fill-type", "Middle Gray (CIELAB)"), NULL },
    { GIMP_FILL_WHITE, NC_("fill-type", "White"), NULL },
    { GIMP_FILL_TRANSPARENT, NC_("fill-type", "Transparency"), NULL },
    { GIMP_FILL_PATTERN, NC_("fill-type", "Pattern"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpFillType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "fill-type");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_foreground_extract_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_FOREGROUND_EXTRACT_MATTING, "GIMP_FOREGROUND_EXTRACT_MATTING", "matting" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_FOREGROUND_EXTRACT_MATTING, "GIMP_FOREGROUND_EXTRACT_MATTING", NULL },
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
gimp_gradient_blend_color_space_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_GRADIENT_BLEND_RGB_PERCEPTUAL, "GIMP_GRADIENT_BLEND_RGB_PERCEPTUAL", "rgb-perceptual" },
    { GIMP_GRADIENT_BLEND_RGB_LINEAR, "GIMP_GRADIENT_BLEND_RGB_LINEAR", "rgb-linear" },
    { GIMP_GRADIENT_BLEND_CIE_LAB, "GIMP_GRADIENT_BLEND_CIE_LAB", "cie-lab" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_GRADIENT_BLEND_RGB_PERCEPTUAL, NC_("gradient-blend-color-space", "Perceptual RGB"), NULL },
    { GIMP_GRADIENT_BLEND_RGB_LINEAR, NC_("gradient-blend-color-space", "Linear RGB"), NULL },
    { GIMP_GRADIENT_BLEND_CIE_LAB, NC_("gradient-blend-color-space", "CIE Lab"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpGradientBlendColorSpace", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "gradient-blend-color-space");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_gradient_segment_color_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_GRADIENT_SEGMENT_RGB, "GIMP_GRADIENT_SEGMENT_RGB", "rgb" },
    { GIMP_GRADIENT_SEGMENT_HSV_CCW, "GIMP_GRADIENT_SEGMENT_HSV_CCW", "hsv-ccw" },
    { GIMP_GRADIENT_SEGMENT_HSV_CW, "GIMP_GRADIENT_SEGMENT_HSV_CW", "hsv-cw" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_GRADIENT_SEGMENT_RGB, NC_("gradient-segment-color", "RGB"), NULL },
    { GIMP_GRADIENT_SEGMENT_HSV_CCW, NC_("gradient-segment-color", "HSV (counter-clockwise hue)"), NULL },
    /* Translators: this is an abbreviated version of "HSV (counter-clockwise hue)".
       Keep it short. */
    { GIMP_GRADIENT_SEGMENT_HSV_CCW, NC_("gradient-segment-color", "HSV (ccw)"), NULL },
    { GIMP_GRADIENT_SEGMENT_HSV_CW, NC_("gradient-segment-color", "HSV (clockwise hue)"), NULL },
    /* Translators: this is an abbreviated version of "HSV (clockwise hue)".
       Keep it short. */
    { GIMP_GRADIENT_SEGMENT_HSV_CW, NC_("gradient-segment-color", "HSV (cw)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpGradientSegmentColor", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "gradient-segment-color");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_gradient_segment_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_GRADIENT_SEGMENT_LINEAR, "GIMP_GRADIENT_SEGMENT_LINEAR", "linear" },
    { GIMP_GRADIENT_SEGMENT_CURVED, "GIMP_GRADIENT_SEGMENT_CURVED", "curved" },
    { GIMP_GRADIENT_SEGMENT_SINE, "GIMP_GRADIENT_SEGMENT_SINE", "sine" },
    { GIMP_GRADIENT_SEGMENT_SPHERE_INCREASING, "GIMP_GRADIENT_SEGMENT_SPHERE_INCREASING", "sphere-increasing" },
    { GIMP_GRADIENT_SEGMENT_SPHERE_DECREASING, "GIMP_GRADIENT_SEGMENT_SPHERE_DECREASING", "sphere-decreasing" },
    { GIMP_GRADIENT_SEGMENT_STEP, "GIMP_GRADIENT_SEGMENT_STEP", "step" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_GRADIENT_SEGMENT_LINEAR, NC_("gradient-segment-type", "Linear"), NULL },
    { GIMP_GRADIENT_SEGMENT_CURVED, NC_("gradient-segment-type", "Curved"), NULL },
    { GIMP_GRADIENT_SEGMENT_SINE, NC_("gradient-segment-type", "Sinusoidal"), NULL },
    { GIMP_GRADIENT_SEGMENT_SPHERE_INCREASING, NC_("gradient-segment-type", "Spherical (increasing)"), NULL },
    /* Translators: this is an abbreviated version of "Spherical (increasing)".
       Keep it short. */
    { GIMP_GRADIENT_SEGMENT_SPHERE_INCREASING, NC_("gradient-segment-type", "Spherical (inc)"), NULL },
    { GIMP_GRADIENT_SEGMENT_SPHERE_DECREASING, NC_("gradient-segment-type", "Spherical (decreasing)"), NULL },
    /* Translators: this is an abbreviated version of "Spherical (decreasing)".
       Keep it short. */
    { GIMP_GRADIENT_SEGMENT_SPHERE_DECREASING, NC_("gradient-segment-type", "Spherical (dec)"), NULL },
    { GIMP_GRADIENT_SEGMENT_STEP, NC_("gradient-segment-type", "Step"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpGradientSegmentType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "gradient-segment-type");
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
    { GIMP_GRADIENT_CONICAL_SYMMETRIC, NC_("gradient-type", "Conical (symmetric)"), NULL },
    /* Translators: this is an abbreviated version of "Conical (symmetric)".
       Keep it short. */
    { GIMP_GRADIENT_CONICAL_SYMMETRIC, NC_("gradient-type", "Conical (sym)"), NULL },
    { GIMP_GRADIENT_CONICAL_ASYMMETRIC, NC_("gradient-type", "Conical (asymmetric)"), NULL },
    /* Translators: this is an abbreviated version of "Conical (asymmetric)".
       Keep it short. */
    { GIMP_GRADIENT_CONICAL_ASYMMETRIC, NC_("gradient-type", "Conical (asym)"), NULL },
    { GIMP_GRADIENT_SHAPEBURST_ANGULAR, NC_("gradient-type", "Shaped (angular)"), NULL },
    { GIMP_GRADIENT_SHAPEBURST_SPHERICAL, NC_("gradient-type", "Shaped (spherical)"), NULL },
    { GIMP_GRADIENT_SHAPEBURST_DIMPLED, NC_("gradient-type", "Shaped (dimpled)"), NULL },
    { GIMP_GRADIENT_SPIRAL_CLOCKWISE, NC_("gradient-type", "Spiral (clockwise)"), NULL },
    /* Translators: this is an abbreviated version of "Spiral (clockwise)".
       Keep it short. */
    { GIMP_GRADIENT_SPIRAL_CLOCKWISE, NC_("gradient-type", "Spiral (cw)"), NULL },
    { GIMP_GRADIENT_SPIRAL_ANTICLOCKWISE, NC_("gradient-type", "Spiral (counter-clockwise)"), NULL },
    /* Translators: this is an abbreviated version of "Spiral (counter-clockwise)".
       Keep it short. */
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
gimp_hue_range_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_HUE_RANGE_ALL, "GIMP_HUE_RANGE_ALL", "all" },
    { GIMP_HUE_RANGE_RED, "GIMP_HUE_RANGE_RED", "red" },
    { GIMP_HUE_RANGE_YELLOW, "GIMP_HUE_RANGE_YELLOW", "yellow" },
    { GIMP_HUE_RANGE_GREEN, "GIMP_HUE_RANGE_GREEN", "green" },
    { GIMP_HUE_RANGE_CYAN, "GIMP_HUE_RANGE_CYAN", "cyan" },
    { GIMP_HUE_RANGE_BLUE, "GIMP_HUE_RANGE_BLUE", "blue" },
    { GIMP_HUE_RANGE_MAGENTA, "GIMP_HUE_RANGE_MAGENTA", "magenta" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_HUE_RANGE_ALL, "GIMP_HUE_RANGE_ALL", NULL },
    { GIMP_HUE_RANGE_RED, "GIMP_HUE_RANGE_RED", NULL },
    { GIMP_HUE_RANGE_YELLOW, "GIMP_HUE_RANGE_YELLOW", NULL },
    { GIMP_HUE_RANGE_GREEN, "GIMP_HUE_RANGE_GREEN", NULL },
    { GIMP_HUE_RANGE_CYAN, "GIMP_HUE_RANGE_CYAN", NULL },
    { GIMP_HUE_RANGE_BLUE, "GIMP_HUE_RANGE_BLUE", NULL },
    { GIMP_HUE_RANGE_MAGENTA, "GIMP_HUE_RANGE_MAGENTA", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpHueRange", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "hue-range");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_icon_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_ICON_TYPE_ICON_NAME, "GIMP_ICON_TYPE_ICON_NAME", "icon-name" },
    { GIMP_ICON_TYPE_PIXBUF, "GIMP_ICON_TYPE_PIXBUF", "pixbuf" },
    { GIMP_ICON_TYPE_IMAGE_FILE, "GIMP_ICON_TYPE_IMAGE_FILE", "image-file" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_ICON_TYPE_ICON_NAME, NC_("icon-type", "Icon name"), NULL },
    { GIMP_ICON_TYPE_PIXBUF, NC_("icon-type", "Pixbuf"), NULL },
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
gimp_ink_blob_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_INK_BLOB_TYPE_CIRCLE, "GIMP_INK_BLOB_TYPE_CIRCLE", "circle" },
    { GIMP_INK_BLOB_TYPE_SQUARE, "GIMP_INK_BLOB_TYPE_SQUARE", "square" },
    { GIMP_INK_BLOB_TYPE_DIAMOND, "GIMP_INK_BLOB_TYPE_DIAMOND", "diamond" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_INK_BLOB_TYPE_CIRCLE, NC_("ink-blob-type", "Circle"), NULL },
    { GIMP_INK_BLOB_TYPE_SQUARE, NC_("ink-blob-type", "Square"), NULL },
    { GIMP_INK_BLOB_TYPE_DIAMOND, NC_("ink-blob-type", "Diamond"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpInkBlobType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "ink-blob-type");
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
    { GIMP_INTERPOLATION_NOHALO, "GIMP_INTERPOLATION_NOHALO", "nohalo" },
    { GIMP_INTERPOLATION_LOHALO, "GIMP_INTERPOLATION_LOHALO", "lohalo" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_INTERPOLATION_NONE, NC_("interpolation-type", "None"), NULL },
    { GIMP_INTERPOLATION_LINEAR, NC_("interpolation-type", "Linear"), NULL },
    { GIMP_INTERPOLATION_CUBIC, NC_("interpolation-type", "Cubic"), NULL },
    { GIMP_INTERPOLATION_NOHALO, NC_("interpolation-type", "NoHalo"), NULL },
    { GIMP_INTERPOLATION_LOHALO, NC_("interpolation-type", "LoHalo"), NULL },
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
    { GIMP_JOIN_MITER, NC_("join-style", "Miter"), NULL },
    { GIMP_JOIN_ROUND, NC_("join-style", "Round"), NULL },
    { GIMP_JOIN_BEVEL, NC_("join-style", "Bevel"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpJoinStyle", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "join-style");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_mask_apply_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_MASK_APPLY, "GIMP_MASK_APPLY", "apply" },
    { GIMP_MASK_DISCARD, "GIMP_MASK_DISCARD", "discard" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_MASK_APPLY, "GIMP_MASK_APPLY", NULL },
    { GIMP_MASK_DISCARD, "GIMP_MASK_DISCARD", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpMaskApplyMode", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "mask-apply-mode");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_merge_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_EXPAND_AS_NECESSARY, "GIMP_EXPAND_AS_NECESSARY", "expand-as-necessary" },
    { GIMP_CLIP_TO_IMAGE, "GIMP_CLIP_TO_IMAGE", "clip-to-image" },
    { GIMP_CLIP_TO_BOTTOM_LAYER, "GIMP_CLIP_TO_BOTTOM_LAYER", "clip-to-bottom-layer" },
    { GIMP_FLATTEN_IMAGE, "GIMP_FLATTEN_IMAGE", "flatten-image" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_EXPAND_AS_NECESSARY, NC_("merge-type", "Expanded as necessary"), NULL },
    { GIMP_CLIP_TO_IMAGE, NC_("merge-type", "Clipped to image"), NULL },
    { GIMP_CLIP_TO_BOTTOM_LAYER, NC_("merge-type", "Clipped to bottom layer"), NULL },
    { GIMP_FLATTEN_IMAGE, NC_("merge-type", "Flatten"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpMergeType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "merge-type");
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
gimp_offset_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_OFFSET_BACKGROUND, "GIMP_OFFSET_BACKGROUND", "background" },
    { GIMP_OFFSET_TRANSPARENT, "GIMP_OFFSET_TRANSPARENT", "transparent" },
    { GIMP_OFFSET_WRAP_AROUND, "GIMP_OFFSET_WRAP_AROUND", "wrap-around" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_OFFSET_BACKGROUND, "GIMP_OFFSET_BACKGROUND", NULL },
    { GIMP_OFFSET_TRANSPARENT, "GIMP_OFFSET_TRANSPARENT", NULL },
    { GIMP_OFFSET_WRAP_AROUND, "GIMP_OFFSET_WRAP_AROUND", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpOffsetType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "offset-type");
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
    { GIMP_ORIENTATION_HORIZONTAL, NC_("orientation-type", "Horizontal"), NULL },
    { GIMP_ORIENTATION_VERTICAL, NC_("orientation-type", "Vertical"), NULL },
    { GIMP_ORIENTATION_UNKNOWN, NC_("orientation-type", "Unknown"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpOrientationType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "orientation-type");
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
    { GIMP_PDB_PROC_TYPE_INTERNAL, "GIMP_PDB_PROC_TYPE_INTERNAL", "internal" },
    { GIMP_PDB_PROC_TYPE_PLUGIN, "GIMP_PDB_PROC_TYPE_PLUGIN", "plugin" },
    { GIMP_PDB_PROC_TYPE_EXTENSION, "GIMP_PDB_PROC_TYPE_EXTENSION", "extension" },
    { GIMP_PDB_PROC_TYPE_TEMPORARY, "GIMP_PDB_PROC_TYPE_TEMPORARY", "temporary" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_PDB_PROC_TYPE_INTERNAL, NC_("pdb-proc-type", "Internal GIMP procedure"), NULL },
    { GIMP_PDB_PROC_TYPE_PLUGIN, NC_("pdb-proc-type", "GIMP Plug-In"), NULL },
    { GIMP_PDB_PROC_TYPE_EXTENSION, NC_("pdb-proc-type", "GIMP Extension"), NULL },
    { GIMP_PDB_PROC_TYPE_TEMPORARY, NC_("pdb-proc-type", "Temporary Procedure"), NULL },
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
gimp_precision_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_PRECISION_U8_LINEAR, "GIMP_PRECISION_U8_LINEAR", "u8-linear" },
    { GIMP_PRECISION_U8_NON_LINEAR, "GIMP_PRECISION_U8_NON_LINEAR", "u8-non-linear" },
    { GIMP_PRECISION_U8_PERCEPTUAL, "GIMP_PRECISION_U8_PERCEPTUAL", "u8-perceptual" },
    { GIMP_PRECISION_U16_LINEAR, "GIMP_PRECISION_U16_LINEAR", "u16-linear" },
    { GIMP_PRECISION_U16_NON_LINEAR, "GIMP_PRECISION_U16_NON_LINEAR", "u16-non-linear" },
    { GIMP_PRECISION_U16_PERCEPTUAL, "GIMP_PRECISION_U16_PERCEPTUAL", "u16-perceptual" },
    { GIMP_PRECISION_U32_LINEAR, "GIMP_PRECISION_U32_LINEAR", "u32-linear" },
    { GIMP_PRECISION_U32_NON_LINEAR, "GIMP_PRECISION_U32_NON_LINEAR", "u32-non-linear" },
    { GIMP_PRECISION_U32_PERCEPTUAL, "GIMP_PRECISION_U32_PERCEPTUAL", "u32-perceptual" },
    { GIMP_PRECISION_HALF_LINEAR, "GIMP_PRECISION_HALF_LINEAR", "half-linear" },
    { GIMP_PRECISION_HALF_NON_LINEAR, "GIMP_PRECISION_HALF_NON_LINEAR", "half-non-linear" },
    { GIMP_PRECISION_HALF_PERCEPTUAL, "GIMP_PRECISION_HALF_PERCEPTUAL", "half-perceptual" },
    { GIMP_PRECISION_FLOAT_LINEAR, "GIMP_PRECISION_FLOAT_LINEAR", "float-linear" },
    { GIMP_PRECISION_FLOAT_NON_LINEAR, "GIMP_PRECISION_FLOAT_NON_LINEAR", "float-non-linear" },
    { GIMP_PRECISION_FLOAT_PERCEPTUAL, "GIMP_PRECISION_FLOAT_PERCEPTUAL", "float-perceptual" },
    { GIMP_PRECISION_DOUBLE_LINEAR, "GIMP_PRECISION_DOUBLE_LINEAR", "double-linear" },
    { GIMP_PRECISION_DOUBLE_NON_LINEAR, "GIMP_PRECISION_DOUBLE_NON_LINEAR", "double-non-linear" },
    { GIMP_PRECISION_DOUBLE_PERCEPTUAL, "GIMP_PRECISION_DOUBLE_PERCEPTUAL", "double-perceptual" },
    { GIMP_PRECISION_U8_GAMMA, "GIMP_PRECISION_U8_GAMMA", "u8-gamma" },
    { GIMP_PRECISION_U16_GAMMA, "GIMP_PRECISION_U16_GAMMA", "u16-gamma" },
    { GIMP_PRECISION_U32_GAMMA, "GIMP_PRECISION_U32_GAMMA", "u32-gamma" },
    { GIMP_PRECISION_HALF_GAMMA, "GIMP_PRECISION_HALF_GAMMA", "half-gamma" },
    { GIMP_PRECISION_FLOAT_GAMMA, "GIMP_PRECISION_FLOAT_GAMMA", "float-gamma" },
    { GIMP_PRECISION_DOUBLE_GAMMA, "GIMP_PRECISION_DOUBLE_GAMMA", "double-gamma" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_PRECISION_U8_LINEAR, NC_("precision", "8-bit linear integer"), NULL },
    { GIMP_PRECISION_U8_NON_LINEAR, NC_("precision", "8-bit non-linear integer"), NULL },
    { GIMP_PRECISION_U8_PERCEPTUAL, NC_("precision", "8-bit perceptual integer"), NULL },
    { GIMP_PRECISION_U16_LINEAR, NC_("precision", "16-bit linear integer"), NULL },
    { GIMP_PRECISION_U16_NON_LINEAR, NC_("precision", "16-bit non-linear integer"), NULL },
    { GIMP_PRECISION_U16_PERCEPTUAL, NC_("precision", "16-bit perceptual integer"), NULL },
    { GIMP_PRECISION_U32_LINEAR, NC_("precision", "32-bit linear integer"), NULL },
    { GIMP_PRECISION_U32_NON_LINEAR, NC_("precision", "32-bit non-linear integer"), NULL },
    { GIMP_PRECISION_U32_PERCEPTUAL, NC_("precision", "32-bit perceptual integer"), NULL },
    { GIMP_PRECISION_HALF_LINEAR, NC_("precision", "16-bit linear floating point"), NULL },
    { GIMP_PRECISION_HALF_NON_LINEAR, NC_("precision", "16-bit non-linear floating point"), NULL },
    { GIMP_PRECISION_HALF_PERCEPTUAL, NC_("precision", "16-bit perceptual floating point"), NULL },
    { GIMP_PRECISION_FLOAT_LINEAR, NC_("precision", "32-bit linear floating point"), NULL },
    { GIMP_PRECISION_FLOAT_NON_LINEAR, NC_("precision", "32-bit non-linear floating point"), NULL },
    { GIMP_PRECISION_FLOAT_PERCEPTUAL, NC_("precision", "32-bit perceptual floating point"), NULL },
    { GIMP_PRECISION_DOUBLE_LINEAR, NC_("precision", "64-bit linear floating point"), NULL },
    { GIMP_PRECISION_DOUBLE_NON_LINEAR, NC_("precision", "64-bit non-linear floating point"), NULL },
    { GIMP_PRECISION_DOUBLE_PERCEPTUAL, NC_("precision", "64-bit perceptual floating point"), NULL },
    { GIMP_PRECISION_U8_GAMMA, "GIMP_PRECISION_U8_GAMMA", NULL },
    { GIMP_PRECISION_U16_GAMMA, "GIMP_PRECISION_U16_GAMMA", NULL },
    { GIMP_PRECISION_U32_GAMMA, "GIMP_PRECISION_U32_GAMMA", NULL },
    { GIMP_PRECISION_HALF_GAMMA, "GIMP_PRECISION_HALF_GAMMA", NULL },
    { GIMP_PRECISION_FLOAT_GAMMA, "GIMP_PRECISION_FLOAT_GAMMA", NULL },
    { GIMP_PRECISION_DOUBLE_GAMMA, "GIMP_PRECISION_DOUBLE_GAMMA", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpPrecision", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "precision");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_procedure_sensitivity_mask_get_type (void)
{
  static const GFlagsValue values[] =
  {
    { GIMP_PROCEDURE_SENSITIVE_DRAWABLE, "GIMP_PROCEDURE_SENSITIVE_DRAWABLE", "drawable" },
    { GIMP_PROCEDURE_SENSITIVE_DRAWABLES, "GIMP_PROCEDURE_SENSITIVE_DRAWABLES", "drawables" },
    { GIMP_PROCEDURE_SENSITIVE_NO_DRAWABLES, "GIMP_PROCEDURE_SENSITIVE_NO_DRAWABLES", "no-drawables" },
    { GIMP_PROCEDURE_SENSITIVE_NO_IMAGE, "GIMP_PROCEDURE_SENSITIVE_NO_IMAGE", "no-image" },
    { GIMP_PROCEDURE_SENSITIVE_ALWAYS, "GIMP_PROCEDURE_SENSITIVE_ALWAYS", "always" },
    { 0, NULL, NULL }
  };

  static const GimpFlagsDesc descs[] =
  {
    { GIMP_PROCEDURE_SENSITIVE_DRAWABLE, "GIMP_PROCEDURE_SENSITIVE_DRAWABLE", NULL },
    { GIMP_PROCEDURE_SENSITIVE_DRAWABLES, "GIMP_PROCEDURE_SENSITIVE_DRAWABLES", NULL },
    { GIMP_PROCEDURE_SENSITIVE_NO_DRAWABLES, "GIMP_PROCEDURE_SENSITIVE_NO_DRAWABLES", NULL },
    { GIMP_PROCEDURE_SENSITIVE_NO_IMAGE, "GIMP_PROCEDURE_SENSITIVE_NO_IMAGE", NULL },
    { GIMP_PROCEDURE_SENSITIVE_ALWAYS, "GIMP_PROCEDURE_SENSITIVE_ALWAYS", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_flags_register_static ("GimpProcedureSensitivityMask", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "procedure-sensitivity-mask");
      gimp_flags_set_value_descriptions (type, descs);
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
gimp_repeat_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_REPEAT_NONE, "GIMP_REPEAT_NONE", "none" },
    { GIMP_REPEAT_TRUNCATE, "GIMP_REPEAT_TRUNCATE", "truncate" },
    { GIMP_REPEAT_SAWTOOTH, "GIMP_REPEAT_SAWTOOTH", "sawtooth" },
    { GIMP_REPEAT_TRIANGULAR, "GIMP_REPEAT_TRIANGULAR", "triangular" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_REPEAT_NONE, NC_("repeat-mode", "None (extend)"), NULL },
    { GIMP_REPEAT_TRUNCATE, NC_("repeat-mode", "None (truncate)"), NULL },
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
gimp_rotation_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_ROTATE_DEGREES90, "GIMP_ROTATE_DEGREES90", "degrees90" },
    { GIMP_ROTATE_DEGREES180, "GIMP_ROTATE_DEGREES180", "degrees180" },
    { GIMP_ROTATE_DEGREES270, "GIMP_ROTATE_DEGREES270", "degrees270" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_ROTATE_DEGREES90, "GIMP_ROTATE_DEGREES90", NULL },
    { GIMP_ROTATE_DEGREES180, "GIMP_ROTATE_DEGREES180", NULL },
    { GIMP_ROTATE_DEGREES270, "GIMP_ROTATE_DEGREES270", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpRotationType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "rotation-type");
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
gimp_select_criterion_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_SELECT_CRITERION_COMPOSITE, "GIMP_SELECT_CRITERION_COMPOSITE", "composite" },
    { GIMP_SELECT_CRITERION_RGB_RED, "GIMP_SELECT_CRITERION_RGB_RED", "rgb-red" },
    { GIMP_SELECT_CRITERION_RGB_GREEN, "GIMP_SELECT_CRITERION_RGB_GREEN", "rgb-green" },
    { GIMP_SELECT_CRITERION_RGB_BLUE, "GIMP_SELECT_CRITERION_RGB_BLUE", "rgb-blue" },
    { GIMP_SELECT_CRITERION_HSV_HUE, "GIMP_SELECT_CRITERION_HSV_HUE", "hsv-hue" },
    { GIMP_SELECT_CRITERION_HSV_SATURATION, "GIMP_SELECT_CRITERION_HSV_SATURATION", "hsv-saturation" },
    { GIMP_SELECT_CRITERION_HSV_VALUE, "GIMP_SELECT_CRITERION_HSV_VALUE", "hsv-value" },
    { GIMP_SELECT_CRITERION_LCH_LIGHTNESS, "GIMP_SELECT_CRITERION_LCH_LIGHTNESS", "lch-lightness" },
    { GIMP_SELECT_CRITERION_LCH_CHROMA, "GIMP_SELECT_CRITERION_LCH_CHROMA", "lch-chroma" },
    { GIMP_SELECT_CRITERION_LCH_HUE, "GIMP_SELECT_CRITERION_LCH_HUE", "lch-hue" },
    { GIMP_SELECT_CRITERION_ALPHA, "GIMP_SELECT_CRITERION_ALPHA", "alpha" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_SELECT_CRITERION_COMPOSITE, NC_("select-criterion", "Composite"), NULL },
    { GIMP_SELECT_CRITERION_RGB_RED, NC_("select-criterion", "Red"), NULL },
    { GIMP_SELECT_CRITERION_RGB_GREEN, NC_("select-criterion", "Green"), NULL },
    { GIMP_SELECT_CRITERION_RGB_BLUE, NC_("select-criterion", "Blue"), NULL },
    { GIMP_SELECT_CRITERION_HSV_HUE, NC_("select-criterion", "HSV Hue"), NULL },
    { GIMP_SELECT_CRITERION_HSV_SATURATION, NC_("select-criterion", "HSV Saturation"), NULL },
    { GIMP_SELECT_CRITERION_HSV_VALUE, NC_("select-criterion", "HSV Value"), NULL },
    { GIMP_SELECT_CRITERION_LCH_LIGHTNESS, NC_("select-criterion", "LCh Lightness"), NULL },
    { GIMP_SELECT_CRITERION_LCH_CHROMA, NC_("select-criterion", "LCh Chroma"), NULL },
    { GIMP_SELECT_CRITERION_LCH_HUE, NC_("select-criterion", "LCh Hue"), NULL },
    { GIMP_SELECT_CRITERION_ALPHA, NC_("select-criterion", "Alpha"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpSelectCriterion", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "select-criterion");
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
gimp_stroke_method_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_STROKE_LINE, "GIMP_STROKE_LINE", "line" },
    { GIMP_STROKE_PAINT_METHOD, "GIMP_STROKE_PAINT_METHOD", "paint-method" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_STROKE_LINE, NC_("stroke-method", "Stroke line"), NULL },
    { GIMP_STROKE_PAINT_METHOD, NC_("stroke-method", "Stroke with a paint tool"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpStrokeMethod", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "stroke-method");
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
    { GIMP_TEXT_DIRECTION_TTB_RTL, "GIMP_TEXT_DIRECTION_TTB_RTL", "ttb-rtl" },
    { GIMP_TEXT_DIRECTION_TTB_RTL_UPRIGHT, "GIMP_TEXT_DIRECTION_TTB_RTL_UPRIGHT", "ttb-rtl-upright" },
    { GIMP_TEXT_DIRECTION_TTB_LTR, "GIMP_TEXT_DIRECTION_TTB_LTR", "ttb-ltr" },
    { GIMP_TEXT_DIRECTION_TTB_LTR_UPRIGHT, "GIMP_TEXT_DIRECTION_TTB_LTR_UPRIGHT", "ttb-ltr-upright" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_TEXT_DIRECTION_LTR, NC_("text-direction", "From left to right"), NULL },
    { GIMP_TEXT_DIRECTION_RTL, NC_("text-direction", "From right to left"), NULL },
    { GIMP_TEXT_DIRECTION_TTB_RTL, NC_("text-direction", "Vertical, right to left (mixed orientation)"), NULL },
    { GIMP_TEXT_DIRECTION_TTB_RTL_UPRIGHT, NC_("text-direction", "Vertical, right to left (upright orientation)"), NULL },
    { GIMP_TEXT_DIRECTION_TTB_LTR, NC_("text-direction", "Vertical, left to right (mixed orientation)"), NULL },
    { GIMP_TEXT_DIRECTION_TTB_LTR_UPRIGHT, NC_("text-direction", "Vertical, left to right (upright orientation)"), NULL },
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
gimp_transfer_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_TRANSFER_SHADOWS, "GIMP_TRANSFER_SHADOWS", "shadows" },
    { GIMP_TRANSFER_MIDTONES, "GIMP_TRANSFER_MIDTONES", "midtones" },
    { GIMP_TRANSFER_HIGHLIGHTS, "GIMP_TRANSFER_HIGHLIGHTS", "highlights" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_TRANSFER_SHADOWS, NC_("transfer-mode", "Shadows"), NULL },
    { GIMP_TRANSFER_MIDTONES, NC_("transfer-mode", "Midtones"), NULL },
    { GIMP_TRANSFER_HIGHLIGHTS, NC_("transfer-mode", "Highlights"), NULL },
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
gimp_path_stroke_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_PATH_STROKE_TYPE_BEZIER, "GIMP_PATH_STROKE_TYPE_BEZIER", "bezier" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_PATH_STROKE_TYPE_BEZIER, "GIMP_PATH_STROKE_TYPE_BEZIER", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpPathStrokeType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "path-stroke-type");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

