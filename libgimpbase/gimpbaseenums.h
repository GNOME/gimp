/* LIBLIGMA - The LIGMA Library
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

#ifndef __LIGMA_BASE_ENUMS_H__
#define __LIGMA_BASE_ENUMS_H__


/**
 * SECTION: ligmabaseenums
 * @title: ligmabaseenums
 * @short_description: Basic LIGMA enumeration data types.
 *
 * Basic LIGMA enumeration data types.
 **/


G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * LigmaAddMaskType:
 * @LIGMA_ADD_MASK_WHITE:          White (full opacity)
 * @LIGMA_ADD_MASK_BLACK:          Black (full transparency)
 * @LIGMA_ADD_MASK_ALPHA:          Layer's alpha channel
 * @LIGMA_ADD_MASK_ALPHA_TRANSFER: Transfer layer's alpha channel
 * @LIGMA_ADD_MASK_SELECTION:      Selection
 * @LIGMA_ADD_MASK_COPY:           Grayscale copy of layer
 * @LIGMA_ADD_MASK_CHANNEL:        Channel
 *
 * Modes of initialising a layer mask.
 **/
#define LIGMA_TYPE_ADD_MASK_TYPE (ligma_add_mask_type_get_type ())

GType ligma_add_mask_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_ADD_MASK_WHITE,          /*< desc="_White (full opacity)"           >*/
  LIGMA_ADD_MASK_BLACK,          /*< desc="_Black (full transparency)"      >*/
  LIGMA_ADD_MASK_ALPHA,          /*< desc="Layer's _alpha channel"          >*/
  LIGMA_ADD_MASK_ALPHA_TRANSFER, /*< desc="_Transfer layer's alpha channel" >*/
  LIGMA_ADD_MASK_SELECTION,      /*< desc="_Selection"                      >*/
  LIGMA_ADD_MASK_COPY,           /*< desc="_Grayscale copy of layer"        >*/
  LIGMA_ADD_MASK_CHANNEL         /*< desc="C_hannel"                        >*/
} LigmaAddMaskType;


/**
 * LigmaBrushGeneratedShape:
 * @LIGMA_BRUSH_GENERATED_CIRCLE:  Circle
 * @LIGMA_BRUSH_GENERATED_SQUARE:  Square
 * @LIGMA_BRUSH_GENERATED_DIAMOND: Diamond
 *
 * Shapes of generated brushes.
 **/
#define LIGMA_TYPE_BRUSH_GENERATED_SHAPE (ligma_brush_generated_shape_get_type ())

GType ligma_brush_generated_shape_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_BRUSH_GENERATED_CIRCLE,  /*< desc="Circle"  >*/
  LIGMA_BRUSH_GENERATED_SQUARE,  /*< desc="Square"  >*/
  LIGMA_BRUSH_GENERATED_DIAMOND  /*< desc="Diamond" >*/
} LigmaBrushGeneratedShape;


/**
 * LigmaCapStyle:
 * @LIGMA_CAP_BUTT:   Butt
 * @LIGMA_CAP_ROUND:  Round
 * @LIGMA_CAP_SQUARE: Square
 *
 * Style of line endings.
 **/
#define LIGMA_TYPE_CAP_STYLE (ligma_cap_style_get_type ())

GType ligma_cap_style_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_CAP_BUTT,   /*< desc="Butt"   >*/
  LIGMA_CAP_ROUND,  /*< desc="Round"  >*/
  LIGMA_CAP_SQUARE  /*< desc="Square" >*/
} LigmaCapStyle;


/**
 * LigmaChannelOps:
 * @LIGMA_CHANNEL_OP_ADD:       Add to the current selection
 * @LIGMA_CHANNEL_OP_SUBTRACT:  Subtract from the current selection
 * @LIGMA_CHANNEL_OP_REPLACE:   Replace the current selection
 * @LIGMA_CHANNEL_OP_INTERSECT: Intersect with the current selection
 *
 * Operations to combine channels and selections.
 **/
#define LIGMA_TYPE_CHANNEL_OPS (ligma_channel_ops_get_type ())

GType ligma_channel_ops_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_CHANNEL_OP_ADD,       /*< desc="Add to the current selection"         >*/
  LIGMA_CHANNEL_OP_SUBTRACT,  /*< desc="Subtract from the current selection"  >*/
  LIGMA_CHANNEL_OP_REPLACE,   /*< desc="Replace the current selection"        >*/
  LIGMA_CHANNEL_OP_INTERSECT  /*< desc="Intersect with the current selection" >*/
} LigmaChannelOps;


/**
 * LigmaChannelType:
 * @LIGMA_CHANNEL_RED:     Red
 * @LIGMA_CHANNEL_GREEN:   Green
 * @LIGMA_CHANNEL_BLUE:    Blue
 * @LIGMA_CHANNEL_GRAY:    Gray
 * @LIGMA_CHANNEL_INDEXED: Indexed
 * @LIGMA_CHANNEL_ALPHA:   Alpha
 *
 * Channels (as in color components).
 **/
#define LIGMA_TYPE_CHANNEL_TYPE (ligma_channel_type_get_type ())

GType ligma_channel_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_CHANNEL_RED,      /*< desc="Red"     >*/
  LIGMA_CHANNEL_GREEN,    /*< desc="Green"   >*/
  LIGMA_CHANNEL_BLUE,     /*< desc="Blue"    >*/
  LIGMA_CHANNEL_GRAY,     /*< desc="Gray"    >*/
  LIGMA_CHANNEL_INDEXED,  /*< desc="Indexed" >*/
  LIGMA_CHANNEL_ALPHA     /*< desc="Alpha"   >*/
} LigmaChannelType;


/**
 * LigmaCheckSize:
 * @LIGMA_CHECK_SIZE_SMALL_CHECKS:  Small
 * @LIGMA_CHECK_SIZE_MEDIUM_CHECKS: Medium
 * @LIGMA_CHECK_SIZE_LARGE_CHECKS:  Large
 *
 * Size of the checkerboard indicating transparency.
 **/
#define LIGMA_TYPE_CHECK_SIZE (ligma_check_size_get_type ())

GType ligma_check_size_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_CHECK_SIZE_SMALL_CHECKS  = 0,  /*< desc="Small"  >*/
  LIGMA_CHECK_SIZE_MEDIUM_CHECKS = 1,  /*< desc="Medium" >*/
  LIGMA_CHECK_SIZE_LARGE_CHECKS  = 2   /*< desc="Large"  >*/
} LigmaCheckSize;


/**
 * LigmaCheckType:
 * @LIGMA_CHECK_TYPE_LIGHT_CHECKS: Light checks
 * @LIGMA_CHECK_TYPE_GRAY_CHECKS:  Mid-tone checks
 * @LIGMA_CHECK_TYPE_DARK_CHECKS:  Dark checks
 * @LIGMA_CHECK_TYPE_WHITE_ONLY:   White only
 * @LIGMA_CHECK_TYPE_GRAY_ONLY:    Gray only
 * @LIGMA_CHECK_TYPE_BLACK_ONLY:   Black only
 * @LIGMA_CHECK_TYPE_CUSTOM_CHECKS: Custom checks
 *
 * Color/Brightness of the checkerboard indicating transparency.
 **/
#define LIGMA_TYPE_CHECK_TYPE (ligma_check_type_get_type ())

