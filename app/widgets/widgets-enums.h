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

#ifndef __WIDGETS_ENUMS_H__
#define __WIDGETS_ENUMS_H__


/*
 * enums that are registered with the type system
 */

#define GIMP_TYPE_ACTIVE_COLOR (gimp_active_color_get_type ())

GType gimp_active_color_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_ACTIVE_COLOR_FOREGROUND, /*< desc="Foreground" >*/
  GIMP_ACTIVE_COLOR_BACKGROUND  /*< desc="Background" >*/
} GimpActiveColor;


#define GIMP_TYPE_CIRCLE_BACKGROUND (gimp_circle_background_get_type ())

GType gimp_circle_background_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_CIRCLE_BACKGROUND_PLAIN, /*< desc="Plain" >*/
  GIMP_CIRCLE_BACKGROUND_HSV    /*< desc="HSV"   >*/
} GimpCircleBackground;


#define GIMP_TYPE_COLOR_DIALOG_STATE (gimp_color_dialog_state_get_type ())

GType gimp_color_dialog_state_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_COLOR_DIALOG_OK,
  GIMP_COLOR_DIALOG_CANCEL,
  GIMP_COLOR_DIALOG_UPDATE
} GimpColorDialogState;


#define GIMP_TYPE_COLOR_FRAME_MODE (gimp_color_frame_mode_get_type ())

GType gimp_color_frame_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_COLOR_FRAME_MODE_PIXEL,  /*< desc="Pixel" >*/
  GIMP_COLOR_FRAME_MODE_RGB,    /*< desc="RGB"   >*/
  GIMP_COLOR_FRAME_MODE_HSV,    /*< desc="HSV"   >*/
  GIMP_COLOR_FRAME_MODE_CMYK    /*< desc="CMYK"  >*/
} GimpColorFrameMode;


#define GIMP_TYPE_COLOR_PICK_MODE (gimp_color_pick_mode_get_type ())

GType gimp_color_pick_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_COLOR_PICK_MODE_NONE,       /*< desc="Pick only"            >*/
  GIMP_COLOR_PICK_MODE_FOREGROUND, /*< desc="Set foreground color" >*/
  GIMP_COLOR_PICK_MODE_BACKGROUND, /*< desc="Set background color" >*/
  GIMP_COLOR_PICK_MODE_PALETTE     /*< desc="Add to palette"       >*/
} GimpColorPickMode;


#define GIMP_TYPE_COLOR_PICK_STATE (gimp_color_pick_state_get_type ())

GType gimp_color_pick_state_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_COLOR_PICK_STATE_NEW,
  GIMP_COLOR_PICK_STATE_UPDATE
} GimpColorPickState;


#define GIMP_TYPE_HISTOGRAM_SCALE (gimp_histogram_scale_get_type ())

GType gimp_histogram_scale_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_HISTOGRAM_SCALE_LINEAR,       /*< desc="Linear histogram"      >*/
  GIMP_HISTOGRAM_SCALE_LOGARITHMIC   /*< desc="Logarithmic histogram" >*/
} GimpHistogramScale;


#define GIMP_TYPE_TAB_STYLE (gimp_tab_style_get_type ())

GType gimp_tab_style_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_TAB_STYLE_ICON,          /*< desc="Icon"           >*/
  GIMP_TAB_STYLE_PREVIEW,       /*< desc="Current status" >*/
  GIMP_TAB_STYLE_NAME,          /*< desc="Text"           >*/
  GIMP_TAB_STYLE_BLURB,         /*< desc="Description"    >*/
  GIMP_TAB_STYLE_ICON_NAME,     /*< desc="Icon & text"    >*/
  GIMP_TAB_STYLE_ICON_BLURB,    /*< desc="Icon & desc"    >*/
  GIMP_TAB_STYLE_PREVIEW_NAME,  /*< desc="Status & text"  >*/
  GIMP_TAB_STYLE_PREVIEW_BLURB, /*< desc="Status & desc"  >*/
  GIMP_TAB_STYLE_UNDEFINED,     /*< desc="Undefined"      >*/
  GIMP_TAB_STYLE_AUTOMATIC      /*< desc="Automatic"      >*/
} GimpTabStyle;


#define GIMP_TYPE_TAG_ENTRY_MODE       (gimp_tag_entry_mode_get_type ())

