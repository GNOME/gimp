
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <gtk/gtk.h>
#include "widgets-enums.h"
#include "gimp-intl.h"

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


/* Generated data ends here */

