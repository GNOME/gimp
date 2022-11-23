/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __CORE_ENUMS_H__
#define __CORE_ENUMS_H__


#if 0
   This file is parsed by two scripts, enumgen.pl in pdb,
   and ligma-mkenums. All enums that are not marked with
   /*< pdb-skip >*/ are exported to libligma and the PDB. Enums that are
   not marked with /*< skip >*/ are registered with the GType system.
   If you want the enum to be skipped by both scripts, you have to use
   /*< pdb-skip, skip >*/.

   The same syntax applies to enum values.
#endif


/*
 * these enums are registered with the type system
 */


#define LIGMA_TYPE_ALIGN_REFERENCE_TYPE (ligma_align_reference_type_get_type ())

GType ligma_align_reference_type_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_ALIGN_REFERENCE_IMAGE,          /*< desc="Image"                   >*/
  LIGMA_ALIGN_REFERENCE_SELECTION,      /*< desc="Selection"               >*/
  LIGMA_ALIGN_REFERENCE_PICK,           /*< desc="Picked reference object" >*/
} LigmaAlignReferenceType;


#define LIGMA_TYPE_ALIGNMENT_TYPE (ligma_alignment_type_get_type ())

GType ligma_alignment_type_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_ALIGN_LEFT,                     /*< desc="Align to the left"                                 >*/
  LIGMA_ALIGN_HCENTER,                  /*< desc="Center horizontally"                               >*/
  LIGMA_ALIGN_RIGHT,                    /*< desc="Align to the right"                                >*/
  LIGMA_ALIGN_TOP,                      /*< desc="Align to the top"                                  >*/
  LIGMA_ALIGN_VCENTER,                  /*< desc="Center vertically"                                 >*/
  LIGMA_ALIGN_BOTTOM,                   /*< desc="Align to the bottom"                               >*/
  LIGMA_ARRANGE_HFILL,                  /*< desc="Distribute anchor points horizontally evenly"      >*/
  LIGMA_ARRANGE_VFILL,                  /*< desc="Distribute anchor points vertically evenly"        >*/
  LIGMA_DISTRIBUTE_EVEN_HORIZONTAL_GAP, /*< desc="Distribute horizontally with even horizontal gaps" >*/
  LIGMA_DISTRIBUTE_EVEN_VERTICAL_GAP,   /*< desc="Distribute vertically with even vertical gaps"     >*/
} LigmaAlignmentType;


/**
 * LigmaBucketFillMode:
 * @LIGMA_BUCKET_FILL_FG:      FG color fill
 * @LIGMA_BUCKET_FILL_BG:      BG color fill
 * @LIGMA_BUCKET_FILL_PATTERN: Pattern fill
 *
 * Bucket fill modes.
 */
#define LIGMA_TYPE_BUCKET_FILL_MODE (ligma_bucket_fill_mode_get_type ())

GType ligma_bucket_fill_mode_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_BUCKET_FILL_FG,      /*< desc="FG color fill" >*/
  LIGMA_BUCKET_FILL_BG,      /*< desc="BG color fill" >*/
  LIGMA_BUCKET_FILL_PATTERN  /*< desc="Pattern fill"  >*/
} LigmaBucketFillMode;


#define LIGMA_TYPE_CHANNEL_BORDER_STYLE (ligma_channel_border_style_get_type ())

GType ligma_channel_border_style_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_CHANNEL_BORDER_STYLE_HARD,     /*< desc="Hard"      >*/
  LIGMA_CHANNEL_BORDER_STYLE_SMOOTH,   /*< desc="Smooth"    >*/
  LIGMA_CHANNEL_BORDER_STYLE_FEATHERED /*< desc="Feathered" >*/
} LigmaChannelBorderStyle;


/*  Note: when appending values here, don't forget to update
 *  LigmaColorFrame and other places use this enum to create combo
 *  boxes.
 */
#define LIGMA_TYPE_COLOR_PICK_MODE (ligma_color_pick_mode_get_type ())

GType ligma_color_pick_mode_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_COLOR_PICK_MODE_PIXEL,       /*< desc="Pixel"        >*/
  LIGMA_COLOR_PICK_MODE_RGB_PERCENT, /*< desc="RGB (%)"      >*/
  LIGMA_COLOR_PICK_MODE_RGB_U8,      /*< desc="RGB (0..255)" >*/
  LIGMA_COLOR_PICK_MODE_HSV,         /*< desc="HSV"          >*/
  LIGMA_COLOR_PICK_MODE_LCH,         /*< desc="CIE LCh"      >*/
  LIGMA_COLOR_PICK_MODE_LAB,         /*< desc="CIE LAB"      >*/
  LIGMA_COLOR_PICK_MODE_CMYK,        /*< desc="CMYK"         >*/
  LIGMA_COLOR_PICK_MODE_XYY,         /*< desc="CIE xyY"      >*/
  LIGMA_COLOR_PICK_MODE_YUV,         /*< desc="CIE Yu'v'"      >*/

  LIGMA_COLOR_PICK_MODE_LAST = LIGMA_COLOR_PICK_MODE_YUV /*< skip >*/
} LigmaColorPickMode;


#define LIGMA_TYPE_COLOR_PROFILE_POLICY (ligma_color_profile_policy_get_type ())

