
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "core-enums.h"
#include "gimp-intl.h"

/* enumerations from "./core-enums.h" */

static const GEnumValue gimp_add_mask_type_enum_values[] =
{
  { GIMP_ADD_WHITE_MASK, N_("_White (Full Opacity)"), "white-mask" },
  { GIMP_ADD_BLACK_MASK, N_("_Black (Full Transparency)"), "black-mask" },
  { GIMP_ADD_ALPHA_MASK, N_("Layer's _Alpha Channel"), "alpha-mask" },
  { GIMP_ADD_ALPHA_MASK_TRANSFER, N_("_Transfer Layer's Alpha Channel"), "alpha-mask-transfer" },
  { GIMP_ADD_SELECTION_MASK, N_("_Selection"), "selection-mask" },
  { GIMP_ADD_COPY_MASK, N_("_Grayscale Copy of Layer"), "copy-mask" },
  { 0, NULL, NULL }
};

GType
gimp_add_mask_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpAddMaskType", gimp_add_mask_type_enum_values);

  return enum_type;
}


static const GEnumValue gimp_blend_mode_enum_values[] =
{
  { GIMP_FG_BG_RGB_MODE, N_("FG to BG (RGB)"), "fg-bg-rgb-mode" },
  { GIMP_FG_BG_HSV_MODE, N_("FG to BG (HSV)"), "fg-bg-hsv-mode" },
  { GIMP_FG_TRANSPARENT_MODE, N_("FG to Transparent"), "fg-transparent-mode" },
  { GIMP_CUSTOM_MODE, N_("Custom Gradient"), "custom-mode" },
  { 0, NULL, NULL }
};

GType
gimp_blend_mode_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpBlendMode", gimp_blend_mode_enum_values);

  return enum_type;
}


static const GEnumValue gimp_bucket_fill_mode_enum_values[] =
{
  { GIMP_FG_BUCKET_FILL, N_("FG Color Fill"), "fg-bucket-fill" },
  { GIMP_BG_BUCKET_FILL, N_("BG Color Fill"), "bg-bucket-fill" },
  { GIMP_PATTERN_BUCKET_FILL, N_("Pattern Fill"), "pattern-bucket-fill" },
  { 0, NULL, NULL }
};

GType
gimp_bucket_fill_mode_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpBucketFillMode", gimp_bucket_fill_mode_enum_values);

  return enum_type;
}


static const GEnumValue gimp_channel_ops_enum_values[] =
{
  { GIMP_CHANNEL_OP_ADD, N_("Add to the current selection"), "add" },
  { GIMP_CHANNEL_OP_SUBTRACT, N_("Subtract from the current selection"), "subtract" },
  { GIMP_CHANNEL_OP_REPLACE, N_("Replace the current selection"), "replace" },
  { GIMP_CHANNEL_OP_INTERSECT, N_("Intersect with the current selection"), "intersect" },
  { 0, NULL, NULL }
};

GType
gimp_channel_ops_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpChannelOps", gimp_channel_ops_enum_values);

  return enum_type;
}


static const GEnumValue gimp_channel_type_enum_values[] =
{
  { GIMP_RED_CHANNEL, N_("Red"), "red-channel" },
  { GIMP_GREEN_CHANNEL, N_("Green"), "green-channel" },
  { GIMP_BLUE_CHANNEL, N_("Blue"), "blue-channel" },
  { GIMP_GRAY_CHANNEL, N_("Gray"), "gray-channel" },
  { GIMP_INDEXED_CHANNEL, N_("Indexed"), "indexed-channel" },
  { GIMP_ALPHA_CHANNEL, N_("Alpha"), "alpha-channel" },
  { 0, NULL, NULL }
};

GType
gimp_channel_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpChannelType", gimp_channel_type_enum_values);

  return enum_type;
}


static const GEnumValue gimp_container_policy_enum_values[] =
{
  { GIMP_CONTAINER_POLICY_STRONG, "GIMP_CONTAINER_POLICY_STRONG", "strong" },
  { GIMP_CONTAINER_POLICY_WEAK, "GIMP_CONTAINER_POLICY_WEAK", "weak" },
  { 0, NULL, NULL }
};

GType
gimp_container_policy_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpContainerPolicy", gimp_container_policy_enum_values);

  return enum_type;
}


