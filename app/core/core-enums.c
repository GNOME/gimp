
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <gio/gio.h>
#include "libgimpbase/gimpbase.h"
#include "core-enums.h"
#include "gimp-intl.h"

/* enumerations from "core-enums.h" */
GType
gimp_component_mask_get_type (void)
{
  static const GFlagsValue values[] =
  {
    { GIMP_COMPONENT_MASK_RED, "GIMP_COMPONENT_MASK_RED", "red" },
    { GIMP_COMPONENT_MASK_GREEN, "GIMP_COMPONENT_MASK_GREEN", "green" },
    { GIMP_COMPONENT_MASK_BLUE, "GIMP_COMPONENT_MASK_BLUE", "blue" },
    { GIMP_COMPONENT_MASK_ALPHA, "GIMP_COMPONENT_MASK_ALPHA", "alpha" },
    { GIMP_COMPONENT_MASK_ALL, "GIMP_COMPONENT_MASK_ALL", "all" },
    { 0, NULL, NULL }
  };

  static const GimpFlagsDesc descs[] =
  {
    { GIMP_COMPONENT_MASK_RED, "GIMP_COMPONENT_MASK_RED", NULL },
    { GIMP_COMPONENT_MASK_GREEN, "GIMP_COMPONENT_MASK_GREEN", NULL },
    { GIMP_COMPONENT_MASK_BLUE, "GIMP_COMPONENT_MASK_BLUE", NULL },
    { GIMP_COMPONENT_MASK_ALPHA, "GIMP_COMPONENT_MASK_ALPHA", NULL },
    { GIMP_COMPONENT_MASK_ALL, "GIMP_COMPONENT_MASK_ALL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_flags_register_static ("GimpComponentMask", values);
      gimp_type_set_translation_context (type, "component-mask");
      gimp_flags_set_value_descriptions (type, descs);
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

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpContainerPolicy", values);
      gimp_type_set_translation_context (type, "container-policy");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_convert_dither_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_CONVERT_DITHER_NONE, "GIMP_CONVERT_DITHER_NONE", "none" },
    { GIMP_CONVERT_DITHER_FS, "GIMP_CONVERT_DITHER_FS", "fs" },
    { GIMP_CONVERT_DITHER_FS_LOWBLEED, "GIMP_CONVERT_DITHER_FS_LOWBLEED", "fs-lowbleed" },
    { GIMP_CONVERT_DITHER_FIXED, "GIMP_CONVERT_DITHER_FIXED", "fixed" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_CONVERT_DITHER_NONE, NC_("convert-dither-type", "None"), NULL },
    { GIMP_CONVERT_DITHER_FS, NC_("convert-dither-type", "Floyd-Steinberg (normal)"), NULL },
    { GIMP_CONVERT_DITHER_FS_LOWBLEED, NC_("convert-dither-type", "Floyd-Steinberg (reduced color bleeding)"), NULL },
    { GIMP_CONVERT_DITHER_FIXED, NC_("convert-dither-type", "Positioned"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpConvertDitherType", values);
      gimp_type_set_translation_context (type, "convert-dither-type");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_convolution_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_NORMAL_CONVOL, "GIMP_NORMAL_CONVOL", "normal-convol" },
    { GIMP_ABSOLUTE_CONVOL, "GIMP_ABSOLUTE_CONVOL", "absolute-convol" },
    { GIMP_NEGATIVE_CONVOL, "GIMP_NEGATIVE_CONVOL", "negative-convol" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_NORMAL_CONVOL, "GIMP_NORMAL_CONVOL", NULL },
    { GIMP_ABSOLUTE_CONVOL, "GIMP_ABSOLUTE_CONVOL", NULL },
    { GIMP_NEGATIVE_CONVOL, "GIMP_NEGATIVE_CONVOL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpConvolutionType", values);
      gimp_type_set_translation_context (type, "convolution-type");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_curve_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_CURVE_SMOOTH, "GIMP_CURVE_SMOOTH", "smooth" },
    { GIMP_CURVE_FREE, "GIMP_CURVE_FREE", "free" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_CURVE_SMOOTH, NC_("curve-type", "Smooth"), NULL },
    { GIMP_CURVE_FREE, NC_("curve-type", "Freehand"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpCurveType", values);
      gimp_type_set_translation_context (type, "curve-type");
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

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpGravityType", values);
      gimp_type_set_translation_context (type, "gravity-type");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_guide_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_GUIDE_STYLE_NONE, "GIMP_GUIDE_STYLE_NONE", "none" },
    { GIMP_GUIDE_STYLE_NORMAL, "GIMP_GUIDE_STYLE_NORMAL", "normal" },
    { GIMP_GUIDE_STYLE_MIRROR, "GIMP_GUIDE_STYLE_MIRROR", "mirror" },
    { GIMP_GUIDE_STYLE_MANDALA, "GIMP_GUIDE_STYLE_MANDALA", "mandala" },
    { GIMP_GUIDE_STYLE_SPLIT_VIEW, "GIMP_GUIDE_STYLE_SPLIT_VIEW", "split-view" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_GUIDE_STYLE_NONE, "GIMP_GUIDE_STYLE_NONE", NULL },
    { GIMP_GUIDE_STYLE_NORMAL, "GIMP_GUIDE_STYLE_NORMAL", NULL },
    { GIMP_GUIDE_STYLE_MIRROR, "GIMP_GUIDE_STYLE_MIRROR", NULL },
    { GIMP_GUIDE_STYLE_MANDALA, "GIMP_GUIDE_STYLE_MANDALA", NULL },
    { GIMP_GUIDE_STYLE_SPLIT_VIEW, "GIMP_GUIDE_STYLE_SPLIT_VIEW", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpGuideStyle", values);
      gimp_type_set_translation_context (type, "guide-style");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_histogram_channel_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_HISTOGRAM_VALUE, "GIMP_HISTOGRAM_VALUE", "value" },
    { GIMP_HISTOGRAM_RED, "GIMP_HISTOGRAM_RED", "red" },
    { GIMP_HISTOGRAM_GREEN, "GIMP_HISTOGRAM_GREEN", "green" },
    { GIMP_HISTOGRAM_BLUE, "GIMP_HISTOGRAM_BLUE", "blue" },
    { GIMP_HISTOGRAM_ALPHA, "GIMP_HISTOGRAM_ALPHA", "alpha" },
    { GIMP_HISTOGRAM_LUMINANCE, "GIMP_HISTOGRAM_LUMINANCE", "luminance" },
    { GIMP_HISTOGRAM_RGB, "GIMP_HISTOGRAM_RGB", "rgb" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_HISTOGRAM_VALUE, NC_("histogram-channel", "Value"), NULL },
    { GIMP_HISTOGRAM_RED, NC_("histogram-channel", "Red"), NULL },
    { GIMP_HISTOGRAM_GREEN, NC_("histogram-channel", "Green"), NULL },
    { GIMP_HISTOGRAM_BLUE, NC_("histogram-channel", "Blue"), NULL },
    { GIMP_HISTOGRAM_ALPHA, NC_("histogram-channel", "Alpha"), NULL },
    { GIMP_HISTOGRAM_LUMINANCE, NC_("histogram-channel", "Luminance"), NULL },
    { GIMP_HISTOGRAM_RGB, NC_("histogram-channel", "RGB"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpHistogramChannel", values);
      gimp_type_set_translation_context (type, "histogram-channel");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_matting_engine_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_MATTING_ENGINE_GLOBAL, "GIMP_MATTING_ENGINE_GLOBAL", "global" },
    { GIMP_MATTING_ENGINE_LEVIN, "GIMP_MATTING_ENGINE_LEVIN", "levin" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_MATTING_ENGINE_GLOBAL, NC_("matting-engine", "Matting Global"), NULL },
    { GIMP_MATTING_ENGINE_LEVIN, NC_("matting-engine", "Matting Levin"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpMattingEngine", values);
      gimp_type_set_translation_context (type, "matting-engine");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_paste_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_PASTE_TYPE_FLOATING, "GIMP_PASTE_TYPE_FLOATING", "floating" },
    { GIMP_PASTE_TYPE_FLOATING_IN_PLACE, "GIMP_PASTE_TYPE_FLOATING_IN_PLACE", "floating-in-place" },
    { GIMP_PASTE_TYPE_FLOATING_INTO, "GIMP_PASTE_TYPE_FLOATING_INTO", "floating-into" },
    { GIMP_PASTE_TYPE_FLOATING_INTO_IN_PLACE, "GIMP_PASTE_TYPE_FLOATING_INTO_IN_PLACE", "floating-into-in-place" },
    { GIMP_PASTE_TYPE_NEW_LAYER, "GIMP_PASTE_TYPE_NEW_LAYER", "new-layer" },
    { GIMP_PASTE_TYPE_NEW_LAYER_IN_PLACE, "GIMP_PASTE_TYPE_NEW_LAYER_IN_PLACE", "new-layer-in-place" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_PASTE_TYPE_FLOATING, "GIMP_PASTE_TYPE_FLOATING", NULL },
    { GIMP_PASTE_TYPE_FLOATING_IN_PLACE, "GIMP_PASTE_TYPE_FLOATING_IN_PLACE", NULL },
    { GIMP_PASTE_TYPE_FLOATING_INTO, "GIMP_PASTE_TYPE_FLOATING_INTO", NULL },
    { GIMP_PASTE_TYPE_FLOATING_INTO_IN_PLACE, "GIMP_PASTE_TYPE_FLOATING_INTO_IN_PLACE", NULL },
    { GIMP_PASTE_TYPE_NEW_LAYER, "GIMP_PASTE_TYPE_NEW_LAYER", NULL },
    { GIMP_PASTE_TYPE_NEW_LAYER_IN_PLACE, "GIMP_PASTE_TYPE_NEW_LAYER_IN_PLACE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpPasteType", values);
      gimp_type_set_translation_context (type, "paste-type");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_alignment_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_ALIGN_LEFT, "GIMP_ALIGN_LEFT", "align-left" },
    { GIMP_ALIGN_HCENTER, "GIMP_ALIGN_HCENTER", "align-hcenter" },
    { GIMP_ALIGN_RIGHT, "GIMP_ALIGN_RIGHT", "align-right" },
    { GIMP_ALIGN_TOP, "GIMP_ALIGN_TOP", "align-top" },
    { GIMP_ALIGN_VCENTER, "GIMP_ALIGN_VCENTER", "align-vcenter" },
    { GIMP_ALIGN_BOTTOM, "GIMP_ALIGN_BOTTOM", "align-bottom" },
    { GIMP_ARRANGE_LEFT, "GIMP_ARRANGE_LEFT", "arrange-left" },
    { GIMP_ARRANGE_HCENTER, "GIMP_ARRANGE_HCENTER", "arrange-hcenter" },
    { GIMP_ARRANGE_RIGHT, "GIMP_ARRANGE_RIGHT", "arrange-right" },
    { GIMP_ARRANGE_TOP, "GIMP_ARRANGE_TOP", "arrange-top" },
    { GIMP_ARRANGE_VCENTER, "GIMP_ARRANGE_VCENTER", "arrange-vcenter" },
    { GIMP_ARRANGE_BOTTOM, "GIMP_ARRANGE_BOTTOM", "arrange-bottom" },
    { GIMP_ARRANGE_HFILL, "GIMP_ARRANGE_HFILL", "arrange-hfill" },
    { GIMP_ARRANGE_VFILL, "GIMP_ARRANGE_VFILL", "arrange-vfill" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_ALIGN_LEFT, "GIMP_ALIGN_LEFT", NULL },
    { GIMP_ALIGN_HCENTER, "GIMP_ALIGN_HCENTER", NULL },
    { GIMP_ALIGN_RIGHT, "GIMP_ALIGN_RIGHT", NULL },
    { GIMP_ALIGN_TOP, "GIMP_ALIGN_TOP", NULL },
    { GIMP_ALIGN_VCENTER, "GIMP_ALIGN_VCENTER", NULL },
    { GIMP_ALIGN_BOTTOM, "GIMP_ALIGN_BOTTOM", NULL },
    { GIMP_ARRANGE_LEFT, "GIMP_ARRANGE_LEFT", NULL },
    { GIMP_ARRANGE_HCENTER, "GIMP_ARRANGE_HCENTER", NULL },
    { GIMP_ARRANGE_RIGHT, "GIMP_ARRANGE_RIGHT", NULL },
    { GIMP_ARRANGE_TOP, "GIMP_ARRANGE_TOP", NULL },
    { GIMP_ARRANGE_VCENTER, "GIMP_ARRANGE_VCENTER", NULL },
    { GIMP_ARRANGE_BOTTOM, "GIMP_ARRANGE_BOTTOM", NULL },
    { GIMP_ARRANGE_HFILL, "GIMP_ARRANGE_HFILL", NULL },
    { GIMP_ARRANGE_VFILL, "GIMP_ARRANGE_VFILL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpAlignmentType", values);
      gimp_type_set_translation_context (type, "alignment-type");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_align_reference_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_ALIGN_REFERENCE_FIRST, "GIMP_ALIGN_REFERENCE_FIRST", "first" },
    { GIMP_ALIGN_REFERENCE_IMAGE, "GIMP_ALIGN_REFERENCE_IMAGE", "image" },
    { GIMP_ALIGN_REFERENCE_SELECTION, "GIMP_ALIGN_REFERENCE_SELECTION", "selection" },
    { GIMP_ALIGN_REFERENCE_ACTIVE_LAYER, "GIMP_ALIGN_REFERENCE_ACTIVE_LAYER", "active-layer" },
    { GIMP_ALIGN_REFERENCE_ACTIVE_CHANNEL, "GIMP_ALIGN_REFERENCE_ACTIVE_CHANNEL", "active-channel" },
    { GIMP_ALIGN_REFERENCE_ACTIVE_PATH, "GIMP_ALIGN_REFERENCE_ACTIVE_PATH", "active-path" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_ALIGN_REFERENCE_FIRST, NC_("align-reference-type", "First item"), NULL },
    { GIMP_ALIGN_REFERENCE_IMAGE, NC_("align-reference-type", "Image"), NULL },
    { GIMP_ALIGN_REFERENCE_SELECTION, NC_("align-reference-type", "Selection"), NULL },
    { GIMP_ALIGN_REFERENCE_ACTIVE_LAYER, NC_("align-reference-type", "Active layer"), NULL },
    { GIMP_ALIGN_REFERENCE_ACTIVE_CHANNEL, NC_("align-reference-type", "Active channel"), NULL },
    { GIMP_ALIGN_REFERENCE_ACTIVE_PATH, NC_("align-reference-type", "Active path"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpAlignReferenceType", values);
      gimp_type_set_translation_context (type, "align-reference-type");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_fill_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_FILL_STYLE_SOLID, "GIMP_FILL_STYLE_SOLID", "solid" },
    { GIMP_FILL_STYLE_PATTERN, "GIMP_FILL_STYLE_PATTERN", "pattern" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_FILL_STYLE_SOLID, NC_("fill-style", "Solid color"), NULL },
    { GIMP_FILL_STYLE_PATTERN, NC_("fill-style", "Pattern"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpFillStyle", values);
      gimp_type_set_translation_context (type, "fill-style");
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
    { GIMP_DASH_CUSTOM, NC_("dash-preset", "Custom"), NULL },
    { GIMP_DASH_LINE, NC_("dash-preset", "Line"), NULL },
    { GIMP_DASH_LONG_DASH, NC_("dash-preset", "Long dashes"), NULL },
    { GIMP_DASH_MEDIUM_DASH, NC_("dash-preset", "Medium dashes"), NULL },
    { GIMP_DASH_SHORT_DASH, NC_("dash-preset", "Short dashes"), NULL },
    { GIMP_DASH_SPARSE_DOTS, NC_("dash-preset", "Sparse dots"), NULL },
    { GIMP_DASH_NORMAL_DOTS, NC_("dash-preset", "Normal dots"), NULL },
    { GIMP_DASH_DENSE_DOTS, NC_("dash-preset", "Dense dots"), NULL },
    { GIMP_DASH_STIPPLES, NC_("dash-preset", "Stipples"), NULL },
    { GIMP_DASH_DASH_DOT, NC_("dash-preset", "Dash, dot"), NULL },
    { GIMP_DASH_DASH_DOT_DOT, NC_("dash-preset", "Dash, dot, dot"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpDashPreset", values);
      gimp_type_set_translation_context (type, "dash-preset");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_item_set_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_ITEM_SET_NONE, "GIMP_ITEM_SET_NONE", "none" },
    { GIMP_ITEM_SET_ALL, "GIMP_ITEM_SET_ALL", "all" },
    { GIMP_ITEM_SET_IMAGE_SIZED, "GIMP_ITEM_SET_IMAGE_SIZED", "image-sized" },
    { GIMP_ITEM_SET_VISIBLE, "GIMP_ITEM_SET_VISIBLE", "visible" },
    { GIMP_ITEM_SET_LINKED, "GIMP_ITEM_SET_LINKED", "linked" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_ITEM_SET_NONE, NC_("item-set", "None"), NULL },
    { GIMP_ITEM_SET_ALL, NC_("item-set", "All layers"), NULL },
    { GIMP_ITEM_SET_IMAGE_SIZED, NC_("item-set", "Image-sized layers"), NULL },
    { GIMP_ITEM_SET_VISIBLE, NC_("item-set", "All visible layers"), NULL },
    { GIMP_ITEM_SET_LINKED, NC_("item-set", "All linked layers"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpItemSet", values);
      gimp_type_set_translation_context (type, "item-set");
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
    { GIMP_VIEW_SIZE_TINY, NC_("view-size", "Tiny"), NULL },
    { GIMP_VIEW_SIZE_EXTRA_SMALL, NC_("view-size", "Very small"), NULL },
    { GIMP_VIEW_SIZE_SMALL, NC_("view-size", "Small"), NULL },
    { GIMP_VIEW_SIZE_MEDIUM, NC_("view-size", "Medium"), NULL },
    { GIMP_VIEW_SIZE_LARGE, NC_("view-size", "Large"), NULL },
    { GIMP_VIEW_SIZE_EXTRA_LARGE, NC_("view-size", "Very large"), NULL },
    { GIMP_VIEW_SIZE_HUGE, NC_("view-size", "Huge"), NULL },
    { GIMP_VIEW_SIZE_ENORMOUS, NC_("view-size", "Enormous"), NULL },
    { GIMP_VIEW_SIZE_GIGANTIC, NC_("view-size", "Gigantic"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpViewSize", values);
      gimp_type_set_translation_context (type, "view-size");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_view_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_VIEW_TYPE_LIST, "GIMP_VIEW_TYPE_LIST", "list" },
    { GIMP_VIEW_TYPE_GRID, "GIMP_VIEW_TYPE_GRID", "grid" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_VIEW_TYPE_LIST, NC_("view-type", "View as list"), NULL },
    { GIMP_VIEW_TYPE_GRID, NC_("view-type", "View as grid"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpViewType", values);
      gimp_type_set_translation_context (type, "view-type");
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
    { GIMP_THUMBNAIL_SIZE_NONE, NC_("thumbnail-size", "No thumbnails"), NULL },
    { GIMP_THUMBNAIL_SIZE_NORMAL, NC_("thumbnail-size", "Normal (128x128)"), NULL },
    { GIMP_THUMBNAIL_SIZE_LARGE, NC_("thumbnail-size", "Large (256x256)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpThumbnailSize", values);
      gimp_type_set_translation_context (type, "thumbnail-size");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_debug_policy_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_DEBUG_POLICY_WARNING, "GIMP_DEBUG_POLICY_WARNING", "warning" },
    { GIMP_DEBUG_POLICY_CRITICAL, "GIMP_DEBUG_POLICY_CRITICAL", "critical" },
    { GIMP_DEBUG_POLICY_FATAL, "GIMP_DEBUG_POLICY_FATAL", "fatal" },
    { GIMP_DEBUG_POLICY_NEVER, "GIMP_DEBUG_POLICY_NEVER", "never" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_DEBUG_POLICY_WARNING, NC_("debug-policy", "Debug warnings, critical errors and crashes"), NULL },
    { GIMP_DEBUG_POLICY_CRITICAL, NC_("debug-policy", "Debug critical errors and crashes"), NULL },
    { GIMP_DEBUG_POLICY_FATAL, NC_("debug-policy", "Debug crashes only"), NULL },
    { GIMP_DEBUG_POLICY_NEVER, NC_("debug-policy", "Never debug GIMP"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpDebugPolicy", values);
      gimp_type_set_translation_context (type, "debug-policy");
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

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpUndoMode", values);
      gimp_type_set_translation_context (type, "undo-mode");
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

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpUndoEvent", values);
      gimp_type_set_translation_context (type, "undo-event");
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
    { GIMP_UNDO_GROUP_IMAGE_ITEM_REORDER, "GIMP_UNDO_GROUP_IMAGE_ITEM_REORDER", "group-image-item-reorder" },
    { GIMP_UNDO_GROUP_IMAGE_LAYERS_MERGE, "GIMP_UNDO_GROUP_IMAGE_LAYERS_MERGE", "group-image-layers-merge" },
    { GIMP_UNDO_GROUP_IMAGE_VECTORS_MERGE, "GIMP_UNDO_GROUP_IMAGE_VECTORS_MERGE", "group-image-vectors-merge" },
    { GIMP_UNDO_GROUP_IMAGE_QUICK_MASK, "GIMP_UNDO_GROUP_IMAGE_QUICK_MASK", "group-image-quick-mask" },
    { GIMP_UNDO_GROUP_IMAGE_GRID, "GIMP_UNDO_GROUP_IMAGE_GRID", "group-image-grid" },
    { GIMP_UNDO_GROUP_GUIDE, "GIMP_UNDO_GROUP_GUIDE", "group-guide" },
    { GIMP_UNDO_GROUP_SAMPLE_POINT, "GIMP_UNDO_GROUP_SAMPLE_POINT", "group-sample-point" },
    { GIMP_UNDO_GROUP_DRAWABLE, "GIMP_UNDO_GROUP_DRAWABLE", "group-drawable" },
    { GIMP_UNDO_GROUP_DRAWABLE_MOD, "GIMP_UNDO_GROUP_DRAWABLE_MOD", "group-drawable-mod" },
    { GIMP_UNDO_GROUP_MASK, "GIMP_UNDO_GROUP_MASK", "group-mask" },
    { GIMP_UNDO_GROUP_ITEM_VISIBILITY, "GIMP_UNDO_GROUP_ITEM_VISIBILITY", "group-item-visibility" },
    { GIMP_UNDO_GROUP_ITEM_LINKED, "GIMP_UNDO_GROUP_ITEM_LINKED", "group-item-linked" },
    { GIMP_UNDO_GROUP_ITEM_PROPERTIES, "GIMP_UNDO_GROUP_ITEM_PROPERTIES", "group-item-properties" },
    { GIMP_UNDO_GROUP_ITEM_DISPLACE, "GIMP_UNDO_GROUP_ITEM_DISPLACE", "group-item-displace" },
    { GIMP_UNDO_GROUP_ITEM_SCALE, "GIMP_UNDO_GROUP_ITEM_SCALE", "group-item-scale" },
    { GIMP_UNDO_GROUP_ITEM_RESIZE, "GIMP_UNDO_GROUP_ITEM_RESIZE", "group-item-resize" },
    { GIMP_UNDO_GROUP_LAYER_ADD, "GIMP_UNDO_GROUP_LAYER_ADD", "group-layer-add" },
    { GIMP_UNDO_GROUP_LAYER_ADD_MASK, "GIMP_UNDO_GROUP_LAYER_ADD_MASK", "group-layer-add-mask" },
    { GIMP_UNDO_GROUP_LAYER_APPLY_MASK, "GIMP_UNDO_GROUP_LAYER_APPLY_MASK", "group-layer-apply-mask" },
    { GIMP_UNDO_GROUP_FS_TO_LAYER, "GIMP_UNDO_GROUP_FS_TO_LAYER", "group-fs-to-layer" },
    { GIMP_UNDO_GROUP_FS_FLOAT, "GIMP_UNDO_GROUP_FS_FLOAT", "group-fs-float" },
    { GIMP_UNDO_GROUP_FS_ANCHOR, "GIMP_UNDO_GROUP_FS_ANCHOR", "group-fs-anchor" },
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
    { GIMP_UNDO_IMAGE_PRECISION, "GIMP_UNDO_IMAGE_PRECISION", "image-precision" },
    { GIMP_UNDO_IMAGE_SIZE, "GIMP_UNDO_IMAGE_SIZE", "image-size" },
    { GIMP_UNDO_IMAGE_RESOLUTION, "GIMP_UNDO_IMAGE_RESOLUTION", "image-resolution" },
    { GIMP_UNDO_IMAGE_GRID, "GIMP_UNDO_IMAGE_GRID", "image-grid" },
    { GIMP_UNDO_IMAGE_METADATA, "GIMP_UNDO_IMAGE_METADATA", "image-metadata" },
    { GIMP_UNDO_IMAGE_COLORMAP, "GIMP_UNDO_IMAGE_COLORMAP", "image-colormap" },
    { GIMP_UNDO_IMAGE_COLOR_MANAGED, "GIMP_UNDO_IMAGE_COLOR_MANAGED", "image-color-managed" },
    { GIMP_UNDO_GUIDE, "GIMP_UNDO_GUIDE", "guide" },
    { GIMP_UNDO_SAMPLE_POINT, "GIMP_UNDO_SAMPLE_POINT", "sample-point" },
    { GIMP_UNDO_DRAWABLE, "GIMP_UNDO_DRAWABLE", "drawable" },
    { GIMP_UNDO_DRAWABLE_MOD, "GIMP_UNDO_DRAWABLE_MOD", "drawable-mod" },
    { GIMP_UNDO_MASK, "GIMP_UNDO_MASK", "mask" },
    { GIMP_UNDO_ITEM_REORDER, "GIMP_UNDO_ITEM_REORDER", "item-reorder" },
    { GIMP_UNDO_ITEM_RENAME, "GIMP_UNDO_ITEM_RENAME", "item-rename" },
    { GIMP_UNDO_ITEM_DISPLACE, "GIMP_UNDO_ITEM_DISPLACE", "item-displace" },
    { GIMP_UNDO_ITEM_VISIBILITY, "GIMP_UNDO_ITEM_VISIBILITY", "item-visibility" },
    { GIMP_UNDO_ITEM_LINKED, "GIMP_UNDO_ITEM_LINKED", "item-linked" },
    { GIMP_UNDO_ITEM_COLOR_TAG, "GIMP_UNDO_ITEM_COLOR_TAG", "item-color-tag" },
    { GIMP_UNDO_ITEM_LOCK_CONTENT, "GIMP_UNDO_ITEM_LOCK_CONTENT", "item-lock-content" },
    { GIMP_UNDO_ITEM_LOCK_POSITION, "GIMP_UNDO_ITEM_LOCK_POSITION", "item-lock-position" },
    { GIMP_UNDO_LAYER_ADD, "GIMP_UNDO_LAYER_ADD", "layer-add" },
    { GIMP_UNDO_LAYER_REMOVE, "GIMP_UNDO_LAYER_REMOVE", "layer-remove" },
    { GIMP_UNDO_LAYER_MODE, "GIMP_UNDO_LAYER_MODE", "layer-mode" },
    { GIMP_UNDO_LAYER_OPACITY, "GIMP_UNDO_LAYER_OPACITY", "layer-opacity" },
    { GIMP_UNDO_LAYER_LOCK_ALPHA, "GIMP_UNDO_LAYER_LOCK_ALPHA", "layer-lock-alpha" },
    { GIMP_UNDO_GROUP_LAYER_SUSPEND_RESIZE, "GIMP_UNDO_GROUP_LAYER_SUSPEND_RESIZE", "group-layer-suspend-resize" },
    { GIMP_UNDO_GROUP_LAYER_RESUME_RESIZE, "GIMP_UNDO_GROUP_LAYER_RESUME_RESIZE", "group-layer-resume-resize" },
    { GIMP_UNDO_GROUP_LAYER_SUSPEND_MASK, "GIMP_UNDO_GROUP_LAYER_SUSPEND_MASK", "group-layer-suspend-mask" },
    { GIMP_UNDO_GROUP_LAYER_RESUME_MASK, "GIMP_UNDO_GROUP_LAYER_RESUME_MASK", "group-layer-resume-mask" },
    { GIMP_UNDO_GROUP_LAYER_START_TRANSFORM, "GIMP_UNDO_GROUP_LAYER_START_TRANSFORM", "group-layer-start-transform" },
    { GIMP_UNDO_GROUP_LAYER_END_TRANSFORM, "GIMP_UNDO_GROUP_LAYER_END_TRANSFORM", "group-layer-end-transform" },
    { GIMP_UNDO_GROUP_LAYER_CONVERT, "GIMP_UNDO_GROUP_LAYER_CONVERT", "group-layer-convert" },
    { GIMP_UNDO_TEXT_LAYER, "GIMP_UNDO_TEXT_LAYER", "text-layer" },
    { GIMP_UNDO_TEXT_LAYER_MODIFIED, "GIMP_UNDO_TEXT_LAYER_MODIFIED", "text-layer-modified" },
    { GIMP_UNDO_TEXT_LAYER_CONVERT, "GIMP_UNDO_TEXT_LAYER_CONVERT", "text-layer-convert" },
    { GIMP_UNDO_LAYER_MASK_ADD, "GIMP_UNDO_LAYER_MASK_ADD", "layer-mask-add" },
    { GIMP_UNDO_LAYER_MASK_REMOVE, "GIMP_UNDO_LAYER_MASK_REMOVE", "layer-mask-remove" },
    { GIMP_UNDO_LAYER_MASK_APPLY, "GIMP_UNDO_LAYER_MASK_APPLY", "layer-mask-apply" },
    { GIMP_UNDO_LAYER_MASK_SHOW, "GIMP_UNDO_LAYER_MASK_SHOW", "layer-mask-show" },
    { GIMP_UNDO_CHANNEL_ADD, "GIMP_UNDO_CHANNEL_ADD", "channel-add" },
    { GIMP_UNDO_CHANNEL_REMOVE, "GIMP_UNDO_CHANNEL_REMOVE", "channel-remove" },
    { GIMP_UNDO_CHANNEL_COLOR, "GIMP_UNDO_CHANNEL_COLOR", "channel-color" },
    { GIMP_UNDO_VECTORS_ADD, "GIMP_UNDO_VECTORS_ADD", "vectors-add" },
    { GIMP_UNDO_VECTORS_REMOVE, "GIMP_UNDO_VECTORS_REMOVE", "vectors-remove" },
    { GIMP_UNDO_VECTORS_MOD, "GIMP_UNDO_VECTORS_MOD", "vectors-mod" },
    { GIMP_UNDO_FS_TO_LAYER, "GIMP_UNDO_FS_TO_LAYER", "fs-to-layer" },
    { GIMP_UNDO_TRANSFORM_GRID, "GIMP_UNDO_TRANSFORM_GRID", "transform-grid" },
    { GIMP_UNDO_PAINT, "GIMP_UNDO_PAINT", "paint" },
    { GIMP_UNDO_INK, "GIMP_UNDO_INK", "ink" },
    { GIMP_UNDO_FOREGROUND_SELECT, "GIMP_UNDO_FOREGROUND_SELECT", "foreground-select" },
    { GIMP_UNDO_PARASITE_ATTACH, "GIMP_UNDO_PARASITE_ATTACH", "parasite-attach" },
    { GIMP_UNDO_PARASITE_REMOVE, "GIMP_UNDO_PARASITE_REMOVE", "parasite-remove" },
    { GIMP_UNDO_CANT, "GIMP_UNDO_CANT", "cant" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_UNDO_GROUP_NONE, NC_("undo-type", "<<invalid>>"), NULL },
    { GIMP_UNDO_GROUP_IMAGE_SCALE, NC_("undo-type", "Scale image"), NULL },
    { GIMP_UNDO_GROUP_IMAGE_RESIZE, NC_("undo-type", "Resize image"), NULL },
    { GIMP_UNDO_GROUP_IMAGE_FLIP, NC_("undo-type", "Flip image"), NULL },
    { GIMP_UNDO_GROUP_IMAGE_ROTATE, NC_("undo-type", "Rotate image"), NULL },
    { GIMP_UNDO_GROUP_IMAGE_CROP, NC_("undo-type", "Crop image"), NULL },
    { GIMP_UNDO_GROUP_IMAGE_CONVERT, NC_("undo-type", "Convert image"), NULL },
    { GIMP_UNDO_GROUP_IMAGE_ITEM_REMOVE, NC_("undo-type", "Remove item"), NULL },
    { GIMP_UNDO_GROUP_IMAGE_ITEM_REORDER, NC_("undo-type", "Reorder item"), NULL },
    { GIMP_UNDO_GROUP_IMAGE_LAYERS_MERGE, NC_("undo-type", "Merge layers"), NULL },
    { GIMP_UNDO_GROUP_IMAGE_VECTORS_MERGE, NC_("undo-type", "Merge paths"), NULL },
    { GIMP_UNDO_GROUP_IMAGE_QUICK_MASK, NC_("undo-type", "Quick Mask"), NULL },
    { GIMP_UNDO_GROUP_IMAGE_GRID, NC_("undo-type", "Grid"), NULL },
    { GIMP_UNDO_GROUP_GUIDE, NC_("undo-type", "Guide"), NULL },
    { GIMP_UNDO_GROUP_SAMPLE_POINT, NC_("undo-type", "Sample Point"), NULL },
    { GIMP_UNDO_GROUP_DRAWABLE, NC_("undo-type", "Layer/Channel"), NULL },
    { GIMP_UNDO_GROUP_DRAWABLE_MOD, NC_("undo-type", "Layer/Channel modification"), NULL },
    { GIMP_UNDO_GROUP_MASK, NC_("undo-type", "Selection mask"), NULL },
    { GIMP_UNDO_GROUP_ITEM_VISIBILITY, NC_("undo-type", "Item visibility"), NULL },
    { GIMP_UNDO_GROUP_ITEM_LINKED, NC_("undo-type", "Link/Unlink item"), NULL },
    { GIMP_UNDO_GROUP_ITEM_PROPERTIES, NC_("undo-type", "Item properties"), NULL },
    { GIMP_UNDO_GROUP_ITEM_DISPLACE, NC_("undo-type", "Move item"), NULL },
    { GIMP_UNDO_GROUP_ITEM_SCALE, NC_("undo-type", "Scale item"), NULL },
    { GIMP_UNDO_GROUP_ITEM_RESIZE, NC_("undo-type", "Resize item"), NULL },
    { GIMP_UNDO_GROUP_LAYER_ADD, NC_("undo-type", "Add layer"), NULL },
    { GIMP_UNDO_GROUP_LAYER_ADD_MASK, NC_("undo-type", "Add layer mask"), NULL },
    { GIMP_UNDO_GROUP_LAYER_APPLY_MASK, NC_("undo-type", "Apply layer mask"), NULL },
    { GIMP_UNDO_GROUP_FS_TO_LAYER, NC_("undo-type", "Floating selection to layer"), NULL },
    { GIMP_UNDO_GROUP_FS_FLOAT, NC_("undo-type", "Float selection"), NULL },
    { GIMP_UNDO_GROUP_FS_ANCHOR, NC_("undo-type", "Anchor floating selection"), NULL },
    { GIMP_UNDO_GROUP_EDIT_PASTE, NC_("undo-type", "Paste"), NULL },
    { GIMP_UNDO_GROUP_EDIT_CUT, NC_("undo-type", "Cut"), NULL },
    { GIMP_UNDO_GROUP_TEXT, NC_("undo-type", "Text"), NULL },
    { GIMP_UNDO_GROUP_TRANSFORM, NC_("undo-type", "Transform"), NULL },
    { GIMP_UNDO_GROUP_PAINT, NC_("undo-type", "Paint"), NULL },
    { GIMP_UNDO_GROUP_PARASITE_ATTACH, NC_("undo-type", "Attach parasite"), NULL },
    { GIMP_UNDO_GROUP_PARASITE_REMOVE, NC_("undo-type", "Remove parasite"), NULL },
    { GIMP_UNDO_GROUP_VECTORS_IMPORT, NC_("undo-type", "Import paths"), NULL },
    { GIMP_UNDO_GROUP_MISC, NC_("undo-type", "Plug-In"), NULL },
    { GIMP_UNDO_IMAGE_TYPE, NC_("undo-type", "Image type"), NULL },
    { GIMP_UNDO_IMAGE_PRECISION, NC_("undo-type", "Image precision"), NULL },
    { GIMP_UNDO_IMAGE_SIZE, NC_("undo-type", "Image size"), NULL },
    { GIMP_UNDO_IMAGE_RESOLUTION, NC_("undo-type", "Image resolution change"), NULL },
    { GIMP_UNDO_IMAGE_GRID, NC_("undo-type", "Grid"), NULL },
    { GIMP_UNDO_IMAGE_METADATA, NC_("undo-type", "Change metadata"), NULL },
    { GIMP_UNDO_IMAGE_COLORMAP, NC_("undo-type", "Change indexed palette"), NULL },
    { GIMP_UNDO_IMAGE_COLOR_MANAGED, NC_("undo-type", "Change color managed state"), NULL },
    { GIMP_UNDO_GUIDE, NC_("undo-type", "Guide"), NULL },
    { GIMP_UNDO_SAMPLE_POINT, NC_("undo-type", "Sample Point"), NULL },
    { GIMP_UNDO_DRAWABLE, NC_("undo-type", "Layer/Channel"), NULL },
    { GIMP_UNDO_DRAWABLE_MOD, NC_("undo-type", "Layer/Channel modification"), NULL },
    { GIMP_UNDO_MASK, NC_("undo-type", "Selection mask"), NULL },
    { GIMP_UNDO_ITEM_REORDER, NC_("undo-type", "Reorder item"), NULL },
    { GIMP_UNDO_ITEM_RENAME, NC_("undo-type", "Rename item"), NULL },
    { GIMP_UNDO_ITEM_DISPLACE, NC_("undo-type", "Move item"), NULL },
    { GIMP_UNDO_ITEM_VISIBILITY, NC_("undo-type", "Item visibility"), NULL },
    { GIMP_UNDO_ITEM_LINKED, NC_("undo-type", "Link/Unlink item"), NULL },
    { GIMP_UNDO_ITEM_COLOR_TAG, NC_("undo-type", "Item color tag"), NULL },
    { GIMP_UNDO_ITEM_LOCK_CONTENT, NC_("undo-type", "Lock/Unlock content"), NULL },
    { GIMP_UNDO_ITEM_LOCK_POSITION, NC_("undo-type", "Lock/Unlock position"), NULL },
    { GIMP_UNDO_LAYER_ADD, NC_("undo-type", "New layer"), NULL },
    { GIMP_UNDO_LAYER_REMOVE, NC_("undo-type", "Delete layer"), NULL },
    { GIMP_UNDO_LAYER_MODE, NC_("undo-type", "Set layer mode"), NULL },
    { GIMP_UNDO_LAYER_OPACITY, NC_("undo-type", "Set layer opacity"), NULL },
    { GIMP_UNDO_LAYER_LOCK_ALPHA, NC_("undo-type", "Lock/Unlock alpha channel"), NULL },
    { GIMP_UNDO_GROUP_LAYER_SUSPEND_RESIZE, NC_("undo-type", "Suspend group layer resize"), NULL },
    { GIMP_UNDO_GROUP_LAYER_RESUME_RESIZE, NC_("undo-type", "Resume group layer resize"), NULL },
    { GIMP_UNDO_GROUP_LAYER_SUSPEND_MASK, NC_("undo-type", "Suspend group layer mask"), NULL },
    { GIMP_UNDO_GROUP_LAYER_RESUME_MASK, NC_("undo-type", "Resume group layer mask"), NULL },
    { GIMP_UNDO_GROUP_LAYER_START_TRANSFORM, NC_("undo-type", "Start transforming group layer"), NULL },
    { GIMP_UNDO_GROUP_LAYER_END_TRANSFORM, NC_("undo-type", "End transforming group layer"), NULL },
    { GIMP_UNDO_GROUP_LAYER_CONVERT, NC_("undo-type", "Convert group layer"), NULL },
    { GIMP_UNDO_TEXT_LAYER, NC_("undo-type", "Text layer"), NULL },
    { GIMP_UNDO_TEXT_LAYER_MODIFIED, NC_("undo-type", "Text layer modification"), NULL },
    { GIMP_UNDO_TEXT_LAYER_CONVERT, NC_("undo-type", "Convert text layer"), NULL },
    { GIMP_UNDO_LAYER_MASK_ADD, NC_("undo-type", "Add layer mask"), NULL },
    { GIMP_UNDO_LAYER_MASK_REMOVE, NC_("undo-type", "Delete layer mask"), NULL },
    { GIMP_UNDO_LAYER_MASK_APPLY, NC_("undo-type", "Apply layer mask"), NULL },
    { GIMP_UNDO_LAYER_MASK_SHOW, NC_("undo-type", "Show layer mask"), NULL },
    { GIMP_UNDO_CHANNEL_ADD, NC_("undo-type", "New channel"), NULL },
    { GIMP_UNDO_CHANNEL_REMOVE, NC_("undo-type", "Delete channel"), NULL },
    { GIMP_UNDO_CHANNEL_COLOR, NC_("undo-type", "Channel color"), NULL },
    { GIMP_UNDO_VECTORS_ADD, NC_("undo-type", "New path"), NULL },
    { GIMP_UNDO_VECTORS_REMOVE, NC_("undo-type", "Delete path"), NULL },
    { GIMP_UNDO_VECTORS_MOD, NC_("undo-type", "Path modification"), NULL },
    { GIMP_UNDO_FS_TO_LAYER, NC_("undo-type", "Floating selection to layer"), NULL },
    { GIMP_UNDO_TRANSFORM_GRID, NC_("undo-type", "Transform grid"), NULL },
    { GIMP_UNDO_PAINT, NC_("undo-type", "Paint"), NULL },
    { GIMP_UNDO_INK, NC_("undo-type", "Ink"), NULL },
    { GIMP_UNDO_FOREGROUND_SELECT, NC_("undo-type", "Select foreground"), NULL },
    { GIMP_UNDO_PARASITE_ATTACH, NC_("undo-type", "Attach parasite"), NULL },
    { GIMP_UNDO_PARASITE_REMOVE, NC_("undo-type", "Remove parasite"), NULL },
    { GIMP_UNDO_CANT, NC_("undo-type", "Not undoable"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpUndoType", values);
      gimp_type_set_translation_context (type, "undo-type");
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
    { GIMP_DIRTY_ACTIVE_DRAWABLE, "GIMP_DIRTY_ACTIVE_DRAWABLE", "active-drawable" },
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
    { GIMP_DIRTY_ACTIVE_DRAWABLE, "GIMP_DIRTY_ACTIVE_DRAWABLE", NULL },
    { GIMP_DIRTY_ALL, "GIMP_DIRTY_ALL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_flags_register_static ("GimpDirtyMask", values);
      gimp_type_set_translation_context (type, "dirty-mask");
      gimp_flags_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_gradient_color_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_GRADIENT_COLOR_FIXED, "GIMP_GRADIENT_COLOR_FIXED", "fixed" },
    { GIMP_GRADIENT_COLOR_FOREGROUND, "GIMP_GRADIENT_COLOR_FOREGROUND", "foreground" },
    { GIMP_GRADIENT_COLOR_FOREGROUND_TRANSPARENT, "GIMP_GRADIENT_COLOR_FOREGROUND_TRANSPARENT", "foreground-transparent" },
    { GIMP_GRADIENT_COLOR_BACKGROUND, "GIMP_GRADIENT_COLOR_BACKGROUND", "background" },
    { GIMP_GRADIENT_COLOR_BACKGROUND_TRANSPARENT, "GIMP_GRADIENT_COLOR_BACKGROUND_TRANSPARENT", "background-transparent" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_GRADIENT_COLOR_FIXED, NC_("gradient-color", "Fixed"), NULL },
    { GIMP_GRADIENT_COLOR_FOREGROUND, NC_("gradient-color", "Foreground color"), NULL },
    /* Translators: this is an abbreviated version of "Foreground color".
       Keep it short. */
    { GIMP_GRADIENT_COLOR_FOREGROUND, NC_("gradient-color", "FG"), NULL },
    { GIMP_GRADIENT_COLOR_FOREGROUND_TRANSPARENT, NC_("gradient-color", "Foreground color (transparent)"), NULL },
    /* Translators: this is an abbreviated version of "Foreground color (transparent)".
       Keep it short. */
    { GIMP_GRADIENT_COLOR_FOREGROUND_TRANSPARENT, NC_("gradient-color", "FG (t)"), NULL },
    { GIMP_GRADIENT_COLOR_BACKGROUND, NC_("gradient-color", "Background color"), NULL },
    /* Translators: this is an abbreviated version of "Background color".
       Keep it short. */
    { GIMP_GRADIENT_COLOR_BACKGROUND, NC_("gradient-color", "BG"), NULL },
    { GIMP_GRADIENT_COLOR_BACKGROUND_TRANSPARENT, NC_("gradient-color", "Background color (transparent)"), NULL },
    /* Translators: this is an abbreviated version of "Background color (transparent)".
       Keep it short. */
    { GIMP_GRADIENT_COLOR_BACKGROUND_TRANSPARENT, NC_("gradient-color", "BG (t)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpGradientColor", values);
      gimp_type_set_translation_context (type, "gradient-color");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_message_severity_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_MESSAGE_INFO, "GIMP_MESSAGE_INFO", "info" },
    { GIMP_MESSAGE_WARNING, "GIMP_MESSAGE_WARNING", "warning" },
    { GIMP_MESSAGE_ERROR, "GIMP_MESSAGE_ERROR", "error" },
    { GIMP_MESSAGE_BUG_WARNING, "GIMP_MESSAGE_BUG_WARNING", "bug-warning" },
    { GIMP_MESSAGE_BUG_CRITICAL, "GIMP_MESSAGE_BUG_CRITICAL", "bug-critical" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_MESSAGE_INFO, NC_("message-severity", "Message"), NULL },
    { GIMP_MESSAGE_WARNING, NC_("message-severity", "Warning"), NULL },
    { GIMP_MESSAGE_ERROR, NC_("message-severity", "Error"), NULL },
    { GIMP_MESSAGE_BUG_WARNING, NC_("message-severity", "WARNING"), NULL },
    { GIMP_MESSAGE_BUG_CRITICAL, NC_("message-severity", "CRITICAL"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpMessageSeverity", values);
      gimp_type_set_translation_context (type, "message-severity");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_color_profile_policy_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_COLOR_PROFILE_POLICY_ASK, "GIMP_COLOR_PROFILE_POLICY_ASK", "ask" },
    { GIMP_COLOR_PROFILE_POLICY_KEEP, "GIMP_COLOR_PROFILE_POLICY_KEEP", "keep" },
    { GIMP_COLOR_PROFILE_POLICY_CONVERT, "GIMP_COLOR_PROFILE_POLICY_CONVERT", "convert" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_COLOR_PROFILE_POLICY_ASK, NC_("color-profile-policy", "Ask what to do"), NULL },
    { GIMP_COLOR_PROFILE_POLICY_KEEP, NC_("color-profile-policy", "Keep embedded profile"), NULL },
    { GIMP_COLOR_PROFILE_POLICY_CONVERT, NC_("color-profile-policy", "Convert to preferred RGB color profile"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpColorProfilePolicy", values);
      gimp_type_set_translation_context (type, "color-profile-policy");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_dynamics_output_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_DYNAMICS_OUTPUT_OPACITY, "GIMP_DYNAMICS_OUTPUT_OPACITY", "opacity" },
    { GIMP_DYNAMICS_OUTPUT_SIZE, "GIMP_DYNAMICS_OUTPUT_SIZE", "size" },
    { GIMP_DYNAMICS_OUTPUT_ANGLE, "GIMP_DYNAMICS_OUTPUT_ANGLE", "angle" },
    { GIMP_DYNAMICS_OUTPUT_COLOR, "GIMP_DYNAMICS_OUTPUT_COLOR", "color" },
    { GIMP_DYNAMICS_OUTPUT_HARDNESS, "GIMP_DYNAMICS_OUTPUT_HARDNESS", "hardness" },
    { GIMP_DYNAMICS_OUTPUT_FORCE, "GIMP_DYNAMICS_OUTPUT_FORCE", "force" },
    { GIMP_DYNAMICS_OUTPUT_ASPECT_RATIO, "GIMP_DYNAMICS_OUTPUT_ASPECT_RATIO", "aspect-ratio" },
    { GIMP_DYNAMICS_OUTPUT_SPACING, "GIMP_DYNAMICS_OUTPUT_SPACING", "spacing" },
    { GIMP_DYNAMICS_OUTPUT_RATE, "GIMP_DYNAMICS_OUTPUT_RATE", "rate" },
    { GIMP_DYNAMICS_OUTPUT_FLOW, "GIMP_DYNAMICS_OUTPUT_FLOW", "flow" },
    { GIMP_DYNAMICS_OUTPUT_JITTER, "GIMP_DYNAMICS_OUTPUT_JITTER", "jitter" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_DYNAMICS_OUTPUT_OPACITY, NC_("dynamics-output-type", "Opacity"), NULL },
    { GIMP_DYNAMICS_OUTPUT_SIZE, NC_("dynamics-output-type", "Size"), NULL },
    { GIMP_DYNAMICS_OUTPUT_ANGLE, NC_("dynamics-output-type", "Angle"), NULL },
    { GIMP_DYNAMICS_OUTPUT_COLOR, NC_("dynamics-output-type", "Color"), NULL },
    { GIMP_DYNAMICS_OUTPUT_HARDNESS, NC_("dynamics-output-type", "Hardness"), NULL },
    { GIMP_DYNAMICS_OUTPUT_FORCE, NC_("dynamics-output-type", "Force"), NULL },
    { GIMP_DYNAMICS_OUTPUT_ASPECT_RATIO, NC_("dynamics-output-type", "Aspect ratio"), NULL },
    { GIMP_DYNAMICS_OUTPUT_SPACING, NC_("dynamics-output-type", "Spacing"), NULL },
    { GIMP_DYNAMICS_OUTPUT_RATE, NC_("dynamics-output-type", "Rate"), NULL },
    { GIMP_DYNAMICS_OUTPUT_FLOW, NC_("dynamics-output-type", "Flow"), NULL },
    { GIMP_DYNAMICS_OUTPUT_JITTER, NC_("dynamics-output-type", "Jitter"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpDynamicsOutputType", values);
      gimp_type_set_translation_context (type, "dynamics-output-type");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_filter_region_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_FILTER_REGION_SELECTION, "GIMP_FILTER_REGION_SELECTION", "selection" },
    { GIMP_FILTER_REGION_DRAWABLE, "GIMP_FILTER_REGION_DRAWABLE", "drawable" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_FILTER_REGION_SELECTION, NC_("filter-region", "Use the selection as input"), NULL },
    { GIMP_FILTER_REGION_DRAWABLE, NC_("filter-region", "Use the entire layer as input"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpFilterRegion", values);
      gimp_type_set_translation_context (type, "filter-region");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_channel_border_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_CHANNEL_BORDER_STYLE_HARD, "GIMP_CHANNEL_BORDER_STYLE_HARD", "hard" },
    { GIMP_CHANNEL_BORDER_STYLE_SMOOTH, "GIMP_CHANNEL_BORDER_STYLE_SMOOTH", "smooth" },
    { GIMP_CHANNEL_BORDER_STYLE_FEATHERED, "GIMP_CHANNEL_BORDER_STYLE_FEATHERED", "feathered" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_CHANNEL_BORDER_STYLE_HARD, NC_("channel-border-style", "Hard"), NULL },
    { GIMP_CHANNEL_BORDER_STYLE_SMOOTH, NC_("channel-border-style", "Smooth"), NULL },
    { GIMP_CHANNEL_BORDER_STYLE_FEATHERED, NC_("channel-border-style", "Feathered"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpChannelBorderStyle", values);
      gimp_type_set_translation_context (type, "channel-border-style");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

