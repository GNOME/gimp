
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <gtk/gtk.h>
#include "widgets-enums.h"
#include "gimp-intl.h"

/* enumerations from "./widgets-enums.h" */

static const GEnumValue gimp_active_color_enum_values[] =
{
  { GIMP_ACTIVE_COLOR_FOREGROUND, N_("Foreground"), "foreground" },
  { GIMP_ACTIVE_COLOR_BACKGROUND, N_("Background"), "background" },
  { 0, NULL, NULL }
};

GType
gimp_active_color_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpActiveColor", gimp_active_color_enum_values);

  return enum_type;
}


static const GEnumValue gimp_aspect_type_enum_values[] =
{
  { GIMP_ASPECT_SQUARE, "GIMP_ASPECT_SQUARE", "square" },
  { GIMP_ASPECT_PORTRAIT, N_("Portrait"), "portrait" },
  { GIMP_ASPECT_LANDSCAPE, N_("Landscape"), "landscape" },
  { 0, NULL, NULL }
};

GType
gimp_aspect_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpAspectType", gimp_aspect_type_enum_values);

  return enum_type;
}


static const GEnumValue gimp_color_frame_mode_enum_values[] =
{
  { GIMP_COLOR_FRAME_MODE_PIXEL, N_("Pixel Values"), "pixel" },
  { GIMP_COLOR_FRAME_MODE_RGB, N_("RGB"), "rgb" },
  { GIMP_COLOR_FRAME_MODE_HSV, N_("HSV"), "hsv" },
  { GIMP_COLOR_FRAME_MODE_CMYK, N_("CMYK"), "cmyk" },
  { 0, NULL, NULL }
};

GType
gimp_color_frame_mode_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpColorFrameMode", gimp_color_frame_mode_enum_values);

  return enum_type;
}


static const GEnumValue gimp_cursor_format_enum_values[] =
{
  { GIMP_CURSOR_FORMAT_BITMAP, N_("Black & White"), "bitmap" },
  { GIMP_CURSOR_FORMAT_PIXBUF, N_("Fancy"), "pixbuf" },
  { 0, NULL, NULL }
};

GType
gimp_cursor_format_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpCursorFormat", gimp_cursor_format_enum_values);

  return enum_type;
}


static const GEnumValue gimp_help_browser_type_enum_values[] =
{
  { GIMP_HELP_BROWSER_GIMP, N_("Internal"), "gimp" },
  { GIMP_HELP_BROWSER_WEB_BROWSER, N_("Web Browser"), "web-browser" },
  { 0, NULL, NULL }
};

GType
gimp_help_browser_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpHelpBrowserType", gimp_help_browser_type_enum_values);

  return enum_type;
}


static const GEnumValue gimp_histogram_scale_enum_values[] =
{
  { GIMP_HISTOGRAM_SCALE_LINEAR, N_("Linear"), "linear" },
  { GIMP_HISTOGRAM_SCALE_LOGARITHMIC, N_("Logarithmic"), "logarithmic" },
  { 0, NULL, NULL }
};

GType
gimp_histogram_scale_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpHistogramScale", gimp_histogram_scale_enum_values);

  return enum_type;
}


static const GEnumValue gimp_tab_style_enum_values[] =
{
  { GIMP_TAB_STYLE_ICON, N_("Icon"), "icon" },
  { GIMP_TAB_STYLE_PREVIEW, N_("Current Status"), "preview" },
  { GIMP_TAB_STYLE_NAME, N_("Text"), "name" },
  { GIMP_TAB_STYLE_BLURB, N_("Description"), "blurb" },
  { GIMP_TAB_STYLE_ICON_NAME, N_("Icon & Text"), "icon-name" },
  { GIMP_TAB_STYLE_ICON_BLURB, N_("Icon & Desc"), "icon-blurb" },
  { GIMP_TAB_STYLE_PREVIEW_NAME, N_("Status & Text"), "preview-name" },
  { GIMP_TAB_STYLE_PREVIEW_BLURB, N_("Status & Desc"), "preview-blurb" },
  { 0, NULL, NULL }
};

GType
gimp_tab_style_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpTabStyle", gimp_tab_style_enum_values);

  return enum_type;
}


static const GEnumValue gimp_view_type_enum_values[] =
{
  { GIMP_VIEW_TYPE_LIST, N_("View as List"), "list" },
  { GIMP_VIEW_TYPE_GRID, N_("View as Grid"), "grid" },
  { 0, NULL, NULL }
};

GType
gimp_view_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpViewType", gimp_view_type_enum_values);

  return enum_type;
}


static const GEnumValue gimp_window_hint_enum_values[] =
{
  { GIMP_WINDOW_HINT_NORMAL, N_("Normal Window"), "normal" },
  { GIMP_WINDOW_HINT_UTILITY, N_("Utility Window"), "utility" },
  { GIMP_WINDOW_HINT_KEEP_ABOVE, N_("Keep Above"), "keep-above" },
  { 0, NULL, NULL }
};

GType
gimp_window_hint_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpWindowHint", gimp_window_hint_enum_values);

  return enum_type;
}


static const GEnumValue gimp_zoom_type_enum_values[] =
{
  { GIMP_ZOOM_IN, N_("Zoom in"), "in" },
  { GIMP_ZOOM_OUT, N_("Zoom out"), "out" },
  { 0, NULL, NULL }
};

GType
gimp_zoom_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpZoomType", gimp_zoom_type_enum_values);

  return enum_type;
}


/* Generated data ends here */

