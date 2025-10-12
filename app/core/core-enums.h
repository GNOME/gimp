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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#if 0
   This file is parsed by two scripts, enumgen.pl in pdb,
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


#define GIMP_TYPE_ALIGN_REFERENCE_TYPE (gimp_align_reference_type_get_type ())

GType gimp_align_reference_type_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_ALIGN_REFERENCE_IMAGE,          /*< desc="Image"                   >*/
  GIMP_ALIGN_REFERENCE_SELECTION,      /*< desc="Selection"               >*/
  GIMP_ALIGN_REFERENCE_PICK,           /*< desc="Picked reference object" >*/
} GimpAlignReferenceType;


#define GIMP_TYPE_ALIGNMENT_TYPE (gimp_alignment_type_get_type ())

GType gimp_alignment_type_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_ALIGN_LEFT,                     /*< desc="Align to the left"                                 >*/
  GIMP_ALIGN_HCENTER,                  /*< desc="Center horizontally"                               >*/
  GIMP_ALIGN_RIGHT,                    /*< desc="Align to the right"                                >*/
  GIMP_ALIGN_TOP,                      /*< desc="Align to the top"                                  >*/
  GIMP_ALIGN_VCENTER,                  /*< desc="Center vertically"                                 >*/
  GIMP_ALIGN_BOTTOM,                   /*< desc="Align to the bottom"                               >*/
  GIMP_ARRANGE_HFILL,                  /*< desc="Distribute anchor points horizontally evenly"      >*/
  GIMP_ARRANGE_VFILL,                  /*< desc="Distribute anchor points vertically evenly"        >*/
  GIMP_DISTRIBUTE_EVEN_HORIZONTAL_GAP, /*< desc="Distribute horizontally with even horizontal gaps" >*/
  GIMP_DISTRIBUTE_EVEN_VERTICAL_GAP,   /*< desc="Distribute vertically with even vertical gaps"     >*/
} GimpAlignmentType;


/**
 * GimpBucketFillMode:
 * @GIMP_BUCKET_FILL_FG:      FG color fill
 * @GIMP_BUCKET_FILL_BG:      BG color fill
 * @GIMP_BUCKET_FILL_PATTERN: Pattern fill
 *
 * Bucket fill modes.
 */
#define GIMP_TYPE_BUCKET_FILL_MODE (gimp_bucket_fill_mode_get_type ())

GType gimp_bucket_fill_mode_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_BUCKET_FILL_FG,      /*< desc="FG color fill" >*/
  GIMP_BUCKET_FILL_BG,      /*< desc="BG color fill" >*/
  GIMP_BUCKET_FILL_PATTERN  /*< desc="Pattern fill"  >*/
} GimpBucketFillMode;


#define GIMP_TYPE_CHANNEL_BORDER_STYLE (gimp_channel_border_style_get_type ())

GType gimp_channel_border_style_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_CHANNEL_BORDER_STYLE_HARD,     /*< desc="Hard"      >*/
  GIMP_CHANNEL_BORDER_STYLE_SMOOTH,   /*< desc="Smooth"    >*/
  GIMP_CHANNEL_BORDER_STYLE_FEATHERED /*< desc="Feathered" >*/
} GimpChannelBorderStyle;


/*  Note: when appending values here, don't forget to update
 *  GimpColorFrame and other places use this enum to create combo
 *  boxes.
 */
#define GIMP_TYPE_COLOR_PICK_MODE (gimp_color_pick_mode_get_type ())

GType gimp_color_pick_mode_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_COLOR_PICK_MODE_PIXEL,       /*< desc="Pixel"        >*/
  GIMP_COLOR_PICK_MODE_RGB_PERCENT, /*< desc="RGB (%)"      >*/
  GIMP_COLOR_PICK_MODE_RGB_U8,      /*< desc="RGB (0..255)" >*/
  GIMP_COLOR_PICK_MODE_GRAYSCALE,   /*< desc="Grayscale (%)">*/
  GIMP_COLOR_PICK_MODE_HSV,         /*< desc="HSV"          >*/
  GIMP_COLOR_PICK_MODE_LCH,         /*< desc="CIE LCh"      >*/
  GIMP_COLOR_PICK_MODE_LAB,         /*< desc="CIE LAB"      >*/
  GIMP_COLOR_PICK_MODE_CMYK,        /*< desc="CMYK"         >*/
  GIMP_COLOR_PICK_MODE_XYY,         /*< desc="CIE xyY"      >*/
  GIMP_COLOR_PICK_MODE_YUV,         /*< desc="CIE Yu'v'"      >*/

  GIMP_COLOR_PICK_MODE_LAST = GIMP_COLOR_PICK_MODE_YUV /*< skip >*/
} GimpColorPickMode;


