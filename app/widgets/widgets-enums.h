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

#ifndef __WIDGETS_ENUMS_H__
#define __WIDGETS_ENUMS_H__


/*
 * enums that are registered with the type system
 */

#define LIGMA_TYPE_ACTIVE_COLOR (ligma_active_color_get_type ())

GType ligma_active_color_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_ACTIVE_COLOR_FOREGROUND, /*< desc="Foreground" >*/
  LIGMA_ACTIVE_COLOR_BACKGROUND  /*< desc="Background" >*/
} LigmaActiveColor;


#define LIGMA_TYPE_CIRCLE_BACKGROUND (ligma_circle_background_get_type ())

GType ligma_circle_background_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_CIRCLE_BACKGROUND_PLAIN, /*< desc="Plain" >*/
  LIGMA_CIRCLE_BACKGROUND_HSV    /*< desc="HSV"   >*/
} LigmaCircleBackground;


#define LIGMA_TYPE_COLOR_DIALOG_STATE (ligma_color_dialog_state_get_type ())

GType ligma_color_dialog_state_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_COLOR_DIALOG_OK,
  LIGMA_COLOR_DIALOG_CANCEL,
  LIGMA_COLOR_DIALOG_UPDATE
} LigmaColorDialogState;


#define LIGMA_TYPE_COLOR_PICK_TARGET (ligma_color_pick_target_get_type ())

GType ligma_color_pick_target_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_COLOR_PICK_TARGET_NONE,       /*< desc="Pick only"            >*/
  LIGMA_COLOR_PICK_TARGET_FOREGROUND, /*< desc="Set foreground color" >*/
  LIGMA_COLOR_PICK_TARGET_BACKGROUND, /*< desc="Set background color" >*/
  LIGMA_COLOR_PICK_TARGET_PALETTE     /*< desc="Add to palette"       >*/
} LigmaColorPickTarget;


#define LIGMA_TYPE_COLOR_PICK_STATE (ligma_color_pick_state_get_type ())

GType ligma_color_pick_state_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_COLOR_PICK_STATE_START,
  LIGMA_COLOR_PICK_STATE_UPDATE,
  LIGMA_COLOR_PICK_STATE_END
} LigmaColorPickState;


#define LIGMA_TYPE_HISTOGRAM_SCALE (ligma_histogram_scale_get_type ())

GType ligma_histogram_scale_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_HISTOGRAM_SCALE_LINEAR,       /*< desc="Linear histogram"      >*/
  LIGMA_HISTOGRAM_SCALE_LOGARITHMIC   /*< desc="Logarithmic histogram" >*/
} LigmaHistogramScale;


#define LIGMA_TYPE_TAB_STYLE (ligma_tab_style_get_type ())

GType ligma_tab_style_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_TAB_STYLE_ICON,          /*< desc="Icon"           >*/
  LIGMA_TAB_STYLE_PREVIEW,       /*< desc="Current status" >*/
  LIGMA_TAB_STYLE_NAME,          /*< desc="Text"           >*/
  LIGMA_TAB_STYLE_BLURB,         /*< desc="Description"    >*/
  LIGMA_TAB_STYLE_ICON_NAME,     /*< desc="Icon & text"    >*/
  LIGMA_TAB_STYLE_ICON_BLURB,    /*< desc="Icon & desc"    >*/
  LIGMA_TAB_STYLE_PREVIEW_NAME,  /*< desc="Status & text"  >*/
  LIGMA_TAB_STYLE_PREVIEW_BLURB  /*< desc="Status & desc"  >*/
} LigmaTabStyle;


#define LIGMA_TYPE_TAG_ENTRY_MODE       (ligma_tag_entry_mode_get_type ())

GType ligma_tag_entry_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_TAG_ENTRY_MODE_QUERY,
  LIGMA_TAG_ENTRY_MODE_ASSIGN,
} LigmaTagEntryMode;


/*
 * non-registered enums; register them if needed
 */

typedef enum  /*< skip >*/
{
  LIGMA_VIEW_BG_CHECKS,
  LIGMA_VIEW_BG_WHITE
} LigmaViewBG;

typedef enum  /*< skip >*/
{
  LIGMA_VIEW_BORDER_BLACK,
  LIGMA_VIEW_BORDER_WHITE,
  LIGMA_VIEW_BORDER_RED,
  LIGMA_VIEW_BORDER_GREEN
} LigmaViewBorderType;

