/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __CORE_ENUMS_H__
#define __CORE_ENUMS_H__


#if 0
   This file is parsed by two scripts, enumgen.pl in tools/pdbgen,
   and gimp-mkenums. All enums that are not marked with
   /*< pdb-skip >*/ are exported to libgimp and the PDB. Enums that are
   not marked with /*< skip >*/ are registered with the GType system.
   If you want the enum to be skipped by both scripts, you have to use
   /*< pdb-skip, skip >*/.

   The same syntax applies to enum values.
#endif


/*
 * these enums that are registered with the type system
 */

#define GIMP_TYPE_ADD_MASK_TYPE (gimp_add_mask_type_get_type ())

GType gimp_add_mask_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_ADD_WHITE_MASK,          /*< desc="_White (full opacity)"           >*/
  GIMP_ADD_BLACK_MASK,          /*< desc="_Black (full transparency)"      >*/
  GIMP_ADD_ALPHA_MASK,          /*< desc="Layer's _alpha channel"          >*/
  GIMP_ADD_ALPHA_TRANSFER_MASK, /*< desc="_Transfer layer's alpha channel" >*/
  GIMP_ADD_SELECTION_MASK,      /*< desc="_Selection"                      >*/
  GIMP_ADD_COPY_MASK            /*< desc="_Grayscale copy of layer"        >*/
} GimpAddMaskType;


#define GIMP_TYPE_BLEND_MODE (gimp_blend_mode_get_type ())

GType gimp_blend_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_FG_BG_RGB_MODE,         /*< desc="FG to BG (RGB)"    >*/
  GIMP_FG_BG_HSV_MODE,         /*< desc="FG to BG (HSV)"    >*/
  GIMP_FG_TRANSPARENT_MODE,    /*< desc="FG to transparent" >*/
  GIMP_CUSTOM_MODE             /*< desc="Custom gradient"   >*/
} GimpBlendMode;


#define GIMP_TYPE_BUCKET_FILL_MODE (gimp_bucket_fill_mode_get_type ())

GType gimp_bucket_fill_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_FG_BUCKET_FILL,      /*< desc="FG color fill" >*/
  GIMP_BG_BUCKET_FILL,      /*< desc="BG color fill" >*/
  GIMP_PATTERN_BUCKET_FILL  /*< desc="Pattern fill"  >*/
} GimpBucketFillMode;


#define GIMP_TYPE_CHANNEL_OPS (gimp_channel_ops_get_type ())

GType gimp_channel_ops_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_CHANNEL_OP_ADD,       /*< desc="Add to the current selection"         >*/
  GIMP_CHANNEL_OP_SUBTRACT,  /*< desc="Subtract from the current selection"  >*/
  GIMP_CHANNEL_OP_REPLACE,   /*< desc="Replace the current selection"        >*/
  GIMP_CHANNEL_OP_INTERSECT  /*< desc="Intersect with the current selection" >*/
} GimpChannelOps;


#define GIMP_TYPE_CHANNEL_TYPE (gimp_channel_type_get_type ())

GType gimp_channel_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_RED_CHANNEL,      /*< desc="Red"     >*/
  GIMP_GREEN_CHANNEL,    /*< desc="Green"   >*/
  GIMP_BLUE_CHANNEL,     /*< desc="Blue"    >*/
  GIMP_GRAY_CHANNEL,     /*< desc="Gray"    >*/
  GIMP_INDEXED_CHANNEL,  /*< desc="Indexed" >*/
  GIMP_ALPHA_CHANNEL     /*< desc="Alpha"   >*/
} GimpChannelType;


#define GIMP_TYPE_CONTAINER_POLICY (gimp_container_policy_get_type ())

GType gimp_container_policy_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_CONTAINER_POLICY_STRONG,
  GIMP_CONTAINER_POLICY_WEAK
} GimpContainerPolicy;


#define GIMP_TYPE_CONVERT_DITHER_TYPE (gimp_convert_dither_type_get_type ())