#define GIMP_TYPE_COLOR_PROFILE_POLICY (gimp_color_profile_policy_get_type ())

GType gimp_color_profile_policy_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_COLOR_PROFILE_POLICY_ASK,               /*< desc="Ask what to do"                                                          >*/
  GIMP_COLOR_PROFILE_POLICY_KEEP,              /*< desc="Keep embedded profile"                                                   >*/
  GIMP_COLOR_PROFILE_POLICY_CONVERT_BUILTIN,   /*< desc="Convert to built-in sRGB or grayscale profile"                           >*/
  GIMP_COLOR_PROFILE_POLICY_CONVERT_PREFERRED, /*< desc="Convert to preferred RGB or grayscale profile (defaulting to built-in)" >*/
} GimpColorProfilePolicy;


#define GIMP_TYPE_COMPONENT_MASK (gimp_component_mask_get_type ())

GType gimp_component_mask_get_type (void) G_GNUC_CONST;

typedef enum /*< pdb-skip >*/
{
  GIMP_COMPONENT_MASK_RED   = 1 << 0,
  GIMP_COMPONENT_MASK_GREEN = 1 << 1,
  GIMP_COMPONENT_MASK_BLUE  = 1 << 2,
  GIMP_COMPONENT_MASK_ALPHA = 1 << 3,

  GIMP_COMPONENT_MASK_ALL = (GIMP_COMPONENT_MASK_RED   |
                             GIMP_COMPONENT_MASK_GREEN |
                             GIMP_COMPONENT_MASK_BLUE  |
                             GIMP_COMPONENT_MASK_ALPHA)
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
  GIMP_CONVERT_DITHER_NONE,        /*< desc="None"                                     >*/
  GIMP_CONVERT_DITHER_FS,          /*< desc="Floyd-Steinberg (normal)"                 >*/
  GIMP_CONVERT_DITHER_FS_LOWBLEED, /*< desc="Floyd-Steinberg (reduced color bleeding)" >*/
  GIMP_CONVERT_DITHER_FIXED,       /*< desc="Positioned"                               >*/
  GIMP_CONVERT_DITHER_NODESTRUCT   /*< pdb-skip, skip >*/
} GimpConvertDitherType;


#define GIMP_TYPE_CONVOLUTION_TYPE (gimp_convolution_type_get_type ())

GType gimp_convolution_type_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_NORMAL_CONVOL,      /*  Negative numbers truncated  */
  GIMP_ABSOLUTE_CONVOL,    /*  Absolute value              */
  GIMP_NEGATIVE_CONVOL     /*  add 127 to values           */
} GimpConvolutionType;


#define GIMP_TYPE_CURVE_POINT_TYPE (gimp_curve_point_type_get_type ())

GType gimp_curve_point_type_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_CURVE_POINT_SMOOTH,   /*< desc="Smooth" >*/
  GIMP_CURVE_POINT_CORNER    /*< desc="Corner" >*/
} GimpCurvePointType;


#define GIMP_TYPE_CURVE_TYPE (gimp_curve_type_get_type ())

GType gimp_curve_type_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_CURVE_SMOOTH,   /*< desc="Smooth"   >*/
  GIMP_CURVE_FREE      /*< desc="Freehand" >*/
} GimpCurveType;


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


#define GIMP_TYPE_DEBUG_POLICY (gimp_debug_policy_get_type ())

GType gimp_debug_policy_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_DEBUG_POLICY_WARNING,    /*< desc="Debug warnings, critical errors and crashes" >*/
  GIMP_DEBUG_POLICY_CRITICAL,   /*< desc="Debug critical errors and crashes"           >*/
  GIMP_DEBUG_POLICY_FATAL,      /*< desc="Debug crashes only"                          >*/
  GIMP_DEBUG_POLICY_NEVER       /*< desc="Never debug GIMP"                            >*/
} GimpDebugPolicy;


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
  GIMP_DIRTY_PATH            = 1 << 7,
  GIMP_DIRTY_SELECTION       = 1 << 8,
  GIMP_DIRTY_ACTIVE_DRAWABLE = 1 << 9,

  GIMP_DIRTY_ALL             = 0xffff
} GimpDirtyMask;


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


#define GIMP_TYPE_CUSTOM_STYLE (gimp_custom_style_get_type ())