GType ligma_check_type_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  LIGMA_CHECK_TYPE_LIGHT_CHECKS = 0,  /*< desc="Light checks"    >*/
  LIGMA_CHECK_TYPE_GRAY_CHECKS  = 1,  /*< desc="Mid-tone checks" >*/
  LIGMA_CHECK_TYPE_DARK_CHECKS  = 2,  /*< desc="Dark checks"     >*/
  LIGMA_CHECK_TYPE_WHITE_ONLY   = 3,  /*< desc="White only"      >*/
  LIGMA_CHECK_TYPE_GRAY_ONLY    = 4,  /*< desc="Gray only"       >*/
  LIGMA_CHECK_TYPE_BLACK_ONLY   = 5,  /*< desc="Black only"      >*/
  LIGMA_CHECK_TYPE_CUSTOM_CHECKS = 6  /*< desc="Custom checks"   >*/
} LigmaCheckType;


/**
 * LigmaCloneType:
 * @LIGMA_CLONE_IMAGE:   Clone from an image/drawable source
 * @LIGMA_CLONE_PATTERN: Clone from a pattern source
 *
 * Clone sources.
 **/
#define LIGMA_TYPE_CLONE_TYPE (ligma_clone_type_get_type ())

GType ligma_clone_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_CLONE_IMAGE,    /*< desc="Image"   >*/
  LIGMA_CLONE_PATTERN   /*< desc="Pattern" >*/
} LigmaCloneType;


/**
 * LigmaColorTag:
 * @LIGMA_COLOR_TAG_NONE:   None
 * @LIGMA_COLOR_TAG_BLUE:   Blue
 * @LIGMA_COLOR_TAG_GREEN:  Green
 * @LIGMA_COLOR_TAG_YELLOW: Yellow
 * @LIGMA_COLOR_TAG_ORANGE: Orange
 * @LIGMA_COLOR_TAG_BROWN:  Brown
 * @LIGMA_COLOR_TAG_RED:    Red
 * @LIGMA_COLOR_TAG_VIOLET: Violet
 * @LIGMA_COLOR_TAG_GRAY:   Gray
 *
 * Possible tag colors.
 *
 * Since: 2.10
 **/
#define LIGMA_TYPE_COLOR_TAG (ligma_color_tag_get_type ())

GType ligma_color_tag_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_COLOR_TAG_NONE,   /*< desc="None"   >*/
  LIGMA_COLOR_TAG_BLUE,   /*< desc="Blue"   >*/
  LIGMA_COLOR_TAG_GREEN,  /*< desc="Green"  >*/
  LIGMA_COLOR_TAG_YELLOW, /*< desc="Yellow" >*/
  LIGMA_COLOR_TAG_ORANGE, /*< desc="Orange" >*/
  LIGMA_COLOR_TAG_BROWN,  /*< desc="Brown"  >*/
  LIGMA_COLOR_TAG_RED,    /*< desc="Red"    >*/
  LIGMA_COLOR_TAG_VIOLET, /*< desc="Violet" >*/
  LIGMA_COLOR_TAG_GRAY    /*< desc="Gray"   >*/
} LigmaColorTag;


/**
 * LigmaComponentType:
 * @LIGMA_COMPONENT_TYPE_U8:     8-bit integer
 * @LIGMA_COMPONENT_TYPE_U16:    16-bit integer
 * @LIGMA_COMPONENT_TYPE_U32:    32-bit integer
 * @LIGMA_COMPONENT_TYPE_HALF:   16-bit floating point
 * @LIGMA_COMPONENT_TYPE_FLOAT:  32-bit floating point
 * @LIGMA_COMPONENT_TYPE_DOUBLE: 64-bit floating point
 *
 * Encoding types of image components.
 *
 * Since: 2.10
 **/
#define LIGMA_TYPE_COMPONENT_TYPE (ligma_component_type_get_type ())

GType ligma_component_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_COMPONENT_TYPE_U8     = 100, /*< desc="8-bit integer"         >*/
  LIGMA_COMPONENT_TYPE_U16    = 200, /*< desc="16-bit integer"        >*/
  LIGMA_COMPONENT_TYPE_U32    = 300, /*< desc="32-bit integer"        >*/
  LIGMA_COMPONENT_TYPE_HALF   = 500, /*< desc="16-bit floating point" >*/
  LIGMA_COMPONENT_TYPE_FLOAT  = 600, /*< desc="32-bit floating point" >*/
  LIGMA_COMPONENT_TYPE_DOUBLE = 700  /*< desc="64-bit floating point" >*/
} LigmaComponentType;


/**
 * LigmaConvertPaletteType:
 * @LIGMA_CONVERT_PALETTE_GENERATE: Generate optimum palette
 * @LIGMA_CONVERT_PALETTE_WEB:      Use web-optimized palette
 * @LIGMA_CONVERT_PALETTE_MONO:     Use black and white (1-bit) palette
 * @LIGMA_CONVERT_PALETTE_CUSTOM:   Use custom palette
 *
 * Types of palettes for indexed conversion.
 **/
#define LIGMA_TYPE_CONVERT_PALETTE_TYPE (ligma_convert_palette_type_get_type ())

GType ligma_convert_palette_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_CONVERT_PALETTE_GENERATE, /*< desc="Generate optimum palette"          >*/
  LIGMA_CONVERT_PALETTE_WEB,      /*< desc="Use web-optimized palette"         >*/
  LIGMA_CONVERT_PALETTE_MONO,     /*< desc="Use black and white (1-bit) palette" >*/
  LIGMA_CONVERT_PALETTE_CUSTOM    /*< desc="Use custom palette"                >*/
} LigmaConvertPaletteType;


/**
 * LigmaConvolveType:
 * @LIGMA_CONVOLVE_BLUR:    Blur
 * @LIGMA_CONVOLVE_SHARPEN: Sharpen
 *
 * Types of convolutions.
 **/
#define LIGMA_TYPE_CONVOLVE_TYPE (ligma_convolve_type_get_type ())

GType ligma_convolve_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_CONVOLVE_BLUR,    /*< desc="Blur"    >*/
  LIGMA_CONVOLVE_SHARPEN  /*< desc="Sharpen" >*/
} LigmaConvolveType;


/**
 * LigmaDesaturateMode:
 * @LIGMA_DESATURATE_LIGHTNESS:  Lightness (HSL)
 * @LIGMA_DESATURATE_LUMA:       Luma
 * @LIGMA_DESATURATE_AVERAGE:    Average (HSI Intensity)
 * @LIGMA_DESATURATE_LUMINANCE:  Luminance
 * @LIGMA_DESATURATE_VALUE:      Value (HSV)
 *
 * Grayscale conversion methods.
 **/
#define LIGMA_TYPE_DESATURATE_MODE (ligma_desaturate_mode_get_type ())

GType ligma_desaturate_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_DESATURATE_LIGHTNESS,   /*< desc="Lightness (HSL)"          >*/
  LIGMA_DESATURATE_LUMA,        /*< desc="Luma"                     >*/
  LIGMA_DESATURATE_AVERAGE,     /*< desc="Average (HSI Intensity)"  >*/
  LIGMA_DESATURATE_LUMINANCE,   /*< desc="Luminance"                >*/
  LIGMA_DESATURATE_VALUE        /*< desc="Value (HSV)"              >*/
} LigmaDesaturateMode;


/**
 * LigmaDodgeBurnType:
 * @LIGMA_DODGE_BURN_TYPE_DODGE: Dodge
 * @LIGMA_DODGE_BURN_TYPE_BURN:  Burn
 *
 * Methods for the dodge/burn operation.
 **/
