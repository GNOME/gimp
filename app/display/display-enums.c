
/* Generated data (by gimp-mkenums) */

#include "stamp-display-enums.h"
#include "config.h"
#include <gio/gio.h>
#include "libgimpbase/gimpbase.h"
#include "display-enums.h"
#include "gimp-intl.h"

/* enumerations from "display-enums.h" */
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
gimp_compass_orientation_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_COMPASS_ORIENTATION_AUTO, "GIMP_COMPASS_ORIENTATION_AUTO", "auto" },
    { GIMP_COMPASS_ORIENTATION_HORIZONTAL, "GIMP_COMPASS_ORIENTATION_HORIZONTAL", "horizontal" },
    { GIMP_COMPASS_ORIENTATION_VERTICAL, "GIMP_COMPASS_ORIENTATION_VERTICAL", "vertical" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_COMPASS_ORIENTATION_AUTO, NC_("compass-orientation", "Auto"), NULL },
    { GIMP_COMPASS_ORIENTATION_HORIZONTAL, NC_("compass-orientation", "Horizontal"), NULL },
    { GIMP_COMPASS_ORIENTATION_VERTICAL, NC_("compass-orientation", "Vertical"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpCompassOrientation", values);
      gimp_type_set_translation_context (type, "compass-orientation");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_cursor_precision_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_CURSOR_PRECISION_PIXEL_CENTER, "GIMP_CURSOR_PRECISION_PIXEL_CENTER", "pixel-center" },
    { GIMP_CURSOR_PRECISION_PIXEL_BORDER, "GIMP_CURSOR_PRECISION_PIXEL_BORDER", "pixel-border" },
    { GIMP_CURSOR_PRECISION_SUBPIXEL, "GIMP_CURSOR_PRECISION_SUBPIXEL", "subpixel" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_CURSOR_PRECISION_PIXEL_CENTER, "GIMP_CURSOR_PRECISION_PIXEL_CENTER", NULL },
    { GIMP_CURSOR_PRECISION_PIXEL_BORDER, "GIMP_CURSOR_PRECISION_PIXEL_BORDER", NULL },
    { GIMP_CURSOR_PRECISION_SUBPIXEL, "GIMP_CURSOR_PRECISION_SUBPIXEL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpCursorPrecision", values);
      gimp_type_set_translation_context (type, "cursor-precision");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_guides_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_GUIDES_NONE, "GIMP_GUIDES_NONE", "none" },
    { GIMP_GUIDES_CENTER_LINES, "GIMP_GUIDES_CENTER_LINES", "center-lines" },
    { GIMP_GUIDES_THIRDS, "GIMP_GUIDES_THIRDS", "thirds" },
    { GIMP_GUIDES_FIFTHS, "GIMP_GUIDES_FIFTHS", "fifths" },
    { GIMP_GUIDES_GOLDEN, "GIMP_GUIDES_GOLDEN", "golden" },
    { GIMP_GUIDES_DIAGONALS, "GIMP_GUIDES_DIAGONALS", "diagonals" },
    { GIMP_GUIDES_N_LINES, "GIMP_GUIDES_N_LINES", "n-lines" },
    { GIMP_GUIDES_SPACING, "GIMP_GUIDES_SPACING", "spacing" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_GUIDES_NONE, NC_("guides-type", "No guides"), NULL },
    { GIMP_GUIDES_CENTER_LINES, NC_("guides-type", "Center lines"), NULL },
    { GIMP_GUIDES_THIRDS, NC_("guides-type", "Rule of thirds"), NULL },
    { GIMP_GUIDES_FIFTHS, NC_("guides-type", "Rule of fifths"), NULL },
    { GIMP_GUIDES_GOLDEN, NC_("guides-type", "Golden sections"), NULL },
    { GIMP_GUIDES_DIAGONALS, NC_("guides-type", "Diagonal lines"), NULL },
    { GIMP_GUIDES_N_LINES, NC_("guides-type", "Number of lines"), NULL },
    { GIMP_GUIDES_SPACING, NC_("guides-type", "Line spacing"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpGuidesType", values);
      gimp_type_set_translation_context (type, "guides-type");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_handle_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_HANDLE_SQUARE, "GIMP_HANDLE_SQUARE", "square" },
    { GIMP_HANDLE_DASHED_SQUARE, "GIMP_HANDLE_DASHED_SQUARE", "dashed-square" },
    { GIMP_HANDLE_FILLED_SQUARE, "GIMP_HANDLE_FILLED_SQUARE", "filled-square" },
    { GIMP_HANDLE_CIRCLE, "GIMP_HANDLE_CIRCLE", "circle" },
    { GIMP_HANDLE_DASHED_CIRCLE, "GIMP_HANDLE_DASHED_CIRCLE", "dashed-circle" },
    { GIMP_HANDLE_FILLED_CIRCLE, "GIMP_HANDLE_FILLED_CIRCLE", "filled-circle" },
    { GIMP_HANDLE_DIAMOND, "GIMP_HANDLE_DIAMOND", "diamond" },
    { GIMP_HANDLE_DASHED_DIAMOND, "GIMP_HANDLE_DASHED_DIAMOND", "dashed-diamond" },
    { GIMP_HANDLE_FILLED_DIAMOND, "GIMP_HANDLE_FILLED_DIAMOND", "filled-diamond" },
    { GIMP_HANDLE_CROSS, "GIMP_HANDLE_CROSS", "cross" },
    { GIMP_HANDLE_CROSSHAIR, "GIMP_HANDLE_CROSSHAIR", "crosshair" },
    { GIMP_HANDLE_DROP, "GIMP_HANDLE_DROP", "drop" },
    { GIMP_HANDLE_FILLED_DROP, "GIMP_HANDLE_FILLED_DROP", "filled-drop" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_HANDLE_SQUARE, "GIMP_HANDLE_SQUARE", NULL },
    { GIMP_HANDLE_DASHED_SQUARE, "GIMP_HANDLE_DASHED_SQUARE", NULL },
    { GIMP_HANDLE_FILLED_SQUARE, "GIMP_HANDLE_FILLED_SQUARE", NULL },
    { GIMP_HANDLE_CIRCLE, "GIMP_HANDLE_CIRCLE", NULL },
    { GIMP_HANDLE_DASHED_CIRCLE, "GIMP_HANDLE_DASHED_CIRCLE", NULL },
    { GIMP_HANDLE_FILLED_CIRCLE, "GIMP_HANDLE_FILLED_CIRCLE", NULL },
    { GIMP_HANDLE_DIAMOND, "GIMP_HANDLE_DIAMOND", NULL },
    { GIMP_HANDLE_DASHED_DIAMOND, "GIMP_HANDLE_DASHED_DIAMOND", NULL },
    { GIMP_HANDLE_FILLED_DIAMOND, "GIMP_HANDLE_FILLED_DIAMOND", NULL },
    { GIMP_HANDLE_CROSS, "GIMP_HANDLE_CROSS", NULL },
    { GIMP_HANDLE_CROSSHAIR, "GIMP_HANDLE_CROSSHAIR", NULL },
    { GIMP_HANDLE_DROP, "GIMP_HANDLE_DROP", NULL },
    { GIMP_HANDLE_FILLED_DROP, "GIMP_HANDLE_FILLED_DROP", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpHandleType", values);
      gimp_type_set_translation_context (type, "handle-type");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_handle_anchor_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_HANDLE_ANCHOR_CENTER, "GIMP_HANDLE_ANCHOR_CENTER", "center" },
    { GIMP_HANDLE_ANCHOR_NORTH, "GIMP_HANDLE_ANCHOR_NORTH", "north" },
    { GIMP_HANDLE_ANCHOR_NORTH_WEST, "GIMP_HANDLE_ANCHOR_NORTH_WEST", "north-west" },
    { GIMP_HANDLE_ANCHOR_NORTH_EAST, "GIMP_HANDLE_ANCHOR_NORTH_EAST", "north-east" },
    { GIMP_HANDLE_ANCHOR_SOUTH, "GIMP_HANDLE_ANCHOR_SOUTH", "south" },
    { GIMP_HANDLE_ANCHOR_SOUTH_WEST, "GIMP_HANDLE_ANCHOR_SOUTH_WEST", "south-west" },
    { GIMP_HANDLE_ANCHOR_SOUTH_EAST, "GIMP_HANDLE_ANCHOR_SOUTH_EAST", "south-east" },
    { GIMP_HANDLE_ANCHOR_WEST, "GIMP_HANDLE_ANCHOR_WEST", "west" },
    { GIMP_HANDLE_ANCHOR_EAST, "GIMP_HANDLE_ANCHOR_EAST", "east" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_HANDLE_ANCHOR_CENTER, "GIMP_HANDLE_ANCHOR_CENTER", NULL },
    { GIMP_HANDLE_ANCHOR_NORTH, "GIMP_HANDLE_ANCHOR_NORTH", NULL },
    { GIMP_HANDLE_ANCHOR_NORTH_WEST, "GIMP_HANDLE_ANCHOR_NORTH_WEST", NULL },
    { GIMP_HANDLE_ANCHOR_NORTH_EAST, "GIMP_HANDLE_ANCHOR_NORTH_EAST", NULL },
    { GIMP_HANDLE_ANCHOR_SOUTH, "GIMP_HANDLE_ANCHOR_SOUTH", NULL },
    { GIMP_HANDLE_ANCHOR_SOUTH_WEST, "GIMP_HANDLE_ANCHOR_SOUTH_WEST", NULL },
    { GIMP_HANDLE_ANCHOR_SOUTH_EAST, "GIMP_HANDLE_ANCHOR_SOUTH_EAST", NULL },
    { GIMP_HANDLE_ANCHOR_WEST, "GIMP_HANDLE_ANCHOR_WEST", NULL },
    { GIMP_HANDLE_ANCHOR_EAST, "GIMP_HANDLE_ANCHOR_EAST", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpHandleAnchor", values);
      gimp_type_set_translation_context (type, "handle-anchor");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_limit_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_LIMIT_CIRCLE, "GIMP_LIMIT_CIRCLE", "circle" },
    { GIMP_LIMIT_SQUARE, "GIMP_LIMIT_SQUARE", "square" },
    { GIMP_LIMIT_DIAMOND, "GIMP_LIMIT_DIAMOND", "diamond" },
    { GIMP_LIMIT_HORIZONTAL, "GIMP_LIMIT_HORIZONTAL", "horizontal" },
    { GIMP_LIMIT_VERTICAL, "GIMP_LIMIT_VERTICAL", "vertical" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_LIMIT_CIRCLE, "GIMP_LIMIT_CIRCLE", NULL },
    { GIMP_LIMIT_SQUARE, "GIMP_LIMIT_SQUARE", NULL },
    { GIMP_LIMIT_DIAMOND, "GIMP_LIMIT_DIAMOND", NULL },
    { GIMP_LIMIT_HORIZONTAL, "GIMP_LIMIT_HORIZONTAL", NULL },
    { GIMP_LIMIT_VERTICAL, "GIMP_LIMIT_VERTICAL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpLimitType", values);
      gimp_type_set_translation_context (type, "limit-type");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_path_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_PATH_STYLE_DEFAULT, "GIMP_PATH_STYLE_DEFAULT", "default" },
    { GIMP_PATH_STYLE_VECTORS, "GIMP_PATH_STYLE_VECTORS", "vectors" },
    { GIMP_PATH_STYLE_OUTLINE, "GIMP_PATH_STYLE_OUTLINE", "outline" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_PATH_STYLE_DEFAULT, "GIMP_PATH_STYLE_DEFAULT", NULL },
    { GIMP_PATH_STYLE_VECTORS, "GIMP_PATH_STYLE_VECTORS", NULL },
    { GIMP_PATH_STYLE_OUTLINE, "GIMP_PATH_STYLE_OUTLINE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpPathStyle", values);
      gimp_type_set_translation_context (type, "path-style");
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
gimp_rectangle_fixed_rule_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_RECTANGLE_FIXED_ASPECT, "GIMP_RECTANGLE_FIXED_ASPECT", "aspect" },
    { GIMP_RECTANGLE_FIXED_WIDTH, "GIMP_RECTANGLE_FIXED_WIDTH", "width" },
    { GIMP_RECTANGLE_FIXED_HEIGHT, "GIMP_RECTANGLE_FIXED_HEIGHT", "height" },
    { GIMP_RECTANGLE_FIXED_SIZE, "GIMP_RECTANGLE_FIXED_SIZE", "size" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_RECTANGLE_FIXED_ASPECT, NC_("rectangle-fixed-rule", "Aspect ratio"), NULL },
    { GIMP_RECTANGLE_FIXED_WIDTH, NC_("rectangle-fixed-rule", "Width"), NULL },
    { GIMP_RECTANGLE_FIXED_HEIGHT, NC_("rectangle-fixed-rule", "Height"), NULL },
    { GIMP_RECTANGLE_FIXED_SIZE, NC_("rectangle-fixed-rule", "Size"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpRectangleFixedRule", values);
      gimp_type_set_translation_context (type, "rectangle-fixed-rule");
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
gimp_transform_3d_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_TRANSFORM_3D_MODE_CAMERA, "GIMP_TRANSFORM_3D_MODE_CAMERA", "camera" },
    { GIMP_TRANSFORM_3D_MODE_MOVE, "GIMP_TRANSFORM_3D_MODE_MOVE", "move" },
    { GIMP_TRANSFORM_3D_MODE_ROTATE, "GIMP_TRANSFORM_3D_MODE_ROTATE", "rotate" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_TRANSFORM_3D_MODE_CAMERA, "GIMP_TRANSFORM_3D_MODE_CAMERA", NULL },
    { GIMP_TRANSFORM_3D_MODE_MOVE, "GIMP_TRANSFORM_3D_MODE_MOVE", NULL },
    { GIMP_TRANSFORM_3D_MODE_ROTATE, "GIMP_TRANSFORM_3D_MODE_ROTATE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpTransform3DMode", values);
      gimp_type_set_translation_context (type, "transform3-dmode");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_transform_function_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_TRANSFORM_FUNCTION_NONE, "GIMP_TRANSFORM_FUNCTION_NONE", "none" },
    { GIMP_TRANSFORM_FUNCTION_MOVE, "GIMP_TRANSFORM_FUNCTION_MOVE", "move" },
    { GIMP_TRANSFORM_FUNCTION_SCALE, "GIMP_TRANSFORM_FUNCTION_SCALE", "scale" },
    { GIMP_TRANSFORM_FUNCTION_ROTATE, "GIMP_TRANSFORM_FUNCTION_ROTATE", "rotate" },
    { GIMP_TRANSFORM_FUNCTION_SHEAR, "GIMP_TRANSFORM_FUNCTION_SHEAR", "shear" },
    { GIMP_TRANSFORM_FUNCTION_PERSPECTIVE, "GIMP_TRANSFORM_FUNCTION_PERSPECTIVE", "perspective" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_TRANSFORM_FUNCTION_NONE, "GIMP_TRANSFORM_FUNCTION_NONE", NULL },
    { GIMP_TRANSFORM_FUNCTION_MOVE, "GIMP_TRANSFORM_FUNCTION_MOVE", NULL },
    { GIMP_TRANSFORM_FUNCTION_SCALE, "GIMP_TRANSFORM_FUNCTION_SCALE", NULL },
    { GIMP_TRANSFORM_FUNCTION_ROTATE, "GIMP_TRANSFORM_FUNCTION_ROTATE", NULL },
    { GIMP_TRANSFORM_FUNCTION_SHEAR, "GIMP_TRANSFORM_FUNCTION_SHEAR", NULL },
    { GIMP_TRANSFORM_FUNCTION_PERSPECTIVE, "GIMP_TRANSFORM_FUNCTION_PERSPECTIVE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpTransformFunction", values);
      gimp_type_set_translation_context (type, "transform-function");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_transform_handle_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_HANDLE_MODE_ADD_TRANSFORM, "GIMP_HANDLE_MODE_ADD_TRANSFORM", "add-transform" },
    { GIMP_HANDLE_MODE_MOVE, "GIMP_HANDLE_MODE_MOVE", "move" },
    { GIMP_HANDLE_MODE_REMOVE, "GIMP_HANDLE_MODE_REMOVE", "remove" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_HANDLE_MODE_ADD_TRANSFORM, NC_("transform-handle-mode", "Add / Transform"), NULL },
    { GIMP_HANDLE_MODE_MOVE, NC_("transform-handle-mode", "Move"), NULL },
    { GIMP_HANDLE_MODE_REMOVE, NC_("transform-handle-mode", "Remove"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpTransformHandleMode", values);
      gimp_type_set_translation_context (type, "transform-handle-mode");
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
gimp_zoom_focus_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_ZOOM_FOCUS_BEST_GUESS, "GIMP_ZOOM_FOCUS_BEST_GUESS", "best-guess" },
    { GIMP_ZOOM_FOCUS_POINTER, "GIMP_ZOOM_FOCUS_POINTER", "pointer" },
    { GIMP_ZOOM_FOCUS_IMAGE_CENTER, "GIMP_ZOOM_FOCUS_IMAGE_CENTER", "image-center" },
    { GIMP_ZOOM_FOCUS_RETAIN_CENTERING_ELSE_BEST_GUESS, "GIMP_ZOOM_FOCUS_RETAIN_CENTERING_ELSE_BEST_GUESS", "retain-centering-else-best-guess" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_ZOOM_FOCUS_BEST_GUESS, "GIMP_ZOOM_FOCUS_BEST_GUESS", NULL },
    { GIMP_ZOOM_FOCUS_POINTER, "GIMP_ZOOM_FOCUS_POINTER", NULL },
    { GIMP_ZOOM_FOCUS_IMAGE_CENTER, "GIMP_ZOOM_FOCUS_IMAGE_CENTER", NULL },
    { GIMP_ZOOM_FOCUS_RETAIN_CENTERING_ELSE_BEST_GUESS, "GIMP_ZOOM_FOCUS_RETAIN_CENTERING_ELSE_BEST_GUESS", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpZoomFocus", values);
      gimp_type_set_translation_context (type, "zoom-focus");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_modifier_action_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_MODIFIER_ACTION_NONE, "GIMP_MODIFIER_ACTION_NONE", "none" },
    { GIMP_MODIFIER_ACTION_PANNING, "GIMP_MODIFIER_ACTION_PANNING", "panning" },
    { GIMP_MODIFIER_ACTION_ZOOMING, "GIMP_MODIFIER_ACTION_ZOOMING", "zooming" },
    { GIMP_MODIFIER_ACTION_ROTATING, "GIMP_MODIFIER_ACTION_ROTATING", "rotating" },
    { GIMP_MODIFIER_ACTION_STEP_ROTATING, "GIMP_MODIFIER_ACTION_STEP_ROTATING", "step-rotating" },
    { GIMP_MODIFIER_ACTION_LAYER_PICKING, "GIMP_MODIFIER_ACTION_LAYER_PICKING", "layer-picking" },
    { GIMP_MODIFIER_ACTION_MENU, "GIMP_MODIFIER_ACTION_MENU", "menu" },
    { GIMP_MODIFIER_ACTION_ACTION, "GIMP_MODIFIER_ACTION_ACTION", "action" },
    { GIMP_MODIFIER_ACTION_BRUSH_PIXEL_SIZE, "GIMP_MODIFIER_ACTION_BRUSH_PIXEL_SIZE", "brush-pixel-size" },
    { GIMP_MODIFIER_ACTION_BRUSH_RADIUS_PIXEL_SIZE, "GIMP_MODIFIER_ACTION_BRUSH_RADIUS_PIXEL_SIZE", "brush-radius-pixel-size" },
    { GIMP_MODIFIER_ACTION_TOOL_OPACITY, "GIMP_MODIFIER_ACTION_TOOL_OPACITY", "tool-opacity" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_MODIFIER_ACTION_NONE, NC_("modifier-action", "No action"), NULL },
    { GIMP_MODIFIER_ACTION_PANNING, NC_("modifier-action", "Pan"), NULL },
    { GIMP_MODIFIER_ACTION_ZOOMING, NC_("modifier-action", "Zoom"), NULL },
    { GIMP_MODIFIER_ACTION_ROTATING, NC_("modifier-action", "Rotate View"), NULL },
    { GIMP_MODIFIER_ACTION_STEP_ROTATING, NC_("modifier-action", "Rotate View by 15 degree steps"), NULL },
    { GIMP_MODIFIER_ACTION_LAYER_PICKING, NC_("modifier-action", "Pick a layer"), NULL },
    { GIMP_MODIFIER_ACTION_MENU, NC_("modifier-action", "Display the menu"), NULL },
    { GIMP_MODIFIER_ACTION_ACTION, NC_("modifier-action", "Custom action"), NULL },
    { GIMP_MODIFIER_ACTION_BRUSH_PIXEL_SIZE, NC_("modifier-action", "Change brush size in canvas pixels"), NULL },
    { GIMP_MODIFIER_ACTION_BRUSH_RADIUS_PIXEL_SIZE, NC_("modifier-action", "Change brush radius' size in canvas pixels"), NULL },
    { GIMP_MODIFIER_ACTION_TOOL_OPACITY, NC_("modifier-action", "Change tool opacity"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpModifierAction", values);
      gimp_type_set_translation_context (type, "modifier-action");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

