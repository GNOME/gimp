/* LIGMA - The GNU Image Manipulation Program
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


#define LIGMA_TYPE_BUTTON_PRESS_TYPE (ligma_button_press_type_get_type ())

GType ligma_button_press_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_BUTTON_PRESS_NORMAL,
  LIGMA_BUTTON_PRESS_DOUBLE,
  LIGMA_BUTTON_PRESS_TRIPLE
} LigmaButtonPressType;


#define LIGMA_TYPE_BUTTON_RELEASE_TYPE (ligma_button_release_type_get_type ())

GType ligma_button_release_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_BUTTON_RELEASE_NORMAL,
  LIGMA_BUTTON_RELEASE_CANCEL,
  LIGMA_BUTTON_RELEASE_CLICK,
  LIGMA_BUTTON_RELEASE_NO_MOTION
} LigmaButtonReleaseType;


#define LIGMA_TYPE_COMPASS_ORIENTATION (ligma_compass_orientation_get_type ())

GType ligma_compass_orientation_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_COMPASS_ORIENTATION_AUTO,       /*< desc="Auto"       >*/
  LIGMA_COMPASS_ORIENTATION_HORIZONTAL, /*< desc="Horizontal" >*/
  LIGMA_COMPASS_ORIENTATION_VERTICAL    /*< desc="Vertical"   >*/
} LigmaCompassOrientation;


#define LIGMA_TYPE_CURSOR_PRECISION (ligma_cursor_precision_get_type ())

GType ligma_cursor_precision_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_CURSOR_PRECISION_PIXEL_CENTER,
  LIGMA_CURSOR_PRECISION_PIXEL_BORDER,
  LIGMA_CURSOR_PRECISION_SUBPIXEL
} LigmaCursorPrecision;


#define LIGMA_TYPE_GUIDES_TYPE (ligma_guides_type_get_type ())

GType ligma_guides_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_GUIDES_NONE,          /*< desc="No guides"       >*/
  LIGMA_GUIDES_CENTER_LINES,  /*< desc="Center lines"    >*/
  LIGMA_GUIDES_THIRDS,        /*< desc="Rule of thirds"  >*/
  LIGMA_GUIDES_FIFTHS,        /*< desc="Rule of fifths"  >*/
  LIGMA_GUIDES_GOLDEN,        /*< desc="Golden sections" >*/
  LIGMA_GUIDES_DIAGONALS,     /*< desc="Diagonal lines"  >*/
  LIGMA_GUIDES_N_LINES,       /*< desc="Number of lines" >*/
  LIGMA_GUIDES_SPACING        /*< desc="Line spacing"    >*/
} LigmaGuidesType;


#define LIGMA_TYPE_HANDLE_TYPE (ligma_handle_type_get_type ())

GType ligma_handle_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_HANDLE_SQUARE,
  LIGMA_HANDLE_DASHED_SQUARE,
  LIGMA_HANDLE_FILLED_SQUARE,
  LIGMA_HANDLE_CIRCLE,
  LIGMA_HANDLE_DASHED_CIRCLE,
  LIGMA_HANDLE_FILLED_CIRCLE,
  LIGMA_HANDLE_DIAMOND,
  LIGMA_HANDLE_DASHED_DIAMOND,
  LIGMA_HANDLE_FILLED_DIAMOND,
  LIGMA_HANDLE_CROSS,
  LIGMA_HANDLE_CROSSHAIR,
  LIGMA_HANDLE_DROP,
  LIGMA_HANDLE_FILLED_DROP
} LigmaHandleType;


#define LIGMA_TYPE_HANDLE_ANCHOR (ligma_handle_anchor_get_type ())

GType ligma_handle_anchor_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_HANDLE_ANCHOR_CENTER,
  LIGMA_HANDLE_ANCHOR_NORTH,
  LIGMA_HANDLE_ANCHOR_NORTH_WEST,
  LIGMA_HANDLE_ANCHOR_NORTH_EAST,
  LIGMA_HANDLE_ANCHOR_SOUTH,
  LIGMA_HANDLE_ANCHOR_SOUTH_WEST,
  LIGMA_HANDLE_ANCHOR_SOUTH_EAST,
  LIGMA_HANDLE_ANCHOR_WEST,
  LIGMA_HANDLE_ANCHOR_EAST
} LigmaHandleAnchor;


#define LIGMA_TYPE_LIMIT_TYPE (ligma_limit_type_get_type ())

GType ligma_limit_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_LIMIT_CIRCLE,
  LIGMA_LIMIT_SQUARE,
  LIGMA_LIMIT_DIAMOND,
  LIGMA_LIMIT_HORIZONTAL,
  LIGMA_LIMIT_VERTICAL
} LigmaLimitType;


#define LIGMA_TYPE_PATH_STYLE (ligma_path_style_get_type ())

GType ligma_path_style_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_PATH_STYLE_DEFAULT,
  LIGMA_PATH_STYLE_VECTORS,
  LIGMA_PATH_STYLE_OUTLINE
} LigmaPathStyle;


#define LIGMA_TYPE_RECTANGLE_CONSTRAINT (ligma_rectangle_constraint_get_type ())

GType ligma_rectangle_constraint_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_RECTANGLE_CONSTRAIN_NONE,
  LIGMA_RECTANGLE_CONSTRAIN_IMAGE,
  LIGMA_RECTANGLE_CONSTRAIN_DRAWABLE
} LigmaRectangleConstraint;