#define LIGMA_TYPE_DODGE_BURN_TYPE (ligma_dodge_burn_type_get_type ())

GType ligma_dodge_burn_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_DODGE_BURN_TYPE_DODGE,  /*< desc="Dodge" >*/
  LIGMA_DODGE_BURN_TYPE_BURN    /*< desc="Burn"  >*/
} LigmaDodgeBurnType;


/**
 * LigmaFillType:
 * @LIGMA_FILL_FOREGROUND:  Foreground color
 * @LIGMA_FILL_BACKGROUND:  Background color
 * @LIGMA_FILL_WHITE:       White
 * @LIGMA_FILL_TRANSPARENT: Transparency
 * @LIGMA_FILL_PATTERN:     Pattern
 *
 * Types of filling.
 **/
#define LIGMA_TYPE_FILL_TYPE (ligma_fill_type_get_type ())

GType ligma_fill_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_FILL_FOREGROUND,  /*< desc="Foreground color" >*/
  LIGMA_FILL_BACKGROUND,  /*< desc="Background color" >*/
  LIGMA_FILL_WHITE,       /*< desc="White"            >*/
  LIGMA_FILL_TRANSPARENT, /*< desc="Transparency"     >*/
  LIGMA_FILL_PATTERN      /*< desc="Pattern"          >*/
} LigmaFillType;


/**
 * LigmaForegroundExtractMode:
 * @LIGMA_FOREGROUND_EXTRACT_MATTING: Matting (Since 2.10)
 *
 * Foreground extract engines.
 **/
#define LIGMA_TYPE_FOREGROUND_EXTRACT_MODE (ligma_foreground_extract_mode_get_type ())

GType ligma_foreground_extract_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_FOREGROUND_EXTRACT_MATTING
} LigmaForegroundExtractMode;


/**
 * LigmaGradientBlendColorSpace:
 * @LIGMA_GRADIENT_BLEND_RGB_PERCEPTUAL: Perceptual RGB
 * @LIGMA_GRADIENT_BLEND_RGB_LINEAR:     Linear RGB
 * @LIGMA_GRADIENT_BLEND_CIE_LAB:        CIE Lab
 *
 * Color space for blending gradients.
 *
 * Since: 2.10
 */
#define LIGMA_TYPE_GRADIENT_BLEND_COLOR_SPACE (ligma_gradient_blend_color_space_get_type ())

GType ligma_gradient_blend_color_space_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_GRADIENT_BLEND_RGB_PERCEPTUAL,  /*< desc="Perceptual RGB", nick=rgb-perceptual >*/
  LIGMA_GRADIENT_BLEND_RGB_LINEAR,      /*< desc="Linear RGB",     nick=rgb-linear     >*/
  LIGMA_GRADIENT_BLEND_CIE_LAB          /*< desc="CIE Lab",        nick=cie-lab        >*/
} LigmaGradientBlendColorSpace;


/**
 * LigmaGradientSegmentColor:
 * @LIGMA_GRADIENT_SEGMENT_RGB:     RGB
 * @LIGMA_GRADIENT_SEGMENT_HSV_CCW: HSV (counter-clockwise hue)
 * @LIGMA_GRADIENT_SEGMENT_HSV_CW:  HSV (clockwise hue)
 *
 * Coloring types for gradient segments.
 **/
#define LIGMA_TYPE_GRADIENT_SEGMENT_COLOR (ligma_gradient_segment_color_get_type ())

GType ligma_gradient_segment_color_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_GRADIENT_SEGMENT_RGB,      /*< desc="RGB"                                             >*/
  LIGMA_GRADIENT_SEGMENT_HSV_CCW,  /*< desc="HSV (counter-clockwise hue)", abbrev="HSV (ccw)" >*/
  LIGMA_GRADIENT_SEGMENT_HSV_CW    /*< desc="HSV (clockwise hue)",         abbrev="HSV (cw)"  >*/
} LigmaGradientSegmentColor;


/**
 * LigmaGradientSegmentType:
 * @LIGMA_GRADIENT_SEGMENT_LINEAR:            Linear
 * @LIGMA_GRADIENT_SEGMENT_CURVED:            Curved
 * @LIGMA_GRADIENT_SEGMENT_SINE:              Sinusoidal
 * @LIGMA_GRADIENT_SEGMENT_SPHERE_INCREASING: Spherical (increasing)
 * @LIGMA_GRADIENT_SEGMENT_SPHERE_DECREASING: Spherical (decreasing)
 * @LIGMA_GRADIENT_SEGMENT_STEP:              Step
 *
 * Transition functions for gradient segments.
 **/
#define LIGMA_TYPE_GRADIENT_SEGMENT_TYPE (ligma_gradient_segment_type_get_type ())

GType ligma_gradient_segment_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_GRADIENT_SEGMENT_LINEAR,             /*< desc="Linear"                                           >*/
  LIGMA_GRADIENT_SEGMENT_CURVED,             /*< desc="Curved"                                           >*/
  LIGMA_GRADIENT_SEGMENT_SINE,               /*< desc="Sinusoidal"                                       >*/
  LIGMA_GRADIENT_SEGMENT_SPHERE_INCREASING,  /*< desc="Spherical (increasing)", abbrev="Spherical (inc)" >*/
  LIGMA_GRADIENT_SEGMENT_SPHERE_DECREASING,  /*< desc="Spherical (decreasing)", abbrev="Spherical (dec)" >*/
  LIGMA_GRADIENT_SEGMENT_STEP                /*< desc="Step"                                             >*/
} LigmaGradientSegmentType;


/**
 * LigmaGradientType:
 * @LIGMA_GRADIENT_LINEAR:               Linear
 * @LIGMA_GRADIENT_BILINEAR:             Bi-linear
 * @LIGMA_GRADIENT_RADIAL:               Radial
 * @LIGMA_GRADIENT_SQUARE:               Square
 * @LIGMA_GRADIENT_CONICAL_SYMMETRIC:    Conical (symmetric)
 * @LIGMA_GRADIENT_CONICAL_ASYMMETRIC:   Conical (asymmetric)
 * @LIGMA_GRADIENT_SHAPEBURST_ANGULAR:   Shaped (angular)
 * @LIGMA_GRADIENT_SHAPEBURST_SPHERICAL: Shaped (spherical)
 * @LIGMA_GRADIENT_SHAPEBURST_DIMPLED:   Shaped (dimpled)
 * @LIGMA_GRADIENT_SPIRAL_CLOCKWISE:     Spiral (clockwise)
 * @LIGMA_GRADIENT_SPIRAL_ANTICLOCKWISE: Spiral (counter-clockwise)
 *
 * Gradient shapes.
 **/
#define LIGMA_TYPE_GRADIENT_TYPE (ligma_gradient_type_get_type ())

