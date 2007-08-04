
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "libgimpbase/gimpbase.h"
#include "core/core-enums.h"
#include "tools-enums.h"
#include "gimp-intl.h"

/* enumerations from "./tools-enums.h" */
GType
gimp_button_release_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_BUTTON_RELEASE_NORMAL, "GIMP_BUTTON_RELEASE_NORMAL", "normal" },
    { GIMP_BUTTON_RELEASE_CANCEL, "GIMP_BUTTON_RELEASE_CANCEL", "cancel" },
    { GIMP_BUTTON_RELEASE_CLICK, "GIMP_BUTTON_RELEASE_CLICK", "click" },
    { GIMP_BUTTON_RELEASE_NO_MOTION, "GIMP_BUTTON_RELEASE_NO_MOTION", "no-motion" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_BUTTON_RELEASE_NORMAL, "GIMP_BUTTON_RELEASE_NORMAL", NULL },
    { GIMP_BUTTON_RELEASE_CANCEL, "GIMP_BUTTON_RELEASE_CANCEL", NULL },
    { GIMP_BUTTON_RELEASE_CLICK, "GIMP_BUTTON_RELEASE_CLICK", NULL },
    { GIMP_BUTTON_RELEASE_NO_MOTION, "GIMP_BUTTON_RELEASE_NO_MOTION", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpButtonReleaseType", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_rectangle_guide_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_RECTANGLE_GUIDE_NONE, "GIMP_RECTANGLE_GUIDE_NONE", "none" },
    { GIMP_RECTANGLE_GUIDE_CENTER_LINES, "GIMP_RECTANGLE_GUIDE_CENTER_LINES", "center-lines" },
    { GIMP_RECTANGLE_GUIDE_THIRDS, "GIMP_RECTANGLE_GUIDE_THIRDS", "thirds" },
    { GIMP_RECTANGLE_GUIDE_GOLDEN, "GIMP_RECTANGLE_GUIDE_GOLDEN", "golden" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_RECTANGLE_GUIDE_NONE, N_("No guides"), NULL },
    { GIMP_RECTANGLE_GUIDE_CENTER_LINES, N_("Center lines"), NULL },
    { GIMP_RECTANGLE_GUIDE_THIRDS, N_("Rule of thirds"), NULL },
    { GIMP_RECTANGLE_GUIDE_GOLDEN, N_("Golden sections"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpRectangleGuide", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_rectangle_constraint_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_RECTANGLE_CONSTRAIN_NONE, "GIMP_RECTANGLE_CONSTRAIN_NONE", "none" },
    { GIMP_RECTANGLE_CONSTRAIN_IMAGE, "GIMP_RECTANGLE_CONSTRAIN_IMAGE", "image" },
    { GIMP_RECTANGLE_CONSTRAIN_DRAWABLE, "GIMP_RECTANGLE_CONSTRAIN_DRAWABLE", "drawable" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_RECTANGLE_CONSTRAIN_NONE, "GIMP_RECTANGLE_CONSTRAIN_NONE", NULL },
    { GIMP_RECTANGLE_CONSTRAIN_IMAGE, "GIMP_RECTANGLE_CONSTRAIN_IMAGE", NULL },
    { GIMP_RECTANGLE_CONSTRAIN_DRAWABLE, "GIMP_RECTANGLE_CONSTRAIN_DRAWABLE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpRectangleConstraint", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_rectangle_tool_fixed_rule_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_RECTANGLE_TOOL_FIXED_ASPECT, "GIMP_RECTANGLE_TOOL_FIXED_ASPECT", "aspect" },
    { GIMP_RECTANGLE_TOOL_FIXED_WIDTH, "GIMP_RECTANGLE_TOOL_FIXED_WIDTH", "width" },
    { GIMP_RECTANGLE_TOOL_FIXED_HEIGHT, "GIMP_RECTANGLE_TOOL_FIXED_HEIGHT", "height" },
    { GIMP_RECTANGLE_TOOL_FIXED_SIZE, "GIMP_RECTANGLE_TOOL_FIXED_SIZE", "size" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_RECTANGLE_TOOL_FIXED_ASPECT, N_("Aspect ratio"), NULL },
    { GIMP_RECTANGLE_TOOL_FIXED_WIDTH, N_("Width"), NULL },
    { GIMP_RECTANGLE_TOOL_FIXED_HEIGHT, N_("Height"), NULL },
    { GIMP_RECTANGLE_TOOL_FIXED_SIZE, N_("Size"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpRectangleToolFixedRule", values);
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
    { GIMP_RECT_SELECT_MODE_FREE, N_("Free select"), NULL },
    { GIMP_RECT_SELECT_MODE_FIXED_SIZE, N_("Fixed size"), NULL },
    { GIMP_RECT_SELECT_MODE_FIXED_RATIO, N_("Fixed aspect ratio"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpRectSelectMode", values);
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
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_TRANSFORM_TYPE_LAYER, N_("Layer"), NULL },
    { GIMP_TRANSFORM_TYPE_SELECTION, N_("Selection"), NULL },
    { GIMP_TRANSFORM_TYPE_PATH, N_("Path"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpTransformType", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_transform_preview_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_TRANSFORM_PREVIEW_TYPE_OUTLINE, "GIMP_TRANSFORM_PREVIEW_TYPE_OUTLINE", "outline" },
    { GIMP_TRANSFORM_PREVIEW_TYPE_GRID, "GIMP_TRANSFORM_PREVIEW_TYPE_GRID", "grid" },
    { GIMP_TRANSFORM_PREVIEW_TYPE_IMAGE, "GIMP_TRANSFORM_PREVIEW_TYPE_IMAGE", "image" },
    { GIMP_TRANSFORM_PREVIEW_TYPE_IMAGE_GRID, "GIMP_TRANSFORM_PREVIEW_TYPE_IMAGE_GRID", "image-grid" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_TRANSFORM_PREVIEW_TYPE_OUTLINE, N_("Outline"), NULL },
    { GIMP_TRANSFORM_PREVIEW_TYPE_GRID, N_("Grid"), NULL },
    { GIMP_TRANSFORM_PREVIEW_TYPE_IMAGE, N_("Image"), NULL },
    { GIMP_TRANSFORM_PREVIEW_TYPE_IMAGE_GRID, N_("Image + Grid"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpTransformPreviewType", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_transform_grid_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_TRANSFORM_GRID_TYPE_N_LINES, "GIMP_TRANSFORM_GRID_TYPE_N_LINES", "n-lines" },
    { GIMP_TRANSFORM_GRID_TYPE_SPACING, "GIMP_TRANSFORM_GRID_TYPE_SPACING", "spacing" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_TRANSFORM_GRID_TYPE_N_LINES, N_("Number of grid lines"), NULL },
    { GIMP_TRANSFORM_GRID_TYPE_SPACING, N_("Grid line spacing"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpTransformGridType", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_vector_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_VECTOR_MODE_DESIGN, "GIMP_VECTOR_MODE_DESIGN", "design" },
    { GIMP_VECTOR_MODE_EDIT, "GIMP_VECTOR_MODE_EDIT", "edit" },
    { GIMP_VECTOR_MODE_MOVE, "GIMP_VECTOR_MODE_MOVE", "move" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_VECTOR_MODE_DESIGN, N_("Design"), NULL },
    { GIMP_VECTOR_MODE_EDIT, N_("Edit"), NULL },
    { GIMP_VECTOR_MODE_MOVE, N_("Move"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpVectorMode", values);
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

