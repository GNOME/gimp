
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "libgimpbase/gimpbase.h"
#include "text-enums.h"
#include "gimp-intl.h"

/* enumerations from "./text-enums.h" */
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
    { GIMP_TEXT_BOX_DYNAMIC, "GIMP_TEXT_BOX_DYNAMIC", NULL },
    { GIMP_TEXT_BOX_FIXED, "GIMP_TEXT_BOX_FIXED", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpTextBoxMode", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_text_direction_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_TEXT_DIRECTION_LTR, "GIMP_TEXT_DIRECTION_LTR", "ltr" },
    { GIMP_TEXT_DIRECTION_RTL, "GIMP_TEXT_DIRECTION_RTL", "rtl" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_TEXT_DIRECTION_LTR, N_("From left to right"), NULL },
    { GIMP_TEXT_DIRECTION_RTL, N_("From right to left"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpTextDirection", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_text_justification_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_TEXT_JUSTIFY_LEFT, "GIMP_TEXT_JUSTIFY_LEFT", "left" },
    { GIMP_TEXT_JUSTIFY_RIGHT, "GIMP_TEXT_JUSTIFY_RIGHT", "right" },
    { GIMP_TEXT_JUSTIFY_CENTER, "GIMP_TEXT_JUSTIFY_CENTER", "center" },
    { GIMP_TEXT_JUSTIFY_FILL, "GIMP_TEXT_JUSTIFY_FILL", "fill" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_TEXT_JUSTIFY_LEFT, N_("Left justified"), NULL },
    { GIMP_TEXT_JUSTIFY_RIGHT, N_("Right justified"), NULL },
    { GIMP_TEXT_JUSTIFY_CENTER, N_("Centered"), NULL },
    { GIMP_TEXT_JUSTIFY_FILL, N_("Filled"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpTextJustification", values);
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

  if (! type)
    {
      type = g_enum_register_static ("GimpTextOutline", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

