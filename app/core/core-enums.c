
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "libgimpbase/gimpbase.h"
#include "core-enums.h"
#include "gimp-intl.h"

/* enumerations from "./core-enums.h" */
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
    { GIMP_NO_DITHER, "GIMP_NO_DITHER", "no-dither" },
    { GIMP_FS_DITHER, "GIMP_FS_DITHER", "fs-dither" },
    { GIMP_FSLOWBLEED_DITHER, "GIMP_FSLOWBLEED_DITHER", "fslowbleed-dither" },
    { GIMP_FIXED_DITHER, "GIMP_FIXED_DITHER", "fixed-dither" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_NO_DITHER, NC_("convert-dither-type", "None"), NULL },
    { GIMP_FS_DITHER, NC_("convert-dither-type", "Floyd-Steinberg (normal)"), NULL },
    { GIMP_FSLOWBLEED_DITHER, NC_("convert-dither-type", "Floyd-Steinberg (reduced color bleeding)"), NULL },
    { GIMP_FIXED_DITHER, NC_("convert-dither-type", "Positioned"), NULL },
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
    { GIMP_MAKE_PALETTE, NC_("convert-palette-type", "Generate optimum palette"), NULL },
    { GIMP_WEB_PALETTE, NC_("convert-palette-type", "Use web-optimized palette"), NULL },
    { GIMP_MONO_PALETTE, NC_("convert-palette-type", "Use black and white (1-bit) palette"), NULL },
    { GIMP_CUSTOM_PALETTE, NC_("convert-palette-type", "Use custom palette"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpConvertPaletteType", values);
      gimp_type_set_translation_context (type, "convert-palette-type");
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
    { GIMP_FOREGROUND_FILL, NC_("fill-type", "Foreground color"), NULL },
    { GIMP_BACKGROUND_FILL, NC_("fill-type", "Background color"), NULL },
    { GIMP_WHITE_FILL, NC_("fill-type", "White"), NULL },
    { GIMP_TRANSPARENT_FILL, NC_("fill-type", "Transparency"), NULL },
    { GIMP_PATTERN_FILL, NC_("fill-type", "Pattern"), NULL },
    { GIMP_NO_FILL, NC_("fill-type", "None"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpFillType", values);
      gimp_type_set_translation_context (type, "fill-type");
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
    { GIMP_STROKE_METHOD_LIBART, NC_("stroke-method", "Stroke line"), NULL },
    { GIMP_STROKE_METHOD_PAINT_CORE, NC_("stroke-method", "Stroke with a paint tool"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpStrokeMethod", values);
      gimp_type_set_translation_context (type, "stroke-method");
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
      gimp_type_set_translation_context (type, "join-style");
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
      gimp_type_set_translation_context (type, "cap-style");
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
      gimp_type_set_translation_context (type, "brush-generated-shape");
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
      gimp_type_set_translation_context (type, "orientation-type");
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

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpRotationType", values);
      gimp_type_set_translation_context (type, "rotation-type");
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
    { GIMP_UNDO_IMAGE_SIZE, "GIMP_UNDO_IMAGE_SIZE", "image-size" },
    { GIMP_UNDO_IMAGE_RESOLUTION, "GIMP_UNDO_IMAGE_RESOLUTION", "image-resolution" },
    { GIMP_UNDO_IMAGE_GRID, "GIMP_UNDO_IMAGE_GRID", "image-grid" },
    { GIMP_UNDO_IMAGE_COLORMAP, "GIMP_UNDO_IMAGE_COLORMAP", "image-colormap" },
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
    { GIMP_UNDO_LAYER_ADD, "GIMP_UNDO_LAYER_ADD", "layer-add" },
    { GIMP_UNDO_LAYER_REMOVE, "GIMP_UNDO_LAYER_REMOVE", "layer-remove" },
    { GIMP_UNDO_LAYER_MODE, "GIMP_UNDO_LAYER_MODE", "layer-mode" },
    { GIMP_UNDO_LAYER_OPACITY, "GIMP_UNDO_LAYER_OPACITY", "layer-opacity" },
    { GIMP_UNDO_LAYER_LOCK_ALPHA, "GIMP_UNDO_LAYER_LOCK_ALPHA", "layer-lock-alpha" },
    { GIMP_UNDO_GROUP_LAYER_SUSPEND, "GIMP_UNDO_GROUP_LAYER_SUSPEND", "group-layer-suspend" },
    { GIMP_UNDO_GROUP_LAYER_RESUME, "GIMP_UNDO_GROUP_LAYER_RESUME", "group-layer-resume" },
    { GIMP_UNDO_GROUP_LAYER_CONVERT, "GIMP_UNDO_GROUP_LAYER_CONVERT", "group-layer-convert" },
    { GIMP_UNDO_TEXT_LAYER, "GIMP_UNDO_TEXT_LAYER", "text-layer" },
    { GIMP_UNDO_TEXT_LAYER_MODIFIED, "GIMP_UNDO_TEXT_LAYER_MODIFIED", "text-layer-modified" },
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
    { GIMP_UNDO_TRANSFORM, "GIMP_UNDO_TRANSFORM", "transform" },
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
    { GIMP_UNDO_IMAGE_SIZE, NC_("undo-type", "Image size"), NULL },
    { GIMP_UNDO_IMAGE_RESOLUTION, NC_("undo-type", "Image resolution change"), NULL },
    { GIMP_UNDO_IMAGE_GRID, NC_("undo-type", "Grid"), NULL },
    { GIMP_UNDO_IMAGE_COLORMAP, NC_("undo-type", "Change indexed palette"), NULL },
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
    { GIMP_UNDO_LAYER_ADD, NC_("undo-type", "New layer"), NULL },
    { GIMP_UNDO_LAYER_REMOVE, NC_("undo-type", "Delete layer"), NULL },
    { GIMP_UNDO_LAYER_MODE, NC_("undo-type", "Set layer mode"), NULL },
    { GIMP_UNDO_LAYER_OPACITY, NC_("undo-type", "Set layer opacity"), NULL },
    { GIMP_UNDO_LAYER_LOCK_ALPHA, NC_("undo-type", "Lock/Unlock alpha channel"), NULL },
    { GIMP_UNDO_GROUP_LAYER_SUSPEND, NC_("undo-type", "Suspend group layer resize"), NULL },
    { GIMP_UNDO_GROUP_LAYER_RESUME, NC_("undo-type", "Resume group layer resize"), NULL },
    { GIMP_UNDO_GROUP_LAYER_CONVERT, NC_("undo-type", "Convert group layer"), NULL },
    { GIMP_UNDO_TEXT_LAYER, NC_("undo-type", "Text layer"), NULL },
    { GIMP_UNDO_TEXT_LAYER_MODIFIED, NC_("undo-type", "Text layer modification"), NULL },
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
    { GIMP_UNDO_TRANSFORM, NC_("undo-type", "Transform"), NULL },
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
gimp_offset_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_OFFSET_BACKGROUND, "GIMP_OFFSET_BACKGROUND", "background" },
    { GIMP_OFFSET_TRANSPARENT, "GIMP_OFFSET_TRANSPARENT", "transparent" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_OFFSET_BACKGROUND, "GIMP_OFFSET_BACKGROUND", NULL },
    { GIMP_OFFSET_TRANSPARENT, "GIMP_OFFSET_TRANSPARENT", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpOffsetType", values);
      gimp_type_set_translation_context (type, "offset-type");
      gimp_enum_set_value_descriptions (type, descs);
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
    { GIMP_GRADIENT_COLOR_FIXED, "GIMP_GRADIENT_COLOR_FIXED", NULL },
    { GIMP_GRADIENT_COLOR_FOREGROUND, "GIMP_GRADIENT_COLOR_FOREGROUND", NULL },
    { GIMP_GRADIENT_COLOR_FOREGROUND_TRANSPARENT, "GIMP_GRADIENT_COLOR_FOREGROUND_TRANSPARENT", NULL },
    { GIMP_GRADIENT_COLOR_BACKGROUND, "GIMP_GRADIENT_COLOR_BACKGROUND", NULL },
    { GIMP_GRADIENT_COLOR_BACKGROUND_TRANSPARENT, "GIMP_GRADIENT_COLOR_BACKGROUND_TRANSPARENT", NULL },
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
gimp_gradient_segment_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_GRADIENT_SEGMENT_LINEAR, "GIMP_GRADIENT_SEGMENT_LINEAR", "linear" },
    { GIMP_GRADIENT_SEGMENT_CURVED, "GIMP_GRADIENT_SEGMENT_CURVED", "curved" },
    { GIMP_GRADIENT_SEGMENT_SINE, "GIMP_GRADIENT_SEGMENT_SINE", "sine" },
    { GIMP_GRADIENT_SEGMENT_SPHERE_INCREASING, "GIMP_GRADIENT_SEGMENT_SPHERE_INCREASING", "sphere-increasing" },
    { GIMP_GRADIENT_SEGMENT_SPHERE_DECREASING, "GIMP_GRADIENT_SEGMENT_SPHERE_DECREASING", "sphere-decreasing" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_GRADIENT_SEGMENT_LINEAR, "GIMP_GRADIENT_SEGMENT_LINEAR", NULL },
    { GIMP_GRADIENT_SEGMENT_CURVED, "GIMP_GRADIENT_SEGMENT_CURVED", NULL },
    { GIMP_GRADIENT_SEGMENT_SINE, "GIMP_GRADIENT_SEGMENT_SINE", NULL },
    { GIMP_GRADIENT_SEGMENT_SPHERE_INCREASING, "GIMP_GRADIENT_SEGMENT_SPHERE_INCREASING", NULL },
    { GIMP_GRADIENT_SEGMENT_SPHERE_DECREASING, "GIMP_GRADIENT_SEGMENT_SPHERE_DECREASING", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpGradientSegmentType", values);
      gimp_type_set_translation_context (type, "gradient-segment-type");
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
    { GIMP_GRADIENT_SEGMENT_RGB, "GIMP_GRADIENT_SEGMENT_RGB", NULL },
    { GIMP_GRADIENT_SEGMENT_HSV_CCW, "GIMP_GRADIENT_SEGMENT_HSV_CCW", NULL },
    { GIMP_GRADIENT_SEGMENT_HSV_CW, "GIMP_GRADIENT_SEGMENT_HSV_CW", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpGradientSegmentColor", values);
      gimp_type_set_translation_context (type, "gradient-segment-color");
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
    { GIMP_EXPAND_AS_NECESSARY, "GIMP_EXPAND_AS_NECESSARY", NULL },
    { GIMP_CLIP_TO_IMAGE, "GIMP_CLIP_TO_IMAGE", NULL },
    { GIMP_CLIP_TO_BOTTOM_LAYER, "GIMP_CLIP_TO_BOTTOM_LAYER", NULL },
    { GIMP_FLATTEN_IMAGE, "GIMP_FLATTEN_IMAGE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpMergeType", values);
      gimp_type_set_translation_context (type, "merge-type");
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
    { GIMP_SELECT_CRITERION_R, "GIMP_SELECT_CRITERION_R", "r" },
    { GIMP_SELECT_CRITERION_G, "GIMP_SELECT_CRITERION_G", "g" },
    { GIMP_SELECT_CRITERION_B, "GIMP_SELECT_CRITERION_B", "b" },
    { GIMP_SELECT_CRITERION_H, "GIMP_SELECT_CRITERION_H", "h" },
    { GIMP_SELECT_CRITERION_S, "GIMP_SELECT_CRITERION_S", "s" },
    { GIMP_SELECT_CRITERION_V, "GIMP_SELECT_CRITERION_V", "v" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_SELECT_CRITERION_COMPOSITE, NC_("select-criterion", "Composite"), NULL },
    { GIMP_SELECT_CRITERION_R, NC_("select-criterion", "Red"), NULL },
    { GIMP_SELECT_CRITERION_G, NC_("select-criterion", "Green"), NULL },
    { GIMP_SELECT_CRITERION_B, NC_("select-criterion", "Blue"), NULL },
    { GIMP_SELECT_CRITERION_H, NC_("select-criterion", "Hue"), NULL },
    { GIMP_SELECT_CRITERION_S, NC_("select-criterion", "Saturation"), NULL },
    { GIMP_SELECT_CRITERION_V, NC_("select-criterion", "Value"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpSelectCriterion", values);
      gimp_type_set_translation_context (type, "select-criterion");
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
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_MESSAGE_INFO, NC_("message-severity", "Message"), NULL },
    { GIMP_MESSAGE_WARNING, NC_("message-severity", "Warning"), NULL },
    { GIMP_MESSAGE_ERROR, NC_("message-severity", "Error"), NULL },
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
    { GIMP_COLOR_PROFILE_POLICY_CONVERT, NC_("color-profile-policy", "Convert to RGB workspace"), NULL },
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


/* Generated data ends here */

