
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "core-enums.h"
#include "gimp-intl.h"

/* enumerations from "./core-enums.h" */
GType
gimp_add_mask_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_ADD_WHITE_MASK, N_("_White (full opacity)"), "white-mask" },
    { GIMP_ADD_BLACK_MASK, N_("_Black (full transparency)"), "black-mask" },
    { GIMP_ADD_ALPHA_MASK, N_("Layer's _alpha channel"), "alpha-mask" },
    { GIMP_ADD_ALPHA_TRANSFER_MASK, N_("_Transfer layer's alpha channel"), "alpha-transfer-mask" },
    { GIMP_ADD_SELECTION_MASK, N_("_Selection"), "selection-mask" },
    { GIMP_ADD_COPY_MASK, N_("_Grayscale copy of layer"), "copy-mask" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpAddMaskType", values);

  return type;
}

GType
gimp_blend_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_FG_BG_RGB_MODE, N_("FG to BG (RGB)"), "fg-bg-rgb-mode" },
    { GIMP_FG_BG_HSV_MODE, N_("FG to BG (HSV)"), "fg-bg-hsv-mode" },
    { GIMP_FG_TRANSPARENT_MODE, N_("FG to transparent"), "fg-transparent-mode" },
    { GIMP_CUSTOM_MODE, N_("Custom gradient"), "custom-mode" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpBlendMode", values);

  return type;
}

GType
gimp_bucket_fill_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_FG_BUCKET_FILL, N_("FG color fill"), "fg-bucket-fill" },
    { GIMP_BG_BUCKET_FILL, N_("BG color fill"), "bg-bucket-fill" },
    { GIMP_PATTERN_BUCKET_FILL, N_("Pattern fill"), "pattern-bucket-fill" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpBucketFillMode", values);

  return type;
}

GType
gimp_channel_ops_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_CHANNEL_OP_ADD, N_("Add to the current selection"), "add" },
    { GIMP_CHANNEL_OP_SUBTRACT, N_("Subtract from the current selection"), "subtract" },
    { GIMP_CHANNEL_OP_REPLACE, N_("Replace the current selection"), "replace" },
    { GIMP_CHANNEL_OP_INTERSECT, N_("Intersect with the current selection"), "intersect" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpChannelOps", values);

  return type;
}

GType
gimp_channel_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_RED_CHANNEL, N_("Red"), "red-channel" },
    { GIMP_GREEN_CHANNEL, N_("Green"), "green-channel" },
    { GIMP_BLUE_CHANNEL, N_("Blue"), "blue-channel" },
    { GIMP_GRAY_CHANNEL, N_("Gray"), "gray-channel" },
    { GIMP_INDEXED_CHANNEL, N_("Indexed"), "indexed-channel" },
    { GIMP_ALPHA_CHANNEL, N_("Alpha"), "alpha-channel" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpChannelType", values);

  return type;
}

GType
gimp_container_policy_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_CONTAINER_POLICY_STRONG, "GIMP_CONTAINER_POLICY_STRONG", "strong" },
    { GIMP_CONTAINER_POLICY_WEAK, "GIMP_CONTAINER_POLICY_WEAK", "weak" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpContainerPolicy", values);

  return type;
}

GType
gimp_convert_dither_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_NO_DITHER, N_("None"), "no-dither" },
    { GIMP_FS_DITHER, N_("Floyd-Steinberg (normal)"), "fs-dither" },
    { GIMP_FSLOWBLEED_DITHER, N_("Floyd-Steinberg (reduced color bleeding)"), "fslowbleed-dither" },
    { GIMP_FIXED_DITHER, N_("Positioned"), "fixed-dither" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpConvertDitherType", values);

  return type;
}

