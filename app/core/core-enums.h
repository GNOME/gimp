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

/*< proxy-skip >*/

#ifndef __CORE_ENUMS_H__
#define __CORE_ENUMS_H__

#if 0
   This file is parsed by three scripts, enumgen.pl in tools/pdbgen,
   gimp-mkenums, and gimp-mkproxy. All enums that are not marked with 
   /*< pdb-skip >*/ are exported to libgimp and the PDB. Enums that are
   not marked with /*< skip >*/ are registered with the GType system. 
   If you want the enum to be skipped by both scripts, you have to use 
   /*< pdb-skip >*/ _before_ /*< skip >*/. 

   All enum values that are marked with /*< skip >*/ are skipped for
   both targets.

   Anything not between proxy-skip and proxy-resume 
   pairs will be copied into libgimpproxy by gimp-mkproxy.
#endif

/* 
 * these enums that are registered with the type system
 */

#define GIMP_TYPE_ORIENTATION_TYPE (gimp_orientation_type_get_type ())

GType gimp_orientation_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_ORIENTATION_HORIZONTAL, /*< desc="Horizontal" >*/
  GIMP_ORIENTATION_VERTICAL,   /*< desc="Vertical"   >*/
  GIMP_ORIENTATION_UNKNOWN     /*< desc="Unknown"    >*/
} GimpOrientationType;


#define GIMP_TYPE_BLEND_MODE (gimp_blend_mode_get_type ())

GType gimp_blend_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_FG_BG_RGB_MODE,         /*< desc="FG to BG (RGB)"    >*/
  GIMP_FG_BG_HSV_MODE,         /*< desc="FG to BG (HSV)"    >*/
  GIMP_FG_TRANSPARENT_MODE,    /*< desc="FG to Transparent" >*/
  GIMP_CUSTOM_MODE             /*< desc="Custom Gradient"   >*/
} GimpBlendMode;


#define GIMP_TYPE_BUCKET_FILL_MODE (gimp_bucket_fill_mode_get_type ())

GType gimp_bucket_fill_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_FG_BUCKET_FILL,      /*< desc="FG Color Fill" >*/
  GIMP_BG_BUCKET_FILL,      /*< desc="BG Color Fill" >*/
  GIMP_PATTERN_BUCKET_FILL  /*< desc="Pattern Fill"  >*/
} GimpBucketFillMode;


#define GIMP_TYPE_CHANNEL_OPS (gimp_channel_ops_get_type ())

GType gimp_channel_ops_get_type (void) G_GNUC_CONST;

/*< proxy-resume >*/
typedef enum
{
  GIMP_CHANNEL_OP_ADD,       /*< desc="Add to the current selection"         >*/
  GIMP_CHANNEL_OP_SUBTRACT,  /*< desc="Subtract from the current selection"  >*/
  GIMP_CHANNEL_OP_REPLACE,   /*< desc="Replace the current selection"        >*/
  GIMP_CHANNEL_OP_INTERSECT  /*< desc="Intersect with the current selection" >*/
} GimpChannelOps;
/*< proxy-skip >*/


#define GIMP_TYPE_CHANNEL_TYPE (gimp_channel_type_get_type ())

GType gimp_channel_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_RED_CHANNEL,
  GIMP_GREEN_CHANNEL,
  GIMP_BLUE_CHANNEL,
  GIMP_GRAY_CHANNEL,
  GIMP_INDEXED_CHANNEL,
  GIMP_ALPHA_CHANNEL
} GimpChannelType;


#define GIMP_TYPE_CONVERT_DITHER_TYPE (gimp_convert_dither_type_get_type ())

GType gimp_convert_dither_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_NO_DITHER,          /*< desc="No Color Dithering"         >*/
  GIMP_FS_DITHER,          /*< desc="Floyd-Steinberg Color Dithering (Normal)" >*/
  GIMP_FSLOWBLEED_DITHER,  /*< desc="Floyd-Steinberg Color Dithering (Reduced Color Bleeding)" >*/
  GIMP_FIXED_DITHER,       /*< desc="Positioned Color Dithering" >*/
  GIMP_NODESTRUCT_DITHER   /*< skip >*/
} GimpConvertDitherType;


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
  GIMP_FOREGROUND_FILL,   /*< desc="Foreground"  >*/ 
  GIMP_BACKGROUND_FILL,   /*< desc="Background"  >*/
  GIMP_WHITE_FILL,        /*< desc="White"       >*/
  GIMP_TRANSPARENT_FILL,  /*< desc="Transparent" >*/
  GIMP_NO_FILL            /*< desc="None"        >*/
} GimpFillType;


