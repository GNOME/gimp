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

#ifndef __APPENUMS_H__
#define __APPENUMS_H__


/* Base image types */
typedef enum
{
  RGB,
  GRAY,
  INDEXED
} GimpImageBaseType;

/* Image types */
typedef enum
{
  RGB_GIMAGE,		/*< nick=RGB_IMAGE >*/
  RGBA_GIMAGE,		/*< nick=RGBA_IMAGE >*/
  GRAY_GIMAGE,		/*< nick=GRAY_IMAGE >*/
  GRAYA_GIMAGE,		/*< nick=GRAYA_IMAGE >*/
  INDEXED_GIMAGE,	/*< nick=INDEXED_IMAGE >*/
  INDEXEDA_GIMAGE	/*< nick=INDEXEDA_IMAGE >*/
} GimpImageType;

/* Fill types */
typedef enum
{
  FOREGROUND_FILL,	/*< nick=FG_IMAGE_FILL >*/
  BACKGROUND_FILL,	/*< nick=BG_IMAGE_FILL >*/
  WHITE_FILL,		/*< nick=WHITE_IMAGE_FILL >*/
  TRANSPARENT_FILL,	/*< nick=TRANS_IMAGE_FILL >*/
  NO_FILL		/*< nick=NO_IMAGE_FILL >*/
} GimpFillType;

/* Layer modes  */
typedef enum 
{
  NORMAL_MODE,
  DISSOLVE_MODE,
  BEHIND_MODE,
  MULTIPLY_MODE,
  SCREEN_MODE,
  OVERLAY_MODE,
  DIFFERENCE_MODE,
  ADDITION_MODE,
  SUBTRACT_MODE,
  DARKEN_ONLY_MODE,
  LIGHTEN_ONLY_MODE,
  HUE_MODE,
  SATURATION_MODE,
  COLOR_MODE,
  VALUE_MODE,
  DIVIDE_MODE,
  DODGE_MODE,
  BURN_MODE,
  HARDLIGHT_MODE,
  ERASE_MODE,         /*< skip >*/
  REPLACE_MODE,       /*< skip >*/
  ANTI_ERASE_MODE     /*< skip >*/
} LayerModeEffects;

/* Types of convolutions  */
typedef enum 
{
  NORMAL_CONVOL,		/*  Negative numbers truncated  */
  ABSOLUTE_CONVOL,		/*  Absolute value              */
  NEGATIVE_CONVOL		/*  add 127 to values           */
} ConvolutionType;

/* Brush application types  */
typedef enum
{
  HARD,     /* pencil */
  SOFT,     /* paintbrush */
  PRESSURE  /* paintbrush with variable pressure */
} BrushApplicationMode;

/* Paint application modes  */
typedef enum
{
  CONSTANT,    /*< nick=CONTINUOUS >*/ /* pencil, paintbrush, airbrush, clone */
  INCREMENTAL  /* convolve, smudge */
} PaintApplicationMode;

typedef enum
{
  APPLY,
  DISCARD
} MaskApplyMode;

typedef enum  /*< chop=ADD_ >*/
{
  ADD_WHITE_MASK,
  ADD_BLACK_MASK,
  ADD_ALPHA_MASK
} AddMaskType;

/* gradient paint modes */
typedef enum
{
  ONCE_FORWARD,    /* paint through once, then stop */
  ONCE_BACKWARDS,  /* paint once, then stop, but run the gradient the other way */
  LOOP_SAWTOOTH,   /* keep painting, looping through the grad start->end,start->end /|/|/| */
  LOOP_TRIANGLE,   /* keep paiting, looping though the grad start->end,end->start /\/\/\/  */
  ONCE_END_COLOR   /* paint once, but keep painting with the end color */
} GradientPaintMode;


/* gradient paint modes */
typedef enum
{
  LINEAR_INTERPOLATION,
  CUBIC_INTERPOLATION,
  NEAREST_NEIGHBOR_INTERPOLATION
} InterpolationType;

typedef enum /*< skip >*/
{
  ORIENTATION_UNKNOWN,
  ORIENTATION_HORIZONTAL,
  ORIENTATION_VERTICAL
} InternalOrientationType;

typedef enum
{
  HORIZONTAL,
  VERTICAL,
  UNKNOWN
} OrientationType;

/*  Procedural database types  */
typedef enum
{
  PDB_INT32,
  PDB_INT16,
  PDB_INT8,
  PDB_FLOAT,
  PDB_STRING,
  PDB_INT32ARRAY,
  PDB_INT16ARRAY,
  PDB_INT8ARRAY,
  PDB_FLOATARRAY,
  PDB_STRINGARRAY,
  PDB_COLOR,
  PDB_REGION,
  PDB_DISPLAY,
  PDB_IMAGE,
  PDB_LAYER,
  PDB_CHANNEL,
  PDB_DRAWABLE,
  PDB_SELECTION,
  PDB_BOUNDARY,
  PDB_PATH,
  PDB_PARASITE,
  PDB_STATUS,
  PDB_END
} PDBArgType;

