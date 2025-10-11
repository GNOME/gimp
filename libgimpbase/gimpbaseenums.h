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
 * <https://www.gnu.org/licenses/>.
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
 * @GIMP_CHECK_TYPE_CUSTOM_CHECKS: Custom checks
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
  GIMP_CHECK_TYPE_BLACK_ONLY   = 5,  /*< desc="Black only"      >*/
  GIMP_CHECK_TYPE_CUSTOM_CHECKS = 6  /*< desc="Custom checks"   >*/
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
  GIMP_CONVERT_PALETTE_GENERATE, /*< desc="_Generate optimum palette"          >*/
  GIMP_CONVERT_PALETTE_WEB,      /*< desc="Use _web-optimized palette"         >*/
  GIMP_CONVERT_PALETTE_MONO,     /*< desc="Use _black and white (1-bit) palette" >*/
  GIMP_CONVERT_PALETTE_CUSTOM    /*< desc="Use custom _palette"                >*/
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
 * @GIMP_FILL_FOREGROUND:         Foreground color
 * @GIMP_FILL_BACKGROUND:         Background color
 * @GIMP_FILL_CIELAB_MIDDLE_GRAY: Middle Gray (CIELAB)
 * @GIMP_FILL_WHITE:              White
 * @GIMP_FILL_TRANSPARENT:        Transparency
 * @GIMP_FILL_PATTERN:            Pattern
 *
 * Types of filling.
 **/
#define GIMP_TYPE_FILL_TYPE (gimp_fill_type_get_type ())

GType gimp_fill_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_FILL_FOREGROUND,         /*< desc="Foreground color" >*/
  GIMP_FILL_BACKGROUND,         /*< desc="Background color" >*/
  GIMP_FILL_CIELAB_MIDDLE_GRAY, /*< desc="Middle Gray (CIELAB)" >*/
  GIMP_FILL_WHITE,              /*< desc="White"            >*/
  GIMP_FILL_TRANSPARENT,        /*< desc="Transparency"     >*/
  GIMP_FILL_PATTERN,            /*< desc="Pattern"          >*/
} GimpFillType;


/**
 * GimpForegroundExtractMode:
 * @GIMP_FOREGROUND_EXTRACT_MATTING: Matting (Since 2.10)
 *
 * Foreground extract engines.
 **/
#define GIMP_TYPE_FOREGROUND_EXTRACT_MODE (gimp_foreground_extract_mode_get_type ())

GType gimp_foreground_extract_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
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
 * @GIMP_GRADIENT_SEGMENT_STEP:              Step
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
  GIMP_GRADIENT_SEGMENT_SPHERE_DECREASING,  /*< desc="Spherical (decreasing)", abbrev="Spherical (dec)" >*/
  GIMP_GRADIENT_SEGMENT_STEP                /*< desc="Step"                                             >*/
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
 * @GIMP_ICON_TYPE_ICON_NAME:  Icon name
 * @GIMP_ICON_TYPE_PIXBUF:     Inline pixbuf
 * @GIMP_ICON_TYPE_IMAGE_FILE: Image file
 *
 * Icon types for plug-ins to register.
 **/
#define GIMP_TYPE_ICON_TYPE (gimp_icon_type_get_type ())

GType gimp_icon_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_ICON_TYPE_ICON_NAME,  /*< desc="Icon name"  >*/
  GIMP_ICON_TYPE_PIXBUF,     /*< desc="Pixbuf"     >*/
  GIMP_ICON_TYPE_IMAGE_FILE  /*< desc="Image file" >*/
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
 * @GIMP_OFFSET_COLOR:       Color
 * @GIMP_OFFSET_TRANSPARENT: Transparent
 * @GIMP_OFFSET_WRAP_AROUND: Wrap image around
 *
 * Background fill types for the offset operation.
 **/
#define GIMP_TYPE_OFFSET_TYPE (gimp_offset_type_get_type ())

