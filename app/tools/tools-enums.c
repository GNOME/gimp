
/* Generated data (by gimp-mkenums) */

#include "stamp-tools-enums.h"
#include "config.h"
#include <gio/gio.h>
#include "libgimpbase/gimpbase.h"
#include "core/core-enums.h"
#include "tools-enums.h"
#include "gimp-intl.h"

/* enumerations from "tools-enums.h" */
GType
gimp_bucket_fill_area_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_BUCKET_FILL_SELECTION, "GIMP_BUCKET_FILL_SELECTION", "selection" },
    { GIMP_BUCKET_FILL_SIMILAR_COLORS, "GIMP_BUCKET_FILL_SIMILAR_COLORS", "similar-colors" },
    { GIMP_BUCKET_FILL_LINE_ART, "GIMP_BUCKET_FILL_LINE_ART", "line-art" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_BUCKET_FILL_SELECTION, NC_("bucket-fill-area", "Fill whole selection"), NULL },
    { GIMP_BUCKET_FILL_SIMILAR_COLORS, NC_("bucket-fill-area", "Fill similar colors"), NULL },
    { GIMP_BUCKET_FILL_LINE_ART, NC_("bucket-fill-area", "Fill by line art detection"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpBucketFillArea", values);
      gimp_type_set_translation_context (type, "bucket-fill-area");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_line_art_source_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_LINE_ART_SOURCE_SAMPLE_MERGED, "GIMP_LINE_ART_SOURCE_SAMPLE_MERGED", "sample-merged" },
    { GIMP_LINE_ART_SOURCE_ACTIVE_LAYER, "GIMP_LINE_ART_SOURCE_ACTIVE_LAYER", "active-layer" },
    { GIMP_LINE_ART_SOURCE_LOWER_LAYER, "GIMP_LINE_ART_SOURCE_LOWER_LAYER", "lower-layer" },
    { GIMP_LINE_ART_SOURCE_UPPER_LAYER, "GIMP_LINE_ART_SOURCE_UPPER_LAYER", "upper-layer" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_LINE_ART_SOURCE_SAMPLE_MERGED, NC_("line-art-source", "All visible layers"), NULL },
    { GIMP_LINE_ART_SOURCE_ACTIVE_LAYER, NC_("line-art-source", "Selected layer"), NULL },
    { GIMP_LINE_ART_SOURCE_LOWER_LAYER, NC_("line-art-source", "Layer below the selected one"), NULL },
    { GIMP_LINE_ART_SOURCE_UPPER_LAYER, NC_("line-art-source", "Layer above the selected one"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpLineArtSource", values);
      gimp_type_set_translation_context (type, "line-art-source");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_rect_select_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_RECT_SELECT_MODE_FREE, "GIMP_RECT_SELECT_MODE_FREE", "free" },
    { GIMP_RECT_SELECT_MODE_FIXED_SIZE, "GIMP_RECT_SELECT_MODE_FIXED_SIZE", "fixed-size" },
    { GIMP_RECT_SELECT_MODE_FIXED_RATIO, "GIMP_RECT_SELECT_MODE_FIXED_RATIO", "fixed-ratio" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_RECT_SELECT_MODE_FREE, NC_("rect-select-mode", "Free select"), NULL },
    { GIMP_RECT_SELECT_MODE_FIXED_SIZE, NC_("rect-select-mode", "Fixed size"), NULL },
    { GIMP_RECT_SELECT_MODE_FIXED_RATIO, NC_("rect-select-mode", "Fixed aspect ratio"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpRectSelectMode", values);
      gimp_type_set_translation_context (type, "rect-select-mode");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_transform_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_TRANSFORM_TYPE_LAYER, "GIMP_TRANSFORM_TYPE_LAYER", "layer" },
    { GIMP_TRANSFORM_TYPE_SELECTION, "GIMP_TRANSFORM_TYPE_SELECTION", "selection" },
    { GIMP_TRANSFORM_TYPE_PATH, "GIMP_TRANSFORM_TYPE_PATH", "path" },
    { GIMP_TRANSFORM_TYPE_IMAGE, "GIMP_TRANSFORM_TYPE_IMAGE", "image" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_TRANSFORM_TYPE_LAYER, NC_("transform-type", "Layer"), NULL },
    { GIMP_TRANSFORM_TYPE_SELECTION, NC_("transform-type", "Selection"), NULL },
    { GIMP_TRANSFORM_TYPE_PATH, NC_("transform-type", "Path"), NULL },
    { GIMP_TRANSFORM_TYPE_IMAGE, NC_("transform-type", "Image"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpTransformType", values);
      gimp_type_set_translation_context (type, "transform-type");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_tool_action_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_TOOL_ACTION_PAUSE, "GIMP_TOOL_ACTION_PAUSE", "pause" },
    { GIMP_TOOL_ACTION_RESUME, "GIMP_TOOL_ACTION_RESUME", "resume" },
    { GIMP_TOOL_ACTION_HALT, "GIMP_TOOL_ACTION_HALT", "halt" },
    { GIMP_TOOL_ACTION_COMMIT, "GIMP_TOOL_ACTION_COMMIT", "commit" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_TOOL_ACTION_PAUSE, "GIMP_TOOL_ACTION_PAUSE", NULL },
    { GIMP_TOOL_ACTION_RESUME, "GIMP_TOOL_ACTION_RESUME", NULL },
    { GIMP_TOOL_ACTION_HALT, "GIMP_TOOL_ACTION_HALT", NULL },
    { GIMP_TOOL_ACTION_COMMIT, "GIMP_TOOL_ACTION_COMMIT", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpToolAction", values);
      gimp_type_set_translation_context (type, "tool-action");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_tool_active_modifiers_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_TOOL_ACTIVE_MODIFIERS_OFF, "GIMP_TOOL_ACTIVE_MODIFIERS_OFF", "off" },
    { GIMP_TOOL_ACTIVE_MODIFIERS_SAME, "GIMP_TOOL_ACTIVE_MODIFIERS_SAME", "same" },
    { GIMP_TOOL_ACTIVE_MODIFIERS_SEPARATE, "GIMP_TOOL_ACTIVE_MODIFIERS_SEPARATE", "separate" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_TOOL_ACTIVE_MODIFIERS_OFF, "GIMP_TOOL_ACTIVE_MODIFIERS_OFF", NULL },
    { GIMP_TOOL_ACTIVE_MODIFIERS_SAME, "GIMP_TOOL_ACTIVE_MODIFIERS_SAME", NULL },
    { GIMP_TOOL_ACTIVE_MODIFIERS_SEPARATE, "GIMP_TOOL_ACTIVE_MODIFIERS_SEPARATE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpToolActiveModifiers", values);
      gimp_type_set_translation_context (type, "tool-active-modifiers");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_matting_draw_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_MATTING_DRAW_MODE_FOREGROUND, "GIMP_MATTING_DRAW_MODE_FOREGROUND", "foreground" },
    { GIMP_MATTING_DRAW_MODE_BACKGROUND, "GIMP_MATTING_DRAW_MODE_BACKGROUND", "background" },
    { GIMP_MATTING_DRAW_MODE_UNKNOWN, "GIMP_MATTING_DRAW_MODE_UNKNOWN", "unknown" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_MATTING_DRAW_MODE_FOREGROUND, NC_("matting-draw-mode", "Draw foreground"), NULL },
    { GIMP_MATTING_DRAW_MODE_BACKGROUND, NC_("matting-draw-mode", "Draw background"), NULL },
    { GIMP_MATTING_DRAW_MODE_UNKNOWN, NC_("matting-draw-mode", "Draw unknown"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpMattingDrawMode", values);
      gimp_type_set_translation_context (type, "matting-draw-mode");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_matting_preview_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_MATTING_PREVIEW_MODE_ON_COLOR, "GIMP_MATTING_PREVIEW_MODE_ON_COLOR", "on-color" },
    { GIMP_MATTING_PREVIEW_MODE_GRAYSCALE, "GIMP_MATTING_PREVIEW_MODE_GRAYSCALE", "grayscale" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_MATTING_PREVIEW_MODE_ON_COLOR, NC_("matting-preview-mode", "Color"), NULL },
    { GIMP_MATTING_PREVIEW_MODE_GRAYSCALE, NC_("matting-preview-mode", "Grayscale"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpMattingPreviewMode", values);
      gimp_type_set_translation_context (type, "matting-preview-mode");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_transform_3d_lens_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_TRANSFORM_3D_LENS_MODE_FOCAL_LENGTH, "GIMP_TRANSFORM_3D_LENS_MODE_FOCAL_LENGTH", "focal-length" },
    { GIMP_TRANSFORM_3D_LENS_MODE_FOV_IMAGE, "GIMP_TRANSFORM_3D_LENS_MODE_FOV_IMAGE", "fov-image" },
    { GIMP_TRANSFORM_3D_LENS_MODE_FOV_ITEM, "GIMP_TRANSFORM_3D_LENS_MODE_FOV_ITEM", "fov-item" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_TRANSFORM_3D_LENS_MODE_FOCAL_LENGTH, NC_("3-dtransform-lens-mode", "Focal length"), NULL },
    { GIMP_TRANSFORM_3D_LENS_MODE_FOV_IMAGE, NC_("3-dtransform-lens-mode", "Field of view (relative to image)"), NULL },
    /* Translators: this is an abbreviated version of "Field of view (relative to image)".
       Keep it short. */
    { GIMP_TRANSFORM_3D_LENS_MODE_FOV_IMAGE, NC_("3-dtransform-lens-mode", "FOV (image)"), NULL },
    { GIMP_TRANSFORM_3D_LENS_MODE_FOV_ITEM, NC_("3-dtransform-lens-mode", "Field of view (relative to item)"), NULL },
    /* Translators: this is an abbreviated version of "Field of view (relative to item)".
       Keep it short. */
    { GIMP_TRANSFORM_3D_LENS_MODE_FOV_ITEM, NC_("3-dtransform-lens-mode", "FOV (item)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("Gimp3DTransformLensMode", values);
      gimp_type_set_translation_context (type, "3-dtransform-lens-mode");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_warp_behavior_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_WARP_BEHAVIOR_MOVE, "GIMP_WARP_BEHAVIOR_MOVE", "move" },
    { GIMP_WARP_BEHAVIOR_GROW, "GIMP_WARP_BEHAVIOR_GROW", "grow" },
    { GIMP_WARP_BEHAVIOR_SHRINK, "GIMP_WARP_BEHAVIOR_SHRINK", "shrink" },
    { GIMP_WARP_BEHAVIOR_SWIRL_CW, "GIMP_WARP_BEHAVIOR_SWIRL_CW", "swirl-cw" },
    { GIMP_WARP_BEHAVIOR_SWIRL_CCW, "GIMP_WARP_BEHAVIOR_SWIRL_CCW", "swirl-ccw" },
    { GIMP_WARP_BEHAVIOR_ERASE, "GIMP_WARP_BEHAVIOR_ERASE", "erase" },
    { GIMP_WARP_BEHAVIOR_SMOOTH, "GIMP_WARP_BEHAVIOR_SMOOTH", "smooth" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_WARP_BEHAVIOR_MOVE, NC_("warp-behavior", "Move pixels"), NULL },
    { GIMP_WARP_BEHAVIOR_GROW, NC_("warp-behavior", "Grow area"), NULL },
    { GIMP_WARP_BEHAVIOR_SHRINK, NC_("warp-behavior", "Shrink area"), NULL },
    { GIMP_WARP_BEHAVIOR_SWIRL_CW, NC_("warp-behavior", "Swirl clockwise"), NULL },
    { GIMP_WARP_BEHAVIOR_SWIRL_CCW, NC_("warp-behavior", "Swirl counter-clockwise"), NULL },
    { GIMP_WARP_BEHAVIOR_ERASE, NC_("warp-behavior", "Erase warping"), NULL },
    { GIMP_WARP_BEHAVIOR_SMOOTH, NC_("warp-behavior", "Smooth warping"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpWarpBehavior", values);
      gimp_type_set_translation_context (type, "warp-behavior");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_paint_select_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_PAINT_SELECT_MODE_ADD, "GIMP_PAINT_SELECT_MODE_ADD", "add" },
    { GIMP_PAINT_SELECT_MODE_SUBTRACT, "GIMP_PAINT_SELECT_MODE_SUBTRACT", "subtract" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_PAINT_SELECT_MODE_ADD, NC_("paint-select-mode", "Add to selection"), NULL },
    { GIMP_PAINT_SELECT_MODE_SUBTRACT, NC_("paint-select-mode", "Subtract from selection"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("GimpPaintSelectMode", values);
      gimp_type_set_translation_context (type, "paint-select-mode");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

