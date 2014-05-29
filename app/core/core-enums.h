/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
 * these enums are registered with the type system
 */


#define GIMP_TYPE_COMPONENT_MASK (gimp_component_mask_get_type ())

GType gimp_component_mask_get_type (void) G_GNUC_CONST;

typedef enum /*< pdb-skip >*/
{
  GIMP_COMPONENT_RED   = 1 << 0,
  GIMP_COMPONENT_GREEN = 1 << 1,
  GIMP_COMPONENT_BLUE  = 1 << 2,
  GIMP_COMPONENT_ALPHA = 1 << 3,

  GIMP_COMPONENT_ALL = (GIMP_COMPONENT_RED | GIMP_COMPONENT_GREEN | GIMP_COMPONENT_BLUE | GIMP_COMPONENT_ALPHA)
} GimpComponentMask;


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


#define GIMP_TYPE_CONVOLUTION_TYPE (gimp_convolution_type_get_type ())

GType gimp_convolution_type_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_NORMAL_CONVOL,      /*  Negative numbers truncated  */
  GIMP_ABSOLUTE_CONVOL,    /*  Absolute value              */
  GIMP_NEGATIVE_CONVOL     /*  add 127 to values           */
} GimpConvolutionType;


#define GIMP_TYPE_CURVE_TYPE (gimp_curve_type_get_type ())

GType gimp_curve_type_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_CURVE_SMOOTH,   /*< desc="Smooth"   >*/
  GIMP_CURVE_FREE      /*< desc="Freehand" >*/
} GimpCurveType;


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


#define GIMP_TYPE_HISTOGRAM_CHANNEL (gimp_histogram_channel_get_type ())

GType gimp_histogram_channel_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_HISTOGRAM_VALUE = 0,  /*< desc="Value" >*/
  GIMP_HISTOGRAM_RED   = 1,  /*< desc="Red"   >*/
  GIMP_HISTOGRAM_GREEN = 2,  /*< desc="Green" >*/
  GIMP_HISTOGRAM_BLUE  = 3,  /*< desc="Blue"  >*/
  GIMP_HISTOGRAM_ALPHA = 4,  /*< desc="Alpha" >*/
  GIMP_HISTOGRAM_RGB   = 5   /*< desc="RGB", pdb-skip >*/
} GimpHistogramChannel;


#define GIMP_TYPE_LAYER_MODE_EFFECTS (gimp_layer_mode_effects_get_type ())

GType gimp_layer_mode_effects_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_NORMAL_MODE,          /*< desc="Normal"               >*/
  GIMP_DISSOLVE_MODE,        /*< desc="Dissolve"             >*/
  GIMP_BEHIND_MODE,          /*< desc="Behind"               >*/
  GIMP_MULTIPLY_MODE,        /*< desc="Multiply"             >*/
  GIMP_SCREEN_MODE,          /*< desc="Screen"               >*/
  GIMP_OVERLAY_MODE,         /*< desc="Overlay"              >*/
  GIMP_DIFFERENCE_MODE,      /*< desc="Difference"           >*/
  GIMP_ADDITION_MODE,        /*< desc="Addition"             >*/
  GIMP_SUBTRACT_MODE,        /*< desc="Subtract"             >*/
  GIMP_DARKEN_ONLY_MODE,     /*< desc="Darken only"          >*/
  GIMP_LIGHTEN_ONLY_MODE,    /*< desc="Lighten only"         >*/
  GIMP_HUE_MODE,             /*< desc="Hue"                  >*/
  GIMP_SATURATION_MODE,      /*< desc="Saturation"           >*/
  GIMP_COLOR_MODE,           /*< desc="Color"                >*/
  GIMP_VALUE_MODE,           /*< desc="Value"                >*/
  GIMP_DIVIDE_MODE,          /*< desc="Divide"               >*/
  GIMP_DODGE_MODE,           /*< desc="Dodge"                >*/
  GIMP_BURN_MODE,            /*< desc="Burn"                 >*/
  GIMP_HARDLIGHT_MODE,       /*< desc="Hard light"           >*/
  GIMP_SOFTLIGHT_MODE,       /*< desc="Soft light"           >*/
  GIMP_GRAIN_EXTRACT_MODE,   /*< desc="Grain extract"        >*/
  GIMP_GRAIN_MERGE_MODE,     /*< desc="Grain merge"          >*/
  GIMP_COLOR_ERASE_MODE,     /*< desc="Color erase"          >*/
  GIMP_ERASE_MODE,           /*< pdb-skip, desc="Erase"      >*/
  GIMP_REPLACE_MODE,         /*< pdb-skip, desc="Replace"    >*/
  GIMP_ANTI_ERASE_MODE       /*< pdb-skip, desc="Anti erase" >*/
} GimpLayerModeEffects;


