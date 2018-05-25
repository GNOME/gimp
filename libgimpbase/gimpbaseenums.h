/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_BASE_ENUMS_H__
#define __GIMP_BASE_ENUMS_H__


/**
 * SECTION: gimpbaseenums
 * @title: gimpbaseenums
 * @short_description: Basic GIMP enumeration data types.
 *
 * Basic GIMP enumeration data types.
 **/


G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * GimpAddMaskType:
 * @GIMP_ADD_MASK_WHITE:          White (full opacity)
 * @GIMP_ADD_MASK_BLACK:          Black (full transparency)
 * @GIMP_ADD_MASK_ALPHA:          Layer's alpha channel
 * @GIMP_ADD_MASK_ALPHA_TRANSFER: Transfer layer's alpha channel
 * @GIMP_ADD_MASK_SELECTION:      Selection
 * @GIMP_ADD_MASK_COPY:           Grayscale copy of layer
 * @GIMP_ADD_MASK_CHANNEL:        Channel
 *
 * Modes of initialising a layer mask.
 **/
#define GIMP_TYPE_ADD_MASK_TYPE (gimp_add_mask_type_get_type ())

GType gimp_add_mask_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_ADD_MASK_WHITE,          /*< desc="_White (full opacity)"           >*/
  GIMP_ADD_MASK_BLACK,          /*< desc="_Black (full transparency)"      >*/
  GIMP_ADD_MASK_ALPHA,          /*< desc="Layer's _alpha channel"          >*/
  GIMP_ADD_MASK_ALPHA_TRANSFER, /*< desc="_Transfer layer's alpha channel" >*/
  GIMP_ADD_MASK_SELECTION,      /*< desc="_Selection"                      >*/
  GIMP_ADD_MASK_COPY,           /*< desc="_Grayscale copy of layer"        >*/
  GIMP_ADD_MASK_CHANNEL         /*< desc="C_hannel"                        >*/
} GimpAddMaskType;


/**
 * GimpBlendMode:
 * @GIMP_BLEND_FG_BG_RGB:      FG to BG (RGB)
 * @GIMP_BLEND_FG_BG_HSV:      FG to BG (HSV)
 * @GIMP_BLEND_FG_TRANSPARENT: FG to transparent
 * @GIMP_BLEND_CUSTOM:         Custom gradient
 *
 * Types of gradients.
 **/
#define GIMP_TYPE_BLEND_MODE (gimp_blend_mode_get_type ())

GType gimp_blend_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_BLEND_FG_BG_RGB,      /*< desc="FG to BG (RGB)"    >*/
  GIMP_BLEND_FG_BG_HSV,      /*< desc="FG to BG (HSV)"    >*/
  GIMP_BLEND_FG_TRANSPARENT, /*< desc="FG to transparent" >*/
  GIMP_BLEND_CUSTOM          /*< desc="Custom gradient"   >*/
} GimpBlendMode;


/**
 * GimpBrushGeneratedShape:
 * @GIMP_BRUSH_GENERATED_CIRCLE:  Circle
 * @GIMP_BRUSH_GENERATED_SQUARE:  Square
 * @GIMP_BRUSH_GENERATED_DIAMOND: Diamond
 *
 * Shapes of generated brushes.
 **/
#define GIMP_TYPE_BRUSH_GENERATED_SHAPE (gimp_brush_generated_shape_get_type ())

GType gimp_brush_generated_shape_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_BRUSH_GENERATED_CIRCLE,  /*< desc="Circle"  >*/
  GIMP_BRUSH_GENERATED_SQUARE,  /*< desc="Square"  >*/
  GIMP_BRUSH_GENERATED_DIAMOND  /*< desc="Diamond" >*/
} GimpBrushGeneratedShape;


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

typedef enum
{
  GIMP_BUCKET_FILL_FG,      /*< desc="FG color fill" >*/
  GIMP_BUCKET_FILL_BG,      /*< desc="BG color fill" >*/
  GIMP_BUCKET_FILL_PATTERN  /*< desc="Pattern fill"  >*/
} GimpBucketFillMode;


/**
 * GimpCapStyle:
 * @GIMP_CAP_BUTT:   Butt
 * @GIMP_CAP_ROUND:  Round
 * @GIMP_CAP_SQUARE: Square
 *
 * Style of line endings.
 **/
#define GIMP_TYPE_CAP_STYLE (gimp_cap_style_get_type ())

GType gimp_cap_style_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_CAP_BUTT,   /*< desc="Butt"   >*/
  GIMP_CAP_ROUND,  /*< desc="Round"  >*/
  GIMP_CAP_SQUARE  /*< desc="Square" >*/
} GimpCapStyle;


/**
 * GimpChannelOps:
 * @GIMP_CHANNEL_OP_ADD:       Add to the current selection
 * @GIMP_CHANNEL_OP_SUBTRACT:  Subtract from the current selection
 * @GIMP_CHANNEL_OP_REPLACE:   Replace the current selection
 * @GIMP_CHANNEL_OP_INTERSECT: Intersect with the current selection
 *
 * Operations to combine channels and selections.
 **/
#define GIMP_TYPE_CHANNEL_OPS (gimp_channel_ops_get_type ())

GType gimp_channel_ops_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_CHANNEL_OP_ADD,       /*< desc="Add to the current selection"         >*/
  GIMP_CHANNEL_OP_SUBTRACT,  /*< desc="Subtract from the current selection"  >*/
  GIMP_CHANNEL_OP_REPLACE,   /*< desc="Replace the current selection"        >*/
  GIMP_CHANNEL_OP_INTERSECT  /*< desc="Intersect with the current selection" >*/
} GimpChannelOps;


/**
 * GimpChannelType:
 * @GIMP_CHANNEL_RED:     Red
 * @GIMP_CHANNEL_GREEN:   Green
 * @GIMP_CHANNEL_BLUE:    Blue
 * @GIMP_CHANNEL_GRAY:    Gray
 * @GIMP_CHANNEL_INDEXED: Indexed
 * @GIMP_CHANNEL_ALPHA:   Alpha
 *
 * Channels (as in color components).
 **/
#define GIMP_TYPE_CHANNEL_TYPE (gimp_channel_type_get_type ())

GType gimp_channel_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_CHANNEL_RED,      /*< desc="Red"     >*/
  GIMP_CHANNEL_GREEN,    /*< desc="Green"   >*/
  GIMP_CHANNEL_BLUE,     /*< desc="Blue"    >*/
  GIMP_CHANNEL_GRAY,     /*< desc="Gray"    >*/
  GIMP_CHANNEL_INDEXED,  /*< desc="Indexed" >*/
  GIMP_CHANNEL_ALPHA     /*< desc="Alpha"   >*/
} GimpChannelType;


/**
 * GimpCheckSize:
 * @GIMP_CHECK_SIZE_SMALL_CHECKS:  Small
 * @GIMP_CHECK_SIZE_MEDIUM_CHECKS: Medium
 * @GIMP_CHECK_SIZE_LARGE_CHECKS:  Large
 *
 * Size of the checkerboard indicating transparency.
 **/