GType
gimp_convert_palette_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_MAKE_PALETTE, N_("Generate optimum palette"), "make-palette" },
    { GIMP_WEB_PALETTE, N_("Use web-optimized palette"), "web-palette" },
    { GIMP_MONO_PALETTE, N_("Use black and white (1-bit) palette"), "mono-palette" },
    { GIMP_CUSTOM_PALETTE, N_("Use custom palette"), "custom-palette" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpConvertPaletteType", values);

  return type;
}

GType
gimp_gravity_type_get_type (void)
{
  static const GEnumValue values[] =
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

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpGravityType", values);

  return type;
}

GType
gimp_fill_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_FOREGROUND_FILL, N_("Foreground color"), "foreground-fill" },
    { GIMP_BACKGROUND_FILL, N_("Background color"), "background-fill" },
    { GIMP_WHITE_FILL, N_("White"), "white-fill" },
    { GIMP_TRANSPARENT_FILL, N_("Transparency"), "transparent-fill" },
    { GIMP_PATTERN_FILL, N_("Pattern"), "pattern-fill" },
    { GIMP_NO_FILL, N_("None"), "no-fill" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpFillType", values);

  return type;
}

GType
gimp_gradient_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_GRADIENT_LINEAR, N_("Linear"), "linear" },
    { GIMP_GRADIENT_BILINEAR, N_("Bi-linear"), "bilinear" },
    { GIMP_GRADIENT_RADIAL, N_("Radial"), "radial" },
    { GIMP_GRADIENT_SQUARE, N_("Square"), "square" },
    { GIMP_GRADIENT_CONICAL_SYMMETRIC, N_("Conical (sym)"), "conical-symmetric" },
    { GIMP_GRADIENT_CONICAL_ASYMMETRIC, N_("Conical (asym)"), "conical-asymmetric" },
    { GIMP_GRADIENT_SHAPEBURST_ANGULAR, N_("Shaped (angular)"), "shapeburst-angular" },
    { GIMP_GRADIENT_SHAPEBURST_SPHERICAL, N_("Shaped (spherical)"), "shapeburst-spherical" },
    { GIMP_GRADIENT_SHAPEBURST_DIMPLED, N_("Shaped (dimpled)"), "shapeburst-dimpled" },
    { GIMP_GRADIENT_SPIRAL_CLOCKWISE, N_("Spiral (cw)"), "spiral-clockwise" },
    { GIMP_GRADIENT_SPIRAL_ANTICLOCKWISE, N_("Spiral (ccw)"), "spiral-anticlockwise" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpGradientType", values);

  return type;
}

GType
gimp_grid_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_GRID_DOTS, N_("Intersections (dots)"), "dots" },
    { GIMP_GRID_INTERSECTIONS, N_("Intersections (crosshairs)"), "intersections" },
    { GIMP_GRID_ON_OFF_DASH, N_("Dashed"), "on-off-dash" },
    { GIMP_GRID_DOUBLE_DASH, N_("Double dashed"), "double-dash" },
    { GIMP_GRID_SOLID, N_("Solid"), "solid" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpGridStyle", values);

  return type;
}

GType
gimp_stroke_method_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_STROKE_METHOD_LIBART, N_("Stroke line"), "libart" },
    { GIMP_STROKE_METHOD_PAINT_CORE, N_("Stroke with a paint tool"), "paint-core" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpStrokeMethod", values);

  return type;
}

GType
gimp_stroke_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_STROKE_STYLE_SOLID, N_("Solid"), "solid" },
    { GIMP_STROKE_STYLE_PATTERN, N_("Pattern"), "pattern" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpStrokeStyle", values);

  return type;
}

GType
gimp_join_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_JOIN_MITER, N_("Miter"), "miter" },
    { GIMP_JOIN_ROUND, N_("Round"), "round" },
    { GIMP_JOIN_BEVEL, N_("Bevel"), "bevel" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpJoinStyle", values);

  return type;
}

GType
gimp_cap_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_CAP_BUTT, N_("Butt"), "butt" },
    { GIMP_CAP_ROUND, N_("Round"), "round" },
    { GIMP_CAP_SQUARE, N_("Square"), "square" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpCapStyle", values);

  return type;
}

