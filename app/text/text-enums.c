
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <gio/gio.h>
#include "libgimpbase/gimpbase.h"
#include "text-enums.h"
#include "gimp-intl.h"

/* enumerations from "text-enums.h" */
GType
gimp_text_box_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_TEXT_BOX_DYNAMIC, "GIMP_TEXT_BOX_DYNAMIC", "dynamic" },
    { GIMP_TEXT_BOX_FIXED, "GIMP_TEXT_BOX_FIXED", "fixed" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_TEXT_BOX_DYNAMIC, NC_("text-box-mode", "Dynamic"), NULL },
    { GIMP_TEXT_BOX_FIXED, NC_("text-box-mode", "Fixed"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpTextBoxMode", values);
      gimp_type_set_translation_context (type, "text-box-mode");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_text_outline_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_TEXT_OUTLINE_NONE, "GIMP_TEXT_OUTLINE_NONE", "none" },
    { GIMP_TEXT_OUTLINE_STROKE_ONLY, "GIMP_TEXT_OUTLINE_STROKE_ONLY", "stroke-only" },
    { GIMP_TEXT_OUTLINE_STROKE_FILL, "GIMP_TEXT_OUTLINE_STROKE_FILL", "stroke-fill" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_TEXT_OUTLINE_NONE, "GIMP_TEXT_OUTLINE_NONE", NULL },
    { GIMP_TEXT_OUTLINE_STROKE_ONLY, "GIMP_TEXT_OUTLINE_STROKE_ONLY", NULL },
    { GIMP_TEXT_OUTLINE_STROKE_FILL, "GIMP_TEXT_OUTLINE_STROKE_FILL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpTextOutline", values);
      gimp_type_set_translation_context (type, "text-outline");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