#define GIMP_TYPE_CHECK_SIZE (gimp_check_size_get_type ())

GType gimp_check_size_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_CHECK_SIZE_SMALL_CHECKS  = 0,  /*< desc="Small"  >*/
  GIMP_CHECK_SIZE_MEDIUM_CHECKS = 1,  /*< desc="Medium" >*/
  GIMP_CHECK_SIZE_LARGE_CHECKS  = 2   /*< desc="Large"  >*/
} GimpCheckSize;


/**
 * GimpCheckType:
 * @GIMP_CHECK_TYPE_LIGHT_CHECKS: Light checks
 * @GIMP_CHECK_TYPE_GRAY_CHECKS:  Mid-tone checks
 * @GIMP_CHECK_TYPE_DARK_CHECKS:  Dark checks
 * @GIMP_CHECK_TYPE_WHITE_ONLY:   White only
 * @GIMP_CHECK_TYPE_GRAY_ONLY:    Gray only
 * @GIMP_CHECK_TYPE_BLACK_ONLY:   Black only
 *
 * Color/Brightness of the checkerboard indicating transparency.
 **/
#define GIMP_TYPE_CHECK_TYPE (gimp_check_type_get_type ())

GType gimp_check_type_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_CHECK_TYPE_LIGHT_CHECKS = 0,  /*< desc="Light checks"    >*/
  GIMP_CHECK_TYPE_GRAY_CHECKS  = 1,  /*< desc="Mid-tone checks" >*/
  GIMP_CHECK_TYPE_DARK_CHECKS  = 2,  /*< desc="Dark checks"     >*/
  GIMP_CHECK_TYPE_WHITE_ONLY   = 3,  /*< desc="White only"      >*/
  GIMP_CHECK_TYPE_GRAY_ONLY    = 4,  /*< desc="Gray only"       >*/
  GIMP_CHECK_TYPE_BLACK_ONLY   = 5   /*< desc="Black only"      >*/
} GimpCheckType;


/**
 * GimpCloneType:
 * @GIMP_CLONE_IMAGE:   Clone from an image/drawable source
 * @GIMP_CLONE_PATTERN: Clone from a pattern source
 *
 * Clone sources.
 **/
#define GIMP_TYPE_CLONE_TYPE (gimp_clone_type_get_type ())

GType gimp_clone_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_CLONE_IMAGE,    /*< desc="Image"   >*/
  GIMP_CLONE_PATTERN   /*< desc="Pattern" >*/
} GimpCloneType;


/**
 * GimpColorTag:
 * @GIMP_COLOR_TAG_NONE:   None
 * @GIMP_COLOR_TAG_BLUE:   Blue
 * @GIMP_COLOR_TAG_GREEN:  Green
 * @GIMP_COLOR_TAG_YELLOW: Yellow
 * @GIMP_COLOR_TAG_ORANGE: Orange
 * @GIMP_COLOR_TAG_BROWN:  Brown
 * @GIMP_COLOR_TAG_RED:    Red
 * @GIMP_COLOR_TAG_VIOLET: Violet
 * @GIMP_COLOR_TAG_GRAY:   Gray
 *
 * Possible tag colors.
 *
 * Since: 2.10
 **/
#define GIMP_TYPE_COLOR_TAG (gimp_color_tag_get_type ())

GType gimp_color_tag_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_COLOR_TAG_NONE,   /*< desc="None"   >*/
  GIMP_COLOR_TAG_BLUE,   /*< desc="Blue"   >*/
  GIMP_COLOR_TAG_GREEN,  /*< desc="Green"  >*/
  GIMP_COLOR_TAG_YELLOW, /*< desc="Yellow" >*/
  GIMP_COLOR_TAG_ORANGE, /*< desc="Orange" >*/
  GIMP_COLOR_TAG_BROWN,  /*< desc="Brown"  >*/
  GIMP_COLOR_TAG_RED,    /*< desc="Red"    >*/
  GIMP_COLOR_TAG_VIOLET, /*< desc="Violet" >*/
  GIMP_COLOR_TAG_GRAY    /*< desc="Gray"   >*/
} GimpColorTag;


/**
 * GimpComponentType:
 * @GIMP_COMPONENT_TYPE_U8:     8-bit integer
 * @GIMP_COMPONENT_TYPE_U16:    16-bit integer
 * @GIMP_COMPONENT_TYPE_U32:    32-bit integer
 * @GIMP_COMPONENT_TYPE_HALF:   16-bit floating point
 * @GIMP_COMPONENT_TYPE_FLOAT:  32-bit floating point
 * @GIMP_COMPONENT_TYPE_DOUBLE: 64-bit floating point
 *
 * Encoding types of image components.
 *
 * Since: 2.10
 **/
#define GIMP_TYPE_COMPONENT_TYPE (gimp_component_type_get_type ())

GType gimp_component_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_COMPONENT_TYPE_U8     = 100, /*< desc="8-bit integer"         >*/
  GIMP_COMPONENT_TYPE_U16    = 200, /*< desc="16-bit integer"        >*/
  GIMP_COMPONENT_TYPE_U32    = 300, /*< desc="32-bit integer"        >*/
  GIMP_COMPONENT_TYPE_HALF   = 500, /*< desc="16-bit floating point" >*/
  GIMP_COMPONENT_TYPE_FLOAT  = 600, /*< desc="32-bit floating point" >*/
  GIMP_COMPONENT_TYPE_DOUBLE = 700  /*< desc="64-bit floating point" >*/
} GimpComponentType;


/**
 * GimpConvertPaletteType:
 * @GIMP_CONVERT_PALETTE_GENERATE: Generate optimum palette
 * @GIMP_CONVERT_PALETTE_REUSE:    Don't use this one
 * @GIMP_CONVERT_PALETTE_WEB:      Use web-optimized palette
 * @GIMP_CONVERT_PALETTE_MONO:     Use black and white (1-bit) palette
 * @GIMP_CONVERT_PALETTE_CUSTOM:   Use custom palette
 *
 * Types of palettes for indexed conversion.
 **/
#define GIMP_TYPE_CONVERT_PALETTE_TYPE (gimp_convert_palette_type_get_type ())

GType gimp_convert_palette_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_CONVERT_PALETTE_GENERATE, /*< desc="Generate optimum palette"          >*/
  GIMP_CONVERT_PALETTE_REUSE,    /*< skip >*/
  GIMP_CONVERT_PALETTE_WEB,      /*< desc="Use web-optimized palette"         >*/
  GIMP_CONVERT_PALETTE_MONO,     /*< desc="Use black and white (1-bit) palette" >*/
  GIMP_CONVERT_PALETTE_CUSTOM    /*< desc="Use custom palette"                >*/
} GimpConvertPaletteType;


/**
 * GimpConvolveType:
 * @GIMP_CONVOLVE_BLUR:    Blur
 * @GIMP_CONVOLVE_SHARPEN: Sharpen
 *
 * Types of convolutions.
 **/