#define GIMP_TYPE_ALIGNMENT_TYPE (gimp_alignment_type_get_type ())

GType gimp_alignment_type_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_ALIGN_LEFT,
  GIMP_ALIGN_HCENTER,
  GIMP_ALIGN_RIGHT,
  GIMP_ALIGN_TOP,
  GIMP_ALIGN_VCENTER,
  GIMP_ALIGN_BOTTOM,
  GIMP_ARRANGE_LEFT,
  GIMP_ARRANGE_HCENTER,
  GIMP_ARRANGE_RIGHT,
  GIMP_ARRANGE_TOP,
  GIMP_ARRANGE_VCENTER,
  GIMP_ARRANGE_BOTTOM,
  GIMP_ARRANGE_HFILL,
  GIMP_ARRANGE_VFILL
} GimpAlignmentType;


#define GIMP_TYPE_ALIGN_REFERENCE_TYPE (gimp_align_reference_type_get_type ())

GType gimp_align_reference_type_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_ALIGN_REFERENCE_FIRST,          /*< desc="First item"     >*/
  GIMP_ALIGN_REFERENCE_IMAGE,          /*< desc="Image"          >*/
  GIMP_ALIGN_REFERENCE_SELECTION,      /*< desc="Selection"      >*/
  GIMP_ALIGN_REFERENCE_ACTIVE_LAYER,   /*< desc="Active layer"   >*/
  GIMP_ALIGN_REFERENCE_ACTIVE_CHANNEL, /*< desc="Active channel" >*/
  GIMP_ALIGN_REFERENCE_ACTIVE_PATH     /*< desc="Active path"    >*/
} GimpAlignReferenceType;


#define GIMP_TYPE_FILL_STYLE (gimp_fill_style_get_type ())

GType gimp_fill_style_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_FILL_STYLE_SOLID,  /*< desc="Solid color" >*/
  GIMP_FILL_STYLE_PATTERN /*< desc="Pattern"     >*/
} GimpFillStyle;


#define GIMP_TYPE_STROKE_METHOD (gimp_stroke_method_get_type ())

GType gimp_stroke_method_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_STROKE_METHOD_LIBART,     /*< desc="Stroke line"              >*/
  GIMP_STROKE_METHOD_PAINT_CORE  /*< desc="Stroke with a paint tool" >*/
} GimpStrokeMethod;


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
  GIMP_DASH_CUSTOM,       /*< desc="Custom"         >*/
  GIMP_DASH_LINE,         /*< desc="Line"           >*/
  GIMP_DASH_LONG_DASH,    /*< desc="Long dashes"    >*/
  GIMP_DASH_MEDIUM_DASH,  /*< desc="Medium dashes"  >*/
  GIMP_DASH_SHORT_DASH,   /*< desc="Short dashes"   >*/
  GIMP_DASH_SPARSE_DOTS,  /*< desc="Sparse dots"    >*/
  GIMP_DASH_NORMAL_DOTS,  /*< desc="Normal dots"    >*/
  GIMP_DASH_DENSE_DOTS,   /*< desc="Dense dots"     >*/
  GIMP_DASH_STIPPLES,     /*< desc="Stipples"       >*/
  GIMP_DASH_DASH_DOT,     /*< desc="Dash, dot"      >*/
  GIMP_DASH_DASH_DOT_DOT  /*< desc="Dash, dot, dot" >*/
} GimpDashPreset;


#define GIMP_TYPE_ITEM_SET (gimp_item_set_get_type ())

GType gimp_item_set_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_ITEM_SET_NONE,        /*< desc="None"               >*/
  GIMP_ITEM_SET_ALL,         /*< desc="All layers"         >*/
  GIMP_ITEM_SET_IMAGE_SIZED, /*< desc="Image-sized layers" >*/
  GIMP_ITEM_SET_VISIBLE,     /*< desc="All visible layers" >*/
  GIMP_ITEM_SET_LINKED       /*< desc="All linked layers"  >*/
} GimpItemSet;


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


