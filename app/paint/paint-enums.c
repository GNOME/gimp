
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "paint-enums.h"
#include "gimp-intl.h"

/* enumerations from "./paint-enums.h" */
GType
gimp_clone_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_IMAGE_CLONE, N_("Image source"), "image-clone" },
    { GIMP_PATTERN_CLONE, N_("Pattern source"), "pattern-clone" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpCloneType", values);

  return type;
}

GType
gimp_clone_align_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_CLONE_ALIGN_NO, N_("Non-aligned"), "no" },
    { GIMP_CLONE_ALIGN_YES, N_("Aligned"), "yes" },
    { GIMP_CLONE_ALIGN_REGISTERED, N_("Registered"), "registered" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpCloneAlignMode", values);

  return type;
}

GType
gimp_dodge_burn_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_DODGE, N_("Dodge"), "dodge" },
    { GIMP_BURN, N_("Burn"), "burn" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpDodgeBurnType", values);

  return type;
}

GType
gimp_convolve_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_BLUR_CONVOLVE, N_("Blur"), "blur-convolve" },
    { GIMP_SHARPEN_CONVOLVE, N_("Sharpen"), "sharpen-convolve" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpConvolveType", values);

  return type;
}

GType
gimp_ink_blob_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_INK_BLOB_TYPE_ELLIPSE, "GIMP_INK_BLOB_TYPE_ELLIPSE", "ellipse" },
    { GIMP_INK_BLOB_TYPE_SQUARE, "GIMP_INK_BLOB_TYPE_SQUARE", "square" },
    { GIMP_INK_BLOB_TYPE_DIAMOND, "GIMP_INK_BLOB_TYPE_DIAMOND", "diamond" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpInkBlobType", values);

  return type;
}

GType
gimp_paint_application_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_PAINT_CONSTANT, N_("Constant"), "constant" },
    { GIMP_PAINT_INCREMENTAL, N_("Incremental"), "incremental" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpPaintApplicationMode", values);

  return type;
}


/* Generated data ends here */

