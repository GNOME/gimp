
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <gtk/gtk.h>
#include "widgets-enums.h"
#include "libgimp/gimpintl.h"

/* enumerations from "./widgets-enums.h" */

static const GEnumValue gimp_help_browser_type_enum_values[] =
{
  { GIMP_HELP_BROWSER_GIMP, N_("Internal"), "gimp" },
  { GIMP_HELP_BROWSER_NETSCAPE, N_("Netscape"), "netscape" },
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

