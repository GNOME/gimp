/* GIMP - The GNU Image Manipulation Program
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
 * these enums are registered with the type system
 */


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


#define GIMP_TYPE_ALIGNMENT (gimp_alignment_type_get_type ())

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
  GIMP_ARRANGE_BOTTOM
} GimpAlignmentType;


#define GIMP_TYPE_ALIGN_REFERENCE (gimp_align_reference_type_get_type ())

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
  GIMP_STROKE_STYLE_SOLID,  /*< desc="Solid color" >*/
  GIMP_STROKE_STYLE_PATTERN /*< desc="Pattern"     >*/
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


#define GIMP_TYPE_BRUSH_GENERATED_SHAPE (gimp_brush_generated_shape_get_type ())

GType gimp_brush_generated_shape_get_type (void) G_GNUC_CONST;

typedef enum
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


#define GIMP_TYPE_VIEW_TYPE (gimp_view_type_get_type ())

GType gimp_view_type_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_VIEW_TYPE_LIST,  /*< desc="View as list" >*/
  GIMP_VIEW_TYPE_GRID   /*< desc="View as grid" >*/
} GimpViewType;


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
  GIMP_UNDO_GROUP_IMAGE_GUIDE,        /*< desc="Guide"                       >*/
  GIMP_UNDO_GROUP_IMAGE_GRID,         /*< desc="Grid"                        >*/
  GIMP_UNDO_GROUP_IMAGE_SAMPLE_POINT, /*< desc="Sample Point"                >*/
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

  GIMP_UNDO_IMAGE_TYPE,               /*< desc="Image type"                  >*/
  GIMP_UNDO_IMAGE_SIZE,               /*< desc="Image size"                  >*/
  GIMP_UNDO_IMAGE_RESOLUTION,         /*< desc="Image resolution change"     >*/
  GIMP_UNDO_IMAGE_GUIDE,              /*< desc="Guide"                       >*/
  GIMP_UNDO_IMAGE_GRID,               /*< desc="Grid"                        >*/
  GIMP_UNDO_IMAGE_SAMPLE_POINT,       /*< desc="Sample Point"                >*/
  GIMP_UNDO_IMAGE_COLORMAP,           /*< desc="Change indexed palette"      >*/
  GIMP_UNDO_DRAWABLE,                 /*< desc="Layer/Channel"               >*/
  GIMP_UNDO_DRAWABLE_MOD,             /*< desc="Layer/Channel modification"  >*/
  GIMP_UNDO_MASK,                     /*< desc="Selection mask"              >*/
  GIMP_UNDO_ITEM_RENAME,              /*< desc="Rename item"                 >*/
  GIMP_UNDO_ITEM_DISPLACE,            /*< desc="Move item"                   >*/
  GIMP_UNDO_ITEM_VISIBILITY,          /*< desc="Item visibility"             >*/
  GIMP_UNDO_ITEM_LINKED,              /*< desc="Link/Unlink item"            >*/
  GIMP_UNDO_LAYER_ADD,                /*< desc="New layer"                   >*/
  GIMP_UNDO_LAYER_REMOVE,             /*< desc="Delete layer"                >*/
  GIMP_UNDO_LAYER_REPOSITION,         /*< desc="Reposition layer"            >*/
  GIMP_UNDO_LAYER_MODE,               /*< desc="Set layer mode"              >*/
  GIMP_UNDO_LAYER_OPACITY,            /*< desc="Set layer opacity"           >*/
  GIMP_UNDO_LAYER_LOCK_ALPHA,         /*< desc="Lock/Unlock alpha channel"   >*/
  GIMP_UNDO_TEXT_LAYER,               /*< desc="Text layer"                  >*/
  GIMP_UNDO_TEXT_LAYER_MODIFIED,      /*< desc="Text layer modification"     >*/
  GIMP_UNDO_LAYER_MASK_ADD,           /*< desc="Add layer mask"              >*/
  GIMP_UNDO_LAYER_MASK_REMOVE,        /*< desc="Delete layer mask"           >*/
  GIMP_UNDO_LAYER_MASK_APPLY,         /*< desc="Apply layer mask"            >*/
  GIMP_UNDO_LAYER_MASK_SHOW,          /*< desc="Show layer mask"             >*/
  GIMP_UNDO_CHANNEL_ADD,              /*< desc="New channel"                 >*/
  GIMP_UNDO_CHANNEL_REMOVE,           /*< desc="Delete channel"              >*/
  GIMP_UNDO_CHANNEL_REPOSITION,       /*< desc="Reposition channel"          >*/
  GIMP_UNDO_CHANNEL_COLOR,            /*< desc="Channel color"               >*/
  GIMP_UNDO_VECTORS_ADD,              /*< desc="New path"                    >*/
  GIMP_UNDO_VECTORS_REMOVE,           /*< desc="Delete path"                 >*/
  GIMP_UNDO_VECTORS_MOD,              /*< desc="Path modification"           >*/
  GIMP_UNDO_VECTORS_REPOSITION,       /*< desc="Reposition path"             >*/
  GIMP_UNDO_FS_TO_LAYER,              /*< desc="Floating selection to layer" >*/
  GIMP_UNDO_FS_RIGOR,                 /*< desc="FS rigor"                    >*/
  GIMP_UNDO_FS_RELAX,                 /*< desc="FS relax"                    >*/
  GIMP_UNDO_TRANSFORM,                /*< desc="Transform"                   >*/
  GIMP_UNDO_PAINT,                    /*< desc="Paint"                       >*/
  GIMP_UNDO_INK,                      /*< desc="Ink"                         >*/
  GIMP_UNDO_FOREGROUND_SELECT,        /*< desc="Select foreground"           >*/
  GIMP_UNDO_PARASITE_ATTACH,          /*< desc="Attach parasite"             >*/
  GIMP_UNDO_PARASITE_REMOVE,          /*< desc="Remove parasite"             >*/

  GIMP_UNDO_CANT                      /*< desc="EEK: can't undo"             >*/
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