GType ligma_gradient_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_GRADIENT_LINEAR,                /*< desc="Linear"                                              >*/
  LIGMA_GRADIENT_BILINEAR,              /*< desc="Bi-linear"                                           >*/
  LIGMA_GRADIENT_RADIAL,                /*< desc="Radial"                                              >*/
  LIGMA_GRADIENT_SQUARE,                /*< desc="Square"                                              >*/
  LIGMA_GRADIENT_CONICAL_SYMMETRIC,     /*< desc="Conical (symmetric)",        abbrev="Conical (sym)"  >*/
  LIGMA_GRADIENT_CONICAL_ASYMMETRIC,    /*< desc="Conical (asymmetric)",       abbrev="Conical (asym)" >*/
  LIGMA_GRADIENT_SHAPEBURST_ANGULAR,    /*< desc="Shaped (angular)"                                    >*/
  LIGMA_GRADIENT_SHAPEBURST_SPHERICAL,  /*< desc="Shaped (spherical)"                                  >*/
  LIGMA_GRADIENT_SHAPEBURST_DIMPLED,    /*< desc="Shaped (dimpled)"                                    >*/
  LIGMA_GRADIENT_SPIRAL_CLOCKWISE,      /*< desc="Spiral (clockwise)",         abbrev="Spiral (cw)"    >*/
  LIGMA_GRADIENT_SPIRAL_ANTICLOCKWISE   /*< desc="Spiral (counter-clockwise)", abbrev="Spiral (ccw)"   >*/
} LigmaGradientType;


/**
 * LigmaGridStyle:
 * @LIGMA_GRID_DOTS:          Intersections (dots)
 * @LIGMA_GRID_INTERSECTIONS: Intersections (crosshairs)
 * @LIGMA_GRID_ON_OFF_DASH:   Dashed
 * @LIGMA_GRID_DOUBLE_DASH:   Double dashed
 * @LIGMA_GRID_SOLID:         Solid
 *
 * Rendering types for the display grid.
 **/
#define LIGMA_TYPE_GRID_STYLE (ligma_grid_style_get_type ())

GType ligma_grid_style_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_GRID_DOTS,           /*< desc="Intersections (dots)"       >*/
  LIGMA_GRID_INTERSECTIONS,  /*< desc="Intersections (crosshairs)" >*/
  LIGMA_GRID_ON_OFF_DASH,    /*< desc="Dashed"                     >*/
  LIGMA_GRID_DOUBLE_DASH,    /*< desc="Double dashed"              >*/
  LIGMA_GRID_SOLID           /*< desc="Solid"                      >*/
} LigmaGridStyle;


/**
 * LigmaHueRange:
 * @LIGMA_HUE_RANGE_ALL:     All hues
 * @LIGMA_HUE_RANGE_RED:     Red hues
 * @LIGMA_HUE_RANGE_YELLOW:  Yellow hues
 * @LIGMA_HUE_RANGE_GREEN:   Green hues
 * @LIGMA_HUE_RANGE_CYAN:    Cyan hues
 * @LIGMA_HUE_RANGE_BLUE:    Blue hues
 * @LIGMA_HUE_RANGE_MAGENTA: Magenta hues
 *
 * Hue ranges.
 **/
#define LIGMA_TYPE_HUE_RANGE (ligma_hue_range_get_type ())

GType ligma_hue_range_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_HUE_RANGE_ALL,
  LIGMA_HUE_RANGE_RED,
  LIGMA_HUE_RANGE_YELLOW,
  LIGMA_HUE_RANGE_GREEN,
  LIGMA_HUE_RANGE_CYAN,
  LIGMA_HUE_RANGE_BLUE,
  LIGMA_HUE_RANGE_MAGENTA
} LigmaHueRange;


/**
 * LigmaIconType:
 * @LIGMA_ICON_TYPE_ICON_NAME:  Icon name
 * @LIGMA_ICON_TYPE_PIXBUF:     Inline pixbuf
 * @LIGMA_ICON_TYPE_IMAGE_FILE: Image file
 *
 * Icon types for plug-ins to register.
 **/
#define LIGMA_TYPE_ICON_TYPE (ligma_icon_type_get_type ())

GType ligma_icon_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_ICON_TYPE_ICON_NAME,  /*< desc="Icon name"  >*/
  LIGMA_ICON_TYPE_PIXBUF,     /*< desc="Pixbuf"     >*/
  LIGMA_ICON_TYPE_IMAGE_FILE  /*< desc="Image file" >*/
} LigmaIconType;


/**
 * LigmaImageBaseType:
 * @LIGMA_RGB:     RGB color
 * @LIGMA_GRAY:    Grayscale
 * @LIGMA_INDEXED: Indexed color
 *
 * Image color models.
 **/
#define LIGMA_TYPE_IMAGE_BASE_TYPE (ligma_image_base_type_get_type ())

GType ligma_image_base_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_RGB,     /*< desc="RGB color"     >*/
  LIGMA_GRAY,    /*< desc="Grayscale"     >*/
  LIGMA_INDEXED  /*< desc="Indexed color" >*/
} LigmaImageBaseType;


/**
 * LigmaImageType:
 * @LIGMA_RGB_IMAGE:      RGB
 * @LIGMA_RGBA_IMAGE:     RGB-alpha
 * @LIGMA_GRAY_IMAGE:     Grayscale
 * @LIGMA_GRAYA_IMAGE:    Grayscale-alpha
 * @LIGMA_INDEXED_IMAGE:  Indexed
 * @LIGMA_INDEXEDA_IMAGE: Indexed-alpha
 *
 * Possible drawable types.
 **/
#define LIGMA_TYPE_IMAGE_TYPE (ligma_image_type_get_type ())

GType ligma_image_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_RGB_IMAGE,      /*< desc="RGB"             >*/
  LIGMA_RGBA_IMAGE,     /*< desc="RGB-alpha"       >*/
  LIGMA_GRAY_IMAGE,     /*< desc="Grayscale"       >*/
  LIGMA_GRAYA_IMAGE,    /*< desc="Grayscale-alpha" >*/
  LIGMA_INDEXED_IMAGE,  /*< desc="Indexed"         >*/
  LIGMA_INDEXEDA_IMAGE  /*< desc="Indexed-alpha"   >*/
} LigmaImageType;


/**
 * LigmaInkBlobType:
 * @LIGMA_INK_BLOB_TYPE_CIRCLE:  Circle
 * @LIGMA_INK_BLOB_TYPE_SQUARE:  Square
 * @LIGMA_INK_BLOB_TYPE_DIAMOND: Diamond
 *
 * Ink tool tips.
 **/
#define LIGMA_TYPE_INK_BLOB_TYPE (ligma_ink_blob_type_get_type ())

GType ligma_ink_blob_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_INK_BLOB_TYPE_CIRCLE,  /*< desc="Circle"  >*/
  LIGMA_INK_BLOB_TYPE_SQUARE,  /*< desc="Square"  >*/
  LIGMA_INK_BLOB_TYPE_DIAMOND  /*< desc="Diamond" >*/
} LigmaInkBlobType;


/**
 * LigmaInterpolationType:
 * @LIGMA_INTERPOLATION_NONE:   None
 * @LIGMA_INTERPOLATION_LINEAR: Linear
 * @LIGMA_INTERPOLATION_CUBIC:  Cubic
 * @LIGMA_INTERPOLATION_NOHALO: NoHalo
 * @LIGMA_INTERPOLATION_LOHALO: LoHalo
 *
 * Interpolation types.
 **/
#define LIGMA_TYPE_INTERPOLATION_TYPE (ligma_interpolation_type_get_type ())

GType ligma_interpolation_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_INTERPOLATION_NONE,   /*< desc="None"   >*/
  LIGMA_INTERPOLATION_LINEAR, /*< desc="Linear" >*/
  LIGMA_INTERPOLATION_CUBIC,  /*< desc="Cubic"  >*/
  LIGMA_INTERPOLATION_NOHALO, /*< desc="NoHalo" >*/
  LIGMA_INTERPOLATION_LOHALO  /*< desc="LoHalo" >*/
} LigmaInterpolationType;