GType
gimp_dash_preset_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_DASH_CUSTOM, N_("Custom"), "custom" },
    { GIMP_DASH_LINE, N_("Line"), "line" },
    { GIMP_DASH_LONG_DASH, N_("Long dashes"), "long-dash" },
    { GIMP_DASH_MEDIUM_DASH, N_("Medium dashes"), "medium-dash" },
    { GIMP_DASH_SHORT_DASH, N_("Short dashes"), "short-dash" },
    { GIMP_DASH_SPARSE_DOTS, N_("Sparse dots"), "sparse-dots" },
    { GIMP_DASH_NORMAL_DOTS, N_("Normal dots"), "normal-dots" },
    { GIMP_DASH_DENSE_DOTS, N_("Dense dots"), "dense-dots" },
    { GIMP_DASH_STIPPLES, N_("Stipples"), "stipples" },
    { GIMP_DASH_DASH_DOT, N_("Dash dot..."), "dash-dot" },
    { GIMP_DASH_DASH_DOT_DOT, N_("Dash dot dot..."), "dash-dot-dot" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpDashPreset", values);

  return type;
}

GType
gimp_icon_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_ICON_TYPE_STOCK_ID, N_("Stock ID"), "stock-id" },
    { GIMP_ICON_TYPE_INLINE_PIXBUF, N_("Inline pixbuf"), "inline-pixbuf" },
    { GIMP_ICON_TYPE_IMAGE_FILE, N_("Image file"), "image-file" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpIconType", values);

  return type;
}

GType
gimp_brush_generated_shape_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_BRUSH_GENERATED_CIRCLE, N_("Circle"), "circle" },
    { GIMP_BRUSH_GENERATED_SQUARE, N_("Square"), "square" },
    { GIMP_BRUSH_GENERATED_DIAMOND, N_("Diamond"), "diamond" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpBrushGeneratedShape", values);

  return type;
}

GType
gimp_orientation_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_ORIENTATION_HORIZONTAL, N_("Horizontal"), "horizontal" },
    { GIMP_ORIENTATION_VERTICAL, N_("Vertical"), "vertical" },
    { GIMP_ORIENTATION_UNKNOWN, N_("Unknown"), "unknown" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpOrientationType", values);

  return type;
}

GType
gimp_rotation_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_ROTATE_90, "GIMP_ROTATE_90", "90" },
    { GIMP_ROTATE_180, "GIMP_ROTATE_180", "180" },
    { GIMP_ROTATE_270, "GIMP_ROTATE_270", "270" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpRotationType", values);

  return type;
}

GType
gimp_view_size_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_VIEW_SIZE_TINY, N_("Tiny"), "tiny" },
    { GIMP_VIEW_SIZE_EXTRA_SMALL, N_("Very small"), "extra-small" },
    { GIMP_VIEW_SIZE_SMALL, N_("Small"), "small" },
    { GIMP_VIEW_SIZE_MEDIUM, N_("Medium"), "medium" },
    { GIMP_VIEW_SIZE_LARGE, N_("Large"), "large" },
    { GIMP_VIEW_SIZE_EXTRA_LARGE, N_("Very large"), "extra-large" },
    { GIMP_VIEW_SIZE_HUGE, N_("Huge"), "huge" },
    { GIMP_VIEW_SIZE_ENORMOUS, N_("Enormous"), "enormous" },
    { GIMP_VIEW_SIZE_GIGANTIC, N_("Gigantic"), "gigantic" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpViewSize", values);

  return type;
}

GType
gimp_repeat_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_REPEAT_NONE, N_("None"), "none" },
    { GIMP_REPEAT_SAWTOOTH, N_("Sawtooth wave"), "sawtooth" },
    { GIMP_REPEAT_TRIANGULAR, N_("Triangular wave"), "triangular" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpRepeatMode", values);

  return type;
}

