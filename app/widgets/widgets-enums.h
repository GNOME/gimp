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

#ifndef __WIDGETS_ENUMS_H__
#define __WIDGETS_ENUMS_H__


/*
 * these enums that are registered with the type system
 */

#define GIMP_TYPE_ASPECT_TYPE (gimp_aspect_type_get_type ())

GType gimp_aspect_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_ASPECT_SQUARE,
  GIMP_ASPECT_PORTRAIT,       /*< desc="Portrait"  >*/
  GIMP_ASPECT_LANDSCAPE       /*< desc="Landscape" >*/
} GimpAspectType;


#define GIMP_TYPE_COLOR_FRAME_MODE (gimp_color_frame_mode_get_type ())

GType gimp_color_frame_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_COLOR_FRAME_MODE_PIXEL,  /*< desc="Pixel Values" >*/
  GIMP_COLOR_FRAME_MODE_RGB,    /*< desc="RGB"          >*/
  GIMP_COLOR_FRAME_MODE_HSV,    /*< desc="HSV"          >*/
  GIMP_COLOR_FRAME_MODE_CMYK    /*< desc="CMYK"         >*/
} GimpColorFrameMode;


#define GIMP_TYPE_HELP_BROWSER_TYPE (gimp_help_browser_type_get_type ())

GType gimp_help_browser_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_HELP_BROWSER_GIMP,        /*< desc="Internal"    >*/
  GIMP_HELP_BROWSER_WEB_BROWSER  /*< desc="Web Browser" >*/
} GimpHelpBrowserType;


#define GIMP_TYPE_HISTOGRAM_SCALE (gimp_histogram_scale_get_type ())

GType gimp_histogram_scale_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_HISTOGRAM_SCALE_LINEAR,       /*< desc="Linear"      >*/
  GIMP_HISTOGRAM_SCALE_LOGARITHMIC   /*< desc="Logarithmic" >*/
} GimpHistogramScale;


#define GIMP_TYPE_TAB_STYLE (gimp_tab_style_get_type ())

GType gimp_tab_style_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_TAB_STYLE_ICON,          /*< desc="Icon"           >*/
  GIMP_TAB_STYLE_PREVIEW,       /*< desc="Current Status" >*/
  GIMP_TAB_STYLE_NAME,          /*< desc="Text"           >*/
  GIMP_TAB_STYLE_BLURB,         /*< desc="Description"    >*/
  GIMP_TAB_STYLE_ICON_NAME,     /*< desc="Icon & Text"    >*/
  GIMP_TAB_STYLE_ICON_BLURB,    /*< desc="Icon & Desc"    >*/
  GIMP_TAB_STYLE_PREVIEW_NAME,  /*< desc="Status & Text"  >*/
  GIMP_TAB_STYLE_PREVIEW_BLURB  /*< desc="Status & Desc"  >*/
} GimpTabStyle;


#define GIMP_TYPE_VIEW_TYPE (gimp_view_type_get_type ())

GType gimp_view_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_VIEW_TYPE_LIST,  /*< desc="View as List" >*/
  GIMP_VIEW_TYPE_GRID   /*< desc="View as Grid" >*/
} GimpViewType;


#define GIMP_TYPE_WINDOW_HINT (gimp_window_hint_get_type ())

GType gimp_window_hint_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_WINDOW_HINT_NORMAL,     /*< desc="Normal Window"  >*/
  GIMP_WINDOW_HINT_UTILITY,    /*< desc="Utility Window" >*/
  GIMP_WINDOW_HINT_KEEP_ABOVE  /*< desc="Keep Above"     >*/
} GimpWindowHint;


#define GIMP_TYPE_ZOOM_TYPE (gimp_zoom_type_get_type ())

GType gimp_zoom_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_ZOOM_IN,  /*< desc="Zoom in"  >*/
  GIMP_ZOOM_OUT, /*< desc="Zoom out" >*/
  GIMP_ZOOM_TO   /*< skip >*/
} GimpZoomType;


/*
 * non-registered enums; register them if needed
 */

typedef enum  /*< skip >*/
{
  GIMP_PREVIEW_BG_CHECKS,
  GIMP_PREVIEW_BG_WHITE
} GimpPreviewBG;

typedef enum  /*< skip >*/
{
  GIMP_PREVIEW_BORDER_BLACK,
  GIMP_PREVIEW_BORDER_WHITE,
  GIMP_PREVIEW_BORDER_RED,
  GIMP_PREVIEW_BORDER_GREEN,
} GimpPreviewBorderType;

typedef enum  /*< skip >*/
{
  GIMP_DROP_NONE,
  GIMP_DROP_ABOVE,
  GIMP_DROP_BELOW
} GimpDropType;

typedef enum  /*< skip >*/
{
  GIMP_MOUSE_CURSOR = 1024  /* (GDK_LAST_CURSOR + 2) yes, this is insane */,
  GIMP_CROSSHAIR_CURSOR,
  GIMP_CROSSHAIR_SMALL_CURSOR,
  GIMP_BAD_CURSOR,
  GIMP_ZOOM_CURSOR,
  GIMP_COLOR_PICKER_CURSOR,
  GIMP_LAST_CURSOR_ENTRY
} GimpCursorType;

typedef enum  /*< skip >*/
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
  GIMP_HAND_TOOL_CURSOR,
  GIMP_LAST_STOCK_TOOL_CURSOR_ENTRY
} GimpToolCursorType;

typedef enum  /*< skip >*/
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
  GIMP_LAST_CURSOR_MODIFIER_ENTRY
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


#endif /* __WIDGETS_ENUMS_H__ */