GType gimp_convert_dither_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_NO_DITHER,         /*< desc="None"                                     >*/
  GIMP_FS_DITHER,         /*< desc="Floyd-Steinberg (normal)"                 >*/
  GIMP_FSLOWBLEED_DITHER, /*< desc="Floyd-Steinberg (reduced color bleeding)" >*/
  GIMP_FIXED_DITHER,      /*< desc="Positioned"                               >*/
  GIMP_NODESTRUCT_DITHER  /*< pdb-skip, skip >*/
} GimpConvertDitherType;


#define GIMP_TYPE_CONVERT_PALETTE_TYPE (gimp_convert_palette_type_get_type ())

GType gimp_convert_palette_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_MAKE_PALETTE,    /*< desc="Generate optimum palette"            >*/
  GIMP_REUSE_PALETTE,   /*< skip >*/
  GIMP_WEB_PALETTE,     /*< desc="Use web-optimized palette"           >*/
  GIMP_MONO_PALETTE,    /*< desc="Use black and white (1-bit) palette" >*/
  GIMP_CUSTOM_PALETTE   /*< desc="Use custom palette"                  >*/
} GimpConvertPaletteType;


#define GIMP_TYPE_GRAVITY_TYPE (gimp_gravity_type_get_type ())

GType gimp_gravity_type_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_GRAVITY_NONE,
  GIMP_GRAVITY_NORTH_WEST,
  GIMP_GRAVITY_NORTH,
  GIMP_GRAVITY_NORTH_EAST,
  GIMP_GRAVITY_WEST,
  GIMP_GRAVITY_CENTER,
  GIMP_GRAVITY_EAST,
  GIMP_GRAVITY_SOUTH_WEST,
  GIMP_GRAVITY_SOUTH,
  GIMP_GRAVITY_SOUTH_EAST
} GimpGravityType;


#define GIMP_TYPE_FILL_TYPE (gimp_fill_type_get_type ())

GType gimp_fill_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_FOREGROUND_FILL,   /*< desc="Foreground color" >*/
  GIMP_BACKGROUND_FILL,   /*< desc="Background color" >*/
  GIMP_WHITE_FILL,        /*< desc="White"            >*/
  GIMP_TRANSPARENT_FILL,  /*< desc="Transparency"     >*/
  GIMP_PATTERN_FILL,      /*< desc="Pattern"          >*/
  GIMP_NO_FILL            /*< desc="None",   pdb-skip >*/
} GimpFillType;


#define GIMP_TYPE_GRADIENT_TYPE (gimp_gradient_type_get_type ())

GType gimp_gradient_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_GRADIENT_LINEAR,                /*< desc="Linear"            >*/
  GIMP_GRADIENT_BILINEAR,              /*< desc="Bi-linear"         >*/
  GIMP_GRADIENT_RADIAL,                /*< desc="Radial"            >*/
  GIMP_GRADIENT_SQUARE,                /*< desc="Square"            >*/
  GIMP_GRADIENT_CONICAL_SYMMETRIC,     /*< desc="Conical (sym)"     >*/
  GIMP_GRADIENT_CONICAL_ASYMMETRIC,    /*< desc="Conical (asym)"    >*/
  GIMP_GRADIENT_SHAPEBURST_ANGULAR,    /*< desc="Shaped (angular)"  >*/
  GIMP_GRADIENT_SHAPEBURST_SPHERICAL,  /*< desc="Shaped (spherical)">*/
  GIMP_GRADIENT_SHAPEBURST_DIMPLED,    /*< desc="Shaped (dimpled)"  >*/
  GIMP_GRADIENT_SPIRAL_CLOCKWISE,      /*< desc="Spiral (cw)"       >*/
  GIMP_GRADIENT_SPIRAL_ANTICLOCKWISE   /*< desc="Spiral (ccw)"      >*/
} GimpGradientType;


#define GIMP_TYPE_GRID_STYLE (gimp_grid_style_get_type ())