#define GIMP_TYPE_CONVOLVE_TYPE (gimp_convolve_type_get_type ())

GType gimp_convolve_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_CONVOLVE_BLUR,    /*< desc="Blur"    >*/
  GIMP_CONVOLVE_SHARPEN  /*< desc="Sharpen" >*/
} GimpConvolveType;


/**
 * GimpDesaturateMode:
 * @GIMP_DESATURATE_LIGHTNESS:  Lightness (HSL)
 * @GIMP_DESATURATE_LUMA:       Luma
 * @GIMP_DESATURATE_AVERAGE:    Average (HSI Intensity)
 * @GIMP_DESATURATE_LUMINANCE:  Luminance
 * @GIMP_DESATURATE_VALUE:      Value (HSV)
 *
 * Grayscale conversion methods.
 **/
#define GIMP_TYPE_DESATURATE_MODE (gimp_desaturate_mode_get_type ())

GType gimp_desaturate_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_DESATURATE_LIGHTNESS,   /*< desc="Lightness (HSL)"          >*/
  GIMP_DESATURATE_LUMA,        /*< desc="Luma"                     >*/
  GIMP_DESATURATE_AVERAGE,     /*< desc="Average (HSI Intensity)"  >*/
  GIMP_DESATURATE_LUMINANCE,   /*< desc="Luminance"                >*/
  GIMP_DESATURATE_VALUE        /*< desc="Value (HSV)"              >*/
} GimpDesaturateMode;


/**
 * GimpDodgeBurnType:
 * @GIMP_DODGE_BURN_TYPE_DODGE: Dodge
 * @GIMP_DODGE_BURN_TYPE_BURN:  Burn
 *
 * Methods for the dodge/burn operation.
 **/
#define GIMP_TYPE_DODGE_BURN_TYPE (gimp_dodge_burn_type_get_type ())

GType gimp_dodge_burn_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_DODGE_BURN_TYPE_DODGE,  /*< desc="Dodge" >*/
  GIMP_DODGE_BURN_TYPE_BURN    /*< desc="Burn"  >*/
} GimpDodgeBurnType;


/**
 * GimpFillType:
 * @GIMP_FILL_FOREGROUND:  Foreground color
 * @GIMP_FILL_BACKGROUND:  Background color
 * @GIMP_FILL_WHITE:       White
 * @GIMP_FILL_TRANSPARENT: Transparency
 * @GIMP_FILL_PATTERN:     Pattern
 *
 * Types of filling.
 **/
#define GIMP_TYPE_FILL_TYPE (gimp_fill_type_get_type ())

GType gimp_fill_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_FILL_FOREGROUND,  /*< desc="Foreground color" >*/
  GIMP_FILL_BACKGROUND,  /*< desc="Background color" >*/
  GIMP_FILL_WHITE,       /*< desc="White"            >*/
  GIMP_FILL_TRANSPARENT, /*< desc="Transparency"     >*/
  GIMP_FILL_PATTERN      /*< desc="Pattern"          >*/
} GimpFillType;


/**
 * GimpForegroundExtractMode:
 * @GIMP_FOREGROUND_EXTRACT_SIOX:    Siox
 * @GIMP_FOREGROUND_EXTRACT_MATTING: Matting (Since 2.10)
 *
 * Foreground extraxt engines.
 **/
#define GIMP_TYPE_FOREGROUND_EXTRACT_MODE (gimp_foreground_extract_mode_get_type ())

GType gimp_foreground_extract_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_FOREGROUND_EXTRACT_SIOX,
  GIMP_FOREGROUND_EXTRACT_MATTING
} GimpForegroundExtractMode;


/**
 * GimpGradientBlendColorSpace:
 * @GIMP_GRADIENT_BLEND_RGB_PERCEPTUAL: Perceptual RGB
 * @GIMP_GRADIENT_BLEND_RGB_LINEAR:     Linear RGB
 * @GIMP_GRADIENT_BLEND_CIE_LAB:        CIE Lab
 *
 * Color space for blending gradients.
 *
 * Since: 2.10
 */
#define GIMP_TYPE_GRADIENT_BLEND_COLOR_SPACE (gimp_gradient_blend_color_space_get_type ())

GType gimp_gradient_blend_color_space_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_GRADIENT_BLEND_RGB_PERCEPTUAL,  /*< desc="Perceptual RGB", nick=rgb-perceptual >*/
  GIMP_GRADIENT_BLEND_RGB_LINEAR,      /*< desc="Linear RGB",     nick=rgb-linear     >*/
  GIMP_GRADIENT_BLEND_CIE_LAB          /*< desc="CIE Lab",        nick=cie-lab        >*/
} GimpGradientBlendColorSpace;


/**
 * GimpGradientSegmentColor:
 * @GIMP_GRADIENT_SEGMENT_RGB:     RGB
 * @GIMP_GRADIENT_SEGMENT_HSV_CCW: HSV (counter-clockwise hue)
 * @GIMP_GRADIENT_SEGMENT_HSV_CW:  HSV (clockwise hue)
 *
 * Coloring types for gradient segments.
 **/
#define GIMP_TYPE_GRADIENT_SEGMENT_COLOR (gimp_gradient_segment_color_get_type ())

GType gimp_gradient_segment_color_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_GRADIENT_SEGMENT_RGB,      /*< desc="RGB"                                             >*/
  GIMP_GRADIENT_SEGMENT_HSV_CCW,  /*< desc="HSV (counter-clockwise hue)", abbrev="HSV (ccw)" >*/
  GIMP_GRADIENT_SEGMENT_HSV_CW    /*< desc="HSV (clockwise hue)",         abbrev="HSV (cw)"  >*/
} GimpGradientSegmentColor;


/**
 * GimpGradientSegmentType:
 * @GIMP_GRADIENT_SEGMENT_LINEAR:            Linear
 * @GIMP_GRADIENT_SEGMENT_CURVED:            Curved
 * @GIMP_GRADIENT_SEGMENT_SINE:              Sinusoidal
 * @GIMP_GRADIENT_SEGMENT_SPHERE_INCREASING: Spherical (increasing)
 * @GIMP_GRADIENT_SEGMENT_SPHERE_DECREASING: Spherical (decreasing)
 *
 * Transition functions for gradient segments.
 **/
#define GIMP_TYPE_GRADIENT_SEGMENT_TYPE (gimp_gradient_segment_type_get_type ())

GType gimp_gradient_segment_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_GRADIENT_SEGMENT_LINEAR,             /*< desc="Linear"                                           >*/
  GIMP_GRADIENT_SEGMENT_CURVED,             /*< desc="Curved"                                           >*/
  GIMP_GRADIENT_SEGMENT_SINE,               /*< desc="Sinusoidal"                                       >*/
  GIMP_GRADIENT_SEGMENT_SPHERE_INCREASING,  /*< desc="Spherical (increasing)", abbrev="Spherical (inc)" >*/
  GIMP_GRADIENT_SEGMENT_SPHERE_DECREASING   /*< desc="Spherical (decreasing)", abbrev="Spherical (dec)" >*/
} GimpGradientSegmentType;


