
/* Generated data (by gimp-mkenums) */

#include "stamp-gimp-gegl-enums.h"
#include "config.h"
#include <gio/gio.h>
#include "libgimpbase/gimpbase.h"
#include "core/core-enums.h"
#include "gimp-gegl-enums.h"
#include "gimp-intl.h"

/* enumerations from "gimp-gegl-enums.h" */
GType
gimp_cage_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_CAGE_MODE_CAGE_CHANGE, "GIMP_CAGE_MODE_CAGE_CHANGE", "cage-change" },
    { GIMP_CAGE_MODE_DEFORM, "GIMP_CAGE_MODE_DEFORM", "deform" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_CAGE_MODE_CAGE_CHANGE, NC_("cage-mode", "Create or adjust the cage"), NULL },
    { GIMP_CAGE_MODE_DEFORM, NC_("cage-mode", "Deform the cage\nto deform the image"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpCageMode", values);
      gimp_type_set_translation_context (type, "cage-mode");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