GType gimp_grid_style_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_GRID_DOTS,           /*< desc="Intersections (dots)"       >*/
  GIMP_GRID_INTERSECTIONS,  /*< desc="Intersections (crosshairs)" >*/
  GIMP_GRID_ON_OFF_DASH,    /*< desc="Dashed"                     >*/
  GIMP_GRID_DOUBLE_DASH,    /*< desc="Double dashed"              >*/
  GIMP_GRID_SOLID           /*< desc="Solid"                      >*/
} GimpGridStyle;


#define GIMP_TYPE_STROKE_METHOD (gimp_stroke_method_get_type ())

GType gimp_stroke_method_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_STROKE_METHOD_LIBART,     /*< desc="Stroke line"              >*/
  GIMP_STROKE_METHOD_PAINT_CORE  /*< desc="Stroke with a paint tool" >*/
} GimpStrokeMethod;


#define GIMP_TYPE_STROKE_STYLE (gimp_stroke_style_get_type ())

GType gimp_stroke_style_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_STROKE_STYLE_SOLID,  /*< desc="Solid"   >*/
  GIMP_STROKE_STYLE_PATTERN /*< desc="Pattern" >*/
} GimpStrokeStyle;


#define GIMP_TYPE_JOIN_STYLE (gimp_join_style_get_type ())

GType gimp_join_style_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_JOIN_MITER,  /*< desc="Miter" >*/
  GIMP_JOIN_ROUND,  /*< desc="Round" >*/
  GIMP_JOIN_BEVEL   /*< desc="Bevel" >*/
} GimpJoinStyle;


#define GIMP_TYPE_CAP_STYLE (gimp_cap_style_get_type ())

GType gimp_cap_style_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_CAP_BUTT,   /*< desc="Butt"   >*/
  GIMP_CAP_ROUND,  /*< desc="Round"  >*/
  GIMP_CAP_SQUARE  /*< desc="Square" >*/
} GimpCapStyle;


#define GIMP_TYPE_DASH_PRESET (gimp_dash_preset_get_type ())

GType gimp_dash_preset_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_DASH_CUSTOM,       /*< desc="Custom"          >*/
  GIMP_DASH_LINE,         /*< desc="Line"            >*/
  GIMP_DASH_LONG_DASH,    /*< desc="Long dashes"     >*/
  GIMP_DASH_MEDIUM_DASH,  /*< desc="Medium dashes"   >*/
  GIMP_DASH_SHORT_DASH,   /*< desc="Short dashes"    >*/
  GIMP_DASH_SPARSE_DOTS,  /*< desc="Sparse dots"     >*/
  GIMP_DASH_NORMAL_DOTS,  /*< desc="Normal dots"     >*/
  GIMP_DASH_DENSE_DOTS,   /*< desc="Dense dots"      >*/
  GIMP_DASH_STIPPLES,     /*< desc="Stipples"        >*/
  GIMP_DASH_DASH_DOT,     /*< desc="Dash dot..."     >*/
  GIMP_DASH_DASH_DOT_DOT  /*< desc="Dash dot dot..." >*/
} GimpDashPreset;


#define GIMP_TYPE_ICON_TYPE (gimp_icon_type_get_type ())

GType gimp_icon_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_ICON_TYPE_STOCK_ID,      /*< desc="Stock ID"      >*/
  GIMP_ICON_TYPE_INLINE_PIXBUF, /*< desc="Inline pixbuf" >*/
  GIMP_ICON_TYPE_IMAGE_FILE     /*< desc="Image file"    >*/
} GimpIconType;


#define GIMP_TYPE_BRUSH_GENERATED_SHAPE (gimp_brush_generated_shape_get_type ())

GType gimp_brush_generated_shape_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_BRUSH_GENERATED_CIRCLE,  /*< desc="Circle"  >*/
  GIMP_BRUSH_GENERATED_SQUARE,  /*< desc="Square"  >*/
  GIMP_BRUSH_GENERATED_DIAMOND  /*< desc="Diamond" >*/
} GimpBrushGeneratedShape;


#define GIMP_TYPE_ORIENTATION_TYPE (gimp_orientation_type_get_type ())