GType
gimp_selection_control_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_SELECTION_OFF, "GIMP_SELECTION_OFF", "off" },
    { GIMP_SELECTION_LAYER_OFF, "GIMP_SELECTION_LAYER_OFF", "layer-off" },
    { GIMP_SELECTION_ON, "GIMP_SELECTION_ON", "on" },
    { GIMP_SELECTION_PAUSE, "GIMP_SELECTION_PAUSE", "pause" },
    { GIMP_SELECTION_RESUME, "GIMP_SELECTION_RESUME", "resume" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpSelectionControl", values);

  return type;
}

GType
gimp_thumbnail_size_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_THUMBNAIL_SIZE_NONE, N_("No thumbnails"), "none" },
    { GIMP_THUMBNAIL_SIZE_NORMAL, N_("Normal (128x128)"), "normal" },
    { GIMP_THUMBNAIL_SIZE_LARGE, N_("Large (256x256)"), "large" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpThumbnailSize", values);

  return type;
}

GType
gimp_transform_direction_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_TRANSFORM_FORWARD, N_("Forward (traditional)"), "forward" },
    { GIMP_TRANSFORM_BACKWARD, N_("Backward (corrective)"), "backward" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpTransformDirection", values);

  return type;
}

GType
gimp_undo_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_UNDO_MODE_UNDO, "GIMP_UNDO_MODE_UNDO", "undo" },
    { GIMP_UNDO_MODE_REDO, "GIMP_UNDO_MODE_REDO", "redo" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpUndoMode", values);

  return type;
}

GType
gimp_undo_event_get_type (void)
{
  static const GEnumValue values[] =
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

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpUndoEvent", values);

  return type;
}