GType gimp_tag_entry_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_TAG_ENTRY_MODE_QUERY,
  GIMP_TAG_ENTRY_MODE_ASSIGN,
} GimpTagEntryMode;


/*
 * non-registered enums; register them if needed
 */

typedef enum  /*< skip >*/
{
  GIMP_VIEW_BG_CHECKS,
  GIMP_VIEW_BG_WHITE
} GimpViewBG;

typedef enum  /*< skip >*/
{
  GIMP_VIEW_BORDER_BLACK,
  GIMP_VIEW_BORDER_WHITE,
  GIMP_VIEW_BORDER_RED,
  GIMP_VIEW_BORDER_GREEN
} GimpViewBorderType;

typedef enum  /*< skip >*/
{
  GIMP_DND_TYPE_NONE         = 0,
  GIMP_DND_TYPE_URI_LIST     = 1,
  GIMP_DND_TYPE_TEXT_PLAIN   = 2,
  GIMP_DND_TYPE_NETSCAPE_URL = 3,
  GIMP_DND_TYPE_XDS          = 4,
  GIMP_DND_TYPE_COLOR        = 5,
  GIMP_DND_TYPE_SVG          = 6,
  GIMP_DND_TYPE_SVG_XML      = 7,
  GIMP_DND_TYPE_PIXBUF       = 8,
  GIMP_DND_TYPE_IMAGE        = 9,
  GIMP_DND_TYPE_COMPONENT    = 10,
  GIMP_DND_TYPE_LAYER        = 11,
  GIMP_DND_TYPE_CHANNEL      = 12,
  GIMP_DND_TYPE_LAYER_MASK   = 13,
  GIMP_DND_TYPE_VECTORS      = 14,
  GIMP_DND_TYPE_BRUSH        = 15,
  GIMP_DND_TYPE_PATTERN      = 16,
  GIMP_DND_TYPE_GRADIENT     = 17,
  GIMP_DND_TYPE_PALETTE      = 18,
  GIMP_DND_TYPE_FONT         = 19,
  GIMP_DND_TYPE_BUFFER       = 20,
  GIMP_DND_TYPE_IMAGEFILE    = 21,
  GIMP_DND_TYPE_TEMPLATE     = 22,
  GIMP_DND_TYPE_TOOL_INFO    = 23,
  GIMP_DND_TYPE_DIALOG       = 24,

  GIMP_DND_TYPE_LAST         = GIMP_DND_TYPE_DIALOG
} GimpDndType;

typedef enum  /*< skip >*/
{
  GIMP_DROP_NONE,
  GIMP_DROP_ABOVE,
  GIMP_DROP_BELOW
} GimpDropType;

typedef enum  /*< skip >*/
{
  GIMP_CURSOR_NONE = 1024,  /* (GDK_LAST_CURSOR + 2) yes, this is insane */
  GIMP_CURSOR_MOUSE,
  GIMP_CURSOR_CROSSHAIR,
  GIMP_CURSOR_CROSSHAIR_SMALL,
  GIMP_CURSOR_BAD,
  GIMP_CURSOR_MOVE,
  GIMP_CURSOR_ZOOM,
  GIMP_CURSOR_COLOR_PICKER,
  GIMP_CURSOR_CORNER_TOP,
  GIMP_CURSOR_CORNER_TOP_RIGHT,
  GIMP_CURSOR_CORNER_RIGHT,
  GIMP_CURSOR_CORNER_BOTTOM_RIGHT,
  GIMP_CURSOR_CORNER_BOTTOM,
  GIMP_CURSOR_CORNER_BOTTOM_LEFT,
  GIMP_CURSOR_CORNER_LEFT,
  GIMP_CURSOR_CORNER_TOP_LEFT,
  GIMP_CURSOR_SIDE_TOP,
  GIMP_CURSOR_SIDE_TOP_RIGHT,
  GIMP_CURSOR_SIDE_RIGHT,
  GIMP_CURSOR_SIDE_BOTTOM_RIGHT,
  GIMP_CURSOR_SIDE_BOTTOM,
  GIMP_CURSOR_SIDE_BOTTOM_LEFT,
  GIMP_CURSOR_SIDE_LEFT,
  GIMP_CURSOR_SIDE_TOP_LEFT,
  GIMP_CURSOR_LAST
} GimpCursorType;