/**
 * LigmaJoinStyle:
 * @LIGMA_JOIN_MITER: Miter
 * @LIGMA_JOIN_ROUND: Round
 * @LIGMA_JOIN_BEVEL: Bevel
 *
 * Line join styles.
 **/
#define LIGMA_TYPE_JOIN_STYLE (ligma_join_style_get_type ())

GType ligma_join_style_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_JOIN_MITER,  /*< desc="Miter" >*/
  LIGMA_JOIN_ROUND,  /*< desc="Round" >*/
  LIGMA_JOIN_BEVEL   /*< desc="Bevel" >*/
} LigmaJoinStyle;


/**
 * LigmaMaskApplyMode:
 * @LIGMA_MASK_APPLY:   Apply the mask
 * @LIGMA_MASK_DISCARD: Discard the mask
 *
 * Layer mask apply modes.
 **/
#define LIGMA_TYPE_MASK_APPLY_MODE (ligma_mask_apply_mode_get_type ())

GType ligma_mask_apply_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_MASK_APPLY,
  LIGMA_MASK_DISCARD
} LigmaMaskApplyMode;


/**
 * LigmaMergeType:
 * @LIGMA_EXPAND_AS_NECESSARY:  Expanded as necessary
 * @LIGMA_CLIP_TO_IMAGE:        Clipped to image
 * @LIGMA_CLIP_TO_BOTTOM_LAYER: Clipped to bottom layer
 * @LIGMA_FLATTEN_IMAGE:        Flatten
 *
 * Types of merging layers.
 **/
#define LIGMA_TYPE_MERGE_TYPE (ligma_merge_type_get_type ())

GType ligma_merge_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_EXPAND_AS_NECESSARY,  /*< desc="Expanded as necessary"  >*/
  LIGMA_CLIP_TO_IMAGE,        /*< desc="Clipped to image"        >*/
  LIGMA_CLIP_TO_BOTTOM_LAYER, /*< desc="Clipped to bottom layer" >*/
  LIGMA_FLATTEN_IMAGE         /*< desc="Flatten"                 >*/
} LigmaMergeType;


/**
 * LigmaMessageHandlerType:
 * @LIGMA_MESSAGE_BOX:   A popup dialog
 * @LIGMA_CONSOLE:       The terminal
 * @LIGMA_ERROR_CONSOLE: The error console dockable
 *
 * How to present messages.
 **/
#define LIGMA_TYPE_MESSAGE_HANDLER_TYPE (ligma_message_handler_type_get_type ())

GType ligma_message_handler_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_MESSAGE_BOX,
  LIGMA_CONSOLE,
  LIGMA_ERROR_CONSOLE
} LigmaMessageHandlerType;


/**
 * LigmaOffsetType:
 * @LIGMA_OFFSET_BACKGROUND:  Background
 * @LIGMA_OFFSET_TRANSPARENT: Transparent
 * @LIGMA_OFFSET_WRAP_AROUND: Wrap image around
 *
 * Background fill types for the offset operation.
 **/
#define LIGMA_TYPE_OFFSET_TYPE (ligma_offset_type_get_type ())

GType ligma_offset_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_OFFSET_BACKGROUND,
  LIGMA_OFFSET_TRANSPARENT,
  LIGMA_OFFSET_WRAP_AROUND
} LigmaOffsetType;


/**
 * LigmaOrientationType:
 * @LIGMA_ORIENTATION_HORIZONTAL: Horizontal
 * @LIGMA_ORIENTATION_VERTICAL:   Vertical
 * @LIGMA_ORIENTATION_UNKNOWN:    Unknown
 *
 * Orientations for various purposes.
 **/
#define LIGMA_TYPE_ORIENTATION_TYPE (ligma_orientation_type_get_type ())

GType ligma_orientation_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_ORIENTATION_HORIZONTAL, /*< desc="Horizontal" >*/
  LIGMA_ORIENTATION_VERTICAL,   /*< desc="Vertical"   >*/
  LIGMA_ORIENTATION_UNKNOWN     /*< desc="Unknown"    >*/
} LigmaOrientationType;


/**
 * LigmaPaintApplicationMode:
 * @LIGMA_PAINT_CONSTANT:    Constant
 * @LIGMA_PAINT_INCREMENTAL: Incremental
 *
 * Paint application modes.
 **/
#define LIGMA_TYPE_PAINT_APPLICATION_MODE (ligma_paint_application_mode_get_type ())

GType ligma_paint_application_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_PAINT_CONSTANT,    /*< desc="Constant"    >*/
  LIGMA_PAINT_INCREMENTAL  /*< desc="Incremental" >*/
} LigmaPaintApplicationMode;


/**
 * LigmaPDBErrorHandler:
 * @LIGMA_PDB_ERROR_HANDLER_INTERNAL: Internal
 * @LIGMA_PDB_ERROR_HANDLER_PLUGIN:   Plug-In
 *
 * PDB error handlers.
 **/
#define LIGMA_TYPE_PDB_ERROR_HANDLER (ligma_pdb_error_handler_get_type ())

GType ligma_pdb_error_handler_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_PDB_ERROR_HANDLER_INTERNAL,
  LIGMA_PDB_ERROR_HANDLER_PLUGIN
} LigmaPDBErrorHandler;


/**
 * LigmaPDBProcType:
 * @LIGMA_PDB_PROC_TYPE_INTERNAL:  Internal LIGMA procedure
 * @LIGMA_PDB_PROC_TYPE_PLUGIN:    LIGMA Plug-In
 * @LIGMA_PDB_PROC_TYPE_EXTENSION: LIGMA Extension
 * @LIGMA_PDB_PROC_TYPE_TEMPORARY: Temporary Procedure
 *
 * Types of PDB procedures.
 **/
#define LIGMA_TYPE_PDB_PROC_TYPE (ligma_pdb_proc_type_get_type ())

GType ligma_pdb_proc_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_PDB_PROC_TYPE_INTERNAL,   /*< desc="Internal LIGMA procedure" >*/
  LIGMA_PDB_PROC_TYPE_PLUGIN,     /*< desc="LIGMA Plug-In" >*/
  LIGMA_PDB_PROC_TYPE_EXTENSION,  /*< desc="LIGMA Extension" >*/
  LIGMA_PDB_PROC_TYPE_TEMPORARY   /*< desc="Temporary Procedure" >*/
} LigmaPDBProcType;


/**
 * LigmaPDBStatusType:
 * @LIGMA_PDB_EXECUTION_ERROR: Execution error
 * @LIGMA_PDB_CALLING_ERROR:   Calling error
 * @LIGMA_PDB_PASS_THROUGH:    Pass through
 * @LIGMA_PDB_SUCCESS:         Success
 * @LIGMA_PDB_CANCEL:          User cancel
 *
 * Return status of PDB calls.
 **/
#define LIGMA_TYPE_PDB_STATUS_TYPE (ligma_pdb_status_type_get_type ())

GType ligma_pdb_status_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_PDB_EXECUTION_ERROR,
  LIGMA_PDB_CALLING_ERROR,
  LIGMA_PDB_PASS_THROUGH,
  LIGMA_PDB_SUCCESS,
  LIGMA_PDB_CANCEL
} LigmaPDBStatusType;