static const GEnumValue gimp_convert_dither_type_enum_values[] =
{
  { GIMP_NO_DITHER, N_("No Color Dithering"), "no-dither" },
  { GIMP_FS_DITHER, N_("Floyd-Steinberg Color Dithering (Normal)"), "fs-dither" },
  { GIMP_FSLOWBLEED_DITHER, N_("Floyd-Steinberg Color Dithering (Reduced Color Bleeding)"), "fslowbleed-dither" },
  { GIMP_FIXED_DITHER, N_("Positioned Color Dithering"), "fixed-dither" },
  { 0, NULL, NULL }
};

GType
gimp_convert_dither_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpConvertDitherType", gimp_convert_dither_type_enum_values);

  return enum_type;
}


static const GEnumValue gimp_gravity_type_enum_values[] =
{
  { GIMP_GRAVITY_NONE, "GIMP_GRAVITY_NONE", "none" },
  { GIMP_GRAVITY_NORTH_WEST, "GIMP_GRAVITY_NORTH_WEST", "north-west" },
  { GIMP_GRAVITY_NORTH, "GIMP_GRAVITY_NORTH", "north" },
  { GIMP_GRAVITY_NORTH_EAST, "GIMP_GRAVITY_NORTH_EAST", "north-east" },
  { GIMP_GRAVITY_WEST, "GIMP_GRAVITY_WEST", "west" },
  { GIMP_GRAVITY_CENTER, "GIMP_GRAVITY_CENTER", "center" },
  { GIMP_GRAVITY_EAST, "GIMP_GRAVITY_EAST", "east" },
  { GIMP_GRAVITY_SOUTH_WEST, "GIMP_GRAVITY_SOUTH_WEST", "south-west" },
  { GIMP_GRAVITY_SOUTH, "GIMP_GRAVITY_SOUTH", "south" },
  { GIMP_GRAVITY_SOUTH_EAST, "GIMP_GRAVITY_SOUTH_EAST", "south-east" },
  { 0, NULL, NULL }
};

GType
gimp_gravity_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpGravityType", gimp_gravity_type_enum_values);

  return enum_type;
}


static const GEnumValue gimp_fill_type_enum_values[] =
{
  { GIMP_FOREGROUND_FILL, N_("Foreground"), "foreground-fill" },
  { GIMP_BACKGROUND_FILL, N_("Background"), "background-fill" },
  { GIMP_WHITE_FILL, N_("White"), "white-fill" },
  { GIMP_TRANSPARENT_FILL, N_("Transparent"), "transparent-fill" },
  { GIMP_NO_FILL, N_("None"), "no-fill" },
  { 0, NULL, NULL }
};

GType
gimp_fill_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpFillType", gimp_fill_type_enum_values);

  return enum_type;
}


static const GEnumValue gimp_gradient_type_enum_values[] =
{
  { GIMP_GRADIENT_LINEAR, N_("Linear"), "linear" },
  { GIMP_GRADIENT_BILINEAR, N_("Bi-Linear"), "bilinear" },
  { GIMP_GRADIENT_RADIAL, N_("Radial"), "radial" },
  { GIMP_GRADIENT_SQUARE, N_("Square"), "square" },
  { GIMP_GRADIENT_CONICAL_SYMMETRIC, N_("Conical (symmetric)"), "conical-symmetric" },
  { GIMP_GRADIENT_CONICAL_ASYMMETRIC, N_("Conical (asymmetric)"), "conical-asymmetric" },
  { GIMP_GRADIENT_SHAPEBURST_ANGULAR, N_("Shapeburst (angular)"), "shapeburst-angular" },
  { GIMP_GRADIENT_SHAPEBURST_SPHERICAL, N_("Shapeburst (spherical)"), "shapeburst-spherical" },
  { GIMP_GRADIENT_SHAPEBURST_DIMPLED, N_("Shapeburst (dimpled)"), "shapeburst-dimpled" },
  { GIMP_GRADIENT_SPIRAL_CLOCKWISE, N_("Spiral (clockwise)"), "spiral-clockwise" },
  { GIMP_GRADIENT_SPIRAL_ANTICLOCKWISE, N_("Spiral (anticlockwise)"), "spiral-anticlockwise" },
  { 0, NULL, NULL }
};

GType
gimp_gradient_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpGradientType", gimp_gradient_type_enum_values);

  return enum_type;
}


static const GEnumValue gimp_grid_style_enum_values[] =
{
  { GIMP_GRID_DOTS, N_("Intersections (dots)"), "dots" },
  { GIMP_GRID_INTERSECTIONS, N_("Intersections (crosshairs)"), "intersections" },
  { GIMP_GRID_ON_OFF_DASH, N_("Dashed"), "on-off-dash" },
  { GIMP_GRID_DOUBLE_DASH, N_("Double Dashed"), "double-dash" },
  { GIMP_GRID_SOLID, N_("Solid"), "solid" },
  { 0, NULL, NULL }
};