typedef enum  /*< skip >*/
{
  LIGMA_DND_TYPE_NONE         = 0,
  LIGMA_DND_TYPE_URI_LIST     = 1,
  LIGMA_DND_TYPE_TEXT_PLAIN   = 2,
  LIGMA_DND_TYPE_NETSCAPE_URL = 3,
  LIGMA_DND_TYPE_XDS          = 4,
  LIGMA_DND_TYPE_COLOR        = 5,
  LIGMA_DND_TYPE_SVG          = 6,
  LIGMA_DND_TYPE_SVG_XML      = 7,
  LIGMA_DND_TYPE_PIXBUF       = 8,
  LIGMA_DND_TYPE_IMAGE        = 9,
  LIGMA_DND_TYPE_COMPONENT    = 10,
  LIGMA_DND_TYPE_LAYER        = 11,
  LIGMA_DND_TYPE_CHANNEL      = 12,
  LIGMA_DND_TYPE_LAYER_MASK   = 13,
  LIGMA_DND_TYPE_VECTORS      = 14,
  LIGMA_DND_TYPE_BRUSH        = 15,
  LIGMA_DND_TYPE_PATTERN      = 16,
  LIGMA_DND_TYPE_GRADIENT     = 17,
  LIGMA_DND_TYPE_PALETTE      = 18,
  LIGMA_DND_TYPE_FONT         = 19,
  LIGMA_DND_TYPE_BUFFER       = 20,
  LIGMA_DND_TYPE_IMAGEFILE    = 21,
  LIGMA_DND_TYPE_TEMPLATE     = 22,
  LIGMA_DND_TYPE_TOOL_ITEM    = 23,
  LIGMA_DND_TYPE_NOTEBOOK_TAB = 24,

  LIGMA_DND_TYPE_LAYER_LIST   = 25,
  LIGMA_DND_TYPE_CHANNEL_LIST = 26,
  LIGMA_DND_TYPE_VECTORS_LIST = 27,

  LIGMA_DND_TYPE_LAST         = LIGMA_DND_TYPE_VECTORS_LIST
} LigmaDndType;

typedef enum  /*< skip >*/
{
  LIGMA_DROP_NONE,
  LIGMA_DROP_ABOVE,
  LIGMA_DROP_BELOW
} LigmaDropType;

typedef enum  /*< skip >*/
{
  LIGMA_CURSOR_NONE = 1024,  /* (GDK_LAST_CURSOR + 2) yes, this is insane */
  LIGMA_CURSOR_MOUSE,
  LIGMA_CURSOR_CROSSHAIR,
  LIGMA_CURSOR_CROSSHAIR_SMALL,
  LIGMA_CURSOR_BAD,
  LIGMA_CURSOR_MOVE,
  LIGMA_CURSOR_ZOOM,
  LIGMA_CURSOR_COLOR_PICKER,
  LIGMA_CURSOR_CORNER_TOP,
  LIGMA_CURSOR_CORNER_TOP_RIGHT,
  LIGMA_CURSOR_CORNER_RIGHT,
  LIGMA_CURSOR_CORNER_BOTTOM_RIGHT,
  LIGMA_CURSOR_CORNER_BOTTOM,
  LIGMA_CURSOR_CORNER_BOTTOM_LEFT,
  LIGMA_CURSOR_CORNER_LEFT,
  LIGMA_CURSOR_CORNER_TOP_LEFT,
  LIGMA_CURSOR_SIDE_TOP,
  LIGMA_CURSOR_SIDE_TOP_RIGHT,
  LIGMA_CURSOR_SIDE_RIGHT,
  LIGMA_CURSOR_SIDE_BOTTOM_RIGHT,
  LIGMA_CURSOR_SIDE_BOTTOM,
  LIGMA_CURSOR_SIDE_BOTTOM_LEFT,
  LIGMA_CURSOR_SIDE_LEFT,
  LIGMA_CURSOR_SIDE_TOP_LEFT,
  LIGMA_CURSOR_SINGLE_DOT,
  LIGMA_CURSOR_LAST
} LigmaCursorType;

typedef enum  /*< skip >*/
{
  LIGMA_TOOL_CURSOR_NONE,
  LIGMA_TOOL_CURSOR_RECT_SELECT,
  LIGMA_TOOL_CURSOR_ELLIPSE_SELECT,
  LIGMA_TOOL_CURSOR_FREE_SELECT,
  LIGMA_TOOL_CURSOR_POLYGON_SELECT,
  LIGMA_TOOL_CURSOR_FUZZY_SELECT,
  LIGMA_TOOL_CURSOR_PATHS,
  LIGMA_TOOL_CURSOR_PATHS_ANCHOR,
  LIGMA_TOOL_CURSOR_PATHS_CONTROL,
  LIGMA_TOOL_CURSOR_PATHS_SEGMENT,
  LIGMA_TOOL_CURSOR_ISCISSORS,
  LIGMA_TOOL_CURSOR_MOVE,
  LIGMA_TOOL_CURSOR_ZOOM,
  LIGMA_TOOL_CURSOR_CROP,
  LIGMA_TOOL_CURSOR_RESIZE,
  LIGMA_TOOL_CURSOR_ROTATE,
  LIGMA_TOOL_CURSOR_SHEAR,
  LIGMA_TOOL_CURSOR_PERSPECTIVE,
  LIGMA_TOOL_CURSOR_TRANSFORM_3D_CAMERA,
  LIGMA_TOOL_CURSOR_FLIP_HORIZONTAL,
  LIGMA_TOOL_CURSOR_FLIP_VERTICAL,
  LIGMA_TOOL_CURSOR_TEXT,
  LIGMA_TOOL_CURSOR_COLOR_PICKER,
  LIGMA_TOOL_CURSOR_BUCKET_FILL,
  LIGMA_TOOL_CURSOR_GRADIENT,
  LIGMA_TOOL_CURSOR_PENCIL,
  LIGMA_TOOL_CURSOR_PAINTBRUSH,
  LIGMA_TOOL_CURSOR_AIRBRUSH,
  LIGMA_TOOL_CURSOR_INK,
  LIGMA_TOOL_CURSOR_CLONE,
  LIGMA_TOOL_CURSOR_HEAL,
  LIGMA_TOOL_CURSOR_ERASER,
  LIGMA_TOOL_CURSOR_SMUDGE,
  LIGMA_TOOL_CURSOR_BLUR,
  LIGMA_TOOL_CURSOR_DODGE,
  LIGMA_TOOL_CURSOR_BURN,
  LIGMA_TOOL_CURSOR_MEASURE,
  LIGMA_TOOL_CURSOR_WARP,
  LIGMA_TOOL_CURSOR_HAND,
  LIGMA_TOOL_CURSOR_LAST
} LigmaToolCursorType;