#define GIMP_TYPE_VIEW_TYPE (gimp_view_type_get_type ())

GType gimp_view_type_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_VIEW_TYPE_LIST,  /*< desc="View as list" >*/
  GIMP_VIEW_TYPE_GRID   /*< desc="View as grid" >*/
} GimpViewType;


#define GIMP_TYPE_THUMBNAIL_SIZE (gimp_thumbnail_size_get_type ())

GType gimp_thumbnail_size_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_THUMBNAIL_SIZE_NONE    = 0,    /*< desc="No thumbnails"    >*/
  GIMP_THUMBNAIL_SIZE_NORMAL  = 128,  /*< desc="Normal (128x128)" >*/
  GIMP_THUMBNAIL_SIZE_LARGE   = 256   /*< desc="Large (256x256)"  >*/
} GimpThumbnailSize;


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
  GIMP_UNDO_GROUP_IMAGE_VECTORS_MERGE,/*< desc="Merge paths"                 >*/
  GIMP_UNDO_GROUP_IMAGE_QUICK_MASK,   /*< desc="Quick Mask"                  >*/
  GIMP_UNDO_GROUP_IMAGE_GRID,         /*< desc="Grid"                        >*/
  GIMP_UNDO_GROUP_GUIDE,              /*< desc="Guide"                       >*/
  GIMP_UNDO_GROUP_SAMPLE_POINT,       /*< desc="Sample Point"                >*/
  GIMP_UNDO_GROUP_DRAWABLE,           /*< desc="Layer/Channel"               >*/
  GIMP_UNDO_GROUP_DRAWABLE_MOD,       /*< desc="Layer/Channel modification"  >*/
  GIMP_UNDO_GROUP_MASK,               /*< desc="Selection mask"              >*/
  GIMP_UNDO_GROUP_ITEM_VISIBILITY,    /*< desc="Item visibility"             >*/
  GIMP_UNDO_GROUP_ITEM_LINKED,        /*< desc="Link/Unlink item"            >*/
  GIMP_UNDO_GROUP_ITEM_PROPERTIES,    /*< desc="Item properties"             >*/
  GIMP_UNDO_GROUP_ITEM_DISPLACE,      /*< desc="Move item"                   >*/
  GIMP_UNDO_GROUP_ITEM_SCALE,         /*< desc="Scale item"                  >*/
  GIMP_UNDO_GROUP_ITEM_RESIZE,        /*< desc="Resize item"                 >*/
  GIMP_UNDO_GROUP_LAYER_ADD,          /*< desc="Add layer"                   >*/
  GIMP_UNDO_GROUP_LAYER_ADD_MASK,     /*< desc="Add layer mask"              >*/
  GIMP_UNDO_GROUP_LAYER_APPLY_MASK,   /*< desc="Apply layer mask"            >*/
  GIMP_UNDO_GROUP_FS_TO_LAYER,        /*< desc="Floating selection to layer" >*/
  GIMP_UNDO_GROUP_FS_FLOAT,           /*< desc="Float selection"             >*/
  GIMP_UNDO_GROUP_FS_ANCHOR,          /*< desc="Anchor floating selection"   >*/
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

  GIMP_UNDO_IMAGE_TYPE,               /*< desc="Image type"                  >*/
  GIMP_UNDO_IMAGE_PRECISION,          /*< desc="Image precision"             >*/
  GIMP_UNDO_IMAGE_SIZE,               /*< desc="Image size"                  >*/
  GIMP_UNDO_IMAGE_RESOLUTION,         /*< desc="Image resolution change"     >*/
  GIMP_UNDO_IMAGE_GRID,               /*< desc="Grid"                        >*/
  GIMP_UNDO_IMAGE_METADATA,           /*< desc="Change metadata"             >*/
  GIMP_UNDO_IMAGE_COLORMAP,           /*< desc="Change indexed palette"      >*/
  GIMP_UNDO_GUIDE,                    /*< desc="Guide"                       >*/
  GIMP_UNDO_SAMPLE_POINT,             /*< desc="Sample Point"                >*/
  GIMP_UNDO_DRAWABLE,                 /*< desc="Layer/Channel"               >*/
  GIMP_UNDO_DRAWABLE_MOD,             /*< desc="Layer/Channel modification"  >*/
  GIMP_UNDO_MASK,                     /*< desc="Selection mask"              >*/
  GIMP_UNDO_ITEM_REORDER,             /*< desc="Reorder item"                >*/
  GIMP_UNDO_ITEM_RENAME,              /*< desc="Rename item"                 >*/
  GIMP_UNDO_ITEM_DISPLACE,            /*< desc="Move item"                   >*/
  GIMP_UNDO_ITEM_VISIBILITY,          /*< desc="Item visibility"             >*/
  GIMP_UNDO_ITEM_LINKED,              /*< desc="Link/Unlink item"            >*/
  GIMP_UNDO_ITEM_LOCK_CONTENT,        /*< desc="Lock/Unlock content"         >*/
  GIMP_UNDO_ITEM_LOCK_POSITION,       /*< desc="Lock/Unlock position"        >*/
  GIMP_UNDO_LAYER_ADD,                /*< desc="New layer"                   >*/
  GIMP_UNDO_LAYER_REMOVE,             /*< desc="Delete layer"                >*/
  GIMP_UNDO_LAYER_MODE,               /*< desc="Set layer mode"              >*/
  GIMP_UNDO_LAYER_OPACITY,            /*< desc="Set layer opacity"           >*/
  GIMP_UNDO_LAYER_LOCK_ALPHA,         /*< desc="Lock/Unlock alpha channel"   >*/
  GIMP_UNDO_GROUP_LAYER_SUSPEND,      /*< desc="Suspend group layer resize"  >*/
  GIMP_UNDO_GROUP_LAYER_RESUME,       /*< desc="Resume group layer resize"   >*/
  GIMP_UNDO_GROUP_LAYER_CONVERT,      /*< desc="Convert group layer"         >*/
  GIMP_UNDO_TEXT_LAYER,               /*< desc="Text layer"                  >*/
  GIMP_UNDO_TEXT_LAYER_MODIFIED,      /*< desc="Text layer modification"     >*/
  GIMP_UNDO_TEXT_LAYER_CONVERT,       /*< desc="Convert text layer"          >*/
  GIMP_UNDO_LAYER_MASK_ADD,           /*< desc="Add layer mask"              >*/
  GIMP_UNDO_LAYER_MASK_REMOVE,        /*< desc="Delete layer mask"           >*/
  GIMP_UNDO_LAYER_MASK_APPLY,         /*< desc="Apply layer mask"            >*/
  GIMP_UNDO_LAYER_MASK_SHOW,          /*< desc="Show layer mask"             >*/
  GIMP_UNDO_CHANNEL_ADD,              /*< desc="New channel"                 >*/
  GIMP_UNDO_CHANNEL_REMOVE,           /*< desc="Delete channel"              >*/
  GIMP_UNDO_CHANNEL_COLOR,            /*< desc="Channel color"               >*/
  GIMP_UNDO_VECTORS_ADD,              /*< desc="New path"                    >*/
  GIMP_UNDO_VECTORS_REMOVE,           /*< desc="Delete path"                 >*/
  GIMP_UNDO_VECTORS_MOD,              /*< desc="Path modification"           >*/
  GIMP_UNDO_FS_TO_LAYER,              /*< desc="Floating selection to layer" >*/
  GIMP_UNDO_TRANSFORM,                /*< desc="Transform"                   >*/
  GIMP_UNDO_PAINT,                    /*< desc="Paint"                       >*/
  GIMP_UNDO_INK,                      /*< desc="Ink"                         >*/
  GIMP_UNDO_FOREGROUND_SELECT,        /*< desc="Select foreground"           >*/
  GIMP_UNDO_PARASITE_ATTACH,          /*< desc="Attach parasite"             >*/
  GIMP_UNDO_PARASITE_REMOVE,          /*< desc="Remove parasite"             >*/

  GIMP_UNDO_CANT                      /*< desc="Not undoable"                >*/
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
  GIMP_DIRTY_ACTIVE_DRAWABLE = 1 << 9,

  GIMP_DIRTY_ALL             = 0xffff
} GimpDirtyMask;


