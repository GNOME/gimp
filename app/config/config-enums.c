
/* Generated data (by gimp-mkenums) */

#include "stamp-config-enums.h"
#include "config.h"
#include <gio/gio.h>
#include "libgimpbase/gimpbase.h"
#include "config-enums.h"
#include "gimp-intl.h"

/* enumerations from "config-enums.h" */
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
gimp_export_file_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_EXPORT_FILE_PNG, "GIMP_EXPORT_FILE_PNG", "png" },
    { GIMP_EXPORT_FILE_JPG, "GIMP_EXPORT_FILE_JPG", "jpg" },
    { GIMP_EXPORT_FILE_ORA, "GIMP_EXPORT_FILE_ORA", "ora" },
    { GIMP_EXPORT_FILE_PSD, "GIMP_EXPORT_FILE_PSD", "psd" },
    { GIMP_EXPORT_FILE_PDF, "GIMP_EXPORT_FILE_PDF", "pdf" },
    { GIMP_EXPORT_FILE_TIF, "GIMP_EXPORT_FILE_TIF", "tif" },
    { GIMP_EXPORT_FILE_BMP, "GIMP_EXPORT_FILE_BMP", "bmp" },
    { GIMP_EXPORT_FILE_WEBP, "GIMP_EXPORT_FILE_WEBP", "webp" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_EXPORT_FILE_PNG, NC_("export-file-type", "PNG Image"), NULL },
    { GIMP_EXPORT_FILE_JPG, NC_("export-file-type", "JPEG Image"), NULL },
    { GIMP_EXPORT_FILE_ORA, NC_("export-file-type", "OpenRaster Image"), NULL },
    { GIMP_EXPORT_FILE_PSD, NC_("export-file-type", "Photoshop Image"), NULL },
    { GIMP_EXPORT_FILE_PDF, NC_("export-file-type", "Portable Document Format"), NULL },
    { GIMP_EXPORT_FILE_TIF, NC_("export-file-type", "TIFF Image"), NULL },
    { GIMP_EXPORT_FILE_BMP, NC_("export-file-type", "Windows BMP Image"), NULL },
    { GIMP_EXPORT_FILE_WEBP, NC_("export-file-type", "WebP Image"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpExportFileType", values);
      gimp_type_set_translation_context (type, "export-file-type");
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
gimp_icon_size_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_ICON_SIZE_SMALL, "GIMP_ICON_SIZE_SMALL", "small" },
    { GIMP_ICON_SIZE_MEDIUM, "GIMP_ICON_SIZE_MEDIUM", "medium" },
    { GIMP_ICON_SIZE_LARGE, "GIMP_ICON_SIZE_LARGE", "large" },
    { GIMP_ICON_SIZE_HUGE, "GIMP_ICON_SIZE_HUGE", "huge" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_ICON_SIZE_SMALL, NC_("icon-size", "Small size"), NULL },
    { GIMP_ICON_SIZE_MEDIUM, NC_("icon-size", "Medium size"), NULL },
    { GIMP_ICON_SIZE_LARGE, NC_("icon-size", "Large size"), NULL },
    { GIMP_ICON_SIZE_HUGE, NC_("icon-size", "Huge size"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpIconSize", values);
      gimp_type_set_translation_context (type, "icon-size");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_position_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_POSITION_TOP, "GIMP_POSITION_TOP", "top" },
    { GIMP_POSITION_BOTTOM, "GIMP_POSITION_BOTTOM", "bottom" },
    { GIMP_POSITION_LEFT, "GIMP_POSITION_LEFT", "left" },
    { GIMP_POSITION_RIGHT, "GIMP_POSITION_RIGHT", "right" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_POSITION_TOP, NC_("position", "Top"), NULL },
    { GIMP_POSITION_BOTTOM, NC_("position", "Bottom"), NULL },
    { GIMP_POSITION_LEFT, NC_("position", "Left"), NULL },
    { GIMP_POSITION_RIGHT, NC_("position", "Right"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpPosition", values);
      gimp_type_set_translation_context (type, "position");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_drag_zoom_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { PROP_DRAG_ZOOM_MODE_DISTANCE, "PROP_DRAG_ZOOM_MODE_DISTANCE", "distance" },
    { PROP_DRAG_ZOOM_MODE_DURATION, "PROP_DRAG_ZOOM_MODE_DURATION", "duration" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { PROP_DRAG_ZOOM_MODE_DISTANCE, NC_("drag-zoom-mode", "By distance"), NULL },
    { PROP_DRAG_ZOOM_MODE_DURATION, NC_("drag-zoom-mode", "By duration"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpDragZoomMode", values);
      gimp_type_set_translation_context (type, "drag-zoom-mode");
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
gimp_theme_scheme_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_THEME_LIGHT, "GIMP_THEME_LIGHT", "light" },
    { GIMP_THEME_GRAY, "GIMP_THEME_GRAY", "gray" },
    { GIMP_THEME_DARK, "GIMP_THEME_DARK", "dark" },
    { GIMP_THEME_SYSTEM, "GIMP_THEME_SYSTEM", "system" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_THEME_LIGHT, NC_("theme-scheme", "Light Colors"), NULL },
    { GIMP_THEME_GRAY, NC_("theme-scheme", "Middle Gray"), NULL },
    { GIMP_THEME_DARK, NC_("theme-scheme", "Dark Colors"), NULL },
    { GIMP_THEME_SYSTEM, NC_("theme-scheme", "System Colors"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpThemeScheme", values);
      gimp_type_set_translation_context (type, "theme-scheme");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

