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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __DISPLAY_ENUMS_H__
#define __DISPLAY_ENUMS_H__


#define GIMP_TYPE_BUTTON_PRESS_TYPE (gimp_button_press_type_get_type ())

GType gimp_button_press_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_BUTTON_PRESS_NORMAL,
  GIMP_BUTTON_PRESS_DOUBLE,
  GIMP_BUTTON_PRESS_TRIPLE
} GimpButtonPressType;


#define GIMP_TYPE_BUTTON_RELEASE_TYPE (gimp_button_release_type_get_type ())

GType gimp_button_release_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_BUTTON_RELEASE_NORMAL,
  GIMP_BUTTON_RELEASE_CANCEL,
  GIMP_BUTTON_RELEASE_CLICK,
  GIMP_BUTTON_RELEASE_NO_MOTION
} GimpButtonReleaseType;


#define GIMP_TYPE_COMPASS_ORIENTATION (gimp_compass_orientation_get_type ())

GType gimp_compass_orientation_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_COMPASS_ORIENTATION_AUTO,       /*< desc="Auto"       >*/
  GIMP_COMPASS_ORIENTATION_HORIZONTAL, /*< desc="Horizontal" >*/
  GIMP_COMPASS_ORIENTATION_VERTICAL    /*< desc="Vertical"   >*/
} GimpCompassOrientation;


#define GIMP_TYPE_CURSOR_PRECISION (gimp_cursor_precision_get_type ())

GType gimp_cursor_precision_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_CURSOR_PRECISION_PIXEL_CENTER,
  GIMP_CURSOR_PRECISION_PIXEL_BORDER,
  GIMP_CURSOR_PRECISION_SUBPIXEL
} GimpCursorPrecision;


#define GIMP_TYPE_GUIDES_TYPE (gimp_guides_type_get_type ())

GType gimp_guides_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_GUIDES_NONE,          /*< desc="No guides"       >*/
  GIMP_GUIDES_CENTER_LINES,  /*< desc="Center lines"    >*/
  GIMP_GUIDES_THIRDS,        /*< desc="Rule of thirds"  >*/
  GIMP_GUIDES_FIFTHS,        /*< desc="Rule of fifths"  >*/
  GIMP_GUIDES_GOLDEN,        /*< desc="Golden sections" >*/
  GIMP_GUIDES_DIAGONALS,     /*< desc="Diagonal lines"  >*/
  GIMP_GUIDES_N_LINES,       /*< desc="Number of lines" >*/
  GIMP_GUIDES_SPACING        /*< desc="Line spacing"    >*/
} GimpGuidesType;


#define GIMP_TYPE_HANDLE_TYPE (gimp_handle_type_get_type ())

GType gimp_handle_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_HANDLE_SQUARE,
  GIMP_HANDLE_DASHED_SQUARE,
  GIMP_HANDLE_FILLED_SQUARE,
  GIMP_HANDLE_CIRCLE,
  GIMP_HANDLE_DASHED_CIRCLE,
  GIMP_HANDLE_FILLED_CIRCLE,
  GIMP_HANDLE_DIAMOND,
  GIMP_HANDLE_DASHED_DIAMOND,
  GIMP_HANDLE_FILLED_DIAMOND,
  GIMP_HANDLE_CROSS,
  GIMP_HANDLE_CROSSHAIR,
  GIMP_HANDLE_DROP,
  GIMP_HANDLE_FILLED_DROP
} GimpHandleType;


#define GIMP_TYPE_HANDLE_ANCHOR (gimp_handle_anchor_get_type ())

GType gimp_handle_anchor_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_HANDLE_ANCHOR_CENTER,
  GIMP_HANDLE_ANCHOR_NORTH,
  GIMP_HANDLE_ANCHOR_NORTH_WEST,
  GIMP_HANDLE_ANCHOR_NORTH_EAST,
  GIMP_HANDLE_ANCHOR_SOUTH,
  GIMP_HANDLE_ANCHOR_SOUTH_WEST,
  GIMP_HANDLE_ANCHOR_SOUTH_EAST,
  GIMP_HANDLE_ANCHOR_WEST,
  GIMP_HANDLE_ANCHOR_EAST
} GimpHandleAnchor;


#define GIMP_TYPE_LIMIT_TYPE (gimp_limit_type_get_type ())

GType gimp_limit_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_LIMIT_CIRCLE,
  GIMP_LIMIT_SQUARE,
  GIMP_LIMIT_DIAMOND,
  GIMP_LIMIT_HORIZONTAL,
  GIMP_LIMIT_VERTICAL
} GimpLimitType;


#define GIMP_TYPE_PATH_STYLE (gimp_path_style_get_type ())

GType gimp_path_style_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_PATH_STYLE_DEFAULT,
  GIMP_PATH_STYLE_VECTORS,
  GIMP_PATH_STYLE_OUTLINE
} GimpPathStyle;


#define GIMP_TYPE_RECTANGLE_CONSTRAINT (gimp_rectangle_constraint_get_type ())

GType gimp_rectangle_constraint_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_RECTANGLE_CONSTRAIN_NONE,
  GIMP_RECTANGLE_CONSTRAIN_IMAGE,
  GIMP_RECTANGLE_CONSTRAIN_DRAWABLE
} GimpRectangleConstraint;


#define GIMP_TYPE_RECTANGLE_FIXED_RULE (gimp_rectangle_fixed_rule_get_type ())