/**
 * GimpGradientType:
 * @GIMP_GRADIENT_LINEAR:               Linear
 * @GIMP_GRADIENT_BILINEAR:             Bi-linear
 * @GIMP_GRADIENT_RADIAL:               Radial
 * @GIMP_GRADIENT_SQUARE:               Square
 * @GIMP_GRADIENT_CONICAL_SYMMETRIC:    Conical (symmetric)
 * @GIMP_GRADIENT_CONICAL_ASYMMETRIC:   Conical (asymmetric)
 * @GIMP_GRADIENT_SHAPEBURST_ANGULAR:   Shaped (angular)
 * @GIMP_GRADIENT_SHAPEBURST_SPHERICAL: Shaped (spherical)
 * @GIMP_GRADIENT_SHAPEBURST_DIMPLED:   Shaped (dimpled)
 * @GIMP_GRADIENT_SPIRAL_CLOCKWISE:     Spiral (clockwise)
 * @GIMP_GRADIENT_SPIRAL_ANTICLOCKWISE: Spiral (counter-clockwise)
 *
 * Gradient shapes.
 **/
#define GIMP_TYPE_GRADIENT_TYPE (gimp_gradient_type_get_type ())

GType gimp_gradient_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_GRADIENT_LINEAR,                /*< desc="Linear"                                              >*/
  GIMP_GRADIENT_BILINEAR,              /*< desc="Bi-linear"                                           >*/
  GIMP_GRADIENT_RADIAL,                /*< desc="Radial"                                              >*/
  GIMP_GRADIENT_SQUARE,                /*< desc="Square"                                              >*/
  GIMP_GRADIENT_CONICAL_SYMMETRIC,     /*< desc="Conical (symmetric)",        abbrev="Conical (sym)"  >*/
  GIMP_GRADIENT_CONICAL_ASYMMETRIC,    /*< desc="Conical (asymmetric)",       abbrev="Conical (asym)" >*/
  GIMP_GRADIENT_SHAPEBURST_ANGULAR,    /*< desc="Shaped (angular)"                                    >*/
  GIMP_GRADIENT_SHAPEBURST_SPHERICAL,  /*< desc="Shaped (spherical)"                                  >*/
  GIMP_GRADIENT_SHAPEBURST_DIMPLED,    /*< desc="Shaped (dimpled)"                                    >*/
  GIMP_GRADIENT_SPIRAL_CLOCKWISE,      /*< desc="Spiral (clockwise)",         abbrev="Spiral (cw)"    >*/
  GIMP_GRADIENT_SPIRAL_ANTICLOCKWISE   /*< desc="Spiral (counter-clockwise)", abbrev="Spiral (ccw)"   >*/
} GimpGradientType;


/**
 * GimpGridStyle:
 * @GIMP_GRID_DOTS:          Intersections (dots)
 * @GIMP_GRID_INTERSECTIONS: Intersections (crosshairs)
 * @GIMP_GRID_ON_OFF_DASH:   Dashed
 * @GIMP_GRID_DOUBLE_DASH:   Double dashed
 * @GIMP_GRID_SOLID:         Solid
 *
 * Rendering types for the display grid.
 **/
#define GIMP_TYPE_GRID_STYLE (gimp_grid_style_get_type ())

GType gimp_grid_style_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_GRID_DOTS,           /*< desc="Intersections (dots)"       >*/
  GIMP_GRID_INTERSECTIONS,  /*< desc="Intersections (crosshairs)" >*/
  GIMP_GRID_ON_OFF_DASH,    /*< desc="Dashed"                     >*/
  GIMP_GRID_DOUBLE_DASH,    /*< desc="Double dashed"              >*/
  GIMP_GRID_SOLID           /*< desc="Solid"                      >*/
} GimpGridStyle;


/**
 * GimpHueRange:
 * @GIMP_HUE_RANGE_ALL:     All hues
 * @GIMP_HUE_RANGE_RED:     Red hues
 * @GIMP_HUE_RANGE_YELLOW:  Yellow hues
 * @GIMP_HUE_RANGE_GREEN:   Green hues
 * @GIMP_HUE_RANGE_CYAN:    Cyan hues
 * @GIMP_HUE_RANGE_BLUE:    Blue hues
 * @GIMP_HUE_RANGE_MAGENTA: Magenta hues
 *
 * Hue ranges.
 **/
#define GIMP_TYPE_HUE_RANGE (gimp_hue_range_get_type ())

GType gimp_hue_range_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_HUE_RANGE_ALL,
  GIMP_HUE_RANGE_RED,
  GIMP_HUE_RANGE_YELLOW,
  GIMP_HUE_RANGE_GREEN,
  GIMP_HUE_RANGE_CYAN,
  GIMP_HUE_RANGE_BLUE,
  GIMP_HUE_RANGE_MAGENTA
} GimpHueRange;


/**
 * GimpIconType:
 * @GIMP_ICON_TYPE_ICON_NAME:     Icon name
 * @GIMP_ICON_TYPE_INLINE_PIXBUF: Inline pixbuf
 * @GIMP_ICON_TYPE_IMAGE_FILE:    Image file
 *
 * Icon types for plug-ins to register.
 **/
#define GIMP_TYPE_ICON_TYPE (gimp_icon_type_get_type ())

GType gimp_icon_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_ICON_TYPE_ICON_NAME,     /*< desc="Icon name"     >*/
  GIMP_ICON_TYPE_INLINE_PIXBUF, /*< desc="Inline pixbuf" >*/
  GIMP_ICON_TYPE_IMAGE_FILE     /*< desc="Image file"    >*/
} GimpIconType;


/**
 * GimpImageBaseType:
 * @GIMP_RGB:     RGB color
 * @GIMP_GRAY:    Grayscale
 * @GIMP_INDEXED: Indexed color
 *
 * Image color models.
 **/
#define GIMP_TYPE_IMAGE_BASE_TYPE (gimp_image_base_type_get_type ())

GType gimp_image_base_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_RGB,     /*< desc="RGB color"     >*/
  GIMP_GRAY,    /*< desc="Grayscale"     >*/
  GIMP_INDEXED  /*< desc="Indexed color" >*/
} GimpImageBaseType;


/**
 * GimpImageType:
 * @GIMP_RGB_IMAGE:      RGB
 * @GIMP_RGBA_IMAGE:     RGB-alpha
 * @GIMP_GRAY_IMAGE:     Grayscale
 * @GIMP_GRAYA_IMAGE:    Grayscale-alpha
 * @GIMP_INDEXED_IMAGE:  Indexed
 * @GIMP_INDEXEDA_IMAGE: Indexed-alpha
 *
 * Possible drawable types.
 **/
#define GIMP_TYPE_IMAGE_TYPE (gimp_image_type_get_type ())

