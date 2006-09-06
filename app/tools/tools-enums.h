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

#ifndef __TOOLS_ENUMS_H__
#define __TOOLS_ENUMS_H__

/*
 * these enums are registered with the type system
 */

#define GIMP_TYPE_COLOR_PICK_MODE (gimp_color_pick_mode_get_type ())

GType gimp_color_pick_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_COLOR_PICK_MODE_NONE,       /*< desc="Pick only"            >*/
  GIMP_COLOR_PICK_MODE_FOREGROUND, /*< desc="Set foreground color" >*/
  GIMP_COLOR_PICK_MODE_BACKGROUND, /*< desc="Set background color" >*/
  GIMP_COLOR_PICK_MODE_PALETTE     /*< desc="Add to palette"       >*/
} GimpColorPickMode;


#define GIMP_TYPE_RECTANGLE_GUIDE (gimp_rectangle_guide_get_type ())

GType gimp_rectangle_guide_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_RECTANGLE_GUIDE_NONE,          /*< desc="No guides"       >*/
  GIMP_RECTANGLE_GUIDE_CENTER_LINES,  /*< desc="Center lines"    >*/
  GIMP_RECTANGLE_GUIDE_THIRDS,        /*< desc="Rule of thirds"  >*/
  GIMP_RECTANGLE_GUIDE_GOLDEN         /*< desc="Golden sections" >*/
} GimpRectangleGuide;

GType gimp_rectangle_constraint_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_RECTANGLE_CONSTRAIN_NONE,      /*< desc="No constraint"   >*/
  GIMP_RECTANGLE_CONSTRAIN_IMAGE,     /*< desc="Image bounds"    >*/
  GIMP_RECTANGLE_CONSTRAIN_DRAWABLE   /*< desc="Drawable bounds" >*/
} GimpRectangleConstraint;


#define GIMP_TYPE_RECT_SELECT_MODE (gimp_rect_select_mode_get_type ())

GType gimp_rect_select_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_RECT_SELECT_MODE_FREE,        /*< desc="Free select"        >*/
  GIMP_RECT_SELECT_MODE_FIXED_SIZE,  /*< desc="Fixed size"         >*/
  GIMP_RECT_SELECT_MODE_FIXED_RATIO  /*< desc="Fixed aspect ratio" >*/
} GimpRectSelectMode;


#define GIMP_TYPE_TRANSFORM_TYPE (gimp_transform_type_get_type ())

GType gimp_transform_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_TRANSFORM_TYPE_LAYER,     /*< desc="Transform layer"     >*/
  GIMP_TRANSFORM_TYPE_SELECTION, /*< desc="Transform selection" >*/
  GIMP_TRANSFORM_TYPE_PATH       /*< desc="Transform path"      >*/
} GimpTransformType;


#define GIMP_TYPE_VECTOR_MODE (gimp_vector_mode_get_type ())

GType gimp_vector_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_VECTOR_MODE_DESIGN,      /*< desc="Design" >*/
  GIMP_VECTOR_MODE_EDIT,        /*< desc="Edit"   >*/
  GIMP_VECTOR_MODE_MOVE         /*< desc="Move"   >*/
} GimpVectorMode;


#define GIMP_TYPE_TRANSFORM_PREVIEW_TYPE (gimp_transform_preview_type_get_type ())

GType gimp_transform_preview_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_TRANSFORM_PREVIEW_TYPE_OUTLINE,     /*< desc="Outline"      >*/
  GIMP_TRANSFORM_PREVIEW_TYPE_GRID,        /*< desc="Grid"         >*/
  GIMP_TRANSFORM_PREVIEW_TYPE_IMAGE,       /*< desc="Image"        >*/
  GIMP_TRANSFORM_PREVIEW_TYPE_IMAGE_GRID   /*< desc="Image + Grid" >*/
} GimpTransformPreviewType;


#define GIMP_TYPE_TRANSFORM_GRID_TYPE (gimp_transform_grid_type_get_type ())

GType gimp_transform_grid_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_TRANSFORM_GRID_TYPE_N_LINES,  /*< desc="Number of grid lines" >*/
  GIMP_TRANSFORM_GRID_TYPE_SPACING   /*< desc="Grid line spacing"    >*/
} GimpTransformGridType;


/*
 * non-registered enums; register them if needed
 */

typedef enum /*< skip >*/
{
  SELECTION_ADD       = GIMP_CHANNEL_OP_ADD,
  SELECTION_SUBTRACT  = GIMP_CHANNEL_OP_SUBTRACT,
  SELECTION_REPLACE   = GIMP_CHANNEL_OP_REPLACE,
  SELECTION_INTERSECT = GIMP_CHANNEL_OP_INTERSECT,
  SELECTION_MOVE_MASK,
  SELECTION_MOVE,
  SELECTION_MOVE_COPY,
  SELECTION_ANCHOR
} SelectOps;

/*  Tool control actions  */
typedef enum /*< skip >*/
{
  GIMP_TOOL_ACTION_PAUSE,
  GIMP_TOOL_ACTION_RESUME,
  GIMP_TOOL_ACTION_HALT
} GimpToolAction;

/*  Modes of GimpEditSelectionTool  */
typedef enum /*< skip >*/
{
  GIMP_TRANSLATE_MODE_VECTORS,
  GIMP_TRANSLATE_MODE_CHANNEL,
  GIMP_TRANSLATE_MODE_LAYER_MASK,
  GIMP_TRANSLATE_MODE_MASK,
  GIMP_TRANSLATE_MODE_MASK_TO_LAYER,
  GIMP_TRANSLATE_MODE_MASK_COPY_TO_LAYER,
  GIMP_TRANSLATE_MODE_LAYER,
  GIMP_TRANSLATE_MODE_FLOATING_SEL
} GimpTranslateMode;

/*  Motion event report modes  */
typedef enum /*< skip >*/
{
  GIMP_MOTION_MODE_EXACT,
  GIMP_MOTION_MODE_HINT,
  GIMP_MOTION_MODE_COMPRESS
} GimpMotionMode;

/*  Possible transform functions  */
typedef enum /*< skip >*/
{
  TRANSFORM_CREATING,
  TRANSFORM_HANDLE_NONE,
  TRANSFORM_HANDLE_NW, /* north west */
  TRANSFORM_HANDLE_NE, /* north east */
  TRANSFORM_HANDLE_SW, /* south west */
  TRANSFORM_HANDLE_SE, /* south east */
  TRANSFORM_HANDLE_N,  /* north      */
  TRANSFORM_HANDLE_S,  /* south      */
  TRANSFORM_HANDLE_E,  /* east       */
  TRANSFORM_HANDLE_W,  /* west       */
  TRANSFORM_HANDLE_CENTER
} TransformAction;


#endif /* __TOOLS_ENUMS_H__ */