GType ligma_color_profile_policy_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_COLOR_PROFILE_POLICY_ASK,               /*< desc="Ask what to do"                                                          >*/
  LIGMA_COLOR_PROFILE_POLICY_KEEP,              /*< desc="Keep embedded profile"                                                   >*/
  LIGMA_COLOR_PROFILE_POLICY_CONVERT_BUILTIN,   /*< desc="Convert to built-in sRGB or grayscale profile"                           >*/
  LIGMA_COLOR_PROFILE_POLICY_CONVERT_PREFERRED, /*< desc="Convert to preferred RGB or grayscale profile (defaulting to built-in)" >*/
} LigmaColorProfilePolicy;


#define LIGMA_TYPE_COMPONENT_MASK (ligma_component_mask_get_type ())

GType ligma_component_mask_get_type (void) G_GNUC_CONST;

typedef enum /*< pdb-skip >*/
{
  LIGMA_COMPONENT_MASK_RED   = 1 << 0,
  LIGMA_COMPONENT_MASK_GREEN = 1 << 1,
  LIGMA_COMPONENT_MASK_BLUE  = 1 << 2,
  LIGMA_COMPONENT_MASK_ALPHA = 1 << 3,

  LIGMA_COMPONENT_MASK_ALL = (LIGMA_COMPONENT_MASK_RED   |
                             LIGMA_COMPONENT_MASK_GREEN |
                             LIGMA_COMPONENT_MASK_BLUE  |
                             LIGMA_COMPONENT_MASK_ALPHA)
} LigmaComponentMask;


#define LIGMA_TYPE_CONTAINER_POLICY (ligma_container_policy_get_type ())

GType ligma_container_policy_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_CONTAINER_POLICY_STRONG,
  LIGMA_CONTAINER_POLICY_WEAK
} LigmaContainerPolicy;


#define LIGMA_TYPE_CONVERT_DITHER_TYPE (ligma_convert_dither_type_get_type ())

GType ligma_convert_dither_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_CONVERT_DITHER_NONE,        /*< desc="None"                                     >*/
  LIGMA_CONVERT_DITHER_FS,          /*< desc="Floyd-Steinberg (normal)"                 >*/
  LIGMA_CONVERT_DITHER_FS_LOWBLEED, /*< desc="Floyd-Steinberg (reduced color bleeding)" >*/
  LIGMA_CONVERT_DITHER_FIXED,       /*< desc="Positioned"                               >*/
  LIGMA_CONVERT_DITHER_NODESTRUCT   /*< pdb-skip, skip >*/
} LigmaConvertDitherType;


#define LIGMA_TYPE_CONVOLUTION_TYPE (ligma_convolution_type_get_type ())

GType ligma_convolution_type_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_NORMAL_CONVOL,      /*  Negative numbers truncated  */
  LIGMA_ABSOLUTE_CONVOL,    /*  Absolute value              */
  LIGMA_NEGATIVE_CONVOL     /*  add 127 to values           */
} LigmaConvolutionType;


#define LIGMA_TYPE_CURVE_POINT_TYPE (ligma_curve_point_type_get_type ())

GType ligma_curve_point_type_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_CURVE_POINT_SMOOTH,   /*< desc="Smooth" >*/
  LIGMA_CURVE_POINT_CORNER    /*< desc="Corner" >*/
} LigmaCurvePointType;


#define LIGMA_TYPE_CURVE_TYPE (ligma_curve_type_get_type ())

GType ligma_curve_type_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_CURVE_SMOOTH,   /*< desc="Smooth"   >*/
  LIGMA_CURVE_FREE      /*< desc="Freehand" >*/
} LigmaCurveType;


#define LIGMA_TYPE_DASH_PRESET (ligma_dash_preset_get_type ())

GType ligma_dash_preset_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_DASH_CUSTOM,       /*< desc="Custom"         >*/
  LIGMA_DASH_LINE,         /*< desc="Line"           >*/
  LIGMA_DASH_LONG_DASH,    /*< desc="Long dashes"    >*/
  LIGMA_DASH_MEDIUM_DASH,  /*< desc="Medium dashes"  >*/
  LIGMA_DASH_SHORT_DASH,   /*< desc="Short dashes"   >*/
  LIGMA_DASH_SPARSE_DOTS,  /*< desc="Sparse dots"    >*/
  LIGMA_DASH_NORMAL_DOTS,  /*< desc="Normal dots"    >*/
  LIGMA_DASH_DENSE_DOTS,   /*< desc="Dense dots"     >*/
  LIGMA_DASH_STIPPLES,     /*< desc="Stipples"       >*/
  LIGMA_DASH_DASH_DOT,     /*< desc="Dash, dot"      >*/
  LIGMA_DASH_DASH_DOT_DOT  /*< desc="Dash, dot, dot" >*/
} LigmaDashPreset;


#define LIGMA_TYPE_DEBUG_POLICY (ligma_debug_policy_get_type ())

GType ligma_debug_policy_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_DEBUG_POLICY_WARNING,    /*< desc="Debug warnings, critical errors and crashes" >*/
  LIGMA_DEBUG_POLICY_CRITICAL,   /*< desc="Debug critical errors and crashes"           >*/
  LIGMA_DEBUG_POLICY_FATAL,      /*< desc="Debug crashes only"                          >*/
  LIGMA_DEBUG_POLICY_NEVER       /*< desc="Never debug LIGMA"                            >*/
} LigmaDebugPolicy;


#define LIGMA_TYPE_DIRTY_MASK (ligma_dirty_mask_get_type ())

