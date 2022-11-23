
/* Generated data (by ligma-mkenums) */

#include "stamp-ligma-gegl-enums.h"
#include "config.h"
#include <gio/gio.h>
#include "libligmabase/ligmabase.h"
#include "core/core-enums.h"
#include "ligma-gegl-enums.h"
#include "ligma-intl.h"

/* enumerations from "ligma-gegl-enums.h" */
GType
ligma_cage_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_CAGE_MODE_CAGE_CHANGE, "LIGMA_CAGE_MODE_CAGE_CHANGE", "cage-change" },
    { LIGMA_CAGE_MODE_DEFORM, "LIGMA_CAGE_MODE_DEFORM", "deform" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_CAGE_MODE_CAGE_CHANGE, NC_("cage-mode", "Create or adjust the cage"), NULL },
    { LIGMA_CAGE_MODE_DEFORM, NC_("cage-mode", "Deform the cage\nto deform the image"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaCageMode", values);
      ligma_type_set_translation_context (type, "cage-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