GType gimp_image_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_RGB_IMAGE,      /*< desc="RGB"             >*/
  GIMP_RGBA_IMAGE,     /*< desc="RGB-alpha"       >*/
  GIMP_GRAY_IMAGE,     /*< desc="Grayscale"       >*/
  GIMP_GRAYA_IMAGE,    /*< desc="Grayscale-alpha" >*/
  GIMP_INDEXED_IMAGE,  /*< desc="Indexed"         >*/
  GIMP_INDEXEDA_IMAGE  /*< desc="Indexed-alpha"   >*/
} GimpImageType;


/**
 * GimpInkBlobType:
 * @GIMP_INK_BLOB_TYPE_CIRCLE:  Circle
 * @GIMP_INK_BLOB_TYPE_SQUARE:  Square
 * @GIMP_INK_BLOB_TYPE_DIAMOND: Diamond
 *
 * Ink tool tips.
 **/
#define GIMP_TYPE_INK_BLOB_TYPE (gimp_ink_blob_type_get_type ())

GType gimp_ink_blob_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_INK_BLOB_TYPE_CIRCLE,  /*< desc="Circle"  >*/
  GIMP_INK_BLOB_TYPE_SQUARE,  /*< desc="Square"  >*/
  GIMP_INK_BLOB_TYPE_DIAMOND  /*< desc="Diamond" >*/
} GimpInkBlobType;


/**
 * GimpInterpolationType:
 * @GIMP_INTERPOLATION_NONE:   None
 * @GIMP_INTERPOLATION_LINEAR: Linear
 * @GIMP_INTERPOLATION_CUBIC:  Cubic
 * @GIMP_INTERPOLATION_NOHALO: NoHalo
 * @GIMP_INTERPOLATION_LOHALO: LoHalo
 *
 * Interpolation types.
 **/
#define GIMP_TYPE_INTERPOLATION_TYPE (gimp_interpolation_type_get_type ())

GType gimp_interpolation_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_INTERPOLATION_NONE,   /*< desc="None"   >*/
  GIMP_INTERPOLATION_LINEAR, /*< desc="Linear" >*/
  GIMP_INTERPOLATION_CUBIC,  /*< desc="Cubic"  >*/
  GIMP_INTERPOLATION_NOHALO, /*< desc="NoHalo" >*/
  GIMP_INTERPOLATION_LOHALO  /*< desc="LoHalo" >*/
} GimpInterpolationType;


/**
 * GimpJoinStyle:
 * @GIMP_JOIN_MITER: Miter
 * @GIMP_JOIN_ROUND: Round
 * @GIMP_JOIN_BEVEL: Bevel
 *
 * Line join styles.
 **/
#define GIMP_TYPE_JOIN_STYLE (gimp_join_style_get_type ())

GType gimp_join_style_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_JOIN_MITER,  /*< desc="Miter" >*/
  GIMP_JOIN_ROUND,  /*< desc="Round" >*/
  GIMP_JOIN_BEVEL   /*< desc="Bevel" >*/
} GimpJoinStyle;


/**
 * GimpMaskApplyMode:
 * @GIMP_MASK_APPLY:   Apply the mask
 * @GIMP_MASK_DISCARD: Discard the mask
 *
 * Layer mask apply modes.
 **/
#define GIMP_TYPE_MASK_APPLY_MODE (gimp_mask_apply_mode_get_type ())

GType gimp_mask_apply_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_MASK_APPLY,
  GIMP_MASK_DISCARD
} GimpMaskApplyMode;


/**
 * GimpMergeType:
 * @GIMP_EXPAND_AS_NECESSARY:  Expanded as necessary
 * @GIMP_CLIP_TO_IMAGE:        Clipped to image
 * @GIMP_CLIP_TO_BOTTOM_LAYER: Clipped to bottom layer
 * @GIMP_FLATTEN_IMAGE:        Flatten
 *
 * Types of merging layers.
 **/
#define GIMP_TYPE_MERGE_TYPE (gimp_merge_type_get_type ())

GType gimp_merge_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_EXPAND_AS_NECESSARY,  /*< desc="Expanded as necessary"  >*/
  GIMP_CLIP_TO_IMAGE,        /*< desc="Clipped to image"        >*/
  GIMP_CLIP_TO_BOTTOM_LAYER, /*< desc="Clipped to bottom layer" >*/
  GIMP_FLATTEN_IMAGE         /*< desc="Flatten"                 >*/
} GimpMergeType;


/**
 * GimpMessageHandlerType:
 * @GIMP_MESSAGE_BOX:   A popup dialog
 * @GIMP_CONSOLE:       The terminal
 * @GIMP_ERROR_CONSOLE: The error console dockable
 *
 * How to present messages.
 **/
#define GIMP_TYPE_MESSAGE_HANDLER_TYPE (gimp_message_handler_type_get_type ())

GType gimp_message_handler_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_MESSAGE_BOX,
  GIMP_CONSOLE,
  GIMP_ERROR_CONSOLE
} GimpMessageHandlerType;


/**
 * GimpOffsetType:
 * @GIMP_OFFSET_BACKGROUND:  Background
 * @GIMP_OFFSET_TRANSPARENT: Transparent
 *
 * Background fill types for the offset operation.
 **/
#define GIMP_TYPE_OFFSET_TYPE (gimp_offset_type_get_type ())

GType gimp_offset_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_OFFSET_BACKGROUND,
  GIMP_OFFSET_TRANSPARENT
} GimpOffsetType;


/**
 * GimpOrientationType:
 * @GIMP_ORIENTATION_HORIZONTAL: Horizontal
 * @GIMP_ORIENTATION_VERTICAL:   Vertical
 * @GIMP_ORIENTATION_UNKNOWN:    Unknown
 *
 * Orientations for verious purposes.
 **/
#define GIMP_TYPE_ORIENTATION_TYPE (gimp_orientation_type_get_type ())

GType gimp_orientation_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_ORIENTATION_HORIZONTAL, /*< desc="Horizontal" >*/
  GIMP_ORIENTATION_VERTICAL,   /*< desc="Vertical"   >*/
  GIMP_ORIENTATION_UNKNOWN     /*< desc="Unknown"    >*/
} GimpOrientationType;


/**
 * GimpPaintApplicationMode:
 * @GIMP_PAINT_CONSTANT:    Constant
 * @GIMP_PAINT_INCREMENTAL: Incremental
 *
 * Paint application modes.
 **/
#define GIMP_TYPE_PAINT_APPLICATION_MODE (gimp_paint_application_mode_get_type ())

GType gimp_paint_application_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_PAINT_CONSTANT,    /*< desc="Constant"    >*/
  GIMP_PAINT_INCREMENTAL  /*< desc="Incremental" >*/
} GimpPaintApplicationMode;


