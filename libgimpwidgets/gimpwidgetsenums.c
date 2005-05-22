
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "libgimpbase/gimpbase.h"
#include "gimpwidgetsenums.h"
#include "libgimp/libgimp-intl.h"

/* enumerations from "./gimpwidgetsenums.h" */
GType
gimp_color_area_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_COLOR_AREA_FLAT, "GIMP_COLOR_AREA_FLAT", "flat" },
    { GIMP_COLOR_AREA_SMALL_CHECKS, "GIMP_COLOR_AREA_SMALL_CHECKS", "small-checks" },
    { GIMP_COLOR_AREA_LARGE_CHECKS, "GIMP_COLOR_AREA_LARGE_CHECKS", "large-checks" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_COLOR_AREA_FLAT, "GIMP_COLOR_AREA_FLAT", NULL },
    { GIMP_COLOR_AREA_SMALL_CHECKS, "GIMP_COLOR_AREA_SMALL_CHECKS", NULL },
    { GIMP_COLOR_AREA_LARGE_CHECKS, "GIMP_COLOR_AREA_LARGE_CHECKS", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpColorAreaType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_color_selector_channel_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_COLOR_SELECTOR_HUE, "GIMP_COLOR_SELECTOR_HUE", "hue" },
    { GIMP_COLOR_SELECTOR_SATURATION, "GIMP_COLOR_SELECTOR_SATURATION", "saturation" },
    { GIMP_COLOR_SELECTOR_VALUE, "GIMP_COLOR_SELECTOR_VALUE", "value" },
    { GIMP_COLOR_SELECTOR_RED, "GIMP_COLOR_SELECTOR_RED", "red" },
    { GIMP_COLOR_SELECTOR_GREEN, "GIMP_COLOR_SELECTOR_GREEN", "green" },
    { GIMP_COLOR_SELECTOR_BLUE, "GIMP_COLOR_SELECTOR_BLUE", "blue" },
    { GIMP_COLOR_SELECTOR_ALPHA, "GIMP_COLOR_SELECTOR_ALPHA", "alpha" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_COLOR_SELECTOR_HUE, N_("_H"), N_("Hue") },
    { GIMP_COLOR_SELECTOR_SATURATION, N_("_S"), N_("Saturation") },
    { GIMP_COLOR_SELECTOR_VALUE, N_("_V"), N_("Value") },
    { GIMP_COLOR_SELECTOR_RED, N_("_R"), N_("Red") },
    { GIMP_COLOR_SELECTOR_GREEN, N_("_G"), N_("Green") },
    { GIMP_COLOR_SELECTOR_BLUE, N_("_B"), N_("Blue") },
    { GIMP_COLOR_SELECTOR_ALPHA, N_("_A"), N_("Alpha") },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpColorSelectorChannel", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

