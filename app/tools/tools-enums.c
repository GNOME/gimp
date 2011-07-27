
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "libgimpbase/gimpbase.h"
#include "core/core-enums.h"
#include "tools-enums.h"
#include "gimp-intl.h"

/* enumerations from "./tools-enums.h" */
GType
gimp_button_press_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_BUTTON_PRESS_NORMAL, "GIMP_BUTTON_PRESS_NORMAL", "normal" },
    { GIMP_BUTTON_PRESS_DOUBLE, "GIMP_BUTTON_PRESS_DOUBLE", "double" },
    { GIMP_BUTTON_PRESS_TRIPLE, "GIMP_BUTTON_PRESS_TRIPLE", "triple" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_BUTTON_PRESS_NORMAL, "GIMP_BUTTON_PRESS_NORMAL", NULL },
    { GIMP_BUTTON_PRESS_DOUBLE, "GIMP_BUTTON_PRESS_DOUBLE", NULL },
    { GIMP_BUTTON_PRESS_TRIPLE, "GIMP_BUTTON_PRESS_TRIPLE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpButtonPressType", values);
      gimp_type_set_translation_context (type, "button-press-type");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_button_release_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_BUTTON_RELEASE_NORMAL, "GIMP_BUTTON_RELEASE_NORMAL", "normal" },
    { GIMP_BUTTON_RELEASE_CANCEL, "GIMP_BUTTON_RELEASE_CANCEL", "cancel" },
    { GIMP_BUTTON_RELEASE_CLICK, "GIMP_BUTTON_RELEASE_CLICK", "click" },
    { GIMP_BUTTON_RELEASE_NO_MOTION, "GIMP_BUTTON_RELEASE_NO_MOTION", "no-motion" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_BUTTON_RELEASE_NORMAL, "GIMP_BUTTON_RELEASE_NORMAL", NULL },
    { GIMP_BUTTON_RELEASE_CANCEL, "GIMP_BUTTON_RELEASE_CANCEL", NULL },
    { GIMP_BUTTON_RELEASE_CLICK, "GIMP_BUTTON_RELEASE_CLICK", NULL },
    { GIMP_BUTTON_RELEASE_NO_MOTION, "GIMP_BUTTON_RELEASE_NO_MOTION", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpButtonReleaseType", values);
      gimp_type_set_translation_context (type, "button-release-type");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

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
gimp_vector_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_VECTOR_MODE_DESIGN, "GIMP_VECTOR_MODE_DESIGN", "design" },
    { GIMP_VECTOR_MODE_EDIT, "GIMP_VECTOR_MODE_EDIT", "edit" },
    { GIMP_VECTOR_MODE_MOVE, "GIMP_VECTOR_MODE_MOVE", "move" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_VECTOR_MODE_DESIGN, NC_("vector-mode", "Design"), NULL },
    { GIMP_VECTOR_MODE_EDIT, NC_("vector-mode", "Edit"), NULL },
    { GIMP_VECTOR_MODE_MOVE, NC_("vector-mode", "Move"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpVectorMode", values);
      gimp_type_set_translation_context (type, "vector-mode");
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
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_TOOL_ACTION_PAUSE, "GIMP_TOOL_ACTION_PAUSE", NULL },
    { GIMP_TOOL_ACTION_RESUME, "GIMP_TOOL_ACTION_RESUME", NULL },
    { GIMP_TOOL_ACTION_HALT, "GIMP_TOOL_ACTION_HALT", NULL },
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


/* Generated data ends here */