typedef enum  /*< skip >*/
{
  LIGMA_CURSOR_MODIFIER_NONE,
  LIGMA_CURSOR_MODIFIER_BAD,
  LIGMA_CURSOR_MODIFIER_PLUS,
  LIGMA_CURSOR_MODIFIER_MINUS,
  LIGMA_CURSOR_MODIFIER_INTERSECT,
  LIGMA_CURSOR_MODIFIER_MOVE,
  LIGMA_CURSOR_MODIFIER_RESIZE,
  LIGMA_CURSOR_MODIFIER_ROTATE,
  LIGMA_CURSOR_MODIFIER_ZOOM,
  LIGMA_CURSOR_MODIFIER_CONTROL,
  LIGMA_CURSOR_MODIFIER_ANCHOR,
  LIGMA_CURSOR_MODIFIER_FOREGROUND,
  LIGMA_CURSOR_MODIFIER_BACKGROUND,
  LIGMA_CURSOR_MODIFIER_PATTERN,
  LIGMA_CURSOR_MODIFIER_JOIN,
  LIGMA_CURSOR_MODIFIER_SELECT,
  LIGMA_CURSOR_MODIFIER_LAST
} LigmaCursorModifier;

typedef enum  /*< skip >*/
{
  LIGMA_DEVICE_VALUE_MODE       = 1 << 0,
  LIGMA_DEVICE_VALUE_AXES       = 1 << 1,
  LIGMA_DEVICE_VALUE_KEYS       = 1 << 2,
  LIGMA_DEVICE_VALUE_TOOL       = 1 << 3,
  LIGMA_DEVICE_VALUE_FOREGROUND = 1 << 4,
  LIGMA_DEVICE_VALUE_BACKGROUND = 1 << 5,
  LIGMA_DEVICE_VALUE_BRUSH      = 1 << 6,
  LIGMA_DEVICE_VALUE_PATTERN    = 1 << 7,
  LIGMA_DEVICE_VALUE_GRADIENT   = 1 << 8
} LigmaDeviceValues;

typedef enum  /*< skip >*/
{
  LIGMA_DIALOGS_SHOWN,
  LIGMA_DIALOGS_HIDDEN_EXPLICITLY,  /* user used the Tab key to hide dialogs */
  LIGMA_DIALOGS_HIDDEN_WITH_DISPLAY /* dialogs are hidden with the display   */
} LigmaDialogsState;

typedef enum  /*< skip >*/
{
  LIGMA_DASHBOARD_UPDATE_INTERVAL_0_25_SEC =    250,
  LIGMA_DASHBOARD_UPDATE_INTERVAL_0_5_SEC  =    500,
  LIGMA_DASHBOARD_UPDATE_INTERVAL_1_SEC    =   1000,
  LIGMA_DASHBOARD_UPDATE_INTERVAL_2_SEC    =   2000,
  LIGMA_DASHBOARD_UPDATE_INTERVAL_4_SEC    =   4000
} LigmaDashboardUpdateInteval;

typedef enum  /*< skip >*/
{
  LIGMA_DASHBOARD_HISTORY_DURATION_15_SEC  =  15000,
  LIGMA_DASHBOARD_HISTORY_DURATION_30_SEC  =  30000,
  LIGMA_DASHBOARD_HISTORY_DURATION_60_SEC  =  60000,
  LIGMA_DASHBOARD_HISTORY_DURATION_120_SEC = 120000,
  LIGMA_DASHBOARD_HISTORY_DURATION_240_SEC = 240000
} LigmaDashboardHistoryDuration;


#endif /* __WIDGETS_ENUMS_H__ */