/**
 * GimpPDBArgType:
 * @GIMP_PDB_INT32:       32-bit integer
 * @GIMP_PDB_INT16:       16-bit integer
 * @GIMP_PDB_INT8:        8-bit integer
 * @GIMP_PDB_FLOAT:       Float
 * @GIMP_PDB_STRING:      String
 * @GIMP_PDB_INT32ARRAY:  Array of INT32
 * @GIMP_PDB_INT16ARRAY:  Array of INT16
 * @GIMP_PDB_INT8ARRAY:   Array of INT8
 * @GIMP_PDB_FLOATARRAY:  Array of floats
 * @GIMP_PDB_STRINGARRAY: Array of strings
 * @GIMP_PDB_COLOR:       Color
 * @GIMP_PDB_ITEM:        Item ID
 * @GIMP_PDB_DISPLAY:     Display ID
 * @GIMP_PDB_IMAGE:       Image ID
 * @GIMP_PDB_LAYER:       Layer ID
 * @GIMP_PDB_CHANNEL:     Channel ID
 * @GIMP_PDB_DRAWABLE:    Drawable ID
 * @GIMP_PDB_SELECTION:   Selection ID
 * @GIMP_PDB_COLORARRAY:  Array of colors
 * @GIMP_PDB_VECTORS:     Vectors (psath) ID
 * @GIMP_PDB_PARASITE:    Parasite
 * @GIMP_PDB_STATUS:      Procedure return status
 * @GIMP_PDB_END:         Marker for last enum value
 *
 * Parameter types of the PDB.
 **/
#define GIMP_TYPE_PDB_ARG_TYPE (gimp_pdb_arg_type_get_type ())

GType gimp_pdb_arg_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_PDB_INT32,
  GIMP_PDB_INT16,
  GIMP_PDB_INT8,
  GIMP_PDB_FLOAT,
  GIMP_PDB_STRING,
  GIMP_PDB_INT32ARRAY,
  GIMP_PDB_INT16ARRAY,
  GIMP_PDB_INT8ARRAY,
  GIMP_PDB_FLOATARRAY,
  GIMP_PDB_STRINGARRAY,
  GIMP_PDB_COLOR,
  GIMP_PDB_ITEM,
  GIMP_PDB_DISPLAY,
  GIMP_PDB_IMAGE,
  GIMP_PDB_LAYER,
  GIMP_PDB_CHANNEL,
  GIMP_PDB_DRAWABLE,
  GIMP_PDB_SELECTION,
  GIMP_PDB_COLORARRAY,
  GIMP_PDB_VECTORS,
  GIMP_PDB_PARASITE,
  GIMP_PDB_STATUS,
  GIMP_PDB_END
} GimpPDBArgType;


/**
 * GimpPDBErrorHandler:
 * @GIMP_PDB_ERROR_HANDLER_INTERNAL: Internal
 * @GIMP_PDB_ERROR_HANDLER_PLUGIN:   Plug-In
 *
 * PDB error handlers.
 **/
#define GIMP_TYPE_PDB_ERROR_HANDLER (gimp_pdb_error_handler_get_type ())

GType gimp_pdb_error_handler_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_PDB_ERROR_HANDLER_INTERNAL,
  GIMP_PDB_ERROR_HANDLER_PLUGIN
} GimpPDBErrorHandler;


/**
 * GimpPDBProcType:
 * @GIMP_INTERNAL:  Internal GIMP procedure
 * @GIMP_PLUGIN:    GIMP Plug-In
 * @GIMP_EXTENSION: GIMP Extension
 * @GIMP_TEMPORARY: Temporary Procedure
 *
 * Types of PDB procedures.
 **/
#define GIMP_TYPE_PDB_PROC_TYPE (gimp_pdb_proc_type_get_type ())

GType gimp_pdb_proc_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_INTERNAL,   /*< desc="Internal GIMP procedure" >*/
  GIMP_PLUGIN,     /*< desc="GIMP Plug-In" >*/
  GIMP_EXTENSION,  /*< desc="GIMP Extension" >*/
  GIMP_TEMPORARY   /*< desc="Temporary Procedure" >*/
} GimpPDBProcType;


/**
 * GimpPDBStatusType:
 * @GIMP_PDB_EXECUTION_ERROR: Execution error
 * @GIMP_PDB_CALLING_ERROR:   Calling error
 * @GIMP_PDB_PASS_THROUGH:    Pass through
 * @GIMP_PDB_SUCCESS:         Success
 * @GIMP_PDB_CANCEL:          User cancel
 *
 * Return status of PDB calls.
 **/
#define GIMP_TYPE_PDB_STATUS_TYPE (gimp_pdb_status_type_get_type ())

GType gimp_pdb_status_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_PDB_EXECUTION_ERROR,
  GIMP_PDB_CALLING_ERROR,
  GIMP_PDB_PASS_THROUGH,
  GIMP_PDB_SUCCESS,
  GIMP_PDB_CANCEL
} GimpPDBStatusType;


/**
 * GimpPrecision:
 * @GIMP_PRECISION_U8_LINEAR:     8-bit linear integer
 * @GIMP_PRECISION_U8_GAMMA:      8-bit gamma integer
 * @GIMP_PRECISION_U16_LINEAR:    16-bit linear integer
 * @GIMP_PRECISION_U16_GAMMA:     16-bit gamma integer
 * @GIMP_PRECISION_U32_LINEAR:    32-bit linear integer
 * @GIMP_PRECISION_U32_GAMMA:     32-bit gamma integer
 * @GIMP_PRECISION_HALF_LINEAR:   16-bit linear floating point
 * @GIMP_PRECISION_HALF_GAMMA:    16-bit gamma floating point
 * @GIMP_PRECISION_FLOAT_LINEAR:  32-bit linear floating point
 * @GIMP_PRECISION_FLOAT_GAMMA:   32-bit gamma floating point
 * @GIMP_PRECISION_DOUBLE_LINEAR: 64-bit linear floating point
 * @GIMP_PRECISION_DOUBLE_GAMMA:  64-bit gamma floating point
 *
 * Precisions for pixel encoding.
 *
 * Since: 2.10
 **/
#define GIMP_TYPE_PRECISION (gimp_precision_get_type ())

GType gimp_precision_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_PRECISION_U8_LINEAR     = 100, /*< desc="8-bit linear integer"         >*/
  GIMP_PRECISION_U8_GAMMA      = 150, /*< desc="8-bit gamma integer"          >*/
  GIMP_PRECISION_U16_LINEAR    = 200, /*< desc="16-bit linear integer"        >*/
  GIMP_PRECISION_U16_GAMMA     = 250, /*< desc="16-bit gamma integer"         >*/
  GIMP_PRECISION_U32_LINEAR    = 300, /*< desc="32-bit linear integer"        >*/
  GIMP_PRECISION_U32_GAMMA     = 350, /*< desc="32-bit gamma integer"         >*/
  GIMP_PRECISION_HALF_LINEAR   = 500, /*< desc="16-bit linear floating point" >*/
  GIMP_PRECISION_HALF_GAMMA    = 550, /*< desc="16-bit gamma floating point"  >*/
  GIMP_PRECISION_FLOAT_LINEAR  = 600, /*< desc="32-bit linear floating point" >*/
  GIMP_PRECISION_FLOAT_GAMMA   = 650, /*< desc="32-bit gamma floating point"  >*/
  GIMP_PRECISION_DOUBLE_LINEAR = 700, /*< desc="64-bit linear floating point" >*/
  GIMP_PRECISION_DOUBLE_GAMMA  = 750  /*< desc="64-bit gamma floating point"  >*/
} GimpPrecision;


