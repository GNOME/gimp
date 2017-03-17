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
  GIMP_ADD_MASK_CHANNEL,        /*< desc="C_hannel"                        >*/

#ifndef GIMP_DISABLE_DEPRECATED
  GIMP_ADD_WHITE_MASK          = GIMP_ADD_MASK_WHITE,     /*< skip, pdb-skip >*/
  GIMP_ADD_BLACK_MASK          = GIMP_ADD_MASK_BLACK,     /*< skip, pdb-skip >*/
  GIMP_ADD_ALPHA_MASK          = GIMP_ADD_MASK_ALPHA,     /*< skip, pdb-skip >*/
  GIMP_ADD_ALPHA_TRANSFER_MASK = GIMP_ADD_MASK_ALPHA_TRANSFER, /*< skip, pdb-skip >*/
  GIMP_ADD_SELECTION_MASK      = GIMP_ADD_MASK_SELECTION, /*< skip, pdb-skip >*/
  GIMP_ADD_COPY_MASK           = GIMP_ADD_MASK_COPY,      /*< skip, pdb-skip >*/
  GIMP_ADD_CHANNEL_MASK        = GIMP_ADD_MASK_CHANNEL    /*< skip, pdb-skip >*/
#endif /* GIMP_DISABLE_DEPRECATED */
} GimpAddMaskType;


#define GIMP_TYPE_BLEND_MODE (gimp_blend_mode_get_type ())

GType gimp_blend_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_BLEND_FG_BG_RGB,      /*< desc="FG to BG (RGB)"    >*/
  GIMP_BLEND_FG_BG_HSV,      /*< desc="FG to BG (HSV)"    >*/
  GIMP_BLEND_FG_TRANSPARENT, /*< desc="FG to transparent" >*/
  GIMP_BLEND_CUSTOM,         /*< desc="Custom gradient"   >*/

#ifndef GIMP_DISABLE_DEPRECATED
  GIMP_FG_BG_RGB_MODE      = GIMP_BLEND_FG_BG_RGB,      /*< skip, pdb-skip >*/
  GIMP_FG_BG_HSV_MODE      = GIMP_BLEND_FG_BG_HSV,      /*< skip, pdb-skip >*/
  GIMP_FG_TRANSPARENT_MODE = GIMP_BLEND_FG_TRANSPARENT, /*< skip, pdb-skip >*/
  GIMP_CUSTOM_MODE         = GIMP_BLEND_CUSTOM          /*< skip, pdb-skip >*/
#endif /* GIMP_DISABLE_DEPRECATED */
} GimpBlendMode;


#define GIMP_TYPE_BRUSH_GENERATED_SHAPE (gimp_brush_generated_shape_get_type ())

GType gimp_brush_generated_shape_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_BRUSH_GENERATED_CIRCLE,  /*< desc="Circle"  >*/
  GIMP_BRUSH_GENERATED_SQUARE,  /*< desc="Square"  >*/
  GIMP_BRUSH_GENERATED_DIAMOND  /*< desc="Diamond" >*/
} GimpBrushGeneratedShape;


#define GIMP_TYPE_BUCKET_FILL_MODE (gimp_bucket_fill_mode_get_type ())

GType gimp_bucket_fill_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_BUCKET_FILL_FG,      /*< desc="FG color fill" >*/
  GIMP_BUCKET_FILL_BG,      /*< desc="BG color fill" >*/
  GIMP_BUCKET_FILL_PATTERN, /*< desc="Pattern fill"  >*/

#ifndef GIMP_DISABLE_DEPRECATED
  GIMP_FG_BUCKET_FILL      = GIMP_BUCKET_FILL_FG,     /*< skip, pdb-skip >*/
  GIMP_BG_BUCKET_FILL      = GIMP_BUCKET_FILL_BG,     /*< skip, pdb-skip >*/
  GIMP_PATTERN_BUCKET_FILL = GIMP_BUCKET_FILL_PATTERN /*< skip, pdb-skip >*/