GType gimp_offset_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_OFFSET_COLOR,
  GIMP_OFFSET_TRANSPARENT,
  GIMP_OFFSET_WRAP_AROUND
} GimpOffsetType;


/**
 * GimpOrientationType:
 * @GIMP_ORIENTATION_HORIZONTAL: Horizontal
 * @GIMP_ORIENTATION_VERTICAL:   Vertical
 * @GIMP_ORIENTATION_UNKNOWN:    Unknown
 *
 * Orientations for various purposes.
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
 * @GIMP_PDB_PROC_TYPE_INTERNAL:   Internal GIMP procedure
 * @GIMP_PDB_PROC_TYPE_PLUGIN:     GIMP Plug-In
 * @GIMP_PDB_PROC_TYPE_PERSISTENT: GIMP Persistent Plug-in
 * @GIMP_PDB_PROC_TYPE_TEMPORARY:  Temporary Procedure
 *
 * Types of PDB procedures.
 **/
#define GIMP_TYPE_PDB_PROC_TYPE (gimp_pdb_proc_type_get_type ())

GType gimp_pdb_proc_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_PDB_PROC_TYPE_INTERNAL,   /*< desc="Internal GIMP procedure" >*/
  GIMP_PDB_PROC_TYPE_PLUGIN,     /*< desc="GIMP Plug-In" >*/
  GIMP_PDB_PROC_TYPE_PERSISTENT, /*< desc="GIMP Persistent Plug-In" >*/
  GIMP_PDB_PROC_TYPE_TEMPORARY   /*< desc="Temporary Procedure" >*/
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
 * @GIMP_PRECISION_U8_LINEAR:         8-bit linear integer
 * @GIMP_PRECISION_U8_NON_LINEAR:     8-bit non-linear integer
 * @GIMP_PRECISION_U8_PERCEPTUAL:     8-bit perceptual integer
 * @GIMP_PRECISION_U16_LINEAR:        16-bit linear integer
 * @GIMP_PRECISION_U16_NON_LINEAR:    16-bit non-linear integer
 * @GIMP_PRECISION_U16_PERCEPTUAL:    16-bit perceptual integer
 * @GIMP_PRECISION_U32_LINEAR:        32-bit linear integer
 * @GIMP_PRECISION_U32_NON_LINEAR:    32-bit non-linear integer
 * @GIMP_PRECISION_U32_PERCEPTUAL:    32-bit perceptual integer
 * @GIMP_PRECISION_HALF_LINEAR:       16-bit linear floating point
 * @GIMP_PRECISION_HALF_NON_LINEAR:   16-bit non-linear floating point
 * @GIMP_PRECISION_HALF_PERCEPTUAL:   16-bit perceptual floating point
 * @GIMP_PRECISION_FLOAT_LINEAR:      32-bit linear floating point
 * @GIMP_PRECISION_FLOAT_NON_LINEAR:  32-bit non-linear floating point
 * @GIMP_PRECISION_FLOAT_PERCEPTUAL:  32-bit perceptual floating point
 * @GIMP_PRECISION_DOUBLE_LINEAR:     64-bit linear floating point
 * @GIMP_PRECISION_DOUBLE_NON_LINEAR: 64-bit non-linear floating point
 * @GIMP_PRECISION_DOUBLE_PERCEPTUAL: 64-bit perceptual floating point
 *
 * Precisions for pixel encoding.
 *
 * Since: 2.10
 **/
#define GIMP_TYPE_PRECISION (gimp_precision_get_type ())

