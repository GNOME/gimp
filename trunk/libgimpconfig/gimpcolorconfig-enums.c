
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "libgimpbase/gimpbasetypes.h"
#include "gimpcolorconfig-enums.h"
#include "libgimp/libgimp-intl.h"

/* enumerations from "./gimpcolorconfig-enums.h" */
GType
gimp_color_management_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_COLOR_MANAGEMENT_OFF, "GIMP_COLOR_MANAGEMENT_OFF", "off" },
    { GIMP_COLOR_MANAGEMENT_DISPLAY, "GIMP_COLOR_MANAGEMENT_DISPLAY", "display" },
    { GIMP_COLOR_MANAGEMENT_SOFTPROOF, "GIMP_COLOR_MANAGEMENT_SOFTPROOF", "softproof" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_COLOR_MANAGEMENT_OFF, N_("No color management"), NULL },
    { GIMP_COLOR_MANAGEMENT_DISPLAY, N_("Color managed display"), NULL },
    { GIMP_COLOR_MANAGEMENT_SOFTPROOF, N_("Print simulation"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpColorManagementMode", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_color_rendering_intent_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL, "GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL", "perceptual" },
    { GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC, "GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC", "relative-colorimetric" },
    { GIMP_COLOR_RENDERING_INTENT_SATURATION, "GIMP_COLOR_RENDERING_INTENT_SATURATION", "saturation" },
    { GIMP_COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC, "GIMP_COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC", "absolute-colorimetric" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL, N_("Perceptual"), NULL },
    { GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC, N_("Relative colorimetric"), NULL },
    { GIMP_COLOR_RENDERING_INTENT_SATURATION, N_("intent|Saturation"), NULL },
    { GIMP_COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC, N_("Absolute colorimetric"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpColorRenderingIntent", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