#endif /* GIMP_DISABLE_DEPRECATED */
} GimpBucketFillMode;


#define GIMP_TYPE_CAP_STYLE (gimp_cap_style_get_type ())

GType gimp_cap_style_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_CAP_BUTT,   /*< desc="Butt"   >*/
  GIMP_CAP_ROUND,  /*< desc="Round"  >*/
  GIMP_CAP_SQUARE  /*< desc="Square" >*/
} GimpCapStyle;


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
  GIMP_CHANNEL_RED,      /*< desc="Red"     >*/
  GIMP_CHANNEL_GREEN,    /*< desc="Green"   >*/
  GIMP_CHANNEL_BLUE,     /*< desc="Blue"    >*/
  GIMP_CHANNEL_GRAY,     /*< desc="Gray"    >*/
  GIMP_CHANNEL_INDEXED,  /*< desc="Indexed" >*/
  GIMP_CHANNEL_ALPHA,    /*< desc="Alpha"   >*/

#ifndef GIMP_DISABLE_DEPRECATED
  GIMP_RED_CHANNEL     = GIMP_CHANNEL_RED,     /*< skip, pdb-skip >*/
  GIMP_GREEN_CHANNEL   = GIMP_CHANNEL_GREEN,   /*< skip, pdb-skip >*/
  GIMP_BLUE_CHANNEL    = GIMP_CHANNEL_BLUE,    /*< skip, pdb-skip >*/
  GIMP_GRAY_CHANNEL    = GIMP_CHANNEL_GRAY,    /*< skip, pdb-skip >*/
  GIMP_INDEXED_CHANNEL = GIMP_CHANNEL_INDEXED, /*< skip, pdb-skip >*/
  GIMP_ALPHA_CHANNEL   = GIMP_CHANNEL_ALPHA    /*< skip, pdb-skip >*/
#endif /* GIMP_DISABLE_DEPRECATED */
} GimpChannelType;


#define GIMP_TYPE_CHECK_SIZE (gimp_check_size_get_type ())

GType gimp_check_size_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_CHECK_SIZE_SMALL_CHECKS  = 0,  /*< desc="Small"  >*/
  GIMP_CHECK_SIZE_MEDIUM_CHECKS = 1,  /*< desc="Medium" >*/
  GIMP_CHECK_SIZE_LARGE_CHECKS  = 2   /*< desc="Large"  >*/
} GimpCheckSize;


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


#define GIMP_TYPE_CLONE_TYPE (gimp_clone_type_get_type ())

GType gimp_clone_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_CLONE_IMAGE,    /*< desc="Image"   >*/
  GIMP_CLONE_PATTERN,  /*< desc="Pattern" >*/

#ifndef GIMP_DISABLE_DEPRECATED
  GIMP_IMAGE_CLONE   = GIMP_CLONE_IMAGE,  /*< skip, pdb-skip >*/
  GIMP_PATTERN_CLONE = GIMP_CLONE_PATTERN /*< skip, pdb-skip >*/
#endif /* GIMP_DISABLE_DEPRECATED */
} GimpCloneType;


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


#define GIMP_TYPE_CONVERT_PALETTE_TYPE (gimp_convert_palette_type_get_type ())

GType gimp_convert_palette_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_CONVERT_PALETTE_GENERATE, /*< desc="Generate optimum palette"          >*/
  GIMP_CONVERT_PALETTE_REUSE,    /*< skip >*/
  GIMP_CONVERT_PALETTE_WEB,      /*< desc="Use web-optimized palette"         >*/
  GIMP_CONVERT_PALETTE_MONO,     /*< desc="Use black and white (1-bit) palette" >*/
  GIMP_CONVERT_PALETTE_CUSTOM,   /*< desc="Use custom palette"                >*/