GType gimp_custom_style_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_CUSTOM_STYLE_SOLID_COLOR,  /*< desc="Solid color" >*/
  GIMP_CUSTOM_STYLE_PATTERN       /*< desc="Pattern"          >*/
} GimpCustomStyle;

#define GIMP_TYPE_FILL_STYLE (gimp_fill_style_get_type ())

GType gimp_fill_style_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_FILL_STYLE_FG_COLOR,  /*< desc="Foreground color" >*/
  GIMP_FILL_STYLE_BG_COLOR,  /*< desc="Background color" >*/
  GIMP_FILL_STYLE_PATTERN    /*< desc="Pattern"          >*/
} GimpFillStyle;


#define GIMP_TYPE_FILTER_REGION (gimp_filter_region_get_type ())

GType gimp_filter_region_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_FILTER_REGION_SELECTION, /*< desc="Use the selection as input"    >*/
  GIMP_FILTER_REGION_DRAWABLE   /*< desc="Use the entire layer as input" >*/
} GimpFilterRegion;


#define GIMP_TYPE_GRADIENT_COLOR (gimp_gradient_color_get_type ())

GType gimp_gradient_color_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_GRADIENT_COLOR_FIXED,                  /*< desc="Fixed"                                           >*/
  GIMP_GRADIENT_COLOR_FOREGROUND,             /*< desc="Foreground color",               abbrev="FG"     >*/
  GIMP_GRADIENT_COLOR_FOREGROUND_TRANSPARENT, /*< desc="Foreground color (transparent)",  abbrev="FG (t)" >*/
  GIMP_GRADIENT_COLOR_BACKGROUND,             /*< desc="Background color",               abbrev="BG"     >*/
  GIMP_GRADIENT_COLOR_BACKGROUND_TRANSPARENT  /*< desc="Background color (transparent)", abbrev="BG (t)" >*/
} GimpGradientColor;


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


#define GIMP_TYPE_GUIDE_STYLE (gimp_guide_style_get_type ())

GType gimp_guide_style_get_type (void) G_GNUC_CONST;

typedef enum /*< pdb-skip >*/
{
  GIMP_GUIDE_STYLE_NONE,
  GIMP_GUIDE_STYLE_NORMAL,
  GIMP_GUIDE_STYLE_MIRROR,
  GIMP_GUIDE_STYLE_MANDALA,
  GIMP_GUIDE_STYLE_SPLIT_VIEW
} GimpGuideStyle;


#define GIMP_TYPE_HISTOGRAM_CHANNEL (gimp_histogram_channel_get_type ())

GType gimp_histogram_channel_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_HISTOGRAM_VALUE     = 0,  /*< desc="Value"         >*/
  GIMP_HISTOGRAM_RED       = 1,  /*< desc="Red"           >*/
  GIMP_HISTOGRAM_GREEN     = 2,  /*< desc="Green"         >*/
  GIMP_HISTOGRAM_BLUE      = 3,  /*< desc="Blue"          >*/
  GIMP_HISTOGRAM_ALPHA     = 4,  /*< desc="Alpha"         >*/
  GIMP_HISTOGRAM_LUMINANCE = 5,  /*< desc="Luminance"     >*/
  GIMP_HISTOGRAM_RGB       = 6   /*< desc="RGB", pdb-skip >*/
} GimpHistogramChannel;


#define GIMP_TYPE_ITEM_SET (gimp_item_set_get_type ())

GType gimp_item_set_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_ITEM_SET_NONE,        /*< desc="None"               >*/
  GIMP_ITEM_SET_ALL,         /*< desc="All layers"         >*/
  GIMP_ITEM_SET_IMAGE_SIZED, /*< desc="Image-sized layers" >*/
  GIMP_ITEM_SET_VISIBLE,     /*< desc="All visible layers" >*/
} GimpItemSet;


#define GIMP_TYPE_MATTING_ENGINE (gimp_matting_engine_get_type ())

GType gimp_matting_engine_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_MATTING_ENGINE_GLOBAL,  /*< desc="Matting Global" >*/
  GIMP_MATTING_ENGINE_LEVIN,   /*< desc="Matting Levin" >*/
} GimpMattingEngine;


#define GIMP_TYPE_MESSAGE_SEVERITY (gimp_message_severity_get_type ())

GType gimp_message_severity_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_MESSAGE_INFO,        /*< desc="Message"  >*/
  GIMP_MESSAGE_WARNING,     /*< desc="Warning"  >*/
  GIMP_MESSAGE_ERROR,       /*< desc="Error"    >*/
  GIMP_MESSAGE_BUG_WARNING, /*< desc="WARNING"  >*/
  GIMP_MESSAGE_BUG_CRITICAL /*< desc="CRITICAL" >*/
} GimpMessageSeverity;