typedef enum  /*< skip >*/
{
  GIMP_TOOL_CURSOR_NONE,
  GIMP_TOOL_CURSOR_RECT_SELECT,
  GIMP_TOOL_CURSOR_ELLIPSE_SELECT,
  GIMP_TOOL_CURSOR_FREE_SELECT,
  GIMP_TOOL_CURSOR_POLYGON_SELECT,
  GIMP_TOOL_CURSOR_FUZZY_SELECT,
  GIMP_TOOL_CURSOR_PATHS,
  GIMP_TOOL_CURSOR_PATHS_ANCHOR,
  GIMP_TOOL_CURSOR_PATHS_CONTROL,
  GIMP_TOOL_CURSOR_PATHS_SEGMENT,
  GIMP_TOOL_CURSOR_ISCISSORS,
  GIMP_TOOL_CURSOR_MOVE,
  GIMP_TOOL_CURSOR_ZOOM,
  GIMP_TOOL_CURSOR_CROP,
  GIMP_TOOL_CURSOR_RESIZE,
  GIMP_TOOL_CURSOR_ROTATE,
  GIMP_TOOL_CURSOR_SHEAR,
  GIMP_TOOL_CURSOR_PERSPECTIVE,
  GIMP_TOOL_CURSOR_FLIP_HORIZONTAL,
  GIMP_TOOL_CURSOR_FLIP_VERTICAL,
  GIMP_TOOL_CURSOR_TEXT,
  GIMP_TOOL_CURSOR_COLOR_PICKER,
  GIMP_TOOL_CURSOR_BUCKET_FILL,
  GIMP_TOOL_CURSOR_BLEND,
  GIMP_TOOL_CURSOR_PENCIL,
  GIMP_TOOL_CURSOR_PAINTBRUSH,
  GIMP_TOOL_CURSOR_AIRBRUSH,
  GIMP_TOOL_CURSOR_INK,
  GIMP_TOOL_CURSOR_CLONE,
  GIMP_TOOL_CURSOR_HEAL,
  GIMP_TOOL_CURSOR_ERASER,
  GIMP_TOOL_CURSOR_SMUDGE,
  GIMP_TOOL_CURSOR_BLUR,
  GIMP_TOOL_CURSOR_DODGE,
  GIMP_TOOL_CURSOR_BURN,
  GIMP_TOOL_CURSOR_MEASURE,
  GIMP_TOOL_CURSOR_HAND,
  GIMP_TOOL_CURSOR_LAST
} GimpToolCursorType;

typedef enum  /*< skip >*/
{
  GIMP_CURSOR_MODIFIER_NONE,
  GIMP_CURSOR_MODIFIER_BAD,
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
  GIMP_CURSOR_MODIFIER_JOIN,
  GIMP_CURSOR_MODIFIER_SELECT,
  GIMP_CURSOR_MODIFIER_LAST
} GimpCursorModifier;

typedef enum  /*< skip >*/
{
  GIMP_DEVICE_VALUE_MODE       = 1 << 0,
  GIMP_DEVICE_VALUE_AXES       = 1 << 1,
  GIMP_DEVICE_VALUE_KEYS       = 1 << 2,
  GIMP_DEVICE_VALUE_TOOL       = 1 << 3,
  GIMP_DEVICE_VALUE_FOREGROUND = 1 << 4,
  GIMP_DEVICE_VALUE_BACKGROUND = 1 << 5,
  GIMP_DEVICE_VALUE_BRUSH      = 1 << 6,
  GIMP_DEVICE_VALUE_PATTERN    = 1 << 7,
  GIMP_DEVICE_VALUE_GRADIENT   = 1 << 8
} GimpDeviceValues;

typedef enum  /*< skip >*/
{
  GIMP_FILE_CHOOSER_ACTION_OPEN,
  GIMP_FILE_CHOOSER_ACTION_SAVE,
  GIMP_FILE_CHOOSER_ACTION_EXPORT
} GimpFileChooserAction;

typedef enum  /*< skip >*/
{
  GIMP_DIALOGS_SHOWN,
  GIMP_DIALOGS_HIDDEN_EXPLICITLY,  /* user used the Tab key to hide dialogs */
  GIMP_DIALOGS_HIDDEN_WITH_DISPLAY /* dialogs are hidden with the display   */
} GimpDialogsState;


#endif /* __WIDGETS_ENUMS_H__ */