GType gimp_precision_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_PRECISION_U8_LINEAR         = 100, /*< desc="8-bit linear integer"         >*/
  GIMP_PRECISION_U8_NON_LINEAR     = 150, /*< desc="8-bit non-linear integer"          >*/
  GIMP_PRECISION_U8_PERCEPTUAL     = 175, /*< desc="8-bit perceptual integer"          >*/
  GIMP_PRECISION_U16_LINEAR        = 200, /*< desc="16-bit linear integer"        >*/
  GIMP_PRECISION_U16_NON_LINEAR    = 250, /*< desc="16-bit non-linear integer"         >*/
  GIMP_PRECISION_U16_PERCEPTUAL    = 275, /*< desc="16-bit perceptual integer"         >*/
  GIMP_PRECISION_U32_LINEAR        = 300, /*< desc="32-bit linear integer"        >*/
  GIMP_PRECISION_U32_NON_LINEAR    = 350, /*< desc="32-bit non-linear integer"         >*/
  GIMP_PRECISION_U32_PERCEPTUAL    = 375, /*< desc="32-bit perceptual integer"         >*/
  GIMP_PRECISION_HALF_LINEAR       = 500, /*< desc="16-bit linear floating point" >*/
  GIMP_PRECISION_HALF_NON_LINEAR   = 550, /*< desc="16-bit non-linear floating point"  >*/
  GIMP_PRECISION_HALF_PERCEPTUAL   = 575, /*< desc="16-bit perceptual floating point"  >*/
  GIMP_PRECISION_FLOAT_LINEAR      = 600, /*< desc="32-bit linear floating point" >*/
  GIMP_PRECISION_FLOAT_NON_LINEAR  = 650, /*< desc="32-bit non-linear floating point"  >*/
  GIMP_PRECISION_FLOAT_PERCEPTUAL  = 675, /*< desc="32-bit perceptual floating point"  >*/
  GIMP_PRECISION_DOUBLE_LINEAR     = 700, /*< desc="64-bit linear floating point" >*/
  GIMP_PRECISION_DOUBLE_NON_LINEAR = 750, /*< desc="64-bit non-linear floating point"  >*/
  GIMP_PRECISION_DOUBLE_PERCEPTUAL = 775, /*< desc="64-bit perceptual floating point"  >*/
} GimpPrecision;


/**
 * GimpProcedureSensitivityMask:
 * @GIMP_PROCEDURE_SENSITIVE_DRAWABLE:     Handles image with one selected drawable.
 * @GIMP_PROCEDURE_SENSITIVE_DRAWABLES:    Handles image with several selected drawables.
 * @GIMP_PROCEDURE_SENSITIVE_NO_DRAWABLES: Handles image with no selected drawables.
 * @GIMP_PROCEDURE_SENSITIVE_NO_IMAGE:     Handles no image.
 *
 * The cases when a #GimpProcedure should be shown as sensitive.
 **/
#define GIMP_TYPE_PROCEDURE_SENSITIVITY_MASK (gimp_procedure_sensitivity_mask_get_type ())

GType gimp_procedure_sensitivity_mask_get_type (void) G_GNUC_CONST;
typedef enum  /*< pdb-skip >*/
{
  GIMP_PROCEDURE_SENSITIVE_DRAWABLE      = 1 << 0,
  GIMP_PROCEDURE_SENSITIVE_DRAWABLES     = 1 << 2,
  GIMP_PROCEDURE_SENSITIVE_NO_DRAWABLES  = 1 << 3,
  GIMP_PROCEDURE_SENSITIVE_NO_IMAGE      = 1 << 4,
  GIMP_PROCEDURE_SENSITIVE_ALWAYS        = G_MAXINT
} GimpProcedureSensitivityMask;


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
 * @GIMP_REPEAT_TRUNCATE:   None (truncate)
 * @GIMP_REPEAT_SAWTOOTH:   Sawtooth wave
 * @GIMP_REPEAT_TRIANGULAR: Triangular wave
 *
 * Repeat modes for example for gradients.
 **/
#define GIMP_TYPE_REPEAT_MODE (gimp_repeat_mode_get_type ())