GType gimp_rectangle_fixed_rule_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_RECTANGLE_FIXED_ASPECT, /*< desc="Aspect ratio" >*/
  GIMP_RECTANGLE_FIXED_WIDTH,  /*< desc="Width"        >*/
  GIMP_RECTANGLE_FIXED_HEIGHT, /*< desc="Height"       >*/
  GIMP_RECTANGLE_FIXED_SIZE,   /*< desc="Size"         >*/
} GimpRectangleFixedRule;


#define GIMP_TYPE_RECTANGLE_PRECISION (gimp_rectangle_precision_get_type ())

GType gimp_rectangle_precision_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_RECTANGLE_PRECISION_INT,
  GIMP_RECTANGLE_PRECISION_DOUBLE,
} GimpRectanglePrecision;


#define GIMP_TYPE_TRANSFORM_3D_MODE (gimp_transform_3d_mode_get_type ())

GType gimp_transform_3d_mode_get_type (void) G_GNUC_CONST;

typedef enum /*< lowercase_name=gimp_transform_3d_mode >*/
{
  GIMP_TRANSFORM_3D_MODE_CAMERA,
  GIMP_TRANSFORM_3D_MODE_MOVE,
  GIMP_TRANSFORM_3D_MODE_ROTATE
} GimpTransform3DMode;


#define GIMP_TYPE_TRANSFORM_FUNCTION (gimp_transform_function_get_type ())

GType gimp_transform_function_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_TRANSFORM_FUNCTION_NONE,
  GIMP_TRANSFORM_FUNCTION_MOVE,
  GIMP_TRANSFORM_FUNCTION_SCALE,
  GIMP_TRANSFORM_FUNCTION_ROTATE,
  GIMP_TRANSFORM_FUNCTION_SHEAR,
  GIMP_TRANSFORM_FUNCTION_PERSPECTIVE
} GimpTransformFunction;


#define GIMP_TYPE_TRANSFORM_HANDLE_MODE (gimp_transform_handle_mode_get_type ())

GType gimp_transform_handle_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_HANDLE_MODE_ADD_TRANSFORM, /*< desc="Add / Transform" >*/
  GIMP_HANDLE_MODE_MOVE,          /*< desc="Move"            >*/
  GIMP_HANDLE_MODE_REMOVE         /*< desc="Remove"          >*/
} GimpTransformHandleMode;


#define GIMP_TYPE_VECTOR_MODE (gimp_vector_mode_get_type ())

GType gimp_vector_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_VECTOR_MODE_DESIGN,      /*< desc="Design" >*/
  GIMP_VECTOR_MODE_EDIT,        /*< desc="Edit"   >*/
  GIMP_VECTOR_MODE_MOVE         /*< desc="Move"   >*/
} GimpVectorMode;


#define GIMP_TYPE_ZOOM_FOCUS (gimp_zoom_focus_get_type ())

GType gimp_zoom_focus_get_type (void) G_GNUC_CONST;

typedef enum
{
  /* Make a best guess */
  GIMP_ZOOM_FOCUS_BEST_GUESS,

  /* Use the mouse cursor (if within canvas) */
  GIMP_ZOOM_FOCUS_POINTER,

  /* Use the image center */
  GIMP_ZOOM_FOCUS_IMAGE_CENTER,

  /* If the image is centered, retain the centering. Else use
   * _BEST_GUESS
   */
  GIMP_ZOOM_FOCUS_RETAIN_CENTERING_ELSE_BEST_GUESS

} GimpZoomFocus;


#define GIMP_TYPE_MODIFIER_ACTION (gimp_modifier_action_get_type ())

GType gimp_modifier_action_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_MODIFIER_ACTION_NONE,                    /*< desc="No action"                                  >*/
  GIMP_MODIFIER_ACTION_PANNING,                 /*< desc="Pan"                                        >*/
  GIMP_MODIFIER_ACTION_ZOOMING,                 /*< desc="Zoom"                                       >*/
  GIMP_MODIFIER_ACTION_ROTATING,                /*< desc="Rotate View"                                >*/
  GIMP_MODIFIER_ACTION_STEP_ROTATING,           /*< desc="Rotate View by 15 degree steps"             >*/
  GIMP_MODIFIER_ACTION_LAYER_PICKING,           /*< desc="Pick a layer"                               >*/

  GIMP_MODIFIER_ACTION_MENU,                    /*< desc="Display the menu"                           >*/

  GIMP_MODIFIER_ACTION_ACTION,                  /*< desc="Custom action"                              >*/

  GIMP_MODIFIER_ACTION_BRUSH_PIXEL_SIZE,        /*< desc="Change brush size in canvas pixels"         >*/
  GIMP_MODIFIER_ACTION_BRUSH_RADIUS_PIXEL_SIZE, /*< desc="Change brush radius' size in canvas pixels" >*/
  GIMP_MODIFIER_ACTION_TOOL_OPACITY,            /*< desc="Change tool opacity"                        >*/
} GimpModifierAction;


/*
 * non-registered enums; register them if needed
 */


typedef enum  /*< pdb-skip, skip >*/
{
  GIMP_HIT_NONE,
  GIMP_HIT_INDIRECT,
  GIMP_HIT_DIRECT
} GimpHit;


#endif /* __DISPLAY_ENUMS_H__ */
