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

#ifndef __CONFIG_ENUMS_H__
#define __CONFIG_ENUMS_H__


#define LIGMA_TYPE_CANVAS_PADDING_MODE (ligma_canvas_padding_mode_get_type ())

GType ligma_canvas_padding_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_CANVAS_PADDING_MODE_DEFAULT,      /*< desc="From theme"        >*/
  LIGMA_CANVAS_PADDING_MODE_LIGHT_CHECK,  /*< desc="Light check color" >*/
  LIGMA_CANVAS_PADDING_MODE_DARK_CHECK,   /*< desc="Dark check color"  >*/
  LIGMA_CANVAS_PADDING_MODE_CUSTOM,       /*< desc="Custom color"      >*/
  LIGMA_CANVAS_PADDING_MODE_RESET = -1    /*< skip >*/
} LigmaCanvasPaddingMode;


#define LIGMA_TYPE_CURSOR_FORMAT (ligma_cursor_format_get_type ())

GType ligma_cursor_format_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_CURSOR_FORMAT_BITMAP, /*< desc="Black & white" >*/
  LIGMA_CURSOR_FORMAT_PIXBUF  /*< desc="Fancy"         >*/
} LigmaCursorFormat;


#define LIGMA_TYPE_CURSOR_MODE (ligma_cursor_mode_get_type ())

GType ligma_cursor_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_CURSOR_MODE_TOOL_ICON,       /*< desc="Tool icon"                >*/
  LIGMA_CURSOR_MODE_TOOL_CROSSHAIR,  /*< desc="Tool icon with crosshair" >*/
  LIGMA_CURSOR_MODE_CROSSHAIR,       /*< desc="Crosshair only"           >*/
} LigmaCursorMode;


#define LIGMA_TYPE_EXPORT_FILE_TYPE (ligma_export_file_type_get_type ())

GType ligma_export_file_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_EXPORT_FILE_PNG,  /*< desc="PNG Image"                >*/
  LIGMA_EXPORT_FILE_JPG,  /*< desc="JPEG Image"               >*/
  LIGMA_EXPORT_FILE_ORA,  /*< desc="OpenRaster Image"         >*/
  LIGMA_EXPORT_FILE_PSD,  /*< desc="Photoshop Image"          >*/
  LIGMA_EXPORT_FILE_PDF,  /*< desc="Portable Document Format" >*/
  LIGMA_EXPORT_FILE_TIF,  /*< desc="TIFF Image"               >*/
  LIGMA_EXPORT_FILE_BMP,  /*< desc="Windows BMP Image"        >*/
  LIGMA_EXPORT_FILE_WEBP, /*< desc="WebP Image"               >*/
} LigmaExportFileType;


#define LIGMA_TYPE_HANDEDNESS (ligma_handedness_get_type ())

GType ligma_handedness_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_HANDEDNESS_LEFT, /*< desc="Left-handed"  >*/
  LIGMA_HANDEDNESS_RIGHT /*< desc="Right-handed" >*/
} LigmaHandedness;


#define LIGMA_TYPE_HELP_BROWSER_TYPE (ligma_help_browser_type_get_type ())

GType ligma_help_browser_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_HELP_BROWSER_LIGMA,        /*< desc="LIGMA help browser" >*/
  LIGMA_HELP_BROWSER_WEB_BROWSER  /*< desc="Web browser"       >*/
} LigmaHelpBrowserType;


#define LIGMA_TYPE_ICON_SIZE (ligma_icon_size_get_type ())

GType ligma_icon_size_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_ICON_SIZE_SMALL,   /*< desc="Small size"       > */
  LIGMA_ICON_SIZE_MEDIUM,  /*< desc="Medium size"      > */
  LIGMA_ICON_SIZE_LARGE,   /*< desc="Large size"       > */
  LIGMA_ICON_SIZE_HUGE     /*< desc="Huge size"        > */
} LigmaIconSize;


#define LIGMA_TYPE_POSITION (ligma_position_get_type ())

GType ligma_position_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_POSITION_TOP,    /*< desc="Top" >*/
  LIGMA_POSITION_BOTTOM, /*< desc="Bottom" >*/
  LIGMA_POSITION_LEFT,   /*< desc="Left"  >*/
  LIGMA_POSITION_RIGHT   /*< desc="Right" >*/
} LigmaPosition;

#define LIGMA_TYPE_DRAG_ZOOM_MODE (ligma_drag_zoom_mode_get_type ())

GType ligma_drag_zoom_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  PROP_DRAG_ZOOM_MODE_DISTANCE,  /*< desc="By distance" >*/
  PROP_DRAG_ZOOM_MODE_DURATION,  /*< desc="By duration" >*/
} LigmaDragZoomMode;

#define LIGMA_TYPE_SPACE_BAR_ACTION (ligma_space_bar_action_get_type ())

GType ligma_space_bar_action_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_SPACE_BAR_ACTION_NONE,  /*< desc="No action"           >*/
  LIGMA_SPACE_BAR_ACTION_PAN,   /*< desc="Pan view"            >*/
  LIGMA_SPACE_BAR_ACTION_MOVE   /*< desc="Switch to Move tool" >*/
} LigmaSpaceBarAction;


#define LIGMA_TYPE_WINDOW_HINT (ligma_window_hint_get_type ())

GType ligma_window_hint_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_WINDOW_HINT_NORMAL,     /*< desc="Normal window"  >*/
  LIGMA_WINDOW_HINT_UTILITY,    /*< desc="Utility window" >*/
  LIGMA_WINDOW_HINT_KEEP_ABOVE  /*< desc="Keep above"     >*/
} LigmaWindowHint;


#define LIGMA_TYPE_ZOOM_QUALITY (ligma_zoom_quality_get_type ())

GType ligma_zoom_quality_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_ZOOM_QUALITY_LOW,   /*< desc="Low"  >*/
  LIGMA_ZOOM_QUALITY_HIGH   /*< desc="High" >*/
} LigmaZoomQuality;


#endif /* __CONFIG_ENUMS_H__ */