GType
gimp_grid_style_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpGridStyle", gimp_grid_style_enum_values);

  return enum_type;
}


static const GEnumValue gimp_stroke_style_enum_values[] =
{
  { GIMP_STROKE_STYLE_SOLID, N_("Solid"), "solid" },
  { GIMP_STROKE_STYLE_PATTERN, N_("Pattern"), "pattern" },
  { 0, NULL, NULL }
};

GType
gimp_stroke_style_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpStrokeStyle", gimp_stroke_style_enum_values);

  return enum_type;
}


static const GEnumValue gimp_join_style_enum_values[] =
{
  { GIMP_JOIN_MITER, N_("Miter"), "miter" },
  { GIMP_JOIN_ROUND, N_("Round"), "round" },
  { GIMP_JOIN_BEVEL, N_("Bevel"), "bevel" },
  { 0, NULL, NULL }
};

GType
gimp_join_style_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpJoinStyle", gimp_join_style_enum_values);

  return enum_type;
}


static const GEnumValue gimp_cap_style_enum_values[] =
{
  { GIMP_CAP_BUTT, N_("Butt"), "butt" },
  { GIMP_CAP_ROUND, N_("Round"), "round" },
  { GIMP_CAP_SQUARE, N_("Square"), "square" },
  { 0, NULL, NULL }
};

GType
gimp_cap_style_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpCapStyle", gimp_cap_style_enum_values);

  return enum_type;
}


static const GEnumValue gimp_image_base_type_enum_values[] =
{
  { GIMP_RGB, N_("RGB"), "rgb" },
  { GIMP_GRAY, N_("Grayscale"), "gray" },
  { GIMP_INDEXED, N_("Indexed"), "indexed" },
  { 0, NULL, NULL }
};

GType
gimp_image_base_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpImageBaseType", gimp_image_base_type_enum_values);

  return enum_type;
}


static const GEnumValue gimp_image_type_enum_values[] =
{
  { GIMP_RGB_IMAGE, N_("RGB"), "rgb-image" },
  { GIMP_RGBA_IMAGE, N_("RGB-Alpha"), "rgba-image" },
  { GIMP_GRAY_IMAGE, N_("Grayscale"), "gray-image" },
  { GIMP_GRAYA_IMAGE, N_("Grayscale-Alpha"), "graya-image" },
  { GIMP_INDEXED_IMAGE, N_("Indexed"), "indexed-image" },
  { GIMP_INDEXEDA_IMAGE, N_("Indexed-Alpha"), "indexeda-image" },
  { 0, NULL, NULL }
};

GType
gimp_image_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpImageType", gimp_image_type_enum_values);

  return enum_type;
}


static const GEnumValue gimp_orientation_type_enum_values[] =
{
  { GIMP_ORIENTATION_HORIZONTAL, N_("Horizontal"), "horizontal" },
  { GIMP_ORIENTATION_VERTICAL, N_("Vertical"), "vertical" },
  { GIMP_ORIENTATION_UNKNOWN, N_("Unknown"), "unknown" },
  { 0, NULL, NULL }
};

GType
gimp_orientation_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpOrientationType", gimp_orientation_type_enum_values);

  return enum_type;
}


static const GEnumValue gimp_rotation_type_enum_values[] =
{
  { GIMP_ROTATE_90, "GIMP_ROTATE_90", "90" },
  { GIMP_ROTATE_180, "GIMP_ROTATE_180", "180" },
  { GIMP_ROTATE_270, "GIMP_ROTATE_270", "270" },
  { 0, NULL, NULL }
};

GType
gimp_rotation_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpRotationType", gimp_rotation_type_enum_values);

  return enum_type;
}


static const GEnumValue gimp_preview_size_enum_values[] =
{
  { GIMP_PREVIEW_SIZE_TINY, N_("Tiny"), "tiny" },
  { GIMP_PREVIEW_SIZE_EXTRA_SMALL, N_("Very Small"), "extra-small" },
  { GIMP_PREVIEW_SIZE_SMALL, N_("Small"), "small" },
  { GIMP_PREVIEW_SIZE_MEDIUM, N_("Medium"), "medium" },
  { GIMP_PREVIEW_SIZE_LARGE, N_("Large"), "large" },
  { GIMP_PREVIEW_SIZE_EXTRA_LARGE, N_("Very Large"), "extra-large" },
  { GIMP_PREVIEW_SIZE_HUGE, N_("Huge"), "huge" },
  { GIMP_PREVIEW_SIZE_ENORMOUS, N_("Enormous"), "enormous" },
  { GIMP_PREVIEW_SIZE_GIGANTIC, N_("Gigantic"), "gigantic" },
  { 0, NULL, NULL }
};

