
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "display-enums.h"
#include"libgimp/gimpintl.h"

/* enumerations from "./display-enums.h" */

static const GEnumValue gimp_cursor_mode_enum_values[] =
{
  { GIMP_CURSOR_MODE_TOOL_ICON, N_("Tool Icon"), "tool-icon" },
  { GIMP_CURSOR_MODE_TOOL_CROSSHAIR, N_("Tool Icon with Crosshair"), "tool-crosshair" },
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


/* Generated data ends here */