#ifndef GIMP_DISABLE_DEPRECATED
  GIMP_MAKE_PALETTE   = GIMP_CONVERT_PALETTE_GENERATE, /*< skip, pdb-skip >*/
  GIMP_REUSE_PALETTE  = GIMP_CONVERT_PALETTE_REUSE,    /*< skip, pdb-skip >*/
  GIMP_WEB_PALETTE    = GIMP_CONVERT_PALETTE_WEB,      /*< skip, pdb-skip >*/
  GIMP_MONO_PALETTE   = GIMP_CONVERT_PALETTE_MONO,     /*< skip, pdb-skip >*/
  GIMP_CUSTOM_PALETTE = GIMP_CONVERT_PALETTE_CUSTOM    /*< skip, pdb-skip >*/
#endif /* GIMP_DISABLE_DEPRECATED */
} GimpConvertPaletteType;


#define GIMP_TYPE_CONVOLVE_TYPE (gimp_convolve_type_get_type ())

GType gimp_convolve_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_CONVOLVE_BLUR,    /*< desc="Blur"    >*/
  GIMP_CONVOLVE_SHARPEN, /*< desc="Sharpen" >*/

#ifndef GIMP_DISABLE_DEPRECATED
  GIMP_BLUR_CONVOLVE    = GIMP_CONVOLVE_BLUR,   /*< skip, pdb-skip >*/
  GIMP_SHARPEN_CONVOLVE = GIMP_CONVOLVE_SHARPEN /*< skip, pdb-skip >*/
#endif /* GIMP_DISABLE_DEPRECATED */
} GimpConvolveType;


#define GIMP_TYPE_DESATURATE_MODE (gimp_desaturate_mode_get_type ())

GType gimp_desaturate_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_DESATURATE_LIGHTNESS,   /*< desc="Lightness (HSL)"          >*/
  GIMP_DESATURATE_LUMA,        /*< desc="Luma"                     >*/
  GIMP_DESATURATE_AVERAGE,     /*< desc="Average (HSI Intensity)"  >*/
  GIMP_DESATURATE_LUMINANCE,   /*< desc="Luminance"                >*/
  GIMP_DESATURATE_VALUE,       /*< desc="Value (HSV)"              >*/

#ifndef GIMP_DISABLE_DEPRECATED
  GIMP_DESATURATE_LUMINOSITY = GIMP_DESATURATE_LUMA /*< skip, pdb-skip >*/
#endif /* GIMP_DISABLE_DEPRECATED */
} GimpDesaturateMode;


#define GIMP_TYPE_DODGE_BURN_TYPE (gimp_dodge_burn_type_get_type ())

GType gimp_dodge_burn_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_DODGE_BURN_TYPE_DODGE,  /*< desc="Dodge" >*/
  GIMP_DODGE_BURN_TYPE_BURN,   /*< desc="Burn"  >*/

#ifndef GIMP_DISABLE_DEPRECATED
  GIMP_DODGE = GIMP_DODGE_BURN_TYPE_DODGE, /*< skip, pdb-skip >*/
  GIMP_BURN  = GIMP_DODGE_BURN_TYPE_BURN   /*< skip, pdb-skip >*/
#endif /* GIMP_DISABLE_DEPRECATED */
} GimpDodgeBurnType;


#define GIMP_TYPE_FILL_TYPE (gimp_fill_type_get_type ())

GType gimp_fill_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_FILL_FOREGROUND,  /*< desc="Foreground color" >*/
  GIMP_FILL_BACKGROUND,  /*< desc="Background color" >*/
  GIMP_FILL_WHITE,       /*< desc="White"            >*/
  GIMP_FILL_TRANSPARENT, /*< desc="Transparency"     >*/
  GIMP_FILL_PATTERN,     /*< desc="Pattern"          >*/

