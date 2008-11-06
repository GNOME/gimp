
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "libgimpbase/gimpbase.h"
#include "gimpwidgetsenums.h"
#include "libgimp/libgimp-intl.h"

/* enumerations from "./gimpwidgetsenums.h" */
GType
gimp_aspect_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_ASPECT_SQUARE, "GIMP_ASPECT_SQUARE", "square" },
    { GIMP_ASPECT_PORTRAIT, "GIMP_ASPECT_PORTRAIT", "portrait" },
    { GIMP_ASPECT_LANDSCAPE, "GIMP_ASPECT_LANDSCAPE", "landscape" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_ASPECT_SQUARE, NC_("aspect-type", "Square"), NULL },
    { GIMP_ASPECT_PORTRAIT, NC_("aspect-type", "Portrait"), NULL },
    { GIMP_ASPECT_LANDSCAPE, NC_("aspect-type", "Landscape"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpAspectType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "aspect-type");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_chain_position_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_CHAIN_TOP, "GIMP_CHAIN_TOP", "top" },
    { GIMP_CHAIN_LEFT, "GIMP_CHAIN_LEFT", "left" },
    { GIMP_CHAIN_BOTTOM, "GIMP_CHAIN_BOTTOM", "bottom" },
    { GIMP_CHAIN_RIGHT, "GIMP_CHAIN_RIGHT", "right" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_CHAIN_TOP, "GIMP_CHAIN_TOP", NULL },
    { GIMP_CHAIN_LEFT, "GIMP_CHAIN_LEFT", NULL },
    { GIMP_CHAIN_BOTTOM, "GIMP_CHAIN_BOTTOM", NULL },
    { GIMP_CHAIN_RIGHT, "GIMP_CHAIN_RIGHT", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpChainPosition", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "chain-position");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_color_area_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_COLOR_AREA_FLAT, "GIMP_COLOR_AREA_FLAT", "flat" },
    { GIMP_COLOR_AREA_SMALL_CHECKS, "GIMP_COLOR_AREA_SMALL_CHECKS", "small-checks" },
    { GIMP_COLOR_AREA_LARGE_CHECKS, "GIMP_COLOR_AREA_LARGE_CHECKS", "large-checks" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_COLOR_AREA_FLAT, "GIMP_COLOR_AREA_FLAT", NULL },
    { GIMP_COLOR_AREA_SMALL_CHECKS, "GIMP_COLOR_AREA_SMALL_CHECKS", NULL },
    { GIMP_COLOR_AREA_LARGE_CHECKS, "GIMP_COLOR_AREA_LARGE_CHECKS", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpColorAreaType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "color-area-type");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_color_selector_channel_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_COLOR_SELECTOR_HUE, "GIMP_COLOR_SELECTOR_HUE", "hue" },
    { GIMP_COLOR_SELECTOR_SATURATION, "GIMP_COLOR_SELECTOR_SATURATION", "saturation" },
    { GIMP_COLOR_SELECTOR_VALUE, "GIMP_COLOR_SELECTOR_VALUE", "value" },
    { GIMP_COLOR_SELECTOR_RED, "GIMP_COLOR_SELECTOR_RED", "red" },
    { GIMP_COLOR_SELECTOR_GREEN, "GIMP_COLOR_SELECTOR_GREEN", "green" },
    { GIMP_COLOR_SELECTOR_BLUE, "GIMP_COLOR_SELECTOR_BLUE", "blue" },
    { GIMP_COLOR_SELECTOR_ALPHA, "GIMP_COLOR_SELECTOR_ALPHA", "alpha" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_COLOR_SELECTOR_HUE, NC_("color-selector-channel", "_H"), N_("Hue") },
    { GIMP_COLOR_SELECTOR_SATURATION, NC_("color-selector-channel", "_S"), N_("Saturation") },
    { GIMP_COLOR_SELECTOR_VALUE, NC_("color-selector-channel", "_V"), N_("Value") },
    { GIMP_COLOR_SELECTOR_RED, NC_("color-selector-channel", "_R"), N_("Red") },
    { GIMP_COLOR_SELECTOR_GREEN, NC_("color-selector-channel", "_G"), N_("Green") },
    { GIMP_COLOR_SELECTOR_BLUE, NC_("color-selector-channel", "_B"), N_("Blue") },
    { GIMP_COLOR_SELECTOR_ALPHA, NC_("color-selector-channel", "_A"), N_("Alpha") },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpColorSelectorChannel", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "color-selector-channel");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_page_selector_target_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_PAGE_SELECTOR_TARGET_LAYERS, "GIMP_PAGE_SELECTOR_TARGET_LAYERS", "layers" },
    { GIMP_PAGE_SELECTOR_TARGET_IMAGES, "GIMP_PAGE_SELECTOR_TARGET_IMAGES", "images" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_PAGE_SELECTOR_TARGET_LAYERS, NC_("page-selector-target", "Layers"), NULL },
    { GIMP_PAGE_SELECTOR_TARGET_IMAGES, NC_("page-selector-target", "Images"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpPageSelectorTarget", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "page-selector-target");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_size_entry_update_policy_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_SIZE_ENTRY_UPDATE_NONE, "GIMP_SIZE_ENTRY_UPDATE_NONE", "none" },
    { GIMP_SIZE_ENTRY_UPDATE_SIZE, "GIMP_SIZE_ENTRY_UPDATE_SIZE", "size" },
    { GIMP_SIZE_ENTRY_UPDATE_RESOLUTION, "GIMP_SIZE_ENTRY_UPDATE_RESOLUTION", "resolution" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_SIZE_ENTRY_UPDATE_NONE, "GIMP_SIZE_ENTRY_UPDATE_NONE", NULL },
    { GIMP_SIZE_ENTRY_UPDATE_SIZE, "GIMP_SIZE_ENTRY_UPDATE_SIZE", NULL },
    { GIMP_SIZE_ENTRY_UPDATE_RESOLUTION, "GIMP_SIZE_ENTRY_UPDATE_RESOLUTION", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpSizeEntryUpdatePolicy", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "size-entry-update-policy");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_zoom_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_ZOOM_IN, "GIMP_ZOOM_IN", "in" },
    { GIMP_ZOOM_OUT, "GIMP_ZOOM_OUT", "out" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_ZOOM_IN, NC_("zoom-type", "Zoom in"), NULL },
    { GIMP_ZOOM_OUT, NC_("zoom-type", "Zoom out"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpZoomType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_type_set_translation_context (type, "zoom-type");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

