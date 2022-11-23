
/* Generated data (by ligma-mkenums) */

#include "stamp-ligmaconfigenums.h"
#include "config.h"
#include <gio/gio.h>
#include "libligmabase/ligmabase.h"
#include "ligmaconfigenums.h"
#include "libligma/libligma-intl.h"

/* enumerations from "ligmaconfigenums.h" */
GType
ligma_color_management_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_COLOR_MANAGEMENT_OFF, "LIGMA_COLOR_MANAGEMENT_OFF", "off" },
    { LIGMA_COLOR_MANAGEMENT_DISPLAY, "LIGMA_COLOR_MANAGEMENT_DISPLAY", "display" },
    { LIGMA_COLOR_MANAGEMENT_SOFTPROOF, "LIGMA_COLOR_MANAGEMENT_SOFTPROOF", "softproof" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_COLOR_MANAGEMENT_OFF, NC_("color-management-mode", "No color management"), NULL },
    { LIGMA_COLOR_MANAGEMENT_DISPLAY, NC_("color-management-mode", "Color-managed display"), NULL },
    { LIGMA_COLOR_MANAGEMENT_SOFTPROOF, NC_("color-management-mode", "Soft-proofing"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaColorManagementMode", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "color-management-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_color_rendering_intent_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_COLOR_RENDERING_INTENT_PERCEPTUAL, "LIGMA_COLOR_RENDERING_INTENT_PERCEPTUAL", "perceptual" },
    { LIGMA_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC, "LIGMA_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC", "relative-colorimetric" },
    { LIGMA_COLOR_RENDERING_INTENT_SATURATION, "LIGMA_COLOR_RENDERING_INTENT_SATURATION", "saturation" },
    { LIGMA_COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC, "LIGMA_COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC", "absolute-colorimetric" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_COLOR_RENDERING_INTENT_PERCEPTUAL, NC_("color-rendering-intent", "Perceptual"), NULL },
    { LIGMA_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC, NC_("color-rendering-intent", "Relative colorimetric"), NULL },
    { LIGMA_COLOR_RENDERING_INTENT_SATURATION, NC_("color-rendering-intent", "Saturation"), NULL },
    { LIGMA_COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC, NC_("color-rendering-intent", "Absolute colorimetric"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaColorRenderingIntent", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "color-rendering-intent");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

