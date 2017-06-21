/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __TOOLS_ENUMS_H__
#define __TOOLS_ENUMS_H__


/*
 * these enums are registered with the type system
 */

#define GIMP_TYPE_RECTANGLE_CONSTRAINT (gimp_rectangle_constraint_get_type ())

GType gimp_rectangle_constraint_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_RECTANGLE_CONSTRAIN_NONE,
  GIMP_RECTANGLE_CONSTRAIN_IMAGE,
  GIMP_RECTANGLE_CONSTRAIN_DRAWABLE
} GimpRectangleConstraint;


#define GIMP_TYPE_RECTANGLE_PRECISION (gimp_rectangle_precision_get_type ())

GType gimp_rectangle_precision_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_RECTANGLE_PRECISION_INT,
  GIMP_RECTANGLE_PRECISION_DOUBLE,
} GimpRectanglePrecision;


#define GIMP_TYPE_RECTANGLE_TOOL_FIXED_RULE (gimp_rectangle_tool_fixed_rule_get_type ())

GType gimp_rectangle_tool_fixed_rule_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_RECTANGLE_TOOL_FIXED_ASPECT, /*< desc="Aspect ratio" >*/
  GIMP_RECTANGLE_TOOL_FIXED_WIDTH,  /*< desc="Width"        >*/
  GIMP_RECTANGLE_TOOL_FIXED_HEIGHT, /*< desc="Height"       >*/
  GIMP_RECTANGLE_TOOL_FIXED_SIZE,   /*< desc="Size"         >*/
} GimpRectangleToolFixedRule;


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
  GIMP_TRANSFORM_TYPE_LAYER,     /*< desc="Layer"     >*/
  GIMP_TRANSFORM_TYPE_SELECTION, /*< desc="Selection" >*/
  GIMP_TRANSFORM_TYPE_PATH       /*< desc="Path"      >*/
} GimpTransformType;


#define GIMP_TYPE_TOOL_ACTION (gimp_tool_action_get_type ())

GType gimp_tool_action_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_TOOL_ACTION_PAUSE,
  GIMP_TOOL_ACTION_RESUME,
  GIMP_TOOL_ACTION_HALT,
  GIMP_TOOL_ACTION_COMMIT
} GimpToolAction;


#define GIMP_TYPE_MATTING_DRAW_MODE (gimp_matting_draw_mode_get_type ())

GType gimp_matting_draw_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
 GIMP_MATTING_DRAW_MODE_FOREGROUND,   /*< desc="Draw foreground" >*/
 GIMP_MATTING_DRAW_MODE_BACKGROUND,   /*< desc="Draw background" >*/
 GIMP_MATTING_DRAW_MODE_UNKNOWN,      /*< desc="Draw unknown" >*/
} GimpMattingDrawMode;


#define GIMP_TYPE_WARP_BEHAVIOR (gimp_warp_behavior_get_type ())

GType gimp_warp_behavior_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_WARP_BEHAVIOR_MOVE,      /*< desc="Move pixels" >*/
  GEGL_WARP_BEHAVIOR_GROW,      /*< desc="Grow area" >*/
  GEGL_WARP_BEHAVIOR_SHRINK,    /*< desc="Shrink area" >*/
  GEGL_WARP_BEHAVIOR_SWIRL_CW,  /*< desc="Swirl clockwise" >*/
  GEGL_WARP_BEHAVIOR_SWIRL_CCW, /*< desc="Swirl counter-clockwise" >*/
  GEGL_WARP_BEHAVIOR_ERASE,     /*< desc="Erase warping" >*/
  GEGL_WARP_BEHAVIOR_SMOOTH     /*< desc="Smooth warping" >*/
} GimpWarpBehavior;


/*
 * non-registered enums; register them if needed
 */

typedef enum /*< skip >*/
{
  SELECTION_SELECT,
  SELECTION_MOVE_MASK,
  SELECTION_MOVE,
  SELECTION_MOVE_COPY,
  SELECTION_ANCHOR
} SelectFunction;

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
  GIMP_MOTION_MODE_COMPRESS
} GimpMotionMode;


#endif /* __TOOLS_ENUMS_H__ */