GType ligma_dirty_mask_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_DIRTY_NONE            = 0,

  LIGMA_DIRTY_IMAGE           = 1 << 0,
  LIGMA_DIRTY_IMAGE_SIZE      = 1 << 1,
  LIGMA_DIRTY_IMAGE_META      = 1 << 2,
  LIGMA_DIRTY_IMAGE_STRUCTURE = 1 << 3,
  LIGMA_DIRTY_ITEM            = 1 << 4,
  LIGMA_DIRTY_ITEM_META       = 1 << 5,
  LIGMA_DIRTY_DRAWABLE        = 1 << 6,
  LIGMA_DIRTY_VECTORS         = 1 << 7,
  LIGMA_DIRTY_SELECTION       = 1 << 8,
  LIGMA_DIRTY_ACTIVE_DRAWABLE = 1 << 9,

  LIGMA_DIRTY_ALL             = 0xffff
} LigmaDirtyMask;


#define LIGMA_TYPE_DYNAMICS_OUTPUT_TYPE (ligma_dynamics_output_type_get_type ())

GType ligma_dynamics_output_type_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_DYNAMICS_OUTPUT_OPACITY,      /*< desc="Opacity"      >*/
  LIGMA_DYNAMICS_OUTPUT_SIZE,         /*< desc="Size"         >*/
  LIGMA_DYNAMICS_OUTPUT_ANGLE,        /*< desc="Angle"        >*/
  LIGMA_DYNAMICS_OUTPUT_COLOR,        /*< desc="Color"        >*/
  LIGMA_DYNAMICS_OUTPUT_HARDNESS,     /*< desc="Hardness"     >*/
  LIGMA_DYNAMICS_OUTPUT_FORCE,        /*< desc="Force"        >*/
  LIGMA_DYNAMICS_OUTPUT_ASPECT_RATIO, /*< desc="Aspect ratio" >*/
  LIGMA_DYNAMICS_OUTPUT_SPACING,      /*< desc="Spacing"      >*/
  LIGMA_DYNAMICS_OUTPUT_RATE,         /*< desc="Rate"         >*/
  LIGMA_DYNAMICS_OUTPUT_FLOW,         /*< desc="Flow"         >*/
  LIGMA_DYNAMICS_OUTPUT_JITTER,       /*< desc="Jitter"       >*/
} LigmaDynamicsOutputType;


#define LIGMA_TYPE_FILL_STYLE (ligma_fill_style_get_type ())

GType ligma_fill_style_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_FILL_STYLE_SOLID,  /*< desc="Solid color" >*/
  LIGMA_FILL_STYLE_PATTERN /*< desc="Pattern"     >*/
} LigmaFillStyle;


#define LIGMA_TYPE_FILTER_REGION (ligma_filter_region_get_type ())

GType ligma_filter_region_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_FILTER_REGION_SELECTION, /*< desc="Use the selection as input"    >*/
  LIGMA_FILTER_REGION_DRAWABLE   /*< desc="Use the entire layer as input" >*/
} LigmaFilterRegion;


#define LIGMA_TYPE_GRADIENT_COLOR (ligma_gradient_color_get_type ())

GType ligma_gradient_color_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_GRADIENT_COLOR_FIXED,                  /*< desc="Fixed"                                           >*/
  LIGMA_GRADIENT_COLOR_FOREGROUND,             /*< desc="Foreground color",               abbrev="FG"     >*/
  LIGMA_GRADIENT_COLOR_FOREGROUND_TRANSPARENT, /*< desc="Foreground color (transparent)",  abbrev="FG (t)" >*/
  LIGMA_GRADIENT_COLOR_BACKGROUND,             /*< desc="Background color",               abbrev="BG"     >*/
  LIGMA_GRADIENT_COLOR_BACKGROUND_TRANSPARENT  /*< desc="Background color (transparent)", abbrev="BG (t)" >*/
} LigmaGradientColor;


#define LIGMA_TYPE_GRAVITY_TYPE (ligma_gravity_type_get_type ())

GType ligma_gravity_type_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_GRAVITY_NONE,
  LIGMA_GRAVITY_NORTH_WEST,
  LIGMA_GRAVITY_NORTH,
  LIGMA_GRAVITY_NORTH_EAST,
  LIGMA_GRAVITY_WEST,
  LIGMA_GRAVITY_CENTER,
  LIGMA_GRAVITY_EAST,
  LIGMA_GRAVITY_SOUTH_WEST,
  LIGMA_GRAVITY_SOUTH,
  LIGMA_GRAVITY_SOUTH_EAST
} LigmaGravityType;


#define LIGMA_TYPE_GUIDE_STYLE (ligma_guide_style_get_type ())

GType ligma_guide_style_get_type (void) G_GNUC_CONST;

typedef enum /*< pdb-skip >*/
{
  LIGMA_GUIDE_STYLE_NONE,
  LIGMA_GUIDE_STYLE_NORMAL,
  LIGMA_GUIDE_STYLE_MIRROR,
  LIGMA_GUIDE_STYLE_MANDALA,
  LIGMA_GUIDE_STYLE_SPLIT_VIEW
} LigmaGuideStyle;


#define LIGMA_TYPE_HISTOGRAM_CHANNEL (ligma_histogram_channel_get_type ())

