
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "core/core-enums.h"
#include "tools-enums.h"
#include "gimp-intl.h"

/* enumerations from "./tools-enums.h" */

static const GEnumValue gimp_color_pick_mode_enum_values[] =
{
  { GIMP_COLOR_PICK_MODE_NONE, N_("Pick only"), "none" },
  { GIMP_COLOR_PICK_MODE_FOREGROUND, N_("Set foreground color"), "foreground" },
  { GIMP_COLOR_PICK_MODE_BACKGROUND, N_("Set background color"), "background" },
  { 0, NULL, NULL }
};

GType
gimp_color_pick_mode_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpColorPickMode", gimp_color_pick_mode_enum_values);

  return enum_type;
}


static const GEnumValue gimp_crop_mode_enum_values[] =
{
  { GIMP_CROP_MODE_CROP, N_("Crop"), "crop" },
  { GIMP_CROP_MODE_RESIZE, N_("Resize"), "resize" },
  { 0, NULL, NULL }
};

GType
gimp_crop_mode_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpCropMode", gimp_crop_mode_enum_values);

  return enum_type;
}


static const GEnumValue gimp_rect_select_mode_enum_values[] =
{
  { GIMP_RECT_SELECT_MODE_FREE, N_("Free select"), "free" },
  { GIMP_RECT_SELECT_MODE_FIXED_SIZE, N_("Fixed size"), "fixed-size" },
  { GIMP_RECT_SELECT_MODE_FIXED_RATIO, N_("Fixed aspect ratio"), "fixed-ratio" },
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
  { GIMP_TRANSFORM_TYPE_LAYER, N_("Transform layer"), "layer" },
  { GIMP_TRANSFORM_TYPE_SELECTION, N_("Transform selection"), "selection" },
  { GIMP_TRANSFORM_TYPE_PATH, N_("Transform path"), "path" },
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


static const GEnumValue gimp_vector_mode_enum_values[] =
{
  { GIMP_VECTOR_MODE_DESIGN, N_("Design"), "design" },
  { GIMP_VECTOR_MODE_EDIT, N_("Edit"), "edit" },
  { GIMP_VECTOR_MODE_MOVE, N_("Move"), "move" },
  { 0, NULL, NULL }
};

GType
gimp_vector_mode_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpVectorMode", gimp_vector_mode_enum_values);

  return enum_type;
}


static const GEnumValue gimp_transform_preview_type_enum_values[] =
{
  { GIMP_TRANSFORM_PREVIEW_TYPE_OUTLINE, N_("Outline"), "outline" },
  { GIMP_TRANSFORM_PREVIEW_TYPE_GRID, N_("Grid"), "grid" },
  { GIMP_TRANSFORM_PREVIEW_TYPE_IMAGE, N_("Image"), "image" },
  { GIMP_TRANSFORM_PREVIEW_TYPE_IMAGE_GRID, N_("Image + Grid"), "image-grid" },
  { 0, NULL, NULL }
};

GType
gimp_transform_preview_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpTransformPreviewType", gimp_transform_preview_type_enum_values);

  return enum_type;
}


static const GEnumValue gimp_transform_grid_type_enum_values[] =
{
  { GIMP_TRANSFORM_GRID_TYPE_N_LINES, N_("Number of grid lines"), "n-lines" },
  { GIMP_TRANSFORM_GRID_TYPE_SPACING, N_("Grid line spacing"), "spacing" },
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