#define GIMP_TYPE_METADATA_ROTATION_POLICY (gimp_metadata_rotation_policy_get_type ())

GType gimp_metadata_rotation_policy_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_METADATA_ROTATION_POLICY_ASK,    /*< desc="Ask what to do"                         >*/
  GIMP_METADATA_ROTATION_POLICY_KEEP,   /*< desc="Discard metadata without rotating"      >*/
  GIMP_METADATA_ROTATION_POLICY_ROTATE  /*< desc="Rotate the image then discard metadata" >*/
} GimpMetadataRotationPolicy;


#define GIMP_TYPE_PASTE_TYPE (gimp_paste_type_get_type ())

GType gimp_paste_type_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_PASTE_TYPE_FLOATING,
  GIMP_PASTE_TYPE_FLOATING_IN_PLACE,
  GIMP_PASTE_TYPE_FLOATING_INTO,
  GIMP_PASTE_TYPE_FLOATING_INTO_IN_PLACE,
  GIMP_PASTE_TYPE_NEW_LAYER,
  GIMP_PASTE_TYPE_NEW_LAYER_IN_PLACE,
  GIMP_PASTE_TYPE_NEW_LAYER_OR_FLOATING,
  GIMP_PASTE_TYPE_NEW_LAYER_OR_FLOATING_IN_PLACE,
  GIMP_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING,
  GIMP_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING_IN_PLACE,
} GimpPasteType;


#define GIMP_TYPE_WIN32_POINTER_INPUT_API (gimp_win32_pointer_input_api_get_type ())

GType gimp_win32_pointer_input_api_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_WIN32_POINTER_INPUT_API_WINTAB,      /*< desc="Wintab"      >*/
  GIMP_WIN32_POINTER_INPUT_API_WINDOWS_INK  /*< desc="Windows Ink" >*/
} GimpWin32PointerInputAPI;


#define GIMP_TYPE_THUMBNAIL_SIZE (gimp_thumbnail_size_get_type ())

GType gimp_thumbnail_size_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_THUMBNAIL_SIZE_NONE    = 0,    /*< desc="No thumbnails"    >*/
  GIMP_THUMBNAIL_SIZE_NORMAL  = 128,  /*< desc="Normal (128x128)" >*/
  GIMP_THUMBNAIL_SIZE_LARGE   = 256   /*< desc="Large (256x256)"  >*/
} GimpThumbnailSize;


#define GIMP_TYPE_TRC_TYPE (gimp_trc_type_get_type ())

GType gimp_trc_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_TRC_LINEAR,     /*< desc="Linear"     >*/
  GIMP_TRC_NON_LINEAR, /*< desc="Non-Linear" >*/
  GIMP_TRC_PERCEPTUAL  /*< desc="Perceptual" >*/
} GimpTRCType;


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


#define GIMP_TYPE_UNDO_MODE (gimp_undo_mode_get_type ())

GType gimp_undo_mode_get_type (void) G_GNUC_CONST;

typedef enum /*< pdb-skip >*/
{
  GIMP_UNDO_MODE_UNDO,
  GIMP_UNDO_MODE_REDO
} GimpUndoMode;


#define GIMP_TYPE_UNDO_TYPE (gimp_undo_type_get_type ())

GType gimp_undo_type_get_type (void) G_GNUC_CONST;

