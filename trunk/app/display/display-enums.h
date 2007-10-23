/* GIMP - The GNU Image Manipulation Program
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

#ifndef __DISPLAY_ENUMS_H__
#define __DISPLAY_ENUMS_H__


#define GIMP_TYPE_CURSOR_MODE (gimp_cursor_mode_get_type ())

GType gimp_cursor_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_CURSOR_MODE_TOOL_ICON,       /*< desc="Tool icon"                >*/
  GIMP_CURSOR_MODE_TOOL_CROSSHAIR,  /*< desc="Tool icon with crosshair" >*/
  GIMP_CURSOR_MODE_CROSSHAIR        /*< desc="Crosshair only"           >*/
} GimpCursorMode;


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


#define GIMP_TYPE_SPACE_BAR_ACTION (gimp_space_bar_action_get_type ())

GType gimp_space_bar_action_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_SPACE_BAR_ACTION_NONE,  /*< desc="No action"           >*/
  GIMP_SPACE_BAR_ACTION_PAN,   /*< desc="Pan view"            >*/
  GIMP_SPACE_BAR_ACTION_MOVE   /*< desc="Switch to Move tool" >*/
} GimpSpaceBarAction;


#define GIMP_TYPE_ZOOM_QUALITY (gimp_zoom_quality_get_type ())

GType gimp_zoom_quality_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_ZOOM_QUALITY_LOW,   /*< desc="quality|Low"  >*/
  GIMP_ZOOM_QUALITY_HIGH   /*< desc="quality|High" >*/
} GimpZoomQuality;


#endif /* __DISPLAY_ENUMS_H__ */