/*  Error types  */
typedef enum
{
  PDB_EXECUTION_ERROR,
  PDB_CALLING_ERROR,
  PDB_PASS_THROUGH,
  PDB_SUCCESS,
  PDB_CANCEL
} PDBStatusType;

/*  Procedure types  */
typedef enum /*< chop=PDB_ >*/
{
  PDB_INTERNAL,
  PDB_PLUGIN,
  PDB_EXTENSION,
  PDB_TEMPORARY
} PDBProcType;

/*  Selection Boolean operations  */
typedef enum /*< chop=CHANNEL_OP_ >*/
{
  CHANNEL_OP_ADD,
  CHANNEL_OP_SUB,
  CHANNEL_OP_REPLACE,
  CHANNEL_OP_INTERSECT
} ChannelOps;

typedef enum /*< skip >*/
{
  SELECTION_ADD       = CHANNEL_OP_ADD,
  SELECTION_SUB       = CHANNEL_OP_SUB,
  SELECTION_REPLACE   = CHANNEL_OP_REPLACE,
  SELECTION_INTERSECT = CHANNEL_OP_INTERSECT,
  SELECTION_MOVE_MASK,
  SELECTION_MOVE,
  SELECTION_ANCHOR
} SelectOps;

/*  The possible states for tools  */
typedef enum /*< skip >*/
{
  INACTIVE,
  ACTIVE,
  PAUSED
} ToolState;

/*  Tool control actions  */
typedef enum /*< skip >*/
{
  PAUSE,
  RESUME,
  HALT,
  UPDATE_CURSOR,
  DESTROY,
  RECREATE
} ToolAction;

/*  Tool types  */
typedef enum /*< skip >*/
{
  TOOL_TYPE_NONE     = -1,
  FIRST_TOOLBOX_TOOL = 0,
  RECT_SELECT        = FIRST_TOOLBOX_TOOL,
  ELLIPSE_SELECT,
  FREE_SELECT,
  FUZZY_SELECT,
  BEZIER_SELECT,
  ISCISSORS,
  MOVE,
  MAGNIFY,
  CROP,
  ROTATE,
  SCALE,
  SHEAR,
  PERSPECTIVE,
  FLIP,
  TEXT,
  COLOR_PICKER,
  BUCKET_FILL,
  BLEND,
  PENCIL,
  PAINTBRUSH,
  AIRBRUSH,
  INK,
  CLONE,
  ERASER,
  SMUDGE,
  CONVOLVE,
  DODGEBURN,
  MEASURE,
  LAST_TOOLBOX_TOOL = MEASURE,

  /*  Non-toolbox tools  */
  BY_COLOR_SELECT,
  COLOR_BALANCE,
  BRIGHTNESS_CONTRAST,
  HUE_SATURATION,
  POSTERIZE,
  THRESHOLD,
  CURVES,
  LEVELS,
  HISTOGRAM
} ToolType;

/* possible transform functions */
typedef enum /*< skip >*/
{
  TRANSFORM_CREATING,
  TRANSFORM_HANDLE_1,
  TRANSFORM_HANDLE_2,
  TRANSFORM_HANDLE_3,
  TRANSFORM_HANDLE_4,
  TRANSFORM_HANDLE_CENTER
} TransformAction;

/* the different states that the transformation function can be called with */
typedef enum /*< skip >*/
{
  TRANSFORM_INIT,
  TRANSFORM_MOTION,
  TRANSFORM_RECALC,
  TRANSFORM_FINISH
} TransformState;

typedef enum /*< skip >*/
{
  CURSOR_MODE_TOOL_ICON,
  CURSOR_MODE_TOOL_CROSSHAIR,
  CURSOR_MODE_CROSSHAIR
} CursorMode;

typedef enum /*< skip >*/
{
  CURSOR_MODIFIER_NONE,
  CURSOR_MODIFIER_PLUS,
  CURSOR_MODIFIER_MINUS,
  CURSOR_MODIFIER_INTERSECT,
  CURSOR_MODIFIER_MOVE,
  CURSOR_MODIFIER_RESIZE,
  CURSOR_MODIFIER_CONTROL,
  CURSOR_MODIFIER_ANCHOR,
  CURSOR_MODIFIER_FOREGROUND,
  CURSOR_MODIFIER_BACKGROUND,
  CURSOR_MODIFIER_PATTERN,
  CURSOR_MODIFIER_HAND
} CursorModifier;


#endif /* __APPENUMS_H__ */
