
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "text-enums.h"
#include "libgimp/gimpintl.h"

/* enumerations from "./text-enums.h" */

static const GEnumValue gimp_text_alignment_enum_values[] =
{
  { GIMP_TEXT_ALIGNMENT_LEFT, N_("Left Aligned"), "left" },
  { GIMP_TEXT_ALIGNMENT_CENTER, N_("Centered"), "center" },
  { GIMP_TEXT_ALIGNMENT_RIGHT, N_("Right Aligned"), "right" },
  { 0, NULL, NULL }
};

GType
gimp_text_alignment_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpTextAlignment", gimp_text_alignment_enum_values);

  return enum_type;
}


/* Generated data ends here */