#ifndef GIMP_DISABLE_DEPRECATED
  GIMP_FOREGROUND_FILL  = GIMP_FILL_FOREGROUND,  /*< skip, pdb-skip >*/
  GIMP_BACKGROUND_FILL  = GIMP_FILL_BACKGROUND,  /*< skip, pdb-skip >*/
  GIMP_WHITE_FILL       = GIMP_FILL_WHITE,       /*< skip, pdb-skip >*/
  GIMP_TRANSPARENT_FILL = GIMP_FILL_TRANSPARENT, /*< skip, pdb-skip >*/
  GIMP_PATTERN_FILL     = GIMP_FILL_PATTERN      /*< skip, pdb-skip >*/
#endif /* GIMP_DISABLE_DEPRECATED */
} GimpFillType;


#define GIMP_TYPE_FOREGROUND_EXTRACT_MODE (gimp_foreground_extract_mode_get_type ())

GType gimp_foreground_extract_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_FOREGROUND_EXTRACT_SIOX,
  GIMP_FOREGROUND_EXTRACT_MATTING
} GimpForegroundExtractMode;


#define GIMP_TYPE_GRADIENT_SEGMENT_COLOR (gimp_gradient_segment_color_get_type ())

GType gimp_gradient_segment_color_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_GRADIENT_SEGMENT_RGB,      /* normal RGB           */
  GIMP_GRADIENT_SEGMENT_HSV_CCW,  /* counterclockwise hue */
  GIMP_GRADIENT_SEGMENT_HSV_CW    /* clockwise hue        */
} GimpGradientSegmentColor;


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

typedef enum
{
  GIMP_GRID_DOTS,           /*< desc="Intersections (dots)"       >*/
  GIMP_GRID_INTERSECTIONS,  /*< desc="Intersections (crosshairs)" >*/
  GIMP_GRID_ON_OFF_DASH,    /*< desc="Dashed"                     >*/
  GIMP_GRID_DOUBLE_DASH,    /*< desc="Double dashed"              >*/
  GIMP_GRID_SOLID           /*< desc="Solid"                      >*/
} GimpGridStyle;


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
  GIMP_HUE_RANGE_MAGENTA,

#ifndef GIMP_DISABLE_DEPRECATED
  GIMP_ALL_HUES     = GIMP_HUE_RANGE_ALL,    /*< skip, pdb-skip >*/
  GIMP_RED_HUES     = GIMP_HUE_RANGE_RED,    /*< skip, pdb-skip >*/
  GIMP_YELLOW_HUES  = GIMP_HUE_RANGE_YELLOW, /*< skip, pdb-skip >*/
  GIMP_GREEN_HUES   = GIMP_HUE_RANGE_GREEN,  /*< skip, pdb-skip >*/
  GIMP_CYAN_HUES    = GIMP_HUE_RANGE_CYAN,   /*< skip, pdb-skip >*/
  GIMP_BLUE_HUES    = GIMP_HUE_RANGE_BLUE,   /*< skip, pdb-skip >*/
  GIMP_MAGENTA_HUES = GIMP_HUE_RANGE_MAGENTA /*< skip, pdb-skip >*/
#endif /* GIMP_DISABLE_DEPRECATED */
} GimpHueRange;


#define GIMP_TYPE_ICON_TYPE (gimp_icon_type_get_type ())

GType gimp_icon_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_ICON_TYPE_ICON_NAME,     /*< desc="Icon name"     >*/
  GIMP_ICON_TYPE_INLINE_PIXBUF, /*< desc="Inline pixbuf" >*/
  GIMP_ICON_TYPE_IMAGE_FILE,    /*< desc="Image file"    >*/

#ifndef GIMP_DISABLE_DEPRECATED
  GIMP_ICON_TYPE_STOCK_ID = GIMP_ICON_TYPE_ICON_NAME  /*< skip, pdb-skip >*/
#endif /* GIMP_DISABLE_DEPRECATED */
} GimpIconType;


#define GIMP_TYPE_IMAGE_BASE_TYPE (gimp_image_base_type_get_type ())

