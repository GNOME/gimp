
/* Generated data (by ligma-mkenums) */

#include "stamp-ligmawidgetsenums.h"
#include "config.h"
#include <gio/gio.h>
#include "libligmabase/ligmabase.h"
#include "ligmawidgetsenums.h"
#include "libligma/libligma-intl.h"

/* enumerations from "ligmawidgetsenums.h" */
GType
ligma_aspect_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_ASPECT_SQUARE, "LIGMA_ASPECT_SQUARE", "square" },
    { LIGMA_ASPECT_PORTRAIT, "LIGMA_ASPECT_PORTRAIT", "portrait" },
    { LIGMA_ASPECT_LANDSCAPE, "LIGMA_ASPECT_LANDSCAPE", "landscape" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_ASPECT_SQUARE, NC_("aspect-type", "Square"), NULL },
    { LIGMA_ASPECT_PORTRAIT, NC_("aspect-type", "Portrait"), NULL },
    { LIGMA_ASPECT_LANDSCAPE, NC_("aspect-type", "Landscape"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaAspectType", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "aspect-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_chain_position_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_CHAIN_TOP, "LIGMA_CHAIN_TOP", "top" },
    { LIGMA_CHAIN_LEFT, "LIGMA_CHAIN_LEFT", "left" },
    { LIGMA_CHAIN_BOTTOM, "LIGMA_CHAIN_BOTTOM", "bottom" },
    { LIGMA_CHAIN_RIGHT, "LIGMA_CHAIN_RIGHT", "right" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_CHAIN_TOP, "LIGMA_CHAIN_TOP", NULL },
    { LIGMA_CHAIN_LEFT, "LIGMA_CHAIN_LEFT", NULL },
    { LIGMA_CHAIN_BOTTOM, "LIGMA_CHAIN_BOTTOM", NULL },
    { LIGMA_CHAIN_RIGHT, "LIGMA_CHAIN_RIGHT", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaChainPosition", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "chain-position");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_color_area_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_COLOR_AREA_FLAT, "LIGMA_COLOR_AREA_FLAT", "flat" },
    { LIGMA_COLOR_AREA_SMALL_CHECKS, "LIGMA_COLOR_AREA_SMALL_CHECKS", "small-checks" },
    { LIGMA_COLOR_AREA_LARGE_CHECKS, "LIGMA_COLOR_AREA_LARGE_CHECKS", "large-checks" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_COLOR_AREA_FLAT, "LIGMA_COLOR_AREA_FLAT", NULL },
    { LIGMA_COLOR_AREA_SMALL_CHECKS, "LIGMA_COLOR_AREA_SMALL_CHECKS", NULL },
    { LIGMA_COLOR_AREA_LARGE_CHECKS, "LIGMA_COLOR_AREA_LARGE_CHECKS", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaColorAreaType", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "color-area-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_color_selector_channel_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_COLOR_SELECTOR_HUE, "LIGMA_COLOR_SELECTOR_HUE", "hue" },
    { LIGMA_COLOR_SELECTOR_SATURATION, "LIGMA_COLOR_SELECTOR_SATURATION", "saturation" },
    { LIGMA_COLOR_SELECTOR_VALUE, "LIGMA_COLOR_SELECTOR_VALUE", "value" },
    { LIGMA_COLOR_SELECTOR_RED, "LIGMA_COLOR_SELECTOR_RED", "red" },
    { LIGMA_COLOR_SELECTOR_GREEN, "LIGMA_COLOR_SELECTOR_GREEN", "green" },
    { LIGMA_COLOR_SELECTOR_BLUE, "LIGMA_COLOR_SELECTOR_BLUE", "blue" },
    { LIGMA_COLOR_SELECTOR_ALPHA, "LIGMA_COLOR_SELECTOR_ALPHA", "alpha" },
    { LIGMA_COLOR_SELECTOR_LCH_LIGHTNESS, "LIGMA_COLOR_SELECTOR_LCH_LIGHTNESS", "lch-lightness" },
    { LIGMA_COLOR_SELECTOR_LCH_CHROMA, "LIGMA_COLOR_SELECTOR_LCH_CHROMA", "lch-chroma" },
    { LIGMA_COLOR_SELECTOR_LCH_HUE, "LIGMA_COLOR_SELECTOR_LCH_HUE", "lch-hue" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_COLOR_SELECTOR_HUE, NC_("color-selector-channel", "_H"), N_("HSV Hue") },
    { LIGMA_COLOR_SELECTOR_SATURATION, NC_("color-selector-channel", "_S"), N_("HSV Saturation") },
    { LIGMA_COLOR_SELECTOR_VALUE, NC_("color-selector-channel", "_V"), N_("HSV Value") },
    { LIGMA_COLOR_SELECTOR_RED, NC_("color-selector-channel", "_R"), N_("Red") },
    { LIGMA_COLOR_SELECTOR_GREEN, NC_("color-selector-channel", "_G"), N_("Green") },
    { LIGMA_COLOR_SELECTOR_BLUE, NC_("color-selector-channel", "_B"), N_("Blue") },
    { LIGMA_COLOR_SELECTOR_ALPHA, NC_("color-selector-channel", "_A"), N_("Alpha") },
    { LIGMA_COLOR_SELECTOR_LCH_LIGHTNESS, NC_("color-selector-channel", "_L"), N_("LCh Lightness") },
    { LIGMA_COLOR_SELECTOR_LCH_CHROMA, NC_("color-selector-channel", "_C"), N_("LCh Chroma") },
    { LIGMA_COLOR_SELECTOR_LCH_HUE, NC_("color-selector-channel", "_h"), N_("LCh Hue") },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaColorSelectorChannel", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "color-selector-channel");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_color_selector_model_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_COLOR_SELECTOR_MODEL_RGB, "LIGMA_COLOR_SELECTOR_MODEL_RGB", "rgb" },
    { LIGMA_COLOR_SELECTOR_MODEL_LCH, "LIGMA_COLOR_SELECTOR_MODEL_LCH", "lch" },
    { LIGMA_COLOR_SELECTOR_MODEL_HSV, "LIGMA_COLOR_SELECTOR_MODEL_HSV", "hsv" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_COLOR_SELECTOR_MODEL_RGB, NC_("color-selector-model", "RGB"), N_("RGB color model") },
    { LIGMA_COLOR_SELECTOR_MODEL_LCH, NC_("color-selector-model", "LCH"), N_("CIE LCh color model") },
    { LIGMA_COLOR_SELECTOR_MODEL_HSV, NC_("color-selector-model", "HSV"), N_("HSV color model") },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaColorSelectorModel", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "color-selector-model");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_int_combo_box_layout_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_INT_COMBO_BOX_LAYOUT_ICON_ONLY, "LIGMA_INT_COMBO_BOX_LAYOUT_ICON_ONLY", "icon-only" },
    { LIGMA_INT_COMBO_BOX_LAYOUT_ABBREVIATED, "LIGMA_INT_COMBO_BOX_LAYOUT_ABBREVIATED", "abbreviated" },
    { LIGMA_INT_COMBO_BOX_LAYOUT_FULL, "LIGMA_INT_COMBO_BOX_LAYOUT_FULL", "full" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_INT_COMBO_BOX_LAYOUT_ICON_ONLY, "LIGMA_INT_COMBO_BOX_LAYOUT_ICON_ONLY", NULL },
    { LIGMA_INT_COMBO_BOX_LAYOUT_ABBREVIATED, "LIGMA_INT_COMBO_BOX_LAYOUT_ABBREVIATED", NULL },
    { LIGMA_INT_COMBO_BOX_LAYOUT_FULL, "LIGMA_INT_COMBO_BOX_LAYOUT_FULL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaIntComboBoxLayout", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "int-combo-box-layout");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_page_selector_target_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_PAGE_SELECTOR_TARGET_LAYERS, "LIGMA_PAGE_SELECTOR_TARGET_LAYERS", "layers" },
    { LIGMA_PAGE_SELECTOR_TARGET_IMAGES, "LIGMA_PAGE_SELECTOR_TARGET_IMAGES", "images" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_PAGE_SELECTOR_TARGET_LAYERS, NC_("page-selector-target", "Layers"), NULL },
    { LIGMA_PAGE_SELECTOR_TARGET_IMAGES, NC_("page-selector-target", "Images"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaPageSelectorTarget", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "page-selector-target");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_size_entry_update_policy_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_SIZE_ENTRY_UPDATE_NONE, "LIGMA_SIZE_ENTRY_UPDATE_NONE", "none" },
    { LIGMA_SIZE_ENTRY_UPDATE_SIZE, "LIGMA_SIZE_ENTRY_UPDATE_SIZE", "size" },
    { LIGMA_SIZE_ENTRY_UPDATE_RESOLUTION, "LIGMA_SIZE_ENTRY_UPDATE_RESOLUTION", "resolution" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_SIZE_ENTRY_UPDATE_NONE, "LIGMA_SIZE_ENTRY_UPDATE_NONE", NULL },
    { LIGMA_SIZE_ENTRY_UPDATE_SIZE, "LIGMA_SIZE_ENTRY_UPDATE_SIZE", NULL },
    { LIGMA_SIZE_ENTRY_UPDATE_RESOLUTION, "LIGMA_SIZE_ENTRY_UPDATE_RESOLUTION", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaSizeEntryUpdatePolicy", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "size-entry-update-policy");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_zoom_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_ZOOM_IN, "LIGMA_ZOOM_IN", "in" },
    { LIGMA_ZOOM_OUT, "LIGMA_ZOOM_OUT", "out" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_ZOOM_IN, NC_("zoom-type", "Zoom in"), NULL },
    { LIGMA_ZOOM_OUT, NC_("zoom-type", "Zoom out"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaZoomType", values);
      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE "-libligma");
      ligma_type_set_translation_context (type, "zoom-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