#define GIMP_TYPE_GRADIENT_TYPE (gimp_gradient_type_get_type ())

GType gimp_gradient_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_LINEAR,                /*< desc="Linear"                 >*/ 
  GIMP_BILINEAR,              /*< desc="Bi-Linear"              >*/
  GIMP_RADIAL,                /*< desc="Radial"                 >*/
  GIMP_SQUARE,                /*< desc="Square"                 >*/
  GIMP_CONICAL_SYMMETRIC,     /*< desc="Conical (symmetric)"    >*/
  GIMP_CONICAL_ASYMMETRIC,    /*< desc="Conical (asymmetric)"   >*/
  GIMP_SHAPEBURST_ANGULAR,    /*< desc="Shapeburst (angular)"   >*/
  GIMP_SHAPEBURST_SPHERICAL,  /*< desc="Shapeburst (spherical)" >*/
  GIMP_SHAPEBURST_DIMPLED,    /*< desc="Shapeburst (dimpled)"   >*/
  GIMP_SPIRAL_CLOCKWISE,      /*< desc="Spiral (clockwise)"     >*/
  GIMP_SPIRAL_ANTICLOCKWISE   /*< desc="Spiral (anticlockwise)" >*/
} GimpGradientType;


#define GIMP_TYPE_IMAGE_BASE_TYPE (gimp_image_base_type_get_type ())

GType gimp_image_base_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_RGB,     /*< desc="RGB"       >*/
  GIMP_GRAY,    /*< desc="Grayscale" >*/
  GIMP_INDEXED  /*< desc="Indexed"   >*/
} GimpImageBaseType;


#define GIMP_TYPE_IMAGE_TYPE (gimp_image_type_get_type ())

GType gimp_image_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_RGB_IMAGE,      /*< desc="RGB"             >*/
  GIMP_RGBA_IMAGE,     /*< desc="RGB-Alpha"       >*/
  GIMP_GRAY_IMAGE,     /*< desc="Grayscale"       >*/
  GIMP_GRAYA_IMAGE,    /*< desc="Grayscale-Alpha" >*/
  GIMP_INDEXED_IMAGE,  /*< desc="Indexed"         >*/
  GIMP_INDEXEDA_IMAGE  /*< desc="Indexed-Alpha"   >*/
} GimpImageType;


#define GIMP_TYPE_PREVIEW_SIZE (gimp_preview_size_get_type ())

GType gimp_preview_size_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_PREVIEW_SIZE_NONE        = 0,    /*< desc="None"        >*/
  GIMP_PREVIEW_SIZE_TINY        = 16,   /*< desc="Tiny"        >*/
  GIMP_PREVIEW_SIZE_EXTRA_SMALL = 24,   /*< desc="Very Small"  >*/
  GIMP_PREVIEW_SIZE_SMALL       = 32,   /*< desc="Small"       >*/
  GIMP_PREVIEW_SIZE_MEDIUM      = 48,   /*< desc="Medium"      >*/
  GIMP_PREVIEW_SIZE_LARGE       = 64,   /*< desc="Large"       >*/
  GIMP_PREVIEW_SIZE_EXTRA_LARGE = 96,   /*< desc="Very Large"  >*/
  GIMP_PREVIEW_SIZE_HUGE        = 128,  /*< desc="Huge"        >*/
  GIMP_PREVIEW_SIZE_ENORMOUS    = 192,  /*< desc="Enormous"    >*/
  GIMP_PREVIEW_SIZE_GIGANTIC    = 256   /*< desc="Gigantic"    >*/
} GimpPreviewSize;


#define GIMP_TYPE_REPEAT_MODE (gimp_repeat_mode_get_type ())

GType gimp_repeat_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_REPEAT_NONE,       /*< desc="None"            >*/
  GIMP_REPEAT_SAWTOOTH,   /*< desc="Sawtooth Wave"   >*/
  GIMP_REPEAT_TRIANGULAR  /*< desc="Triangular Wave" >*/
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
  GIMP_THUMBNAIL_SIZE_NONE    = 0,    /*< desc="No Thumbnails"    >*/
  GIMP_THUMBNAIL_SIZE_NORMAL  = 128,  /*< desc="Normal (128x128)" >*/
  GIMP_THUMBNAIL_SIZE_LARGE   = 256   /*< desc="Large (256x256)"  >*/
} GimpThumbnailSize;


