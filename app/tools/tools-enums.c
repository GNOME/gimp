
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


static const GEnumValue gimp_transform_type_enum_values[] =
{
  { GIMP_TRANSFORM_TYPE_LAYER, N_("Transform Active Layer"), "layer" },
  { GIMP_TRANSFORM_TYPE_SELECTION, N_("Transform Selection"), "selection" },
  { GIMP_TRANSFORM_TYPE_PATH, N_("Transform Active Path"), "path" },
  { 0, NULL, NULL }
};

GType
gimp_transform_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpTransformType", gimp_transform_type_enum_values);

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


/* Generated data ends here */

