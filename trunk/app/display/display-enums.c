
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "libgimpbase/gimpbase.h"
#include "display-enums.h"
#include"gimp-intl.h"

/* enumerations from "./display-enums.h" */
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
    { GIMP_CURSOR_MODE_TOOL_ICON, N_("Tool icon"), NULL },
    { GIMP_CURSOR_MODE_TOOL_CROSSHAIR, N_("Tool icon with crosshair"), NULL },
    { GIMP_CURSOR_MODE_CROSSHAIR, N_("Crosshair only"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpCursorMode", values);
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
    { GIMP_CANVAS_PADDING_MODE_DEFAULT, N_("From theme"), NULL },
    { GIMP_CANVAS_PADDING_MODE_LIGHT_CHECK, N_("Light check color"), NULL },
    { GIMP_CANVAS_PADDING_MODE_DARK_CHECK, N_("Dark check color"), NULL },
    { GIMP_CANVAS_PADDING_MODE_CUSTOM, N_("Custom color"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpCanvasPaddingMode", values);
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
    { GIMP_SPACE_BAR_ACTION_NONE, N_("No action"), NULL },
    { GIMP_SPACE_BAR_ACTION_PAN, N_("Pan view"), NULL },
    { GIMP_SPACE_BAR_ACTION_MOVE, N_("Switch to Move tool"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpSpaceBarAction", values);
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
    { GIMP_ZOOM_QUALITY_LOW, N_("quality|Low"), NULL },
    { GIMP_ZOOM_QUALITY_HIGH, N_("quality|High"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpZoomQuality", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

