
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "gimpbasetypes.h"
#include "libgimp/libgimp-intl.h"

/* enumerations from "./gimpbaseenums.h" */
GType
gimp_image_base_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_RGB, N_("RGB color"), "rgb" },
    { GIMP_GRAY, N_("Grayscale"), "gray" },
    { GIMP_INDEXED, N_("Indexed color"), "indexed" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
  {
    type = g_enum_register_static ("GimpImageBaseType", values);
    gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
  }

  return type;
}

GType
gimp_image_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_RGB_IMAGE, N_("RGB"), "rgb-image" },
    { GIMP_RGBA_IMAGE, N_("RGB-alpha"), "rgba-image" },
    { GIMP_GRAY_IMAGE, N_("Grayscale"), "gray-image" },
    { GIMP_GRAYA_IMAGE, N_("Grayscale-alpha"), "graya-image" },
    { GIMP_INDEXED_IMAGE, N_("Indexed"), "indexed-image" },
    { GIMP_INDEXEDA_IMAGE, N_("Indexed-alpha"), "indexeda-image" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
  {
    type = g_enum_register_static ("GimpImageType", values);
    gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
  }

  return type;
}


/* Generated data ends here */

