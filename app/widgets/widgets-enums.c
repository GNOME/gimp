
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <gtk/gtk.h>
#include "widgets-enums.h"
#include "gimp-intl.h"

/* enumerations from "./widgets-enums.h" */
GType
gimp_active_color_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_ACTIVE_COLOR_FOREGROUND, N_("Foreground"), "foreground" },
    { GIMP_ACTIVE_COLOR_BACKGROUND, N_("Background"), "background" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpActiveColor", values);

  return type;
}

GType
gimp_aspect_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_ASPECT_SQUARE, "GIMP_ASPECT_SQUARE", "square" },
    { GIMP_ASPECT_PORTRAIT, N_("Portrait"), "portrait" },
    { GIMP_ASPECT_LANDSCAPE, N_("Landscape"), "landscape" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpAspectType", values);

  return type;
}

GType
gimp_color_dialog_state_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_COLOR_DIALOG_OK, "GIMP_COLOR_DIALOG_OK", "ok" },
    { GIMP_COLOR_DIALOG_CANCEL, "GIMP_COLOR_DIALOG_CANCEL", "cancel" },
    { GIMP_COLOR_DIALOG_UPDATE, "GIMP_COLOR_DIALOG_UPDATE", "update" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpColorDialogState", values);

  return type;
}

GType
gimp_color_frame_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_COLOR_FRAME_MODE_PIXEL, N_("Pixel values"), "pixel" },
    { GIMP_COLOR_FRAME_MODE_RGB, N_("RGB"), "rgb" },
    { GIMP_COLOR_FRAME_MODE_HSV, N_("HSV"), "hsv" },
    { GIMP_COLOR_FRAME_MODE_CMYK, N_("CMYK"), "cmyk" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpColorFrameMode", values);

  return type;
}

GType
gimp_color_pick_state_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_COLOR_PICK_STATE_NEW, "GIMP_COLOR_PICK_STATE_NEW", "new" },
    { GIMP_COLOR_PICK_STATE_UPDATE, "GIMP_COLOR_PICK_STATE_UPDATE", "update" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpColorPickState", values);

  return type;
}

GType
gimp_cursor_format_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_CURSOR_FORMAT_BITMAP, N_("Black & white"), "bitmap" },
    { GIMP_CURSOR_FORMAT_PIXBUF, N_("Fancy"), "pixbuf" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpCursorFormat", values);

  return type;
}

GType
gimp_help_browser_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_HELP_BROWSER_GIMP, N_("GIMP help browser"), "gimp" },
    { GIMP_HELP_BROWSER_WEB_BROWSER, N_("Web browser"), "web-browser" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpHelpBrowserType", values);

  return type;
}

GType
gimp_histogram_scale_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_HISTOGRAM_SCALE_LINEAR, N_("Linear"), "linear" },
    { GIMP_HISTOGRAM_SCALE_LOGARITHMIC, N_("Logarithmic"), "logarithmic" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpHistogramScale", values);

  return type;
}

GType
gimp_tab_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_TAB_STYLE_ICON, N_("Icon"), "icon" },
    { GIMP_TAB_STYLE_PREVIEW, N_("Current status"), "preview" },
    { GIMP_TAB_STYLE_NAME, N_("Text"), "name" },
    { GIMP_TAB_STYLE_BLURB, N_("Description"), "blurb" },
    { GIMP_TAB_STYLE_ICON_NAME, N_("Icon & text"), "icon-name" },
    { GIMP_TAB_STYLE_ICON_BLURB, N_("Icon & desc"), "icon-blurb" },
    { GIMP_TAB_STYLE_PREVIEW_NAME, N_("Status & text"), "preview-name" },
    { GIMP_TAB_STYLE_PREVIEW_BLURB, N_("Status & desc"), "preview-blurb" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpTabStyle", values);

  return type;
}

GType
gimp_view_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_VIEW_TYPE_LIST, N_("View as list"), "list" },
    { GIMP_VIEW_TYPE_GRID, N_("View as grid"), "grid" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpViewType", values);

  return type;
}

GType
gimp_window_hint_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_WINDOW_HINT_NORMAL, N_("Normal window"), "normal" },
    { GIMP_WINDOW_HINT_UTILITY, N_("Utility window"), "utility" },
    { GIMP_WINDOW_HINT_KEEP_ABOVE, N_("Keep above"), "keep-above" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpWindowHint", values);

  return type;
}

GType
gimp_zoom_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_ZOOM_IN, N_("Zoom in"), "in" },
    { GIMP_ZOOM_OUT, N_("Zoom out"), "out" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpZoomType", values);

  return type;
}


/* Generated data ends here */

