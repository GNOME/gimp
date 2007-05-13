/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_BASE_ENUMS_H__
#define __GIMP_BASE_ENUMS_H__


G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_ADD_MASK_TYPE (gimp_add_mask_type_get_type ())

GType gimp_add_mask_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_ADD_WHITE_MASK,          /*< desc="_White (full opacity)"           >*/
  GIMP_ADD_BLACK_MASK,          /*< desc="_Black (full transparency)"      >*/
  GIMP_ADD_ALPHA_MASK,          /*< desc="Layer's _alpha channel"          >*/
  GIMP_ADD_ALPHA_TRANSFER_MASK, /*< desc="_Transfer layer's alpha channel" >*/
  GIMP_ADD_SELECTION_MASK,      /*< desc="_Selection"                      >*/
  GIMP_ADD_COPY_MASK,           /*< desc="_Grayscale copy of layer"        >*/
  GIMP_ADD_CHANNEL_MASK         /*< desc="C_hannel"                        >*/
} GimpAddMaskType;


#define GIMP_TYPE_BLEND_MODE (gimp_blend_mode_get_type ())

GType gimp_blend_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_FG_BG_RGB_MODE,         /*< desc="FG to BG (RGB)"    >*/
  GIMP_FG_BG_HSV_MODE,         /*< desc="FG to BG (HSV)"    >*/
  GIMP_FG_TRANSPARENT_MODE,    /*< desc="FG to transparent" >*/
  GIMP_CUSTOM_MODE             /*< desc="Custom gradient"   >*/
} GimpBlendMode;


#define GIMP_TYPE_BUCKET_FILL_MODE (gimp_bucket_fill_mode_get_type ())

GType gimp_bucket_fill_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_FG_BUCKET_FILL,      /*< desc="FG color fill" >*/
  GIMP_BG_BUCKET_FILL,      /*< desc="BG color fill" >*/
  GIMP_PATTERN_BUCKET_FILL  /*< desc="Pattern fill"  >*/
} GimpBucketFillMode;


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
  GIMP_RED_CHANNEL,      /*< desc="Red"     >*/
  GIMP_GREEN_CHANNEL,    /*< desc="Green"   >*/
  GIMP_BLUE_CHANNEL,     /*< desc="Blue"    >*/
  GIMP_GRAY_CHANNEL,     /*< desc="Gray"    >*/
  GIMP_INDEXED_CHANNEL,  /*< desc="Indexed" >*/
  GIMP_ALPHA_CHANNEL     /*< desc="Alpha"   >*/
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
  GIMP_IMAGE_CLONE,   /*< desc="Image"   >*/
  GIMP_PATTERN_CLONE  /*< desc="Pattern" >*/
} GimpCloneType;


#define GIMP_TYPE_DESATURATE_MODE (gimp_desaturate_mode_get_type ())

GType gimp_desaturate_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_DESATURATE_LIGHTNESS,   /*< desc="Lightness"  >*/
  GIMP_DESATURATE_LUMINOSITY,  /*< desc="Luminosity" >*/
  GIMP_DESATURATE_AVERAGE      /*< desc="Average"    >*/
} GimpDesaturateMode;


#define GIMP_TYPE_DODGE_BURN_TYPE (gimp_dodge_burn_type_get_type ())

GType gimp_dodge_burn_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_DODGE,  /*< desc="Dodge" >*/
  GIMP_BURN    /*< desc="Burn"  >*/
} GimpDodgeBurnType;


#define GIMP_TYPE_FOREGROUND_EXTRACT_MODE (gimp_foreground_extract_mode_get_type ())

GType gimp_foreground_extract_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_FOREGROUND_EXTRACT_SIOX
} GimpForegroundExtractMode;


#define GIMP_TYPE_GRADIENT_TYPE (gimp_gradient_type_get_type ())

GType gimp_gradient_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_GRADIENT_LINEAR,                /*< desc="gradient|Linear"   >*/
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


#define GIMP_TYPE_ICON_TYPE (gimp_icon_type_get_type ())

GType gimp_icon_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_ICON_TYPE_STOCK_ID,      /*< desc="Stock ID"      >*/
  GIMP_ICON_TYPE_INLINE_PIXBUF, /*< desc="Inline pixbuf" >*/
  GIMP_ICON_TYPE_IMAGE_FILE     /*< desc="Image file"    >*/
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


#define GIMP_TYPE_INTERPOLATION_TYPE (gimp_interpolation_type_get_type ())