GType gimp_repeat_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_REPEAT_NONE,       /*< desc="None (extend)"   >*/
  GIMP_REPEAT_TRUNCATE,   /*< desc="None (truncate)" >*/
  GIMP_REPEAT_SAWTOOTH,   /*< desc="Sawtooth wave"   >*/
  GIMP_REPEAT_TRIANGULAR  /*< desc="Triangular wave" >*/
} GimpRepeatMode;


/**
 * GimpRotationType:
 * @GIMP_ROTATE_DEGREES90:  90 degrees
 * @GIMP_ROTATE_DEGREES180: 180 degrees
 * @GIMP_ROTATE_DEGREES270: 270 degrees
 *
 * Types of simple rotations.
 **/
#define GIMP_TYPE_ROTATION_TYPE (gimp_rotation_type_get_type ())

GType gimp_rotation_type_get_type (void) G_GNUC_CONST;

/* Due to GObject Introspection we can't have the last part of an identifier
 * start with a digit, since that part will be used in Python as the
 * identifier, and Python doesn't allow that to start with a digit. */
typedef enum
{
  GIMP_ROTATE_DEGREES90,
  GIMP_ROTATE_DEGREES180,
  GIMP_ROTATE_DEGREES270
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
 * @GIMP_SELECT_CRITERION_COMPOSITE:      Composite
 * @GIMP_SELECT_CRITERION_RGB_RED:        Red
 * @GIMP_SELECT_CRITERION_RGB_GREEN:      Green
 * @GIMP_SELECT_CRITERION_RGB_BLUE:       Blue
 * @GIMP_SELECT_CRITERION_HSV_HUE:        HSV Hue
 * @GIMP_SELECT_CRITERION_HSV_SATURATION: HSV Saturation
 * @GIMP_SELECT_CRITERION_HSV_VALUE:      HSV Value
 * @GIMP_SELECT_CRITERION_LCH_LIGHTNESS:  LCh Lightness
 * @GIMP_SELECT_CRITERION_LCH_CHROMA:     LCh Chroma
 * @GIMP_SELECT_CRITERION_LCH_HUE:        LCh Hue
 * @GIMP_SELECT_CRITERION_ALPHA:          Alpha
 *
 * Criterions for color similarity.
 **/
#define GIMP_TYPE_SELECT_CRITERION (gimp_select_criterion_get_type ())

GType gimp_select_criterion_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_SELECT_CRITERION_COMPOSITE,      /*< desc="Composite"      >*/
  GIMP_SELECT_CRITERION_RGB_RED,        /*< desc="Red"            >*/
  GIMP_SELECT_CRITERION_RGB_GREEN,      /*< desc="Green"          >*/
  GIMP_SELECT_CRITERION_RGB_BLUE,       /*< desc="Blue"           >*/
  GIMP_SELECT_CRITERION_HSV_HUE,        /*< desc="HSV Hue"        >*/
  GIMP_SELECT_CRITERION_HSV_SATURATION, /*< desc="HSV Saturation" >*/
  GIMP_SELECT_CRITERION_HSV_VALUE,      /*< desc="HSV Value"      >*/
  GIMP_SELECT_CRITERION_LCH_LIGHTNESS,  /*< desc="LCh Lightness"  >*/
  GIMP_SELECT_CRITERION_LCH_CHROMA,     /*< desc="LCh Chroma"     >*/
  GIMP_SELECT_CRITERION_LCH_HUE,        /*< desc="LCh Hue"        >*/
  GIMP_SELECT_CRITERION_ALPHA,          /*< desc="Alpha"          >*/
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
 * @GIMP_TEXT_DIRECTION_TTB_RTL: Characters are from top to bottom, Lines are from right to left
 * @GIMP_TEXT_DIRECTION_TTB_RTL_UPRIGHT: Upright characters are from top to bottom, Lines are from right to left
 * @GIMP_TEXT_DIRECTION_TTB_LTR: Characters are from top to bottom, Lines are from left to right
 * @GIMP_TEXT_DIRECTION_TTB_LTR_UPRIGHT: Upright characters are from top to bottom, Lines are from left to right
 *
 * Text directions.
 **/
#define GIMP_TYPE_TEXT_DIRECTION (gimp_text_direction_get_type ())

GType gimp_text_direction_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_TEXT_DIRECTION_LTR,              /*< desc="From left to right"                                     >*/
  GIMP_TEXT_DIRECTION_RTL,              /*< desc="From right to left"                                     >*/
  GIMP_TEXT_DIRECTION_TTB_RTL,          /*< desc="Vertical, right to left (mixed orientation)"  >*/
  GIMP_TEXT_DIRECTION_TTB_RTL_UPRIGHT,  /*< desc="Vertical, right to left (upright orientation)" >*/
  GIMP_TEXT_DIRECTION_TTB_LTR,          /*< desc="Vertical, left to right (mixed orientation)"  >*/
  GIMP_TEXT_DIRECTION_TTB_LTR_UPRIGHT   /*< desc="Vertical, left to right (upright orientation)" >*/
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
 * GimpTextOutline:
 * @GIMP_TEXT_OUTLINE_NONE:        Filled
 * @GIMP_TEXT_OUTLINE_STROKE_ONLY: Outlined
 * @GIMP_TEXT_OUTLINE_STROKE_FILL: Outlined and filled
 *
 * Settings for text stroke and fill.
 **/
#define GIMP_TYPE_TEXT_OUTLINE (gimp_text_outline_get_type ())

GType gimp_text_outline_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_TEXT_OUTLINE_NONE,        /*< desc="Filled"              >*/
  GIMP_TEXT_OUTLINE_STROKE_ONLY, /*< desc="Outlined"            >*/
  GIMP_TEXT_OUTLINE_STROKE_FILL  /*< desc="Outlined and filled" >*/
} GimpTextOutline;