#define GIMP_TYPE_GRADIENT_COLOR (gimp_gradient_color_get_type ())

GType gimp_gradient_color_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_GRADIENT_COLOR_FIXED,
  GIMP_GRADIENT_COLOR_FOREGROUND,
  GIMP_GRADIENT_COLOR_FOREGROUND_TRANSPARENT,
  GIMP_GRADIENT_COLOR_BACKGROUND,
  GIMP_GRADIENT_COLOR_BACKGROUND_TRANSPARENT
} GimpGradientColor;


#define GIMP_TYPE_MESSAGE_SEVERITY (gimp_message_severity_get_type ())

GType gimp_message_severity_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_MESSAGE_INFO,     /*< desc="Message" >*/
  GIMP_MESSAGE_WARNING,  /*< desc="Warning" >*/
  GIMP_MESSAGE_ERROR     /*< desc="Error"   >*/
} GimpMessageSeverity;


#define GIMP_TYPE_COLOR_PROFILE_POLICY (gimp_color_profile_policy_get_type ())

GType gimp_color_profile_policy_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_COLOR_PROFILE_POLICY_ASK,    /*< desc="Ask what to do"           >*/
  GIMP_COLOR_PROFILE_POLICY_KEEP,   /*< desc="Keep embedded profile"    >*/
  GIMP_COLOR_PROFILE_POLICY_CONVERT /*< desc="Convert to RGB workspace" >*/
} GimpColorProfilePolicy;


