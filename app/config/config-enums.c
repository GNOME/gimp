
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "libgimpbase/gimpbase.h"
#include "config-enums.h"
#include"gimp-intl.h"

/* enumerations from "./config-enums.h" */
GType
gimp_cursor_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_CURSOR_MODE_TOOL_ICON, "GIMP_CURSOR_MODE_TOOL_ICON", "tool-icon" },
    { GIMP_CURSOR_MODE_TOOL_CROSSHAIR, "GIMP_CURSOR_MODE_TOOL_CROSSHAIR", "tool-crosshair" },
    { GIMP_CURSOR_MODE_CROSSHAIR, "GIMP_CURSOR_MODE_CROSSHAIR", "crosshair" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_CURSOR_MODE_TOOL_ICON, NC_("cursor-mode", "Tool icon"), NULL },
    { GIMP_CURSOR_MODE_TOOL_CROSSHAIR, NC_("cursor-mode", "Tool icon with crosshair"), NULL },
    { GIMP_CURSOR_MODE_CROSSHAIR, NC_("cursor-mode", "Crosshair only"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpCursorMode", values);
      gimp_type_set_translation_context (type, "cursor-mode");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_canvas_padding_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_CANVAS_PADDING_MODE_DEFAULT, "GIMP_CANVAS_PADDING_MODE_DEFAULT", "default" },
    { GIMP_CANVAS_PADDING_MODE_LIGHT_CHECK, "GIMP_CANVAS_PADDING_MODE_LIGHT_CHECK", "light-check" },
    { GIMP_CANVAS_PADDING_MODE_DARK_CHECK, "GIMP_CANVAS_PADDING_MODE_DARK_CHECK", "dark-check" },
    { GIMP_CANVAS_PADDING_MODE_CUSTOM, "GIMP_CANVAS_PADDING_MODE_CUSTOM", "custom" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_CANVAS_PADDING_MODE_DEFAULT, NC_("canvas-padding-mode", "From theme"), NULL },
    { GIMP_CANVAS_PADDING_MODE_LIGHT_CHECK, NC_("canvas-padding-mode", "Light check color"), NULL },
    { GIMP_CANVAS_PADDING_MODE_DARK_CHECK, NC_("canvas-padding-mode", "Dark check color"), NULL },
    { GIMP_CANVAS_PADDING_MODE_CUSTOM, NC_("canvas-padding-mode", "Custom color"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpCanvasPaddingMode", values);
      gimp_type_set_translation_context (type, "canvas-padding-mode");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_space_bar_action_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_SPACE_BAR_ACTION_NONE, "GIMP_SPACE_BAR_ACTION_NONE", "none" },
    { GIMP_SPACE_BAR_ACTION_PAN, "GIMP_SPACE_BAR_ACTION_PAN", "pan" },
    { GIMP_SPACE_BAR_ACTION_MOVE, "GIMP_SPACE_BAR_ACTION_MOVE", "move" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_SPACE_BAR_ACTION_NONE, NC_("space-bar-action", "No action"), NULL },
    { GIMP_SPACE_BAR_ACTION_PAN, NC_("space-bar-action", "Pan view"), NULL },
    { GIMP_SPACE_BAR_ACTION_MOVE, NC_("space-bar-action", "Switch to Move tool"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpSpaceBarAction", values);
      gimp_type_set_translation_context (type, "space-bar-action");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_zoom_quality_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_ZOOM_QUALITY_LOW, "GIMP_ZOOM_QUALITY_LOW", "low" },
    { GIMP_ZOOM_QUALITY_HIGH, "GIMP_ZOOM_QUALITY_HIGH", "high" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_ZOOM_QUALITY_LOW, NC_("zoom-quality", "Low"), NULL },
    { GIMP_ZOOM_QUALITY_HIGH, NC_("zoom-quality", "High"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpZoomQuality", values);
      gimp_type_set_translation_context (type, "zoom-quality");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_help_browser_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_HELP_BROWSER_GIMP, "GIMP_HELP_BROWSER_GIMP", "gimp" },
    { GIMP_HELP_BROWSER_WEB_BROWSER, "GIMP_HELP_BROWSER_WEB_BROWSER", "web-browser" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_HELP_BROWSER_GIMP, NC_("help-browser-type", "GIMP help browser"), NULL },
    { GIMP_HELP_BROWSER_WEB_BROWSER, NC_("help-browser-type", "Web browser"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpHelpBrowserType", values);
      gimp_type_set_translation_context (type, "help-browser-type");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_window_hint_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_WINDOW_HINT_NORMAL, "GIMP_WINDOW_HINT_NORMAL", "normal" },
    { GIMP_WINDOW_HINT_UTILITY, "GIMP_WINDOW_HINT_UTILITY", "utility" },
    { GIMP_WINDOW_HINT_KEEP_ABOVE, "GIMP_WINDOW_HINT_KEEP_ABOVE", "keep-above" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_WINDOW_HINT_NORMAL, NC_("window-hint", "Normal window"), NULL },
    { GIMP_WINDOW_HINT_UTILITY, NC_("window-hint", "Utility window"), NULL },
    { GIMP_WINDOW_HINT_KEEP_ABOVE, NC_("window-hint", "Keep above"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpWindowHint", values);
      gimp_type_set_translation_context (type, "window-hint");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_cursor_format_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_CURSOR_FORMAT_BITMAP, "GIMP_CURSOR_FORMAT_BITMAP", "bitmap" },
    { GIMP_CURSOR_FORMAT_PIXBUF, "GIMP_CURSOR_FORMAT_PIXBUF", "pixbuf" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_CURSOR_FORMAT_BITMAP, NC_("cursor-format", "Black & white"), NULL },
    { GIMP_CURSOR_FORMAT_PIXBUF, NC_("cursor-format", "Fancy"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpCursorFormat", values);
      gimp_type_set_translation_context (type, "cursor-format");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_handedness_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_HANDEDNESS_LEFT, "GIMP_HANDEDNESS_LEFT", "left" },
    { GIMP_HANDEDNESS_RIGHT, "GIMP_HANDEDNESS_RIGHT", "right" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_HANDEDNESS_LEFT, NC_("handedness", "Left-handed"), NULL },
    { GIMP_HANDEDNESS_RIGHT, NC_("handedness", "Right-handed"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpHandedness", values);
      gimp_type_set_translation_context (type, "handedness");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