GType
gimp_undo_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_UNDO_GROUP_NONE, N_("<<invalid>>"), "group-none" },
    { GIMP_UNDO_GROUP_IMAGE_SCALE, N_("Scale image"), "group-image-scale" },
    { GIMP_UNDO_GROUP_IMAGE_RESIZE, N_("Resize image"), "group-image-resize" },
    { GIMP_UNDO_GROUP_IMAGE_FLIP, N_("Flip image"), "group-image-flip" },
    { GIMP_UNDO_GROUP_IMAGE_ROTATE, N_("Rotate image"), "group-image-rotate" },
    { GIMP_UNDO_GROUP_IMAGE_CROP, N_("Crop image"), "group-image-crop" },
    { GIMP_UNDO_GROUP_IMAGE_CONVERT, N_("Convert image"), "group-image-convert" },
    { GIMP_UNDO_GROUP_IMAGE_LAYERS_MERGE, N_("Merge layers"), "group-image-layers-merge" },
    { GIMP_UNDO_GROUP_IMAGE_VECTORS_MERGE, N_("Merge vectors"), "group-image-vectors-merge" },
    { GIMP_UNDO_GROUP_IMAGE_QMASK, N_("Quick Mask"), "group-image-qmask" },
    { GIMP_UNDO_GROUP_IMAGE_GRID, N_("Grid"), "group-image-grid" },
    { GIMP_UNDO_GROUP_IMAGE_GUIDE, N_("Guide"), "group-image-guide" },
    { GIMP_UNDO_GROUP_DRAWABLE, N_("Drawable"), "group-drawable" },
    { GIMP_UNDO_GROUP_DRAWABLE_MOD, N_("Drawable mod"), "group-drawable-mod" },
    { GIMP_UNDO_GROUP_MASK, N_("Selection mask"), "group-mask" },
    { GIMP_UNDO_GROUP_ITEM_VISIBILITY, N_("Item visibility"), "group-item-visibility" },
    { GIMP_UNDO_GROUP_ITEM_LINKED, N_("Linked item"), "group-item-linked" },
    { GIMP_UNDO_GROUP_ITEM_PROPERTIES, N_("Item properties"), "group-item-properties" },
    { GIMP_UNDO_GROUP_ITEM_DISPLACE, N_("Move item"), "group-item-displace" },
    { GIMP_UNDO_GROUP_ITEM_SCALE, N_("Scale item"), "group-item-scale" },
    { GIMP_UNDO_GROUP_ITEM_RESIZE, N_("Resize item"), "group-item-resize" },
    { GIMP_UNDO_GROUP_LAYER_ADD_MASK, N_("Add layer mask"), "group-layer-add-mask" },
    { GIMP_UNDO_GROUP_LAYER_APPLY_MASK, N_("Apply layer mask"), "group-layer-apply-mask" },
    { GIMP_UNDO_GROUP_FS_TO_LAYER, N_("Floating selection to layer"), "group-fs-to-layer" },
    { GIMP_UNDO_GROUP_FS_FLOAT, N_("Float selection"), "group-fs-float" },
    { GIMP_UNDO_GROUP_FS_ANCHOR, N_("Anchor floating selection"), "group-fs-anchor" },
    { GIMP_UNDO_GROUP_FS_REMOVE, N_("Remove floating selection"), "group-fs-remove" },
    { GIMP_UNDO_GROUP_EDIT_PASTE, N_("Paste"), "group-edit-paste" },
    { GIMP_UNDO_GROUP_EDIT_CUT, N_("Cut"), "group-edit-cut" },
    { GIMP_UNDO_GROUP_TEXT, N_("Text"), "group-text" },
    { GIMP_UNDO_GROUP_TRANSFORM, N_("Transform"), "group-transform" },
    { GIMP_UNDO_GROUP_PAINT, N_("Paint"), "group-paint" },
    { GIMP_UNDO_GROUP_PARASITE_ATTACH, N_("Attach parasite"), "group-parasite-attach" },
    { GIMP_UNDO_GROUP_PARASITE_REMOVE, N_("Remove parasite"), "group-parasite-remove" },
    { GIMP_UNDO_GROUP_VECTORS_IMPORT, N_("Import paths"), "group-vectors-import" },
    { GIMP_UNDO_GROUP_MISC, N_("Plug-In"), "group-misc" },
    { GIMP_UNDO_IMAGE_TYPE, N_("Image type"), "image-type" },
    { GIMP_UNDO_IMAGE_SIZE, N_("Image size"), "image-size" },
    { GIMP_UNDO_IMAGE_RESOLUTION, N_("Resolution change"), "image-resolution" },
    { GIMP_UNDO_IMAGE_GRID, N_("Grid"), "image-grid" },
    { GIMP_UNDO_IMAGE_GUIDE, N_("Guide"), "image-guide" },
    { GIMP_UNDO_IMAGE_COLORMAP, N_("Change indexed palette"), "image-colormap" },
    { GIMP_UNDO_DRAWABLE, N_("Drawable"), "drawable" },
    { GIMP_UNDO_DRAWABLE_MOD, N_("Drawable mod"), "drawable-mod" },
    { GIMP_UNDO_MASK, N_("Selection mask"), "mask" },
    { GIMP_UNDO_ITEM_RENAME, N_("Rename item"), "item-rename" },
    { GIMP_UNDO_ITEM_DISPLACE, N_("Move item"), "item-displace" },
    { GIMP_UNDO_ITEM_VISIBILITY, N_("Item visibility"), "item-visibility" },
    { GIMP_UNDO_ITEM_LINKED, N_("Set item linked"), "item-linked" },
    { GIMP_UNDO_LAYER_ADD, N_("New layer"), "layer-add" },
    { GIMP_UNDO_LAYER_REMOVE, N_("Delete layer"), "layer-remove" },
    { GIMP_UNDO_LAYER_MASK_ADD, N_("Add layer mask"), "layer-mask-add" },
    { GIMP_UNDO_LAYER_MASK_REMOVE, N_("Delete layer mask"), "layer-mask-remove" },
    { GIMP_UNDO_LAYER_REPOSITION, N_("Reposition layer"), "layer-reposition" },
    { GIMP_UNDO_LAYER_MODE, N_("Set layer mode"), "layer-mode" },
    { GIMP_UNDO_LAYER_OPACITY, N_("Set layer opacity"), "layer-opacity" },
    { GIMP_UNDO_LAYER_PRESERVE_TRANS, N_("Set preserve trans"), "layer-preserve-trans" },
    { GIMP_UNDO_TEXT_LAYER, N_("Text"), "text-layer" },
    { GIMP_UNDO_TEXT_LAYER_MODIFIED, N_("Text modified"), "text-layer-modified" },
    { GIMP_UNDO_CHANNEL_ADD, N_("New channel"), "channel-add" },
    { GIMP_UNDO_CHANNEL_REMOVE, N_("Delete channel"), "channel-remove" },
    { GIMP_UNDO_CHANNEL_REPOSITION, N_("Reposition channel"), "channel-reposition" },
    { GIMP_UNDO_CHANNEL_COLOR, N_("Channel color"), "channel-color" },
    { GIMP_UNDO_VECTORS_ADD, N_("New vectors"), "vectors-add" },
    { GIMP_UNDO_VECTORS_REMOVE, N_("Delete vectors"), "vectors-remove" },
    { GIMP_UNDO_VECTORS_MOD, N_("Vectors mod"), "vectors-mod" },
    { GIMP_UNDO_VECTORS_REPOSITION, N_("Reposition vectors"), "vectors-reposition" },
    { GIMP_UNDO_FS_TO_LAYER, N_("FS to layer"), "fs-to-layer" },
    { GIMP_UNDO_FS_RIGOR, N_("FS rigor"), "fs-rigor" },
    { GIMP_UNDO_FS_RELAX, N_("FS relax"), "fs-relax" },
    { GIMP_UNDO_TRANSFORM, N_("Transform"), "transform" },
    { GIMP_UNDO_PAINT, N_("Paint"), "paint" },
    { GIMP_UNDO_PARASITE_ATTACH, N_("Attach parasite"), "parasite-attach" },
    { GIMP_UNDO_PARASITE_REMOVE, N_("Remove parasite"), "parasite-remove" },
    { GIMP_UNDO_CANT, N_("EEK: can't undo"), "cant" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_enum_register_static ("GimpUndoType", values);

  return type;
}