#define GIMP_TYPE_TRANSFORM_DIRECTION (gimp_transform_direction_get_type ())

GType gimp_transform_direction_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_TRANSFORM_FORWARD,   /*< desc="Forward (Traditional)" >*/
  GIMP_TRANSFORM_BACKWARD   /*< desc="Backward (Corrective)" >*/
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
  GIMP_UNDO_EVENT_UNDO_FREE     /* all undo and redo info has been cleared     */
} GimpUndoEvent;


#define GIMP_TYPE_UNDO_TYPE (gimp_undo_type_get_type ())

GType gimp_undo_type_get_type (void) G_GNUC_CONST;

typedef enum /*< pdb-skip >*/
{
  /* Type NO_UNDO_GROUP (0) is special - in the gimpimage structure it
   * means there is no undo group currently being added to.
   */
  GIMP_UNDO_GROUP_NONE = 0,           /*< desc="<<invalid>>"               >*/

  GIMP_UNDO_GROUP_FIRST = GIMP_UNDO_GROUP_NONE, /*< skip >*/

  GIMP_UNDO_GROUP_IMAGE_SCALE,        /*< desc="Scale Image"               >*/
  GIMP_UNDO_GROUP_IMAGE_RESIZE,       /*< desc="Resize Image"              >*/
  GIMP_UNDO_GROUP_IMAGE_CONVERT,      /*< desc="Convert Image"             >*/
  GIMP_UNDO_GROUP_IMAGE_CROP,         /*< desc="Crop Image"                >*/
  GIMP_UNDO_GROUP_IMAGE_LAYERS_MERGE, /*< desc="Merge Layers"              >*/
  GIMP_UNDO_GROUP_IMAGE_QMASK,        /*< desc="QuickMask"                 >*/
  GIMP_UNDO_GROUP_IMAGE_GUIDE,        /*< desc="Guide"                     >*/
  GIMP_UNDO_GROUP_LAYER_PROPERTIES,   /*< desc="Layer Properties"          >*/
  GIMP_UNDO_GROUP_LAYER_SCALE,        /*< desc="Scale Layer"               >*/
  GIMP_UNDO_GROUP_LAYER_RESIZE,       /*< desc="Resize Layer"              >*/
  GIMP_UNDO_GROUP_LAYER_DISPLACE,     /*< desc="Move Layer"                >*/
  GIMP_UNDO_GROUP_LAYER_LINKED,       /*< desc="Linked Layer"              >*/
  GIMP_UNDO_GROUP_LAYER_APPLY_MASK,   /*< desc="Apply Layer Mask"          >*/
  GIMP_UNDO_GROUP_FS_FLOAT,           /*< desc="Float Selection"           >*/
  GIMP_UNDO_GROUP_FS_ANCHOR,          /*< desc="Anchor Floating Selection" >*/
  GIMP_UNDO_GROUP_EDIT_PASTE,         /*< desc="Paste"                     >*/
  GIMP_UNDO_GROUP_EDIT_CUT,           /*< desc="Cut"                       >*/
  GIMP_UNDO_GROUP_EDIT_COPY,          /*< desc="Copy"                      >*/
  GIMP_UNDO_GROUP_TEXT,               /*< desc="Text"                      >*/
  GIMP_UNDO_GROUP_TRANSFORM,          /*< desc="Transform"                 >*/
  GIMP_UNDO_GROUP_PAINT,              /*< desc="Paint"                     >*/
  GIMP_UNDO_GROUP_PARASITE_ATTACH,    /*< desc="Attach Parasite"           >*/
  GIMP_UNDO_GROUP_PARASITE_REMOVE,    /*< desc="Remove Parasite"           >*/
  GIMP_UNDO_GROUP_MISC,               /*< desc="Plug-In"                   >*/

  GIMP_UNDO_GROUP_LAST = GIMP_UNDO_GROUP_MISC, /*< skip >*/

  /*  Undo types which actually do something  */

  GIMP_UNDO_IMAGE,                    /*< desc="Image"                     >*/
  GIMP_UNDO_IMAGE_MOD,                /*< desc="Image Mod"                 >*/
  GIMP_UNDO_IMAGE_TYPE,               /*< desc="Image Type"                >*/
  GIMP_UNDO_IMAGE_SIZE,               /*< desc="Image Size"                >*/
  GIMP_UNDO_IMAGE_RESOLUTION,         /*< desc="Resolution Change"         >*/
  GIMP_UNDO_IMAGE_QMASK,              /*< desc="QuickMask"                 >*/
  GIMP_UNDO_IMAGE_GUIDE,              /*< desc="Guide"                     >*/
  GIMP_UNDO_MASK,                     /*< desc="Selection Mask"            >*/
  GIMP_UNDO_ITEM_RENAME,              /*< desc="Rename Item"               >*/
  GIMP_UNDO_LAYER_ADD,                /*< desc="New Layer"                 >*/
  GIMP_UNDO_LAYER_REMOVE,             /*< desc="Delete Layer"              >*/
  GIMP_UNDO_LAYER_MOD,                /*< desc="Layer Mod"                 >*/
  GIMP_UNDO_LAYER_MASK_ADD,           /*< desc="Add Layer Mask"            >*/
  GIMP_UNDO_LAYER_MASK_REMOVE,        /*< desc="Delete Layer Mask"         >*/
  GIMP_UNDO_LAYER_REPOSITION,         /*< desc="Layer Reposition"          >*/
  GIMP_UNDO_LAYER_DISPLACE,           /*< desc="Layer Move"                >*/
  GIMP_UNDO_CHANNEL_ADD,              /*< desc="New Channel"               >*/
  GIMP_UNDO_CHANNEL_REMOVE,           /*< desc="Delete Channel"            >*/
  GIMP_UNDO_CHANNEL_MOD,              /*< desc="Channel Mod"               >*/
  GIMP_UNDO_CHANNEL_REPOSITION,       /*< desc="Channel Reposition"        >*/
  GIMP_UNDO_VECTORS_ADD,              /*< desc="New Vectors"               >*/
  GIMP_UNDO_VECTORS_REMOVE,           /*< desc="Delete Vectors"            >*/
  GIMP_UNDO_VECTORS_MOD,              /*< desc="Vectors Mod"               >*/
  GIMP_UNDO_VECTORS_REPOSITION,       /*< desc="Vectors Reposition"        >*/
  GIMP_UNDO_FS_TO_LAYER,              /*< desc="FS to Layer"               >*/
  GIMP_UNDO_FS_RIGOR,                 /*< desc="FS Rigor"                  >*/
  GIMP_UNDO_FS_RELAX,                 /*< desc="FS Relax"                  >*/
  GIMP_UNDO_TRANSFORM,                /*< desc="Transform"                 >*/
  GIMP_UNDO_PAINT,                    /*< desc="Paint"                     >*/
  GIMP_UNDO_PARASITE_ATTACH,          /*< desc="Attach Parasite"           >*/
  GIMP_UNDO_PARASITE_REMOVE,          /*< desc="Remove Parasite"           >*/

  GIMP_UNDO_CANT                      /*< desc="EEK: can't undo"           >*/
} GimpUndoType;