/**
 * LigmaPrecision:
 * @LIGMA_PRECISION_U8_LINEAR:         8-bit linear integer
 * @LIGMA_PRECISION_U8_NON_LINEAR:     8-bit non-linear integer
 * @LIGMA_PRECISION_U8_PERCEPTUAL:     8-bit perceptual integer
 * @LIGMA_PRECISION_U16_LINEAR:        16-bit linear integer
 * @LIGMA_PRECISION_U16_NON_LINEAR:    16-bit non-linear integer
 * @LIGMA_PRECISION_U16_PERCEPTUAL:    16-bit perceptual integer
 * @LIGMA_PRECISION_U32_LINEAR:        32-bit linear integer
 * @LIGMA_PRECISION_U32_NON_LINEAR:    32-bit non-linear integer
 * @LIGMA_PRECISION_U32_PERCEPTUAL:    32-bit perceptual integer
 * @LIGMA_PRECISION_HALF_LINEAR:       16-bit linear floating point
 * @LIGMA_PRECISION_HALF_NON_LINEAR:   16-bit non-linear floating point
 * @LIGMA_PRECISION_HALF_PERCEPTUAL:   16-bit perceptual floating point
 * @LIGMA_PRECISION_FLOAT_LINEAR:      32-bit linear floating point
 * @LIGMA_PRECISION_FLOAT_NON_LINEAR:  32-bit non-linear floating point
 * @LIGMA_PRECISION_FLOAT_PERCEPTUAL:  32-bit perceptual floating point
 * @LIGMA_PRECISION_DOUBLE_LINEAR:     64-bit linear floating point
 * @LIGMA_PRECISION_DOUBLE_NON_LINEAR: 64-bit non-linear floating point
 * @LIGMA_PRECISION_DOUBLE_PERCEPTUAL: 64-bit perceptual floating point
 * @LIGMA_PRECISION_U8_GAMMA:      deprecated alias for
 *                                @LIGMA_PRECISION_U8_NON_LINEAR
 * @LIGMA_PRECISION_U16_GAMMA:     deprecated alias for
 *                                @LIGMA_PRECISION_U16_NON_LINEAR
 * @LIGMA_PRECISION_U32_GAMMA:     deprecated alias for
 *                                @LIGMA_PRECISION_U32_NON_LINEAR
 * @LIGMA_PRECISION_HALF_GAMMA:    deprecated alias for
 *                                @LIGMA_PRECISION_HALF_NON_LINEAR
 * @LIGMA_PRECISION_FLOAT_GAMMA:   deprecated alias for
 *                                @LIGMA_PRECISION_FLOAT_NON_LINEAR
 * @LIGMA_PRECISION_DOUBLE_GAMMA:  deprecated alias for
 *                                @LIGMA_PRECISION_DOUBLE_NON_LINEAR
 *
 * Precisions for pixel encoding.
 *
 * Since: 2.10
 **/
#define LIGMA_TYPE_PRECISION (ligma_precision_get_type ())

GType ligma_precision_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_PRECISION_U8_LINEAR         = 100, /*< desc="8-bit linear integer"         >*/
  LIGMA_PRECISION_U8_NON_LINEAR     = 150, /*< desc="8-bit non-linear integer"          >*/
  LIGMA_PRECISION_U8_PERCEPTUAL     = 175, /*< desc="8-bit perceptual integer"          >*/
  LIGMA_PRECISION_U16_LINEAR        = 200, /*< desc="16-bit linear integer"        >*/
  LIGMA_PRECISION_U16_NON_LINEAR    = 250, /*< desc="16-bit non-linear integer"         >*/
  LIGMA_PRECISION_U16_PERCEPTUAL    = 275, /*< desc="16-bit perceptual integer"         >*/
  LIGMA_PRECISION_U32_LINEAR        = 300, /*< desc="32-bit linear integer"        >*/
  LIGMA_PRECISION_U32_NON_LINEAR    = 350, /*< desc="32-bit non-linear integer"         >*/
  LIGMA_PRECISION_U32_PERCEPTUAL    = 375, /*< desc="32-bit perceptual integer"         >*/
  LIGMA_PRECISION_HALF_LINEAR       = 500, /*< desc="16-bit linear floating point" >*/
  LIGMA_PRECISION_HALF_NON_LINEAR   = 550, /*< desc="16-bit non-linear floating point"  >*/
  LIGMA_PRECISION_HALF_PERCEPTUAL   = 575, /*< desc="16-bit perceptual floating point"  >*/
  LIGMA_PRECISION_FLOAT_LINEAR      = 600, /*< desc="32-bit linear floating point" >*/
  LIGMA_PRECISION_FLOAT_NON_LINEAR  = 650, /*< desc="32-bit non-linear floating point"  >*/
  LIGMA_PRECISION_FLOAT_PERCEPTUAL  = 675, /*< desc="32-bit perceptual floating point"  >*/
  LIGMA_PRECISION_DOUBLE_LINEAR     = 700, /*< desc="64-bit linear floating point" >*/
  LIGMA_PRECISION_DOUBLE_NON_LINEAR = 750, /*< desc="64-bit non-linear floating point"  >*/
  LIGMA_PRECISION_DOUBLE_PERCEPTUAL = 775, /*< desc="64-bit perceptual floating point"  >*/

#ifndef LIGMA_DISABLE_DEPRECATED
  LIGMA_PRECISION_U8_GAMMA      = LIGMA_PRECISION_U8_NON_LINEAR,
  LIGMA_PRECISION_U16_GAMMA     = LIGMA_PRECISION_U16_NON_LINEAR,
  LIGMA_PRECISION_U32_GAMMA     = LIGMA_PRECISION_U32_NON_LINEAR,
  LIGMA_PRECISION_HALF_GAMMA    = LIGMA_PRECISION_HALF_NON_LINEAR,
  LIGMA_PRECISION_FLOAT_GAMMA   = LIGMA_PRECISION_FLOAT_NON_LINEAR,
  LIGMA_PRECISION_DOUBLE_GAMMA  = LIGMA_PRECISION_DOUBLE_NON_LINEAR
#endif
} LigmaPrecision;


/**
 * LigmaProcedureSensitivityMask:
 * @LIGMA_PROCEDURE_SENSITIVE_DRAWABLE:     Handles image with one selected drawable.
 * @LIGMA_PROCEDURE_SENSITIVE_DRAWABLES:    Handles image with several selected drawables.
 * @LIGMA_PROCEDURE_SENSITIVE_NO_DRAWABLES: Handles image with no selected drawables.
 * @LIGMA_PROCEDURE_SENSITIVE_NO_IMAGE:     Handles no image.
 *
 * The cases when a #LigmaProcedure should be shown as sensitive.
 **/
#define LIGMA_TYPE_PROCEDURE_SENSITIVITY_MASK (ligma_procedure_sensitivity_mask_get_type ())

GType ligma_procedure_sensitivity_mask_get_type (void) G_GNUC_CONST;
typedef enum  /*< pdb-skip >*/
{
  LIGMA_PROCEDURE_SENSITIVE_DRAWABLE      = 1 << 0,
  LIGMA_PROCEDURE_SENSITIVE_DRAWABLES     = 1 << 2,
  LIGMA_PROCEDURE_SENSITIVE_NO_DRAWABLES  = 1 << 3,
  LIGMA_PROCEDURE_SENSITIVE_NO_IMAGE      = 1 << 4,
  LIGMA_PROCEDURE_SENSITIVE_ALWAYS        = G_MAXINT
} LigmaProcedureSensitivityMask;


