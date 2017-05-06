
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <gio/gio.h>
#include "libgimpbase/gimpbase.h"
#include "gimpconfigenums.h"
#include "libgimp/libgimp-intl.h"

/* enumerations from "gimpconfigenums.h" */
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
    { GIMP_COLOR_MANAGEMENT_OFF, NC_("color-management-mode", "No color management"), NULL },
    { GIMP_COLOR_MANAGEMENT_DISPLAY, NC_("color-management-mode", "Color-managed display"), NULL },
    { GIMP_COLOR_MANAGEMENT_SOFTPROOF, NC_("color-management-mode", "Soft-proofing"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpColorManagementMode", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "color-management-mode");
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
    { GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL, NC_("color-rendering-intent", "Perceptual"), NULL },
    { GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC, NC_("color-rendering-intent", "Relative colorimetric"), NULL },
    { GIMP_COLOR_RENDERING_INTENT_SATURATION, NC_("color-rendering-intent", "Saturation"), NULL },
    { GIMP_COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC, NC_("color-rendering-intent", "Absolute colorimetric"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpColorRenderingIntent", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "color-rendering-intent");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

