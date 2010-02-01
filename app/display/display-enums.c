
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "libgimpbase/gimpbase.h"
#include "display-enums.h"
#include"gimp-intl.h"

/* enumerations from "./display-enums.h" */
GType
gimp_cursor_precision_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_CURSOR_PRECISION_PIXEL_CENTER, "GIMP_CURSOR_PRECISION_PIXEL_CENTER", "pixel-center" },
    { GIMP_CURSOR_PRECISION_PIXEL_BORDER, "GIMP_CURSOR_PRECISION_PIXEL_BORDER", "pixel-border" },
    { GIMP_CURSOR_PRECISION_SUBPIXEL, "GIMP_CURSOR_PRECISION_SUBPIXEL", "subpixel" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_CURSOR_PRECISION_PIXEL_CENTER, "GIMP_CURSOR_PRECISION_PIXEL_CENTER", NULL },
    { GIMP_CURSOR_PRECISION_PIXEL_BORDER, "GIMP_CURSOR_PRECISION_PIXEL_BORDER", NULL },
    { GIMP_CURSOR_PRECISION_SUBPIXEL, "GIMP_CURSOR_PRECISION_SUBPIXEL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpCursorPrecision", values);
      gimp_type_set_translation_context (type, "cursor-precision");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_zoom_focus_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_ZOOM_FOCUS_BEST_GUESS, "GIMP_ZOOM_FOCUS_BEST_GUESS", "best-guess" },
    { GIMP_ZOOM_FOCUS_POINTER, "GIMP_ZOOM_FOCUS_POINTER", "pointer" },
    { GIMP_ZOOM_FOCUS_IMAGE_CENTER, "GIMP_ZOOM_FOCUS_IMAGE_CENTER", "image-center" },
    { GIMP_ZOOM_FOCUS_RETAIN_CENTERING_ELSE_BEST_GUESS, "GIMP_ZOOM_FOCUS_RETAIN_CENTERING_ELSE_BEST_GUESS", "retain-centering-else-best-guess" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_ZOOM_FOCUS_BEST_GUESS, "GIMP_ZOOM_FOCUS_BEST_GUESS", NULL },
    { GIMP_ZOOM_FOCUS_POINTER, "GIMP_ZOOM_FOCUS_POINTER", NULL },
    { GIMP_ZOOM_FOCUS_IMAGE_CENTER, "GIMP_ZOOM_FOCUS_IMAGE_CENTER", NULL },
    { GIMP_ZOOM_FOCUS_RETAIN_CENTERING_ELSE_BEST_GUESS, "GIMP_ZOOM_FOCUS_RETAIN_CENTERING_ELSE_BEST_GUESS", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpZoomFocus", values);
      gimp_type_set_translation_context (type, "zoom-focus");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