GType gimp_orientation_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_ORIENTATION_HORIZONTAL, /*< desc="Horizontal" >*/
  GIMP_ORIENTATION_VERTICAL,   /*< desc="Vertical"   >*/
  GIMP_ORIENTATION_UNKNOWN     /*< desc="Unknown"    >*/
} GimpOrientationType;


#define GIMP_TYPE_ROTATION_TYPE (gimp_rotation_type_get_type ())

GType gimp_rotation_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_ROTATE_90,
  GIMP_ROTATE_180,
  GIMP_ROTATE_270
} GimpRotationType;


#define GIMP_TYPE_VIEW_SIZE (gimp_view_size_get_type ())

GType gimp_view_size_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_VIEW_SIZE_TINY        = 12,   /*< desc="Tiny"        >*/
  GIMP_VIEW_SIZE_EXTRA_SMALL = 16,   /*< desc="Very small"  >*/
  GIMP_VIEW_SIZE_SMALL       = 24,   /*< desc="Small"       >*/
  GIMP_VIEW_SIZE_MEDIUM      = 32,   /*< desc="Medium"      >*/
  GIMP_VIEW_SIZE_LARGE       = 48,   /*< desc="Large"       >*/
  GIMP_VIEW_SIZE_EXTRA_LARGE = 64,   /*< desc="Very large"  >*/
  GIMP_VIEW_SIZE_HUGE        = 96,   /*< desc="Huge"        >*/
  GIMP_VIEW_SIZE_ENORMOUS    = 128,  /*< desc="Enormous"    >*/
  GIMP_VIEW_SIZE_GIGANTIC    = 192   /*< desc="Gigantic"    >*/
} GimpViewSize;


#define GIMP_TYPE_REPEAT_MODE (gimp_repeat_mode_get_type ())

GType gimp_repeat_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_REPEAT_NONE,       /*< desc="None"            >*/
  GIMP_REPEAT_SAWTOOTH,   /*< desc="Sawtooth wave"   >*/
  GIMP_REPEAT_TRIANGULAR  /*< desc="Triangular wave" >*/
} GimpRepeatMode;


#define GIMP_TYPE_SELECTION_CONTROL (gimp_selection_control_get_type ())

GType gimp_selection_control_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_SELECTION_OFF,
  GIMP_SELECTION_LAYER_OFF,
  GIMP_SELECTION_ON,
  GIMP_SELECTION_PAUSE,
  GIMP_SELECTION_RESUME
} GimpSelectionControl;


#define GIMP_TYPE_THUMBNAIL_SIZE (gimp_thumbnail_size_get_type ())

GType gimp_thumbnail_size_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_THUMBNAIL_SIZE_NONE    = 0,    /*< desc="No thumbnails"    >*/
  GIMP_THUMBNAIL_SIZE_NORMAL  = 128,  /*< desc="Normal (128x128)" >*/
  GIMP_THUMBNAIL_SIZE_LARGE   = 256   /*< desc="Large (256x256)"  >*/
} GimpThumbnailSize;


#define GIMP_TYPE_TRANSFORM_DIRECTION (gimp_transform_direction_get_type ())

GType gimp_transform_direction_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_TRANSFORM_FORWARD,   /*< desc="Forward (traditional)" >*/
  GIMP_TRANSFORM_BACKWARD   /*< desc="Backward (corrective)" >*/
} GimpTransformDirection;


#define GIMP_TYPE_UNDO_MODE (gimp_undo_mode_get_type ())

GType gimp_undo_mode_get_type (void) G_GNUC_CONST;

typedef enum /*< pdb-skip >*/
{
  GIMP_UNDO_MODE_UNDO,
  GIMP_UNDO_MODE_REDO
} GimpUndoMode;


#define GIMP_TYPE_UNDO_EVENT (gimp_undo_event_get_type ())