GType
gimp_preview_size_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpPreviewSize", gimp_preview_size_enum_values);

  return enum_type;
}


static const GEnumValue gimp_repeat_mode_enum_values[] =
{
  { GIMP_REPEAT_NONE, N_("None"), "none" },
  { GIMP_REPEAT_SAWTOOTH, N_("Sawtooth Wave"), "sawtooth" },
  { GIMP_REPEAT_TRIANGULAR, N_("Triangular Wave"), "triangular" },
  { 0, NULL, NULL }
};

GType
gimp_repeat_mode_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpRepeatMode", gimp_repeat_mode_enum_values);

  return enum_type;
}


static const GEnumValue gimp_selection_control_enum_values[] =
{
  { GIMP_SELECTION_OFF, "GIMP_SELECTION_OFF", "off" },
  { GIMP_SELECTION_LAYER_OFF, "GIMP_SELECTION_LAYER_OFF", "layer-off" },
  { GIMP_SELECTION_ON, "GIMP_SELECTION_ON", "on" },
  { GIMP_SELECTION_PAUSE, "GIMP_SELECTION_PAUSE", "pause" },
  { GIMP_SELECTION_RESUME, "GIMP_SELECTION_RESUME", "resume" },
  { 0, NULL, NULL }
};

GType
gimp_selection_control_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpSelectionControl", gimp_selection_control_enum_values);

  return enum_type;
}


static const GEnumValue gimp_thumbnail_size_enum_values[] =
{
  { GIMP_THUMBNAIL_SIZE_NONE, N_("No Thumbnails"), "none" },
  { GIMP_THUMBNAIL_SIZE_NORMAL, N_("Normal (128x128)"), "normal" },
  { GIMP_THUMBNAIL_SIZE_LARGE, N_("Large (256x256)"), "large" },
  { 0, NULL, NULL }
};

GType
gimp_thumbnail_size_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpThumbnailSize", gimp_thumbnail_size_enum_values);

  return enum_type;
}


static const GEnumValue gimp_transform_direction_enum_values[] =
{
  { GIMP_TRANSFORM_FORWARD, N_("Forward (Traditional)"), "forward" },
  { GIMP_TRANSFORM_BACKWARD, N_("Backward (Corrective)"), "backward" },
  { 0, NULL, NULL }
};

GType
gimp_transform_direction_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpTransformDirection", gimp_transform_direction_enum_values);

  return enum_type;
}


static const GEnumValue gimp_undo_mode_enum_values[] =
{
  { GIMP_UNDO_MODE_UNDO, "GIMP_UNDO_MODE_UNDO", "undo" },
  { GIMP_UNDO_MODE_REDO, "GIMP_UNDO_MODE_REDO", "redo" },
  { 0, NULL, NULL }
};

GType
gimp_undo_mode_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpUndoMode", gimp_undo_mode_enum_values);

  return enum_type;
}


static const GEnumValue gimp_undo_event_enum_values[] =
{
  { GIMP_UNDO_EVENT_UNDO_PUSHED, "GIMP_UNDO_EVENT_UNDO_PUSHED", "undo-pushed" },
  { GIMP_UNDO_EVENT_UNDO_EXPIRED, "GIMP_UNDO_EVENT_UNDO_EXPIRED", "undo-expired" },
  { GIMP_UNDO_EVENT_REDO_EXPIRED, "GIMP_UNDO_EVENT_REDO_EXPIRED", "redo-expired" },
  { GIMP_UNDO_EVENT_UNDO, "GIMP_UNDO_EVENT_UNDO", "undo" },
  { GIMP_UNDO_EVENT_REDO, "GIMP_UNDO_EVENT_REDO", "redo" },
  { GIMP_UNDO_EVENT_UNDO_FREE, "GIMP_UNDO_EVENT_UNDO_FREE", "undo-free" },
  { GIMP_UNDO_EVENT_UNDO_FREEZE, "GIMP_UNDO_EVENT_UNDO_FREEZE", "undo-freeze" },
  { GIMP_UNDO_EVENT_UNDO_THAW, "GIMP_UNDO_EVENT_UNDO_THAW", "undo-thaw" },
  { 0, NULL, NULL }
};

