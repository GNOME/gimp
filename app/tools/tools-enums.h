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

#pragma once


/*
 * these enums are registered with the type system
 */

/**
 * GimpBucketFillArea:
 * @GIMP_BUCKET_FILL_SELECTION:      Fill whole selection
 * @GIMP_BUCKET_FILL_SIMILAR_COLORS: Fill similar colors
 * @GIMP_BUCKET_FILL_LINE_ART:       Fill by line art detection
 *
 * Bucket fill area.
 */
#define GIMP_TYPE_BUCKET_FILL_AREA (gimp_bucket_fill_area_get_type ())

GType gimp_bucket_fill_area_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_BUCKET_FILL_SELECTION,       /*< desc="Fill whole selection" >*/
  GIMP_BUCKET_FILL_SIMILAR_COLORS,  /*< desc="Fill similar colors" >*/
  GIMP_BUCKET_FILL_LINE_ART         /*< desc="Fill by line art detection" >*/
} GimpBucketFillArea;


/**
 * GimpLineArtSource:
 * @GIMP_LINE_ART_SOURCE_SAMPLE_MERGED: All visible layers
 * @GIMP_LINE_ART_SOURCE_ACTIVE_LAYER:  Active layer
 * @GIMP_LINE_ART_SOURCE_LOWER_LAYER:   Layer below the active one
 * @GIMP_LINE_ART_SOURCE_UPPER_LAYER:   Layer above the active one
 *
 * Bucket fill area.
 */
#define GIMP_TYPE_LINE_ART_SOURCE (gimp_line_art_source_get_type ())

GType gimp_line_art_source_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_LINE_ART_SOURCE_SAMPLE_MERGED, /*< desc="All visible layers" >*/
  GIMP_LINE_ART_SOURCE_ACTIVE_LAYER,  /*< desc="Selected layer" >*/
  GIMP_LINE_ART_SOURCE_LOWER_LAYER,   /*< desc="Layer below the selected one" >*/
  GIMP_LINE_ART_SOURCE_UPPER_LAYER    /*< desc="Layer above the selected one" >*/
} GimpLineArtSource;


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
  GIMP_TRANSFORM_TYPE_PATH,      /*< desc="Path"      >*/
  GIMP_TRANSFORM_TYPE_IMAGE      /*< desc="Image"     >*/
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


#define GIMP_TYPE_TOOL_ACTIVE_MODIFIERS (gimp_tool_active_modifiers_get_type ())

GType gimp_tool_active_modifiers_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_TOOL_ACTIVE_MODIFIERS_OFF,
  GIMP_TOOL_ACTIVE_MODIFIERS_SAME,
  GIMP_TOOL_ACTIVE_MODIFIERS_SEPARATE,
} GimpToolActiveModifiers;


#define GIMP_TYPE_MATTING_DRAW_MODE (gimp_matting_draw_mode_get_type ())

GType gimp_matting_draw_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
 GIMP_MATTING_DRAW_MODE_FOREGROUND,   /*< desc="Draw foreground" >*/
 GIMP_MATTING_DRAW_MODE_BACKGROUND,   /*< desc="Draw background" >*/
 GIMP_MATTING_DRAW_MODE_UNKNOWN,      /*< desc="Draw unknown" >*/
} GimpMattingDrawMode;


#define GIMP_TYPE_MATTING_PREVIEW_MODE (gimp_matting_preview_mode_get_type ())

GType gimp_matting_preview_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
 GIMP_MATTING_PREVIEW_MODE_ON_COLOR,        /*< desc="Color" >*/
 GIMP_MATTING_PREVIEW_MODE_GRAYSCALE,       /*< desc="Grayscale" >*/
} GimpMattingPreviewMode;


#define GIMP_TYPE_TRANSFORM_3D_LENS_MODE (gimp_transform_3d_lens_mode_get_type ())

GType gimp_transform_3d_lens_mode_get_type (void) G_GNUC_CONST;

typedef enum /*< lowercase_name=gimp_transform_3d_lens_mode >*/
{
  GIMP_TRANSFORM_3D_LENS_MODE_FOCAL_LENGTH, /*< desc="Focal length" >*/
  GIMP_TRANSFORM_3D_LENS_MODE_FOV_IMAGE,    /*< desc="Field of view (relative to image)", abbrev="FOV (image)" >*/
  GIMP_TRANSFORM_3D_LENS_MODE_FOV_ITEM,     /*< desc="Field of view (relative to item)", abbrev="FOV (item)" >*/
} Gimp3DTransformLensMode;


#define GIMP_TYPE_WARP_BEHAVIOR (gimp_warp_behavior_get_type ())

GType gimp_warp_behavior_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_WARP_BEHAVIOR_MOVE,      /*< desc="Move pixels" >*/
  GIMP_WARP_BEHAVIOR_GROW,      /*< desc="Grow area" >*/
  GIMP_WARP_BEHAVIOR_SHRINK,    /*< desc="Shrink area" >*/
  GIMP_WARP_BEHAVIOR_SWIRL_CW,  /*< desc="Swirl clockwise" >*/
  GIMP_WARP_BEHAVIOR_SWIRL_CCW, /*< desc="Swirl counter-clockwise" >*/
  GIMP_WARP_BEHAVIOR_ERASE,     /*< desc="Erase warping" >*/
  GIMP_WARP_BEHAVIOR_SMOOTH     /*< desc="Smooth warping" >*/
} GimpWarpBehavior;


#define GIMP_TYPE_PAINT_SELECT_MODE (gimp_paint_select_mode_get_type ())

GType gimp_paint_select_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_PAINT_SELECT_MODE_ADD,      /*< desc="Add to selection" >*/
  GIMP_PAINT_SELECT_MODE_SUBTRACT, /*< desc="Subtract from selection" >*/
} GimpPaintSelectMode;

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
  GIMP_TRANSLATE_MODE_PATH,
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
