
/* Generated data (by ligma-mkenums) */

#include "stamp-tools-enums.h"
#include "config.h"
#include <gio/gio.h>
#include "libligmabase/ligmabase.h"
#include "core/core-enums.h"
#include "tools-enums.h"
#include "ligma-intl.h"

/* enumerations from "tools-enums.h" */
GType
ligma_bucket_fill_area_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_BUCKET_FILL_SELECTION, "LIGMA_BUCKET_FILL_SELECTION", "selection" },
    { LIGMA_BUCKET_FILL_SIMILAR_COLORS, "LIGMA_BUCKET_FILL_SIMILAR_COLORS", "similar-colors" },
    { LIGMA_BUCKET_FILL_LINE_ART, "LIGMA_BUCKET_FILL_LINE_ART", "line-art" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_BUCKET_FILL_SELECTION, NC_("bucket-fill-area", "Fill whole selection"), NULL },
    { LIGMA_BUCKET_FILL_SIMILAR_COLORS, NC_("bucket-fill-area", "Fill similar colors"), NULL },
    { LIGMA_BUCKET_FILL_LINE_ART, NC_("bucket-fill-area", "Fill by line art detection"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaBucketFillArea", values);
      ligma_type_set_translation_context (type, "bucket-fill-area");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_line_art_source_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_LINE_ART_SOURCE_SAMPLE_MERGED, "LIGMA_LINE_ART_SOURCE_SAMPLE_MERGED", "sample-merged" },
    { LIGMA_LINE_ART_SOURCE_ACTIVE_LAYER, "LIGMA_LINE_ART_SOURCE_ACTIVE_LAYER", "active-layer" },
    { LIGMA_LINE_ART_SOURCE_LOWER_LAYER, "LIGMA_LINE_ART_SOURCE_LOWER_LAYER", "lower-layer" },
    { LIGMA_LINE_ART_SOURCE_UPPER_LAYER, "LIGMA_LINE_ART_SOURCE_UPPER_LAYER", "upper-layer" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_LINE_ART_SOURCE_SAMPLE_MERGED, NC_("line-art-source", "All visible layers"), NULL },
    { LIGMA_LINE_ART_SOURCE_ACTIVE_LAYER, NC_("line-art-source", "Selected layer"), NULL },
    { LIGMA_LINE_ART_SOURCE_LOWER_LAYER, NC_("line-art-source", "Layer below the selected one"), NULL },
    { LIGMA_LINE_ART_SOURCE_UPPER_LAYER, NC_("line-art-source", "Layer above the selected one"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaLineArtSource", values);
      ligma_type_set_translation_context (type, "line-art-source");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_rect_select_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_RECT_SELECT_MODE_FREE, "LIGMA_RECT_SELECT_MODE_FREE", "free" },
    { LIGMA_RECT_SELECT_MODE_FIXED_SIZE, "LIGMA_RECT_SELECT_MODE_FIXED_SIZE", "fixed-size" },
    { LIGMA_RECT_SELECT_MODE_FIXED_RATIO, "LIGMA_RECT_SELECT_MODE_FIXED_RATIO", "fixed-ratio" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_RECT_SELECT_MODE_FREE, NC_("rect-select-mode", "Free select"), NULL },
    { LIGMA_RECT_SELECT_MODE_FIXED_SIZE, NC_("rect-select-mode", "Fixed size"), NULL },
    { LIGMA_RECT_SELECT_MODE_FIXED_RATIO, NC_("rect-select-mode", "Fixed aspect ratio"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaRectSelectMode", values);
      ligma_type_set_translation_context (type, "rect-select-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_transform_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_TRANSFORM_TYPE_LAYER, "LIGMA_TRANSFORM_TYPE_LAYER", "layer" },
    { LIGMA_TRANSFORM_TYPE_SELECTION, "LIGMA_TRANSFORM_TYPE_SELECTION", "selection" },
    { LIGMA_TRANSFORM_TYPE_PATH, "LIGMA_TRANSFORM_TYPE_PATH", "path" },
    { LIGMA_TRANSFORM_TYPE_IMAGE, "LIGMA_TRANSFORM_TYPE_IMAGE", "image" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_TRANSFORM_TYPE_LAYER, NC_("transform-type", "Layer"), NULL },
    { LIGMA_TRANSFORM_TYPE_SELECTION, NC_("transform-type", "Selection"), NULL },
    { LIGMA_TRANSFORM_TYPE_PATH, NC_("transform-type", "Path"), NULL },
    { LIGMA_TRANSFORM_TYPE_IMAGE, NC_("transform-type", "Image"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaTransformType", values);
      ligma_type_set_translation_context (type, "transform-type");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_tool_action_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_TOOL_ACTION_PAUSE, "LIGMA_TOOL_ACTION_PAUSE", "pause" },
    { LIGMA_TOOL_ACTION_RESUME, "LIGMA_TOOL_ACTION_RESUME", "resume" },
    { LIGMA_TOOL_ACTION_HALT, "LIGMA_TOOL_ACTION_HALT", "halt" },
    { LIGMA_TOOL_ACTION_COMMIT, "LIGMA_TOOL_ACTION_COMMIT", "commit" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_TOOL_ACTION_PAUSE, "LIGMA_TOOL_ACTION_PAUSE", NULL },
    { LIGMA_TOOL_ACTION_RESUME, "LIGMA_TOOL_ACTION_RESUME", NULL },
    { LIGMA_TOOL_ACTION_HALT, "LIGMA_TOOL_ACTION_HALT", NULL },
    { LIGMA_TOOL_ACTION_COMMIT, "LIGMA_TOOL_ACTION_COMMIT", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaToolAction", values);
      ligma_type_set_translation_context (type, "tool-action");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_tool_active_modifiers_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_TOOL_ACTIVE_MODIFIERS_OFF, "LIGMA_TOOL_ACTIVE_MODIFIERS_OFF", "off" },
    { LIGMA_TOOL_ACTIVE_MODIFIERS_SAME, "LIGMA_TOOL_ACTIVE_MODIFIERS_SAME", "same" },
    { LIGMA_TOOL_ACTIVE_MODIFIERS_SEPARATE, "LIGMA_TOOL_ACTIVE_MODIFIERS_SEPARATE", "separate" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_TOOL_ACTIVE_MODIFIERS_OFF, "LIGMA_TOOL_ACTIVE_MODIFIERS_OFF", NULL },
    { LIGMA_TOOL_ACTIVE_MODIFIERS_SAME, "LIGMA_TOOL_ACTIVE_MODIFIERS_SAME", NULL },
    { LIGMA_TOOL_ACTIVE_MODIFIERS_SEPARATE, "LIGMA_TOOL_ACTIVE_MODIFIERS_SEPARATE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaToolActiveModifiers", values);
      ligma_type_set_translation_context (type, "tool-active-modifiers");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_matting_draw_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_MATTING_DRAW_MODE_FOREGROUND, "LIGMA_MATTING_DRAW_MODE_FOREGROUND", "foreground" },
    { LIGMA_MATTING_DRAW_MODE_BACKGROUND, "LIGMA_MATTING_DRAW_MODE_BACKGROUND", "background" },
    { LIGMA_MATTING_DRAW_MODE_UNKNOWN, "LIGMA_MATTING_DRAW_MODE_UNKNOWN", "unknown" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_MATTING_DRAW_MODE_FOREGROUND, NC_("matting-draw-mode", "Draw foreground"), NULL },
    { LIGMA_MATTING_DRAW_MODE_BACKGROUND, NC_("matting-draw-mode", "Draw background"), NULL },
    { LIGMA_MATTING_DRAW_MODE_UNKNOWN, NC_("matting-draw-mode", "Draw unknown"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaMattingDrawMode", values);
      ligma_type_set_translation_context (type, "matting-draw-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_matting_preview_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_MATTING_PREVIEW_MODE_ON_COLOR, "LIGMA_MATTING_PREVIEW_MODE_ON_COLOR", "on-color" },
    { LIGMA_MATTING_PREVIEW_MODE_GRAYSCALE, "LIGMA_MATTING_PREVIEW_MODE_GRAYSCALE", "grayscale" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_MATTING_PREVIEW_MODE_ON_COLOR, NC_("matting-preview-mode", "Color"), NULL },
    { LIGMA_MATTING_PREVIEW_MODE_GRAYSCALE, NC_("matting-preview-mode", "Grayscale"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaMattingPreviewMode", values);
      ligma_type_set_translation_context (type, "matting-preview-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_transform_3d_lens_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_TRANSFORM_3D_LENS_MODE_FOCAL_LENGTH, "LIGMA_TRANSFORM_3D_LENS_MODE_FOCAL_LENGTH", "focal-length" },
    { LIGMA_TRANSFORM_3D_LENS_MODE_FOV_IMAGE, "LIGMA_TRANSFORM_3D_LENS_MODE_FOV_IMAGE", "fov-image" },
    { LIGMA_TRANSFORM_3D_LENS_MODE_FOV_ITEM, "LIGMA_TRANSFORM_3D_LENS_MODE_FOV_ITEM", "fov-item" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_TRANSFORM_3D_LENS_MODE_FOCAL_LENGTH, NC_("3-dtransform-lens-mode", "Focal length"), NULL },
    { LIGMA_TRANSFORM_3D_LENS_MODE_FOV_IMAGE, NC_("3-dtransform-lens-mode", "Field of view (relative to image)"), NULL },
    /* Translators: this is an abbreviated version of "Field of view (relative to image)".
       Keep it short. */
    { LIGMA_TRANSFORM_3D_LENS_MODE_FOV_IMAGE, NC_("3-dtransform-lens-mode", "FOV (image)"), NULL },
    { LIGMA_TRANSFORM_3D_LENS_MODE_FOV_ITEM, NC_("3-dtransform-lens-mode", "Field of view (relative to item)"), NULL },
    /* Translators: this is an abbreviated version of "Field of view (relative to item)".
       Keep it short. */
    { LIGMA_TRANSFORM_3D_LENS_MODE_FOV_ITEM, NC_("3-dtransform-lens-mode", "FOV (item)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("Ligma3DTransformLensMode", values);
      ligma_type_set_translation_context (type, "3-dtransform-lens-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_warp_behavior_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_WARP_BEHAVIOR_MOVE, "LIGMA_WARP_BEHAVIOR_MOVE", "move" },
    { LIGMA_WARP_BEHAVIOR_GROW, "LIGMA_WARP_BEHAVIOR_GROW", "grow" },
    { LIGMA_WARP_BEHAVIOR_SHRINK, "LIGMA_WARP_BEHAVIOR_SHRINK", "shrink" },
    { LIGMA_WARP_BEHAVIOR_SWIRL_CW, "LIGMA_WARP_BEHAVIOR_SWIRL_CW", "swirl-cw" },
    { LIGMA_WARP_BEHAVIOR_SWIRL_CCW, "LIGMA_WARP_BEHAVIOR_SWIRL_CCW", "swirl-ccw" },
    { LIGMA_WARP_BEHAVIOR_ERASE, "LIGMA_WARP_BEHAVIOR_ERASE", "erase" },
    { LIGMA_WARP_BEHAVIOR_SMOOTH, "LIGMA_WARP_BEHAVIOR_SMOOTH", "smooth" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_WARP_BEHAVIOR_MOVE, NC_("warp-behavior", "Move pixels"), NULL },
    { LIGMA_WARP_BEHAVIOR_GROW, NC_("warp-behavior", "Grow area"), NULL },
    { LIGMA_WARP_BEHAVIOR_SHRINK, NC_("warp-behavior", "Shrink area"), NULL },
    { LIGMA_WARP_BEHAVIOR_SWIRL_CW, NC_("warp-behavior", "Swirl clockwise"), NULL },
    { LIGMA_WARP_BEHAVIOR_SWIRL_CCW, NC_("warp-behavior", "Swirl counter-clockwise"), NULL },
    { LIGMA_WARP_BEHAVIOR_ERASE, NC_("warp-behavior", "Erase warping"), NULL },
    { LIGMA_WARP_BEHAVIOR_SMOOTH, NC_("warp-behavior", "Smooth warping"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaWarpBehavior", values);
      ligma_type_set_translation_context (type, "warp-behavior");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_paint_select_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_PAINT_SELECT_MODE_ADD, "LIGMA_PAINT_SELECT_MODE_ADD", "add" },
    { LIGMA_PAINT_SELECT_MODE_SUBTRACT, "LIGMA_PAINT_SELECT_MODE_SUBTRACT", "subtract" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_PAINT_SELECT_MODE_ADD, NC_("paint-select-mode", "Add to selection"), NULL },
    { LIGMA_PAINT_SELECT_MODE_SUBTRACT, NC_("paint-select-mode", "Subtract from selection"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaPaintSelectMode", values);
      ligma_type_set_translation_context (type, "paint-select-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