GType ligma_histogram_channel_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_HISTOGRAM_VALUE     = 0,  /*< desc="Value"         >*/
  LIGMA_HISTOGRAM_RED       = 1,  /*< desc="Red"           >*/
  LIGMA_HISTOGRAM_GREEN     = 2,  /*< desc="Green"         >*/
  LIGMA_HISTOGRAM_BLUE      = 3,  /*< desc="Blue"          >*/
  LIGMA_HISTOGRAM_ALPHA     = 4,  /*< desc="Alpha"         >*/
  LIGMA_HISTOGRAM_LUMINANCE = 5,  /*< desc="Luminance"     >*/
  LIGMA_HISTOGRAM_RGB       = 6   /*< desc="RGB", pdb-skip >*/
} LigmaHistogramChannel;


#define LIGMA_TYPE_ITEM_SET (ligma_item_set_get_type ())

GType ligma_item_set_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_ITEM_SET_NONE,        /*< desc="None"               >*/
  LIGMA_ITEM_SET_ALL,         /*< desc="All layers"         >*/
  LIGMA_ITEM_SET_IMAGE_SIZED, /*< desc="Image-sized layers" >*/
  LIGMA_ITEM_SET_VISIBLE,     /*< desc="All visible layers" >*/
} LigmaItemSet;


#define LIGMA_TYPE_MATTING_ENGINE (ligma_matting_engine_get_type ())

GType ligma_matting_engine_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_MATTING_ENGINE_GLOBAL,  /*< desc="Matting Global" >*/
  LIGMA_MATTING_ENGINE_LEVIN,   /*< desc="Matting Levin" >*/
} LigmaMattingEngine;


#define LIGMA_TYPE_MESSAGE_SEVERITY (ligma_message_severity_get_type ())

GType ligma_message_severity_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_MESSAGE_INFO,        /*< desc="Message"  >*/
  LIGMA_MESSAGE_WARNING,     /*< desc="Warning"  >*/
  LIGMA_MESSAGE_ERROR,       /*< desc="Error"    >*/
  LIGMA_MESSAGE_BUG_WARNING, /*< desc="WARNING"  >*/
  LIGMA_MESSAGE_BUG_CRITICAL /*< desc="CRITICAL" >*/
} LigmaMessageSeverity;


#define LIGMA_TYPE_METADATA_ROTATION_POLICY (ligma_metadata_rotation_policy_get_type ())

GType ligma_metadata_rotation_policy_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_METADATA_ROTATION_POLICY_ASK,    /*< desc="Ask what to do"                         >*/
  LIGMA_METADATA_ROTATION_POLICY_KEEP,   /*< desc="Discard metadata without rotating"      >*/
  LIGMA_METADATA_ROTATION_POLICY_ROTATE  /*< desc="Rotate the image then discard metadata" >*/
} LigmaMetadataRotationPolicy;


#define LIGMA_TYPE_PASTE_TYPE (ligma_paste_type_get_type ())

GType ligma_paste_type_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_PASTE_TYPE_FLOATING,
  LIGMA_PASTE_TYPE_FLOATING_IN_PLACE,
  LIGMA_PASTE_TYPE_FLOATING_INTO,
  LIGMA_PASTE_TYPE_FLOATING_INTO_IN_PLACE,
  LIGMA_PASTE_TYPE_NEW_LAYER,
  LIGMA_PASTE_TYPE_NEW_LAYER_IN_PLACE,
  LIGMA_PASTE_TYPE_NEW_LAYER_OR_FLOATING,
  LIGMA_PASTE_TYPE_NEW_LAYER_OR_FLOATING_IN_PLACE,
  LIGMA_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING,
  LIGMA_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING_IN_PLACE,
} LigmaPasteType;


#define LIGMA_TYPE_WIN32_POINTER_INPUT_API (ligma_win32_pointer_input_api_get_type ())

GType ligma_win32_pointer_input_api_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_WIN32_POINTER_INPUT_API_WINTAB,      /*< desc="Wintab"      >*/
  LIGMA_WIN32_POINTER_INPUT_API_WINDOWS_INK  /*< desc="Windows Ink" >*/
} LigmaWin32PointerInputAPI;


#define LIGMA_TYPE_THUMBNAIL_SIZE (ligma_thumbnail_size_get_type ())

GType ligma_thumbnail_size_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_THUMBNAIL_SIZE_NONE    = 0,    /*< desc="No thumbnails"    >*/
  LIGMA_THUMBNAIL_SIZE_NORMAL  = 128,  /*< desc="Normal (128x128)" >*/
  LIGMA_THUMBNAIL_SIZE_LARGE   = 256   /*< desc="Large (256x256)"  >*/
} LigmaThumbnailSize;


#define LIGMA_TYPE_TRC_TYPE (ligma_trc_type_get_type ())

GType ligma_trc_type_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_TRC_LINEAR,     /*< desc="Linear"     >*/
  LIGMA_TRC_NON_LINEAR, /*< desc="Non-Linear" >*/
  LIGMA_TRC_PERCEPTUAL  /*< desc="Perceptual" >*/
} LigmaTRCType;


#define LIGMA_TYPE_UNDO_EVENT (ligma_undo_event_get_type ())

GType ligma_undo_event_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_UNDO_EVENT_UNDO_PUSHED,  /* a new undo has been added to the undo stack */
  LIGMA_UNDO_EVENT_UNDO_EXPIRED, /* an undo has been freed from the undo stack  */
  LIGMA_UNDO_EVENT_REDO_EXPIRED, /* a redo has been freed from the redo stack   */
  LIGMA_UNDO_EVENT_UNDO,         /* an undo has been executed                   */
  LIGMA_UNDO_EVENT_REDO,         /* a redo has been executed                    */
  LIGMA_UNDO_EVENT_UNDO_FREE,    /* all undo and redo info has been cleared     */
  LIGMA_UNDO_EVENT_UNDO_FREEZE,  /* undo has been frozen                        */
  LIGMA_UNDO_EVENT_UNDO_THAW     /* undo has been thawn                         */
} LigmaUndoEvent;


