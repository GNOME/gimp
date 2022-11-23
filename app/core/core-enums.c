
/* Generated data (by ligma-mkenums) */

#include "stamp-core-enums.h"
#include "config.h"
#include <gio/gio.h>
#include "libligmabase/ligmabase.h"
#include "core-enums.h"
#include "ligma-intl.h"

/* enumerations from "core-enums.h" */
GType
ligma_align_reference_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_ALIGN_REFERENCE_IMAGE, "LIGMA_ALIGN_REFERENCE_IMAGE", "image" },
    { LIGMA_ALIGN_REFERENCE_SELECTION, "LIGMA_ALIGN_REFERENCE_SELECTION", "selection" },
    { LIGMA_ALIGN_REFERENCE_PICK, "LIGMA_ALIGN_REFERENCE_PICK", "pick" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_ALIGN_REFERENCE_IMAGE, NC_("align-reference-type", "Image"), NULL },
    { LIGMA_ALIGN_REFERENCE_SELECTION, NC_("align-reference-type", "Selection"), NULL },
    { LIGMA_ALIGN_REFERENCE_PICK, NC_("align-reference-type", "Picked reference object"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaAlignReferenceType", values);
      ligma_type_set_translation_context (type, "align-reference-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_alignment_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_ALIGN_LEFT, "LIGMA_ALIGN_LEFT", "align-left" },
    { LIGMA_ALIGN_HCENTER, "LIGMA_ALIGN_HCENTER", "align-hcenter" },
    { LIGMA_ALIGN_RIGHT, "LIGMA_ALIGN_RIGHT", "align-right" },
    { LIGMA_ALIGN_TOP, "LIGMA_ALIGN_TOP", "align-top" },
    { LIGMA_ALIGN_VCENTER, "LIGMA_ALIGN_VCENTER", "align-vcenter" },
    { LIGMA_ALIGN_BOTTOM, "LIGMA_ALIGN_BOTTOM", "align-bottom" },
    { LIGMA_ARRANGE_HFILL, "LIGMA_ARRANGE_HFILL", "arrange-hfill" },
    { LIGMA_ARRANGE_VFILL, "LIGMA_ARRANGE_VFILL", "arrange-vfill" },
    { LIGMA_DISTRIBUTE_EVEN_HORIZONTAL_GAP, "LIGMA_DISTRIBUTE_EVEN_HORIZONTAL_GAP", "distribute-even-horizontal-gap" },
    { LIGMA_DISTRIBUTE_EVEN_VERTICAL_GAP, "LIGMA_DISTRIBUTE_EVEN_VERTICAL_GAP", "distribute-even-vertical-gap" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_ALIGN_LEFT, NC_("alignment-type", "Align to the left"), NULL },
    { LIGMA_ALIGN_HCENTER, NC_("alignment-type", "Center horizontally"), NULL },
    { LIGMA_ALIGN_RIGHT, NC_("alignment-type", "Align to the right"), NULL },
    { LIGMA_ALIGN_TOP, NC_("alignment-type", "Align to the top"), NULL },
    { LIGMA_ALIGN_VCENTER, NC_("alignment-type", "Center vertically"), NULL },
    { LIGMA_ALIGN_BOTTOM, NC_("alignment-type", "Align to the bottom"), NULL },
    { LIGMA_ARRANGE_HFILL, NC_("alignment-type", "Distribute anchor points horizontally evenly"), NULL },
    { LIGMA_ARRANGE_VFILL, NC_("alignment-type", "Distribute anchor points vertically evenly"), NULL },
    { LIGMA_DISTRIBUTE_EVEN_HORIZONTAL_GAP, NC_("alignment-type", "Distribute horizontally with even horizontal gaps"), NULL },
    { LIGMA_DISTRIBUTE_EVEN_VERTICAL_GAP, NC_("alignment-type", "Distribute vertically with even vertical gaps"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaAlignmentType", values);
      ligma_type_set_translation_context (type, "alignment-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_bucket_fill_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_BUCKET_FILL_FG, "LIGMA_BUCKET_FILL_FG", "fg" },
    { LIGMA_BUCKET_FILL_BG, "LIGMA_BUCKET_FILL_BG", "bg" },
    { LIGMA_BUCKET_FILL_PATTERN, "LIGMA_BUCKET_FILL_PATTERN", "pattern" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_BUCKET_FILL_FG, NC_("bucket-fill-mode", "FG color fill"), NULL },
    { LIGMA_BUCKET_FILL_BG, NC_("bucket-fill-mode", "BG color fill"), NULL },
    { LIGMA_BUCKET_FILL_PATTERN, NC_("bucket-fill-mode", "Pattern fill"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaBucketFillMode", values);
      ligma_type_set_translation_context (type, "bucket-fill-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_channel_border_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_CHANNEL_BORDER_STYLE_HARD, "LIGMA_CHANNEL_BORDER_STYLE_HARD", "hard" },
    { LIGMA_CHANNEL_BORDER_STYLE_SMOOTH, "LIGMA_CHANNEL_BORDER_STYLE_SMOOTH", "smooth" },
    { LIGMA_CHANNEL_BORDER_STYLE_FEATHERED, "LIGMA_CHANNEL_BORDER_STYLE_FEATHERED", "feathered" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_CHANNEL_BORDER_STYLE_HARD, NC_("channel-border-style", "Hard"), NULL },
    { LIGMA_CHANNEL_BORDER_STYLE_SMOOTH, NC_("channel-border-style", "Smooth"), NULL },
    { LIGMA_CHANNEL_BORDER_STYLE_FEATHERED, NC_("channel-border-style", "Feathered"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaChannelBorderStyle", values);
      ligma_type_set_translation_context (type, "channel-border-style");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_color_pick_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_COLOR_PICK_MODE_PIXEL, "LIGMA_COLOR_PICK_MODE_PIXEL", "pixel" },
    { LIGMA_COLOR_PICK_MODE_RGB_PERCENT, "LIGMA_COLOR_PICK_MODE_RGB_PERCENT", "rgb-percent" },
    { LIGMA_COLOR_PICK_MODE_RGB_U8, "LIGMA_COLOR_PICK_MODE_RGB_U8", "rgb-u8" },
    { LIGMA_COLOR_PICK_MODE_HSV, "LIGMA_COLOR_PICK_MODE_HSV", "hsv" },
    { LIGMA_COLOR_PICK_MODE_LCH, "LIGMA_COLOR_PICK_MODE_LCH", "lch" },
    { LIGMA_COLOR_PICK_MODE_LAB, "LIGMA_COLOR_PICK_MODE_LAB", "lab" },
    { LIGMA_COLOR_PICK_MODE_CMYK, "LIGMA_COLOR_PICK_MODE_CMYK", "cmyk" },
    { LIGMA_COLOR_PICK_MODE_XYY, "LIGMA_COLOR_PICK_MODE_XYY", "xyy" },
    { LIGMA_COLOR_PICK_MODE_YUV, "LIGMA_COLOR_PICK_MODE_YUV", "yuv" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_COLOR_PICK_MODE_PIXEL, NC_("color-pick-mode", "Pixel"), NULL },
    { LIGMA_COLOR_PICK_MODE_RGB_PERCENT, NC_("color-pick-mode", "RGB (%)"), NULL },
    { LIGMA_COLOR_PICK_MODE_RGB_U8, NC_("color-pick-mode", "RGB (0..255)"), NULL },
    { LIGMA_COLOR_PICK_MODE_HSV, NC_("color-pick-mode", "HSV"), NULL },
    { LIGMA_COLOR_PICK_MODE_LCH, NC_("color-pick-mode", "CIE LCh"), NULL },
    { LIGMA_COLOR_PICK_MODE_LAB, NC_("color-pick-mode", "CIE LAB"), NULL },
    { LIGMA_COLOR_PICK_MODE_CMYK, NC_("color-pick-mode", "CMYK"), NULL },
    { LIGMA_COLOR_PICK_MODE_XYY, NC_("color-pick-mode", "CIE xyY"), NULL },
    { LIGMA_COLOR_PICK_MODE_YUV, NC_("color-pick-mode", "CIE Yu'v'"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaColorPickMode", values);
      ligma_type_set_translation_context (type, "color-pick-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_color_profile_policy_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_COLOR_PROFILE_POLICY_ASK, "LIGMA_COLOR_PROFILE_POLICY_ASK", "ask" },
    { LIGMA_COLOR_PROFILE_POLICY_KEEP, "LIGMA_COLOR_PROFILE_POLICY_KEEP", "keep" },
    { LIGMA_COLOR_PROFILE_POLICY_CONVERT_BUILTIN, "LIGMA_COLOR_PROFILE_POLICY_CONVERT_BUILTIN", "convert-builtin" },
    { LIGMA_COLOR_PROFILE_POLICY_CONVERT_PREFERRED, "LIGMA_COLOR_PROFILE_POLICY_CONVERT_PREFERRED", "convert-preferred" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_COLOR_PROFILE_POLICY_ASK, NC_("color-profile-policy", "Ask what to do"), NULL },
    { LIGMA_COLOR_PROFILE_POLICY_KEEP, NC_("color-profile-policy", "Keep embedded profile"), NULL },
    { LIGMA_COLOR_PROFILE_POLICY_CONVERT_BUILTIN, NC_("color-profile-policy", "Convert to built-in sRGB or grayscale profile"), NULL },
    { LIGMA_COLOR_PROFILE_POLICY_CONVERT_PREFERRED, NC_("color-profile-policy", "Convert to preferred RGB or grayscale profile (defaulting to built-in)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaColorProfilePolicy", values);
      ligma_type_set_translation_context (type, "color-profile-policy");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_component_mask_get_type (void)
{
  static const GFlagsValue values[] =
  {
    { LIGMA_COMPONENT_MASK_RED, "LIGMA_COMPONENT_MASK_RED", "red" },
    { LIGMA_COMPONENT_MASK_GREEN, "LIGMA_COMPONENT_MASK_GREEN", "green" },
    { LIGMA_COMPONENT_MASK_BLUE, "LIGMA_COMPONENT_MASK_BLUE", "blue" },
    { LIGMA_COMPONENT_MASK_ALPHA, "LIGMA_COMPONENT_MASK_ALPHA", "alpha" },
    { LIGMA_COMPONENT_MASK_ALL, "LIGMA_COMPONENT_MASK_ALL", "all" },
    { 0, NULL, NULL }
  };

  static const LigmaFlagsDesc descs[] =
  {
    { LIGMA_COMPONENT_MASK_RED, "LIGMA_COMPONENT_MASK_RED", NULL },
    { LIGMA_COMPONENT_MASK_GREEN, "LIGMA_COMPONENT_MASK_GREEN", NULL },
    { LIGMA_COMPONENT_MASK_BLUE, "LIGMA_COMPONENT_MASK_BLUE", NULL },
    { LIGMA_COMPONENT_MASK_ALPHA, "LIGMA_COMPONENT_MASK_ALPHA", NULL },
    { LIGMA_COMPONENT_MASK_ALL, "LIGMA_COMPONENT_MASK_ALL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_flags_register_static ("LigmaComponentMask", values);
      ligma_type_set_translation_context (type, "component-mask");
      ligma_flags_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_container_policy_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_CONTAINER_POLICY_STRONG, "LIGMA_CONTAINER_POLICY_STRONG", "strong" },
    { LIGMA_CONTAINER_POLICY_WEAK, "LIGMA_CONTAINER_POLICY_WEAK", "weak" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_CONTAINER_POLICY_STRONG, "LIGMA_CONTAINER_POLICY_STRONG", NULL },
    { LIGMA_CONTAINER_POLICY_WEAK, "LIGMA_CONTAINER_POLICY_WEAK", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaContainerPolicy", values);
      ligma_type_set_translation_context (type, "container-policy");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_convert_dither_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_CONVERT_DITHER_NONE, "LIGMA_CONVERT_DITHER_NONE", "none" },
    { LIGMA_CONVERT_DITHER_FS, "LIGMA_CONVERT_DITHER_FS", "fs" },
    { LIGMA_CONVERT_DITHER_FS_LOWBLEED, "LIGMA_CONVERT_DITHER_FS_LOWBLEED", "fs-lowbleed" },
    { LIGMA_CONVERT_DITHER_FIXED, "LIGMA_CONVERT_DITHER_FIXED", "fixed" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_CONVERT_DITHER_NONE, NC_("convert-dither-type", "None"), NULL },
    { LIGMA_CONVERT_DITHER_FS, NC_("convert-dither-type", "Floyd-Steinberg (normal)"), NULL },
    { LIGMA_CONVERT_DITHER_FS_LOWBLEED, NC_("convert-dither-type", "Floyd-Steinberg (reduced color bleeding)"), NULL },
    { LIGMA_CONVERT_DITHER_FIXED, NC_("convert-dither-type", "Positioned"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaConvertDitherType", values);
      ligma_type_set_translation_context (type, "convert-dither-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_convolution_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_NORMAL_CONVOL, "LIGMA_NORMAL_CONVOL", "normal-convol" },
    { LIGMA_ABSOLUTE_CONVOL, "LIGMA_ABSOLUTE_CONVOL", "absolute-convol" },
    { LIGMA_NEGATIVE_CONVOL, "LIGMA_NEGATIVE_CONVOL", "negative-convol" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_NORMAL_CONVOL, "LIGMA_NORMAL_CONVOL", NULL },
    { LIGMA_ABSOLUTE_CONVOL, "LIGMA_ABSOLUTE_CONVOL", NULL },
    { LIGMA_NEGATIVE_CONVOL, "LIGMA_NEGATIVE_CONVOL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaConvolutionType", values);
      ligma_type_set_translation_context (type, "convolution-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_curve_point_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_CURVE_POINT_SMOOTH, "LIGMA_CURVE_POINT_SMOOTH", "smooth" },
    { LIGMA_CURVE_POINT_CORNER, "LIGMA_CURVE_POINT_CORNER", "corner" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_CURVE_POINT_SMOOTH, NC_("curve-point-type", "Smooth"), NULL },
    { LIGMA_CURVE_POINT_CORNER, NC_("curve-point-type", "Corner"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaCurvePointType", values);
      ligma_type_set_translation_context (type, "curve-point-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_curve_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_CURVE_SMOOTH, "LIGMA_CURVE_SMOOTH", "smooth" },
    { LIGMA_CURVE_FREE, "LIGMA_CURVE_FREE", "free" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_CURVE_SMOOTH, NC_("curve-type", "Smooth"), NULL },
    { LIGMA_CURVE_FREE, NC_("curve-type", "Freehand"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaCurveType", values);
      ligma_type_set_translation_context (type, "curve-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_dash_preset_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_DASH_CUSTOM, "LIGMA_DASH_CUSTOM", "custom" },
    { LIGMA_DASH_LINE, "LIGMA_DASH_LINE", "line" },
    { LIGMA_DASH_LONG_DASH, "LIGMA_DASH_LONG_DASH", "long-dash" },
    { LIGMA_DASH_MEDIUM_DASH, "LIGMA_DASH_MEDIUM_DASH", "medium-dash" },
    { LIGMA_DASH_SHORT_DASH, "LIGMA_DASH_SHORT_DASH", "short-dash" },
    { LIGMA_DASH_SPARSE_DOTS, "LIGMA_DASH_SPARSE_DOTS", "sparse-dots" },
    { LIGMA_DASH_NORMAL_DOTS, "LIGMA_DASH_NORMAL_DOTS", "normal-dots" },
    { LIGMA_DASH_DENSE_DOTS, "LIGMA_DASH_DENSE_DOTS", "dense-dots" },
    { LIGMA_DASH_STIPPLES, "LIGMA_DASH_STIPPLES", "stipples" },
    { LIGMA_DASH_DASH_DOT, "LIGMA_DASH_DASH_DOT", "dash-dot" },
    { LIGMA_DASH_DASH_DOT_DOT, "LIGMA_DASH_DASH_DOT_DOT", "dash-dot-dot" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_DASH_CUSTOM, NC_("dash-preset", "Custom"), NULL },
    { LIGMA_DASH_LINE, NC_("dash-preset", "Line"), NULL },
    { LIGMA_DASH_LONG_DASH, NC_("dash-preset", "Long dashes"), NULL },
    { LIGMA_DASH_MEDIUM_DASH, NC_("dash-preset", "Medium dashes"), NULL },
    { LIGMA_DASH_SHORT_DASH, NC_("dash-preset", "Short dashes"), NULL },
    { LIGMA_DASH_SPARSE_DOTS, NC_("dash-preset", "Sparse dots"), NULL },
    { LIGMA_DASH_NORMAL_DOTS, NC_("dash-preset", "Normal dots"), NULL },
    { LIGMA_DASH_DENSE_DOTS, NC_("dash-preset", "Dense dots"), NULL },
    { LIGMA_DASH_STIPPLES, NC_("dash-preset", "Stipples"), NULL },
    { LIGMA_DASH_DASH_DOT, NC_("dash-preset", "Dash, dot"), NULL },
    { LIGMA_DASH_DASH_DOT_DOT, NC_("dash-preset", "Dash, dot, dot"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaDashPreset", values);
      ligma_type_set_translation_context (type, "dash-preset");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_debug_policy_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_DEBUG_POLICY_WARNING, "LIGMA_DEBUG_POLICY_WARNING", "warning" },
    { LIGMA_DEBUG_POLICY_CRITICAL, "LIGMA_DEBUG_POLICY_CRITICAL", "critical" },
    { LIGMA_DEBUG_POLICY_FATAL, "LIGMA_DEBUG_POLICY_FATAL", "fatal" },
    { LIGMA_DEBUG_POLICY_NEVER, "LIGMA_DEBUG_POLICY_NEVER", "never" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_DEBUG_POLICY_WARNING, NC_("debug-policy", "Debug warnings, critical errors and crashes"), NULL },
    { LIGMA_DEBUG_POLICY_CRITICAL, NC_("debug-policy", "Debug critical errors and crashes"), NULL },
    { LIGMA_DEBUG_POLICY_FATAL, NC_("debug-policy", "Debug crashes only"), NULL },
    { LIGMA_DEBUG_POLICY_NEVER, NC_("debug-policy", "Never debug LIGMA"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaDebugPolicy", values);
      ligma_type_set_translation_context (type, "debug-policy");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_dirty_mask_get_type (void)
{
  static const GFlagsValue values[] =
  {
    { LIGMA_DIRTY_NONE, "LIGMA_DIRTY_NONE", "none" },
    { LIGMA_DIRTY_IMAGE, "LIGMA_DIRTY_IMAGE", "image" },
    { LIGMA_DIRTY_IMAGE_SIZE, "LIGMA_DIRTY_IMAGE_SIZE", "image-size" },
    { LIGMA_DIRTY_IMAGE_META, "LIGMA_DIRTY_IMAGE_META", "image-meta" },
    { LIGMA_DIRTY_IMAGE_STRUCTURE, "LIGMA_DIRTY_IMAGE_STRUCTURE", "image-structure" },
    { LIGMA_DIRTY_ITEM, "LIGMA_DIRTY_ITEM", "item" },
    { LIGMA_DIRTY_ITEM_META, "LIGMA_DIRTY_ITEM_META", "item-meta" },
    { LIGMA_DIRTY_DRAWABLE, "LIGMA_DIRTY_DRAWABLE", "drawable" },
    { LIGMA_DIRTY_VECTORS, "LIGMA_DIRTY_VECTORS", "vectors" },
    { LIGMA_DIRTY_SELECTION, "LIGMA_DIRTY_SELECTION", "selection" },
    { LIGMA_DIRTY_ACTIVE_DRAWABLE, "LIGMA_DIRTY_ACTIVE_DRAWABLE", "active-drawable" },
    { LIGMA_DIRTY_ALL, "LIGMA_DIRTY_ALL", "all" },
    { 0, NULL, NULL }
  };

  static const LigmaFlagsDesc descs[] =
  {
    { LIGMA_DIRTY_NONE, "LIGMA_DIRTY_NONE", NULL },
    { LIGMA_DIRTY_IMAGE, "LIGMA_DIRTY_IMAGE", NULL },
    { LIGMA_DIRTY_IMAGE_SIZE, "LIGMA_DIRTY_IMAGE_SIZE", NULL },
    { LIGMA_DIRTY_IMAGE_META, "LIGMA_DIRTY_IMAGE_META", NULL },
    { LIGMA_DIRTY_IMAGE_STRUCTURE, "LIGMA_DIRTY_IMAGE_STRUCTURE", NULL },
    { LIGMA_DIRTY_ITEM, "LIGMA_DIRTY_ITEM", NULL },
    { LIGMA_DIRTY_ITEM_META, "LIGMA_DIRTY_ITEM_META", NULL },
    { LIGMA_DIRTY_DRAWABLE, "LIGMA_DIRTY_DRAWABLE", NULL },
    { LIGMA_DIRTY_VECTORS, "LIGMA_DIRTY_VECTORS", NULL },
    { LIGMA_DIRTY_SELECTION, "LIGMA_DIRTY_SELECTION", NULL },
    { LIGMA_DIRTY_ACTIVE_DRAWABLE, "LIGMA_DIRTY_ACTIVE_DRAWABLE", NULL },
    { LIGMA_DIRTY_ALL, "LIGMA_DIRTY_ALL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_flags_register_static ("LigmaDirtyMask", values);
      ligma_type_set_translation_context (type, "dirty-mask");
      ligma_flags_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_dynamics_output_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_DYNAMICS_OUTPUT_OPACITY, "LIGMA_DYNAMICS_OUTPUT_OPACITY", "opacity" },
    { LIGMA_DYNAMICS_OUTPUT_SIZE, "LIGMA_DYNAMICS_OUTPUT_SIZE", "size" },
    { LIGMA_DYNAMICS_OUTPUT_ANGLE, "LIGMA_DYNAMICS_OUTPUT_ANGLE", "angle" },
    { LIGMA_DYNAMICS_OUTPUT_COLOR, "LIGMA_DYNAMICS_OUTPUT_COLOR", "color" },
    { LIGMA_DYNAMICS_OUTPUT_HARDNESS, "LIGMA_DYNAMICS_OUTPUT_HARDNESS", "hardness" },
    { LIGMA_DYNAMICS_OUTPUT_FORCE, "LIGMA_DYNAMICS_OUTPUT_FORCE", "force" },
    { LIGMA_DYNAMICS_OUTPUT_ASPECT_RATIO, "LIGMA_DYNAMICS_OUTPUT_ASPECT_RATIO", "aspect-ratio" },
    { LIGMA_DYNAMICS_OUTPUT_SPACING, "LIGMA_DYNAMICS_OUTPUT_SPACING", "spacing" },
    { LIGMA_DYNAMICS_OUTPUT_RATE, "LIGMA_DYNAMICS_OUTPUT_RATE", "rate" },
    { LIGMA_DYNAMICS_OUTPUT_FLOW, "LIGMA_DYNAMICS_OUTPUT_FLOW", "flow" },
    { LIGMA_DYNAMICS_OUTPUT_JITTER, "LIGMA_DYNAMICS_OUTPUT_JITTER", "jitter" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_DYNAMICS_OUTPUT_OPACITY, NC_("dynamics-output-type", "Opacity"), NULL },
    { LIGMA_DYNAMICS_OUTPUT_SIZE, NC_("dynamics-output-type", "Size"), NULL },
    { LIGMA_DYNAMICS_OUTPUT_ANGLE, NC_("dynamics-output-type", "Angle"), NULL },
    { LIGMA_DYNAMICS_OUTPUT_COLOR, NC_("dynamics-output-type", "Color"), NULL },
    { LIGMA_DYNAMICS_OUTPUT_HARDNESS, NC_("dynamics-output-type", "Hardness"), NULL },
    { LIGMA_DYNAMICS_OUTPUT_FORCE, NC_("dynamics-output-type", "Force"), NULL },
    { LIGMA_DYNAMICS_OUTPUT_ASPECT_RATIO, NC_("dynamics-output-type", "Aspect ratio"), NULL },
    { LIGMA_DYNAMICS_OUTPUT_SPACING, NC_("dynamics-output-type", "Spacing"), NULL },
    { LIGMA_DYNAMICS_OUTPUT_RATE, NC_("dynamics-output-type", "Rate"), NULL },
    { LIGMA_DYNAMICS_OUTPUT_FLOW, NC_("dynamics-output-type", "Flow"), NULL },
    { LIGMA_DYNAMICS_OUTPUT_JITTER, NC_("dynamics-output-type", "Jitter"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaDynamicsOutputType", values);
      ligma_type_set_translation_context (type, "dynamics-output-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_fill_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_FILL_STYLE_SOLID, "LIGMA_FILL_STYLE_SOLID", "solid" },
    { LIGMA_FILL_STYLE_PATTERN, "LIGMA_FILL_STYLE_PATTERN", "pattern" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_FILL_STYLE_SOLID, NC_("fill-style", "Solid color"), NULL },
    { LIGMA_FILL_STYLE_PATTERN, NC_("fill-style", "Pattern"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaFillStyle", values);
      ligma_type_set_translation_context (type, "fill-style");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_filter_region_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_FILTER_REGION_SELECTION, "LIGMA_FILTER_REGION_SELECTION", "selection" },
    { LIGMA_FILTER_REGION_DRAWABLE, "LIGMA_FILTER_REGION_DRAWABLE", "drawable" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_FILTER_REGION_SELECTION, NC_("filter-region", "Use the selection as input"), NULL },
    { LIGMA_FILTER_REGION_DRAWABLE, NC_("filter-region", "Use the entire layer as input"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaFilterRegion", values);
      ligma_type_set_translation_context (type, "filter-region");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_gradient_color_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_GRADIENT_COLOR_FIXED, "LIGMA_GRADIENT_COLOR_FIXED", "fixed" },
    { LIGMA_GRADIENT_COLOR_FOREGROUND, "LIGMA_GRADIENT_COLOR_FOREGROUND", "foreground" },
    { LIGMA_GRADIENT_COLOR_FOREGROUND_TRANSPARENT, "LIGMA_GRADIENT_COLOR_FOREGROUND_TRANSPARENT", "foreground-transparent" },
    { LIGMA_GRADIENT_COLOR_BACKGROUND, "LIGMA_GRADIENT_COLOR_BACKGROUND", "background" },
    { LIGMA_GRADIENT_COLOR_BACKGROUND_TRANSPARENT, "LIGMA_GRADIENT_COLOR_BACKGROUND_TRANSPARENT", "background-transparent" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_GRADIENT_COLOR_FIXED, NC_("gradient-color", "Fixed"), NULL },
    { LIGMA_GRADIENT_COLOR_FOREGROUND, NC_("gradient-color", "Foreground color"), NULL },
    /* Translators: this is an abbreviated version of "Foreground color".
       Keep it short. */
    { LIGMA_GRADIENT_COLOR_FOREGROUND, NC_("gradient-color", "FG"), NULL },
    { LIGMA_GRADIENT_COLOR_FOREGROUND_TRANSPARENT, NC_("gradient-color", "Foreground color (transparent)"), NULL },
    /* Translators: this is an abbreviated version of "Foreground color (transparent)".
       Keep it short. */
    { LIGMA_GRADIENT_COLOR_FOREGROUND_TRANSPARENT, NC_("gradient-color", "FG (t)"), NULL },
    { LIGMA_GRADIENT_COLOR_BACKGROUND, NC_("gradient-color", "Background color"), NULL },
    /* Translators: this is an abbreviated version of "Background color".
       Keep it short. */
    { LIGMA_GRADIENT_COLOR_BACKGROUND, NC_("gradient-color", "BG"), NULL },
    { LIGMA_GRADIENT_COLOR_BACKGROUND_TRANSPARENT, NC_("gradient-color", "Background color (transparent)"), NULL },
    /* Translators: this is an abbreviated version of "Background color (transparent)".
       Keep it short. */
    { LIGMA_GRADIENT_COLOR_BACKGROUND_TRANSPARENT, NC_("gradient-color", "BG (t)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaGradientColor", values);
      ligma_type_set_translation_context (type, "gradient-color");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_gravity_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_GRAVITY_NONE, "LIGMA_GRAVITY_NONE", "none" },
    { LIGMA_GRAVITY_NORTH_WEST, "LIGMA_GRAVITY_NORTH_WEST", "north-west" },
    { LIGMA_GRAVITY_NORTH, "LIGMA_GRAVITY_NORTH", "north" },
    { LIGMA_GRAVITY_NORTH_EAST, "LIGMA_GRAVITY_NORTH_EAST", "north-east" },
    { LIGMA_GRAVITY_WEST, "LIGMA_GRAVITY_WEST", "west" },
    { LIGMA_GRAVITY_CENTER, "LIGMA_GRAVITY_CENTER", "center" },
    { LIGMA_GRAVITY_EAST, "LIGMA_GRAVITY_EAST", "east" },
    { LIGMA_GRAVITY_SOUTH_WEST, "LIGMA_GRAVITY_SOUTH_WEST", "south-west" },
    { LIGMA_GRAVITY_SOUTH, "LIGMA_GRAVITY_SOUTH", "south" },
    { LIGMA_GRAVITY_SOUTH_EAST, "LIGMA_GRAVITY_SOUTH_EAST", "south-east" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_GRAVITY_NONE, "LIGMA_GRAVITY_NONE", NULL },
    { LIGMA_GRAVITY_NORTH_WEST, "LIGMA_GRAVITY_NORTH_WEST", NULL },
    { LIGMA_GRAVITY_NORTH, "LIGMA_GRAVITY_NORTH", NULL },
    { LIGMA_GRAVITY_NORTH_EAST, "LIGMA_GRAVITY_NORTH_EAST", NULL },
    { LIGMA_GRAVITY_WEST, "LIGMA_GRAVITY_WEST", NULL },
    { LIGMA_GRAVITY_CENTER, "LIGMA_GRAVITY_CENTER", NULL },
    { LIGMA_GRAVITY_EAST, "LIGMA_GRAVITY_EAST", NULL },
    { LIGMA_GRAVITY_SOUTH_WEST, "LIGMA_GRAVITY_SOUTH_WEST", NULL },
    { LIGMA_GRAVITY_SOUTH, "LIGMA_GRAVITY_SOUTH", NULL },
    { LIGMA_GRAVITY_SOUTH_EAST, "LIGMA_GRAVITY_SOUTH_EAST", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaGravityType", values);
      ligma_type_set_translation_context (type, "gravity-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_guide_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_GUIDE_STYLE_NONE, "LIGMA_GUIDE_STYLE_NONE", "none" },
    { LIGMA_GUIDE_STYLE_NORMAL, "LIGMA_GUIDE_STYLE_NORMAL", "normal" },
    { LIGMA_GUIDE_STYLE_MIRROR, "LIGMA_GUIDE_STYLE_MIRROR", "mirror" },
    { LIGMA_GUIDE_STYLE_MANDALA, "LIGMA_GUIDE_STYLE_MANDALA", "mandala" },
    { LIGMA_GUIDE_STYLE_SPLIT_VIEW, "LIGMA_GUIDE_STYLE_SPLIT_VIEW", "split-view" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_GUIDE_STYLE_NONE, "LIGMA_GUIDE_STYLE_NONE", NULL },
    { LIGMA_GUIDE_STYLE_NORMAL, "LIGMA_GUIDE_STYLE_NORMAL", NULL },
    { LIGMA_GUIDE_STYLE_MIRROR, "LIGMA_GUIDE_STYLE_MIRROR", NULL },
    { LIGMA_GUIDE_STYLE_MANDALA, "LIGMA_GUIDE_STYLE_MANDALA", NULL },
    { LIGMA_GUIDE_STYLE_SPLIT_VIEW, "LIGMA_GUIDE_STYLE_SPLIT_VIEW", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaGuideStyle", values);
      ligma_type_set_translation_context (type, "guide-style");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_histogram_channel_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_HISTOGRAM_VALUE, "LIGMA_HISTOGRAM_VALUE", "value" },
    { LIGMA_HISTOGRAM_RED, "LIGMA_HISTOGRAM_RED", "red" },
    { LIGMA_HISTOGRAM_GREEN, "LIGMA_HISTOGRAM_GREEN", "green" },
    { LIGMA_HISTOGRAM_BLUE, "LIGMA_HISTOGRAM_BLUE", "blue" },
    { LIGMA_HISTOGRAM_ALPHA, "LIGMA_HISTOGRAM_ALPHA", "alpha" },
    { LIGMA_HISTOGRAM_LUMINANCE, "LIGMA_HISTOGRAM_LUMINANCE", "luminance" },
    { LIGMA_HISTOGRAM_RGB, "LIGMA_HISTOGRAM_RGB", "rgb" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_HISTOGRAM_VALUE, NC_("histogram-channel", "Value"), NULL },
    { LIGMA_HISTOGRAM_RED, NC_("histogram-channel", "Red"), NULL },
    { LIGMA_HISTOGRAM_GREEN, NC_("histogram-channel", "Green"), NULL },
    { LIGMA_HISTOGRAM_BLUE, NC_("histogram-channel", "Blue"), NULL },
    { LIGMA_HISTOGRAM_ALPHA, NC_("histogram-channel", "Alpha"), NULL },
    { LIGMA_HISTOGRAM_LUMINANCE, NC_("histogram-channel", "Luminance"), NULL },
    { LIGMA_HISTOGRAM_RGB, NC_("histogram-channel", "RGB"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaHistogramChannel", values);
      ligma_type_set_translation_context (type, "histogram-channel");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_item_set_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_ITEM_SET_NONE, "LIGMA_ITEM_SET_NONE", "none" },
    { LIGMA_ITEM_SET_ALL, "LIGMA_ITEM_SET_ALL", "all" },
    { LIGMA_ITEM_SET_IMAGE_SIZED, "LIGMA_ITEM_SET_IMAGE_SIZED", "image-sized" },
    { LIGMA_ITEM_SET_VISIBLE, "LIGMA_ITEM_SET_VISIBLE", "visible" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_ITEM_SET_NONE, NC_("item-set", "None"), NULL },
    { LIGMA_ITEM_SET_ALL, NC_("item-set", "All layers"), NULL },
    { LIGMA_ITEM_SET_IMAGE_SIZED, NC_("item-set", "Image-sized layers"), NULL },
    { LIGMA_ITEM_SET_VISIBLE, NC_("item-set", "All visible layers"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaItemSet", values);
      ligma_type_set_translation_context (type, "item-set");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_matting_engine_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_MATTING_ENGINE_GLOBAL, "LIGMA_MATTING_ENGINE_GLOBAL", "global" },
    { LIGMA_MATTING_ENGINE_LEVIN, "LIGMA_MATTING_ENGINE_LEVIN", "levin" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_MATTING_ENGINE_GLOBAL, NC_("matting-engine", "Matting Global"), NULL },
    { LIGMA_MATTING_ENGINE_LEVIN, NC_("matting-engine", "Matting Levin"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaMattingEngine", values);
      ligma_type_set_translation_context (type, "matting-engine");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_message_severity_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_MESSAGE_INFO, "LIGMA_MESSAGE_INFO", "info" },
    { LIGMA_MESSAGE_WARNING, "LIGMA_MESSAGE_WARNING", "warning" },
    { LIGMA_MESSAGE_ERROR, "LIGMA_MESSAGE_ERROR", "error" },
    { LIGMA_MESSAGE_BUG_WARNING, "LIGMA_MESSAGE_BUG_WARNING", "bug-warning" },
    { LIGMA_MESSAGE_BUG_CRITICAL, "LIGMA_MESSAGE_BUG_CRITICAL", "bug-critical" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_MESSAGE_INFO, NC_("message-severity", "Message"), NULL },
    { LIGMA_MESSAGE_WARNING, NC_("message-severity", "Warning"), NULL },
    { LIGMA_MESSAGE_ERROR, NC_("message-severity", "Error"), NULL },
    { LIGMA_MESSAGE_BUG_WARNING, NC_("message-severity", "WARNING"), NULL },
    { LIGMA_MESSAGE_BUG_CRITICAL, NC_("message-severity", "CRITICAL"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaMessageSeverity", values);
      ligma_type_set_translation_context (type, "message-severity");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_metadata_rotation_policy_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_METADATA_ROTATION_POLICY_ASK, "LIGMA_METADATA_ROTATION_POLICY_ASK", "ask" },
    { LIGMA_METADATA_ROTATION_POLICY_KEEP, "LIGMA_METADATA_ROTATION_POLICY_KEEP", "keep" },
    { LIGMA_METADATA_ROTATION_POLICY_ROTATE, "LIGMA_METADATA_ROTATION_POLICY_ROTATE", "rotate" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_METADATA_ROTATION_POLICY_ASK, NC_("metadata-rotation-policy", "Ask what to do"), NULL },
    { LIGMA_METADATA_ROTATION_POLICY_KEEP, NC_("metadata-rotation-policy", "Discard metadata without rotating"), NULL },
    { LIGMA_METADATA_ROTATION_POLICY_ROTATE, NC_("metadata-rotation-policy", "Rotate the image then discard metadata"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaMetadataRotationPolicy", values);
      ligma_type_set_translation_context (type, "metadata-rotation-policy");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_paste_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_PASTE_TYPE_FLOATING, "LIGMA_PASTE_TYPE_FLOATING", "floating" },
    { LIGMA_PASTE_TYPE_FLOATING_IN_PLACE, "LIGMA_PASTE_TYPE_FLOATING_IN_PLACE", "floating-in-place" },
    { LIGMA_PASTE_TYPE_FLOATING_INTO, "LIGMA_PASTE_TYPE_FLOATING_INTO", "floating-into" },
    { LIGMA_PASTE_TYPE_FLOATING_INTO_IN_PLACE, "LIGMA_PASTE_TYPE_FLOATING_INTO_IN_PLACE", "floating-into-in-place" },
    { LIGMA_PASTE_TYPE_NEW_LAYER, "LIGMA_PASTE_TYPE_NEW_LAYER", "new-layer" },
    { LIGMA_PASTE_TYPE_NEW_LAYER_IN_PLACE, "LIGMA_PASTE_TYPE_NEW_LAYER_IN_PLACE", "new-layer-in-place" },
    { LIGMA_PASTE_TYPE_NEW_LAYER_OR_FLOATING, "LIGMA_PASTE_TYPE_NEW_LAYER_OR_FLOATING", "new-layer-or-floating" },
    { LIGMA_PASTE_TYPE_NEW_LAYER_OR_FLOATING_IN_PLACE, "LIGMA_PASTE_TYPE_NEW_LAYER_OR_FLOATING_IN_PLACE", "new-layer-or-floating-in-place" },
    { LIGMA_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING, "LIGMA_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING", "new-merged-layer-or-floating" },
    { LIGMA_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING_IN_PLACE, "LIGMA_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING_IN_PLACE", "new-merged-layer-or-floating-in-place" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_PASTE_TYPE_FLOATING, "LIGMA_PASTE_TYPE_FLOATING", NULL },
    { LIGMA_PASTE_TYPE_FLOATING_IN_PLACE, "LIGMA_PASTE_TYPE_FLOATING_IN_PLACE", NULL },
    { LIGMA_PASTE_TYPE_FLOATING_INTO, "LIGMA_PASTE_TYPE_FLOATING_INTO", NULL },
    { LIGMA_PASTE_TYPE_FLOATING_INTO_IN_PLACE, "LIGMA_PASTE_TYPE_FLOATING_INTO_IN_PLACE", NULL },
    { LIGMA_PASTE_TYPE_NEW_LAYER, "LIGMA_PASTE_TYPE_NEW_LAYER", NULL },
    { LIGMA_PASTE_TYPE_NEW_LAYER_IN_PLACE, "LIGMA_PASTE_TYPE_NEW_LAYER_IN_PLACE", NULL },
    { LIGMA_PASTE_TYPE_NEW_LAYER_OR_FLOATING, "LIGMA_PASTE_TYPE_NEW_LAYER_OR_FLOATING", NULL },
    { LIGMA_PASTE_TYPE_NEW_LAYER_OR_FLOATING_IN_PLACE, "LIGMA_PASTE_TYPE_NEW_LAYER_OR_FLOATING_IN_PLACE", NULL },
    { LIGMA_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING, "LIGMA_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING", NULL },
    { LIGMA_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING_IN_PLACE, "LIGMA_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING_IN_PLACE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaPasteType", values);
      ligma_type_set_translation_context (type, "paste-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_win32_pointer_input_api_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_WIN32_POINTER_INPUT_API_WINTAB, "LIGMA_WIN32_POINTER_INPUT_API_WINTAB", "wintab" },
    { LIGMA_WIN32_POINTER_INPUT_API_WINDOWS_INK, "LIGMA_WIN32_POINTER_INPUT_API_WINDOWS_INK", "windows-ink" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_WIN32_POINTER_INPUT_API_WINTAB, NC_("win32-pointer-input-api", "Wintab"), NULL },
    { LIGMA_WIN32_POINTER_INPUT_API_WINDOWS_INK, NC_("win32-pointer-input-api", "Windows Ink"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaWin32PointerInputAPI", values);
      ligma_type_set_translation_context (type, "win32-pointer-input-api");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_thumbnail_size_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_THUMBNAIL_SIZE_NONE, "LIGMA_THUMBNAIL_SIZE_NONE", "none" },
    { LIGMA_THUMBNAIL_SIZE_NORMAL, "LIGMA_THUMBNAIL_SIZE_NORMAL", "normal" },
    { LIGMA_THUMBNAIL_SIZE_LARGE, "LIGMA_THUMBNAIL_SIZE_LARGE", "large" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_THUMBNAIL_SIZE_NONE, NC_("thumbnail-size", "No thumbnails"), NULL },
    { LIGMA_THUMBNAIL_SIZE_NORMAL, NC_("thumbnail-size", "Normal (128x128)"), NULL },
    { LIGMA_THUMBNAIL_SIZE_LARGE, NC_("thumbnail-size", "Large (256x256)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaThumbnailSize", values);
      ligma_type_set_translation_context (type, "thumbnail-size");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_trc_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_TRC_LINEAR, "LIGMA_TRC_LINEAR", "linear" },
    { LIGMA_TRC_NON_LINEAR, "LIGMA_TRC_NON_LINEAR", "non-linear" },
    { LIGMA_TRC_PERCEPTUAL, "LIGMA_TRC_PERCEPTUAL", "perceptual" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_TRC_LINEAR, NC_("trc-type", "Linear"), NULL },
    { LIGMA_TRC_NON_LINEAR, NC_("trc-type", "Non-Linear"), NULL },
    { LIGMA_TRC_PERCEPTUAL, NC_("trc-type", "Perceptual"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaTRCType", values);
      ligma_type_set_translation_context (type, "trc-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_undo_event_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_UNDO_EVENT_UNDO_PUSHED, "LIGMA_UNDO_EVENT_UNDO_PUSHED", "undo-pushed" },
    { LIGMA_UNDO_EVENT_UNDO_EXPIRED, "LIGMA_UNDO_EVENT_UNDO_EXPIRED", "undo-expired" },
    { LIGMA_UNDO_EVENT_REDO_EXPIRED, "LIGMA_UNDO_EVENT_REDO_EXPIRED", "redo-expired" },
    { LIGMA_UNDO_EVENT_UNDO, "LIGMA_UNDO_EVENT_UNDO", "undo" },
    { LIGMA_UNDO_EVENT_REDO, "LIGMA_UNDO_EVENT_REDO", "redo" },
    { LIGMA_UNDO_EVENT_UNDO_FREE, "LIGMA_UNDO_EVENT_UNDO_FREE", "undo-free" },
    { LIGMA_UNDO_EVENT_UNDO_FREEZE, "LIGMA_UNDO_EVENT_UNDO_FREEZE", "undo-freeze" },
    { LIGMA_UNDO_EVENT_UNDO_THAW, "LIGMA_UNDO_EVENT_UNDO_THAW", "undo-thaw" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_UNDO_EVENT_UNDO_PUSHED, "LIGMA_UNDO_EVENT_UNDO_PUSHED", NULL },
    { LIGMA_UNDO_EVENT_UNDO_EXPIRED, "LIGMA_UNDO_EVENT_UNDO_EXPIRED", NULL },
    { LIGMA_UNDO_EVENT_REDO_EXPIRED, "LIGMA_UNDO_EVENT_REDO_EXPIRED", NULL },
    { LIGMA_UNDO_EVENT_UNDO, "LIGMA_UNDO_EVENT_UNDO", NULL },
    { LIGMA_UNDO_EVENT_REDO, "LIGMA_UNDO_EVENT_REDO", NULL },
    { LIGMA_UNDO_EVENT_UNDO_FREE, "LIGMA_UNDO_EVENT_UNDO_FREE", NULL },
    { LIGMA_UNDO_EVENT_UNDO_FREEZE, "LIGMA_UNDO_EVENT_UNDO_FREEZE", NULL },
    { LIGMA_UNDO_EVENT_UNDO_THAW, "LIGMA_UNDO_EVENT_UNDO_THAW", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaUndoEvent", values);
      ligma_type_set_translation_context (type, "undo-event");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_undo_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_UNDO_MODE_UNDO, "LIGMA_UNDO_MODE_UNDO", "undo" },
    { LIGMA_UNDO_MODE_REDO, "LIGMA_UNDO_MODE_REDO", "redo" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_UNDO_MODE_UNDO, "LIGMA_UNDO_MODE_UNDO", NULL },
    { LIGMA_UNDO_MODE_REDO, "LIGMA_UNDO_MODE_REDO", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaUndoMode", values);
      ligma_type_set_translation_context (type, "undo-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_undo_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_UNDO_GROUP_NONE, "LIGMA_UNDO_GROUP_NONE", "group-none" },
    { LIGMA_UNDO_GROUP_IMAGE_SCALE, "LIGMA_UNDO_GROUP_IMAGE_SCALE", "group-image-scale" },
    { LIGMA_UNDO_GROUP_IMAGE_RESIZE, "LIGMA_UNDO_GROUP_IMAGE_RESIZE", "group-image-resize" },
    { LIGMA_UNDO_GROUP_IMAGE_FLIP, "LIGMA_UNDO_GROUP_IMAGE_FLIP", "group-image-flip" },
    { LIGMA_UNDO_GROUP_IMAGE_ROTATE, "LIGMA_UNDO_GROUP_IMAGE_ROTATE", "group-image-rotate" },
    { LIGMA_UNDO_GROUP_IMAGE_TRANSFORM, "LIGMA_UNDO_GROUP_IMAGE_TRANSFORM", "group-image-transform" },
    { LIGMA_UNDO_GROUP_IMAGE_CROP, "LIGMA_UNDO_GROUP_IMAGE_CROP", "group-image-crop" },
    { LIGMA_UNDO_GROUP_IMAGE_CONVERT, "LIGMA_UNDO_GROUP_IMAGE_CONVERT", "group-image-convert" },
    { LIGMA_UNDO_GROUP_IMAGE_ITEM_REMOVE, "LIGMA_UNDO_GROUP_IMAGE_ITEM_REMOVE", "group-image-item-remove" },
    { LIGMA_UNDO_GROUP_IMAGE_ITEM_REORDER, "LIGMA_UNDO_GROUP_IMAGE_ITEM_REORDER", "group-image-item-reorder" },
    { LIGMA_UNDO_GROUP_IMAGE_LAYERS_MERGE, "LIGMA_UNDO_GROUP_IMAGE_LAYERS_MERGE", "group-image-layers-merge" },
    { LIGMA_UNDO_GROUP_IMAGE_VECTORS_MERGE, "LIGMA_UNDO_GROUP_IMAGE_VECTORS_MERGE", "group-image-vectors-merge" },
    { LIGMA_UNDO_GROUP_IMAGE_QUICK_MASK, "LIGMA_UNDO_GROUP_IMAGE_QUICK_MASK", "group-image-quick-mask" },
    { LIGMA_UNDO_GROUP_IMAGE_GRID, "LIGMA_UNDO_GROUP_IMAGE_GRID", "group-image-grid" },
    { LIGMA_UNDO_GROUP_GUIDE, "LIGMA_UNDO_GROUP_GUIDE", "group-guide" },
    { LIGMA_UNDO_GROUP_SAMPLE_POINT, "LIGMA_UNDO_GROUP_SAMPLE_POINT", "group-sample-point" },
    { LIGMA_UNDO_GROUP_DRAWABLE, "LIGMA_UNDO_GROUP_DRAWABLE", "group-drawable" },
    { LIGMA_UNDO_GROUP_DRAWABLE_MOD, "LIGMA_UNDO_GROUP_DRAWABLE_MOD", "group-drawable-mod" },
    { LIGMA_UNDO_GROUP_MASK, "LIGMA_UNDO_GROUP_MASK", "group-mask" },
    { LIGMA_UNDO_GROUP_ITEM_VISIBILITY, "LIGMA_UNDO_GROUP_ITEM_VISIBILITY", "group-item-visibility" },
    { LIGMA_UNDO_GROUP_ITEM_LOCK_CONTENTS, "LIGMA_UNDO_GROUP_ITEM_LOCK_CONTENTS", "group-item-lock-contents" },
    { LIGMA_UNDO_GROUP_ITEM_LOCK_POSITION, "LIGMA_UNDO_GROUP_ITEM_LOCK_POSITION", "group-item-lock-position" },
    { LIGMA_UNDO_GROUP_ITEM_LOCK_VISIBILITY, "LIGMA_UNDO_GROUP_ITEM_LOCK_VISIBILITY", "group-item-lock-visibility" },
    { LIGMA_UNDO_GROUP_ITEM_PROPERTIES, "LIGMA_UNDO_GROUP_ITEM_PROPERTIES", "group-item-properties" },
    { LIGMA_UNDO_GROUP_ITEM_DISPLACE, "LIGMA_UNDO_GROUP_ITEM_DISPLACE", "group-item-displace" },
    { LIGMA_UNDO_GROUP_ITEM_SCALE, "LIGMA_UNDO_GROUP_ITEM_SCALE", "group-item-scale" },
    { LIGMA_UNDO_GROUP_ITEM_RESIZE, "LIGMA_UNDO_GROUP_ITEM_RESIZE", "group-item-resize" },
    { LIGMA_UNDO_GROUP_LAYER_ADD, "LIGMA_UNDO_GROUP_LAYER_ADD", "group-layer-add" },
    { LIGMA_UNDO_GROUP_LAYER_ADD_ALPHA, "LIGMA_UNDO_GROUP_LAYER_ADD_ALPHA", "group-layer-add-alpha" },
    { LIGMA_UNDO_GROUP_LAYER_ADD_MASK, "LIGMA_UNDO_GROUP_LAYER_ADD_MASK", "group-layer-add-mask" },
    { LIGMA_UNDO_GROUP_LAYER_APPLY_MASK, "LIGMA_UNDO_GROUP_LAYER_APPLY_MASK", "group-layer-apply-mask" },
    { LIGMA_UNDO_GROUP_LAYER_REMOVE_ALPHA, "LIGMA_UNDO_GROUP_LAYER_REMOVE_ALPHA", "group-layer-remove-alpha" },
    { LIGMA_UNDO_GROUP_LAYER_LOCK_ALPHA, "LIGMA_UNDO_GROUP_LAYER_LOCK_ALPHA", "group-layer-lock-alpha" },
    { LIGMA_UNDO_GROUP_LAYER_OPACITY, "LIGMA_UNDO_GROUP_LAYER_OPACITY", "group-layer-opacity" },
    { LIGMA_UNDO_GROUP_LAYER_MODE, "LIGMA_UNDO_GROUP_LAYER_MODE", "group-layer-mode" },
    { LIGMA_UNDO_GROUP_CHANNEL_ADD, "LIGMA_UNDO_GROUP_CHANNEL_ADD", "group-channel-add" },
    { LIGMA_UNDO_GROUP_FS_TO_LAYER, "LIGMA_UNDO_GROUP_FS_TO_LAYER", "group-fs-to-layer" },
    { LIGMA_UNDO_GROUP_FS_FLOAT, "LIGMA_UNDO_GROUP_FS_FLOAT", "group-fs-float" },
    { LIGMA_UNDO_GROUP_FS_ANCHOR, "LIGMA_UNDO_GROUP_FS_ANCHOR", "group-fs-anchor" },
    { LIGMA_UNDO_GROUP_EDIT_PASTE, "LIGMA_UNDO_GROUP_EDIT_PASTE", "group-edit-paste" },
    { LIGMA_UNDO_GROUP_EDIT_CUT, "LIGMA_UNDO_GROUP_EDIT_CUT", "group-edit-cut" },
    { LIGMA_UNDO_GROUP_TEXT, "LIGMA_UNDO_GROUP_TEXT", "group-text" },
    { LIGMA_UNDO_GROUP_TRANSFORM, "LIGMA_UNDO_GROUP_TRANSFORM", "group-transform" },
    { LIGMA_UNDO_GROUP_PAINT, "LIGMA_UNDO_GROUP_PAINT", "group-paint" },
    { LIGMA_UNDO_GROUP_PARASITE_ATTACH, "LIGMA_UNDO_GROUP_PARASITE_ATTACH", "group-parasite-attach" },
    { LIGMA_UNDO_GROUP_PARASITE_REMOVE, "LIGMA_UNDO_GROUP_PARASITE_REMOVE", "group-parasite-remove" },
    { LIGMA_UNDO_GROUP_VECTORS_IMPORT, "LIGMA_UNDO_GROUP_VECTORS_IMPORT", "group-vectors-import" },
    { LIGMA_UNDO_GROUP_MISC, "LIGMA_UNDO_GROUP_MISC", "group-misc" },
    { LIGMA_UNDO_IMAGE_TYPE, "LIGMA_UNDO_IMAGE_TYPE", "image-type" },
    { LIGMA_UNDO_IMAGE_PRECISION, "LIGMA_UNDO_IMAGE_PRECISION", "image-precision" },
    { LIGMA_UNDO_IMAGE_SIZE, "LIGMA_UNDO_IMAGE_SIZE", "image-size" },
    { LIGMA_UNDO_IMAGE_RESOLUTION, "LIGMA_UNDO_IMAGE_RESOLUTION", "image-resolution" },
    { LIGMA_UNDO_IMAGE_GRID, "LIGMA_UNDO_IMAGE_GRID", "image-grid" },
    { LIGMA_UNDO_IMAGE_METADATA, "LIGMA_UNDO_IMAGE_METADATA", "image-metadata" },
    { LIGMA_UNDO_IMAGE_COLORMAP, "LIGMA_UNDO_IMAGE_COLORMAP", "image-colormap" },
    { LIGMA_UNDO_IMAGE_HIDDEN_PROFILE, "LIGMA_UNDO_IMAGE_HIDDEN_PROFILE", "image-hidden-profile" },
    { LIGMA_UNDO_GUIDE, "LIGMA_UNDO_GUIDE", "guide" },
    { LIGMA_UNDO_SAMPLE_POINT, "LIGMA_UNDO_SAMPLE_POINT", "sample-point" },
    { LIGMA_UNDO_DRAWABLE, "LIGMA_UNDO_DRAWABLE", "drawable" },
    { LIGMA_UNDO_DRAWABLE_MOD, "LIGMA_UNDO_DRAWABLE_MOD", "drawable-mod" },
    { LIGMA_UNDO_DRAWABLE_FORMAT, "LIGMA_UNDO_DRAWABLE_FORMAT", "drawable-format" },
    { LIGMA_UNDO_MASK, "LIGMA_UNDO_MASK", "mask" },
    { LIGMA_UNDO_ITEM_REORDER, "LIGMA_UNDO_ITEM_REORDER", "item-reorder" },
    { LIGMA_UNDO_ITEM_RENAME, "LIGMA_UNDO_ITEM_RENAME", "item-rename" },
    { LIGMA_UNDO_ITEM_DISPLACE, "LIGMA_UNDO_ITEM_DISPLACE", "item-displace" },
    { LIGMA_UNDO_ITEM_VISIBILITY, "LIGMA_UNDO_ITEM_VISIBILITY", "item-visibility" },
    { LIGMA_UNDO_ITEM_COLOR_TAG, "LIGMA_UNDO_ITEM_COLOR_TAG", "item-color-tag" },
    { LIGMA_UNDO_ITEM_LOCK_CONTENT, "LIGMA_UNDO_ITEM_LOCK_CONTENT", "item-lock-content" },
    { LIGMA_UNDO_ITEM_LOCK_POSITION, "LIGMA_UNDO_ITEM_LOCK_POSITION", "item-lock-position" },
    { LIGMA_UNDO_ITEM_LOCK_VISIBILITY, "LIGMA_UNDO_ITEM_LOCK_VISIBILITY", "item-lock-visibility" },
    { LIGMA_UNDO_LAYER_ADD, "LIGMA_UNDO_LAYER_ADD", "layer-add" },
    { LIGMA_UNDO_LAYER_REMOVE, "LIGMA_UNDO_LAYER_REMOVE", "layer-remove" },
    { LIGMA_UNDO_LAYER_MODE, "LIGMA_UNDO_LAYER_MODE", "layer-mode" },
    { LIGMA_UNDO_LAYER_OPACITY, "LIGMA_UNDO_LAYER_OPACITY", "layer-opacity" },
    { LIGMA_UNDO_LAYER_LOCK_ALPHA, "LIGMA_UNDO_LAYER_LOCK_ALPHA", "layer-lock-alpha" },
    { LIGMA_UNDO_GROUP_LAYER_SUSPEND_RESIZE, "LIGMA_UNDO_GROUP_LAYER_SUSPEND_RESIZE", "group-layer-suspend-resize" },
    { LIGMA_UNDO_GROUP_LAYER_RESUME_RESIZE, "LIGMA_UNDO_GROUP_LAYER_RESUME_RESIZE", "group-layer-resume-resize" },
    { LIGMA_UNDO_GROUP_LAYER_SUSPEND_MASK, "LIGMA_UNDO_GROUP_LAYER_SUSPEND_MASK", "group-layer-suspend-mask" },
    { LIGMA_UNDO_GROUP_LAYER_RESUME_MASK, "LIGMA_UNDO_GROUP_LAYER_RESUME_MASK", "group-layer-resume-mask" },
    { LIGMA_UNDO_GROUP_LAYER_START_TRANSFORM, "LIGMA_UNDO_GROUP_LAYER_START_TRANSFORM", "group-layer-start-transform" },
    { LIGMA_UNDO_GROUP_LAYER_END_TRANSFORM, "LIGMA_UNDO_GROUP_LAYER_END_TRANSFORM", "group-layer-end-transform" },
    { LIGMA_UNDO_GROUP_LAYER_CONVERT, "LIGMA_UNDO_GROUP_LAYER_CONVERT", "group-layer-convert" },
    { LIGMA_UNDO_TEXT_LAYER, "LIGMA_UNDO_TEXT_LAYER", "text-layer" },
    { LIGMA_UNDO_TEXT_LAYER_MODIFIED, "LIGMA_UNDO_TEXT_LAYER_MODIFIED", "text-layer-modified" },
    { LIGMA_UNDO_TEXT_LAYER_CONVERT, "LIGMA_UNDO_TEXT_LAYER_CONVERT", "text-layer-convert" },
    { LIGMA_UNDO_LAYER_MASK_ADD, "LIGMA_UNDO_LAYER_MASK_ADD", "layer-mask-add" },
    { LIGMA_UNDO_LAYER_MASK_REMOVE, "LIGMA_UNDO_LAYER_MASK_REMOVE", "layer-mask-remove" },
    { LIGMA_UNDO_LAYER_MASK_APPLY, "LIGMA_UNDO_LAYER_MASK_APPLY", "layer-mask-apply" },
    { LIGMA_UNDO_LAYER_MASK_SHOW, "LIGMA_UNDO_LAYER_MASK_SHOW", "layer-mask-show" },
    { LIGMA_UNDO_CHANNEL_ADD, "LIGMA_UNDO_CHANNEL_ADD", "channel-add" },
    { LIGMA_UNDO_CHANNEL_REMOVE, "LIGMA_UNDO_CHANNEL_REMOVE", "channel-remove" },
    { LIGMA_UNDO_CHANNEL_COLOR, "LIGMA_UNDO_CHANNEL_COLOR", "channel-color" },
    { LIGMA_UNDO_VECTORS_ADD, "LIGMA_UNDO_VECTORS_ADD", "vectors-add" },
    { LIGMA_UNDO_VECTORS_REMOVE, "LIGMA_UNDO_VECTORS_REMOVE", "vectors-remove" },
    { LIGMA_UNDO_VECTORS_MOD, "LIGMA_UNDO_VECTORS_MOD", "vectors-mod" },
    { LIGMA_UNDO_FS_TO_LAYER, "LIGMA_UNDO_FS_TO_LAYER", "fs-to-layer" },
    { LIGMA_UNDO_TRANSFORM_GRID, "LIGMA_UNDO_TRANSFORM_GRID", "transform-grid" },
    { LIGMA_UNDO_PAINT, "LIGMA_UNDO_PAINT", "paint" },
    { LIGMA_UNDO_INK, "LIGMA_UNDO_INK", "ink" },
    { LIGMA_UNDO_FOREGROUND_SELECT, "LIGMA_UNDO_FOREGROUND_SELECT", "foreground-select" },
    { LIGMA_UNDO_PARASITE_ATTACH, "LIGMA_UNDO_PARASITE_ATTACH", "parasite-attach" },
    { LIGMA_UNDO_PARASITE_REMOVE, "LIGMA_UNDO_PARASITE_REMOVE", "parasite-remove" },
    { LIGMA_UNDO_CANT, "LIGMA_UNDO_CANT", "cant" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_UNDO_GROUP_NONE, NC_("undo-type", "<<invalid>>"), NULL },
    { LIGMA_UNDO_GROUP_IMAGE_SCALE, NC_("undo-type", "Scale image"), NULL },
    { LIGMA_UNDO_GROUP_IMAGE_RESIZE, NC_("undo-type", "Resize image"), NULL },
    { LIGMA_UNDO_GROUP_IMAGE_FLIP, NC_("undo-type", "Flip image"), NULL },
    { LIGMA_UNDO_GROUP_IMAGE_ROTATE, NC_("undo-type", "Rotate image"), NULL },
    { LIGMA_UNDO_GROUP_IMAGE_TRANSFORM, NC_("undo-type", "Transform image"), NULL },
    { LIGMA_UNDO_GROUP_IMAGE_CROP, NC_("undo-type", "Crop image"), NULL },
    { LIGMA_UNDO_GROUP_IMAGE_CONVERT, NC_("undo-type", "Convert image"), NULL },
    { LIGMA_UNDO_GROUP_IMAGE_ITEM_REMOVE, NC_("undo-type", "Remove item"), NULL },
    { LIGMA_UNDO_GROUP_IMAGE_ITEM_REORDER, NC_("undo-type", "Reorder item"), NULL },
    { LIGMA_UNDO_GROUP_IMAGE_LAYERS_MERGE, NC_("undo-type", "Merge layers"), NULL },
    { LIGMA_UNDO_GROUP_IMAGE_VECTORS_MERGE, NC_("undo-type", "Merge paths"), NULL },
    { LIGMA_UNDO_GROUP_IMAGE_QUICK_MASK, NC_("undo-type", "Quick Mask"), NULL },
    { LIGMA_UNDO_GROUP_IMAGE_GRID, NC_("undo-type", "Grid"), NULL },
    { LIGMA_UNDO_GROUP_GUIDE, NC_("undo-type", "Guide"), NULL },
    { LIGMA_UNDO_GROUP_SAMPLE_POINT, NC_("undo-type", "Sample Point"), NULL },
    { LIGMA_UNDO_GROUP_DRAWABLE, NC_("undo-type", "Layer/Channel"), NULL },
    { LIGMA_UNDO_GROUP_DRAWABLE_MOD, NC_("undo-type", "Layer/Channel modification"), NULL },
    { LIGMA_UNDO_GROUP_MASK, NC_("undo-type", "Selection mask"), NULL },
    { LIGMA_UNDO_GROUP_ITEM_VISIBILITY, NC_("undo-type", "Item visibility"), NULL },
    { LIGMA_UNDO_GROUP_ITEM_LOCK_CONTENTS, NC_("undo-type", "Lock/Unlock contents"), NULL },
    { LIGMA_UNDO_GROUP_ITEM_LOCK_POSITION, NC_("undo-type", "Lock/Unlock position"), NULL },
    { LIGMA_UNDO_GROUP_ITEM_LOCK_VISIBILITY, NC_("undo-type", "Lock/Unlock visibility"), NULL },
    { LIGMA_UNDO_GROUP_ITEM_PROPERTIES, NC_("undo-type", "Item properties"), NULL },
    { LIGMA_UNDO_GROUP_ITEM_DISPLACE, NC_("undo-type", "Move item"), NULL },
    { LIGMA_UNDO_GROUP_ITEM_SCALE, NC_("undo-type", "Scale item"), NULL },
    { LIGMA_UNDO_GROUP_ITEM_RESIZE, NC_("undo-type", "Resize item"), NULL },
    { LIGMA_UNDO_GROUP_LAYER_ADD, NC_("undo-type", "Add layer"), NULL },
    { LIGMA_UNDO_GROUP_LAYER_ADD_ALPHA, NC_("undo-type", "Add alpha channel"), NULL },
    { LIGMA_UNDO_GROUP_LAYER_ADD_MASK, NC_("undo-type", "Add layer mask"), NULL },
    { LIGMA_UNDO_GROUP_LAYER_APPLY_MASK, NC_("undo-type", "Apply layer mask"), NULL },
    { LIGMA_UNDO_GROUP_LAYER_REMOVE_ALPHA, NC_("undo-type", "Remove alpha channel"), NULL },
    { LIGMA_UNDO_GROUP_LAYER_LOCK_ALPHA, NC_("undo-type", "Lock/Unlock alpha channels"), NULL },
    { LIGMA_UNDO_GROUP_LAYER_OPACITY, NC_("undo-type", "Set layers opacity"), NULL },
    { LIGMA_UNDO_GROUP_LAYER_MODE, NC_("undo-type", "Set layers mode"), NULL },
    { LIGMA_UNDO_GROUP_CHANNEL_ADD, NC_("undo-type", "Add channels"), NULL },
    { LIGMA_UNDO_GROUP_FS_TO_LAYER, NC_("undo-type", "Floating selection to layer"), NULL },
    { LIGMA_UNDO_GROUP_FS_FLOAT, NC_("undo-type", "Float selection"), NULL },
    { LIGMA_UNDO_GROUP_FS_ANCHOR, NC_("undo-type", "Anchor floating selection"), NULL },
    { LIGMA_UNDO_GROUP_EDIT_PASTE, NC_("undo-type", "Paste"), NULL },
    { LIGMA_UNDO_GROUP_EDIT_CUT, NC_("undo-type", "Cut"), NULL },
    { LIGMA_UNDO_GROUP_TEXT, NC_("undo-type", "Text"), NULL },
    { LIGMA_UNDO_GROUP_TRANSFORM, NC_("undo-type", "Transform"), NULL },
    { LIGMA_UNDO_GROUP_PAINT, NC_("undo-type", "Paint"), NULL },
    { LIGMA_UNDO_GROUP_PARASITE_ATTACH, NC_("undo-type", "Attach parasite"), NULL },
    { LIGMA_UNDO_GROUP_PARASITE_REMOVE, NC_("undo-type", "Remove parasite"), NULL },
    { LIGMA_UNDO_GROUP_VECTORS_IMPORT, NC_("undo-type", "Import paths"), NULL },
    { LIGMA_UNDO_GROUP_MISC, NC_("undo-type", "Plug-In"), NULL },
    { LIGMA_UNDO_IMAGE_TYPE, NC_("undo-type", "Image type"), NULL },
    { LIGMA_UNDO_IMAGE_PRECISION, NC_("undo-type", "Image precision"), NULL },
    { LIGMA_UNDO_IMAGE_SIZE, NC_("undo-type", "Image size"), NULL },
    { LIGMA_UNDO_IMAGE_RESOLUTION, NC_("undo-type", "Image resolution change"), NULL },
    { LIGMA_UNDO_IMAGE_GRID, NC_("undo-type", "Grid"), NULL },
    { LIGMA_UNDO_IMAGE_METADATA, NC_("undo-type", "Change metadata"), NULL },
    { LIGMA_UNDO_IMAGE_COLORMAP, NC_("undo-type", "Change indexed palette"), NULL },
    { LIGMA_UNDO_IMAGE_HIDDEN_PROFILE, NC_("undo-type", "Hide/Unhide color profile"), NULL },
    { LIGMA_UNDO_GUIDE, NC_("undo-type", "Guide"), NULL },
    { LIGMA_UNDO_SAMPLE_POINT, NC_("undo-type", "Sample Point"), NULL },
    { LIGMA_UNDO_DRAWABLE, NC_("undo-type", "Layer/Channel"), NULL },
    { LIGMA_UNDO_DRAWABLE_MOD, NC_("undo-type", "Layer/Channel modification"), NULL },
    { LIGMA_UNDO_DRAWABLE_FORMAT, NC_("undo-type", "Layer/Channel format"), NULL },
    { LIGMA_UNDO_MASK, NC_("undo-type", "Selection mask"), NULL },
    { LIGMA_UNDO_ITEM_REORDER, NC_("undo-type", "Reorder item"), NULL },
    { LIGMA_UNDO_ITEM_RENAME, NC_("undo-type", "Rename item"), NULL },
    { LIGMA_UNDO_ITEM_DISPLACE, NC_("undo-type", "Move item"), NULL },
    { LIGMA_UNDO_ITEM_VISIBILITY, NC_("undo-type", "Item visibility"), NULL },
    { LIGMA_UNDO_ITEM_COLOR_TAG, NC_("undo-type", "Item color tag"), NULL },
    { LIGMA_UNDO_ITEM_LOCK_CONTENT, NC_("undo-type", "Lock/Unlock content"), NULL },
    { LIGMA_UNDO_ITEM_LOCK_POSITION, NC_("undo-type", "Lock/Unlock position"), NULL },
    { LIGMA_UNDO_ITEM_LOCK_VISIBILITY, NC_("undo-type", "Lock/Unlock visibility"), NULL },
    { LIGMA_UNDO_LAYER_ADD, NC_("undo-type", "New layer"), NULL },
    { LIGMA_UNDO_LAYER_REMOVE, NC_("undo-type", "Delete layer"), NULL },
    { LIGMA_UNDO_LAYER_MODE, NC_("undo-type", "Set layer mode"), NULL },
    { LIGMA_UNDO_LAYER_OPACITY, NC_("undo-type", "Set layer opacity"), NULL },
    { LIGMA_UNDO_LAYER_LOCK_ALPHA, NC_("undo-type", "Lock/Unlock alpha channel"), NULL },
    { LIGMA_UNDO_GROUP_LAYER_SUSPEND_RESIZE, NC_("undo-type", "Suspend group layer resize"), NULL },
    { LIGMA_UNDO_GROUP_LAYER_RESUME_RESIZE, NC_("undo-type", "Resume group layer resize"), NULL },
    { LIGMA_UNDO_GROUP_LAYER_SUSPEND_MASK, NC_("undo-type", "Suspend group layer mask"), NULL },
    { LIGMA_UNDO_GROUP_LAYER_RESUME_MASK, NC_("undo-type", "Resume group layer mask"), NULL },
    { LIGMA_UNDO_GROUP_LAYER_START_TRANSFORM, NC_("undo-type", "Start transforming group layer"), NULL },
    { LIGMA_UNDO_GROUP_LAYER_END_TRANSFORM, NC_("undo-type", "End transforming group layer"), NULL },
    { LIGMA_UNDO_GROUP_LAYER_CONVERT, NC_("undo-type", "Convert group layer"), NULL },
    { LIGMA_UNDO_TEXT_LAYER, NC_("undo-type", "Text layer"), NULL },
    { LIGMA_UNDO_TEXT_LAYER_MODIFIED, NC_("undo-type", "Text layer modification"), NULL },
    { LIGMA_UNDO_TEXT_LAYER_CONVERT, NC_("undo-type", "Convert text layer"), NULL },
    { LIGMA_UNDO_LAYER_MASK_ADD, NC_("undo-type", "Add layer mask"), NULL },
    { LIGMA_UNDO_LAYER_MASK_REMOVE, NC_("undo-type", "Delete layer mask"), NULL },
    { LIGMA_UNDO_LAYER_MASK_APPLY, NC_("undo-type", "Apply layer mask"), NULL },
    { LIGMA_UNDO_LAYER_MASK_SHOW, NC_("undo-type", "Show layer mask"), NULL },
    { LIGMA_UNDO_CHANNEL_ADD, NC_("undo-type", "New channel"), NULL },
    { LIGMA_UNDO_CHANNEL_REMOVE, NC_("undo-type", "Delete channel"), NULL },
    { LIGMA_UNDO_CHANNEL_COLOR, NC_("undo-type", "Channel color"), NULL },
    { LIGMA_UNDO_VECTORS_ADD, NC_("undo-type", "New path"), NULL },
    { LIGMA_UNDO_VECTORS_REMOVE, NC_("undo-type", "Delete path"), NULL },
    { LIGMA_UNDO_VECTORS_MOD, NC_("undo-type", "Path modification"), NULL },
    { LIGMA_UNDO_FS_TO_LAYER, NC_("undo-type", "Floating selection to layer"), NULL },
    { LIGMA_UNDO_TRANSFORM_GRID, NC_("undo-type", "Transform grid"), NULL },
    { LIGMA_UNDO_PAINT, NC_("undo-type", "Paint"), NULL },
    { LIGMA_UNDO_INK, NC_("undo-type", "Ink"), NULL },
    { LIGMA_UNDO_FOREGROUND_SELECT, NC_("undo-type", "Select foreground"), NULL },
    { LIGMA_UNDO_PARASITE_ATTACH, NC_("undo-type", "Attach parasite"), NULL },
    { LIGMA_UNDO_PARASITE_REMOVE, NC_("undo-type", "Remove parasite"), NULL },
    { LIGMA_UNDO_CANT, NC_("undo-type", "Not undoable"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaUndoType", values);
      ligma_type_set_translation_context (type, "undo-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_view_size_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_VIEW_SIZE_TINY, "LIGMA_VIEW_SIZE_TINY", "tiny" },
    { LIGMA_VIEW_SIZE_EXTRA_SMALL, "LIGMA_VIEW_SIZE_EXTRA_SMALL", "extra-small" },
    { LIGMA_VIEW_SIZE_SMALL, "LIGMA_VIEW_SIZE_SMALL", "small" },
    { LIGMA_VIEW_SIZE_MEDIUM, "LIGMA_VIEW_SIZE_MEDIUM", "medium" },
    { LIGMA_VIEW_SIZE_LARGE, "LIGMA_VIEW_SIZE_LARGE", "large" },
    { LIGMA_VIEW_SIZE_EXTRA_LARGE, "LIGMA_VIEW_SIZE_EXTRA_LARGE", "extra-large" },
    { LIGMA_VIEW_SIZE_HUGE, "LIGMA_VIEW_SIZE_HUGE", "huge" },
    { LIGMA_VIEW_SIZE_ENORMOUS, "LIGMA_VIEW_SIZE_ENORMOUS", "enormous" },
    { LIGMA_VIEW_SIZE_GIGANTIC, "LIGMA_VIEW_SIZE_GIGANTIC", "gigantic" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_VIEW_SIZE_TINY, NC_("view-size", "Tiny"), NULL },
    { LIGMA_VIEW_SIZE_EXTRA_SMALL, NC_("view-size", "Very small"), NULL },
    { LIGMA_VIEW_SIZE_SMALL, NC_("view-size", "Small"), NULL },
    { LIGMA_VIEW_SIZE_MEDIUM, NC_("view-size", "Medium"), NULL },
    { LIGMA_VIEW_SIZE_LARGE, NC_("view-size", "Large"), NULL },
    { LIGMA_VIEW_SIZE_EXTRA_LARGE, NC_("view-size", "Very large"), NULL },
    { LIGMA_VIEW_SIZE_HUGE, NC_("view-size", "Huge"), NULL },
    { LIGMA_VIEW_SIZE_ENORMOUS, NC_("view-size", "Enormous"), NULL },
    { LIGMA_VIEW_SIZE_GIGANTIC, NC_("view-size", "Gigantic"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaViewSize", values);
      ligma_type_set_translation_context (type, "view-size");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_view_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_VIEW_TYPE_LIST, "LIGMA_VIEW_TYPE_LIST", "list" },
    { LIGMA_VIEW_TYPE_GRID, "LIGMA_VIEW_TYPE_GRID", "grid" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_VIEW_TYPE_LIST, NC_("view-type", "View as list"), NULL },
    { LIGMA_VIEW_TYPE_GRID, NC_("view-type", "View as grid"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaViewType", values);
      ligma_type_set_translation_context (type, "view-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_select_method_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_SELECT_PLAIN_TEXT, "LIGMA_SELECT_PLAIN_TEXT", "plain-text" },
    { LIGMA_SELECT_REGEX_PATTERN, "LIGMA_SELECT_REGEX_PATTERN", "regex-pattern" },
    { LIGMA_SELECT_GLOB_PATTERN, "LIGMA_SELECT_GLOB_PATTERN", "glob-pattern" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_SELECT_PLAIN_TEXT, NC_("select-method", "Selection by basic text search"), NULL },
    { LIGMA_SELECT_REGEX_PATTERN, NC_("select-method", "Selection by regular expression search"), NULL },
    { LIGMA_SELECT_GLOB_PATTERN, NC_("select-method", "Selection by glob pattern search"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaSelectMethod", values);
      ligma_type_set_translation_context (type, "select-method");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