/**
 * GimpTextOutlineDirection:
 * @GIMP_TEXT_OUTLINE_DIRECTION_OUTER:    Outer
 * @GIMP_TEXT_OUTLINE_DIRECTION_INNER:    Inner
 * @GIMP_TEXT_OUTLINE_DIRECTION_CENTERED: Centered
 *
 * Options for how the text outline's stroke is drawn.
 **/
#define GIMP_TYPE_TEXT_OUTLINE_DIRECTION (gimp_text_outline_direction_get_type ())

GType gimp_text_outline_direction_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_TEXT_OUTLINE_DIRECTION_OUTER,   /*< desc="Outer"    >*/
  GIMP_TEXT_OUTLINE_DIRECTION_INNER,   /*< desc="Inner"    >*/
  GIMP_TEXT_OUTLINE_DIRECTION_CENTERED /*< desc="Centered" >*/
} GimpTextOutlineDirection;

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
 * GimpUnitID:
 * @GIMP_UNIT_PIXEL:   Pixels
 * @GIMP_UNIT_INCH:    Inches
 * @GIMP_UNIT_MM:      Millimeters
 * @GIMP_UNIT_POINT:   Points
 * @GIMP_UNIT_PICA:    Picas
 * @GIMP_UNIT_END:     Marker for end-of-builtin-units
 * @GIMP_UNIT_PERCENT: Pseudo-unit percent
 *
 * Integer IDs of built-in units used for dimensions in images. These
 * IDs are meant to stay stable but user-created units IDs may change
 * from one session to another.
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
} GimpUnitID;


/**
 * GimpPathStrokeType:
 * @GIMP_PATH_STROKE_TYPE_BEZIER: A bezier stroke
 *
 * Possible type of strokes in path objects.
 **/
#define GIMP_TYPE_PATH_STROKE_TYPE (gimp_path_stroke_type_get_type ())

GType gimp_path_stroke_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_PATH_STROKE_TYPE_BEZIER
} GimpPathStrokeType;