typedef enum /*< pdb-skip >*/
{
  /* Type NO_UNDO_GROUP (0) is special - in the gimpimage structure it
   * means there is no undo group currently being added to.
   */
  GIMP_UNDO_GROUP_NONE = 0,              /*< desc="<<invalid>>"                    >*/

  GIMP_UNDO_GROUP_FIRST = GIMP_UNDO_GROUP_NONE, /*< skip >*/

  GIMP_UNDO_GROUP_IMAGE_SCALE,           /*< desc="Scale image"                    >*/
  GIMP_UNDO_GROUP_IMAGE_RESIZE,          /*< desc="Resize image"                   >*/
  GIMP_UNDO_GROUP_IMAGE_FLIP,            /*< desc="Flip image"                     >*/
  GIMP_UNDO_GROUP_IMAGE_ROTATE,          /*< desc="Rotate image"                   >*/
  GIMP_UNDO_GROUP_IMAGE_TRANSFORM,       /*< desc="Transform image"                >*/
  GIMP_UNDO_GROUP_IMAGE_CROP,            /*< desc="Crop image"                     >*/
  GIMP_UNDO_GROUP_IMAGE_CONVERT,         /*< desc="Convert image"                  >*/
  GIMP_UNDO_GROUP_IMAGE_ITEM_REMOVE,     /*< desc="Remove item"                    >*/
  GIMP_UNDO_GROUP_IMAGE_ITEM_REORDER,    /*< desc="Reorder item"                   >*/
  GIMP_UNDO_GROUP_IMAGE_LAYERS_MERGE,    /*< desc="Merge layers"                   >*/
  GIMP_UNDO_GROUP_IMAGE_PATHS_MERGE,     /*< desc="Merge paths"                    >*/
  GIMP_UNDO_GROUP_IMAGE_QUICK_MASK,      /*< desc="Quick Mask"                     >*/
  GIMP_UNDO_GROUP_IMAGE_GRID,            /*< desc="Grid"                           >*/
  GIMP_UNDO_GROUP_IMAGE_COLORMAP_REMAP,  /*< desc="Colormap remapping"             >*/
  GIMP_UNDO_GROUP_GUIDE,                 /*< desc="Guide"                          >*/
  GIMP_UNDO_GROUP_SAMPLE_POINT,          /*< desc="Sample Point"                   >*/
  GIMP_UNDO_GROUP_DRAWABLE,              /*< desc="Layer/Channel"                  >*/
  GIMP_UNDO_GROUP_DRAWABLE_MOD,          /*< desc="Layer/Channel modification"     >*/
  GIMP_UNDO_GROUP_MASK,                  /*< desc="Selection mask"                 >*/
  GIMP_UNDO_GROUP_ITEM_VISIBILITY,       /*< desc="Item visibility"                >*/
  GIMP_UNDO_GROUP_ITEM_LOCK_CONTENTS,    /*< desc="Lock/Unlock contents"           >*/
  GIMP_UNDO_GROUP_ITEM_LOCK_POSITION,    /*< desc="Lock/Unlock position"           >*/
  GIMP_UNDO_GROUP_ITEM_LOCK_VISIBILITY,  /*< desc="Lock/Unlock visibility"         >*/
  GIMP_UNDO_GROUP_ITEM_PROPERTIES,       /*< desc="Item properties"                >*/
  GIMP_UNDO_GROUP_ITEM_DISPLACE,         /*< desc="Move item"                      >*/
  GIMP_UNDO_GROUP_ITEM_SCALE,            /*< desc="Scale item"                     >*/
  GIMP_UNDO_GROUP_ITEM_RESIZE,           /*< desc="Resize item"                    >*/
  GIMP_UNDO_GROUP_LAYER_ADD,             /*< desc="Add layer"                      >*/
  GIMP_UNDO_GROUP_LAYER_ADD_ALPHA,       /*< desc="Add alpha channel"              >*/
  GIMP_UNDO_GROUP_LAYER_ADD_MASK,        /*< desc="Add layer masks"                >*/
  GIMP_UNDO_GROUP_LAYER_APPLY_MASK,      /*< desc="Apply layer masks"              >*/
  GIMP_UNDO_GROUP_LAYER_REMOVE_ALPHA,    /*< desc="Remove alpha channel"           >*/
  GIMP_UNDO_GROUP_LAYER_LOCK_ALPHA,      /*< desc="Lock/Unlock alpha channels"     >*/
  GIMP_UNDO_GROUP_LAYER_OPACITY,         /*< desc="Set layers opacity"             >*/
  GIMP_UNDO_GROUP_LAYER_MODE,            /*< desc="Set layers mode"                >*/
  GIMP_UNDO_GROUP_CHANNEL_ADD,           /*< desc="Add channels"                   >*/
  GIMP_UNDO_GROUP_FS_TO_LAYER,           /*< desc="Floating selection to layer"    >*/
  GIMP_UNDO_GROUP_FS_FLOAT,              /*< desc="Float selection"                >*/
  GIMP_UNDO_GROUP_FS_ANCHOR,             /*< desc="Anchor floating selection"      >*/
  GIMP_UNDO_GROUP_EDIT_PASTE,            /*< desc="Paste"                          >*/
  GIMP_UNDO_GROUP_EDIT_CUT,              /*< desc="Cut"                            >*/
  GIMP_UNDO_GROUP_TEXT,                  /*< desc="Text"                           >*/
  GIMP_UNDO_GROUP_TRANSFORM,             /*< desc="Transform"                      >*/
  GIMP_UNDO_GROUP_PAINT,                 /*< desc="Paint"                          >*/
  GIMP_UNDO_GROUP_PARASITE_ATTACH,       /*< desc="Attach parasite"                >*/
  GIMP_UNDO_GROUP_PARASITE_REMOVE,       /*< desc="Remove parasite"                >*/
  GIMP_UNDO_GROUP_PATHS_IMPORT,          /*< desc="Import paths"                   >*/
  GIMP_UNDO_GROUP_MISC,                  /*< desc="Plug-In"                        >*/

  GIMP_UNDO_GROUP_LAST = GIMP_UNDO_GROUP_MISC, /*< skip >*/

  /*  Undo types which actually do something  */

  GIMP_UNDO_IMAGE_TYPE,                  /*< desc="Image type"                     >*/
  GIMP_UNDO_IMAGE_PRECISION,             /*< desc="Image precision"                >*/
  GIMP_UNDO_IMAGE_SIZE,                  /*< desc="Image size"                     >*/
  GIMP_UNDO_IMAGE_RESOLUTION,            /*< desc="Image resolution change"        >*/
  GIMP_UNDO_IMAGE_GRID,                  /*< desc="Grid"                           >*/
  GIMP_UNDO_IMAGE_METADATA,              /*< desc="Change metadata"                >*/
  GIMP_UNDO_IMAGE_COLORMAP,              /*< desc="Change indexed palette"         >*/
  GIMP_UNDO_IMAGE_HIDDEN_PROFILE,        /*< desc="Hide/Unhide color profile"      >*/
  GIMP_UNDO_GUIDE,                       /*< desc="Guide"                          >*/
  GIMP_UNDO_SAMPLE_POINT,                /*< desc="Sample Point"                   >*/
  GIMP_UNDO_DRAWABLE,                    /*< desc="Layer/Channel"                  >*/
  GIMP_UNDO_DRAWABLE_MOD,                /*< desc="Layer/Channel modification"     >*/
  GIMP_UNDO_DRAWABLE_FORMAT,             /*< desc="Layer/Channel format"           >*/
  GIMP_UNDO_MASK,                        /*< desc="Selection mask"                 >*/
  GIMP_UNDO_ITEM_REORDER,                /*< desc="Reorder item"                   >*/
  GIMP_UNDO_ITEM_RENAME,                 /*< desc="Rename item"                    >*/
  GIMP_UNDO_ITEM_DISPLACE,               /*< desc="Move item"                      >*/
  GIMP_UNDO_ITEM_VISIBILITY,             /*< desc="Item visibility"                >*/
  GIMP_UNDO_ITEM_COLOR_TAG,              /*< desc="Item color tag"                 >*/
  GIMP_UNDO_ITEM_LOCK_CONTENT,           /*< desc="Lock/Unlock content"            >*/
  GIMP_UNDO_ITEM_LOCK_POSITION,          /*< desc="Lock/Unlock position"           >*/
  GIMP_UNDO_ITEM_LOCK_VISIBILITY,        /*< desc="Lock/Unlock visibility"         >*/
  GIMP_UNDO_LAYER_ADD,                   /*< desc="New layer"                      >*/
  GIMP_UNDO_LAYER_REMOVE,                /*< desc="Delete layer"                   >*/
  GIMP_UNDO_LAYER_MODE,                  /*< desc="Set layer mode"                 >*/
  GIMP_UNDO_LAYER_OPACITY,               /*< desc="Set layer opacity"              >*/
  GIMP_UNDO_LAYER_LOCK_ALPHA,            /*< desc="Lock/Unlock alpha channel"      >*/
  GIMP_UNDO_LINK_LAYER,                  /*< desc="Link layer"                     >*/
  GIMP_UNDO_GROUP_LAYER_SUSPEND_RESIZE,  /*< desc="Suspend group layer resize"     >*/
  GIMP_UNDO_GROUP_LAYER_RESUME_RESIZE,   /*< desc="Resume group layer resize"      >*/
  GIMP_UNDO_GROUP_LAYER_SUSPEND_MASK,    /*< desc="Suspend group layer mask"       >*/
  GIMP_UNDO_GROUP_LAYER_RESUME_MASK,     /*< desc="Resume group layer mask"        >*/
  GIMP_UNDO_GROUP_LAYER_START_TRANSFORM, /*< desc="Start transforming group layer" >*/
  GIMP_UNDO_GROUP_LAYER_END_TRANSFORM,   /*< desc="End transforming group layer"   >*/
  GIMP_UNDO_GROUP_LAYER_CONVERT,         /*< desc="Convert group layer"            >*/
  GIMP_UNDO_TEXT_LAYER,                  /*< desc="Text layer"                     >*/
  GIMP_UNDO_TEXT_LAYER_CONVERT,          /*< desc="Convert text layer"             >*/
  GIMP_UNDO_VECTOR_LAYER,                /*< desc="Vector layer"                   >*/
  GIMP_UNDO_LAYER_MASK_ADD,              /*< desc="Add layer masks"                >*/
  GIMP_UNDO_LAYER_MASK_REMOVE,           /*< desc="Delete layer masks"             >*/
  GIMP_UNDO_LAYER_MASK_APPLY,            /*< desc="Apply layer masks"              >*/
  GIMP_UNDO_LAYER_MASK_SHOW,             /*< desc="Show layer masks"               >*/
  GIMP_UNDO_CHANNEL_ADD,                 /*< desc="New channel"                    >*/
  GIMP_UNDO_CHANNEL_REMOVE,              /*< desc="Delete channel"                 >*/
  GIMP_UNDO_CHANNEL_COLOR,               /*< desc="Channel color"                  >*/
  GIMP_UNDO_PATH_ADD,                    /*< desc="New path"                       >*/
  GIMP_UNDO_PATH_REMOVE,                 /*< desc="Delete path"                    >*/
  GIMP_UNDO_PATH_MOD,                    /*< desc="Path modification"              >*/
  GIMP_UNDO_FS_TO_LAYER,                 /*< desc="Floating selection to layer"    >*/
  GIMP_UNDO_TRANSFORM_GRID,              /*< desc="Transform grid"                 >*/
  GIMP_UNDO_PAINT,                       /*< desc="Paint"                          >*/
  GIMP_UNDO_INK,                         /*< desc="Ink"                            >*/
  GIMP_UNDO_FOREGROUND_SELECT,           /*< desc="Select foreground"              >*/
  GIMP_UNDO_PARASITE_ATTACH,             /*< desc="Attach parasite"                >*/
  GIMP_UNDO_PARASITE_REMOVE,             /*< desc="Remove parasite"                >*/
  GIMP_UNDO_FILTER_ADD,                  /*< desc="Add effect"                     >*/
  GIMP_UNDO_FILTER_REMOVE,               /*< desc="Remove effect"                  >*/
  GIMP_UNDO_FILTER_REORDER,              /*< desc="Reorder effect"                 >*/
  GIMP_UNDO_FILTER_MODIFIED,             /*< desc="Effect modification"            >*/
  GIMP_UNDO_RASTERIZABLE,                /*< desc="Text, link or vector layer"     >*/

  GIMP_UNDO_CANT                         /*< desc="Not undoable"                   >*/
} GimpUndoType;


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


