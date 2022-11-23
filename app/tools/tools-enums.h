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

#ifndef __TOOLS_ENUMS_H__
#define __TOOLS_ENUMS_H__


/*
 * these enums are registered with the type system
 */

/**
 * LigmaBucketFillArea:
 * @LIGMA_BUCKET_FILL_SELECTION:      Fill whole selection
 * @LIGMA_BUCKET_FILL_SIMILAR_COLORS: Fill similar colors
 * @LIGMA_BUCKET_FILL_LINE_ART:       Fill by line art detection
 *
 * Bucket fill area.
 */
#define LIGMA_TYPE_BUCKET_FILL_AREA (ligma_bucket_fill_area_get_type ())

GType ligma_bucket_fill_area_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_BUCKET_FILL_SELECTION,       /*< desc="Fill whole selection" >*/
  LIGMA_BUCKET_FILL_SIMILAR_COLORS,  /*< desc="Fill similar colors" >*/
  LIGMA_BUCKET_FILL_LINE_ART         /*< desc="Fill by line art detection" >*/
} LigmaBucketFillArea;


/**
 * LigmaLineArtSource:
 * @LIGMA_LINE_ART_SOURCE_SAMPLE_MERGED: All visible layers
 * @LIGMA_LINE_ART_SOURCE_ACTIVE_LAYER:  Active layer
 * @LIGMA_LINE_ART_SOURCE_LOWER_LAYER:   Layer below the active one
 * @LIGMA_LINE_ART_SOURCE_UPPER_LAYER:   Layer above the active one
 *
 * Bucket fill area.
 */
#define LIGMA_TYPE_LINE_ART_SOURCE (ligma_line_art_source_get_type ())

GType ligma_line_art_source_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_LINE_ART_SOURCE_SAMPLE_MERGED, /*< desc="All visible layers" >*/
  LIGMA_LINE_ART_SOURCE_ACTIVE_LAYER,  /*< desc="Selected layer" >*/
  LIGMA_LINE_ART_SOURCE_LOWER_LAYER,   /*< desc="Layer below the selected one" >*/
  LIGMA_LINE_ART_SOURCE_UPPER_LAYER    /*< desc="Layer above the selected one" >*/
} LigmaLineArtSource;


#define LIGMA_TYPE_RECT_SELECT_MODE (ligma_rect_select_mode_get_type ())

GType ligma_rect_select_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_RECT_SELECT_MODE_FREE,        /*< desc="Free select"        >*/
  LIGMA_RECT_SELECT_MODE_FIXED_SIZE,  /*< desc="Fixed size"         >*/
  LIGMA_RECT_SELECT_MODE_FIXED_RATIO  /*< desc="Fixed aspect ratio" >*/
} LigmaRectSelectMode;


#define LIGMA_TYPE_TRANSFORM_TYPE (ligma_transform_type_get_type ())

GType ligma_transform_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_TRANSFORM_TYPE_LAYER,     /*< desc="Layer"     >*/
  LIGMA_TRANSFORM_TYPE_SELECTION, /*< desc="Selection" >*/
  LIGMA_TRANSFORM_TYPE_PATH,      /*< desc="Path"      >*/
  LIGMA_TRANSFORM_TYPE_IMAGE      /*< desc="Image"     >*/
} LigmaTransformType;


#define LIGMA_TYPE_TOOL_ACTION (ligma_tool_action_get_type ())

GType ligma_tool_action_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_TOOL_ACTION_PAUSE,
  LIGMA_TOOL_ACTION_RESUME,
  LIGMA_TOOL_ACTION_HALT,
  LIGMA_TOOL_ACTION_COMMIT
} LigmaToolAction;


#define LIGMA_TYPE_TOOL_ACTIVE_MODIFIERS (ligma_tool_active_modifiers_get_type ())

GType ligma_tool_active_modifiers_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_TOOL_ACTIVE_MODIFIERS_OFF,
  LIGMA_TOOL_ACTIVE_MODIFIERS_SAME,
  LIGMA_TOOL_ACTIVE_MODIFIERS_SEPARATE,
} LigmaToolActiveModifiers;


