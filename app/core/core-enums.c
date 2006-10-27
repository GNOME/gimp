
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

  if (! type)
    {
      type = g_enum_register_static ("GimpAlignmentType", values);
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
    { GIMP_ALIGN_REFERENCE_FIRST, N_("First item"), NULL },
    { GIMP_ALIGN_REFERENCE_IMAGE, N_("Image"), NULL },
    { GIMP_ALIGN_REFERENCE_SELECTION, N_("Selection"), NULL },
    { GIMP_ALIGN_REFERENCE_ACTIVE_LAYER, N_("Active layer"), NULL },
    { GIMP_ALIGN_REFERENCE_ACTIVE_CHANNEL, N_("Active channel"), NULL },
    { GIMP_ALIGN_REFERENCE_ACTIVE_PATH, N_("Active path"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpAlignReferenceType", values);
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
    { GIMP_STROKE_STYLE_SOLID, N_("Solid color"), NULL },
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
    { GIMP_DASH_DASH_DOT, N_("Dash, dot"), NULL },
    { GIMP_DASH_DASH_DOT_DOT, N_("Dash, dot, dot"), NULL },
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
    { GIMP_ITEM_SET_NONE, N_("None"), NULL },
    { GIMP_ITEM_SET_ALL, N_("All layers"), NULL },
    { GIMP_ITEM_SET_IMAGE_SIZED, N_("Image-sized layers"), NULL },
    { GIMP_ITEM_SET_VISIBLE, N_("All visible layers"), NULL },
    { GIMP_ITEM_SET_LINKED, N_("All linked layers"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpItemSet", values);
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
    { GIMP_VIEW_TYPE_LIST, N_("View as list"), NULL },
    { GIMP_VIEW_TYPE_GRID, N_("View as grid"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpViewType", values);
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
    { GIMP_UNDO_GROUP_IMAGE_QUICK_MASK, "GIMP_UNDO_GROUP_IMAGE_QUICK_MASK", "group-image-quick-mask" },
    { GIMP_UNDO_GROUP_IMAGE_GUIDE, "GIMP_UNDO_GROUP_IMAGE_GUIDE", "group-image-guide" },
    { GIMP_UNDO_GROUP_IMAGE_GRID, "GIMP_UNDO_GROUP_IMAGE_GRID", "group-image-grid" },
    { GIMP_UNDO_GROUP_IMAGE_SAMPLE_POINT, "GIMP_UNDO_GROUP_IMAGE_SAMPLE_POINT", "group-image-sample-point" },
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
    { GIMP_UNDO_IMAGE_GUIDE, "GIMP_UNDO_IMAGE_GUIDE", "image-guide" },
    { GIMP_UNDO_IMAGE_GRID, "GIMP_UNDO_IMAGE_GRID", "image-grid" },
    { GIMP_UNDO_IMAGE_SAMPLE_POINT, "GIMP_UNDO_IMAGE_SAMPLE_POINT", "image-sample-point" },
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
    { GIMP_UNDO_LAYER_REPOSITION, "GIMP_UNDO_LAYER_REPOSITION", "layer-reposition" },
    { GIMP_UNDO_LAYER_MODE, "GIMP_UNDO_LAYER_MODE", "layer-mode" },
    { GIMP_UNDO_LAYER_OPACITY, "GIMP_UNDO_LAYER_OPACITY", "layer-opacity" },
    { GIMP_UNDO_LAYER_LOCK_ALPHA, "GIMP_UNDO_LAYER_LOCK_ALPHA", "layer-lock-alpha" },
    { GIMP_UNDO_TEXT_LAYER, "GIMP_UNDO_TEXT_LAYER", "text-layer" },
    { GIMP_UNDO_TEXT_LAYER_MODIFIED, "GIMP_UNDO_TEXT_LAYER_MODIFIED", "text-layer-modified" },
    { GIMP_UNDO_LAYER_MASK_ADD, "GIMP_UNDO_LAYER_MASK_ADD", "layer-mask-add" },
    { GIMP_UNDO_LAYER_MASK_REMOVE, "GIMP_UNDO_LAYER_MASK_REMOVE", "layer-mask-remove" },
    { GIMP_UNDO_LAYER_MASK_APPLY, "GIMP_UNDO_LAYER_MASK_APPLY", "layer-mask-apply" },
    { GIMP_UNDO_LAYER_MASK_SHOW, "GIMP_UNDO_LAYER_MASK_SHOW", "layer-mask-show" },
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
    { GIMP_UNDO_FOREGROUND_SELECT, "GIMP_UNDO_FOREGROUND_SELECT", "foreground-select" },
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
    { GIMP_UNDO_GROUP_IMAGE_VECTORS_MERGE, N_("Merge paths"), NULL },
    { GIMP_UNDO_GROUP_IMAGE_QUICK_MASK, N_("Quick Mask"), NULL },
    { GIMP_UNDO_GROUP_IMAGE_GUIDE, N_("Guide"), NULL },
    { GIMP_UNDO_GROUP_IMAGE_GRID, N_("Grid"), NULL },
    { GIMP_UNDO_GROUP_IMAGE_SAMPLE_POINT, N_("Sample Point"), NULL },
    { GIMP_UNDO_GROUP_DRAWABLE, N_("Layer/Channel"), NULL },
    { GIMP_UNDO_GROUP_DRAWABLE_MOD, N_("Layer/Channel modification"), NULL },
    { GIMP_UNDO_GROUP_MASK, N_("Selection mask"), NULL },
    { GIMP_UNDO_GROUP_ITEM_VISIBILITY, N_("Item visibility"), NULL },
    { GIMP_UNDO_GROUP_ITEM_LINKED, N_("Link/Unlink item"), NULL },
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
    { GIMP_UNDO_IMAGE_RESOLUTION, N_("Image resolution change"), NULL },
    { GIMP_UNDO_IMAGE_GUIDE, N_("Guide"), NULL },
    { GIMP_UNDO_IMAGE_GRID, N_("Grid"), NULL },
    { GIMP_UNDO_IMAGE_SAMPLE_POINT, N_("Sample Point"), NULL },
    { GIMP_UNDO_IMAGE_COLORMAP, N_("Change indexed palette"), NULL },
    { GIMP_UNDO_DRAWABLE, N_("Layer/Channel"), NULL },
    { GIMP_UNDO_DRAWABLE_MOD, N_("Layer/Channel modification"), NULL },
    { GIMP_UNDO_MASK, N_("Selection mask"), NULL },
    { GIMP_UNDO_ITEM_RENAME, N_("Rename item"), NULL },
    { GIMP_UNDO_ITEM_DISPLACE, N_("Move item"), NULL },
    { GIMP_UNDO_ITEM_VISIBILITY, N_("Item visibility"), NULL },
    { GIMP_UNDO_ITEM_LINKED, N_("Link/Unlink item"), NULL },
    { GIMP_UNDO_LAYER_ADD, N_("New layer"), NULL },
    { GIMP_UNDO_LAYER_REMOVE, N_("Delete layer"), NULL },
    { GIMP_UNDO_LAYER_REPOSITION, N_("Reposition layer"), NULL },
    { GIMP_UNDO_LAYER_MODE, N_("Set layer mode"), NULL },
    { GIMP_UNDO_LAYER_OPACITY, N_("Set layer opacity"), NULL },
    { GIMP_UNDO_LAYER_LOCK_ALPHA, N_("Lock/Unlock alpha channel"), NULL },
    { GIMP_UNDO_TEXT_LAYER, N_("Text layer"), NULL },
    { GIMP_UNDO_TEXT_LAYER_MODIFIED, N_("Text layer modification"), NULL },
    { GIMP_UNDO_LAYER_MASK_ADD, N_("Add layer mask"), NULL },
    { GIMP_UNDO_LAYER_MASK_REMOVE, N_("Delete layer mask"), NULL },
    { GIMP_UNDO_LAYER_MASK_APPLY, N_("Apply layer mask"), NULL },
    { GIMP_UNDO_LAYER_MASK_SHOW, N_("Show layer mask"), NULL },
    { GIMP_UNDO_CHANNEL_ADD, N_("New channel"), NULL },
    { GIMP_UNDO_CHANNEL_REMOVE, N_("Delete channel"), NULL },
    { GIMP_UNDO_CHANNEL_REPOSITION, N_("Reposition channel"), NULL },
    { GIMP_UNDO_CHANNEL_COLOR, N_("Channel color"), NULL },
    { GIMP_UNDO_VECTORS_ADD, N_("New path"), NULL },
    { GIMP_UNDO_VECTORS_REMOVE, N_("Delete path"), NULL },
    { GIMP_UNDO_VECTORS_MOD, N_("Path modification"), NULL },
    { GIMP_UNDO_VECTORS_REPOSITION, N_("Reposition path"), NULL },
    { GIMP_UNDO_FS_TO_LAYER, N_("Floating selection to layer"), NULL },
    { GIMP_UNDO_FS_RIGOR, N_("FS rigor"), NULL },
    { GIMP_UNDO_FS_RELAX, N_("FS relax"), NULL },
    { GIMP_UNDO_TRANSFORM, N_("Transform"), NULL },
    { GIMP_UNDO_PAINT, N_("Paint"), NULL },
    { GIMP_UNDO_INK, N_("Ink"), NULL },
    { GIMP_UNDO_FOREGROUND_SELECT, N_("Select foreground"), NULL },
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

  if (! type)
    {
      type = g_enum_register_static ("GimpOffsetType", values);
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

  if (! type)
    {
      type = g_enum_register_static ("GimpGradientColor", values);
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

  if (! type)
    {
      type = g_enum_register_static ("GimpGradientSegmentType", values);
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

  if (! type)
    {
      type = g_enum_register_static ("GimpGradientSegmentColor", values);
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

  if (! type)
    {
      type = g_enum_register_static ("GimpMaskApplyMode", values);
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

  if (! type)
    {
      type = g_enum_register_static ("GimpMergeType", values);
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
    { GIMP_SELECT_CRITERION_COMPOSITE, N_("Composite"), NULL },
    { GIMP_SELECT_CRITERION_R, N_("Red"), NULL },
    { GIMP_SELECT_CRITERION_G, N_("Green"), NULL },
    { GIMP_SELECT_CRITERION_B, N_("Blue"), NULL },
    { GIMP_SELECT_CRITERION_H, N_("Hue"), NULL },
    { GIMP_SELECT_CRITERION_S, N_("Saturation"), NULL },
    { GIMP_SELECT_CRITERION_V, N_("Value"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpSelectCriterion", values);
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
    { GIMP_MESSAGE_INFO, N_("Message"), NULL },
    { GIMP_MESSAGE_WARNING, N_("Warning"), NULL },
    { GIMP_MESSAGE_ERROR, N_("Error"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpMessageSeverity", values);
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
    { GIMP_COLOR_PROFILE_POLICY_ASK, N_("Ask"), NULL },
    { GIMP_COLOR_PROFILE_POLICY_KEEP, N_("Keep embedded profile"), NULL },
    { GIMP_COLOR_PROFILE_POLICY_CONVERT, N_("Convert to RGB workspace"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpColorProfilePolicy", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