/**
 * GimpProgressCommand:
 * @GIMP_PROGRESS_COMMAND_START:      Start a progress
 * @GIMP_PROGRESS_COMMAND_END:        End the progress
 * @GIMP_PROGRESS_COMMAND_SET_TEXT:   Set the text
 * @GIMP_PROGRESS_COMMAND_SET_VALUE:  Set the percentage
 * @GIMP_PROGRESS_COMMAND_PULSE:      Pulse the progress
 * @GIMP_PROGRESS_COMMAND_GET_WINDOW: Get the window where the progress is shown
 *
 * Commands for the progress API.
 **/
#define GIMP_TYPE_PROGRESS_COMMAND (gimp_progress_command_get_type ())

GType gimp_progress_command_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_PROGRESS_COMMAND_START,
  GIMP_PROGRESS_COMMAND_END,
  GIMP_PROGRESS_COMMAND_SET_TEXT,
  GIMP_PROGRESS_COMMAND_SET_VALUE,
  GIMP_PROGRESS_COMMAND_PULSE,
  GIMP_PROGRESS_COMMAND_GET_WINDOW
} GimpProgressCommand;


/**
 * GimpRepeatMode:
 * @GIMP_REPEAT_NONE:       None (extend)
 * @GIMP_REPEAT_SAWTOOTH:   Sawtooth wave
 * @GIMP_REPEAT_TRIANGULAR: Triangular wave
 * @GIMP_REPEAT_TRUNCATE:   Truncate
 *
 * Repeat modes for example for gradients.
 **/
#define GIMP_TYPE_REPEAT_MODE (gimp_repeat_mode_get_type ())

GType gimp_repeat_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_REPEAT_NONE,       /*< desc="None (extend)"   >*/
  GIMP_REPEAT_SAWTOOTH,   /*< desc="Sawtooth wave"   >*/
  GIMP_REPEAT_TRIANGULAR, /*< desc="Triangular wave" >*/
  GIMP_REPEAT_TRUNCATE    /*< desc="Truncate"        >*/
} GimpRepeatMode;


/**
 * GimpRotationType:
 * @GIMP_ROTATE_90:  90 degrees
 * @GIMP_ROTATE_180: 180 degrees
 * @GIMP_ROTATE_270: 270 degrees
 *
 * Types of simple rotations.
 **/
#define GIMP_TYPE_ROTATION_TYPE (gimp_rotation_type_get_type ())

GType gimp_rotation_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_ROTATE_90,
  GIMP_ROTATE_180,
  GIMP_ROTATE_270
} GimpRotationType;


/**
 * GimpRunMode:
 * @GIMP_RUN_INTERACTIVE:    Run interactively
 * @GIMP_RUN_NONINTERACTIVE: Run non-interactively
 * @GIMP_RUN_WITH_LAST_VALS: Run with last used values
 *
 * Run modes for plug-ins.
 **/
#define GIMP_TYPE_RUN_MODE (gimp_run_mode_get_type ())

GType gimp_run_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_RUN_INTERACTIVE,     /*< desc="Run interactively"         >*/
  GIMP_RUN_NONINTERACTIVE,  /*< desc="Run non-interactively"     >*/
  GIMP_RUN_WITH_LAST_VALS   /*< desc="Run with last used values" >*/
} GimpRunMode;


/**
 * GimpSelectCriterion:
 * @GIMP_SELECT_CRITERION_COMPOSITE: Composite
 * @GIMP_SELECT_CRITERION_R:         Red
 * @GIMP_SELECT_CRITERION_G:         Green
 * @GIMP_SELECT_CRITERION_B:         Blue
 * @GIMP_SELECT_CRITERION_H:         Hue (HSV)
 * @GIMP_SELECT_CRITERION_S:         Saturation (HSV)
 * @GIMP_SELECT_CRITERION_V:         Value (HSV)
 * @GIMP_SELECT_CRITERION_A:         Alpha
 * @GIMP_SELECT_CRITERION_LCH_L:     Lightness (LCH)
 * @GIMP_SELECT_CRITERION_LCH_C:     Chroma (LCH)
 * @GIMP_SELECT_CRITERION_LCH_H:     Hue (LCH)
 *
 * Criterions for color similarity.
 **/
#define GIMP_TYPE_SELECT_CRITERION (gimp_select_criterion_get_type ())

GType gimp_select_criterion_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_SELECT_CRITERION_COMPOSITE,  /*< desc="Composite"        >*/
  GIMP_SELECT_CRITERION_R,          /*< desc="Red"              >*/
  GIMP_SELECT_CRITERION_G,          /*< desc="Green"            >*/
  GIMP_SELECT_CRITERION_B,          /*< desc="Blue"             >*/
  GIMP_SELECT_CRITERION_H,          /*< desc="Hue (HSV)"        >*/
  GIMP_SELECT_CRITERION_S,          /*< desc="Saturation (HSV)" >*/
  GIMP_SELECT_CRITERION_V,          /*< desc="Value (HSV)"      >*/
  GIMP_SELECT_CRITERION_A,          /*< desc="Alpha"            >*/
  GIMP_SELECT_CRITERION_LCH_L,      /*< desc="Lightness (LCH)"  >*/
  GIMP_SELECT_CRITERION_LCH_C,      /*< desc="Chroma (LCH)"     >*/
  GIMP_SELECT_CRITERION_LCH_H,      /*< desc="Hue (LCH)"        >*/
} GimpSelectCriterion;


/**
 * GimpSizeType:
 * @GIMP_PIXELS: Pixels
 * @GIMP_POINTS: Points
 *
 * Size types for the old-style text API.
 **/
#define GIMP_TYPE_SIZE_TYPE (gimp_size_type_get_type ())

GType gimp_size_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_PIXELS,  /*< desc="Pixels" >*/
  GIMP_POINTS   /*< desc="Points" >*/
} GimpSizeType;


/**
 * GimpStackTraceMode:
 * @GIMP_STACK_TRACE_NEVER:  Never
 * @GIMP_STACK_TRACE_QUERY:  Ask each time
 * @GIMP_STACK_TRACE_ALWAYS: Always
 *
 * When to generate stack traces in case of an error.
 **/
#define GIMP_TYPE_STACK_TRACE_MODE (gimp_stack_trace_mode_get_type ())

GType gimp_stack_trace_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_STACK_TRACE_NEVER,
  GIMP_STACK_TRACE_QUERY,
  GIMP_STACK_TRACE_ALWAYS
} GimpStackTraceMode;