#define GIMP_TYPE_DYNAMICS_OUTPUT_TYPE (gimp_dynamics_output_type_get_type ())

GType gimp_dynamics_output_type_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_DYNAMICS_OUTPUT_OPACITY,      /*< desc="Opacity"      >*/
  GIMP_DYNAMICS_OUTPUT_SIZE,         /*< desc="Size"         >*/
  GIMP_DYNAMICS_OUTPUT_ANGLE,        /*< desc="Angle"        >*/
  GIMP_DYNAMICS_OUTPUT_COLOR,        /*< desc="Color"        >*/
  GIMP_DYNAMICS_OUTPUT_HARDNESS,     /*< desc="Hardness"     >*/
  GIMP_DYNAMICS_OUTPUT_FORCE,        /*< desc="Force"        >*/
  GIMP_DYNAMICS_OUTPUT_ASPECT_RATIO, /*< desc="Aspect ratio" >*/
  GIMP_DYNAMICS_OUTPUT_SPACING,      /*< desc="Spacing"      >*/
  GIMP_DYNAMICS_OUTPUT_RATE,         /*< desc="Rate"         >*/
  GIMP_DYNAMICS_OUTPUT_FLOW,         /*< desc="Flow"         >*/
  GIMP_DYNAMICS_OUTPUT_JITTER,       /*< desc="Jitter"       >*/
} GimpDynamicsOutputType;


#define GIMP_TYPE_IMAGE_MAP_REGION (gimp_image_map_region_get_type ())

GType gimp_image_map_region_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_IMAGE_MAP_REGION_SELECTION, /*< desc="Use the selection as input"    >*/
  GIMP_IMAGE_MAP_REGION_DRAWABLE   /*< desc="Use the entire layer as input" >*/
} GimpImageMapRegion;


/*
 * non-registered enums; register them if needed
 */


typedef enum  /*< pdb-skip, skip >*/
{
  GIMP_CONTEXT_FIRST_PROP       =  2,

  GIMP_CONTEXT_PROP_IMAGE       =  GIMP_CONTEXT_FIRST_PROP,
  GIMP_CONTEXT_PROP_DISPLAY     =  3,
  GIMP_CONTEXT_PROP_TOOL        =  4,
  GIMP_CONTEXT_PROP_PAINT_INFO  =  5,
  GIMP_CONTEXT_PROP_FOREGROUND  =  6,
  GIMP_CONTEXT_PROP_BACKGROUND  =  7,
  GIMP_CONTEXT_PROP_OPACITY     =  8,
  GIMP_CONTEXT_PROP_PAINT_MODE  =  9,
  GIMP_CONTEXT_PROP_BRUSH       = 10,
  GIMP_CONTEXT_PROP_DYNAMICS    = 11,
  GIMP_CONTEXT_PROP_PATTERN     = 12,
  GIMP_CONTEXT_PROP_GRADIENT    = 13,
  GIMP_CONTEXT_PROP_PALETTE     = 14,
  GIMP_CONTEXT_PROP_TOOL_PRESET = 15,
  GIMP_CONTEXT_PROP_FONT        = 16,
  GIMP_CONTEXT_PROP_BUFFER      = 17,
  GIMP_CONTEXT_PROP_IMAGEFILE   = 18,
  GIMP_CONTEXT_PROP_TEMPLATE    = 19,

  GIMP_CONTEXT_LAST_PROP        = GIMP_CONTEXT_PROP_TEMPLATE
} GimpContextPropType;


