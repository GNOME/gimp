
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "text-enums.h"
#include "libgimp/gimpintl.h"

/* enumerations from "./text-enums.h" */

static const GEnumValue gimp_text_justification_enum_values[] =
{
  { GIMP_TEXT_JUSTIFY_LEFT, N_("Left Justified"), "left" },
  { GIMP_TEXT_JUSTIFY_RIGHT, N_("Right Justified"), "right" },
  { GIMP_TEXT_JUSTIFY_CENTER, N_("Centered"), "center" },
  { GIMP_TEXT_JUSTIFY_FILL, N_("Filled"), "fill" },
  { 0, NULL, NULL }
};

GType
gimp_text_justification_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpTextJustification", gimp_text_justification_enum_values);

  return enum_type;
}


/* Generated data ends here */