#define GIMP_TYPE_SELECT_METHOD (gimp_select_method_get_type ())

GType gimp_select_method_get_type (void) G_GNUC_CONST;

typedef enum /*< pdb-skip >*/
{
  GIMP_SELECT_PLAIN_TEXT,    /*< desc="Selection by basic text search"         >*/
  GIMP_SELECT_REGEX_PATTERN, /*< desc="Selection by regular expression search" >*/
  GIMP_SELECT_GLOB_PATTERN,  /*< desc="Selection by glob pattern search"       >*/
} GimpSelectMethod;

/*
 * non-registered enums; register them if needed
 */


typedef enum  /*< pdb-skip, skip >*/
{
  GIMP_CONTEXT_PROP_FIRST       =  2,

  GIMP_CONTEXT_PROP_IMAGE       =  GIMP_CONTEXT_PROP_FIRST,
  GIMP_CONTEXT_PROP_DISPLAY     =  3,
  GIMP_CONTEXT_PROP_TOOL        =  4,
  GIMP_CONTEXT_PROP_PAINT_INFO  =  5,
  GIMP_CONTEXT_PROP_FOREGROUND  =  6,
  GIMP_CONTEXT_PROP_BACKGROUND  =  7,
  GIMP_CONTEXT_PROP_OPACITY     =  8,
  GIMP_CONTEXT_PROP_PAINT_MODE  =  9,
  GIMP_CONTEXT_PROP_BRUSH       = 10,
  GIMP_CONTEXT_PROP_DYNAMICS    = 11,
  GIMP_CONTEXT_PROP_MYBRUSH     = 12,
  GIMP_CONTEXT_PROP_PATTERN     = 13,
  GIMP_CONTEXT_PROP_GRADIENT    = 14,
  GIMP_CONTEXT_PROP_PALETTE     = 15,
  GIMP_CONTEXT_PROP_FONT        = 16,
  GIMP_CONTEXT_PROP_TOOL_PRESET = 17,
  GIMP_CONTEXT_PROP_BUFFER      = 18,
  GIMP_CONTEXT_PROP_IMAGEFILE   = 19,
  GIMP_CONTEXT_PROP_TEMPLATE    = 20,
  GIMP_CONTEXT_PROP_EXPAND      = 21,

  GIMP_CONTEXT_PROP_LAST        = GIMP_CONTEXT_PROP_TEMPLATE
} GimpContextPropType;


