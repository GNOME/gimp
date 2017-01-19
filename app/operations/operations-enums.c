
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <gio/gio.h>
#include "libgimpbase/gimpbase.h"
#include "operations-enums.h"
#include "gimp-intl.h"

/* enumerations from "./operations-enums.h" */
GType
gimp_layer_color_space_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_LAYER_COLOR_SPACE_RGB_LINEAR, "GIMP_LAYER_COLOR_SPACE_RGB_LINEAR", "rgb-linear" },
    { GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL, "GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL", "rgb-perceptual" },
    { GIMP_LAYER_COLOR_SPACE_LAB, "GIMP_LAYER_COLOR_SPACE_LAB", "lab" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_LAYER_COLOR_SPACE_RGB_LINEAR, "GIMP_LAYER_COLOR_SPACE_RGB_LINEAR", NULL },
    { GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL, "GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL", NULL },
    { GIMP_LAYER_COLOR_SPACE_LAB, "GIMP_LAYER_COLOR_SPACE_LAB", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpLayerColorSpace", values);
      gimp_type_set_translation_context (type, "layer-color-space");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_layer_composite_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_LAYER_COMPOSITE_SRC_ATOP, "GIMP_LAYER_COMPOSITE_SRC_ATOP", "src-atop" },
    { GIMP_LAYER_COMPOSITE_SRC_OVER, "GIMP_LAYER_COMPOSITE_SRC_OVER", "src-over" },
    { GIMP_LAYER_COMPOSITE_SRC_IN, "GIMP_LAYER_COMPOSITE_SRC_IN", "src-in" },
    { GIMP_LAYER_COMPOSITE_DST_ATOP, "GIMP_LAYER_COMPOSITE_DST_ATOP", "dst-atop" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_LAYER_COMPOSITE_SRC_ATOP, "GIMP_LAYER_COMPOSITE_SRC_ATOP", NULL },
    { GIMP_LAYER_COMPOSITE_SRC_OVER, "GIMP_LAYER_COMPOSITE_SRC_OVER", NULL },
    { GIMP_LAYER_COMPOSITE_SRC_IN, "GIMP_LAYER_COMPOSITE_SRC_IN", NULL },
    { GIMP_LAYER_COMPOSITE_DST_ATOP, "GIMP_LAYER_COMPOSITE_DST_ATOP", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpLayerCompositeMode", values);
      gimp_type_set_translation_context (type, "layer-composite-mode");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