GType gimp_undo_event_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_UNDO_EVENT_UNDO_PUSHED,  /* a new undo has been added to the undo stack */
  GIMP_UNDO_EVENT_UNDO_EXPIRED, /* an undo has been freed from the undo stack  */
  GIMP_UNDO_EVENT_REDO_EXPIRED, /* a redo has been freed from the redo stack   */
  GIMP_UNDO_EVENT_UNDO,         /* an undo has been executed                   */
  GIMP_UNDO_EVENT_REDO,         /* a redo has been executed                    */
  GIMP_UNDO_EVENT_UNDO_FREE,    /* all undo and redo info has been cleared     */
  GIMP_UNDO_EVENT_UNDO_FREEZE,  /* undo has been frozen                        */
  GIMP_UNDO_EVENT_UNDO_THAW     /* undo has been thawn                         */
} GimpUndoEvent;


#define GIMP_TYPE_UNDO_TYPE (gimp_undo_type_get_type ())

GType gimp_undo_type_get_type (void) G_GNUC_CONST;

typedef enum /*< pdb-skip >*/
{
  /* Type NO_UNDO_GROUP (0) is special - in the gimpimage structure it
   * means there is no undo group currently being added to.
   */
  GIMP_UNDO_GROUP_NONE = 0,           /*< desc="<<invalid>>"                 >*/

  GIMP_UNDO_GROUP_FIRST = GIMP_UNDO_GROUP_NONE, /*< skip >*/

  GIMP_UNDO_GROUP_IMAGE_SCALE,        /*< desc="Scale image"                 >*/
  GIMP_UNDO_GROUP_IMAGE_RESIZE,       /*< desc="Resize image"                >*/
  GIMP_UNDO_GROUP_IMAGE_FLIP,         /*< desc="Flip image"                  >*/
  GIMP_UNDO_GROUP_IMAGE_ROTATE,       /*< desc="Rotate image"                >*/
  GIMP_UNDO_GROUP_IMAGE_CROP,         /*< desc="Crop image"                  >*/
  GIMP_UNDO_GROUP_IMAGE_CONVERT,      /*< desc="Convert image"               >*/
  GIMP_UNDO_GROUP_IMAGE_ITEM_REMOVE,  /*< desc="Remove item"                 >*/
  GIMP_UNDO_GROUP_IMAGE_LAYERS_MERGE, /*< desc="Merge layers"                >*/
  GIMP_UNDO_GROUP_IMAGE_VECTORS_MERGE,/*< desc="Merge vectors"               >*/
  GIMP_UNDO_GROUP_IMAGE_QMASK,        /*< desc="Quick Mask"                  >*/
  GIMP_UNDO_GROUP_IMAGE_GRID,         /*< desc="Grid"                        >*/
  GIMP_UNDO_GROUP_IMAGE_GUIDE,        /*< desc="Guide"                       >*/
  GIMP_UNDO_GROUP_DRAWABLE,           /*< desc="Drawable"                    >*/
  GIMP_UNDO_GROUP_DRAWABLE_MOD,       /*< desc="Drawable mod"                >*/
  GIMP_UNDO_GROUP_MASK,               /*< desc="Selection mask"              >*/
  GIMP_UNDO_GROUP_ITEM_VISIBILITY,    /*< desc="Item visibility"             >*/
  GIMP_UNDO_GROUP_ITEM_LINKED,        /*< desc="Linked item"                 >*/
  GIMP_UNDO_GROUP_ITEM_PROPERTIES,    /*< desc="Item properties"             >*/
  GIMP_UNDO_GROUP_ITEM_DISPLACE,      /*< desc="Move item"                   >*/
  GIMP_UNDO_GROUP_ITEM_SCALE,         /*< desc="Scale item"                  >*/
  GIMP_UNDO_GROUP_ITEM_RESIZE,        /*< desc="Resize item"                 >*/
  GIMP_UNDO_GROUP_LAYER_ADD_MASK,     /*< desc="Add layer mask"              >*/
  GIMP_UNDO_GROUP_LAYER_APPLY_MASK,   /*< desc="Apply layer mask"            >*/
  GIMP_UNDO_GROUP_FS_TO_LAYER,        /*< desc="Floating selection to layer" >*/
  GIMP_UNDO_GROUP_FS_FLOAT,           /*< desc="Float selection"             >*/
  GIMP_UNDO_GROUP_FS_ANCHOR,          /*< desc="Anchor floating selection"   >*/
  GIMP_UNDO_GROUP_FS_REMOVE,          /*< desc="Remove floating selection"   >*/
  GIMP_UNDO_GROUP_EDIT_PASTE,         /*< desc="Paste"                       >*/
  GIMP_UNDO_GROUP_EDIT_CUT,           /*< desc="Cut"                         >*/
  GIMP_UNDO_GROUP_TEXT,               /*< desc="Text"                        >*/
  GIMP_UNDO_GROUP_TRANSFORM,          /*< desc="Transform"                   >*/
  GIMP_UNDO_GROUP_PAINT,              /*< desc="Paint"                       >*/
  GIMP_UNDO_GROUP_PARASITE_ATTACH,    /*< desc="Attach parasite"             >*/
  GIMP_UNDO_GROUP_PARASITE_REMOVE,    /*< desc="Remove parasite"             >*/
  GIMP_UNDO_GROUP_VECTORS_IMPORT,     /*< desc="Import paths"                >*/
  GIMP_UNDO_GROUP_MISC,               /*< desc="Plug-In"                     >*/

  GIMP_UNDO_GROUP_LAST = GIMP_UNDO_GROUP_MISC, /*< skip >*/

  /*  Undo types which actually do something  */

  GIMP_UNDO_IMAGE_TYPE,               /*< desc="Image type"                >*/
  GIMP_UNDO_IMAGE_SIZE,               /*< desc="Image size"                >*/
  GIMP_UNDO_IMAGE_RESOLUTION,         /*< desc="Resolution change"         >*/
  GIMP_UNDO_IMAGE_GRID,               /*< desc="Grid"                      >*/
  GIMP_UNDO_IMAGE_GUIDE,              /*< desc="Guide"                     >*/
  GIMP_UNDO_IMAGE_COLORMAP,           /*< desc="Change indexed palette"    >*/
  GIMP_UNDO_DRAWABLE,                 /*< desc="Drawable"                  >*/
  GIMP_UNDO_DRAWABLE_MOD,             /*< desc="Drawable mod"              >*/
  GIMP_UNDO_MASK,                     /*< desc="Selection mask"            >*/
  GIMP_UNDO_ITEM_RENAME,              /*< desc="Rename item"               >*/
  GIMP_UNDO_ITEM_DISPLACE,            /*< desc="Move item"                 >*/
  GIMP_UNDO_ITEM_VISIBILITY,          /*< desc="Item visibility"           >*/
  GIMP_UNDO_ITEM_LINKED,              /*< desc="Set item linked"           >*/
  GIMP_UNDO_LAYER_ADD,                /*< desc="New layer"                 >*/
  GIMP_UNDO_LAYER_REMOVE,             /*< desc="Delete layer"              >*/
  GIMP_UNDO_LAYER_MASK_ADD,           /*< desc="Add layer mask"            >*/
  GIMP_UNDO_LAYER_MASK_REMOVE,        /*< desc="Delete layer mask"         >*/
  GIMP_UNDO_LAYER_REPOSITION,         /*< desc="Reposition layer"          >*/
  GIMP_UNDO_LAYER_MODE,               /*< desc="Set layer mode"            >*/
  GIMP_UNDO_LAYER_OPACITY,            /*< desc="Set layer opacity"         >*/
  GIMP_UNDO_LAYER_PRESERVE_TRANS,     /*< desc="Set preserve trans"        >*/
  GIMP_UNDO_TEXT_LAYER,               /*< desc="Text"                      >*/
  GIMP_UNDO_TEXT_LAYER_MODIFIED,      /*< desc="Text modified"             >*/
  GIMP_UNDO_CHANNEL_ADD,              /*< desc="New channel"               >*/
  GIMP_UNDO_CHANNEL_REMOVE,           /*< desc="Delete channel"            >*/
  GIMP_UNDO_CHANNEL_REPOSITION,       /*< desc="Reposition channel"        >*/
  GIMP_UNDO_CHANNEL_COLOR,            /*< desc="Channel color"             >*/
  GIMP_UNDO_VECTORS_ADD,              /*< desc="New vectors"               >*/
  GIMP_UNDO_VECTORS_REMOVE,           /*< desc="Delete vectors"            >*/
  GIMP_UNDO_VECTORS_MOD,              /*< desc="Vectors mod"               >*/
  GIMP_UNDO_VECTORS_REPOSITION,       /*< desc="Reposition vectors"        >*/
  GIMP_UNDO_FS_TO_LAYER,              /*< desc="FS to layer"               >*/
  GIMP_UNDO_FS_RIGOR,                 /*< desc="FS rigor"                  >*/
  GIMP_UNDO_FS_RELAX,                 /*< desc="FS relax"                  >*/
  GIMP_UNDO_TRANSFORM,                /*< desc="Transform"                 >*/
  GIMP_UNDO_PAINT,                    /*< desc="Paint"                     >*/
  GIMP_UNDO_INK,                      /*< desc="Ink"                       >*/
  GIMP_UNDO_PARASITE_ATTACH,          /*< desc="Attach parasite"           >*/
  GIMP_UNDO_PARASITE_REMOVE,          /*< desc="Remove parasite"           >*/

  GIMP_UNDO_CANT                      /*< desc="EEK: can't undo"           >*/
} GimpUndoType;


