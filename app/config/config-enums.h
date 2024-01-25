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

#ifndef __CONFIG_ENUMS_H__
#define __CONFIG_ENUMS_H__


#define GIMP_TYPE_CANVAS_PADDING_MODE (gimp_canvas_padding_mode_get_type ())

GType gimp_canvas_padding_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_CANVAS_PADDING_MODE_DEFAULT,      /*< desc="From theme"        >*/
  GIMP_CANVAS_PADDING_MODE_LIGHT_CHECK,  /*< desc="Light check color" >*/
  GIMP_CANVAS_PADDING_MODE_DARK_CHECK,   /*< desc="Dark check color"  >*/
  GIMP_CANVAS_PADDING_MODE_CUSTOM,       /*< desc="Custom color"      >*/
  GIMP_CANVAS_PADDING_MODE_RESET = -1    /*< skip >*/
} GimpCanvasPaddingMode;


#define GIMP_TYPE_CURSOR_FORMAT (gimp_cursor_format_get_type ())

GType gimp_cursor_format_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_CURSOR_FORMAT_BITMAP, /*< desc="Black & white" >*/
  GIMP_CURSOR_FORMAT_PIXBUF  /*< desc="Fancy"         >*/
} GimpCursorFormat;


#define GIMP_TYPE_CURSOR_MODE (gimp_cursor_mode_get_type ())

GType gimp_cursor_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_CURSOR_MODE_TOOL_ICON,       /*< desc="Tool icon"                >*/
  GIMP_CURSOR_MODE_TOOL_CROSSHAIR,  /*< desc="Tool icon with crosshair" >*/
  GIMP_CURSOR_MODE_CROSSHAIR,       /*< desc="Crosshair only"           >*/
} GimpCursorMode;


#define GIMP_TYPE_EXPORT_FILE_TYPE (gimp_export_file_type_get_type ())

GType gimp_export_file_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_EXPORT_FILE_PNG,  /*< desc="PNG Image"                >*/
  GIMP_EXPORT_FILE_JPG,  /*< desc="JPEG Image"               >*/
  GIMP_EXPORT_FILE_ORA,  /*< desc="OpenRaster Image"         >*/
  GIMP_EXPORT_FILE_PSD,  /*< desc="Photoshop Image"          >*/
  GIMP_EXPORT_FILE_PDF,  /*< desc="Portable Document Format" >*/
  GIMP_EXPORT_FILE_TIF,  /*< desc="TIFF Image"               >*/
  GIMP_EXPORT_FILE_BMP,  /*< desc="Windows BMP Image"        >*/
  GIMP_EXPORT_FILE_WEBP, /*< desc="WebP Image"               >*/
} GimpExportFileType;


#define GIMP_TYPE_HANDEDNESS (gimp_handedness_get_type ())

GType gimp_handedness_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_HANDEDNESS_LEFT, /*< desc="Left-handed"  >*/
  GIMP_HANDEDNESS_RIGHT /*< desc="Right-handed" >*/
} GimpHandedness;


#define GIMP_TYPE_HELP_BROWSER_TYPE (gimp_help_browser_type_get_type ())

GType gimp_help_browser_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_HELP_BROWSER_GIMP,        /*< desc="GIMP help browser" >*/
  GIMP_HELP_BROWSER_WEB_BROWSER  /*< desc="Web browser"       >*/
} GimpHelpBrowserType;


#define GIMP_TYPE_ICON_SIZE (gimp_icon_size_get_type ())

GType gimp_icon_size_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_ICON_SIZE_SMALL,   /*< desc="Small size"       > */
  GIMP_ICON_SIZE_MEDIUM,  /*< desc="Medium size"      > */
  GIMP_ICON_SIZE_LARGE,   /*< desc="Large size"       > */
  GIMP_ICON_SIZE_HUGE     /*< desc="Huge size"        > */
} GimpIconSize;


#define GIMP_TYPE_POSITION (gimp_position_get_type ())

GType gimp_position_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_POSITION_TOP,    /*< desc="Top" >*/
  GIMP_POSITION_BOTTOM, /*< desc="Bottom" >*/
  GIMP_POSITION_LEFT,   /*< desc="Left"  >*/
  GIMP_POSITION_RIGHT   /*< desc="Right" >*/
} GimpPosition;

#define GIMP_TYPE_DRAG_ZOOM_MODE (gimp_drag_zoom_mode_get_type ())

GType gimp_drag_zoom_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  PROP_DRAG_ZOOM_MODE_DISTANCE,  /*< desc="By distance" >*/
  PROP_DRAG_ZOOM_MODE_DURATION,  /*< desc="By duration" >*/
} GimpDragZoomMode;

#define GIMP_TYPE_SPACE_BAR_ACTION (gimp_space_bar_action_get_type ())

GType gimp_space_bar_action_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_SPACE_BAR_ACTION_NONE,  /*< desc="No action"           >*/
  GIMP_SPACE_BAR_ACTION_PAN,   /*< desc="Pan view"            >*/
  GIMP_SPACE_BAR_ACTION_MOVE   /*< desc="Switch to Move tool" >*/
} GimpSpaceBarAction;


#define GIMP_TYPE_WINDOW_HINT (gimp_window_hint_get_type ())

GType gimp_window_hint_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_WINDOW_HINT_NORMAL,     /*< desc="Normal window"  >*/
  GIMP_WINDOW_HINT_UTILITY,    /*< desc="Utility window" >*/
  GIMP_WINDOW_HINT_KEEP_ABOVE  /*< desc="Keep above"     >*/
} GimpWindowHint;


#define GIMP_TYPE_ZOOM_QUALITY (gimp_zoom_quality_get_type ())

GType gimp_zoom_quality_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_ZOOM_QUALITY_LOW,   /*< desc="Low"  >*/
  GIMP_ZOOM_QUALITY_HIGH   /*< desc="High" >*/
} GimpZoomQuality;

#define GIMP_TYPE_THEME_SCHEME (gimp_theme_scheme_get_type ())

GType gimp_theme_scheme_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_THEME_LIGHT,  /*< desc="Light Colors" >*/
  GIMP_THEME_GRAY,   /*< desc="Middle Gray"  >*/
  GIMP_THEME_DARK,   /*< desc="Dark Colors"  >*/
  /* TODO: it might be interesting eventually to add a GIMP_THEME_SYSTEM
   * following up the system-wide color scheme preference. See #8675.
   */
} GimpThemeScheme;


#endif /* __CONFIG_ENUMS_H__ */