#define LIGMA_TYPE_MATTING_DRAW_MODE (ligma_matting_draw_mode_get_type ())

GType ligma_matting_draw_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
 LIGMA_MATTING_DRAW_MODE_FOREGROUND,   /*< desc="Draw foreground" >*/
 LIGMA_MATTING_DRAW_MODE_BACKGROUND,   /*< desc="Draw background" >*/
 LIGMA_MATTING_DRAW_MODE_UNKNOWN,      /*< desc="Draw unknown" >*/
} LigmaMattingDrawMode;


#define LIGMA_TYPE_MATTING_PREVIEW_MODE (ligma_matting_preview_mode_get_type ())

GType ligma_matting_preview_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
 LIGMA_MATTING_PREVIEW_MODE_ON_COLOR,        /*< desc="Color" >*/
 LIGMA_MATTING_PREVIEW_MODE_GRAYSCALE,       /*< desc="Grayscale" >*/
} LigmaMattingPreviewMode;


#define LIGMA_TYPE_TRANSFORM_3D_LENS_MODE (ligma_transform_3d_lens_mode_get_type ())

GType ligma_transform_3d_lens_mode_get_type (void) G_GNUC_CONST;

typedef enum /*< lowercase_name=ligma_transform_3d_lens_mode >*/
{
  LIGMA_TRANSFORM_3D_LENS_MODE_FOCAL_LENGTH, /*< desc="Focal length" >*/
  LIGMA_TRANSFORM_3D_LENS_MODE_FOV_IMAGE,    /*< desc="Field of view (relative to image)", abbrev="FOV (image)" >*/
  LIGMA_TRANSFORM_3D_LENS_MODE_FOV_ITEM,     /*< desc="Field of view (relative to item)", abbrev="FOV (item)" >*/
} Ligma3DTransformLensMode;


#define LIGMA_TYPE_WARP_BEHAVIOR (ligma_warp_behavior_get_type ())

GType ligma_warp_behavior_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_WARP_BEHAVIOR_MOVE,      /*< desc="Move pixels" >*/
  LIGMA_WARP_BEHAVIOR_GROW,      /*< desc="Grow area" >*/
  LIGMA_WARP_BEHAVIOR_SHRINK,    /*< desc="Shrink area" >*/
  LIGMA_WARP_BEHAVIOR_SWIRL_CW,  /*< desc="Swirl clockwise" >*/
  LIGMA_WARP_BEHAVIOR_SWIRL_CCW, /*< desc="Swirl counter-clockwise" >*/
  LIGMA_WARP_BEHAVIOR_ERASE,     /*< desc="Erase warping" >*/
  LIGMA_WARP_BEHAVIOR_SMOOTH     /*< desc="Smooth warping" >*/
} LigmaWarpBehavior;


#define LIGMA_TYPE_PAINT_SELECT_MODE (ligma_paint_select_mode_get_type ())

GType ligma_paint_select_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_PAINT_SELECT_MODE_ADD,      /*< desc="Add to selection" >*/
  LIGMA_PAINT_SELECT_MODE_SUBTRACT, /*< desc="Subtract from selection" >*/
} LigmaPaintSelectMode;

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

/*  Modes of LigmaEditSelectionTool  */
typedef enum /*< skip >*/
{
  LIGMA_TRANSLATE_MODE_VECTORS,
  LIGMA_TRANSLATE_MODE_CHANNEL,
  LIGMA_TRANSLATE_MODE_LAYER_MASK,
  LIGMA_TRANSLATE_MODE_MASK,
  LIGMA_TRANSLATE_MODE_MASK_TO_LAYER,
  LIGMA_TRANSLATE_MODE_MASK_COPY_TO_LAYER,
  LIGMA_TRANSLATE_MODE_LAYER,
  LIGMA_TRANSLATE_MODE_FLOATING_SEL
} LigmaTranslateMode;

/*  Motion event report modes  */
typedef enum /*< skip >*/
{
  LIGMA_MOTION_MODE_EXACT,
  LIGMA_MOTION_MODE_COMPRESS
} LigmaMotionMode;


#endif /* __TOOLS_ENUMS_H__ */
