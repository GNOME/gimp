
/* Generated data (by ligma-mkenums) */

#include "stamp-widgets-enums.h"
#include "config.h"
#include <gtk/gtk.h>
#include "libligmabase/ligmabase.h"
#include "widgets-enums.h"
#include "ligma-intl.h"

/* enumerations from "widgets-enums.h" */
GType
ligma_active_color_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_ACTIVE_COLOR_FOREGROUND, "LIGMA_ACTIVE_COLOR_FOREGROUND", "foreground" },
    { LIGMA_ACTIVE_COLOR_BACKGROUND, "LIGMA_ACTIVE_COLOR_BACKGROUND", "background" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_ACTIVE_COLOR_FOREGROUND, NC_("active-color", "Foreground"), NULL },
    { LIGMA_ACTIVE_COLOR_BACKGROUND, NC_("active-color", "Background"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaActiveColor", values);
      ligma_type_set_translation_context (type, "active-color");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_circle_background_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_CIRCLE_BACKGROUND_PLAIN, "LIGMA_CIRCLE_BACKGROUND_PLAIN", "plain" },
    { LIGMA_CIRCLE_BACKGROUND_HSV, "LIGMA_CIRCLE_BACKGROUND_HSV", "hsv" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_CIRCLE_BACKGROUND_PLAIN, NC_("circle-background", "Plain"), NULL },
    { LIGMA_CIRCLE_BACKGROUND_HSV, NC_("circle-background", "HSV"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaCircleBackground", values);
      ligma_type_set_translation_context (type, "circle-background");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_color_dialog_state_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_COLOR_DIALOG_OK, "LIGMA_COLOR_DIALOG_OK", "ok" },
    { LIGMA_COLOR_DIALOG_CANCEL, "LIGMA_COLOR_DIALOG_CANCEL", "cancel" },
    { LIGMA_COLOR_DIALOG_UPDATE, "LIGMA_COLOR_DIALOG_UPDATE", "update" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_COLOR_DIALOG_OK, "LIGMA_COLOR_DIALOG_OK", NULL },
    { LIGMA_COLOR_DIALOG_CANCEL, "LIGMA_COLOR_DIALOG_CANCEL", NULL },
    { LIGMA_COLOR_DIALOG_UPDATE, "LIGMA_COLOR_DIALOG_UPDATE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaColorDialogState", values);
      ligma_type_set_translation_context (type, "color-dialog-state");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_color_pick_target_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_COLOR_PICK_TARGET_NONE, "LIGMA_COLOR_PICK_TARGET_NONE", "none" },
    { LIGMA_COLOR_PICK_TARGET_FOREGROUND, "LIGMA_COLOR_PICK_TARGET_FOREGROUND", "foreground" },
    { LIGMA_COLOR_PICK_TARGET_BACKGROUND, "LIGMA_COLOR_PICK_TARGET_BACKGROUND", "background" },
    { LIGMA_COLOR_PICK_TARGET_PALETTE, "LIGMA_COLOR_PICK_TARGET_PALETTE", "palette" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_COLOR_PICK_TARGET_NONE, NC_("color-pick-target", "Pick only"), NULL },
    { LIGMA_COLOR_PICK_TARGET_FOREGROUND, NC_("color-pick-target", "Set foreground color"), NULL },
    { LIGMA_COLOR_PICK_TARGET_BACKGROUND, NC_("color-pick-target", "Set background color"), NULL },
    { LIGMA_COLOR_PICK_TARGET_PALETTE, NC_("color-pick-target", "Add to palette"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaColorPickTarget", values);
      ligma_type_set_translation_context (type, "color-pick-target");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_color_pick_state_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_COLOR_PICK_STATE_START, "LIGMA_COLOR_PICK_STATE_START", "start" },
    { LIGMA_COLOR_PICK_STATE_UPDATE, "LIGMA_COLOR_PICK_STATE_UPDATE", "update" },
    { LIGMA_COLOR_PICK_STATE_END, "LIGMA_COLOR_PICK_STATE_END", "end" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_COLOR_PICK_STATE_START, "LIGMA_COLOR_PICK_STATE_START", NULL },
    { LIGMA_COLOR_PICK_STATE_UPDATE, "LIGMA_COLOR_PICK_STATE_UPDATE", NULL },
    { LIGMA_COLOR_PICK_STATE_END, "LIGMA_COLOR_PICK_STATE_END", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaColorPickState", values);
      ligma_type_set_translation_context (type, "color-pick-state");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_histogram_scale_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_HISTOGRAM_SCALE_LINEAR, "LIGMA_HISTOGRAM_SCALE_LINEAR", "linear" },
    { LIGMA_HISTOGRAM_SCALE_LOGARITHMIC, "LIGMA_HISTOGRAM_SCALE_LOGARITHMIC", "logarithmic" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_HISTOGRAM_SCALE_LINEAR, NC_("histogram-scale", "Linear histogram"), NULL },
    { LIGMA_HISTOGRAM_SCALE_LOGARITHMIC, NC_("histogram-scale", "Logarithmic histogram"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaHistogramScale", values);
      ligma_type_set_translation_context (type, "histogram-scale");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_tab_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_TAB_STYLE_ICON, "LIGMA_TAB_STYLE_ICON", "icon" },
    { LIGMA_TAB_STYLE_PREVIEW, "LIGMA_TAB_STYLE_PREVIEW", "preview" },
    { LIGMA_TAB_STYLE_NAME, "LIGMA_TAB_STYLE_NAME", "name" },
    { LIGMA_TAB_STYLE_BLURB, "LIGMA_TAB_STYLE_BLURB", "blurb" },
    { LIGMA_TAB_STYLE_ICON_NAME, "LIGMA_TAB_STYLE_ICON_NAME", "icon-name" },
    { LIGMA_TAB_STYLE_ICON_BLURB, "LIGMA_TAB_STYLE_ICON_BLURB", "icon-blurb" },
    { LIGMA_TAB_STYLE_PREVIEW_NAME, "LIGMA_TAB_STYLE_PREVIEW_NAME", "preview-name" },
    { LIGMA_TAB_STYLE_PREVIEW_BLURB, "LIGMA_TAB_STYLE_PREVIEW_BLURB", "preview-blurb" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_TAB_STYLE_ICON, NC_("tab-style", "Icon"), NULL },
    { LIGMA_TAB_STYLE_PREVIEW, NC_("tab-style", "Current status"), NULL },
    { LIGMA_TAB_STYLE_NAME, NC_("tab-style", "Text"), NULL },
    { LIGMA_TAB_STYLE_BLURB, NC_("tab-style", "Description"), NULL },
    { LIGMA_TAB_STYLE_ICON_NAME, NC_("tab-style", "Icon & text"), NULL },
    { LIGMA_TAB_STYLE_ICON_BLURB, NC_("tab-style", "Icon & desc"), NULL },
    { LIGMA_TAB_STYLE_PREVIEW_NAME, NC_("tab-style", "Status & text"), NULL },
    { LIGMA_TAB_STYLE_PREVIEW_BLURB, NC_("tab-style", "Status & desc"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaTabStyle", values);
      ligma_type_set_translation_context (type, "tab-style");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_tag_entry_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_TAG_ENTRY_MODE_QUERY, "LIGMA_TAG_ENTRY_MODE_QUERY", "query" },
    { LIGMA_TAG_ENTRY_MODE_ASSIGN, "LIGMA_TAG_ENTRY_MODE_ASSIGN", "assign" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_TAG_ENTRY_MODE_QUERY, "LIGMA_TAG_ENTRY_MODE_QUERY", NULL },
    { LIGMA_TAG_ENTRY_MODE_ASSIGN, "LIGMA_TAG_ENTRY_MODE_ASSIGN", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaTagEntryMode", values);
      ligma_type_set_translation_context (type, "tag-entry-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