GType gimp_image_base_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_RGB,     /*< desc="RGB color"     >*/
  GIMP_GRAY,    /*< desc="Grayscale"     >*/
  GIMP_INDEXED  /*< desc="Indexed color" >*/
} GimpImageBaseType;


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


#define GIMP_TYPE_INK_BLOB_TYPE (gimp_ink_blob_type_get_type ())

GType gimp_ink_blob_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_INK_BLOB_TYPE_CIRCLE,  /*< desc="Circle"  >*/
  GIMP_INK_BLOB_TYPE_SQUARE,  /*< desc="Square"  >*/
  GIMP_INK_BLOB_TYPE_DIAMOND  /*< desc="Diamond" >*/
} GimpInkBlobType;


#define GIMP_TYPE_INTERPOLATION_TYPE (gimp_interpolation_type_get_type ())

GType gimp_interpolation_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_INTERPOLATION_NONE,   /*< desc="None"   >*/
  GIMP_INTERPOLATION_LINEAR, /*< desc="Linear" >*/
  GIMP_INTERPOLATION_CUBIC,  /*< desc="Cubic"  >*/
  GIMP_INTERPOLATION_NOHALO, /*< desc="NoHalo" >*/
  GIMP_INTERPOLATION_LOHALO, /*< desc="LoHalo" >*/

#ifndef GIMP_DISABLE_DEPRECATED
  GIMP_INTERPOLATION_LANCZOS = GIMP_INTERPOLATION_NOHALO /*< skip, pdb-skip >*/
#endif /* GIMP_DISABLE_DEPRECATED */
} GimpInterpolationType;


#define GIMP_TYPE_JOIN_STYLE (gimp_join_style_get_type ())

GType gimp_join_style_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_JOIN_MITER,  /*< desc="Miter" >*/
  GIMP_JOIN_ROUND,  /*< desc="Round" >*/
  GIMP_JOIN_BEVEL   /*< desc="Bevel" >*/
} GimpJoinStyle;


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
  GIMP_EXPAND_AS_NECESSARY,  /*< desc="Expanded as necessary"  >*/
  GIMP_CLIP_TO_IMAGE,        /*< desc="Clipped to image"        >*/
  GIMP_CLIP_TO_BOTTOM_LAYER, /*< desc="Clipped to bottom layer" >*/
  GIMP_FLATTEN_IMAGE         /*< desc="Flatten"                 >*/
} GimpMergeType;


#define GIMP_TYPE_MESSAGE_HANDLER_TYPE (gimp_message_handler_type_get_type ())

GType gimp_message_handler_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_MESSAGE_BOX,
  GIMP_CONSOLE,
  GIMP_ERROR_CONSOLE
} GimpMessageHandlerType;


#define GIMP_TYPE_OFFSET_TYPE (gimp_offset_type_get_type ())

GType gimp_offset_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_OFFSET_BACKGROUND,
  GIMP_OFFSET_TRANSPARENT
} GimpOffsetType;


#define GIMP_TYPE_ORIENTATION_TYPE (gimp_orientation_type_get_type ())

GType gimp_orientation_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_ORIENTATION_HORIZONTAL, /*< desc="Horizontal" >*/
  GIMP_ORIENTATION_VERTICAL,   /*< desc="Vertical"   >*/
  GIMP_ORIENTATION_UNKNOWN     /*< desc="Unknown"    >*/
} GimpOrientationType;


#define GIMP_TYPE_PAINT_APPLICATION_MODE (gimp_paint_application_mode_get_type ())

GType gimp_paint_application_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_PAINT_CONSTANT,    /*< desc="Constant"    >*/
  GIMP_PAINT_INCREMENTAL  /*< desc="Incremental" >*/
} GimpPaintApplicationMode;


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
  GIMP_PDB_END,

#ifndef GIMP_DISABLE_DEPRECATED
  GIMP_PDB_PATH     = GIMP_PDB_VECTORS,     /*< skip >*/
  GIMP_PDB_BOUNDARY = GIMP_PDB_COLORARRAY,  /*< skip >*/
  GIMP_PDB_REGION   = GIMP_PDB_ITEM         /*< skip >*/