#define GIMP_TYPE_DIRTY_MASK (gimp_dirty_mask_get_type ())

GType gimp_dirty_mask_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_DIRTY_NONE            = 0,

  GIMP_DIRTY_IMAGE           = 1 << 0,
  GIMP_DIRTY_IMAGE_SIZE      = 1 << 1,
  GIMP_DIRTY_IMAGE_META      = 1 << 2,
  GIMP_DIRTY_IMAGE_STRUCTURE = 1 << 3,
  GIMP_DIRTY_ITEM            = 1 << 4,
  GIMP_DIRTY_ITEM_META       = 1 << 5,
  GIMP_DIRTY_DRAWABLE        = 1 << 6,
  GIMP_DIRTY_VECTORS         = 1 << 7,
  GIMP_DIRTY_SELECTION       = 1 << 8,

  GIMP_DIRTY_ALL             = 0xffff
} GimpDirtyMask;


/*
 * non-registered enums; register them if needed
 */

typedef enum  /*< pdb-skip, skip >*/
{
  GIMP_CONTEXT_FIRST_PROP      =  2,

  GIMP_CONTEXT_PROP_IMAGE      =  GIMP_CONTEXT_FIRST_PROP,
  GIMP_CONTEXT_PROP_DISPLAY    =  3,
  GIMP_CONTEXT_PROP_TOOL       =  4,
  GIMP_CONTEXT_PROP_FOREGROUND =  5,
  GIMP_CONTEXT_PROP_BACKGROUND =  6,
  GIMP_CONTEXT_PROP_OPACITY    =  7,
  GIMP_CONTEXT_PROP_PAINT_MODE =  8,
  GIMP_CONTEXT_PROP_BRUSH      =  9,
  GIMP_CONTEXT_PROP_PATTERN    = 10,
  GIMP_CONTEXT_PROP_GRADIENT   = 11,
  GIMP_CONTEXT_PROP_PALETTE    = 12,
  GIMP_CONTEXT_PROP_FONT       = 13,
  GIMP_CONTEXT_PROP_BUFFER     = 14,
  GIMP_CONTEXT_PROP_IMAGEFILE  = 15,
  GIMP_CONTEXT_PROP_TEMPLATE   = 16,

  GIMP_CONTEXT_LAST_PROP       = GIMP_CONTEXT_PROP_TEMPLATE
} GimpContextPropType;