#define LIGMA_TYPE_UNDO_MODE (ligma_undo_mode_get_type ())

GType ligma_undo_mode_get_type (void) G_GNUC_CONST;

typedef enum /*< pdb-skip >*/
{
  LIGMA_UNDO_MODE_UNDO,
  LIGMA_UNDO_MODE_REDO
} LigmaUndoMode;


#define LIGMA_TYPE_UNDO_TYPE (ligma_undo_type_get_type ())

GType ligma_undo_type_get_type (void) G_GNUC_CONST;

typedef enum /*< pdb-skip >*/
{
  /* Type NO_UNDO_GROUP (0) is special - in the ligmaimage structure it
   * means there is no undo group currently being added to.
   */
  LIGMA_UNDO_GROUP_NONE = 0,              /*< desc="<<invalid>>"                    >*/

  LIGMA_UNDO_GROUP_FIRST = LIGMA_UNDO_GROUP_NONE, /*< skip >*/

  LIGMA_UNDO_GROUP_IMAGE_SCALE,           /*< desc="Scale image"                    >*/
  LIGMA_UNDO_GROUP_IMAGE_RESIZE,          /*< desc="Resize image"                   >*/
  LIGMA_UNDO_GROUP_IMAGE_FLIP,            /*< desc="Flip image"                     >*/
  LIGMA_UNDO_GROUP_IMAGE_ROTATE,          /*< desc="Rotate image"                   >*/
  LIGMA_UNDO_GROUP_IMAGE_TRANSFORM,       /*< desc="Transform image"                >*/
  LIGMA_UNDO_GROUP_IMAGE_CROP,            /*< desc="Crop image"                     >*/
  LIGMA_UNDO_GROUP_IMAGE_CONVERT,         /*< desc="Convert image"                  >*/
  LIGMA_UNDO_GROUP_IMAGE_ITEM_REMOVE,     /*< desc="Remove item"                    >*/
  LIGMA_UNDO_GROUP_IMAGE_ITEM_REORDER,    /*< desc="Reorder item"                   >*/
  LIGMA_UNDO_GROUP_IMAGE_LAYERS_MERGE,    /*< desc="Merge layers"                   >*/
  LIGMA_UNDO_GROUP_IMAGE_VECTORS_MERGE,   /*< desc="Merge paths"                    >*/
  LIGMA_UNDO_GROUP_IMAGE_QUICK_MASK,      /*< desc="Quick Mask"                     >*/
  LIGMA_UNDO_GROUP_IMAGE_GRID,            /*< desc="Grid"                           >*/
  LIGMA_UNDO_GROUP_GUIDE,                 /*< desc="Guide"                          >*/
  LIGMA_UNDO_GROUP_SAMPLE_POINT,          /*< desc="Sample Point"                   >*/
  LIGMA_UNDO_GROUP_DRAWABLE,              /*< desc="Layer/Channel"                  >*/
  LIGMA_UNDO_GROUP_DRAWABLE_MOD,          /*< desc="Layer/Channel modification"     >*/
  LIGMA_UNDO_GROUP_MASK,                  /*< desc="Selection mask"                 >*/
  LIGMA_UNDO_GROUP_ITEM_VISIBILITY,       /*< desc="Item visibility"                >*/
  LIGMA_UNDO_GROUP_ITEM_LOCK_CONTENTS,    /*< desc="Lock/Unlock contents"           >*/
  LIGMA_UNDO_GROUP_ITEM_LOCK_POSITION,    /*< desc="Lock/Unlock position"           >*/
  LIGMA_UNDO_GROUP_ITEM_LOCK_VISIBILITY,  /*< desc="Lock/Unlock visibility"         >*/
  LIGMA_UNDO_GROUP_ITEM_PROPERTIES,       /*< desc="Item properties"                >*/
  LIGMA_UNDO_GROUP_ITEM_DISPLACE,         /*< desc="Move item"                      >*/
  LIGMA_UNDO_GROUP_ITEM_SCALE,            /*< desc="Scale item"                     >*/
  LIGMA_UNDO_GROUP_ITEM_RESIZE,           /*< desc="Resize item"                    >*/
  LIGMA_UNDO_GROUP_LAYER_ADD,             /*< desc="Add layer"                      >*/
  LIGMA_UNDO_GROUP_LAYER_ADD_ALPHA,       /*< desc="Add alpha channel"              >*/
  LIGMA_UNDO_GROUP_LAYER_ADD_MASK,        /*< desc="Add layer mask"                 >*/
  LIGMA_UNDO_GROUP_LAYER_APPLY_MASK,      /*< desc="Apply layer mask"               >*/
  LIGMA_UNDO_GROUP_LAYER_REMOVE_ALPHA,    /*< desc="Remove alpha channel"           >*/
  LIGMA_UNDO_GROUP_LAYER_LOCK_ALPHA,      /*< desc="Lock/Unlock alpha channels"     >*/
  LIGMA_UNDO_GROUP_LAYER_OPACITY,         /*< desc="Set layers opacity"             >*/
  LIGMA_UNDO_GROUP_LAYER_MODE,            /*< desc="Set layers mode"                >*/
  LIGMA_UNDO_GROUP_CHANNEL_ADD,           /*< desc="Add channels"                   >*/
  LIGMA_UNDO_GROUP_FS_TO_LAYER,           /*< desc="Floating selection to layer"    >*/
  LIGMA_UNDO_GROUP_FS_FLOAT,              /*< desc="Float selection"                >*/
  LIGMA_UNDO_GROUP_FS_ANCHOR,             /*< desc="Anchor floating selection"      >*/
  LIGMA_UNDO_GROUP_EDIT_PASTE,            /*< desc="Paste"                          >*/
  LIGMA_UNDO_GROUP_EDIT_CUT,              /*< desc="Cut"                            >*/
  LIGMA_UNDO_GROUP_TEXT,                  /*< desc="Text"                           >*/
  LIGMA_UNDO_GROUP_TRANSFORM,             /*< desc="Transform"                      >*/
  LIGMA_UNDO_GROUP_PAINT,                 /*< desc="Paint"                          >*/
  LIGMA_UNDO_GROUP_PARASITE_ATTACH,       /*< desc="Attach parasite"                >*/
  LIGMA_UNDO_GROUP_PARASITE_REMOVE,       /*< desc="Remove parasite"                >*/
  LIGMA_UNDO_GROUP_VECTORS_IMPORT,        /*< desc="Import paths"                   >*/
  LIGMA_UNDO_GROUP_MISC,                  /*< desc="Plug-In"                        >*/

  LIGMA_UNDO_GROUP_LAST = LIGMA_UNDO_GROUP_MISC, /*< skip >*/

  /*  Undo types which actually do something  */

  LIGMA_UNDO_IMAGE_TYPE,                  /*< desc="Image type"                     >*/
  LIGMA_UNDO_IMAGE_PRECISION,             /*< desc="Image precision"                >*/
  LIGMA_UNDO_IMAGE_SIZE,                  /*< desc="Image size"                     >*/
  LIGMA_UNDO_IMAGE_RESOLUTION,            /*< desc="Image resolution change"        >*/
  LIGMA_UNDO_IMAGE_GRID,                  /*< desc="Grid"                           >*/
  LIGMA_UNDO_IMAGE_METADATA,              /*< desc="Change metadata"                >*/
  LIGMA_UNDO_IMAGE_COLORMAP,              /*< desc="Change indexed palette"         >*/
  LIGMA_UNDO_IMAGE_HIDDEN_PROFILE,        /*< desc="Hide/Unhide color profile"      >*/
  LIGMA_UNDO_GUIDE,                       /*< desc="Guide"                          >*/
  LIGMA_UNDO_SAMPLE_POINT,                /*< desc="Sample Point"                   >*/
  LIGMA_UNDO_DRAWABLE,                    /*< desc="Layer/Channel"                  >*/
  LIGMA_UNDO_DRAWABLE_MOD,                /*< desc="Layer/Channel modification"     >*/
  LIGMA_UNDO_DRAWABLE_FORMAT,             /*< desc="Layer/Channel format"           >*/
  LIGMA_UNDO_MASK,                        /*< desc="Selection mask"                 >*/
  LIGMA_UNDO_ITEM_REORDER,                /*< desc="Reorder item"                   >*/
  LIGMA_UNDO_ITEM_RENAME,                 /*< desc="Rename item"                    >*/
  LIGMA_UNDO_ITEM_DISPLACE,               /*< desc="Move item"                      >*/
  LIGMA_UNDO_ITEM_VISIBILITY,             /*< desc="Item visibility"                >*/
  LIGMA_UNDO_ITEM_COLOR_TAG,              /*< desc="Item color tag"                 >*/
  LIGMA_UNDO_ITEM_LOCK_CONTENT,           /*< desc="Lock/Unlock content"            >*/
  LIGMA_UNDO_ITEM_LOCK_POSITION,          /*< desc="Lock/Unlock position"           >*/
  LIGMA_UNDO_ITEM_LOCK_VISIBILITY,        /*< desc="Lock/Unlock visibility"      >*/
  LIGMA_UNDO_LAYER_ADD,                   /*< desc="New layer"                      >*/
  LIGMA_UNDO_LAYER_REMOVE,                /*< desc="Delete layer"                   >*/
  LIGMA_UNDO_LAYER_MODE,                  /*< desc="Set layer mode"                 >*/
  LIGMA_UNDO_LAYER_OPACITY,               /*< desc="Set layer opacity"              >*/
  LIGMA_UNDO_LAYER_LOCK_ALPHA,            /*< desc="Lock/Unlock alpha channel"      >*/
  LIGMA_UNDO_GROUP_LAYER_SUSPEND_RESIZE,  /*< desc="Suspend group layer resize"     >*/
  LIGMA_UNDO_GROUP_LAYER_RESUME_RESIZE,   /*< desc="Resume group layer resize"      >*/
  LIGMA_UNDO_GROUP_LAYER_SUSPEND_MASK,    /*< desc="Suspend group layer mask"       >*/
  LIGMA_UNDO_GROUP_LAYER_RESUME_MASK,     /*< desc="Resume group layer mask"        >*/
  LIGMA_UNDO_GROUP_LAYER_START_TRANSFORM, /*< desc="Start transforming group layer" >*/
  LIGMA_UNDO_GROUP_LAYER_END_TRANSFORM,   /*< desc="End transforming group layer"   >*/
  LIGMA_UNDO_GROUP_LAYER_CONVERT,         /*< desc="Convert group layer"            >*/
  LIGMA_UNDO_TEXT_LAYER,                  /*< desc="Text layer"                     >*/
  LIGMA_UNDO_TEXT_LAYER_MODIFIED,         /*< desc="Text layer modification"        >*/
  LIGMA_UNDO_TEXT_LAYER_CONVERT,          /*< desc="Convert text layer"             >*/
  LIGMA_UNDO_LAYER_MASK_ADD,              /*< desc="Add layer mask"                 >*/
  LIGMA_UNDO_LAYER_MASK_REMOVE,           /*< desc="Delete layer mask"              >*/
  LIGMA_UNDO_LAYER_MASK_APPLY,            /*< desc="Apply layer mask"               >*/
  LIGMA_UNDO_LAYER_MASK_SHOW,             /*< desc="Show layer mask"                >*/
  LIGMA_UNDO_CHANNEL_ADD,                 /*< desc="New channel"                    >*/
  LIGMA_UNDO_CHANNEL_REMOVE,              /*< desc="Delete channel"                 >*/
  LIGMA_UNDO_CHANNEL_COLOR,               /*< desc="Channel color"                  >*/
  LIGMA_UNDO_VECTORS_ADD,                 /*< desc="New path"                       >*/
  LIGMA_UNDO_VECTORS_REMOVE,              /*< desc="Delete path"                    >*/
  LIGMA_UNDO_VECTORS_MOD,                 /*< desc="Path modification"              >*/
  LIGMA_UNDO_FS_TO_LAYER,                 /*< desc="Floating selection to layer"    >*/
  LIGMA_UNDO_TRANSFORM_GRID,              /*< desc="Transform grid"                 >*/
  LIGMA_UNDO_PAINT,                       /*< desc="Paint"                          >*/
  LIGMA_UNDO_INK,                         /*< desc="Ink"                            >*/
  LIGMA_UNDO_FOREGROUND_SELECT,           /*< desc="Select foreground"              >*/
  LIGMA_UNDO_PARASITE_ATTACH,             /*< desc="Attach parasite"                >*/
  LIGMA_UNDO_PARASITE_REMOVE,             /*< desc="Remove parasite"                >*/

  LIGMA_UNDO_CANT                         /*< desc="Not undoable"                   >*/
} LigmaUndoType;