/**
 * GimpStrokeMethod:
 * @GIMP_STROKE_LINE:         Stroke line
 * @GIMP_STROKE_PAINT_METHOD: Stroke with a paint tool
 *
 * Methods of stroking selections and paths.
 **/
#define GIMP_TYPE_STROKE_METHOD (gimp_stroke_method_get_type ())

GType gimp_stroke_method_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_STROKE_LINE,         /*< desc="Stroke line"              >*/
  GIMP_STROKE_PAINT_METHOD  /*< desc="Stroke with a paint tool" >*/
} GimpStrokeMethod;


/**
 * GimpTextDirection:
 * @GIMP_TEXT_DIRECTION_LTR: From left to right
 * @GIMP_TEXT_DIRECTION_RTL: From right to left
 *
 * Text directions.
 **/
#define GIMP_TYPE_TEXT_DIRECTION (gimp_text_direction_get_type ())

GType gimp_text_direction_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_TEXT_DIRECTION_LTR,   /*< desc="From left to right" >*/
  GIMP_TEXT_DIRECTION_RTL    /*< desc="From right to left" >*/
} GimpTextDirection;


/**
 * GimpTextHintStyle:
 * @GIMP_TEXT_HINT_STYLE_NONE:   None
 * @GIMP_TEXT_HINT_STYLE_SLIGHT: Slight
 * @GIMP_TEXT_HINT_STYLE_MEDIUM: Medium
 * @GIMP_TEXT_HINT_STYLE_FULL:   Full
 *
 * Text hint strengths.
 **/
#define GIMP_TYPE_TEXT_HINT_STYLE (gimp_text_hint_style_get_type ())

GType gimp_text_hint_style_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_TEXT_HINT_STYLE_NONE,     /*< desc="None"   >*/
  GIMP_TEXT_HINT_STYLE_SLIGHT,   /*< desc="Slight" >*/
  GIMP_TEXT_HINT_STYLE_MEDIUM,   /*< desc="Medium" >*/
  GIMP_TEXT_HINT_STYLE_FULL      /*< desc="Full"   >*/
} GimpTextHintStyle;


/**
 * GimpTextJustification:
 * @GIMP_TEXT_JUSTIFY_LEFT:   Left justified
 * @GIMP_TEXT_JUSTIFY_RIGHT:  Right justified
 * @GIMP_TEXT_JUSTIFY_CENTER: Centered
 * @GIMP_TEXT_JUSTIFY_FILL:   Filled
 *
 * Text justifications.
 **/
#define GIMP_TYPE_TEXT_JUSTIFICATION (gimp_text_justification_get_type ())

GType gimp_text_justification_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_TEXT_JUSTIFY_LEFT,    /*< desc="Left justified"  >*/
  GIMP_TEXT_JUSTIFY_RIGHT,   /*< desc="Right justified" >*/
  GIMP_TEXT_JUSTIFY_CENTER,  /*< desc="Centered"        >*/
  GIMP_TEXT_JUSTIFY_FILL     /*< desc="Filled"          >*/
} GimpTextJustification;


/**
 * GimpTransferMode:
 * @GIMP_TRANSFER_SHADOWS:    Shadows
 * @GIMP_TRANSFER_MIDTONES:   Midtones
 * @GIMP_TRANSFER_HIGHLIGHTS: Highlights
 *
 * For choosing which brightness ranges to transform.
 **/
#define GIMP_TYPE_TRANSFER_MODE (gimp_transfer_mode_get_type ())

GType gimp_transfer_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_TRANSFER_SHADOWS,     /*< desc="Shadows"    >*/
  GIMP_TRANSFER_MIDTONES,    /*< desc="Midtones"   >*/
  GIMP_TRANSFER_HIGHLIGHTS   /*< desc="Highlights" >*/
} GimpTransferMode;


/**
 * GimpTransformDirection:
 * @GIMP_TRANSFORM_FORWARD:  Normal (Forward)
 * @GIMP_TRANSFORM_BACKWARD: Corrective (Backward)
 *
 * Transform directions.
 **/
#define GIMP_TYPE_TRANSFORM_DIRECTION (gimp_transform_direction_get_type ())

GType gimp_transform_direction_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_TRANSFORM_FORWARD,   /*< desc="Normal (Forward)" >*/
  GIMP_TRANSFORM_BACKWARD   /*< desc="Corrective (Backward)" >*/
} GimpTransformDirection;


/**
 * GimpTransformResize:
 * @GIMP_TRANSFORM_RESIZE_ADJUST:           Adjust
 * @GIMP_TRANSFORM_RESIZE_CLIP:             Clip
 * @GIMP_TRANSFORM_RESIZE_CROP:             Crop to result
 * @GIMP_TRANSFORM_RESIZE_CROP_WITH_ASPECT: Crop with aspect
 *
 * Ways of clipping the result when transforming drawables.
 **/
#define GIMP_TYPE_TRANSFORM_RESIZE (gimp_transform_resize_get_type ())

GType gimp_transform_resize_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_TRANSFORM_RESIZE_ADJUST,           /*< desc="Adjust"           >*/
  GIMP_TRANSFORM_RESIZE_CLIP,             /*< desc="Clip"             >*/
  GIMP_TRANSFORM_RESIZE_CROP,             /*< desc="Crop to result"   >*/
  GIMP_TRANSFORM_RESIZE_CROP_WITH_ASPECT  /*< desc="Crop with aspect" >*/
} GimpTransformResize;


/**
 * GimpUnit:
 * @GIMP_UNIT_PIXEL:   Pixels
 * @GIMP_UNIT_INCH:    Inches
 * @GIMP_UNIT_MM:      Millimeters
 * @GIMP_UNIT_POINT:   Points
 * @GIMP_UNIT_PICA:    Picas
 * @GIMP_UNIT_END:     Marker for end-of-builtin-units
 * @GIMP_UNIT_PERCENT: Pseudo-unit percent
 *
 * Units used for dimensions in images.
 **/
typedef enum /*< skip >*/
{
  GIMP_UNIT_PIXEL   = 0,

  GIMP_UNIT_INCH    = 1,
  GIMP_UNIT_MM      = 2,
  GIMP_UNIT_POINT   = 3,
  GIMP_UNIT_PICA    = 4,

  GIMP_UNIT_END     = 5,

  GIMP_UNIT_PERCENT = 65536 /*< pdb-skip >*/
} GimpUnit;


/**
 * GimpVectorsStrokeType:
 * @GIMP_VECTORS_STROKE_TYPE_BEZIER: A bezier stroke
 *
 * Possible type of strokes in vectors objects.
 **/
#define GIMP_TYPE_VECTORS_STROKE_TYPE (gimp_vectors_stroke_type_get_type ())

GType gimp_vectors_stroke_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_VECTORS_STROKE_TYPE_BEZIER
} GimpVectorsStrokeType;

G_END_DECLS

#endif  /* __GIMP_BASE_ENUMS_H__ */