/*
 * non-registered enums; register them if needed
 */

typedef enum  /*< skip >*/
{
  GIMP_MAKE_PALETTE,
  GIMP_REUSE_PALETTE,
  GIMP_WEB_PALETTE,
  GIMP_MONO_PALETTE,
  GIMP_CUSTOM_PALETTE
} GimpConvertPaletteType;

typedef enum  /*< pdb-skip >*/ /*< skip >*/
{
  GIMP_GRAD_LINEAR = 0,
  GIMP_GRAD_CURVED,
  GIMP_GRAD_SINE,
  GIMP_GRAD_SPHERE_INCREASING,
  GIMP_GRAD_SPHERE_DECREASING
} GimpGradientSegmentType;

typedef enum  /*< pdb-skip >*/ /*< skip >*/
{
  GIMP_GRAD_RGB,      /* normal RGB           */
  GIMP_GRAD_HSV_CCW,  /* counterclockwise hue */
  GIMP_GRAD_HSV_CW    /* clockwise hue        */
} GimpGradientSegmentColor;

typedef enum  /*< skip >*/
{
  GIMP_ADD_WHITE_MASK,
  GIMP_ADD_BLACK_MASK,
  GIMP_ADD_ALPHA_MASK,
  GIMP_ADD_SELECTION_MASK,
  GIMP_ADD_INVERSE_SELECTION_MASK,
  GIMP_ADD_COPY_MASK,
  GIMP_ADD_INVERSE_COPY_MASK
} GimpAddMaskType;

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


#endif /* __CORE_ENUMS_H__ */