#define LIGMA_TYPE_RECTANGLE_FIXED_RULE (ligma_rectangle_fixed_rule_get_type ())

GType ligma_rectangle_fixed_rule_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_RECTANGLE_FIXED_ASPECT, /*< desc="Aspect ratio" >*/
  LIGMA_RECTANGLE_FIXED_WIDTH,  /*< desc="Width"        >*/
  LIGMA_RECTANGLE_FIXED_HEIGHT, /*< desc="Height"       >*/
  LIGMA_RECTANGLE_FIXED_SIZE,   /*< desc="Size"         >*/
} LigmaRectangleFixedRule;


#define LIGMA_TYPE_RECTANGLE_PRECISION (ligma_rectangle_precision_get_type ())

GType ligma_rectangle_precision_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_RECTANGLE_PRECISION_INT,
  LIGMA_RECTANGLE_PRECISION_DOUBLE,
} LigmaRectanglePrecision;


#define LIGMA_TYPE_TRANSFORM_3D_MODE (ligma_transform_3d_mode_get_type ())

GType ligma_transform_3d_mode_get_type (void) G_GNUC_CONST;

typedef enum /*< lowercase_name=ligma_transform_3d_mode >*/
{
  LIGMA_TRANSFORM_3D_MODE_CAMERA,
  LIGMA_TRANSFORM_3D_MODE_MOVE,
  LIGMA_TRANSFORM_3D_MODE_ROTATE
} LigmaTransform3DMode;


#define LIGMA_TYPE_TRANSFORM_FUNCTION (ligma_transform_function_get_type ())

GType ligma_transform_function_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_TRANSFORM_FUNCTION_NONE,
  LIGMA_TRANSFORM_FUNCTION_MOVE,
  LIGMA_TRANSFORM_FUNCTION_SCALE,
  LIGMA_TRANSFORM_FUNCTION_ROTATE,
  LIGMA_TRANSFORM_FUNCTION_SHEAR,
  LIGMA_TRANSFORM_FUNCTION_PERSPECTIVE
} LigmaTransformFunction;


#define LIGMA_TYPE_TRANSFORM_HANDLE_MODE (ligma_transform_handle_mode_get_type ())

GType ligma_transform_handle_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_HANDLE_MODE_ADD_TRANSFORM, /*< desc="Add / Transform" >*/
  LIGMA_HANDLE_MODE_MOVE,          /*< desc="Move"            >*/
  LIGMA_HANDLE_MODE_REMOVE         /*< desc="Remove"          >*/
} LigmaTransformHandleMode;


#define LIGMA_TYPE_VECTOR_MODE (ligma_vector_mode_get_type ())

GType ligma_vector_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_VECTOR_MODE_DESIGN,      /*< desc="Design" >*/
  LIGMA_VECTOR_MODE_EDIT,        /*< desc="Edit"   >*/
  LIGMA_VECTOR_MODE_MOVE         /*< desc="Move"   >*/
} LigmaVectorMode;


#define LIGMA_TYPE_ZOOM_FOCUS (ligma_zoom_focus_get_type ())

GType ligma_zoom_focus_get_type (void) G_GNUC_CONST;

typedef enum
{
  /* Make a best guess */
  LIGMA_ZOOM_FOCUS_BEST_GUESS,

  /* Use the mouse cursor (if within canvas) */
  LIGMA_ZOOM_FOCUS_POINTER,

  /* Use the image center */
  LIGMA_ZOOM_FOCUS_IMAGE_CENTER,

  /* If the image is centered, retain the centering. Else use
   * _BEST_GUESS
   */
  LIGMA_ZOOM_FOCUS_RETAIN_CENTERING_ELSE_BEST_GUESS

} LigmaZoomFocus;


#define LIGMA_TYPE_MODIFIER_ACTION (ligma_modifier_action_get_type ())

GType ligma_modifier_action_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_MODIFIER_ACTION_NONE,                    /*< desc="No action"                                  >*/
  LIGMA_MODIFIER_ACTION_PANNING,                 /*< desc="Pan"                                        >*/
  LIGMA_MODIFIER_ACTION_ZOOMING,                 /*< desc="Zoom"                                       >*/
  LIGMA_MODIFIER_ACTION_ROTATING,                /*< desc="Rotate View"                                >*/
  LIGMA_MODIFIER_ACTION_STEP_ROTATING,           /*< desc="Rotate View by 15 degree steps"             >*/
  LIGMA_MODIFIER_ACTION_LAYER_PICKING,           /*< desc="Pick a layer"                               >*/

  LIGMA_MODIFIER_ACTION_MENU,                    /*< desc="Display the menu"                           >*/

  LIGMA_MODIFIER_ACTION_ACTION,                  /*< desc="Custom action"                              >*/

  LIGMA_MODIFIER_ACTION_BRUSH_PIXEL_SIZE,        /*< desc="Change brush size in canvas pixels"         >*/
  LIGMA_MODIFIER_ACTION_BRUSH_RADIUS_PIXEL_SIZE, /*< desc="Change brush radius' size in canvas pixels" >*/
  LIGMA_MODIFIER_ACTION_TOOL_OPACITY,            /*< desc="Change tool opacity"                        >*/
} LigmaModifierAction;


/*
 * non-registered enums; register them if needed
 */


typedef enum  /*< pdb-skip, skip >*/
{
  LIGMA_HIT_NONE,
  LIGMA_HIT_INDIRECT,
  LIGMA_HIT_DIRECT
} LigmaHit;


#endif /* __DISPLAY_ENUMS_H__ */