#define GIMP_TYPE_OFFSET_TYPE (gimp_offset_type_get_type ())

GType gimp_offset_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_OFFSET_BACKGROUND,
  GIMP_OFFSET_TRANSPARENT
} GimpOffsetType;


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


#define GIMP_TYPE_GRADIENT_SEGMENT_TYPE (gimp_gradient_segment_type_get_type ())

GType gimp_gradient_segment_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_GRADIENT_SEGMENT_LINEAR,
  GIMP_GRADIENT_SEGMENT_CURVED,
  GIMP_GRADIENT_SEGMENT_SINE,
  GIMP_GRADIENT_SEGMENT_SPHERE_INCREASING,
  GIMP_GRADIENT_SEGMENT_SPHERE_DECREASING
} GimpGradientSegmentType;


#define GIMP_TYPE_GRADIENT_SEGMENT_COLOR (gimp_gradient_segment_color_get_type ())

GType gimp_gradient_segment_color_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_GRADIENT_SEGMENT_RGB,      /* normal RGB           */
  GIMP_GRADIENT_SEGMENT_HSV_CCW,  /* counterclockwise hue */
  GIMP_GRADIENT_SEGMENT_HSV_CW    /* clockwise hue        */
} GimpGradientSegmentColor;


#define GIMP_TYPE_MASK_APPLY_MODE (gimp_mask_apply_mode_get_type ())

GType gimp_mask_apply_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_MASK_APPLY,
  GIMP_MASK_DISCARD
} GimpMaskApplyMode;


#define GIMP_TYPE_MERGE_TYPE (gimp_merge_type_get_type ())