GType
gimp_undo_event_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpUndoEvent", gimp_undo_event_enum_values);

  return enum_type;
}


static const GEnumValue gimp_undo_type_enum_values[] =
{
  { GIMP_UNDO_GROUP_NONE, N_("<<invalid>>"), "group-none" },
  { GIMP_UNDO_GROUP_IMAGE_SCALE, N_("Scale Image"), "group-image-scale" },
  { GIMP_UNDO_GROUP_IMAGE_RESIZE, N_("Resize Image"), "group-image-resize" },
  { GIMP_UNDO_GROUP_IMAGE_FLIP, N_("Flip Image"), "group-image-flip" },
  { GIMP_UNDO_GROUP_IMAGE_ROTATE, N_("Rotate Image"), "group-image-rotate" },
  { GIMP_UNDO_GROUP_IMAGE_CONVERT, N_("Convert Image"), "group-image-convert" },
  { GIMP_UNDO_GROUP_IMAGE_CROP, N_("Crop Image"), "group-image-crop" },
  { GIMP_UNDO_GROUP_IMAGE_LAYERS_MERGE, N_("Merge Layers"), "group-image-layers-merge" },
  { GIMP_UNDO_GROUP_IMAGE_VECTORS_MERGE, N_("Merge Vectors"), "group-image-vectors-merge" },
  { GIMP_UNDO_GROUP_IMAGE_QMASK, N_("QuickMask"), "group-image-qmask" },
  { GIMP_UNDO_GROUP_IMAGE_GRID, N_("Grid"), "group-image-grid" },
  { GIMP_UNDO_GROUP_IMAGE_GUIDE, N_("Guide"), "group-image-guide" },
  { GIMP_UNDO_GROUP_MASK, N_("Selection Mask"), "group-mask" },
  { GIMP_UNDO_GROUP_ITEM_PROPERTIES, N_("Item Properties"), "group-item-properties" },
  { GIMP_UNDO_GROUP_ITEM_DISPLACE, N_("Move Item"), "group-item-displace" },
  { GIMP_UNDO_GROUP_ITEM_VISIBILITY, N_("Item Visibility"), "group-item-visibility" },
  { GIMP_UNDO_GROUP_ITEM_LINKED, N_("Linked Item"), "group-item-linked" },
  { GIMP_UNDO_GROUP_LAYER_SCALE, N_("Scale Layer"), "group-layer-scale" },
  { GIMP_UNDO_GROUP_LAYER_RESIZE, N_("Resize Layer"), "group-layer-resize" },
  { GIMP_UNDO_GROUP_LAYER_ADD_MASK, N_("Add Layer Mask"), "group-layer-add-mask" },
  { GIMP_UNDO_GROUP_LAYER_APPLY_MASK, N_("Apply Layer Mask"), "group-layer-apply-mask" },
  { GIMP_UNDO_GROUP_FS_TO_LAYER, N_("Floating Selection to Layer"), "group-fs-to-layer" },
  { GIMP_UNDO_GROUP_FS_FLOAT, N_("Float Selection"), "group-fs-float" },
  { GIMP_UNDO_GROUP_FS_ANCHOR, N_("Anchor Floating Selection"), "group-fs-anchor" },
  { GIMP_UNDO_GROUP_EDIT_PASTE, N_("Paste"), "group-edit-paste" },
  { GIMP_UNDO_GROUP_EDIT_CUT, N_("Cut"), "group-edit-cut" },
  { GIMP_UNDO_GROUP_EDIT_COPY, N_("Copy"), "group-edit-copy" },
  { GIMP_UNDO_GROUP_TEXT, N_("Text"), "group-text" },
  { GIMP_UNDO_GROUP_TRANSFORM, N_("Transform"), "group-transform" },
  { GIMP_UNDO_GROUP_PAINT, N_("Paint"), "group-paint" },
  { GIMP_UNDO_GROUP_PARASITE_ATTACH, N_("Attach Parasite"), "group-parasite-attach" },
  { GIMP_UNDO_GROUP_PARASITE_REMOVE, N_("Remove Parasite"), "group-parasite-remove" },
  { GIMP_UNDO_GROUP_VECTORS_IMPORT, N_("Import Paths"), "group-vectors-import" },
  { GIMP_UNDO_GROUP_MISC, N_("Plug-In"), "group-misc" },
  { GIMP_UNDO_IMAGE, N_("Image"), "image" },
  { GIMP_UNDO_IMAGE_MOD, N_("Image Mod"), "image-mod" },
  { GIMP_UNDO_IMAGE_TYPE, N_("Image Type"), "image-type" },
  { GIMP_UNDO_IMAGE_SIZE, N_("Image Size"), "image-size" },
  { GIMP_UNDO_IMAGE_RESOLUTION, N_("Resolution Change"), "image-resolution" },
  { GIMP_UNDO_IMAGE_QMASK, N_("QuickMask"), "image-qmask" },
  { GIMP_UNDO_IMAGE_GRID, N_("Grid"), "image-grid" },
  { GIMP_UNDO_IMAGE_GUIDE, N_("Guide"), "image-guide" },
  { GIMP_UNDO_IMAGE_COLORMAP, N_("Change Indexed Palette"), "image-colormap" },
  { GIMP_UNDO_MASK, N_("Selection Mask"), "mask" },
  { GIMP_UNDO_ITEM_RENAME, N_("Rename Item"), "item-rename" },
  { GIMP_UNDO_ITEM_DISPLACE, N_("Move Item"), "item-displace" },
  { GIMP_UNDO_ITEM_VISIBILITY, N_("Item Visibility"), "item-visibility" },
  { GIMP_UNDO_ITEM_LINKED, N_("Set Item Linked"), "item-linked" },
  { GIMP_UNDO_LAYER_ADD, N_("New Layer"), "layer-add" },
  { GIMP_UNDO_LAYER_REMOVE, N_("Delete Layer"), "layer-remove" },
  { GIMP_UNDO_LAYER_MOD, N_("Layer Mod"), "layer-mod" },
  { GIMP_UNDO_LAYER_MASK_ADD, N_("Add Layer Mask"), "layer-mask-add" },
  { GIMP_UNDO_LAYER_MASK_REMOVE, N_("Delete Layer Mask"), "layer-mask-remove" },
  { GIMP_UNDO_LAYER_REPOSITION, N_("Reposition Layer"), "layer-reposition" },
  { GIMP_UNDO_LAYER_MODE, N_("Set Layer Mode"), "layer-mode" },
  { GIMP_UNDO_LAYER_OPACITY, N_("Set Layer Opacity"), "layer-opacity" },
  { GIMP_UNDO_LAYER_PRESERVE_TRANS, N_("Set Preserve Trans"), "layer-preserve-trans" },
  { GIMP_UNDO_CHANNEL_ADD, N_("New Channel"), "channel-add" },
  { GIMP_UNDO_CHANNEL_REMOVE, N_("Delete Channel"), "channel-remove" },
  { GIMP_UNDO_CHANNEL_MOD, N_("Channel Mod"), "channel-mod" },
  { GIMP_UNDO_CHANNEL_REPOSITION, N_("Reposition Channel"), "channel-reposition" },
  { GIMP_UNDO_CHANNEL_COLOR, N_("Channel Color"), "channel-color" },
  { GIMP_UNDO_VECTORS_ADD, N_("New Vectors"), "vectors-add" },
  { GIMP_UNDO_VECTORS_REMOVE, N_("Delete Vectors"), "vectors-remove" },
  { GIMP_UNDO_VECTORS_MOD, N_("Vectors Mod"), "vectors-mod" },
  { GIMP_UNDO_VECTORS_REPOSITION, N_("Reposition Vectors"), "vectors-reposition" },
  { GIMP_UNDO_FS_TO_LAYER, N_("FS to Layer"), "fs-to-layer" },
  { GIMP_UNDO_FS_RIGOR, N_("FS Rigor"), "fs-rigor" },
  { GIMP_UNDO_FS_RELAX, N_("FS Relax"), "fs-relax" },
  { GIMP_UNDO_TRANSFORM, N_("Transform"), "transform" },
  { GIMP_UNDO_PAINT, N_("Paint"), "paint" },
  { GIMP_UNDO_PARASITE_ATTACH, N_("Attach Parasite"), "parasite-attach" },
  { GIMP_UNDO_PARASITE_REMOVE, N_("Remove Parasite"), "parasite-remove" },
  { GIMP_UNDO_CANT, N_("EEK: can't undo"), "cant" },
  { 0, NULL, NULL }
};

GType
gimp_undo_type_get_type (void)
{
  static GType enum_type = 0;

  if (!enum_type)
    enum_type = g_enum_register_static ("GimpUndoType", gimp_undo_type_enum_values);

  return enum_type;
}


/* Generated data ends here */