#define LIGMA_TYPE_VIEW_SIZE (ligma_view_size_get_type ())

GType ligma_view_size_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_VIEW_SIZE_TINY        = 12,   /*< desc="Tiny"        >*/
  LIGMA_VIEW_SIZE_EXTRA_SMALL = 16,   /*< desc="Very small"  >*/
  LIGMA_VIEW_SIZE_SMALL       = 24,   /*< desc="Small"       >*/
  LIGMA_VIEW_SIZE_MEDIUM      = 32,   /*< desc="Medium"      >*/
  LIGMA_VIEW_SIZE_LARGE       = 48,   /*< desc="Large"       >*/
  LIGMA_VIEW_SIZE_EXTRA_LARGE = 64,   /*< desc="Very large"  >*/
  LIGMA_VIEW_SIZE_HUGE        = 96,   /*< desc="Huge"        >*/
  LIGMA_VIEW_SIZE_ENORMOUS    = 128,  /*< desc="Enormous"    >*/
  LIGMA_VIEW_SIZE_GIGANTIC    = 192   /*< desc="Gigantic"    >*/
} LigmaViewSize;


#define LIGMA_TYPE_VIEW_TYPE (ligma_view_type_get_type ())

GType ligma_view_type_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_VIEW_TYPE_LIST,  /*< desc="View as list" >*/
  LIGMA_VIEW_TYPE_GRID   /*< desc="View as grid" >*/
} LigmaViewType;