#endif /* GIMP_DISABLE_DEPRECATED */
} GimpPDBArgType;


#define GIMP_TYPE_PDB_ERROR_HANDLER (gimp_pdb_error_handler_get_type ())

GType gimp_pdb_error_handler_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_PDB_ERROR_HANDLER_INTERNAL,
  GIMP_PDB_ERROR_HANDLER_PLUGIN
} GimpPDBErrorHandler;


#define GIMP_TYPE_PDB_PROC_TYPE (gimp_pdb_proc_type_get_type ())

GType gimp_pdb_proc_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_INTERNAL,   /*< desc="Internal GIMP procedure" >*/
  GIMP_PLUGIN,     /*< desc="GIMP Plug-In" >*/
  GIMP_EXTENSION,  /*< desc="GIMP Extension" >*/
  GIMP_TEMPORARY   /*< desc="Temporary Procedure" >*/
} GimpPDBProcType;


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


#define GIMP_TYPE_REPEAT_MODE (gimp_repeat_mode_get_type ())

GType gimp_repeat_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_REPEAT_NONE,       /*< desc="None (extend)"   >*/
  GIMP_REPEAT_SAWTOOTH,   /*< desc="Sawtooth wave"   >*/
  GIMP_REPEAT_TRIANGULAR, /*< desc="Triangular wave" >*/
  GIMP_REPEAT_TRUNCATE    /*< desc="Truncate"        >*/
} GimpRepeatMode;


#define GIMP_TYPE_ROTATION_TYPE (gimp_rotation_type_get_type ())

GType gimp_rotation_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_ROTATE_90,
  GIMP_ROTATE_180,
  GIMP_ROTATE_270
} GimpRotationType;


#define GIMP_TYPE_RUN_MODE (gimp_run_mode_get_type ())

GType gimp_run_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_RUN_INTERACTIVE,     /*< desc="Run interactively"         >*/
  GIMP_RUN_NONINTERACTIVE,  /*< desc="Run non-interactively"     >*/
  GIMP_RUN_WITH_LAST_VALS   /*< desc="Run with last used values" >*/
} GimpRunMode;


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
  GIMP_SELECT_CRITERION_V,          /*< desc="Value"      >*/
  GIMP_SELECT_CRITERION_A           /*< desc="Alpha"      >*/
} GimpSelectCriterion;


#define GIMP_TYPE_SIZE_TYPE (gimp_size_type_get_type ())

GType gimp_size_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_PIXELS,  /*< desc="Pixels" >*/
  GIMP_POINTS   /*< desc="Points" >*/
} GimpSizeType;


#define GIMP_TYPE_STACK_TRACE_MODE (gimp_stack_trace_mode_get_type ())

GType gimp_stack_trace_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_STACK_TRACE_NEVER,
  GIMP_STACK_TRACE_QUERY,
  GIMP_STACK_TRACE_ALWAYS
} GimpStackTraceMode;


#define GIMP_TYPE_STROKE_METHOD (gimp_stroke_method_get_type ())

GType gimp_stroke_method_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_STROKE_LINE,         /*< desc="Stroke line"              >*/
  GIMP_STROKE_PAINT_METHOD  /*< desc="Stroke with a paint tool" >*/
} GimpStrokeMethod;


#define GIMP_TYPE_TEXT_DIRECTION (gimp_text_direction_get_type ())

GType gimp_text_direction_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_TEXT_DIRECTION_LTR,   /*< desc="From left to right" >*/
  GIMP_TEXT_DIRECTION_RTL    /*< desc="From right to left" >*/
} GimpTextDirection;


#define GIMP_TYPE_TEXT_HINT_STYLE (gimp_text_hint_style_get_type ())

