
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "display-enums.h"
#include"gimp-intl.h"

/* enumerations from "./display-enums.h" */

static const GEnumValue gimp_cursor_mode_enum_values[] =
{
  { GIMP_CURSOR_MODE_TOOL_ICON, N_("Tool icon"), "tool-icon" },
  { GIMP_CURSOR_MODE_TOOL_CROSSHAIR, N_("Tool icon with crosshair"), "tool-crosshair" },
  { GIMP_CURSOR_MODE_CROSSHAIR, N_("Crosshair only"), "crosshair" },
  { 0, NULL, NULL }
};

GType
gimp_cursor_mode_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpCursorMode", gimp_cursor_mode_enum_values);

  return enum_type;
}


static const GEnumValue gimp_canvas_padding_mode_enum_values[] =
{
  { GIMP_CANVAS_PADDING_MODE_DEFAULT, N_("From theme"), "default" },
  { GIMP_CANVAS_PADDING_MODE_LIGHT_CHECK, N_("Light check color"), "light-check" },
  { GIMP_CANVAS_PADDING_MODE_DARK_CHECK, N_("Dark check color"), "dark-check" },
  { GIMP_CANVAS_PADDING_MODE_CUSTOM, N_("Custom color"), "custom" },
  { 0, NULL, NULL }
};

GType
gimp_canvas_padding_mode_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpCanvasPaddingMode", gimp_canvas_padding_mode_enum_values);

  return enum_type;
}


/* Generated data ends here */

