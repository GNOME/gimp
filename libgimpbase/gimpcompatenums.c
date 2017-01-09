
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "gimpbasetypes.h"
#include "gimpcompatenums.h"
#include "libgimp/libgimp-intl.h"

/* enumerations from "./gimpcompatenums.h" */
GType
gimp_add_mask_type_compat_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_ADD_WHITE_MASK, "GIMP_ADD_WHITE_MASK", "white-mask" },
    { GIMP_ADD_BLACK_MASK, "GIMP_ADD_BLACK_MASK", "black-mask" },
    { GIMP_ADD_ALPHA_MASK, "GIMP_ADD_ALPHA_MASK", "alpha-mask" },
    { GIMP_ADD_ALPHA_TRANSFER_MASK, "GIMP_ADD_ALPHA_TRANSFER_MASK", "alpha-transfer-mask" },
    { GIMP_ADD_SELECTION_MASK, "GIMP_ADD_SELECTION_MASK", "selection-mask" },
    { GIMP_ADD_COPY_MASK, "GIMP_ADD_COPY_MASK", "copy-mask" },
    { GIMP_ADD_CHANNEL_MASK, "GIMP_ADD_CHANNEL_MASK", "channel-mask" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_ADD_WHITE_MASK, "GIMP_ADD_WHITE_MASK", NULL },
    { GIMP_ADD_BLACK_MASK, "GIMP_ADD_BLACK_MASK", NULL },
    { GIMP_ADD_ALPHA_MASK, "GIMP_ADD_ALPHA_MASK", NULL },
    { GIMP_ADD_ALPHA_TRANSFER_MASK, "GIMP_ADD_ALPHA_TRANSFER_MASK", NULL },
    { GIMP_ADD_SELECTION_MASK, "GIMP_ADD_SELECTION_MASK", NULL },
    { GIMP_ADD_COPY_MASK, "GIMP_ADD_COPY_MASK", NULL },
    { GIMP_ADD_CHANNEL_MASK, "GIMP_ADD_CHANNEL_MASK", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpAddMaskTypeCompat", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "add-mask-type-compat");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_blend_mode_compat_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_FG_BG_RGB_MODE, "GIMP_FG_BG_RGB_MODE", "fg-bg-rgb-mode" },
    { GIMP_FG_BG_HSV_MODE, "GIMP_FG_BG_HSV_MODE", "fg-bg-hsv-mode" },
    { GIMP_FG_TRANSPARENT_MODE, "GIMP_FG_TRANSPARENT_MODE", "fg-transparent-mode" },
    { GIMP_CUSTOM_MODE, "GIMP_CUSTOM_MODE", "custom-mode" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_FG_BG_RGB_MODE, "GIMP_FG_BG_RGB_MODE", NULL },
    { GIMP_FG_BG_HSV_MODE, "GIMP_FG_BG_HSV_MODE", NULL },
    { GIMP_FG_TRANSPARENT_MODE, "GIMP_FG_TRANSPARENT_MODE", NULL },
    { GIMP_CUSTOM_MODE, "GIMP_CUSTOM_MODE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpBlendModeCompat", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "blend-mode-compat");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_bucket_fill_mode_compat_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_FG_BUCKET_FILL, "GIMP_FG_BUCKET_FILL", "fg-bucket-fill" },
    { GIMP_BG_BUCKET_FILL, "GIMP_BG_BUCKET_FILL", "bg-bucket-fill" },
    { GIMP_PATTERN_BUCKET_FILL, "GIMP_PATTERN_BUCKET_FILL", "pattern-bucket-fill" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_FG_BUCKET_FILL, "GIMP_FG_BUCKET_FILL", NULL },
    { GIMP_BG_BUCKET_FILL, "GIMP_BG_BUCKET_FILL", NULL },
    { GIMP_PATTERN_BUCKET_FILL, "GIMP_PATTERN_BUCKET_FILL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpBucketFillModeCompat", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "bucket-fill-mode-compat");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_clone_type_compat_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_IMAGE_CLONE, "GIMP_IMAGE_CLONE", "image-clone" },
    { GIMP_PATTERN_CLONE, "GIMP_PATTERN_CLONE", "pattern-clone" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_IMAGE_CLONE, "GIMP_IMAGE_CLONE", NULL },
    { GIMP_PATTERN_CLONE, "GIMP_PATTERN_CLONE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpCloneTypeCompat", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "clone-type-compat");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_convolve_type_compat_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_BLUR_CONVOLVE, "GIMP_BLUR_CONVOLVE", "blur-convolve" },
    { GIMP_SHARPEN_CONVOLVE, "GIMP_SHARPEN_CONVOLVE", "sharpen-convolve" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_BLUR_CONVOLVE, "GIMP_BLUR_CONVOLVE", NULL },
    { GIMP_SHARPEN_CONVOLVE, "GIMP_SHARPEN_CONVOLVE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpConvolveTypeCompat", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "convolve-type-compat");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_fill_type_compat_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_FOREGROUND_FILL, "GIMP_FOREGROUND_FILL", "foreground-fill" },
    { GIMP_BACKGROUND_FILL, "GIMP_BACKGROUND_FILL", "background-fill" },
    { GIMP_WHITE_FILL, "GIMP_WHITE_FILL", "white-fill" },
    { GIMP_TRANSPARENT_FILL, "GIMP_TRANSPARENT_FILL", "transparent-fill" },
    { GIMP_PATTERN_FILL, "GIMP_PATTERN_FILL", "pattern-fill" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_FOREGROUND_FILL, "GIMP_FOREGROUND_FILL", NULL },
    { GIMP_BACKGROUND_FILL, "GIMP_BACKGROUND_FILL", NULL },
    { GIMP_WHITE_FILL, "GIMP_WHITE_FILL", NULL },
    { GIMP_TRANSPARENT_FILL, "GIMP_TRANSPARENT_FILL", NULL },
    { GIMP_PATTERN_FILL, "GIMP_PATTERN_FILL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpFillTypeCompat", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "fill-type-compat");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