/**
 * LigmaProgressCommand:
 * @LIGMA_PROGRESS_COMMAND_START:      Start a progress
 * @LIGMA_PROGRESS_COMMAND_END:        End the progress
 * @LIGMA_PROGRESS_COMMAND_SET_TEXT:   Set the text
 * @LIGMA_PROGRESS_COMMAND_SET_VALUE:  Set the percentage
 * @LIGMA_PROGRESS_COMMAND_PULSE:      Pulse the progress
 * @LIGMA_PROGRESS_COMMAND_GET_WINDOW: Get the window where the progress is shown
 *
 * Commands for the progress API.
 **/
#define LIGMA_TYPE_PROGRESS_COMMAND (ligma_progress_command_get_type ())

GType ligma_progress_command_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_PROGRESS_COMMAND_START,
  LIGMA_PROGRESS_COMMAND_END,
  LIGMA_PROGRESS_COMMAND_SET_TEXT,
  LIGMA_PROGRESS_COMMAND_SET_VALUE,
  LIGMA_PROGRESS_COMMAND_PULSE,
  LIGMA_PROGRESS_COMMAND_GET_WINDOW
} LigmaProgressCommand;


/**
 * LigmaRepeatMode:
 * @LIGMA_REPEAT_NONE:       None (extend)
 * @LIGMA_REPEAT_SAWTOOTH:   Sawtooth wave
 * @LIGMA_REPEAT_TRIANGULAR: Triangular wave
 * @LIGMA_REPEAT_TRUNCATE:   Truncate
 *
 * Repeat modes for example for gradients.
 **/
#define LIGMA_TYPE_REPEAT_MODE (ligma_repeat_mode_get_type ())

GType ligma_repeat_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_REPEAT_NONE,       /*< desc="None (extend)"   >*/
  LIGMA_REPEAT_SAWTOOTH,   /*< desc="Sawtooth wave"   >*/
  LIGMA_REPEAT_TRIANGULAR, /*< desc="Triangular wave" >*/
  LIGMA_REPEAT_TRUNCATE    /*< desc="Truncate"        >*/
} LigmaRepeatMode;


/**
 * LigmaRotationType:
 * @LIGMA_ROTATE_90:  90 degrees
 * @LIGMA_ROTATE_180: 180 degrees
 * @LIGMA_ROTATE_270: 270 degrees
 *
 * Types of simple rotations.
 **/
#define LIGMA_TYPE_ROTATION_TYPE (ligma_rotation_type_get_type ())

GType ligma_rotation_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_ROTATE_90,
  LIGMA_ROTATE_180,
  LIGMA_ROTATE_270
} LigmaRotationType;


/**
 * LigmaRunMode:
 * @LIGMA_RUN_INTERACTIVE:    Run interactively
 * @LIGMA_RUN_NONINTERACTIVE: Run non-interactively
 * @LIGMA_RUN_WITH_LAST_VALS: Run with last used values
 *
 * Run modes for plug-ins.
 **/
#define LIGMA_TYPE_RUN_MODE (ligma_run_mode_get_type ())

GType ligma_run_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_RUN_INTERACTIVE,     /*< desc="Run interactively"         >*/
  LIGMA_RUN_NONINTERACTIVE,  /*< desc="Run non-interactively"     >*/
  LIGMA_RUN_WITH_LAST_VALS   /*< desc="Run with last used values" >*/
} LigmaRunMode;


/**
 * LigmaSelectCriterion:
 * @LIGMA_SELECT_CRITERION_COMPOSITE:      Composite
 * @LIGMA_SELECT_CRITERION_RGB_RED:        Red
 * @LIGMA_SELECT_CRITERION_RGB_GREEN:      Green
 * @LIGMA_SELECT_CRITERION_RGB_BLUE:       Blue
 * @LIGMA_SELECT_CRITERION_HSV_HUE:        HSV Hue
 * @LIGMA_SELECT_CRITERION_HSV_SATURATION: HSV Saturation
 * @LIGMA_SELECT_CRITERION_HSV_VALUE:      HSV Value
 * @LIGMA_SELECT_CRITERION_LCH_LIGHTNESS:  LCh Lightness
 * @LIGMA_SELECT_CRITERION_LCH_CHROMA:     LCh Chroma
 * @LIGMA_SELECT_CRITERION_LCH_HUE:        LCh Hue
 * @LIGMA_SELECT_CRITERION_ALPHA:          Alpha
 *
 * Criterions for color similarity.
 **/
#define LIGMA_TYPE_SELECT_CRITERION (ligma_select_criterion_get_type ())

GType ligma_select_criterion_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_SELECT_CRITERION_COMPOSITE,      /*< desc="Composite"      >*/
  LIGMA_SELECT_CRITERION_RGB_RED,        /*< desc="Red"            >*/
  LIGMA_SELECT_CRITERION_RGB_GREEN,      /*< desc="Green"          >*/
  LIGMA_SELECT_CRITERION_RGB_BLUE,       /*< desc="Blue"           >*/
  LIGMA_SELECT_CRITERION_HSV_HUE,        /*< desc="HSV Hue"        >*/
  LIGMA_SELECT_CRITERION_HSV_SATURATION, /*< desc="HSV Saturation" >*/
  LIGMA_SELECT_CRITERION_HSV_VALUE,      /*< desc="HSV Value"      >*/
  LIGMA_SELECT_CRITERION_LCH_LIGHTNESS,  /*< desc="LCh Lightness"  >*/
  LIGMA_SELECT_CRITERION_LCH_CHROMA,     /*< desc="LCh Chroma"     >*/
  LIGMA_SELECT_CRITERION_LCH_HUE,        /*< desc="LCh Hue"        >*/
  LIGMA_SELECT_CRITERION_ALPHA,          /*< desc="Alpha"          >*/
} LigmaSelectCriterion;


/**
 * LigmaSizeType:
 * @LIGMA_PIXELS: Pixels
 * @LIGMA_POINTS: Points
 *
 * Size types for the old-style text API.
 **/
#define LIGMA_TYPE_SIZE_TYPE (ligma_size_type_get_type ())

GType ligma_size_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_PIXELS,  /*< desc="Pixels" >*/
  LIGMA_POINTS   /*< desc="Points" >*/
} LigmaSizeType;


/**
 * LigmaStackTraceMode:
 * @LIGMA_STACK_TRACE_NEVER:  Never
 * @LIGMA_STACK_TRACE_QUERY:  Ask each time
 * @LIGMA_STACK_TRACE_ALWAYS: Always
 *
 * When to generate stack traces in case of an error.
 **/
#define LIGMA_TYPE_STACK_TRACE_MODE (ligma_stack_trace_mode_get_type ())

GType ligma_stack_trace_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_STACK_TRACE_NEVER,
  LIGMA_STACK_TRACE_QUERY,
  LIGMA_STACK_TRACE_ALWAYS
} LigmaStackTraceMode;


/**
 * LigmaStrokeMethod:
 * @LIGMA_STROKE_LINE:         Stroke line
 * @LIGMA_STROKE_PAINT_METHOD: Stroke with a paint tool
 *
 * Methods of stroking selections and paths.
 **/
#define LIGMA_TYPE_STROKE_METHOD (ligma_stroke_method_get_type ())

GType ligma_stroke_method_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_STROKE_LINE,         /*< desc="Stroke line"              >*/
  LIGMA_STROKE_PAINT_METHOD  /*< desc="Stroke with a paint tool" >*/
} LigmaStrokeMethod;


