
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <gio/gio.h>
#include "libgimpbase/gimpbase.h"
#include "core/core-enums.h"
#include "tools-enums.h"
#include "gimp-intl.h"

/* enumerations from "tools-enums.h" */
GType
gimp_rectangle_constraint_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_RECTANGLE_CONSTRAIN_NONE, "GIMP_RECTANGLE_CONSTRAIN_NONE", "none" },
    { GIMP_RECTANGLE_CONSTRAIN_IMAGE, "GIMP_RECTANGLE_CONSTRAIN_IMAGE", "image" },
    { GIMP_RECTANGLE_CONSTRAIN_DRAWABLE, "GIMP_RECTANGLE_CONSTRAIN_DRAWABLE", "drawable" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_RECTANGLE_CONSTRAIN_NONE, "GIMP_RECTANGLE_CONSTRAIN_NONE", NULL },
    { GIMP_RECTANGLE_CONSTRAIN_IMAGE, "GIMP_RECTANGLE_CONSTRAIN_IMAGE", NULL },
    { GIMP_RECTANGLE_CONSTRAIN_DRAWABLE, "GIMP_RECTANGLE_CONSTRAIN_DRAWABLE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpRectangleConstraint", values);
      gimp_type_set_translation_context (type, "rectangle-constraint");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_rectangle_precision_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_RECTANGLE_PRECISION_INT, "GIMP_RECTANGLE_PRECISION_INT", "int" },
    { GIMP_RECTANGLE_PRECISION_DOUBLE, "GIMP_RECTANGLE_PRECISION_DOUBLE", "double" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_RECTANGLE_PRECISION_INT, "GIMP_RECTANGLE_PRECISION_INT", NULL },
    { GIMP_RECTANGLE_PRECISION_DOUBLE, "GIMP_RECTANGLE_PRECISION_DOUBLE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpRectanglePrecision", values);
      gimp_type_set_translation_context (type, "rectangle-precision");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_rectangle_tool_fixed_rule_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_RECTANGLE_TOOL_FIXED_ASPECT, "GIMP_RECTANGLE_TOOL_FIXED_ASPECT", "aspect" },
    { GIMP_RECTANGLE_TOOL_FIXED_WIDTH, "GIMP_RECTANGLE_TOOL_FIXED_WIDTH", "width" },
    { GIMP_RECTANGLE_TOOL_FIXED_HEIGHT, "GIMP_RECTANGLE_TOOL_FIXED_HEIGHT", "height" },
    { GIMP_RECTANGLE_TOOL_FIXED_SIZE, "GIMP_RECTANGLE_TOOL_FIXED_SIZE", "size" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_RECTANGLE_TOOL_FIXED_ASPECT, NC_("rectangle-tool-fixed-rule", "Aspect ratio"), NULL },
    { GIMP_RECTANGLE_TOOL_FIXED_WIDTH, NC_("rectangle-tool-fixed-rule", "Width"), NULL },
    { GIMP_RECTANGLE_TOOL_FIXED_HEIGHT, NC_("rectangle-tool-fixed-rule", "Height"), NULL },
    { GIMP_RECTANGLE_TOOL_FIXED_SIZE, NC_("rectangle-tool-fixed-rule", "Size"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpRectangleToolFixedRule", values);
      gimp_type_set_translation_context (type, "rectangle-tool-fixed-rule");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_rect_select_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_RECT_SELECT_MODE_FREE, "GIMP_RECT_SELECT_MODE_FREE", "free" },
    { GIMP_RECT_SELECT_MODE_FIXED_SIZE, "GIMP_RECT_SELECT_MODE_FIXED_SIZE", "fixed-size" },
    { GIMP_RECT_SELECT_MODE_FIXED_RATIO, "GIMP_RECT_SELECT_MODE_FIXED_RATIO", "fixed-ratio" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_RECT_SELECT_MODE_FREE, NC_("rect-select-mode", "Free select"), NULL },
    { GIMP_RECT_SELECT_MODE_FIXED_SIZE, NC_("rect-select-mode", "Fixed size"), NULL },
    { GIMP_RECT_SELECT_MODE_FIXED_RATIO, NC_("rect-select-mode", "Fixed aspect ratio"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpRectSelectMode", values);
      gimp_type_set_translation_context (type, "rect-select-mode");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_transform_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_TRANSFORM_TYPE_LAYER, "GIMP_TRANSFORM_TYPE_LAYER", "layer" },
    { GIMP_TRANSFORM_TYPE_SELECTION, "GIMP_TRANSFORM_TYPE_SELECTION", "selection" },
    { GIMP_TRANSFORM_TYPE_PATH, "GIMP_TRANSFORM_TYPE_PATH", "path" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_TRANSFORM_TYPE_LAYER, NC_("transform-type", "Layer"), NULL },
    { GIMP_TRANSFORM_TYPE_SELECTION, NC_("transform-type", "Selection"), NULL },
    { GIMP_TRANSFORM_TYPE_PATH, NC_("transform-type", "Path"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpTransformType", values);
      gimp_type_set_translation_context (type, "transform-type");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_tool_action_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_TOOL_ACTION_PAUSE, "GIMP_TOOL_ACTION_PAUSE", "pause" },
    { GIMP_TOOL_ACTION_RESUME, "GIMP_TOOL_ACTION_RESUME", "resume" },
    { GIMP_TOOL_ACTION_HALT, "GIMP_TOOL_ACTION_HALT", "halt" },
    { GIMP_TOOL_ACTION_COMMIT, "GIMP_TOOL_ACTION_COMMIT", "commit" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_TOOL_ACTION_PAUSE, "GIMP_TOOL_ACTION_PAUSE", NULL },
    { GIMP_TOOL_ACTION_RESUME, "GIMP_TOOL_ACTION_RESUME", NULL },
    { GIMP_TOOL_ACTION_HALT, "GIMP_TOOL_ACTION_HALT", NULL },
    { GIMP_TOOL_ACTION_COMMIT, "GIMP_TOOL_ACTION_COMMIT", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpToolAction", values);
      gimp_type_set_translation_context (type, "tool-action");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_matting_draw_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_MATTING_DRAW_MODE_FOREGROUND, "GIMP_MATTING_DRAW_MODE_FOREGROUND", "foreground" },
    { GIMP_MATTING_DRAW_MODE_BACKGROUND, "GIMP_MATTING_DRAW_MODE_BACKGROUND", "background" },
    { GIMP_MATTING_DRAW_MODE_UNKNOWN, "GIMP_MATTING_DRAW_MODE_UNKNOWN", "unknown" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_MATTING_DRAW_MODE_FOREGROUND, NC_("matting-draw-mode", "Draw foreground"), NULL },
    { GIMP_MATTING_DRAW_MODE_BACKGROUND, NC_("matting-draw-mode", "Draw background"), NULL },
    { GIMP_MATTING_DRAW_MODE_UNKNOWN, NC_("matting-draw-mode", "Draw unknown"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpMattingDrawMode", values);
      gimp_type_set_translation_context (type, "matting-draw-mode");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_warp_behavior_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_WARP_BEHAVIOR_MOVE, "GIMP_WARP_BEHAVIOR_MOVE", "imp-warp-behavior-move" },
    { GEGL_WARP_BEHAVIOR_GROW, "GEGL_WARP_BEHAVIOR_GROW", "egl-warp-behavior-grow" },
    { GEGL_WARP_BEHAVIOR_SHRINK, "GEGL_WARP_BEHAVIOR_SHRINK", "egl-warp-behavior-shrink" },
    { GEGL_WARP_BEHAVIOR_SWIRL_CW, "GEGL_WARP_BEHAVIOR_SWIRL_CW", "egl-warp-behavior-swirl-cw" },
    { GEGL_WARP_BEHAVIOR_SWIRL_CCW, "GEGL_WARP_BEHAVIOR_SWIRL_CCW", "egl-warp-behavior-swirl-ccw" },
    { GEGL_WARP_BEHAVIOR_ERASE, "GEGL_WARP_BEHAVIOR_ERASE", "egl-warp-behavior-erase" },
    { GEGL_WARP_BEHAVIOR_SMOOTH, "GEGL_WARP_BEHAVIOR_SMOOTH", "egl-warp-behavior-smooth" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_WARP_BEHAVIOR_MOVE, NC_("warp-behavior", "Move pixels"), NULL },
    { GEGL_WARP_BEHAVIOR_GROW, NC_("warp-behavior", "Grow area"), NULL },
    { GEGL_WARP_BEHAVIOR_SHRINK, NC_("warp-behavior", "Shrink area"), NULL },
    { GEGL_WARP_BEHAVIOR_SWIRL_CW, NC_("warp-behavior", "Swirl clockwise"), NULL },
    { GEGL_WARP_BEHAVIOR_SWIRL_CCW, NC_("warp-behavior", "Swirl counter-clockwise"), NULL },
    { GEGL_WARP_BEHAVIOR_ERASE, NC_("warp-behavior", "Erase warping"), NULL },
    { GEGL_WARP_BEHAVIOR_SMOOTH, NC_("warp-behavior", "Smooth warping"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpWarpBehavior", values);
      gimp_type_set_translation_context (type, "warp-behavior");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