typedef enum  /*< pdb-skip, skip >*/
{
  GIMP_CONTEXT_PROP_MASK_IMAGE       = 1 <<  2,
  GIMP_CONTEXT_PROP_MASK_DISPLAY     = 1 <<  3,
  GIMP_CONTEXT_PROP_MASK_TOOL        = 1 <<  4,
  GIMP_CONTEXT_PROP_MASK_PAINT_INFO  = 1 <<  5,
  GIMP_CONTEXT_PROP_MASK_FOREGROUND  = 1 <<  6,
  GIMP_CONTEXT_PROP_MASK_BACKGROUND  = 1 <<  7,
  GIMP_CONTEXT_PROP_MASK_OPACITY     = 1 <<  8,
  GIMP_CONTEXT_PROP_MASK_PAINT_MODE  = 1 <<  9,
  GIMP_CONTEXT_PROP_MASK_BRUSH       = 1 << 10,
  GIMP_CONTEXT_PROP_MASK_DYNAMICS    = 1 << 11,
  GIMP_CONTEXT_PROP_MASK_MYBRUSH     = 1 << 12,
  GIMP_CONTEXT_PROP_MASK_PATTERN     = 1 << 13,
  GIMP_CONTEXT_PROP_MASK_GRADIENT    = 1 << 14,
  GIMP_CONTEXT_PROP_MASK_PALETTE     = 1 << 15,
  GIMP_CONTEXT_PROP_MASK_FONT        = 1 << 16,
  GIMP_CONTEXT_PROP_MASK_TOOL_PRESET = 1 << 17,
  GIMP_CONTEXT_PROP_MASK_BUFFER      = 1 << 18,
  GIMP_CONTEXT_PROP_MASK_IMAGEFILE   = 1 << 19,
  GIMP_CONTEXT_PROP_MASK_TEMPLATE    = 1 << 20,
  GIMP_CONTEXT_PROP_MASK_EXPAND      = 1 << 21,

  /*  aliases  */
  GIMP_CONTEXT_PROP_MASK_PAINT = (GIMP_CONTEXT_PROP_MASK_FOREGROUND |
                                  GIMP_CONTEXT_PROP_MASK_BACKGROUND |
                                  GIMP_CONTEXT_PROP_MASK_OPACITY    |
                                  GIMP_CONTEXT_PROP_MASK_PAINT_MODE |
                                  GIMP_CONTEXT_PROP_MASK_BRUSH      |
                                  GIMP_CONTEXT_PROP_MASK_DYNAMICS   |
                                  GIMP_CONTEXT_PROP_MASK_PATTERN    |
                                  GIMP_CONTEXT_PROP_MASK_GRADIENT   |
                                  GIMP_CONTEXT_PROP_MASK_EXPAND),

  GIMP_CONTEXT_PROP_MASK_ALL   = (GIMP_CONTEXT_PROP_MASK_IMAGE       |
                                  GIMP_CONTEXT_PROP_MASK_DISPLAY     |
                                  GIMP_CONTEXT_PROP_MASK_TOOL        |
                                  GIMP_CONTEXT_PROP_MASK_PAINT_INFO  |
                                  GIMP_CONTEXT_PROP_MASK_MYBRUSH     |
                                  GIMP_CONTEXT_PROP_MASK_PALETTE     |
                                  GIMP_CONTEXT_PROP_MASK_FONT        |
                                  GIMP_CONTEXT_PROP_MASK_TOOL_PRESET |
                                  GIMP_CONTEXT_PROP_MASK_BUFFER      |
                                  GIMP_CONTEXT_PROP_MASK_IMAGEFILE   |
                                  GIMP_CONTEXT_PROP_MASK_TEMPLATE    |
                                  GIMP_CONTEXT_PROP_MASK_PAINT)
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
  GIMP_ITEM_TYPE_PATHS    = 1 << 2,

  GIMP_ITEM_TYPE_ALL      = (GIMP_ITEM_TYPE_LAYERS   |
                             GIMP_ITEM_TYPE_CHANNELS |
                             GIMP_ITEM_TYPE_PATHS)
} GimpItemTypeMask;