#define LIGMA_TYPE_SELECT_METHOD (ligma_select_method_get_type ())

GType ligma_select_method_get_type (void) G_GNUC_CONST;

typedef enum /*< pdb-skip >*/
{
  LIGMA_SELECT_PLAIN_TEXT,    /*< desc="Selection by basic text search"         >*/
  LIGMA_SELECT_REGEX_PATTERN, /*< desc="Selection by regular expression search" >*/
  LIGMA_SELECT_GLOB_PATTERN,  /*< desc="Selection by glob pattern search"       >*/
} LigmaSelectMethod;

/*
 * non-registered enums; register them if needed
 */


typedef enum  /*< pdb-skip, skip >*/
{
  LIGMA_CONTEXT_PROP_FIRST       =  2,

  LIGMA_CONTEXT_PROP_IMAGE       =  LIGMA_CONTEXT_PROP_FIRST,
  LIGMA_CONTEXT_PROP_DISPLAY     =  3,
  LIGMA_CONTEXT_PROP_TOOL        =  4,
  LIGMA_CONTEXT_PROP_PAINT_INFO  =  5,
  LIGMA_CONTEXT_PROP_FOREGROUND  =  6,
  LIGMA_CONTEXT_PROP_BACKGROUND  =  7,
  LIGMA_CONTEXT_PROP_OPACITY     =  8,
  LIGMA_CONTEXT_PROP_PAINT_MODE  =  9,
  LIGMA_CONTEXT_PROP_BRUSH       = 10,
  LIGMA_CONTEXT_PROP_DYNAMICS    = 11,
  LIGMA_CONTEXT_PROP_MYBRUSH     = 12,
  LIGMA_CONTEXT_PROP_PATTERN     = 13,
  LIGMA_CONTEXT_PROP_GRADIENT    = 14,
  LIGMA_CONTEXT_PROP_PALETTE     = 15,
  LIGMA_CONTEXT_PROP_FONT        = 16,
  LIGMA_CONTEXT_PROP_TOOL_PRESET = 17,
  LIGMA_CONTEXT_PROP_BUFFER      = 18,
  LIGMA_CONTEXT_PROP_IMAGEFILE   = 19,
  LIGMA_CONTEXT_PROP_TEMPLATE    = 20,

  LIGMA_CONTEXT_PROP_LAST        = LIGMA_CONTEXT_PROP_TEMPLATE
} LigmaContextPropType;