GType gimp_merge_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_EXPAND_AS_NECESSARY,
  GIMP_CLIP_TO_IMAGE,
  GIMP_CLIP_TO_BOTTOM_LAYER,
  GIMP_FLATTEN_IMAGE
} GimpMergeType;


#define GIMP_TYPE_SELECT_CRITERION (gimp_select_criterion_get_type ())

GType gimp_select_criterion_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_SELECT_CRITERION_COMPOSITE,  /*< desc="Composite"  >*/
  GIMP_SELECT_CRITERION_R,          /*< desc="Red"        >*/
  GIMP_SELECT_CRITERION_G,          /*< desc="Green"      >*/
  GIMP_SELECT_CRITERION_B,          /*< desc="Blue"       >*/
  GIMP_SELECT_CRITERION_H,          /*< desc="Hue"        >*/
  GIMP_SELECT_CRITERION_S,          /*< desc="Saturation" >*/
  GIMP_SELECT_CRITERION_V           /*< desc="Value"      >*/
} GimpSelectCriterion;


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


/*
 * non-registered enums; register them if needed
 */


typedef enum  /*< pdb-skip, skip >*/
{
  GIMP_CONTEXT_FIRST_PROP      =  2,

  GIMP_CONTEXT_PROP_IMAGE      =  GIMP_CONTEXT_FIRST_PROP,
  GIMP_CONTEXT_PROP_DISPLAY    =  3,
  GIMP_CONTEXT_PROP_TOOL       =  4,
  GIMP_CONTEXT_PROP_PAINT_INFO =  5,
  GIMP_CONTEXT_PROP_FOREGROUND =  6,
  GIMP_CONTEXT_PROP_BACKGROUND =  7,
  GIMP_CONTEXT_PROP_OPACITY    =  8,
  GIMP_CONTEXT_PROP_PAINT_MODE =  9,
  GIMP_CONTEXT_PROP_BRUSH      = 10,
  GIMP_CONTEXT_PROP_PATTERN    = 11,
  GIMP_CONTEXT_PROP_GRADIENT   = 12,
  GIMP_CONTEXT_PROP_PALETTE    = 13,
  GIMP_CONTEXT_PROP_FONT       = 14,
  GIMP_CONTEXT_PROP_BUFFER     = 15,
  GIMP_CONTEXT_PROP_IMAGEFILE  = 16,
  GIMP_CONTEXT_PROP_TEMPLATE   = 17,

  GIMP_CONTEXT_LAST_PROP       = GIMP_CONTEXT_PROP_TEMPLATE
} GimpContextPropType;


typedef enum  /*< pdb-skip, skip >*/
{
  GIMP_CONTEXT_IMAGE_MASK      = 1 <<  2,
  GIMP_CONTEXT_DISPLAY_MASK    = 1 <<  3,
  GIMP_CONTEXT_TOOL_MASK       = 1 <<  4,
  GIMP_CONTEXT_PAINT_INFO_MASK = 1 <<  5,
  GIMP_CONTEXT_FOREGROUND_MASK = 1 <<  6,
  GIMP_CONTEXT_BACKGROUND_MASK = 1 <<  7,
  GIMP_CONTEXT_OPACITY_MASK    = 1 <<  8,
  GIMP_CONTEXT_PAINT_MODE_MASK = 1 <<  9,
  GIMP_CONTEXT_BRUSH_MASK      = 1 << 10,
  GIMP_CONTEXT_PATTERN_MASK    = 1 << 11,
  GIMP_CONTEXT_GRADIENT_MASK   = 1 << 12,
  GIMP_CONTEXT_PALETTE_MASK    = 1 << 13,
  GIMP_CONTEXT_FONT_MASK       = 1 << 14,
  GIMP_CONTEXT_BUFFER_MASK     = 1 << 15,
  GIMP_CONTEXT_IMAGEFILE_MASK  = 1 << 16,
  GIMP_CONTEXT_TEMPLATE_MASK   = 1 << 17,

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
