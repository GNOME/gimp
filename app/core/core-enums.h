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


#define GIMP_TYPE_GRADIENT_TYPE (gimp_gradient_type_get_type ())

GType gimp_gradient_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_LINEAR,                 /*< desc="Linear"                 >*/ 
  GIMP_BILINEAR,               /*< desc="Bi-Linear"              >*/
  GIMP_RADIAL,                 /*< desc="Radial"                 >*/
  GIMP_SQUARE,                 /*< desc="Square"                 >*/
  GIMP_CONICAL_SYMMETRIC,      /*< desc="Conical (symmetric)"    >*/
  GIMP_CONICAL_ASYMMETRIC,     /*< desc="Conical (asymmetric)"   >*/
  GIMP_SHAPEBURST_ANGULAR,     /*< desc="Shapeburst (angular)"   >*/
  GIMP_SHAPEBURST_SPHERICAL,   /*< desc="Shapeburst (spherical)" >*/
  GIMP_SHAPEBURST_DIMPLED,     /*< desc="Shapeburst (dimpled)"   >*/
  GIMP_SPIRAL_CLOCKWISE,       /*< desc="Spiral (clockwise)"     >*/
  GIMP_SPIRAL_ANTICLOCKWISE    /*< desc="Spiral (anticlockwise)" >*/
} GimpGradientType;


#define GIMP_TYPE_IMAGE_BASE_TYPE (gimp_image_base_type_get_type ())

GType gimp_image_base_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_RGB,
  GIMP_GRAY,
  GIMP_INDEXED
} GimpImageBaseType;


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


#define GIMP_TYPE_TRANSFER_MODE (gimp_transfer_mode_get_type ())

GType gimp_transfer_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_SHADOWS,     /*< desc="Shadows"    >*/
  GIMP_MIDTONES,    /*< desc="Midtones"   >*/
  GIMP_HIGHLIGHTS   /*< desc="Highlights" >*/
} GimpTransferMode;


#define GIMP_TYPE_TRANSFORM_DIRECTION (gimp_transform_direction_get_type ())

GType gimp_transform_direction_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_TRANSFORM_FORWARD,   /*< desc="Forward (Traditional)" >*/
  GIMP_TRANSFORM_BACKWARD   /*< desc="Backward (Corrective)" >*/
} GimpTransformDirection;


/*
 * non-registered enums; register them if needed
 */

typedef enum  /*< proxy-resume >*/ /*< skip >*/
{
  GIMP_CHANNEL_OP_ADD,
  GIMP_CHANNEL_OP_SUBTRACT,
  GIMP_CHANNEL_OP_REPLACE,
  GIMP_CHANNEL_OP_INTERSECT
} GimpChannelOps;

typedef enum  /*< proxy-skip >*/ /*< skip >*/
{
  GIMP_MAKE_PALETTE,
  GIMP_REUSE_PALETTE,
  GIMP_WEB_PALETTE,
  GIMP_MONO_PALETTE,
  GIMP_CUSTOM_PALETTE
} GimpConvertPaletteType;

typedef enum  /*< skip >*/
{
  GIMP_FOREGROUND_FILL,
  GIMP_BACKGROUND_FILL,
  GIMP_WHITE_FILL,
  GIMP_TRANSPARENT_FILL,
  GIMP_NO_FILL
} GimpFillType;

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
  GIMP_RGB_IMAGE,
  GIMP_RGBA_IMAGE,
  GIMP_GRAY_IMAGE,
  GIMP_GRAYA_IMAGE,
  GIMP_INDEXED_IMAGE,
  GIMP_INDEXEDA_IMAGE
} GimpImageType;

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
