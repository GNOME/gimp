
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "core/core-enums.h"
#include "tools-enums.h"
#include "gimp-intl.h"

/* enumerations from "./tools-enums.h" */

static const GEnumValue gimp_crop_type_enum_values[] =
{
  { GIMP_CROP, N_("Crop"), "crop" },
  { GIMP_RESIZE, N_("Resize"), "resize" },
  { 0, NULL, NULL }
};

GType
gimp_crop_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpCropType", gimp_crop_type_enum_values);

  return enum_type;
}


static const GEnumValue select_ops_enum_values[] =
{
  { SELECTION_ADD, "SELECTION_ADD", "add" },
  { SELECTION_SUBTRACT, "SELECTION_SUBTRACT", "subtract" },
  { SELECTION_REPLACE, "SELECTION_REPLACE", "replace" },
  { SELECTION_INTERSECT, "SELECTION_INTERSECT", "intersect" },
  { SELECTION_MOVE_MASK, "SELECTION_MOVE_MASK", "move-mask" },
  { SELECTION_MOVE, "SELECTION_MOVE", "move" },
  { SELECTION_MOVE_COPY, "SELECTION_MOVE_COPY", "move-copy" },
  { SELECTION_ANCHOR, "SELECTION_ANCHOR", "anchor" },
  { 0, NULL, NULL }
};

GType
select_ops_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("SelectOps", select_ops_enum_values);

  return enum_type;
}


static const GEnumValue gimp_tool_action_enum_values[] =
{
  { PAUSE, "PAUSE", "pause" },
  { RESUME, "RESUME", "resume" },
  { HALT, "HALT", "halt" },
  { 0, NULL, NULL }
};

GType
gimp_tool_action_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpToolAction", gimp_tool_action_enum_values);

  return enum_type;
}


static const GEnumValue gimp_motion_mode_enum_values[] =
{
  { GIMP_MOTION_MODE_EXACT, "GIMP_MOTION_MODE_EXACT", "exact" },
  { GIMP_MOTION_MODE_HINT, "GIMP_MOTION_MODE_HINT", "hint" },
  { GIMP_MOTION_MODE_COMPRESS, "GIMP_MOTION_MODE_COMPRESS", "compress" },
  { 0, NULL, NULL }
};

GType
gimp_motion_mode_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpMotionMode", gimp_motion_mode_enum_values);

  return enum_type;
}


static const GEnumValue gimp_rect_select_mode_enum_values[] =
{
  { GIMP_RECT_SELECT_MODE_FREE, N_("Free Select"), "free" },
  { GIMP_RECT_SELECT_MODE_FIXED_SIZE, N_("Fixed Size"), "fixed-size" },
  { GIMP_RECT_SELECT_MODE_FIXED_RATIO, N_("Fixed Aspect Ratio"), "fixed-ratio" },
  { 0, NULL, NULL }
};

GType
gimp_rect_select_mode_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpRectSelectMode", gimp_rect_select_mode_enum_values);

  return enum_type;
}


static const GEnumValue gimp_transform_grid_type_enum_values[] =
{
  { GIMP_TRANSFORM_GRID_TYPE_NONE, N_("Don't Show Grid"), "none" },
  { GIMP_TRANSFORM_GRID_TYPE_N_LINES, N_("Number of Grid Lines"), "n-lines" },
  { GIMP_TRANSFORM_GRID_TYPE_SPACING, N_("Grid Line Spacing"), "spacing" },
  { 0, NULL, NULL }
};

GType
gimp_transform_grid_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpTransformGridType", gimp_transform_grid_type_enum_values);

  return enum_type;
}


static const GEnumValue transform_action_enum_values[] =
{
  { TRANSFORM_CREATING, "TRANSFORM_CREATING", "creating" },
  { TRANSFORM_HANDLE_1, "TRANSFORM_HANDLE_1", "handle-1" },
  { TRANSFORM_HANDLE_2, "TRANSFORM_HANDLE_2", "handle-2" },
  { TRANSFORM_HANDLE_3, "TRANSFORM_HANDLE_3", "handle-3" },
  { TRANSFORM_HANDLE_4, "TRANSFORM_HANDLE_4", "handle-4" },
  { TRANSFORM_HANDLE_CENTER, "TRANSFORM_HANDLE_CENTER", "handle-center" },
  { 0, NULL, NULL }
};

GType
transform_action_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("TransformAction", transform_action_enum_values);

  return enum_type;
}


/* Generated data ends here */

