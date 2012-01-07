
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "libgimpbase/gimpbase.h"
#include "paint-enums.h"
#include "gimp-intl.h"

/* enumerations from "./paint-enums.h" */
GType
gimp_brush_application_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_BRUSH_HARD, "GIMP_BRUSH_HARD", "hard" },
    { GIMP_BRUSH_SOFT, "GIMP_BRUSH_SOFT", "soft" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_BRUSH_HARD, "GIMP_BRUSH_HARD", NULL },
    { GIMP_BRUSH_SOFT, "GIMP_BRUSH_SOFT", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpBrushApplicationMode", values);
      gimp_type_set_translation_context (type, "brush-application-mode");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_perspective_clone_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_PERSPECTIVE_CLONE_MODE_ADJUST, "GIMP_PERSPECTIVE_CLONE_MODE_ADJUST", "adjust" },
    { GIMP_PERSPECTIVE_CLONE_MODE_PAINT, "GIMP_PERSPECTIVE_CLONE_MODE_PAINT", "paint" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_PERSPECTIVE_CLONE_MODE_ADJUST, NC_("perspective-clone-mode", "Modify Perspective"), NULL },
    { GIMP_PERSPECTIVE_CLONE_MODE_PAINT, NC_("perspective-clone-mode", "Perspective Clone"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpPerspectiveCloneMode", values);
      gimp_type_set_translation_context (type, "perspective-clone-mode");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_source_align_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_SOURCE_ALIGN_NO, "GIMP_SOURCE_ALIGN_NO", "no" },
    { GIMP_SOURCE_ALIGN_YES, "GIMP_SOURCE_ALIGN_YES", "yes" },
    { GIMP_SOURCE_ALIGN_REGISTERED, "GIMP_SOURCE_ALIGN_REGISTERED", "registered" },
    { GIMP_SOURCE_ALIGN_FIXED, "GIMP_SOURCE_ALIGN_FIXED", "fixed" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_SOURCE_ALIGN_NO, NC_("source-align-mode", "None"), NULL },
    { GIMP_SOURCE_ALIGN_YES, NC_("source-align-mode", "Aligned"), NULL },
    { GIMP_SOURCE_ALIGN_REGISTERED, NC_("source-align-mode", "Registered"), NULL },
    { GIMP_SOURCE_ALIGN_FIXED, NC_("source-align-mode", "Fixed"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpSourceAlignMode", values);
      gimp_type_set_translation_context (type, "source-align-mode");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_convolve_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_BLUR_CONVOLVE, "GIMP_BLUR_CONVOLVE", "blur-convolve" },
    { GIMP_SHARPEN_CONVOLVE, "GIMP_SHARPEN_CONVOLVE", "sharpen-convolve" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_BLUR_CONVOLVE, NC_("convolve-type", "Blur"), NULL },
    { GIMP_SHARPEN_CONVOLVE, NC_("convolve-type", "Sharpen"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpConvolveType", values);
      gimp_type_set_translation_context (type, "convolve-type");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_ink_blob_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_INK_BLOB_TYPE_CIRCLE, "GIMP_INK_BLOB_TYPE_CIRCLE", "circle" },
    { GIMP_INK_BLOB_TYPE_SQUARE, "GIMP_INK_BLOB_TYPE_SQUARE", "square" },
    { GIMP_INK_BLOB_TYPE_DIAMOND, "GIMP_INK_BLOB_TYPE_DIAMOND", "diamond" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_INK_BLOB_TYPE_CIRCLE, NC_("ink-blob-type", "Circle"), NULL },
    { GIMP_INK_BLOB_TYPE_SQUARE, NC_("ink-blob-type", "Square"), NULL },
    { GIMP_INK_BLOB_TYPE_DIAMOND, NC_("ink-blob-type", "Diamond"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpInkBlobType", values);
      gimp_type_set_translation_context (type, "ink-blob-type");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