GType
gimp_dirty_mask_get_type (void)
{
  static const GFlagsValue values[] =
  {
    { GIMP_DIRTY_NONE, "GIMP_DIRTY_NONE", "none" },
    { GIMP_DIRTY_IMAGE, "GIMP_DIRTY_IMAGE", "image" },
    { GIMP_DIRTY_IMAGE_SIZE, "GIMP_DIRTY_IMAGE_SIZE", "image-size" },
    { GIMP_DIRTY_IMAGE_META, "GIMP_DIRTY_IMAGE_META", "image-meta" },
    { GIMP_DIRTY_IMAGE_STRUCTURE, "GIMP_DIRTY_IMAGE_STRUCTURE", "image-structure" },
    { GIMP_DIRTY_ITEM, "GIMP_DIRTY_ITEM", "item" },
    { GIMP_DIRTY_ITEM_META, "GIMP_DIRTY_ITEM_META", "item-meta" },
    { GIMP_DIRTY_DRAWABLE, "GIMP_DIRTY_DRAWABLE", "drawable" },
    { GIMP_DIRTY_VECTORS, "GIMP_DIRTY_VECTORS", "vectors" },
    { GIMP_DIRTY_SELECTION, "GIMP_DIRTY_SELECTION", "selection" },
    { GIMP_DIRTY_ALL, "GIMP_DIRTY_ALL", "all" },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    type = g_flags_register_static ("GimpDirtyMask", values);

  return type;
}


/* Generated data ends here */

