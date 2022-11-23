
/* Generated data (by ligma-mkenums) */

#include "stamp-display-enums.h"
#include "config.h"
#include <gio/gio.h>
#include "libligmabase/ligmabase.h"
#include "display-enums.h"
#include "ligma-intl.h"

/* enumerations from "display-enums.h" */
GType
ligma_button_press_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_BUTTON_PRESS_NORMAL, "LIGMA_BUTTON_PRESS_NORMAL", "normal" },
    { LIGMA_BUTTON_PRESS_DOUBLE, "LIGMA_BUTTON_PRESS_DOUBLE", "double" },
    { LIGMA_BUTTON_PRESS_TRIPLE, "LIGMA_BUTTON_PRESS_TRIPLE", "triple" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_BUTTON_PRESS_NORMAL, "LIGMA_BUTTON_PRESS_NORMAL", NULL },
    { LIGMA_BUTTON_PRESS_DOUBLE, "LIGMA_BUTTON_PRESS_DOUBLE", NULL },
    { LIGMA_BUTTON_PRESS_TRIPLE, "LIGMA_BUTTON_PRESS_TRIPLE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaButtonPressType", values);
      ligma_type_set_translation_context (type, "button-press-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_button_release_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_BUTTON_RELEASE_NORMAL, "LIGMA_BUTTON_RELEASE_NORMAL", "normal" },
    { LIGMA_BUTTON_RELEASE_CANCEL, "LIGMA_BUTTON_RELEASE_CANCEL", "cancel" },
    { LIGMA_BUTTON_RELEASE_CLICK, "LIGMA_BUTTON_RELEASE_CLICK", "click" },
    { LIGMA_BUTTON_RELEASE_NO_MOTION, "LIGMA_BUTTON_RELEASE_NO_MOTION", "no-motion" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_BUTTON_RELEASE_NORMAL, "LIGMA_BUTTON_RELEASE_NORMAL", NULL },
    { LIGMA_BUTTON_RELEASE_CANCEL, "LIGMA_BUTTON_RELEASE_CANCEL", NULL },
    { LIGMA_BUTTON_RELEASE_CLICK, "LIGMA_BUTTON_RELEASE_CLICK", NULL },
    { LIGMA_BUTTON_RELEASE_NO_MOTION, "LIGMA_BUTTON_RELEASE_NO_MOTION", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaButtonReleaseType", values);
      ligma_type_set_translation_context (type, "button-release-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_compass_orientation_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_COMPASS_ORIENTATION_AUTO, "LIGMA_COMPASS_ORIENTATION_AUTO", "auto" },
    { LIGMA_COMPASS_ORIENTATION_HORIZONTAL, "LIGMA_COMPASS_ORIENTATION_HORIZONTAL", "horizontal" },
    { LIGMA_COMPASS_ORIENTATION_VERTICAL, "LIGMA_COMPASS_ORIENTATION_VERTICAL", "vertical" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_COMPASS_ORIENTATION_AUTO, NC_("compass-orientation", "Auto"), NULL },
    { LIGMA_COMPASS_ORIENTATION_HORIZONTAL, NC_("compass-orientation", "Horizontal"), NULL },
    { LIGMA_COMPASS_ORIENTATION_VERTICAL, NC_("compass-orientation", "Vertical"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaCompassOrientation", values);
      ligma_type_set_translation_context (type, "compass-orientation");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_cursor_precision_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_CURSOR_PRECISION_PIXEL_CENTER, "LIGMA_CURSOR_PRECISION_PIXEL_CENTER", "pixel-center" },
    { LIGMA_CURSOR_PRECISION_PIXEL_BORDER, "LIGMA_CURSOR_PRECISION_PIXEL_BORDER", "pixel-border" },
    { LIGMA_CURSOR_PRECISION_SUBPIXEL, "LIGMA_CURSOR_PRECISION_SUBPIXEL", "subpixel" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_CURSOR_PRECISION_PIXEL_CENTER, "LIGMA_CURSOR_PRECISION_PIXEL_CENTER", NULL },
    { LIGMA_CURSOR_PRECISION_PIXEL_BORDER, "LIGMA_CURSOR_PRECISION_PIXEL_BORDER", NULL },
    { LIGMA_CURSOR_PRECISION_SUBPIXEL, "LIGMA_CURSOR_PRECISION_SUBPIXEL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaCursorPrecision", values);
      ligma_type_set_translation_context (type, "cursor-precision");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_guides_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_GUIDES_NONE, "LIGMA_GUIDES_NONE", "none" },
    { LIGMA_GUIDES_CENTER_LINES, "LIGMA_GUIDES_CENTER_LINES", "center-lines" },
    { LIGMA_GUIDES_THIRDS, "LIGMA_GUIDES_THIRDS", "thirds" },
    { LIGMA_GUIDES_FIFTHS, "LIGMA_GUIDES_FIFTHS", "fifths" },
    { LIGMA_GUIDES_GOLDEN, "LIGMA_GUIDES_GOLDEN", "golden" },
    { LIGMA_GUIDES_DIAGONALS, "LIGMA_GUIDES_DIAGONALS", "diagonals" },
    { LIGMA_GUIDES_N_LINES, "LIGMA_GUIDES_N_LINES", "n-lines" },
    { LIGMA_GUIDES_SPACING, "LIGMA_GUIDES_SPACING", "spacing" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_GUIDES_NONE, NC_("guides-type", "No guides"), NULL },
    { LIGMA_GUIDES_CENTER_LINES, NC_("guides-type", "Center lines"), NULL },
    { LIGMA_GUIDES_THIRDS, NC_("guides-type", "Rule of thirds"), NULL },
    { LIGMA_GUIDES_FIFTHS, NC_("guides-type", "Rule of fifths"), NULL },
    { LIGMA_GUIDES_GOLDEN, NC_("guides-type", "Golden sections"), NULL },
    { LIGMA_GUIDES_DIAGONALS, NC_("guides-type", "Diagonal lines"), NULL },
    { LIGMA_GUIDES_N_LINES, NC_("guides-type", "Number of lines"), NULL },
    { LIGMA_GUIDES_SPACING, NC_("guides-type", "Line spacing"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaGuidesType", values);
      ligma_type_set_translation_context (type, "guides-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_handle_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_HANDLE_SQUARE, "LIGMA_HANDLE_SQUARE", "square" },
    { LIGMA_HANDLE_DASHED_SQUARE, "LIGMA_HANDLE_DASHED_SQUARE", "dashed-square" },
    { LIGMA_HANDLE_FILLED_SQUARE, "LIGMA_HANDLE_FILLED_SQUARE", "filled-square" },
    { LIGMA_HANDLE_CIRCLE, "LIGMA_HANDLE_CIRCLE", "circle" },
    { LIGMA_HANDLE_DASHED_CIRCLE, "LIGMA_HANDLE_DASHED_CIRCLE", "dashed-circle" },
    { LIGMA_HANDLE_FILLED_CIRCLE, "LIGMA_HANDLE_FILLED_CIRCLE", "filled-circle" },
    { LIGMA_HANDLE_DIAMOND, "LIGMA_HANDLE_DIAMOND", "diamond" },
    { LIGMA_HANDLE_DASHED_DIAMOND, "LIGMA_HANDLE_DASHED_DIAMOND", "dashed-diamond" },
    { LIGMA_HANDLE_FILLED_DIAMOND, "LIGMA_HANDLE_FILLED_DIAMOND", "filled-diamond" },
    { LIGMA_HANDLE_CROSS, "LIGMA_HANDLE_CROSS", "cross" },
    { LIGMA_HANDLE_CROSSHAIR, "LIGMA_HANDLE_CROSSHAIR", "crosshair" },
    { LIGMA_HANDLE_DROP, "LIGMA_HANDLE_DROP", "drop" },
    { LIGMA_HANDLE_FILLED_DROP, "LIGMA_HANDLE_FILLED_DROP", "filled-drop" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_HANDLE_SQUARE, "LIGMA_HANDLE_SQUARE", NULL },
    { LIGMA_HANDLE_DASHED_SQUARE, "LIGMA_HANDLE_DASHED_SQUARE", NULL },
    { LIGMA_HANDLE_FILLED_SQUARE, "LIGMA_HANDLE_FILLED_SQUARE", NULL },
    { LIGMA_HANDLE_CIRCLE, "LIGMA_HANDLE_CIRCLE", NULL },
    { LIGMA_HANDLE_DASHED_CIRCLE, "LIGMA_HANDLE_DASHED_CIRCLE", NULL },
    { LIGMA_HANDLE_FILLED_CIRCLE, "LIGMA_HANDLE_FILLED_CIRCLE", NULL },
    { LIGMA_HANDLE_DIAMOND, "LIGMA_HANDLE_DIAMOND", NULL },
    { LIGMA_HANDLE_DASHED_DIAMOND, "LIGMA_HANDLE_DASHED_DIAMOND", NULL },
    { LIGMA_HANDLE_FILLED_DIAMOND, "LIGMA_HANDLE_FILLED_DIAMOND", NULL },
    { LIGMA_HANDLE_CROSS, "LIGMA_HANDLE_CROSS", NULL },
    { LIGMA_HANDLE_CROSSHAIR, "LIGMA_HANDLE_CROSSHAIR", NULL },
    { LIGMA_HANDLE_DROP, "LIGMA_HANDLE_DROP", NULL },
    { LIGMA_HANDLE_FILLED_DROP, "LIGMA_HANDLE_FILLED_DROP", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaHandleType", values);
      ligma_type_set_translation_context (type, "handle-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_handle_anchor_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_HANDLE_ANCHOR_CENTER, "LIGMA_HANDLE_ANCHOR_CENTER", "center" },
    { LIGMA_HANDLE_ANCHOR_NORTH, "LIGMA_HANDLE_ANCHOR_NORTH", "north" },
    { LIGMA_HANDLE_ANCHOR_NORTH_WEST, "LIGMA_HANDLE_ANCHOR_NORTH_WEST", "north-west" },
    { LIGMA_HANDLE_ANCHOR_NORTH_EAST, "LIGMA_HANDLE_ANCHOR_NORTH_EAST", "north-east" },
    { LIGMA_HANDLE_ANCHOR_SOUTH, "LIGMA_HANDLE_ANCHOR_SOUTH", "south" },
    { LIGMA_HANDLE_ANCHOR_SOUTH_WEST, "LIGMA_HANDLE_ANCHOR_SOUTH_WEST", "south-west" },
    { LIGMA_HANDLE_ANCHOR_SOUTH_EAST, "LIGMA_HANDLE_ANCHOR_SOUTH_EAST", "south-east" },
    { LIGMA_HANDLE_ANCHOR_WEST, "LIGMA_HANDLE_ANCHOR_WEST", "west" },
    { LIGMA_HANDLE_ANCHOR_EAST, "LIGMA_HANDLE_ANCHOR_EAST", "east" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_HANDLE_ANCHOR_CENTER, "LIGMA_HANDLE_ANCHOR_CENTER", NULL },
    { LIGMA_HANDLE_ANCHOR_NORTH, "LIGMA_HANDLE_ANCHOR_NORTH", NULL },
    { LIGMA_HANDLE_ANCHOR_NORTH_WEST, "LIGMA_HANDLE_ANCHOR_NORTH_WEST", NULL },
    { LIGMA_HANDLE_ANCHOR_NORTH_EAST, "LIGMA_HANDLE_ANCHOR_NORTH_EAST", NULL },
    { LIGMA_HANDLE_ANCHOR_SOUTH, "LIGMA_HANDLE_ANCHOR_SOUTH", NULL },
    { LIGMA_HANDLE_ANCHOR_SOUTH_WEST, "LIGMA_HANDLE_ANCHOR_SOUTH_WEST", NULL },
    { LIGMA_HANDLE_ANCHOR_SOUTH_EAST, "LIGMA_HANDLE_ANCHOR_SOUTH_EAST", NULL },
    { LIGMA_HANDLE_ANCHOR_WEST, "LIGMA_HANDLE_ANCHOR_WEST", NULL },
    { LIGMA_HANDLE_ANCHOR_EAST, "LIGMA_HANDLE_ANCHOR_EAST", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaHandleAnchor", values);
      ligma_type_set_translation_context (type, "handle-anchor");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_limit_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_LIMIT_CIRCLE, "LIGMA_LIMIT_CIRCLE", "circle" },
    { LIGMA_LIMIT_SQUARE, "LIGMA_LIMIT_SQUARE", "square" },
    { LIGMA_LIMIT_DIAMOND, "LIGMA_LIMIT_DIAMOND", "diamond" },
    { LIGMA_LIMIT_HORIZONTAL, "LIGMA_LIMIT_HORIZONTAL", "horizontal" },
    { LIGMA_LIMIT_VERTICAL, "LIGMA_LIMIT_VERTICAL", "vertical" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_LIMIT_CIRCLE, "LIGMA_LIMIT_CIRCLE", NULL },
    { LIGMA_LIMIT_SQUARE, "LIGMA_LIMIT_SQUARE", NULL },
    { LIGMA_LIMIT_DIAMOND, "LIGMA_LIMIT_DIAMOND", NULL },
    { LIGMA_LIMIT_HORIZONTAL, "LIGMA_LIMIT_HORIZONTAL", NULL },
    { LIGMA_LIMIT_VERTICAL, "LIGMA_LIMIT_VERTICAL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaLimitType", values);
      ligma_type_set_translation_context (type, "limit-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_path_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_PATH_STYLE_DEFAULT, "LIGMA_PATH_STYLE_DEFAULT", "default" },
    { LIGMA_PATH_STYLE_VECTORS, "LIGMA_PATH_STYLE_VECTORS", "vectors" },
    { LIGMA_PATH_STYLE_OUTLINE, "LIGMA_PATH_STYLE_OUTLINE", "outline" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_PATH_STYLE_DEFAULT, "LIGMA_PATH_STYLE_DEFAULT", NULL },
    { LIGMA_PATH_STYLE_VECTORS, "LIGMA_PATH_STYLE_VECTORS", NULL },
    { LIGMA_PATH_STYLE_OUTLINE, "LIGMA_PATH_STYLE_OUTLINE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaPathStyle", values);
      ligma_type_set_translation_context (type, "path-style");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_rectangle_constraint_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_RECTANGLE_CONSTRAIN_NONE, "LIGMA_RECTANGLE_CONSTRAIN_NONE", "none" },
    { LIGMA_RECTANGLE_CONSTRAIN_IMAGE, "LIGMA_RECTANGLE_CONSTRAIN_IMAGE", "image" },
    { LIGMA_RECTANGLE_CONSTRAIN_DRAWABLE, "LIGMA_RECTANGLE_CONSTRAIN_DRAWABLE", "drawable" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_RECTANGLE_CONSTRAIN_NONE, "LIGMA_RECTANGLE_CONSTRAIN_NONE", NULL },
    { LIGMA_RECTANGLE_CONSTRAIN_IMAGE, "LIGMA_RECTANGLE_CONSTRAIN_IMAGE", NULL },
    { LIGMA_RECTANGLE_CONSTRAIN_DRAWABLE, "LIGMA_RECTANGLE_CONSTRAIN_DRAWABLE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaRectangleConstraint", values);
      ligma_type_set_translation_context (type, "rectangle-constraint");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_rectangle_fixed_rule_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_RECTANGLE_FIXED_ASPECT, "LIGMA_RECTANGLE_FIXED_ASPECT", "aspect" },
    { LIGMA_RECTANGLE_FIXED_WIDTH, "LIGMA_RECTANGLE_FIXED_WIDTH", "width" },
    { LIGMA_RECTANGLE_FIXED_HEIGHT, "LIGMA_RECTANGLE_FIXED_HEIGHT", "height" },
    { LIGMA_RECTANGLE_FIXED_SIZE, "LIGMA_RECTANGLE_FIXED_SIZE", "size" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_RECTANGLE_FIXED_ASPECT, NC_("rectangle-fixed-rule", "Aspect ratio"), NULL },
    { LIGMA_RECTANGLE_FIXED_WIDTH, NC_("rectangle-fixed-rule", "Width"), NULL },
    { LIGMA_RECTANGLE_FIXED_HEIGHT, NC_("rectangle-fixed-rule", "Height"), NULL },
    { LIGMA_RECTANGLE_FIXED_SIZE, NC_("rectangle-fixed-rule", "Size"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaRectangleFixedRule", values);
      ligma_type_set_translation_context (type, "rectangle-fixed-rule");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_rectangle_precision_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_RECTANGLE_PRECISION_INT, "LIGMA_RECTANGLE_PRECISION_INT", "int" },
    { LIGMA_RECTANGLE_PRECISION_DOUBLE, "LIGMA_RECTANGLE_PRECISION_DOUBLE", "double" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_RECTANGLE_PRECISION_INT, "LIGMA_RECTANGLE_PRECISION_INT", NULL },
    { LIGMA_RECTANGLE_PRECISION_DOUBLE, "LIGMA_RECTANGLE_PRECISION_DOUBLE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaRectanglePrecision", values);
      ligma_type_set_translation_context (type, "rectangle-precision");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_transform_3d_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_TRANSFORM_3D_MODE_CAMERA, "LIGMA_TRANSFORM_3D_MODE_CAMERA", "camera" },
    { LIGMA_TRANSFORM_3D_MODE_MOVE, "LIGMA_TRANSFORM_3D_MODE_MOVE", "move" },
    { LIGMA_TRANSFORM_3D_MODE_ROTATE, "LIGMA_TRANSFORM_3D_MODE_ROTATE", "rotate" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_TRANSFORM_3D_MODE_CAMERA, "LIGMA_TRANSFORM_3D_MODE_CAMERA", NULL },
    { LIGMA_TRANSFORM_3D_MODE_MOVE, "LIGMA_TRANSFORM_3D_MODE_MOVE", NULL },
    { LIGMA_TRANSFORM_3D_MODE_ROTATE, "LIGMA_TRANSFORM_3D_MODE_ROTATE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaTransform3DMode", values);
      ligma_type_set_translation_context (type, "transform3-dmode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_transform_function_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_TRANSFORM_FUNCTION_NONE, "LIGMA_TRANSFORM_FUNCTION_NONE", "none" },
    { LIGMA_TRANSFORM_FUNCTION_MOVE, "LIGMA_TRANSFORM_FUNCTION_MOVE", "move" },
    { LIGMA_TRANSFORM_FUNCTION_SCALE, "LIGMA_TRANSFORM_FUNCTION_SCALE", "scale" },
    { LIGMA_TRANSFORM_FUNCTION_ROTATE, "LIGMA_TRANSFORM_FUNCTION_ROTATE", "rotate" },
    { LIGMA_TRANSFORM_FUNCTION_SHEAR, "LIGMA_TRANSFORM_FUNCTION_SHEAR", "shear" },
    { LIGMA_TRANSFORM_FUNCTION_PERSPECTIVE, "LIGMA_TRANSFORM_FUNCTION_PERSPECTIVE", "perspective" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_TRANSFORM_FUNCTION_NONE, "LIGMA_TRANSFORM_FUNCTION_NONE", NULL },
    { LIGMA_TRANSFORM_FUNCTION_MOVE, "LIGMA_TRANSFORM_FUNCTION_MOVE", NULL },
    { LIGMA_TRANSFORM_FUNCTION_SCALE, "LIGMA_TRANSFORM_FUNCTION_SCALE", NULL },
    { LIGMA_TRANSFORM_FUNCTION_ROTATE, "LIGMA_TRANSFORM_FUNCTION_ROTATE", NULL },
    { LIGMA_TRANSFORM_FUNCTION_SHEAR, "LIGMA_TRANSFORM_FUNCTION_SHEAR", NULL },
    { LIGMA_TRANSFORM_FUNCTION_PERSPECTIVE, "LIGMA_TRANSFORM_FUNCTION_PERSPECTIVE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaTransformFunction", values);
      ligma_type_set_translation_context (type, "transform-function");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_transform_handle_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_HANDLE_MODE_ADD_TRANSFORM, "LIGMA_HANDLE_MODE_ADD_TRANSFORM", "add-transform" },
    { LIGMA_HANDLE_MODE_MOVE, "LIGMA_HANDLE_MODE_MOVE", "move" },
    { LIGMA_HANDLE_MODE_REMOVE, "LIGMA_HANDLE_MODE_REMOVE", "remove" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_HANDLE_MODE_ADD_TRANSFORM, NC_("transform-handle-mode", "Add / Transform"), NULL },
    { LIGMA_HANDLE_MODE_MOVE, NC_("transform-handle-mode", "Move"), NULL },
    { LIGMA_HANDLE_MODE_REMOVE, NC_("transform-handle-mode", "Remove"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaTransformHandleMode", values);
      ligma_type_set_translation_context (type, "transform-handle-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_vector_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_VECTOR_MODE_DESIGN, "LIGMA_VECTOR_MODE_DESIGN", "design" },
    { LIGMA_VECTOR_MODE_EDIT, "LIGMA_VECTOR_MODE_EDIT", "edit" },
    { LIGMA_VECTOR_MODE_MOVE, "LIGMA_VECTOR_MODE_MOVE", "move" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_VECTOR_MODE_DESIGN, NC_("vector-mode", "Design"), NULL },
    { LIGMA_VECTOR_MODE_EDIT, NC_("vector-mode", "Edit"), NULL },
    { LIGMA_VECTOR_MODE_MOVE, NC_("vector-mode", "Move"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaVectorMode", values);
      ligma_type_set_translation_context (type, "vector-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_zoom_focus_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_ZOOM_FOCUS_BEST_GUESS, "LIGMA_ZOOM_FOCUS_BEST_GUESS", "best-guess" },
    { LIGMA_ZOOM_FOCUS_POINTER, "LIGMA_ZOOM_FOCUS_POINTER", "pointer" },
    { LIGMA_ZOOM_FOCUS_IMAGE_CENTER, "LIGMA_ZOOM_FOCUS_IMAGE_CENTER", "image-center" },
    { LIGMA_ZOOM_FOCUS_RETAIN_CENTERING_ELSE_BEST_GUESS, "LIGMA_ZOOM_FOCUS_RETAIN_CENTERING_ELSE_BEST_GUESS", "retain-centering-else-best-guess" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_ZOOM_FOCUS_BEST_GUESS, "LIGMA_ZOOM_FOCUS_BEST_GUESS", NULL },
    { LIGMA_ZOOM_FOCUS_POINTER, "LIGMA_ZOOM_FOCUS_POINTER", NULL },
    { LIGMA_ZOOM_FOCUS_IMAGE_CENTER, "LIGMA_ZOOM_FOCUS_IMAGE_CENTER", NULL },
    { LIGMA_ZOOM_FOCUS_RETAIN_CENTERING_ELSE_BEST_GUESS, "LIGMA_ZOOM_FOCUS_RETAIN_CENTERING_ELSE_BEST_GUESS", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaZoomFocus", values);
      ligma_type_set_translation_context (type, "zoom-focus");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_modifier_action_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_MODIFIER_ACTION_NONE, "LIGMA_MODIFIER_ACTION_NONE", "none" },
    { LIGMA_MODIFIER_ACTION_PANNING, "LIGMA_MODIFIER_ACTION_PANNING", "panning" },
    { LIGMA_MODIFIER_ACTION_ZOOMING, "LIGMA_MODIFIER_ACTION_ZOOMING", "zooming" },
    { LIGMA_MODIFIER_ACTION_ROTATING, "LIGMA_MODIFIER_ACTION_ROTATING", "rotating" },
    { LIGMA_MODIFIER_ACTION_STEP_ROTATING, "LIGMA_MODIFIER_ACTION_STEP_ROTATING", "step-rotating" },
    { LIGMA_MODIFIER_ACTION_LAYER_PICKING, "LIGMA_MODIFIER_ACTION_LAYER_PICKING", "layer-picking" },
    { LIGMA_MODIFIER_ACTION_MENU, "LIGMA_MODIFIER_ACTION_MENU", "menu" },
    { LIGMA_MODIFIER_ACTION_ACTION, "LIGMA_MODIFIER_ACTION_ACTION", "action" },
    { LIGMA_MODIFIER_ACTION_BRUSH_PIXEL_SIZE, "LIGMA_MODIFIER_ACTION_BRUSH_PIXEL_SIZE", "brush-pixel-size" },
    { LIGMA_MODIFIER_ACTION_BRUSH_RADIUS_PIXEL_SIZE, "LIGMA_MODIFIER_ACTION_BRUSH_RADIUS_PIXEL_SIZE", "brush-radius-pixel-size" },
    { LIGMA_MODIFIER_ACTION_TOOL_OPACITY, "LIGMA_MODIFIER_ACTION_TOOL_OPACITY", "tool-opacity" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_MODIFIER_ACTION_NONE, NC_("modifier-action", "No action"), NULL },
    { LIGMA_MODIFIER_ACTION_PANNING, NC_("modifier-action", "Pan"), NULL },
    { LIGMA_MODIFIER_ACTION_ZOOMING, NC_("modifier-action", "Zoom"), NULL },
    { LIGMA_MODIFIER_ACTION_ROTATING, NC_("modifier-action", "Rotate View"), NULL },
    { LIGMA_MODIFIER_ACTION_STEP_ROTATING, NC_("modifier-action", "Rotate View by 15 degree steps"), NULL },
    { LIGMA_MODIFIER_ACTION_LAYER_PICKING, NC_("modifier-action", "Pick a layer"), NULL },
    { LIGMA_MODIFIER_ACTION_MENU, NC_("modifier-action", "Display the menu"), NULL },
    { LIGMA_MODIFIER_ACTION_ACTION, NC_("modifier-action", "Custom action"), NULL },
    { LIGMA_MODIFIER_ACTION_BRUSH_PIXEL_SIZE, NC_("modifier-action", "Change brush size in canvas pixels"), NULL },
    { LIGMA_MODIFIER_ACTION_BRUSH_RADIUS_PIXEL_SIZE, NC_("modifier-action", "Change brush radius' size in canvas pixels"), NULL },
    { LIGMA_MODIFIER_ACTION_TOOL_OPACITY, NC_("modifier-action", "Change tool opacity"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaModifierAction", values);
      ligma_type_set_translation_context (type, "modifier-action");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

