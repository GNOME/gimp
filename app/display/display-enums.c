
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <gio/gio.h>
#include "libgimpbase/gimpbase.h"
#include "display-enums.h"
#include"gimp-intl.h"

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
    { GIMP_HANDLE_FILLED_SQUARE, "GIMP_HANDLE_FILLED_SQUARE", "filled-square" },
    { GIMP_HANDLE_CIRCLE, "GIMP_HANDLE_CIRCLE", "circle" },
    { GIMP_HANDLE_FILLED_CIRCLE, "GIMP_HANDLE_FILLED_CIRCLE", "filled-circle" },
    { GIMP_HANDLE_DIAMOND, "GIMP_HANDLE_DIAMOND", "diamond" },
    { GIMP_HANDLE_FILLED_DIAMOND, "GIMP_HANDLE_FILLED_DIAMOND", "filled-diamond" },
    { GIMP_HANDLE_CROSS, "GIMP_HANDLE_CROSS", "cross" },
    { GIMP_HANDLE_CROSSHAIR, "GIMP_HANDLE_CROSSHAIR", "crosshair" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_HANDLE_SQUARE, "GIMP_HANDLE_SQUARE", NULL },
    { GIMP_HANDLE_FILLED_SQUARE, "GIMP_HANDLE_FILLED_SQUARE", NULL },
    { GIMP_HANDLE_CIRCLE, "GIMP_HANDLE_CIRCLE", NULL },
    { GIMP_HANDLE_FILLED_CIRCLE, "GIMP_HANDLE_FILLED_CIRCLE", NULL },
    { GIMP_HANDLE_DIAMOND, "GIMP_HANDLE_DIAMOND", NULL },
    { GIMP_HANDLE_FILLED_DIAMOND, "GIMP_HANDLE_FILLED_DIAMOND", NULL },
    { GIMP_HANDLE_CROSS, "GIMP_HANDLE_CROSS", NULL },
    { GIMP_HANDLE_CROSSHAIR, "GIMP_HANDLE_CROSSHAIR", NULL },
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
gimp_transform_function_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_TRANSFORM_FUNCTION_MOVE, "GIMP_TRANSFORM_FUNCTION_MOVE", "move" },
    { GIMP_TRANSFORM_FUNCTION_SCALE, "GIMP_TRANSFORM_FUNCTION_SCALE", "scale" },
    { GIMP_TRANSFORM_FUNCTION_ROTATE, "GIMP_TRANSFORM_FUNCTION_ROTATE", "rotate" },
    { GIMP_TRANSFORM_FUNCTION_SHEAR, "GIMP_TRANSFORM_FUNCTION_SHEAR", "shear" },
    { GIMP_TRANSFORM_FUNCTION_PERSPECTIVE, "GIMP_TRANSFORM_FUNCTION_PERSPECTIVE", "perspective" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
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


/* Generated data ends here */