typedef enum  /*< pdb-skip, skip >*/
{
  LIGMA_CONTEXT_PROP_MASK_IMAGE       = 1 <<  2,
  LIGMA_CONTEXT_PROP_MASK_DISPLAY     = 1 <<  3,
  LIGMA_CONTEXT_PROP_MASK_TOOL        = 1 <<  4,
  LIGMA_CONTEXT_PROP_MASK_PAINT_INFO  = 1 <<  5,
  LIGMA_CONTEXT_PROP_MASK_FOREGROUND  = 1 <<  6,
  LIGMA_CONTEXT_PROP_MASK_BACKGROUND  = 1 <<  7,
  LIGMA_CONTEXT_PROP_MASK_OPACITY     = 1 <<  8,
  LIGMA_CONTEXT_PROP_MASK_PAINT_MODE  = 1 <<  9,
  LIGMA_CONTEXT_PROP_MASK_BRUSH       = 1 << 10,
  LIGMA_CONTEXT_PROP_MASK_DYNAMICS    = 1 << 11,
  LIGMA_CONTEXT_PROP_MASK_MYBRUSH     = 1 << 12,
  LIGMA_CONTEXT_PROP_MASK_PATTERN     = 1 << 13,
  LIGMA_CONTEXT_PROP_MASK_GRADIENT    = 1 << 14,
  LIGMA_CONTEXT_PROP_MASK_PALETTE     = 1 << 15,
  LIGMA_CONTEXT_PROP_MASK_FONT        = 1 << 16,
  LIGMA_CONTEXT_PROP_MASK_TOOL_PRESET = 1 << 17,
  LIGMA_CONTEXT_PROP_MASK_BUFFER      = 1 << 18,
  LIGMA_CONTEXT_PROP_MASK_IMAGEFILE   = 1 << 19,
  LIGMA_CONTEXT_PROP_MASK_TEMPLATE    = 1 << 20,

  /*  aliases  */
  LIGMA_CONTEXT_PROP_MASK_PAINT = (LIGMA_CONTEXT_PROP_MASK_FOREGROUND |
                                  LIGMA_CONTEXT_PROP_MASK_BACKGROUND |
                                  LIGMA_CONTEXT_PROP_MASK_OPACITY    |
                                  LIGMA_CONTEXT_PROP_MASK_PAINT_MODE |
                                  LIGMA_CONTEXT_PROP_MASK_BRUSH      |
                                  LIGMA_CONTEXT_PROP_MASK_DYNAMICS   |
                                  LIGMA_CONTEXT_PROP_MASK_PATTERN    |
                                  LIGMA_CONTEXT_PROP_MASK_GRADIENT),

  LIGMA_CONTEXT_PROP_MASK_ALL   = (LIGMA_CONTEXT_PROP_MASK_IMAGE       |
                                  LIGMA_CONTEXT_PROP_MASK_DISPLAY     |
                                  LIGMA_CONTEXT_PROP_MASK_TOOL        |
                                  LIGMA_CONTEXT_PROP_MASK_PAINT_INFO  |
                                  LIGMA_CONTEXT_PROP_MASK_MYBRUSH     |
                                  LIGMA_CONTEXT_PROP_MASK_PALETTE     |
                                  LIGMA_CONTEXT_PROP_MASK_FONT        |
                                  LIGMA_CONTEXT_PROP_MASK_TOOL_PRESET |
                                  LIGMA_CONTEXT_PROP_MASK_BUFFER      |
                                  LIGMA_CONTEXT_PROP_MASK_IMAGEFILE   |
                                  LIGMA_CONTEXT_PROP_MASK_TEMPLATE    |
                                  LIGMA_CONTEXT_PROP_MASK_PAINT)
} LigmaContextPropMask;


typedef enum  /*< pdb-skip, skip >*/
{
  LIGMA_IMAGE_SCALE_OK,
  LIGMA_IMAGE_SCALE_TOO_SMALL,
  LIGMA_IMAGE_SCALE_TOO_BIG
} LigmaImageScaleCheckType;


typedef enum  /*< pdb-skip, skip >*/
{
  LIGMA_ITEM_TYPE_LAYERS   = 1 << 0,
  LIGMA_ITEM_TYPE_CHANNELS = 1 << 1,
  LIGMA_ITEM_TYPE_VECTORS  = 1 << 2,

  LIGMA_ITEM_TYPE_ALL      = (LIGMA_ITEM_TYPE_LAYERS   |
                             LIGMA_ITEM_TYPE_CHANNELS |
                             LIGMA_ITEM_TYPE_VECTORS)
} LigmaItemTypeMask;


#endif /* __CORE_ENUMS_H__ */
