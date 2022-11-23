
/* Generated data (by ligma-mkenums) */

#include "stamp-ligmabaseenums.h"
#include "config.h"
#include <glib-object.h>
#undef LIGMA_DISABLE_DEPRECATED
#include "ligmabasetypes.h"
#include "libligma/libligma-intl.h"
#include "ligmabaseenums.h"


/* enumerations from "ligmabaseenums.h" */
GType
ligma_add_mask_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_ADD_MASK_WHITE, "LIGMA_ADD_MASK_WHITE", "white" },
    { LIGMA_ADD_MASK_BLACK, "LIGMA_ADD_MASK_BLACK", "black" },
    { LIGMA_ADD_MASK_ALPHA, "LIGMA_ADD_MASK_ALPHA", "alpha" },
    { LIGMA_ADD_MASK_ALPHA_TRANSFER, "LIGMA_ADD_MASK_ALPHA_TRANSFER", "alpha-transfer" },
    { LIGMA_ADD_MASK_SELECTION, "LIGMA_ADD_MASK_SELECTION", "selection" },
    { LIGMA_ADD_MASK_COPY, "LIGMA_ADD_MASK_COPY", "copy" },
    { LIGMA_ADD_MASK_CHANNEL, "LIGMA_ADD_MASK_CHANNEL", "channel" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_ADD_MASK_WHITE, NC_("add-mask-type", "_White (full opacity)"), NULL },
    { LIGMA_ADD_MASK_BLACK, NC_("add-mask-type", "_Black (full transparency)"), NULL },
    { LIGMA_ADD_MASK_ALPHA, NC_("add-mask-type", "Layer's _alpha channel"), NULL },
    { LIGMA_ADD_MASK_ALPHA_TRANSFER, NC_("add-mask-type", "_Transfer layer's alpha channel"), NULL },
    { LIGMA_ADD_MASK_SELECTION, NC_("add-mask-type", "_Selection"), NULL },
    { LIGMA_ADD_MASK_COPY, NC_("add-mask-type", "_Grayscale copy of layer"), NULL },
    { LIGMA_ADD_MASK_CHANNEL, NC_("add-mask-type", "C_hannel"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaAddMaskType", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "add-mask-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_brush_generated_shape_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_BRUSH_GENERATED_CIRCLE, "LIGMA_BRUSH_GENERATED_CIRCLE", "circle" },
    { LIGMA_BRUSH_GENERATED_SQUARE, "LIGMA_BRUSH_GENERATED_SQUARE", "square" },
    { LIGMA_BRUSH_GENERATED_DIAMOND, "LIGMA_BRUSH_GENERATED_DIAMOND", "diamond" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_BRUSH_GENERATED_CIRCLE, NC_("brush-generated-shape", "Circle"), NULL },
    { LIGMA_BRUSH_GENERATED_SQUARE, NC_("brush-generated-shape", "Square"), NULL },
    { LIGMA_BRUSH_GENERATED_DIAMOND, NC_("brush-generated-shape", "Diamond"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaBrushGeneratedShape", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "brush-generated-shape");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_cap_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_CAP_BUTT, "LIGMA_CAP_BUTT", "butt" },
    { LIGMA_CAP_ROUND, "LIGMA_CAP_ROUND", "round" },
    { LIGMA_CAP_SQUARE, "LIGMA_CAP_SQUARE", "square" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_CAP_BUTT, NC_("cap-style", "Butt"), NULL },
    { LIGMA_CAP_ROUND, NC_("cap-style", "Round"), NULL },
    { LIGMA_CAP_SQUARE, NC_("cap-style", "Square"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaCapStyle", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "cap-style");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_channel_ops_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_CHANNEL_OP_ADD, "LIGMA_CHANNEL_OP_ADD", "add" },
    { LIGMA_CHANNEL_OP_SUBTRACT, "LIGMA_CHANNEL_OP_SUBTRACT", "subtract" },
    { LIGMA_CHANNEL_OP_REPLACE, "LIGMA_CHANNEL_OP_REPLACE", "replace" },
    { LIGMA_CHANNEL_OP_INTERSECT, "LIGMA_CHANNEL_OP_INTERSECT", "intersect" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_CHANNEL_OP_ADD, NC_("channel-ops", "Add to the current selection"), NULL },
    { LIGMA_CHANNEL_OP_SUBTRACT, NC_("channel-ops", "Subtract from the current selection"), NULL },
    { LIGMA_CHANNEL_OP_REPLACE, NC_("channel-ops", "Replace the current selection"), NULL },
    { LIGMA_CHANNEL_OP_INTERSECT, NC_("channel-ops", "Intersect with the current selection"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaChannelOps", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "channel-ops");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_channel_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_CHANNEL_RED, "LIGMA_CHANNEL_RED", "red" },
    { LIGMA_CHANNEL_GREEN, "LIGMA_CHANNEL_GREEN", "green" },
    { LIGMA_CHANNEL_BLUE, "LIGMA_CHANNEL_BLUE", "blue" },
    { LIGMA_CHANNEL_GRAY, "LIGMA_CHANNEL_GRAY", "gray" },
    { LIGMA_CHANNEL_INDEXED, "LIGMA_CHANNEL_INDEXED", "indexed" },
    { LIGMA_CHANNEL_ALPHA, "LIGMA_CHANNEL_ALPHA", "alpha" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_CHANNEL_RED, NC_("channel-type", "Red"), NULL },
    { LIGMA_CHANNEL_GREEN, NC_("channel-type", "Green"), NULL },
    { LIGMA_CHANNEL_BLUE, NC_("channel-type", "Blue"), NULL },
    { LIGMA_CHANNEL_GRAY, NC_("channel-type", "Gray"), NULL },
    { LIGMA_CHANNEL_INDEXED, NC_("channel-type", "Indexed"), NULL },
    { LIGMA_CHANNEL_ALPHA, NC_("channel-type", "Alpha"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaChannelType", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "channel-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_check_size_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_CHECK_SIZE_SMALL_CHECKS, "LIGMA_CHECK_SIZE_SMALL_CHECKS", "small-checks" },
    { LIGMA_CHECK_SIZE_MEDIUM_CHECKS, "LIGMA_CHECK_SIZE_MEDIUM_CHECKS", "medium-checks" },
    { LIGMA_CHECK_SIZE_LARGE_CHECKS, "LIGMA_CHECK_SIZE_LARGE_CHECKS", "large-checks" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_CHECK_SIZE_SMALL_CHECKS, NC_("check-size", "Small"), NULL },
    { LIGMA_CHECK_SIZE_MEDIUM_CHECKS, NC_("check-size", "Medium"), NULL },
    { LIGMA_CHECK_SIZE_LARGE_CHECKS, NC_("check-size", "Large"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaCheckSize", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "check-size");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_check_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_CHECK_TYPE_LIGHT_CHECKS, "LIGMA_CHECK_TYPE_LIGHT_CHECKS", "light-checks" },
    { LIGMA_CHECK_TYPE_GRAY_CHECKS, "LIGMA_CHECK_TYPE_GRAY_CHECKS", "gray-checks" },
    { LIGMA_CHECK_TYPE_DARK_CHECKS, "LIGMA_CHECK_TYPE_DARK_CHECKS", "dark-checks" },
    { LIGMA_CHECK_TYPE_WHITE_ONLY, "LIGMA_CHECK_TYPE_WHITE_ONLY", "white-only" },
    { LIGMA_CHECK_TYPE_GRAY_ONLY, "LIGMA_CHECK_TYPE_GRAY_ONLY", "gray-only" },
    { LIGMA_CHECK_TYPE_BLACK_ONLY, "LIGMA_CHECK_TYPE_BLACK_ONLY", "black-only" },
    { LIGMA_CHECK_TYPE_CUSTOM_CHECKS, "LIGMA_CHECK_TYPE_CUSTOM_CHECKS", "custom-checks" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_CHECK_TYPE_LIGHT_CHECKS, NC_("check-type", "Light checks"), NULL },
    { LIGMA_CHECK_TYPE_GRAY_CHECKS, NC_("check-type", "Mid-tone checks"), NULL },
    { LIGMA_CHECK_TYPE_DARK_CHECKS, NC_("check-type", "Dark checks"), NULL },
    { LIGMA_CHECK_TYPE_WHITE_ONLY, NC_("check-type", "White only"), NULL },
    { LIGMA_CHECK_TYPE_GRAY_ONLY, NC_("check-type", "Gray only"), NULL },
    { LIGMA_CHECK_TYPE_BLACK_ONLY, NC_("check-type", "Black only"), NULL },
    { LIGMA_CHECK_TYPE_CUSTOM_CHECKS, NC_("check-type", "Custom checks"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaCheckType", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "check-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_clone_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_CLONE_IMAGE, "LIGMA_CLONE_IMAGE", "image" },
    { LIGMA_CLONE_PATTERN, "LIGMA_CLONE_PATTERN", "pattern" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_CLONE_IMAGE, NC_("clone-type", "Image"), NULL },
    { LIGMA_CLONE_PATTERN, NC_("clone-type", "Pattern"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaCloneType", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "clone-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_color_tag_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_COLOR_TAG_NONE, "LIGMA_COLOR_TAG_NONE", "none" },
    { LIGMA_COLOR_TAG_BLUE, "LIGMA_COLOR_TAG_BLUE", "blue" },
    { LIGMA_COLOR_TAG_GREEN, "LIGMA_COLOR_TAG_GREEN", "green" },
    { LIGMA_COLOR_TAG_YELLOW, "LIGMA_COLOR_TAG_YELLOW", "yellow" },
    { LIGMA_COLOR_TAG_ORANGE, "LIGMA_COLOR_TAG_ORANGE", "orange" },
    { LIGMA_COLOR_TAG_BROWN, "LIGMA_COLOR_TAG_BROWN", "brown" },
    { LIGMA_COLOR_TAG_RED, "LIGMA_COLOR_TAG_RED", "red" },
    { LIGMA_COLOR_TAG_VIOLET, "LIGMA_COLOR_TAG_VIOLET", "violet" },
    { LIGMA_COLOR_TAG_GRAY, "LIGMA_COLOR_TAG_GRAY", "gray" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_COLOR_TAG_NONE, NC_("color-tag", "None"), NULL },
    { LIGMA_COLOR_TAG_BLUE, NC_("color-tag", "Blue"), NULL },
    { LIGMA_COLOR_TAG_GREEN, NC_("color-tag", "Green"), NULL },
    { LIGMA_COLOR_TAG_YELLOW, NC_("color-tag", "Yellow"), NULL },
    { LIGMA_COLOR_TAG_ORANGE, NC_("color-tag", "Orange"), NULL },
    { LIGMA_COLOR_TAG_BROWN, NC_("color-tag", "Brown"), NULL },
    { LIGMA_COLOR_TAG_RED, NC_("color-tag", "Red"), NULL },
    { LIGMA_COLOR_TAG_VIOLET, NC_("color-tag", "Violet"), NULL },
    { LIGMA_COLOR_TAG_GRAY, NC_("color-tag", "Gray"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaColorTag", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "color-tag");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_component_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_COMPONENT_TYPE_U8, "LIGMA_COMPONENT_TYPE_U8", "u8" },
    { LIGMA_COMPONENT_TYPE_U16, "LIGMA_COMPONENT_TYPE_U16", "u16" },
    { LIGMA_COMPONENT_TYPE_U32, "LIGMA_COMPONENT_TYPE_U32", "u32" },
    { LIGMA_COMPONENT_TYPE_HALF, "LIGMA_COMPONENT_TYPE_HALF", "half" },
    { LIGMA_COMPONENT_TYPE_FLOAT, "LIGMA_COMPONENT_TYPE_FLOAT", "float" },
    { LIGMA_COMPONENT_TYPE_DOUBLE, "LIGMA_COMPONENT_TYPE_DOUBLE", "double" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_COMPONENT_TYPE_U8, NC_("component-type", "8-bit integer"), NULL },
    { LIGMA_COMPONENT_TYPE_U16, NC_("component-type", "16-bit integer"), NULL },
    { LIGMA_COMPONENT_TYPE_U32, NC_("component-type", "32-bit integer"), NULL },
    { LIGMA_COMPONENT_TYPE_HALF, NC_("component-type", "16-bit floating point"), NULL },
    { LIGMA_COMPONENT_TYPE_FLOAT, NC_("component-type", "32-bit floating point"), NULL },
    { LIGMA_COMPONENT_TYPE_DOUBLE, NC_("component-type", "64-bit floating point"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaComponentType", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "component-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_convert_palette_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_CONVERT_PALETTE_GENERATE, "LIGMA_CONVERT_PALETTE_GENERATE", "generate" },
    { LIGMA_CONVERT_PALETTE_WEB, "LIGMA_CONVERT_PALETTE_WEB", "web" },
    { LIGMA_CONVERT_PALETTE_MONO, "LIGMA_CONVERT_PALETTE_MONO", "mono" },
    { LIGMA_CONVERT_PALETTE_CUSTOM, "LIGMA_CONVERT_PALETTE_CUSTOM", "custom" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_CONVERT_PALETTE_GENERATE, NC_("convert-palette-type", "Generate optimum palette"), NULL },
    { LIGMA_CONVERT_PALETTE_WEB, NC_("convert-palette-type", "Use web-optimized palette"), NULL },
    { LIGMA_CONVERT_PALETTE_MONO, NC_("convert-palette-type", "Use black and white (1-bit) palette"), NULL },
    { LIGMA_CONVERT_PALETTE_CUSTOM, NC_("convert-palette-type", "Use custom palette"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaConvertPaletteType", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "convert-palette-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_convolve_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_CONVOLVE_BLUR, "LIGMA_CONVOLVE_BLUR", "blur" },
    { LIGMA_CONVOLVE_SHARPEN, "LIGMA_CONVOLVE_SHARPEN", "sharpen" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_CONVOLVE_BLUR, NC_("convolve-type", "Blur"), NULL },
    { LIGMA_CONVOLVE_SHARPEN, NC_("convolve-type", "Sharpen"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaConvolveType", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "convolve-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_desaturate_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_DESATURATE_LIGHTNESS, "LIGMA_DESATURATE_LIGHTNESS", "lightness" },
    { LIGMA_DESATURATE_LUMA, "LIGMA_DESATURATE_LUMA", "luma" },
    { LIGMA_DESATURATE_AVERAGE, "LIGMA_DESATURATE_AVERAGE", "average" },
    { LIGMA_DESATURATE_LUMINANCE, "LIGMA_DESATURATE_LUMINANCE", "luminance" },
    { LIGMA_DESATURATE_VALUE, "LIGMA_DESATURATE_VALUE", "value" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_DESATURATE_LIGHTNESS, NC_("desaturate-mode", "Lightness (HSL)"), NULL },
    { LIGMA_DESATURATE_LUMA, NC_("desaturate-mode", "Luma"), NULL },
    { LIGMA_DESATURATE_AVERAGE, NC_("desaturate-mode", "Average (HSI Intensity)"), NULL },
    { LIGMA_DESATURATE_LUMINANCE, NC_("desaturate-mode", "Luminance"), NULL },
    { LIGMA_DESATURATE_VALUE, NC_("desaturate-mode", "Value (HSV)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaDesaturateMode", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "desaturate-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_dodge_burn_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_DODGE_BURN_TYPE_DODGE, "LIGMA_DODGE_BURN_TYPE_DODGE", "dodge" },
    { LIGMA_DODGE_BURN_TYPE_BURN, "LIGMA_DODGE_BURN_TYPE_BURN", "burn" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_DODGE_BURN_TYPE_DODGE, NC_("dodge-burn-type", "Dodge"), NULL },
    { LIGMA_DODGE_BURN_TYPE_BURN, NC_("dodge-burn-type", "Burn"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaDodgeBurnType", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "dodge-burn-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_fill_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_FILL_FOREGROUND, "LIGMA_FILL_FOREGROUND", "foreground" },
    { LIGMA_FILL_BACKGROUND, "LIGMA_FILL_BACKGROUND", "background" },
    { LIGMA_FILL_WHITE, "LIGMA_FILL_WHITE", "white" },
    { LIGMA_FILL_TRANSPARENT, "LIGMA_FILL_TRANSPARENT", "transparent" },
    { LIGMA_FILL_PATTERN, "LIGMA_FILL_PATTERN", "pattern" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_FILL_FOREGROUND, NC_("fill-type", "Foreground color"), NULL },
    { LIGMA_FILL_BACKGROUND, NC_("fill-type", "Background color"), NULL },
    { LIGMA_FILL_WHITE, NC_("fill-type", "White"), NULL },
    { LIGMA_FILL_TRANSPARENT, NC_("fill-type", "Transparency"), NULL },
    { LIGMA_FILL_PATTERN, NC_("fill-type", "Pattern"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaFillType", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "fill-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_foreground_extract_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_FOREGROUND_EXTRACT_MATTING, "LIGMA_FOREGROUND_EXTRACT_MATTING", "matting" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_FOREGROUND_EXTRACT_MATTING, "LIGMA_FOREGROUND_EXTRACT_MATTING", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaForegroundExtractMode", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "foreground-extract-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_gradient_blend_color_space_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_GRADIENT_BLEND_RGB_PERCEPTUAL, "LIGMA_GRADIENT_BLEND_RGB_PERCEPTUAL", "rgb-perceptual" },
    { LIGMA_GRADIENT_BLEND_RGB_LINEAR, "LIGMA_GRADIENT_BLEND_RGB_LINEAR", "rgb-linear" },
    { LIGMA_GRADIENT_BLEND_CIE_LAB, "LIGMA_GRADIENT_BLEND_CIE_LAB", "cie-lab" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_GRADIENT_BLEND_RGB_PERCEPTUAL, NC_("gradient-blend-color-space", "Perceptual RGB"), NULL },
    { LIGMA_GRADIENT_BLEND_RGB_LINEAR, NC_("gradient-blend-color-space", "Linear RGB"), NULL },
    { LIGMA_GRADIENT_BLEND_CIE_LAB, NC_("gradient-blend-color-space", "CIE Lab"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaGradientBlendColorSpace", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "gradient-blend-color-space");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_gradient_segment_color_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_GRADIENT_SEGMENT_RGB, "LIGMA_GRADIENT_SEGMENT_RGB", "rgb" },
    { LIGMA_GRADIENT_SEGMENT_HSV_CCW, "LIGMA_GRADIENT_SEGMENT_HSV_CCW", "hsv-ccw" },
    { LIGMA_GRADIENT_SEGMENT_HSV_CW, "LIGMA_GRADIENT_SEGMENT_HSV_CW", "hsv-cw" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_GRADIENT_SEGMENT_RGB, NC_("gradient-segment-color", "RGB"), NULL },
    { LIGMA_GRADIENT_SEGMENT_HSV_CCW, NC_("gradient-segment-color", "HSV (counter-clockwise hue)"), NULL },
    /* Translators: this is an abbreviated version of "HSV (counter-clockwise hue)".
       Keep it short. */
    { LIGMA_GRADIENT_SEGMENT_HSV_CCW, NC_("gradient-segment-color", "HSV (ccw)"), NULL },
    { LIGMA_GRADIENT_SEGMENT_HSV_CW, NC_("gradient-segment-color", "HSV (clockwise hue)"), NULL },
    /* Translators: this is an abbreviated version of "HSV (clockwise hue)".
       Keep it short. */
    { LIGMA_GRADIENT_SEGMENT_HSV_CW, NC_("gradient-segment-color", "HSV (cw)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaGradientSegmentColor", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "gradient-segment-color");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_gradient_segment_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_GRADIENT_SEGMENT_LINEAR, "LIGMA_GRADIENT_SEGMENT_LINEAR", "linear" },
    { LIGMA_GRADIENT_SEGMENT_CURVED, "LIGMA_GRADIENT_SEGMENT_CURVED", "curved" },
    { LIGMA_GRADIENT_SEGMENT_SINE, "LIGMA_GRADIENT_SEGMENT_SINE", "sine" },
    { LIGMA_GRADIENT_SEGMENT_SPHERE_INCREASING, "LIGMA_GRADIENT_SEGMENT_SPHERE_INCREASING", "sphere-increasing" },
    { LIGMA_GRADIENT_SEGMENT_SPHERE_DECREASING, "LIGMA_GRADIENT_SEGMENT_SPHERE_DECREASING", "sphere-decreasing" },
    { LIGMA_GRADIENT_SEGMENT_STEP, "LIGMA_GRADIENT_SEGMENT_STEP", "step" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_GRADIENT_SEGMENT_LINEAR, NC_("gradient-segment-type", "Linear"), NULL },
    { LIGMA_GRADIENT_SEGMENT_CURVED, NC_("gradient-segment-type", "Curved"), NULL },
    { LIGMA_GRADIENT_SEGMENT_SINE, NC_("gradient-segment-type", "Sinusoidal"), NULL },
    { LIGMA_GRADIENT_SEGMENT_SPHERE_INCREASING, NC_("gradient-segment-type", "Spherical (increasing)"), NULL },
    /* Translators: this is an abbreviated version of "Spherical (increasing)".
       Keep it short. */
    { LIGMA_GRADIENT_SEGMENT_SPHERE_INCREASING, NC_("gradient-segment-type", "Spherical (inc)"), NULL },
    { LIGMA_GRADIENT_SEGMENT_SPHERE_DECREASING, NC_("gradient-segment-type", "Spherical (decreasing)"), NULL },
    /* Translators: this is an abbreviated version of "Spherical (decreasing)".
       Keep it short. */
    { LIGMA_GRADIENT_SEGMENT_SPHERE_DECREASING, NC_("gradient-segment-type", "Spherical (dec)"), NULL },
    { LIGMA_GRADIENT_SEGMENT_STEP, NC_("gradient-segment-type", "Step"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaGradientSegmentType", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "gradient-segment-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_gradient_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_GRADIENT_LINEAR, "LIGMA_GRADIENT_LINEAR", "linear" },
    { LIGMA_GRADIENT_BILINEAR, "LIGMA_GRADIENT_BILINEAR", "bilinear" },
    { LIGMA_GRADIENT_RADIAL, "LIGMA_GRADIENT_RADIAL", "radial" },
    { LIGMA_GRADIENT_SQUARE, "LIGMA_GRADIENT_SQUARE", "square" },
    { LIGMA_GRADIENT_CONICAL_SYMMETRIC, "LIGMA_GRADIENT_CONICAL_SYMMETRIC", "conical-symmetric" },
    { LIGMA_GRADIENT_CONICAL_ASYMMETRIC, "LIGMA_GRADIENT_CONICAL_ASYMMETRIC", "conical-asymmetric" },
    { LIGMA_GRADIENT_SHAPEBURST_ANGULAR, "LIGMA_GRADIENT_SHAPEBURST_ANGULAR", "shapeburst-angular" },
    { LIGMA_GRADIENT_SHAPEBURST_SPHERICAL, "LIGMA_GRADIENT_SHAPEBURST_SPHERICAL", "shapeburst-spherical" },
    { LIGMA_GRADIENT_SHAPEBURST_DIMPLED, "LIGMA_GRADIENT_SHAPEBURST_DIMPLED", "shapeburst-dimpled" },
    { LIGMA_GRADIENT_SPIRAL_CLOCKWISE, "LIGMA_GRADIENT_SPIRAL_CLOCKWISE", "spiral-clockwise" },
    { LIGMA_GRADIENT_SPIRAL_ANTICLOCKWISE, "LIGMA_GRADIENT_SPIRAL_ANTICLOCKWISE", "spiral-anticlockwise" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_GRADIENT_LINEAR, NC_("gradient-type", "Linear"), NULL },
    { LIGMA_GRADIENT_BILINEAR, NC_("gradient-type", "Bi-linear"), NULL },
    { LIGMA_GRADIENT_RADIAL, NC_("gradient-type", "Radial"), NULL },
    { LIGMA_GRADIENT_SQUARE, NC_("gradient-type", "Square"), NULL },
    { LIGMA_GRADIENT_CONICAL_SYMMETRIC, NC_("gradient-type", "Conical (symmetric)"), NULL },
    /* Translators: this is an abbreviated version of "Conical (symmetric)".
       Keep it short. */
    { LIGMA_GRADIENT_CONICAL_SYMMETRIC, NC_("gradient-type", "Conical (sym)"), NULL },
    { LIGMA_GRADIENT_CONICAL_ASYMMETRIC, NC_("gradient-type", "Conical (asymmetric)"), NULL },
    /* Translators: this is an abbreviated version of "Conical (asymmetric)".
       Keep it short. */
    { LIGMA_GRADIENT_CONICAL_ASYMMETRIC, NC_("gradient-type", "Conical (asym)"), NULL },
    { LIGMA_GRADIENT_SHAPEBURST_ANGULAR, NC_("gradient-type", "Shaped (angular)"), NULL },
    { LIGMA_GRADIENT_SHAPEBURST_SPHERICAL, NC_("gradient-type", "Shaped (spherical)"), NULL },
    { LIGMA_GRADIENT_SHAPEBURST_DIMPLED, NC_("gradient-type", "Shaped (dimpled)"), NULL },
    { LIGMA_GRADIENT_SPIRAL_CLOCKWISE, NC_("gradient-type", "Spiral (clockwise)"), NULL },
    /* Translators: this is an abbreviated version of "Spiral (clockwise)".
       Keep it short. */
    { LIGMA_GRADIENT_SPIRAL_CLOCKWISE, NC_("gradient-type", "Spiral (cw)"), NULL },
    { LIGMA_GRADIENT_SPIRAL_ANTICLOCKWISE, NC_("gradient-type", "Spiral (counter-clockwise)"), NULL },
    /* Translators: this is an abbreviated version of "Spiral (counter-clockwise)".
       Keep it short. */
    { LIGMA_GRADIENT_SPIRAL_ANTICLOCKWISE, NC_("gradient-type", "Spiral (ccw)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaGradientType", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "gradient-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_grid_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_GRID_DOTS, "LIGMA_GRID_DOTS", "dots" },
    { LIGMA_GRID_INTERSECTIONS, "LIGMA_GRID_INTERSECTIONS", "intersections" },
    { LIGMA_GRID_ON_OFF_DASH, "LIGMA_GRID_ON_OFF_DASH", "on-off-dash" },
    { LIGMA_GRID_DOUBLE_DASH, "LIGMA_GRID_DOUBLE_DASH", "double-dash" },
    { LIGMA_GRID_SOLID, "LIGMA_GRID_SOLID", "solid" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_GRID_DOTS, NC_("grid-style", "Intersections (dots)"), NULL },
    { LIGMA_GRID_INTERSECTIONS, NC_("grid-style", "Intersections (crosshairs)"), NULL },
    { LIGMA_GRID_ON_OFF_DASH, NC_("grid-style", "Dashed"), NULL },
    { LIGMA_GRID_DOUBLE_DASH, NC_("grid-style", "Double dashed"), NULL },
    { LIGMA_GRID_SOLID, NC_("grid-style", "Solid"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaGridStyle", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "grid-style");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_hue_range_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_HUE_RANGE_ALL, "LIGMA_HUE_RANGE_ALL", "all" },
    { LIGMA_HUE_RANGE_RED, "LIGMA_HUE_RANGE_RED", "red" },
    { LIGMA_HUE_RANGE_YELLOW, "LIGMA_HUE_RANGE_YELLOW", "yellow" },
    { LIGMA_HUE_RANGE_GREEN, "LIGMA_HUE_RANGE_GREEN", "green" },
    { LIGMA_HUE_RANGE_CYAN, "LIGMA_HUE_RANGE_CYAN", "cyan" },
    { LIGMA_HUE_RANGE_BLUE, "LIGMA_HUE_RANGE_BLUE", "blue" },
    { LIGMA_HUE_RANGE_MAGENTA, "LIGMA_HUE_RANGE_MAGENTA", "magenta" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_HUE_RANGE_ALL, "LIGMA_HUE_RANGE_ALL", NULL },
    { LIGMA_HUE_RANGE_RED, "LIGMA_HUE_RANGE_RED", NULL },
    { LIGMA_HUE_RANGE_YELLOW, "LIGMA_HUE_RANGE_YELLOW", NULL },
    { LIGMA_HUE_RANGE_GREEN, "LIGMA_HUE_RANGE_GREEN", NULL },
    { LIGMA_HUE_RANGE_CYAN, "LIGMA_HUE_RANGE_CYAN", NULL },
    { LIGMA_HUE_RANGE_BLUE, "LIGMA_HUE_RANGE_BLUE", NULL },
    { LIGMA_HUE_RANGE_MAGENTA, "LIGMA_HUE_RANGE_MAGENTA", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaHueRange", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "hue-range");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_icon_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_ICON_TYPE_ICON_NAME, "LIGMA_ICON_TYPE_ICON_NAME", "icon-name" },
    { LIGMA_ICON_TYPE_PIXBUF, "LIGMA_ICON_TYPE_PIXBUF", "pixbuf" },
    { LIGMA_ICON_TYPE_IMAGE_FILE, "LIGMA_ICON_TYPE_IMAGE_FILE", "image-file" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_ICON_TYPE_ICON_NAME, NC_("icon-type", "Icon name"), NULL },
    { LIGMA_ICON_TYPE_PIXBUF, NC_("icon-type", "Pixbuf"), NULL },
    { LIGMA_ICON_TYPE_IMAGE_FILE, NC_("icon-type", "Image file"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaIconType", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "icon-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_image_base_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_RGB, "LIGMA_RGB", "rgb" },
    { LIGMA_GRAY, "LIGMA_GRAY", "gray" },
    { LIGMA_INDEXED, "LIGMA_INDEXED", "indexed" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_RGB, NC_("image-base-type", "RGB color"), NULL },
    { LIGMA_GRAY, NC_("image-base-type", "Grayscale"), NULL },
    { LIGMA_INDEXED, NC_("image-base-type", "Indexed color"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaImageBaseType", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "image-base-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_image_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_RGB_IMAGE, "LIGMA_RGB_IMAGE", "rgb-image" },
    { LIGMA_RGBA_IMAGE, "LIGMA_RGBA_IMAGE", "rgba-image" },
    { LIGMA_GRAY_IMAGE, "LIGMA_GRAY_IMAGE", "gray-image" },
    { LIGMA_GRAYA_IMAGE, "LIGMA_GRAYA_IMAGE", "graya-image" },
    { LIGMA_INDEXED_IMAGE, "LIGMA_INDEXED_IMAGE", "indexed-image" },
    { LIGMA_INDEXEDA_IMAGE, "LIGMA_INDEXEDA_IMAGE", "indexeda-image" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_RGB_IMAGE, NC_("image-type", "RGB"), NULL },
    { LIGMA_RGBA_IMAGE, NC_("image-type", "RGB-alpha"), NULL },
    { LIGMA_GRAY_IMAGE, NC_("image-type", "Grayscale"), NULL },
    { LIGMA_GRAYA_IMAGE, NC_("image-type", "Grayscale-alpha"), NULL },
    { LIGMA_INDEXED_IMAGE, NC_("image-type", "Indexed"), NULL },
    { LIGMA_INDEXEDA_IMAGE, NC_("image-type", "Indexed-alpha"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaImageType", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "image-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_ink_blob_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_INK_BLOB_TYPE_CIRCLE, "LIGMA_INK_BLOB_TYPE_CIRCLE", "circle" },
    { LIGMA_INK_BLOB_TYPE_SQUARE, "LIGMA_INK_BLOB_TYPE_SQUARE", "square" },
    { LIGMA_INK_BLOB_TYPE_DIAMOND, "LIGMA_INK_BLOB_TYPE_DIAMOND", "diamond" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_INK_BLOB_TYPE_CIRCLE, NC_("ink-blob-type", "Circle"), NULL },
    { LIGMA_INK_BLOB_TYPE_SQUARE, NC_("ink-blob-type", "Square"), NULL },
    { LIGMA_INK_BLOB_TYPE_DIAMOND, NC_("ink-blob-type", "Diamond"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaInkBlobType", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "ink-blob-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_interpolation_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_INTERPOLATION_NONE, "LIGMA_INTERPOLATION_NONE", "none" },
    { LIGMA_INTERPOLATION_LINEAR, "LIGMA_INTERPOLATION_LINEAR", "linear" },
    { LIGMA_INTERPOLATION_CUBIC, "LIGMA_INTERPOLATION_CUBIC", "cubic" },
    { LIGMA_INTERPOLATION_NOHALO, "LIGMA_INTERPOLATION_NOHALO", "nohalo" },
    { LIGMA_INTERPOLATION_LOHALO, "LIGMA_INTERPOLATION_LOHALO", "lohalo" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_INTERPOLATION_NONE, NC_("interpolation-type", "None"), NULL },
    { LIGMA_INTERPOLATION_LINEAR, NC_("interpolation-type", "Linear"), NULL },
    { LIGMA_INTERPOLATION_CUBIC, NC_("interpolation-type", "Cubic"), NULL },
    { LIGMA_INTERPOLATION_NOHALO, NC_("interpolation-type", "NoHalo"), NULL },
    { LIGMA_INTERPOLATION_LOHALO, NC_("interpolation-type", "LoHalo"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaInterpolationType", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "interpolation-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_join_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_JOIN_MITER, "LIGMA_JOIN_MITER", "miter" },
    { LIGMA_JOIN_ROUND, "LIGMA_JOIN_ROUND", "round" },
    { LIGMA_JOIN_BEVEL, "LIGMA_JOIN_BEVEL", "bevel" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_JOIN_MITER, NC_("join-style", "Miter"), NULL },
    { LIGMA_JOIN_ROUND, NC_("join-style", "Round"), NULL },
    { LIGMA_JOIN_BEVEL, NC_("join-style", "Bevel"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaJoinStyle", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "join-style");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_mask_apply_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_MASK_APPLY, "LIGMA_MASK_APPLY", "apply" },
    { LIGMA_MASK_DISCARD, "LIGMA_MASK_DISCARD", "discard" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_MASK_APPLY, "LIGMA_MASK_APPLY", NULL },
    { LIGMA_MASK_DISCARD, "LIGMA_MASK_DISCARD", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaMaskApplyMode", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "mask-apply-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_merge_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_EXPAND_AS_NECESSARY, "LIGMA_EXPAND_AS_NECESSARY", "expand-as-necessary" },
    { LIGMA_CLIP_TO_IMAGE, "LIGMA_CLIP_TO_IMAGE", "clip-to-image" },
    { LIGMA_CLIP_TO_BOTTOM_LAYER, "LIGMA_CLIP_TO_BOTTOM_LAYER", "clip-to-bottom-layer" },
    { LIGMA_FLATTEN_IMAGE, "LIGMA_FLATTEN_IMAGE", "flatten-image" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_EXPAND_AS_NECESSARY, NC_("merge-type", "Expanded as necessary"), NULL },
    { LIGMA_CLIP_TO_IMAGE, NC_("merge-type", "Clipped to image"), NULL },
    { LIGMA_CLIP_TO_BOTTOM_LAYER, NC_("merge-type", "Clipped to bottom layer"), NULL },
    { LIGMA_FLATTEN_IMAGE, NC_("merge-type", "Flatten"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaMergeType", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "merge-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_message_handler_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_MESSAGE_BOX, "LIGMA_MESSAGE_BOX", "message-box" },
    { LIGMA_CONSOLE, "LIGMA_CONSOLE", "console" },
    { LIGMA_ERROR_CONSOLE, "LIGMA_ERROR_CONSOLE", "error-console" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_MESSAGE_BOX, "LIGMA_MESSAGE_BOX", NULL },
    { LIGMA_CONSOLE, "LIGMA_CONSOLE", NULL },
    { LIGMA_ERROR_CONSOLE, "LIGMA_ERROR_CONSOLE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaMessageHandlerType", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "message-handler-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_offset_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_OFFSET_BACKGROUND, "LIGMA_OFFSET_BACKGROUND", "background" },
    { LIGMA_OFFSET_TRANSPARENT, "LIGMA_OFFSET_TRANSPARENT", "transparent" },
    { LIGMA_OFFSET_WRAP_AROUND, "LIGMA_OFFSET_WRAP_AROUND", "wrap-around" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_OFFSET_BACKGROUND, "LIGMA_OFFSET_BACKGROUND", NULL },
    { LIGMA_OFFSET_TRANSPARENT, "LIGMA_OFFSET_TRANSPARENT", NULL },
    { LIGMA_OFFSET_WRAP_AROUND, "LIGMA_OFFSET_WRAP_AROUND", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaOffsetType", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "offset-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_orientation_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_ORIENTATION_HORIZONTAL, "LIGMA_ORIENTATION_HORIZONTAL", "horizontal" },
    { LIGMA_ORIENTATION_VERTICAL, "LIGMA_ORIENTATION_VERTICAL", "vertical" },
    { LIGMA_ORIENTATION_UNKNOWN, "LIGMA_ORIENTATION_UNKNOWN", "unknown" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_ORIENTATION_HORIZONTAL, NC_("orientation-type", "Horizontal"), NULL },
    { LIGMA_ORIENTATION_VERTICAL, NC_("orientation-type", "Vertical"), NULL },
    { LIGMA_ORIENTATION_UNKNOWN, NC_("orientation-type", "Unknown"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaOrientationType", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "orientation-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_paint_application_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_PAINT_CONSTANT, "LIGMA_PAINT_CONSTANT", "constant" },
    { LIGMA_PAINT_INCREMENTAL, "LIGMA_PAINT_INCREMENTAL", "incremental" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_PAINT_CONSTANT, NC_("paint-application-mode", "Constant"), NULL },
    { LIGMA_PAINT_INCREMENTAL, NC_("paint-application-mode", "Incremental"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaPaintApplicationMode", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "paint-application-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_pdb_error_handler_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_PDB_ERROR_HANDLER_INTERNAL, "LIGMA_PDB_ERROR_HANDLER_INTERNAL", "internal" },
    { LIGMA_PDB_ERROR_HANDLER_PLUGIN, "LIGMA_PDB_ERROR_HANDLER_PLUGIN", "plugin" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_PDB_ERROR_HANDLER_INTERNAL, "LIGMA_PDB_ERROR_HANDLER_INTERNAL", NULL },
    { LIGMA_PDB_ERROR_HANDLER_PLUGIN, "LIGMA_PDB_ERROR_HANDLER_PLUGIN", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaPDBErrorHandler", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "pdb-error-handler");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_pdb_proc_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_PDB_PROC_TYPE_INTERNAL, "LIGMA_PDB_PROC_TYPE_INTERNAL", "internal" },
    { LIGMA_PDB_PROC_TYPE_PLUGIN, "LIGMA_PDB_PROC_TYPE_PLUGIN", "plugin" },
    { LIGMA_PDB_PROC_TYPE_EXTENSION, "LIGMA_PDB_PROC_TYPE_EXTENSION", "extension" },
    { LIGMA_PDB_PROC_TYPE_TEMPORARY, "LIGMA_PDB_PROC_TYPE_TEMPORARY", "temporary" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_PDB_PROC_TYPE_INTERNAL, NC_("pdb-proc-type", "Internal LIGMA procedure"), NULL },
    { LIGMA_PDB_PROC_TYPE_PLUGIN, NC_("pdb-proc-type", "LIGMA Plug-In"), NULL },
    { LIGMA_PDB_PROC_TYPE_EXTENSION, NC_("pdb-proc-type", "LIGMA Extension"), NULL },
    { LIGMA_PDB_PROC_TYPE_TEMPORARY, NC_("pdb-proc-type", "Temporary Procedure"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaPDBProcType", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "pdb-proc-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_pdb_status_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_PDB_EXECUTION_ERROR, "LIGMA_PDB_EXECUTION_ERROR", "execution-error" },
    { LIGMA_PDB_CALLING_ERROR, "LIGMA_PDB_CALLING_ERROR", "calling-error" },
    { LIGMA_PDB_PASS_THROUGH, "LIGMA_PDB_PASS_THROUGH", "pass-through" },
    { LIGMA_PDB_SUCCESS, "LIGMA_PDB_SUCCESS", "success" },
    { LIGMA_PDB_CANCEL, "LIGMA_PDB_CANCEL", "cancel" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_PDB_EXECUTION_ERROR, "LIGMA_PDB_EXECUTION_ERROR", NULL },
    { LIGMA_PDB_CALLING_ERROR, "LIGMA_PDB_CALLING_ERROR", NULL },
    { LIGMA_PDB_PASS_THROUGH, "LIGMA_PDB_PASS_THROUGH", NULL },
    { LIGMA_PDB_SUCCESS, "LIGMA_PDB_SUCCESS", NULL },
    { LIGMA_PDB_CANCEL, "LIGMA_PDB_CANCEL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaPDBStatusType", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "pdb-status-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_precision_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_PRECISION_U8_LINEAR, "LIGMA_PRECISION_U8_LINEAR", "u8-linear" },
    { LIGMA_PRECISION_U8_NON_LINEAR, "LIGMA_PRECISION_U8_NON_LINEAR", "u8-non-linear" },
    { LIGMA_PRECISION_U8_PERCEPTUAL, "LIGMA_PRECISION_U8_PERCEPTUAL", "u8-perceptual" },
    { LIGMA_PRECISION_U16_LINEAR, "LIGMA_PRECISION_U16_LINEAR", "u16-linear" },
    { LIGMA_PRECISION_U16_NON_LINEAR, "LIGMA_PRECISION_U16_NON_LINEAR", "u16-non-linear" },
    { LIGMA_PRECISION_U16_PERCEPTUAL, "LIGMA_PRECISION_U16_PERCEPTUAL", "u16-perceptual" },
    { LIGMA_PRECISION_U32_LINEAR, "LIGMA_PRECISION_U32_LINEAR", "u32-linear" },
    { LIGMA_PRECISION_U32_NON_LINEAR, "LIGMA_PRECISION_U32_NON_LINEAR", "u32-non-linear" },
    { LIGMA_PRECISION_U32_PERCEPTUAL, "LIGMA_PRECISION_U32_PERCEPTUAL", "u32-perceptual" },
    { LIGMA_PRECISION_HALF_LINEAR, "LIGMA_PRECISION_HALF_LINEAR", "half-linear" },
    { LIGMA_PRECISION_HALF_NON_LINEAR, "LIGMA_PRECISION_HALF_NON_LINEAR", "half-non-linear" },
    { LIGMA_PRECISION_HALF_PERCEPTUAL, "LIGMA_PRECISION_HALF_PERCEPTUAL", "half-perceptual" },
    { LIGMA_PRECISION_FLOAT_LINEAR, "LIGMA_PRECISION_FLOAT_LINEAR", "float-linear" },
    { LIGMA_PRECISION_FLOAT_NON_LINEAR, "LIGMA_PRECISION_FLOAT_NON_LINEAR", "float-non-linear" },
    { LIGMA_PRECISION_FLOAT_PERCEPTUAL, "LIGMA_PRECISION_FLOAT_PERCEPTUAL", "float-perceptual" },
    { LIGMA_PRECISION_DOUBLE_LINEAR, "LIGMA_PRECISION_DOUBLE_LINEAR", "double-linear" },
    { LIGMA_PRECISION_DOUBLE_NON_LINEAR, "LIGMA_PRECISION_DOUBLE_NON_LINEAR", "double-non-linear" },
    { LIGMA_PRECISION_DOUBLE_PERCEPTUAL, "LIGMA_PRECISION_DOUBLE_PERCEPTUAL", "double-perceptual" },
    { LIGMA_PRECISION_U8_GAMMA, "LIGMA_PRECISION_U8_GAMMA", "u8-gamma" },
    { LIGMA_PRECISION_U16_GAMMA, "LIGMA_PRECISION_U16_GAMMA", "u16-gamma" },
    { LIGMA_PRECISION_U32_GAMMA, "LIGMA_PRECISION_U32_GAMMA", "u32-gamma" },
    { LIGMA_PRECISION_HALF_GAMMA, "LIGMA_PRECISION_HALF_GAMMA", "half-gamma" },
    { LIGMA_PRECISION_FLOAT_GAMMA, "LIGMA_PRECISION_FLOAT_GAMMA", "float-gamma" },
    { LIGMA_PRECISION_DOUBLE_GAMMA, "LIGMA_PRECISION_DOUBLE_GAMMA", "double-gamma" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_PRECISION_U8_LINEAR, NC_("precision", "8-bit linear integer"), NULL },
    { LIGMA_PRECISION_U8_NON_LINEAR, NC_("precision", "8-bit non-linear integer"), NULL },
    { LIGMA_PRECISION_U8_PERCEPTUAL, NC_("precision", "8-bit perceptual integer"), NULL },
    { LIGMA_PRECISION_U16_LINEAR, NC_("precision", "16-bit linear integer"), NULL },
    { LIGMA_PRECISION_U16_NON_LINEAR, NC_("precision", "16-bit non-linear integer"), NULL },
    { LIGMA_PRECISION_U16_PERCEPTUAL, NC_("precision", "16-bit perceptual integer"), NULL },
    { LIGMA_PRECISION_U32_LINEAR, NC_("precision", "32-bit linear integer"), NULL },
    { LIGMA_PRECISION_U32_NON_LINEAR, NC_("precision", "32-bit non-linear integer"), NULL },
    { LIGMA_PRECISION_U32_PERCEPTUAL, NC_("precision", "32-bit perceptual integer"), NULL },
    { LIGMA_PRECISION_HALF_LINEAR, NC_("precision", "16-bit linear floating point"), NULL },
    { LIGMA_PRECISION_HALF_NON_LINEAR, NC_("precision", "16-bit non-linear floating point"), NULL },
    { LIGMA_PRECISION_HALF_PERCEPTUAL, NC_("precision", "16-bit perceptual floating point"), NULL },
    { LIGMA_PRECISION_FLOAT_LINEAR, NC_("precision", "32-bit linear floating point"), NULL },
    { LIGMA_PRECISION_FLOAT_NON_LINEAR, NC_("precision", "32-bit non-linear floating point"), NULL },
    { LIGMA_PRECISION_FLOAT_PERCEPTUAL, NC_("precision", "32-bit perceptual floating point"), NULL },
    { LIGMA_PRECISION_DOUBLE_LINEAR, NC_("precision", "64-bit linear floating point"), NULL },
    { LIGMA_PRECISION_DOUBLE_NON_LINEAR, NC_("precision", "64-bit non-linear floating point"), NULL },
    { LIGMA_PRECISION_DOUBLE_PERCEPTUAL, NC_("precision", "64-bit perceptual floating point"), NULL },
    { LIGMA_PRECISION_U8_GAMMA, "LIGMA_PRECISION_U8_GAMMA", NULL },
    { LIGMA_PRECISION_U16_GAMMA, "LIGMA_PRECISION_U16_GAMMA", NULL },
    { LIGMA_PRECISION_U32_GAMMA, "LIGMA_PRECISION_U32_GAMMA", NULL },
    { LIGMA_PRECISION_HALF_GAMMA, "LIGMA_PRECISION_HALF_GAMMA", NULL },
    { LIGMA_PRECISION_FLOAT_GAMMA, "LIGMA_PRECISION_FLOAT_GAMMA", NULL },
    { LIGMA_PRECISION_DOUBLE_GAMMA, "LIGMA_PRECISION_DOUBLE_GAMMA", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaPrecision", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "precision");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_procedure_sensitivity_mask_get_type (void)
{
  static const GFlagsValue values[] =
  {
    { LIGMA_PROCEDURE_SENSITIVE_DRAWABLE, "LIGMA_PROCEDURE_SENSITIVE_DRAWABLE", "drawable" },
    { LIGMA_PROCEDURE_SENSITIVE_DRAWABLES, "LIGMA_PROCEDURE_SENSITIVE_DRAWABLES", "drawables" },
    { LIGMA_PROCEDURE_SENSITIVE_NO_DRAWABLES, "LIGMA_PROCEDURE_SENSITIVE_NO_DRAWABLES", "no-drawables" },
    { LIGMA_PROCEDURE_SENSITIVE_NO_IMAGE, "LIGMA_PROCEDURE_SENSITIVE_NO_IMAGE", "no-image" },
    { LIGMA_PROCEDURE_SENSITIVE_ALWAYS, "LIGMA_PROCEDURE_SENSITIVE_ALWAYS", "always" },
    { 0, NULL, NULL }
  };

  static const LigmaFlagsDesc descs[] =
  {
    { LIGMA_PROCEDURE_SENSITIVE_DRAWABLE, "LIGMA_PROCEDURE_SENSITIVE_DRAWABLE", NULL },
    { LIGMA_PROCEDURE_SENSITIVE_DRAWABLES, "LIGMA_PROCEDURE_SENSITIVE_DRAWABLES", NULL },
    { LIGMA_PROCEDURE_SENSITIVE_NO_DRAWABLES, "LIGMA_PROCEDURE_SENSITIVE_NO_DRAWABLES", NULL },
    { LIGMA_PROCEDURE_SENSITIVE_NO_IMAGE, "LIGMA_PROCEDURE_SENSITIVE_NO_IMAGE", NULL },
    { LIGMA_PROCEDURE_SENSITIVE_ALWAYS, "LIGMA_PROCEDURE_SENSITIVE_ALWAYS", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_flags_register_static ("LigmaProcedureSensitivityMask", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "procedure-sensitivity-mask");
      ligma_flags_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_progress_command_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_PROGRESS_COMMAND_START, "LIGMA_PROGRESS_COMMAND_START", "start" },
    { LIGMA_PROGRESS_COMMAND_END, "LIGMA_PROGRESS_COMMAND_END", "end" },
    { LIGMA_PROGRESS_COMMAND_SET_TEXT, "LIGMA_PROGRESS_COMMAND_SET_TEXT", "set-text" },
    { LIGMA_PROGRESS_COMMAND_SET_VALUE, "LIGMA_PROGRESS_COMMAND_SET_VALUE", "set-value" },
    { LIGMA_PROGRESS_COMMAND_PULSE, "LIGMA_PROGRESS_COMMAND_PULSE", "pulse" },
    { LIGMA_PROGRESS_COMMAND_GET_WINDOW, "LIGMA_PROGRESS_COMMAND_GET_WINDOW", "get-window" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_PROGRESS_COMMAND_START, "LIGMA_PROGRESS_COMMAND_START", NULL },
    { LIGMA_PROGRESS_COMMAND_END, "LIGMA_PROGRESS_COMMAND_END", NULL },
    { LIGMA_PROGRESS_COMMAND_SET_TEXT, "LIGMA_PROGRESS_COMMAND_SET_TEXT", NULL },
    { LIGMA_PROGRESS_COMMAND_SET_VALUE, "LIGMA_PROGRESS_COMMAND_SET_VALUE", NULL },
    { LIGMA_PROGRESS_COMMAND_PULSE, "LIGMA_PROGRESS_COMMAND_PULSE", NULL },
    { LIGMA_PROGRESS_COMMAND_GET_WINDOW, "LIGMA_PROGRESS_COMMAND_GET_WINDOW", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaProgressCommand", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "progress-command");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_repeat_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_REPEAT_NONE, "LIGMA_REPEAT_NONE", "none" },
    { LIGMA_REPEAT_SAWTOOTH, "LIGMA_REPEAT_SAWTOOTH", "sawtooth" },
    { LIGMA_REPEAT_TRIANGULAR, "LIGMA_REPEAT_TRIANGULAR", "triangular" },
    { LIGMA_REPEAT_TRUNCATE, "LIGMA_REPEAT_TRUNCATE", "truncate" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_REPEAT_NONE, NC_("repeat-mode", "None (extend)"), NULL },
    { LIGMA_REPEAT_SAWTOOTH, NC_("repeat-mode", "Sawtooth wave"), NULL },
    { LIGMA_REPEAT_TRIANGULAR, NC_("repeat-mode", "Triangular wave"), NULL },
    { LIGMA_REPEAT_TRUNCATE, NC_("repeat-mode", "Truncate"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaRepeatMode", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "repeat-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_rotation_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_ROTATE_90, "LIGMA_ROTATE_90", "90" },
    { LIGMA_ROTATE_180, "LIGMA_ROTATE_180", "180" },
    { LIGMA_ROTATE_270, "LIGMA_ROTATE_270", "270" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_ROTATE_90, "LIGMA_ROTATE_90", NULL },
    { LIGMA_ROTATE_180, "LIGMA_ROTATE_180", NULL },
    { LIGMA_ROTATE_270, "LIGMA_ROTATE_270", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaRotationType", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "rotation-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_run_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_RUN_INTERACTIVE, "LIGMA_RUN_INTERACTIVE", "interactive" },
    { LIGMA_RUN_NONINTERACTIVE, "LIGMA_RUN_NONINTERACTIVE", "noninteractive" },
    { LIGMA_RUN_WITH_LAST_VALS, "LIGMA_RUN_WITH_LAST_VALS", "with-last-vals" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_RUN_INTERACTIVE, NC_("run-mode", "Run interactively"), NULL },
    { LIGMA_RUN_NONINTERACTIVE, NC_("run-mode", "Run non-interactively"), NULL },
    { LIGMA_RUN_WITH_LAST_VALS, NC_("run-mode", "Run with last used values"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaRunMode", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "run-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_select_criterion_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_SELECT_CRITERION_COMPOSITE, "LIGMA_SELECT_CRITERION_COMPOSITE", "composite" },
    { LIGMA_SELECT_CRITERION_RGB_RED, "LIGMA_SELECT_CRITERION_RGB_RED", "rgb-red" },
    { LIGMA_SELECT_CRITERION_RGB_GREEN, "LIGMA_SELECT_CRITERION_RGB_GREEN", "rgb-green" },
    { LIGMA_SELECT_CRITERION_RGB_BLUE, "LIGMA_SELECT_CRITERION_RGB_BLUE", "rgb-blue" },
    { LIGMA_SELECT_CRITERION_HSV_HUE, "LIGMA_SELECT_CRITERION_HSV_HUE", "hsv-hue" },
    { LIGMA_SELECT_CRITERION_HSV_SATURATION, "LIGMA_SELECT_CRITERION_HSV_SATURATION", "hsv-saturation" },
    { LIGMA_SELECT_CRITERION_HSV_VALUE, "LIGMA_SELECT_CRITERION_HSV_VALUE", "hsv-value" },
    { LIGMA_SELECT_CRITERION_LCH_LIGHTNESS, "LIGMA_SELECT_CRITERION_LCH_LIGHTNESS", "lch-lightness" },
    { LIGMA_SELECT_CRITERION_LCH_CHROMA, "LIGMA_SELECT_CRITERION_LCH_CHROMA", "lch-chroma" },
    { LIGMA_SELECT_CRITERION_LCH_HUE, "LIGMA_SELECT_CRITERION_LCH_HUE", "lch-hue" },
    { LIGMA_SELECT_CRITERION_ALPHA, "LIGMA_SELECT_CRITERION_ALPHA", "alpha" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_SELECT_CRITERION_COMPOSITE, NC_("select-criterion", "Composite"), NULL },
    { LIGMA_SELECT_CRITERION_RGB_RED, NC_("select-criterion", "Red"), NULL },
    { LIGMA_SELECT_CRITERION_RGB_GREEN, NC_("select-criterion", "Green"), NULL },
    { LIGMA_SELECT_CRITERION_RGB_BLUE, NC_("select-criterion", "Blue"), NULL },
    { LIGMA_SELECT_CRITERION_HSV_HUE, NC_("select-criterion", "HSV Hue"), NULL },
    { LIGMA_SELECT_CRITERION_HSV_SATURATION, NC_("select-criterion", "HSV Saturation"), NULL },
    { LIGMA_SELECT_CRITERION_HSV_VALUE, NC_("select-criterion", "HSV Value"), NULL },
    { LIGMA_SELECT_CRITERION_LCH_LIGHTNESS, NC_("select-criterion", "LCh Lightness"), NULL },
    { LIGMA_SELECT_CRITERION_LCH_CHROMA, NC_("select-criterion", "LCh Chroma"), NULL },
    { LIGMA_SELECT_CRITERION_LCH_HUE, NC_("select-criterion", "LCh Hue"), NULL },
    { LIGMA_SELECT_CRITERION_ALPHA, NC_("select-criterion", "Alpha"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaSelectCriterion", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "select-criterion");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_size_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_PIXELS, "LIGMA_PIXELS", "pixels" },
    { LIGMA_POINTS, "LIGMA_POINTS", "points" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_PIXELS, NC_("size-type", "Pixels"), NULL },
    { LIGMA_POINTS, NC_("size-type", "Points"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaSizeType", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "size-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_stack_trace_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_STACK_TRACE_NEVER, "LIGMA_STACK_TRACE_NEVER", "never" },
    { LIGMA_STACK_TRACE_QUERY, "LIGMA_STACK_TRACE_QUERY", "query" },
    { LIGMA_STACK_TRACE_ALWAYS, "LIGMA_STACK_TRACE_ALWAYS", "always" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_STACK_TRACE_NEVER, "LIGMA_STACK_TRACE_NEVER", NULL },
    { LIGMA_STACK_TRACE_QUERY, "LIGMA_STACK_TRACE_QUERY", NULL },
    { LIGMA_STACK_TRACE_ALWAYS, "LIGMA_STACK_TRACE_ALWAYS", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaStackTraceMode", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "stack-trace-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_stroke_method_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_STROKE_LINE, "LIGMA_STROKE_LINE", "line" },
    { LIGMA_STROKE_PAINT_METHOD, "LIGMA_STROKE_PAINT_METHOD", "paint-method" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_STROKE_LINE, NC_("stroke-method", "Stroke line"), NULL },
    { LIGMA_STROKE_PAINT_METHOD, NC_("stroke-method", "Stroke with a paint tool"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaStrokeMethod", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "stroke-method");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_text_direction_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_TEXT_DIRECTION_LTR, "LIGMA_TEXT_DIRECTION_LTR", "ltr" },
    { LIGMA_TEXT_DIRECTION_RTL, "LIGMA_TEXT_DIRECTION_RTL", "rtl" },
    { LIGMA_TEXT_DIRECTION_TTB_RTL, "LIGMA_TEXT_DIRECTION_TTB_RTL", "ttb-rtl" },
    { LIGMA_TEXT_DIRECTION_TTB_RTL_UPRIGHT, "LIGMA_TEXT_DIRECTION_TTB_RTL_UPRIGHT", "ttb-rtl-upright" },
    { LIGMA_TEXT_DIRECTION_TTB_LTR, "LIGMA_TEXT_DIRECTION_TTB_LTR", "ttb-ltr" },
    { LIGMA_TEXT_DIRECTION_TTB_LTR_UPRIGHT, "LIGMA_TEXT_DIRECTION_TTB_LTR_UPRIGHT", "ttb-ltr-upright" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_TEXT_DIRECTION_LTR, NC_("text-direction", "From left to right"), NULL },
    { LIGMA_TEXT_DIRECTION_RTL, NC_("text-direction", "From right to left"), NULL },
    { LIGMA_TEXT_DIRECTION_TTB_RTL, NC_("text-direction", "Vertical, right to left (mixed orientation)"), NULL },
    { LIGMA_TEXT_DIRECTION_TTB_RTL_UPRIGHT, NC_("text-direction", "Vertical, right to left (upright orientation)"), NULL },
    { LIGMA_TEXT_DIRECTION_TTB_LTR, NC_("text-direction", "Vertical, left to right (mixed orientation)"), NULL },
    { LIGMA_TEXT_DIRECTION_TTB_LTR_UPRIGHT, NC_("text-direction", "Vertical, left to right (upright orientation)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaTextDirection", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "text-direction");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_text_hint_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_TEXT_HINT_STYLE_NONE, "LIGMA_TEXT_HINT_STYLE_NONE", "none" },
    { LIGMA_TEXT_HINT_STYLE_SLIGHT, "LIGMA_TEXT_HINT_STYLE_SLIGHT", "slight" },
    { LIGMA_TEXT_HINT_STYLE_MEDIUM, "LIGMA_TEXT_HINT_STYLE_MEDIUM", "medium" },
    { LIGMA_TEXT_HINT_STYLE_FULL, "LIGMA_TEXT_HINT_STYLE_FULL", "full" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_TEXT_HINT_STYLE_NONE, NC_("text-hint-style", "None"), NULL },
    { LIGMA_TEXT_HINT_STYLE_SLIGHT, NC_("text-hint-style", "Slight"), NULL },
    { LIGMA_TEXT_HINT_STYLE_MEDIUM, NC_("text-hint-style", "Medium"), NULL },
    { LIGMA_TEXT_HINT_STYLE_FULL, NC_("text-hint-style", "Full"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaTextHintStyle", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "text-hint-style");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_text_justification_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_TEXT_JUSTIFY_LEFT, "LIGMA_TEXT_JUSTIFY_LEFT", "left" },
    { LIGMA_TEXT_JUSTIFY_RIGHT, "LIGMA_TEXT_JUSTIFY_RIGHT", "right" },
    { LIGMA_TEXT_JUSTIFY_CENTER, "LIGMA_TEXT_JUSTIFY_CENTER", "center" },
    { LIGMA_TEXT_JUSTIFY_FILL, "LIGMA_TEXT_JUSTIFY_FILL", "fill" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_TEXT_JUSTIFY_LEFT, NC_("text-justification", "Left justified"), NULL },
    { LIGMA_TEXT_JUSTIFY_RIGHT, NC_("text-justification", "Right justified"), NULL },
    { LIGMA_TEXT_JUSTIFY_CENTER, NC_("text-justification", "Centered"), NULL },
    { LIGMA_TEXT_JUSTIFY_FILL, NC_("text-justification", "Filled"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaTextJustification", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "text-justification");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_transfer_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_TRANSFER_SHADOWS, "LIGMA_TRANSFER_SHADOWS", "shadows" },
    { LIGMA_TRANSFER_MIDTONES, "LIGMA_TRANSFER_MIDTONES", "midtones" },
    { LIGMA_TRANSFER_HIGHLIGHTS, "LIGMA_TRANSFER_HIGHLIGHTS", "highlights" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_TRANSFER_SHADOWS, NC_("transfer-mode", "Shadows"), NULL },
    { LIGMA_TRANSFER_MIDTONES, NC_("transfer-mode", "Midtones"), NULL },
    { LIGMA_TRANSFER_HIGHLIGHTS, NC_("transfer-mode", "Highlights"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaTransferMode", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "transfer-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_transform_direction_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_TRANSFORM_FORWARD, "LIGMA_TRANSFORM_FORWARD", "forward" },
    { LIGMA_TRANSFORM_BACKWARD, "LIGMA_TRANSFORM_BACKWARD", "backward" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_TRANSFORM_FORWARD, NC_("transform-direction", "Normal (Forward)"), NULL },
    { LIGMA_TRANSFORM_BACKWARD, NC_("transform-direction", "Corrective (Backward)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaTransformDirection", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "transform-direction");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_transform_resize_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_TRANSFORM_RESIZE_ADJUST, "LIGMA_TRANSFORM_RESIZE_ADJUST", "adjust" },
    { LIGMA_TRANSFORM_RESIZE_CLIP, "LIGMA_TRANSFORM_RESIZE_CLIP", "clip" },
    { LIGMA_TRANSFORM_RESIZE_CROP, "LIGMA_TRANSFORM_RESIZE_CROP", "crop" },
    { LIGMA_TRANSFORM_RESIZE_CROP_WITH_ASPECT, "LIGMA_TRANSFORM_RESIZE_CROP_WITH_ASPECT", "crop-with-aspect" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_TRANSFORM_RESIZE_ADJUST, NC_("transform-resize", "Adjust"), NULL },
    { LIGMA_TRANSFORM_RESIZE_CLIP, NC_("transform-resize", "Clip"), NULL },
    { LIGMA_TRANSFORM_RESIZE_CROP, NC_("transform-resize", "Crop to result"), NULL },
    { LIGMA_TRANSFORM_RESIZE_CROP_WITH_ASPECT, NC_("transform-resize", "Crop with aspect"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaTransformResize", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "transform-resize");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_vectors_stroke_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_VECTORS_STROKE_TYPE_BEZIER, "LIGMA_VECTORS_STROKE_TYPE_BEZIER", "bezier" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_VECTORS_STROKE_TYPE_BEZIER, "LIGMA_VECTORS_STROKE_TYPE_BEZIER", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaVectorsStrokeType", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "vectors-stroke-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