GType gimp_interpolation_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_INTERPOLATION_NONE,   /*< desc="interpolation|None"   >*/
  GIMP_INTERPOLATION_LINEAR, /*< desc="interpolation|Linear" >*/
  GIMP_INTERPOLATION_CUBIC,  /*< desc="Cubic"                >*/
  GIMP_INTERPOLATION_LANCZOS /*< desc="Sinc (Lanczos3)"      >*/
} GimpInterpolationType;


#define GIMP_TYPE_PAINT_APPLICATION_MODE (gimp_paint_application_mode_get_type ())

GType gimp_paint_application_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_PAINT_CONSTANT,    /*< desc="Constant"    >*/
  GIMP_PAINT_INCREMENTAL  /*< desc="Incremental" >*/
} GimpPaintApplicationMode;


#define GIMP_TYPE_REPEAT_MODE (gimp_repeat_mode_get_type ())

GType gimp_repeat_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_REPEAT_NONE,       /*< desc="None"            >*/
  GIMP_REPEAT_SAWTOOTH,   /*< desc="Sawtooth wave"   >*/
  GIMP_REPEAT_TRIANGULAR  /*< desc="Triangular wave" >*/
} GimpRepeatMode;


#define GIMP_TYPE_RUN_MODE (gimp_run_mode_get_type ())

GType gimp_run_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_RUN_INTERACTIVE,     /*< desc="Run interactively"         >*/
  GIMP_RUN_NONINTERACTIVE,  /*< desc="Run non-interactively"     >*/
  GIMP_RUN_WITH_LAST_VALS   /*< desc="Run with last used values" >*/
} GimpRunMode;


#define GIMP_TYPE_SIZE_TYPE (gimp_size_type_get_type ())

GType gimp_size_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_PIXELS,  /*< desc="Pixels" >*/
  GIMP_POINTS   /*< desc="Points" >*/
} GimpSizeType;


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

typedef enum
{
  GIMP_TRANSFORM_FORWARD,   /*< desc="Normal (Forward)" >*/
  GIMP_TRANSFORM_BACKWARD   /*< desc="Corrective (Backward)" >*/
} GimpTransformDirection;


#define GIMP_TYPE_TRANSFORM_RESIZE (gimp_transform_resize_get_type ())

GType gimp_transform_resize_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_TRANSFORM_RESIZE_ADJUST           = 0, /*< desc="Adjust" >*/
  GIMP_TRANSFORM_RESIZE_CLIP             = 1, /*< desc="Clip" >*/
  GIMP_TRANSFORM_RESIZE_CROP,                 /*< desc="Crop to result" >*/
  GIMP_TRANSFORM_RESIZE_CROP_WITH_ASPECT,     /*< desc="Crop with aspect" >*/
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
  GIMP_PDB_REGION,
  GIMP_PDB_DISPLAY,
  GIMP_PDB_IMAGE,
  GIMP_PDB_LAYER,
  GIMP_PDB_CHANNEL,
  GIMP_PDB_DRAWABLE,
  GIMP_PDB_SELECTION,
  GIMP_PDB_BOUNDARY,
  GIMP_PDB_VECTORS,
  GIMP_PDB_PARASITE,
  GIMP_PDB_STATUS,
  GIMP_PDB_END,

  GIMP_PDB_PATH = GIMP_PDB_VECTORS /* deprecated */
} GimpPDBArgType;


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


#define GIMP_TYPE_MESSAGE_HANDLER_TYPE (gimp_message_handler_type_get_type ())

GType gimp_message_handler_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_MESSAGE_BOX,
  GIMP_CONSOLE,
  GIMP_ERROR_CONSOLE
} GimpMessageHandlerType;


#define GIMP_TYPE_STACK_TRACE_MODE (gimp_stack_trace_mode_get_type ())

GType gimp_stack_trace_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_STACK_TRACE_NEVER,
  GIMP_STACK_TRACE_QUERY,
  GIMP_STACK_TRACE_ALWAYS
} GimpStackTraceMode;


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


#define GIMP_TYPE_USER_DIRECTORY (gimp_user_directory_get_type ())

GType gimp_user_directory_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_USER_DIRECTORY_DESKTOP,
  GIMP_USER_DIRECTORY_DOCUMENTS,
  GIMP_USER_DIRECTORY_MUSIC,
  GIMP_USER_DIRECTORY_PICTURES,
  GIMP_USER_DIRECTORY_TEMPLATES,
  GIMP_USER_DIRECTORY_VIDEOS
} GimpUserDirectory;


#define GIMP_TYPE_VECTORS_STROKE_TYPE (gimp_vectors_stroke_type_get_type ())

GType gimp_vectors_stroke_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_VECTORS_STROKE_TYPE_BEZIER
} GimpVectorsStrokeType;

G_END_DECLS

#endif  /* __GIMP_BASE_ENUMS_H__ */
