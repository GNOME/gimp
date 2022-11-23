
/* Generated data (by ligma-mkenums) */

#include "stamp-config-enums.h"
#include "config.h"
#include <gio/gio.h>
#include "libligmabase/ligmabase.h"
#include "config-enums.h"
#include "ligma-intl.h"

/* enumerations from "config-enums.h" */
GType
ligma_canvas_padding_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_CANVAS_PADDING_MODE_DEFAULT, "LIGMA_CANVAS_PADDING_MODE_DEFAULT", "default" },
    { LIGMA_CANVAS_PADDING_MODE_LIGHT_CHECK, "LIGMA_CANVAS_PADDING_MODE_LIGHT_CHECK", "light-check" },
    { LIGMA_CANVAS_PADDING_MODE_DARK_CHECK, "LIGMA_CANVAS_PADDING_MODE_DARK_CHECK", "dark-check" },
    { LIGMA_CANVAS_PADDING_MODE_CUSTOM, "LIGMA_CANVAS_PADDING_MODE_CUSTOM", "custom" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_CANVAS_PADDING_MODE_DEFAULT, NC_("canvas-padding-mode", "From theme"), NULL },
    { LIGMA_CANVAS_PADDING_MODE_LIGHT_CHECK, NC_("canvas-padding-mode", "Light check color"), NULL },
    { LIGMA_CANVAS_PADDING_MODE_DARK_CHECK, NC_("canvas-padding-mode", "Dark check color"), NULL },
    { LIGMA_CANVAS_PADDING_MODE_CUSTOM, NC_("canvas-padding-mode", "Custom color"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaCanvasPaddingMode", values);
      ligma_type_set_translation_context (type, "canvas-padding-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_cursor_format_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_CURSOR_FORMAT_BITMAP, "LIGMA_CURSOR_FORMAT_BITMAP", "bitmap" },
    { LIGMA_CURSOR_FORMAT_PIXBUF, "LIGMA_CURSOR_FORMAT_PIXBUF", "pixbuf" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_CURSOR_FORMAT_BITMAP, NC_("cursor-format", "Black & white"), NULL },
    { LIGMA_CURSOR_FORMAT_PIXBUF, NC_("cursor-format", "Fancy"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaCursorFormat", values);
      ligma_type_set_translation_context (type, "cursor-format");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_cursor_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_CURSOR_MODE_TOOL_ICON, "LIGMA_CURSOR_MODE_TOOL_ICON", "tool-icon" },
    { LIGMA_CURSOR_MODE_TOOL_CROSSHAIR, "LIGMA_CURSOR_MODE_TOOL_CROSSHAIR", "tool-crosshair" },
    { LIGMA_CURSOR_MODE_CROSSHAIR, "LIGMA_CURSOR_MODE_CROSSHAIR", "crosshair" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_CURSOR_MODE_TOOL_ICON, NC_("cursor-mode", "Tool icon"), NULL },
    { LIGMA_CURSOR_MODE_TOOL_CROSSHAIR, NC_("cursor-mode", "Tool icon with crosshair"), NULL },
    { LIGMA_CURSOR_MODE_CROSSHAIR, NC_("cursor-mode", "Crosshair only"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaCursorMode", values);
      ligma_type_set_translation_context (type, "cursor-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_export_file_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_EXPORT_FILE_PNG, "LIGMA_EXPORT_FILE_PNG", "png" },
    { LIGMA_EXPORT_FILE_JPG, "LIGMA_EXPORT_FILE_JPG", "jpg" },
    { LIGMA_EXPORT_FILE_ORA, "LIGMA_EXPORT_FILE_ORA", "ora" },
    { LIGMA_EXPORT_FILE_PSD, "LIGMA_EXPORT_FILE_PSD", "psd" },
    { LIGMA_EXPORT_FILE_PDF, "LIGMA_EXPORT_FILE_PDF", "pdf" },
    { LIGMA_EXPORT_FILE_TIF, "LIGMA_EXPORT_FILE_TIF", "tif" },
    { LIGMA_EXPORT_FILE_BMP, "LIGMA_EXPORT_FILE_BMP", "bmp" },
    { LIGMA_EXPORT_FILE_WEBP, "LIGMA_EXPORT_FILE_WEBP", "webp" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_EXPORT_FILE_PNG, NC_("export-file-type", "PNG Image"), NULL },
    { LIGMA_EXPORT_FILE_JPG, NC_("export-file-type", "JPEG Image"), NULL },
    { LIGMA_EXPORT_FILE_ORA, NC_("export-file-type", "OpenRaster Image"), NULL },
    { LIGMA_EXPORT_FILE_PSD, NC_("export-file-type", "Photoshop Image"), NULL },
    { LIGMA_EXPORT_FILE_PDF, NC_("export-file-type", "Portable Document Format"), NULL },
    { LIGMA_EXPORT_FILE_TIF, NC_("export-file-type", "TIFF Image"), NULL },
    { LIGMA_EXPORT_FILE_BMP, NC_("export-file-type", "Windows BMP Image"), NULL },
    { LIGMA_EXPORT_FILE_WEBP, NC_("export-file-type", "WebP Image"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaExportFileType", values);
      ligma_type_set_translation_context (type, "export-file-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_handedness_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_HANDEDNESS_LEFT, "LIGMA_HANDEDNESS_LEFT", "left" },
    { LIGMA_HANDEDNESS_RIGHT, "LIGMA_HANDEDNESS_RIGHT", "right" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_HANDEDNESS_LEFT, NC_("handedness", "Left-handed"), NULL },
    { LIGMA_HANDEDNESS_RIGHT, NC_("handedness", "Right-handed"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaHandedness", values);
      ligma_type_set_translation_context (type, "handedness");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_help_browser_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_HELP_BROWSER_LIGMA, "LIGMA_HELP_BROWSER_LIGMA", "ligma" },
    { LIGMA_HELP_BROWSER_WEB_BROWSER, "LIGMA_HELP_BROWSER_WEB_BROWSER", "web-browser" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_HELP_BROWSER_LIGMA, NC_("help-browser-type", "LIGMA help browser"), NULL },
    { LIGMA_HELP_BROWSER_WEB_BROWSER, NC_("help-browser-type", "Web browser"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaHelpBrowserType", values);
      ligma_type_set_translation_context (type, "help-browser-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_icon_size_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_ICON_SIZE_SMALL, "LIGMA_ICON_SIZE_SMALL", "small" },
    { LIGMA_ICON_SIZE_MEDIUM, "LIGMA_ICON_SIZE_MEDIUM", "medium" },
    { LIGMA_ICON_SIZE_LARGE, "LIGMA_ICON_SIZE_LARGE", "large" },
    { LIGMA_ICON_SIZE_HUGE, "LIGMA_ICON_SIZE_HUGE", "huge" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_ICON_SIZE_SMALL, NC_("icon-size", "Small size"), NULL },
    { LIGMA_ICON_SIZE_MEDIUM, NC_("icon-size", "Medium size"), NULL },
    { LIGMA_ICON_SIZE_LARGE, NC_("icon-size", "Large size"), NULL },
    { LIGMA_ICON_SIZE_HUGE, NC_("icon-size", "Huge size"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaIconSize", values);
      ligma_type_set_translation_context (type, "icon-size");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_position_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_POSITION_TOP, "LIGMA_POSITION_TOP", "top" },
    { LIGMA_POSITION_BOTTOM, "LIGMA_POSITION_BOTTOM", "bottom" },
    { LIGMA_POSITION_LEFT, "LIGMA_POSITION_LEFT", "left" },
    { LIGMA_POSITION_RIGHT, "LIGMA_POSITION_RIGHT", "right" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_POSITION_TOP, NC_("position", "Top"), NULL },
    { LIGMA_POSITION_BOTTOM, NC_("position", "Bottom"), NULL },
    { LIGMA_POSITION_LEFT, NC_("position", "Left"), NULL },
    { LIGMA_POSITION_RIGHT, NC_("position", "Right"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaPosition", values);
      ligma_type_set_translation_context (type, "position");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_drag_zoom_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { PROP_DRAG_ZOOM_MODE_DISTANCE, "PROP_DRAG_ZOOM_MODE_DISTANCE", "distance" },
    { PROP_DRAG_ZOOM_MODE_DURATION, "PROP_DRAG_ZOOM_MODE_DURATION", "duration" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { PROP_DRAG_ZOOM_MODE_DISTANCE, NC_("drag-zoom-mode", "By distance"), NULL },
    { PROP_DRAG_ZOOM_MODE_DURATION, NC_("drag-zoom-mode", "By duration"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaDragZoomMode", values);
      ligma_type_set_translation_context (type, "drag-zoom-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_space_bar_action_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_SPACE_BAR_ACTION_NONE, "LIGMA_SPACE_BAR_ACTION_NONE", "none" },
    { LIGMA_SPACE_BAR_ACTION_PAN, "LIGMA_SPACE_BAR_ACTION_PAN", "pan" },
    { LIGMA_SPACE_BAR_ACTION_MOVE, "LIGMA_SPACE_BAR_ACTION_MOVE", "move" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_SPACE_BAR_ACTION_NONE, NC_("space-bar-action", "No action"), NULL },
    { LIGMA_SPACE_BAR_ACTION_PAN, NC_("space-bar-action", "Pan view"), NULL },
    { LIGMA_SPACE_BAR_ACTION_MOVE, NC_("space-bar-action", "Switch to Move tool"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaSpaceBarAction", values);
      ligma_type_set_translation_context (type, "space-bar-action");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_window_hint_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_WINDOW_HINT_NORMAL, "LIGMA_WINDOW_HINT_NORMAL", "normal" },
    { LIGMA_WINDOW_HINT_UTILITY, "LIGMA_WINDOW_HINT_UTILITY", "utility" },
    { LIGMA_WINDOW_HINT_KEEP_ABOVE, "LIGMA_WINDOW_HINT_KEEP_ABOVE", "keep-above" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_WINDOW_HINT_NORMAL, NC_("window-hint", "Normal window"), NULL },
    { LIGMA_WINDOW_HINT_UTILITY, NC_("window-hint", "Utility window"), NULL },
    { LIGMA_WINDOW_HINT_KEEP_ABOVE, NC_("window-hint", "Keep above"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaWindowHint", values);
      ligma_type_set_translation_context (type, "window-hint");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_zoom_quality_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_ZOOM_QUALITY_LOW, "LIGMA_ZOOM_QUALITY_LOW", "low" },
    { LIGMA_ZOOM_QUALITY_HIGH, "LIGMA_ZOOM_QUALITY_HIGH", "high" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_ZOOM_QUALITY_LOW, NC_("zoom-quality", "Low"), NULL },
    { LIGMA_ZOOM_QUALITY_HIGH, NC_("zoom-quality", "High"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaZoomQuality", values);
      ligma_type_set_translation_context (type, "zoom-quality");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