typedef enum  /*< pdb-skip, skip >*/
{
  GIMP_CONTEXT_IMAGE_MASK      = 1 <<  2,
  GIMP_CONTEXT_DISPLAY_MASK    = 1 <<  3,
  GIMP_CONTEXT_TOOL_MASK       = 1 <<  4,
  GIMP_CONTEXT_FOREGROUND_MASK = 1 <<  5,
  GIMP_CONTEXT_BACKGROUND_MASK = 1 <<  6,
  GIMP_CONTEXT_OPACITY_MASK    = 1 <<  7,
  GIMP_CONTEXT_PAINT_MODE_MASK = 1 <<  8,
  GIMP_CONTEXT_BRUSH_MASK      = 1 <<  9,
  GIMP_CONTEXT_PATTERN_MASK    = 1 << 10,
  GIMP_CONTEXT_GRADIENT_MASK   = 1 << 11,
  GIMP_CONTEXT_PALETTE_MASK    = 1 << 12,
  GIMP_CONTEXT_FONT_MASK       = 1 << 13,
  GIMP_CONTEXT_BUFFER_MASK     = 1 << 14,
  GIMP_CONTEXT_IMAGEFILE_MASK  = 1 << 15,
  GIMP_CONTEXT_TEMPLATE_MASK   = 1 << 16,

  /*  aliases  */
  GIMP_CONTEXT_PAINT_PROPS_MASK = (GIMP_CONTEXT_FOREGROUND_MASK |
                                   GIMP_CONTEXT_BACKGROUND_MASK |
                                   GIMP_CONTEXT_OPACITY_MASK    |
                                   GIMP_CONTEXT_PAINT_MODE_MASK |
                                   GIMP_CONTEXT_BRUSH_MASK      |
                                   GIMP_CONTEXT_PATTERN_MASK    |
                                   GIMP_CONTEXT_GRADIENT_MASK),
  GIMP_CONTEXT_ALL_PROPS_MASK   = (GIMP_CONTEXT_IMAGE_MASK      |
                                   GIMP_CONTEXT_DISPLAY_MASK    |
                                   GIMP_CONTEXT_TOOL_MASK       |
                                   GIMP_CONTEXT_PALETTE_MASK    |
                                   GIMP_CONTEXT_FONT_MASK       |
                                   GIMP_CONTEXT_BUFFER_MASK     |
                                   GIMP_CONTEXT_IMAGEFILE_MASK  |
                                   GIMP_CONTEXT_TEMPLATE_MASK   |
                                   GIMP_CONTEXT_PAINT_PROPS_MASK)
} GimpContextPropMask;

