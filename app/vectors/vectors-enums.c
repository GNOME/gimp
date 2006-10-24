
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "libgimpbase/gimpbase.h"
#include "vectors-enums.h"
#include "gimp-intl.h"

/* enumerations from "./vectors-enums.h" */
GType
gimp_vectors_stroke_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_VECTORS_STROKE_TYPE_BEZIER, "GIMP_VECTORS_STROKE_TYPE_BEZIER", "bezier" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_VECTORS_STROKE_TYPE_BEZIER, "GIMP_VECTORS_STROKE_TYPE_BEZIER", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpVectorsStrokeType", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