GType gimp_text_hint_style_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_TEXT_HINT_STYLE_NONE,     /*< desc="None"   >*/
  GIMP_TEXT_HINT_STYLE_SLIGHT,   /*< desc="Slight" >*/
  GIMP_TEXT_HINT_STYLE_MEDIUM,   /*< desc="Medium" >*/
  GIMP_TEXT_HINT_STYLE_FULL      /*< desc="Full"   >*/
} GimpTextHintStyle;


#define GIMP_TYPE_TEXT_JUSTIFICATION (gimp_text_justification_get_type ())

GType gimp_text_justification_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_TEXT_JUSTIFY_LEFT,    /*< desc="Left justified"  >*/
  GIMP_TEXT_JUSTIFY_RIGHT,   /*< desc="Right justified" >*/
  GIMP_TEXT_JUSTIFY_CENTER,  /*< desc="Centered"        >*/
  GIMP_TEXT_JUSTIFY_FILL     /*< desc="Filled"          >*/
} GimpTextJustification;


#define GIMP_TYPE_TRANSFER_MODE (gimp_transfer_mode_get_type ())

GType gimp_transfer_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_TRANSFER_SHADOWS,     /*< desc="Shadows"    >*/
  GIMP_TRANSFER_MIDTONES,    /*< desc="Midtones"   >*/
  GIMP_TRANSFER_HIGHLIGHTS,  /*< desc="Highlights" >*/

#ifndef GIMP_DISABLE_DEPRECATED
  GIMP_SHADOWS    = GIMP_TRANSFER_SHADOWS,   /*< skip, pdb-skip >*/
  GIMP_MIDTONES   = GIMP_TRANSFER_MIDTONES,  /*< skip, pdb-skip >*/
  GIMP_HIGHLIGHTS = GIMP_TRANSFER_HIGHLIGHTS /*< skip, pdb-skip >*/
#endif /* GIMP_DISABLE_DEPRECATED */
} GimpTransferMode;


#define GIMP_TYPE_TRANSFORM_DIRECTION (gimp_transform_direction_get_type ())

GType gimp_transform_direction_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_TRANSFORM_FORWARD,   /*< desc="Normal (Forward)" >*/
  GIMP_TRANSFORM_BACKWARD   /*< desc="Corrective (Backward)" >*/
} GimpTransformDirection;


#define GIMP_TYPE_TRANSFORM_RESIZE (gimp_transform_resize_get_type ())

GType gimp_transform_resize_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_TRANSFORM_RESIZE_ADJUST,           /*< desc="Adjust"           >*/
  GIMP_TRANSFORM_RESIZE_CLIP,             /*< desc="Clip"             >*/
  GIMP_TRANSFORM_RESIZE_CROP,             /*< desc="Crop to result"   >*/
  GIMP_TRANSFORM_RESIZE_CROP_WITH_ASPECT  /*< desc="Crop with aspect" >*/
} GimpTransformResize;


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


#ifndef GIMP_DISABLE_DEPRECATED
#define GIMP_TYPE_USER_DIRECTORY (gimp_user_directory_get_type ())

GType gimp_user_directory_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_USER_DIRECTORY_DESKTOP,
  GIMP_USER_DIRECTORY_DOCUMENTS,
  GIMP_USER_DIRECTORY_DOWNLOAD,
  GIMP_USER_DIRECTORY_MUSIC,
  GIMP_USER_DIRECTORY_PICTURES,
  GIMP_USER_DIRECTORY_PUBLIC_SHARE,
  GIMP_USER_DIRECTORY_TEMPLATES,
  GIMP_USER_DIRECTORY_VIDEOS
} GimpUserDirectory;
#endif /* !GIMP_DISABLE_DEPRECATED */


#define GIMP_TYPE_VECTORS_STROKE_TYPE (gimp_vectors_stroke_type_get_type ())

GType gimp_vectors_stroke_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_VECTORS_STROKE_TYPE_BEZIER
} GimpVectorsStrokeType;

G_END_DECLS

#endif  /* __GIMP_BASE_ENUMS_H__ */