typedef enum  /*< skip >*/
{
  GIMP_GRADIENT_SEGMENT_LINEAR,
  GIMP_GRADIENT_SEGMENT_CURVED,
  GIMP_GRADIENT_SEGMENT_SINE,
  GIMP_GRADIENT_SEGMENT_SPHERE_INCREASING,
  GIMP_GRADIENT_SEGMENT_SPHERE_DECREASING
} GimpGradientSegmentType;

typedef enum  /*< skip >*/
{
  GIMP_GRADIENT_SEGMENT_RGB,      /* normal RGB           */
  GIMP_GRADIENT_SEGMENT_HSV_CCW,  /* counterclockwise hue */
  GIMP_GRADIENT_SEGMENT_HSV_CW    /* clockwise hue        */
} GimpGradientSegmentColor;

typedef enum  /*< skip >*/
{
  GIMP_MASK_APPLY,
  GIMP_MASK_DISCARD
} GimpMaskApplyMode;

typedef enum  /*< skip >*/
{
  GIMP_EXPAND_AS_NECESSARY,
  GIMP_CLIP_TO_IMAGE,
  GIMP_CLIP_TO_BOTTOM_LAYER,
  GIMP_FLATTEN_IMAGE
} GimpMergeType;

typedef enum  /*< skip >*/
{
  GIMP_OFFSET_BACKGROUND,
  GIMP_OFFSET_TRANSPARENT
} GimpOffsetType;

typedef enum  /*< pdb-skip, skip >*/
{
  GIMP_IMAGE_SCALE_OK,
  GIMP_IMAGE_SCALE_TOO_SMALL,
  GIMP_IMAGE_SCALE_TOO_BIG
} GimpImageScaleCheckType;


#endif /* __CORE_ENUMS_H__ */
