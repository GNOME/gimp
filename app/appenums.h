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
  ADD_ALPHA_MASK,
  ADD_SELECTION_MASK,
  ADD_INV_SELECTION_MASK
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
  DESTROY,
  RECREATE
} ToolAction;

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
  GIMP_TOOL_CURSOR_NONE,
  GIMP_RECT_SELECT_TOOL_CURSOR,
  GIMP_ELLIPSE_SELECT_TOOL_CURSOR,
  GIMP_FREE_SELECT_TOOL_CURSOR,
  GIMP_FUZZY_SELECT_TOOL_CURSOR,
  GIMP_BEZIER_SELECT_TOOL_CURSOR,
  GIMP_SCISSORS_TOOL_CURSOR,
  GIMP_MOVE_TOOL_CURSOR,
  GIMP_ZOOM_TOOL_CURSOR,
  GIMP_CROP_TOOL_CURSOR,
  GIMP_RESIZE_TOOL_CURSOR,
  GIMP_ROTATE_TOOL_CURSOR,
  GIMP_SHEAR_TOOL_CURSOR,
  GIMP_PERSPECTIVE_TOOL_CURSOR,
  GIMP_FLIP_HORIZONTAL_TOOL_CURSOR,
  GIMP_FLIP_VERTICAL_TOOL_CURSOR,
  GIMP_TEXT_TOOL_CURSOR,
  GIMP_COLOR_PICKER_TOOL_CURSOR,
  GIMP_BUCKET_FILL_TOOL_CURSOR,
  GIMP_BLEND_TOOL_CURSOR,
  GIMP_PENCIL_TOOL_CURSOR,
  GIMP_PAINTBRUSH_TOOL_CURSOR,
  GIMP_AIRBRUSH_TOOL_CURSOR,
  GIMP_INK_TOOL_CURSOR,
  GIMP_CLONE_TOOL_CURSOR,
  GIMP_ERASER_TOOL_CURSOR,
  GIMP_SMUDGE_TOOL_CURSOR,
  GIMP_BLUR_TOOL_CURSOR,
  GIMP_DODGE_TOOL_CURSOR,
  GIMP_BURN_TOOL_CURSOR,
  GIMP_MEASURE_TOOL_CURSOR,
  GIMP_LAST_STOCK_TOOL_CURSOR_ENTRY
} GimpToolCursorType;

typedef enum /*< skip >*/
{
  GIMP_CURSOR_MODIFIER_NONE,
  GIMP_CURSOR_MODIFIER_PLUS,
  GIMP_CURSOR_MODIFIER_MINUS,
  GIMP_CURSOR_MODIFIER_INTERSECT,
  GIMP_CURSOR_MODIFIER_MOVE,
  GIMP_CURSOR_MODIFIER_RESIZE,
  GIMP_CURSOR_MODIFIER_CONTROL,
  GIMP_CURSOR_MODIFIER_ANCHOR,
  GIMP_CURSOR_MODIFIER_FOREGROUND,
  GIMP_CURSOR_MODIFIER_BACKGROUND,
  GIMP_CURSOR_MODIFIER_PATTERN,
  GIMP_CURSOR_MODIFIER_HAND,
  GIMP_LAST_CURSOR_MODIFIER_ENTRY
} GimpCursorModifier;


#endif /* __APPENUMS_H__ */