/**
 * LigmaTextDirection:
 * @LIGMA_TEXT_DIRECTION_LTR: From left to right
 * @LIGMA_TEXT_DIRECTION_RTL: From right to left
 * @LIGMA_TEXT_DIRECTION_TTB_RTL: Characters are from top to bottom, Lines are from right to left
 * @LIGMA_TEXT_DIRECTION_TTB_RTL_UPRIGHT: Upright characters are from top to bottom, Lines are from right to left
 * @LIGMA_TEXT_DIRECTION_TTB_LTR: Characters are from top to bottom, Lines are from left to right
 * @LIGMA_TEXT_DIRECTION_TTB_LTR_UPRIGHT: Upright characters are from top to bottom, Lines are from left to right
 *
 * Text directions.
 **/
#define LIGMA_TYPE_TEXT_DIRECTION (ligma_text_direction_get_type ())

GType ligma_text_direction_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_TEXT_DIRECTION_LTR,              /*< desc="From left to right"                                     >*/
  LIGMA_TEXT_DIRECTION_RTL,              /*< desc="From right to left"                                     >*/
  LIGMA_TEXT_DIRECTION_TTB_RTL,          /*< desc="Vertical, right to left (mixed orientation)"  >*/
  LIGMA_TEXT_DIRECTION_TTB_RTL_UPRIGHT,  /*< desc="Vertical, right to left (upright orientation)" >*/
  LIGMA_TEXT_DIRECTION_TTB_LTR,          /*< desc="Vertical, left to right (mixed orientation)"  >*/
  LIGMA_TEXT_DIRECTION_TTB_LTR_UPRIGHT   /*< desc="Vertical, left to right (upright orientation)" >*/
} LigmaTextDirection;


/**
 * LigmaTextHintStyle:
 * @LIGMA_TEXT_HINT_STYLE_NONE:   None
 * @LIGMA_TEXT_HINT_STYLE_SLIGHT: Slight
 * @LIGMA_TEXT_HINT_STYLE_MEDIUM: Medium
 * @LIGMA_TEXT_HINT_STYLE_FULL:   Full
 *
 * Text hint strengths.
 **/
#define LIGMA_TYPE_TEXT_HINT_STYLE (ligma_text_hint_style_get_type ())

GType ligma_text_hint_style_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_TEXT_HINT_STYLE_NONE,     /*< desc="None"   >*/
  LIGMA_TEXT_HINT_STYLE_SLIGHT,   /*< desc="Slight" >*/
  LIGMA_TEXT_HINT_STYLE_MEDIUM,   /*< desc="Medium" >*/
  LIGMA_TEXT_HINT_STYLE_FULL      /*< desc="Full"   >*/
} LigmaTextHintStyle;


/**
 * LigmaTextJustification:
 * @LIGMA_TEXT_JUSTIFY_LEFT:   Left justified
 * @LIGMA_TEXT_JUSTIFY_RIGHT:  Right justified
 * @LIGMA_TEXT_JUSTIFY_CENTER: Centered
 * @LIGMA_TEXT_JUSTIFY_FILL:   Filled
 *
 * Text justifications.
 **/
#define LIGMA_TYPE_TEXT_JUSTIFICATION (ligma_text_justification_get_type ())

GType ligma_text_justification_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_TEXT_JUSTIFY_LEFT,    /*< desc="Left justified"  >*/
  LIGMA_TEXT_JUSTIFY_RIGHT,   /*< desc="Right justified" >*/
  LIGMA_TEXT_JUSTIFY_CENTER,  /*< desc="Centered"        >*/
  LIGMA_TEXT_JUSTIFY_FILL     /*< desc="Filled"          >*/
} LigmaTextJustification;


/**
 * LigmaTransferMode:
 * @LIGMA_TRANSFER_SHADOWS:    Shadows
 * @LIGMA_TRANSFER_MIDTONES:   Midtones
 * @LIGMA_TRANSFER_HIGHLIGHTS: Highlights
 *
 * For choosing which brightness ranges to transform.
 **/
#define LIGMA_TYPE_TRANSFER_MODE (ligma_transfer_mode_get_type ())

GType ligma_transfer_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_TRANSFER_SHADOWS,     /*< desc="Shadows"    >*/
  LIGMA_TRANSFER_MIDTONES,    /*< desc="Midtones"   >*/
  LIGMA_TRANSFER_HIGHLIGHTS   /*< desc="Highlights" >*/
} LigmaTransferMode;


/**
 * LigmaTransformDirection:
 * @LIGMA_TRANSFORM_FORWARD:  Normal (Forward)
 * @LIGMA_TRANSFORM_BACKWARD: Corrective (Backward)
 *
 * Transform directions.
 **/
#define LIGMA_TYPE_TRANSFORM_DIRECTION (ligma_transform_direction_get_type ())

GType ligma_transform_direction_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_TRANSFORM_FORWARD,   /*< desc="Normal (Forward)" >*/
  LIGMA_TRANSFORM_BACKWARD   /*< desc="Corrective (Backward)" >*/
} LigmaTransformDirection;


/**
 * LigmaTransformResize:
 * @LIGMA_TRANSFORM_RESIZE_ADJUST:           Adjust
 * @LIGMA_TRANSFORM_RESIZE_CLIP:             Clip
 * @LIGMA_TRANSFORM_RESIZE_CROP:             Crop to result
 * @LIGMA_TRANSFORM_RESIZE_CROP_WITH_ASPECT: Crop with aspect
 *
 * Ways of clipping the result when transforming drawables.
 **/
#define LIGMA_TYPE_TRANSFORM_RESIZE (ligma_transform_resize_get_type ())

GType ligma_transform_resize_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_TRANSFORM_RESIZE_ADJUST,           /*< desc="Adjust"           >*/
  LIGMA_TRANSFORM_RESIZE_CLIP,             /*< desc="Clip"             >*/
  LIGMA_TRANSFORM_RESIZE_CROP,             /*< desc="Crop to result"   >*/
  LIGMA_TRANSFORM_RESIZE_CROP_WITH_ASPECT  /*< desc="Crop with aspect" >*/
} LigmaTransformResize;


/**
 * LigmaUnit:
 * @LIGMA_UNIT_PIXEL:   Pixels
 * @LIGMA_UNIT_INCH:    Inches
 * @LIGMA_UNIT_MM:      Millimeters
 * @LIGMA_UNIT_POINT:   Points
 * @LIGMA_UNIT_PICA:    Picas
 * @LIGMA_UNIT_END:     Marker for end-of-builtin-units
 * @LIGMA_UNIT_PERCENT: Pseudo-unit percent
 *
 * Units used for dimensions in images.
 **/
typedef enum /*< skip >*/
{
  LIGMA_UNIT_PIXEL   = 0,

  LIGMA_UNIT_INCH    = 1,
  LIGMA_UNIT_MM      = 2,
  LIGMA_UNIT_POINT   = 3,
  LIGMA_UNIT_PICA    = 4,

  LIGMA_UNIT_END     = 5,

  LIGMA_UNIT_PERCENT = 65536 /*< pdb-skip >*/
} LigmaUnit;


/**
 * LigmaVectorsStrokeType:
 * @LIGMA_VECTORS_STROKE_TYPE_BEZIER: A bezier stroke
 *
 * Possible type of strokes in vectors objects.
 **/
#define LIGMA_TYPE_VECTORS_STROKE_TYPE (ligma_vectors_stroke_type_get_type ())

GType ligma_vectors_stroke_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_VECTORS_STROKE_TYPE_BEZIER
} LigmaVectorsStrokeType;

G_END_DECLS

#endif  /* __LIGMA_BASE_ENUMS_H__ */
