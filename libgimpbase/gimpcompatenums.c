
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "gimpbasetypes.h"
#include "gimpcompatenums.h"
#include "libgimp/libgimp-intl.h"

/* enumerations from "gimpcompatenums.h" */
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
gimp_channel_type_compat_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_RED_CHANNEL, "GIMP_RED_CHANNEL", "red-channel" },
    { GIMP_GREEN_CHANNEL, "GIMP_GREEN_CHANNEL", "green-channel" },
    { GIMP_BLUE_CHANNEL, "GIMP_BLUE_CHANNEL", "blue-channel" },
    { GIMP_GRAY_CHANNEL, "GIMP_GRAY_CHANNEL", "gray-channel" },
    { GIMP_INDEXED_CHANNEL, "GIMP_INDEXED_CHANNEL", "indexed-channel" },
    { GIMP_ALPHA_CHANNEL, "GIMP_ALPHA_CHANNEL", "alpha-channel" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_RED_CHANNEL, "GIMP_RED_CHANNEL", NULL },
    { GIMP_GREEN_CHANNEL, "GIMP_GREEN_CHANNEL", NULL },
    { GIMP_BLUE_CHANNEL, "GIMP_BLUE_CHANNEL", NULL },
    { GIMP_GRAY_CHANNEL, "GIMP_GRAY_CHANNEL", NULL },
    { GIMP_INDEXED_CHANNEL, "GIMP_INDEXED_CHANNEL", NULL },
    { GIMP_ALPHA_CHANNEL, "GIMP_ALPHA_CHANNEL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpChannelTypeCompat", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "channel-type-compat");
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
gimp_convert_dither_type_compat_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_NO_DITHER, "GIMP_NO_DITHER", "no-dither" },
    { GIMP_FS_DITHER, "GIMP_FS_DITHER", "fs-dither" },
    { GIMP_FSLOWBLEED_DITHER, "GIMP_FSLOWBLEED_DITHER", "fslowbleed-dither" },
    { GIMP_FIXED_DITHER, "GIMP_FIXED_DITHER", "fixed-dither" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_NO_DITHER, "GIMP_NO_DITHER", NULL },
    { GIMP_FS_DITHER, "GIMP_FS_DITHER", NULL },
    { GIMP_FSLOWBLEED_DITHER, "GIMP_FSLOWBLEED_DITHER", NULL },
    { GIMP_FIXED_DITHER, "GIMP_FIXED_DITHER", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpConvertDitherTypeCompat", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "convert-dither-type-compat");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_convert_palette_type_compat_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_MAKE_PALETTE, "GIMP_MAKE_PALETTE", "make-palette" },
    { GIMP_REUSE_PALETTE, "GIMP_REUSE_PALETTE", "reuse-palette" },
    { GIMP_WEB_PALETTE, "GIMP_WEB_PALETTE", "web-palette" },
    { GIMP_MONO_PALETTE, "GIMP_MONO_PALETTE", "mono-palette" },
    { GIMP_CUSTOM_PALETTE, "GIMP_CUSTOM_PALETTE", "custom-palette" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_MAKE_PALETTE, "GIMP_MAKE_PALETTE", NULL },
    { GIMP_REUSE_PALETTE, "GIMP_REUSE_PALETTE", NULL },
    { GIMP_WEB_PALETTE, "GIMP_WEB_PALETTE", NULL },
    { GIMP_MONO_PALETTE, "GIMP_MONO_PALETTE", NULL },
    { GIMP_CUSTOM_PALETTE, "GIMP_CUSTOM_PALETTE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpConvertPaletteTypeCompat", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "convert-palette-type-compat");
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
gimp_desaturate_mode_compat_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_DESATURATE_LUMINOSITY, "GIMP_DESATURATE_LUMINOSITY", "luminosity" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_DESATURATE_LUMINOSITY, "GIMP_DESATURATE_LUMINOSITY", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpDesaturateModeCompat", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "desaturate-mode-compat");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_dodge_burn_type_compat_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_DODGE, "GIMP_DODGE", "dodge" },
    { GIMP_BURN, "GIMP_BURN", "burn" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_DODGE, "GIMP_DODGE", NULL },
    { GIMP_BURN, "GIMP_BURN", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpDodgeBurnTypeCompat", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "dodge-burn-type-compat");
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

GType
gimp_hue_range_compat_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_ALL_HUES, "GIMP_ALL_HUES", "all-hues" },
    { GIMP_RED_HUES, "GIMP_RED_HUES", "red-hues" },
    { GIMP_YELLOW_HUES, "GIMP_YELLOW_HUES", "yellow-hues" },
    { GIMP_GREEN_HUES, "GIMP_GREEN_HUES", "green-hues" },
    { GIMP_CYAN_HUES, "GIMP_CYAN_HUES", "cyan-hues" },
    { GIMP_BLUE_HUES, "GIMP_BLUE_HUES", "blue-hues" },
    { GIMP_MAGENTA_HUES, "GIMP_MAGENTA_HUES", "magenta-hues" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_ALL_HUES, "GIMP_ALL_HUES", NULL },
    { GIMP_RED_HUES, "GIMP_RED_HUES", NULL },
    { GIMP_YELLOW_HUES, "GIMP_YELLOW_HUES", NULL },
    { GIMP_GREEN_HUES, "GIMP_GREEN_HUES", NULL },
    { GIMP_CYAN_HUES, "GIMP_CYAN_HUES", NULL },
    { GIMP_BLUE_HUES, "GIMP_BLUE_HUES", NULL },
    { GIMP_MAGENTA_HUES, "GIMP_MAGENTA_HUES", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpHueRangeCompat", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "hue-range-compat");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_icon_type_compat_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_ICON_TYPE_STOCK_ID, "GIMP_ICON_TYPE_STOCK_ID", "id" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_ICON_TYPE_STOCK_ID, "GIMP_ICON_TYPE_STOCK_ID", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpIconTypeCompat", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "icon-type-compat");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_interpolation_type_compat_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_INTERPOLATION_LANCZOS, "GIMP_INTERPOLATION_LANCZOS", "lanczos" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_INTERPOLATION_LANCZOS, "GIMP_INTERPOLATION_LANCZOS", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpInterpolationTypeCompat", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "interpolation-type-compat");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_layer_mode_effects_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_NORMAL_MODE, "GIMP_NORMAL_MODE", "normal-mode" },
    { GIMP_DISSOLVE_MODE, "GIMP_DISSOLVE_MODE", "dissolve-mode" },
    { GIMP_BEHIND_MODE, "GIMP_BEHIND_MODE", "behind-mode" },
    { GIMP_MULTIPLY_MODE, "GIMP_MULTIPLY_MODE", "multiply-mode" },
    { GIMP_SCREEN_MODE, "GIMP_SCREEN_MODE", "screen-mode" },
    { GIMP_OVERLAY_MODE, "GIMP_OVERLAY_MODE", "overlay-mode" },
    { GIMP_DIFFERENCE_MODE, "GIMP_DIFFERENCE_MODE", "difference-mode" },
    { GIMP_ADDITION_MODE, "GIMP_ADDITION_MODE", "addition-mode" },
    { GIMP_SUBTRACT_MODE, "GIMP_SUBTRACT_MODE", "subtract-mode" },
    { GIMP_DARKEN_ONLY_MODE, "GIMP_DARKEN_ONLY_MODE", "darken-only-mode" },
    { GIMP_LIGHTEN_ONLY_MODE, "GIMP_LIGHTEN_ONLY_MODE", "lighten-only-mode" },
    { GIMP_HUE_MODE, "GIMP_HUE_MODE", "hue-mode" },
    { GIMP_SATURATION_MODE, "GIMP_SATURATION_MODE", "saturation-mode" },
    { GIMP_COLOR_MODE, "GIMP_COLOR_MODE", "color-mode" },
    { GIMP_VALUE_MODE, "GIMP_VALUE_MODE", "value-mode" },
    { GIMP_DIVIDE_MODE, "GIMP_DIVIDE_MODE", "divide-mode" },
    { GIMP_DODGE_MODE, "GIMP_DODGE_MODE", "dodge-mode" },
    { GIMP_BURN_MODE, "GIMP_BURN_MODE", "burn-mode" },
    { GIMP_HARDLIGHT_MODE, "GIMP_HARDLIGHT_MODE", "hardlight-mode" },
    { GIMP_SOFTLIGHT_MODE, "GIMP_SOFTLIGHT_MODE", "softlight-mode" },
    { GIMP_GRAIN_EXTRACT_MODE, "GIMP_GRAIN_EXTRACT_MODE", "grain-extract-mode" },
    { GIMP_GRAIN_MERGE_MODE, "GIMP_GRAIN_MERGE_MODE", "grain-merge-mode" },
    { GIMP_COLOR_ERASE_MODE, "GIMP_COLOR_ERASE_MODE", "color-erase-mode" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_NORMAL_MODE, "GIMP_NORMAL_MODE", NULL },
    { GIMP_DISSOLVE_MODE, "GIMP_DISSOLVE_MODE", NULL },
    { GIMP_BEHIND_MODE, "GIMP_BEHIND_MODE", NULL },
    { GIMP_MULTIPLY_MODE, "GIMP_MULTIPLY_MODE", NULL },
    { GIMP_SCREEN_MODE, "GIMP_SCREEN_MODE", NULL },
    { GIMP_OVERLAY_MODE, "GIMP_OVERLAY_MODE", NULL },
    { GIMP_DIFFERENCE_MODE, "GIMP_DIFFERENCE_MODE", NULL },
    { GIMP_ADDITION_MODE, "GIMP_ADDITION_MODE", NULL },
    { GIMP_SUBTRACT_MODE, "GIMP_SUBTRACT_MODE", NULL },
    { GIMP_DARKEN_ONLY_MODE, "GIMP_DARKEN_ONLY_MODE", NULL },
    { GIMP_LIGHTEN_ONLY_MODE, "GIMP_LIGHTEN_ONLY_MODE", NULL },
    { GIMP_HUE_MODE, "GIMP_HUE_MODE", NULL },
    { GIMP_SATURATION_MODE, "GIMP_SATURATION_MODE", NULL },
    { GIMP_COLOR_MODE, "GIMP_COLOR_MODE", NULL },
    { GIMP_VALUE_MODE, "GIMP_VALUE_MODE", NULL },
    { GIMP_DIVIDE_MODE, "GIMP_DIVIDE_MODE", NULL },
    { GIMP_DODGE_MODE, "GIMP_DODGE_MODE", NULL },
    { GIMP_BURN_MODE, "GIMP_BURN_MODE", NULL },
    { GIMP_HARDLIGHT_MODE, "GIMP_HARDLIGHT_MODE", NULL },
    { GIMP_SOFTLIGHT_MODE, "GIMP_SOFTLIGHT_MODE", NULL },
    { GIMP_GRAIN_EXTRACT_MODE, "GIMP_GRAIN_EXTRACT_MODE", NULL },
    { GIMP_GRAIN_MERGE_MODE, "GIMP_GRAIN_MERGE_MODE", NULL },
    { GIMP_COLOR_ERASE_MODE, "GIMP_COLOR_ERASE_MODE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpLayerModeEffects", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "layer-mode-effects");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_transfer_mode_compat_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_SHADOWS, "GIMP_SHADOWS", "shadows" },
    { GIMP_MIDTONES, "GIMP_MIDTONES", "midtones" },
    { GIMP_HIGHLIGHTS, "GIMP_HIGHLIGHTS", "highlights" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_SHADOWS, "GIMP_SHADOWS", NULL },
    { GIMP_MIDTONES, "GIMP_MIDTONES", NULL },
    { GIMP_HIGHLIGHTS, "GIMP_HIGHLIGHTS", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpTransferModeCompat", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "transfer-mode-compat");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

