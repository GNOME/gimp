
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "gimpbasetypes.h"
#include "libgimp/libgimp-intl.h"

/* enumerations from "./gimpbaseenums.h" */
GType
gimp_check_size_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_CHECK_SIZE_SMALL_CHECKS, N_("Small"), "small-checks" },
    { GIMP_CHECK_SIZE_MEDIUM_CHECKS, N_("Medium"), "medium-checks" },
    { GIMP_CHECK_SIZE_LARGE_CHECKS, N_("Large"), "large-checks" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
  {
    type = g_enum_register_static ("GimpCheckSize", values);
    gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
  }

  return type;
}

GType
gimp_check_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_CHECK_TYPE_LIGHT_CHECKS, N_("Light Checks"), "light-checks" },
    { GIMP_CHECK_TYPE_GRAY_CHECKS, N_("Mid-Tone Checks"), "gray-checks" },
    { GIMP_CHECK_TYPE_DARK_CHECKS, N_("Dark Checks"), "dark-checks" },
    { GIMP_CHECK_TYPE_WHITE_ONLY, N_("White Only"), "white-only" },
    { GIMP_CHECK_TYPE_GRAY_ONLY, N_("Gray Only"), "gray-only" },
    { GIMP_CHECK_TYPE_BLACK_ONLY, N_("Black Only"), "black-only" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
  {
    type = g_enum_register_static ("GimpCheckType", values);
    gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
  }

  return type;
}

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