/**
 * GimpExportCapabilities:
 * @GIMP_EXPORT_CAN_HANDLE_RGB:                 Handles RGB images
 * @GIMP_EXPORT_CAN_HANDLE_GRAY:                Handles grayscale images
 * @GIMP_EXPORT_CAN_HANDLE_INDEXED:             Handles indexed images
 * @GIMP_EXPORT_CAN_HANDLE_BITMAP:              Handles two-color indexed images
 * @GIMP_EXPORT_CAN_HANDLE_ALPHA:               Handles alpha channels
 * @GIMP_EXPORT_CAN_HANDLE_LAYERS:              Handles layers
 * @GIMP_EXPORT_CAN_HANDLE_LAYERS_AS_ANIMATION: Handles animation of layers
 * @GIMP_EXPORT_CAN_HANDLE_LAYER_EFFECTS:       Handles layer effects
 * @GIMP_EXPORT_CAN_HANDLE_LAYER_MASKS:         Handles layer masks
 * @GIMP_EXPORT_NEEDS_ALPHA:                    Needs alpha channels
 * @GIMP_EXPORT_NEEDS_CROP:                     Needs to crop content to image bounds
 *
 * The types of images and layers an export procedure can handle
 **/
#define GIMP_TYPE_EXPORT_CAPABILITIES (gimp_export_capabilities_get_type ())

GType gimp_export_capabilities_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_EXPORT_CAN_HANDLE_RGB                 = 1 << 0,
  GIMP_EXPORT_CAN_HANDLE_GRAY                = 1 << 1,
  GIMP_EXPORT_CAN_HANDLE_INDEXED             = 1 << 2,
  GIMP_EXPORT_CAN_HANDLE_BITMAP              = 1 << 3,
  GIMP_EXPORT_CAN_HANDLE_ALPHA               = 1 << 4,
  GIMP_EXPORT_CAN_HANDLE_LAYERS              = 1 << 5,
  GIMP_EXPORT_CAN_HANDLE_LAYERS_AS_ANIMATION = 1 << 6,
  GIMP_EXPORT_CAN_HANDLE_LAYER_MASKS         = 1 << 7,
  GIMP_EXPORT_CAN_HANDLE_LAYER_EFFECTS       = 1 << 8,
  GIMP_EXPORT_NEEDS_ALPHA                    = 1 << 9,
  GIMP_EXPORT_NEEDS_CROP                     = 1 << 10
} GimpExportCapabilities;


/**
 * GimpFileChooserAction:
 * @GIMP_FILE_CHOOSER_ACTION_ANY:           No restriction.
 * @GIMP_FILE_CHOOSER_ACTION_OPEN:          Opens an existing file.
 * @GIMP_FILE_CHOOSER_ACTION_SAVE:          Saves a file (over a new file or an existing one.
 * @GIMP_FILE_CHOOSER_ACTION_SELECT_FOLDER: Picks an existing folder.
 * @GIMP_FILE_CHOOSER_ACTION_CREATE_FOLDER: Picks an existing or new folder.
 *
 * A type of file to choose from when actions are expected to choose a
 * file. This is basically a mapping to %GtkFileChooserAction except for
 * [enum@Gimp.FileChooserAction.ANY] which should not be used for any
 * GUI functions since we can't know what you are looking for.
 **/
#define GIMP_TYPE_FILE_CHOOSER_ACTION (gimp_file_chooser_action_get_type ())

GType gimp_file_chooser_action_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_FILE_CHOOSER_ACTION_ANY           = -1,
  GIMP_FILE_CHOOSER_ACTION_OPEN          = 0,
  GIMP_FILE_CHOOSER_ACTION_SAVE          = 1,
  GIMP_FILE_CHOOSER_ACTION_SELECT_FOLDER = 2,
  GIMP_FILE_CHOOSER_ACTION_CREATE_FOLDER = 3,
} GimpFileChooserAction;


G_END_DECLS

#endif  /* __GIMP_BASE_ENUMS_H__ */