typedef enum  /*< pdb-skip, skip >*/
{
  GIMP_CONTEXT_IMAGE_MASK       = 1 <<  2,
  GIMP_CONTEXT_DISPLAY_MASK     = 1 <<  3,
  GIMP_CONTEXT_TOOL_MASK        = 1 <<  4,
  GIMP_CONTEXT_PAINT_INFO_MASK  = 1 <<  5,
  GIMP_CONTEXT_FOREGROUND_MASK  = 1 <<  6,
  GIMP_CONTEXT_BACKGROUND_MASK  = 1 <<  7,
  GIMP_CONTEXT_OPACITY_MASK     = 1 <<  8,
  GIMP_CONTEXT_PAINT_MODE_MASK  = 1 <<  9,
  GIMP_CONTEXT_BRUSH_MASK       = 1 << 10,
  GIMP_CONTEXT_DYNAMICS_MASK    = 1 << 11,
  GIMP_CONTEXT_PATTERN_MASK     = 1 << 12,
  GIMP_CONTEXT_GRADIENT_MASK    = 1 << 13,
  GIMP_CONTEXT_PALETTE_MASK     = 1 << 14,
  GIMP_CONTEXT_TOOL_PRESET_MASK = 1 << 15,
  GIMP_CONTEXT_FONT_MASK        = 1 << 16,
  GIMP_CONTEXT_BUFFER_MASK      = 1 << 17,
  GIMP_CONTEXT_IMAGEFILE_MASK   = 1 << 18,
  GIMP_CONTEXT_TEMPLATE_MASK    = 1 << 19,

  /*  aliases  */
  GIMP_CONTEXT_PAINT_PROPS_MASK = (GIMP_CONTEXT_FOREGROUND_MASK |
                                   GIMP_CONTEXT_BACKGROUND_MASK |
                                   GIMP_CONTEXT_OPACITY_MASK    |
                                   GIMP_CONTEXT_PAINT_MODE_MASK |
                                   GIMP_CONTEXT_BRUSH_MASK      |
                                   GIMP_CONTEXT_DYNAMICS_MASK   |
                                   GIMP_CONTEXT_PATTERN_MASK    |
                                   GIMP_CONTEXT_GRADIENT_MASK),
  GIMP_CONTEXT_ALL_PROPS_MASK   = (GIMP_CONTEXT_IMAGE_MASK      |
                                   GIMP_CONTEXT_DISPLAY_MASK    |
                                   GIMP_CONTEXT_TOOL_MASK       |
                                   GIMP_CONTEXT_PAINT_INFO_MASK |
                                   GIMP_CONTEXT_PALETTE_MASK    |
                                   GIMP_CONTEXT_FONT_MASK       |
                                   GIMP_CONTEXT_BUFFER_MASK     |
                                   GIMP_CONTEXT_IMAGEFILE_MASK  |
                                   GIMP_CONTEXT_TEMPLATE_MASK   |
                                   GIMP_CONTEXT_PAINT_PROPS_MASK)
} GimpContextPropMask;


typedef enum  /*< pdb-skip, skip >*/
{
  GIMP_IMAGE_SCALE_OK,
  GIMP_IMAGE_SCALE_TOO_SMALL,
  GIMP_IMAGE_SCALE_TOO_BIG
} GimpImageScaleCheckType;


typedef enum  /*< pdb-skip, skip >*/
{
  GIMP_ITEM_TYPE_LAYERS   = 1 << 0,
  GIMP_ITEM_TYPE_CHANNELS = 1 << 1,
  GIMP_ITEM_TYPE_VECTORS  = 1 << 2,

  GIMP_ITEM_TYPE_ALL      = (GIMP_ITEM_TYPE_LAYERS   |
                             GIMP_ITEM_TYPE_CHANNELS |
                             GIMP_ITEM_TYPE_VECTORS)
} GimpItemTypeMask;


#endif /* __CORE_ENUMS_H__ */
