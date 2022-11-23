
/* Generated data (by ligma-mkenums) */

#include "stamp-text-enums.h"
#include "config.h"
#include <gio/gio.h>
#include "libligmabase/ligmabase.h"
#include "text-enums.h"
#include "ligma-intl.h"

/* enumerations from "text-enums.h" */
GType
ligma_text_box_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_TEXT_BOX_DYNAMIC, "LIGMA_TEXT_BOX_DYNAMIC", "dynamic" },
    { LIGMA_TEXT_BOX_FIXED, "LIGMA_TEXT_BOX_FIXED", "fixed" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_TEXT_BOX_DYNAMIC, NC_("text-box-mode", "Dynamic"), NULL },
    { LIGMA_TEXT_BOX_FIXED, NC_("text-box-mode", "Fixed"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaTextBoxMode", values);
      ligma_type_set_translation_context (type, "text-box-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_text_outline_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_TEXT_OUTLINE_NONE, "LIGMA_TEXT_OUTLINE_NONE", "none" },
    { LIGMA_TEXT_OUTLINE_STROKE_ONLY, "LIGMA_TEXT_OUTLINE_STROKE_ONLY", "stroke-only" },
    { LIGMA_TEXT_OUTLINE_STROKE_FILL, "LIGMA_TEXT_OUTLINE_STROKE_FILL", "stroke-fill" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_TEXT_OUTLINE_NONE, NC_("text-outline", "Filled"), NULL },
    { LIGMA_TEXT_OUTLINE_STROKE_ONLY, NC_("text-outline", "Outlined"), NULL },
    { LIGMA_TEXT_OUTLINE_STROKE_FILL, NC_("text-outline", "Outlined and filled"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaTextOutline", values);
      ligma_type_set_translation_context (type, "text-outline");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

