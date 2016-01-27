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

#ifndef __DISPLAY_ENUMS_H__
#define __DISPLAY_ENUMS_H__


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
  GIMP_HANDLE_FILLED_SQUARE,
  GIMP_HANDLE_CIRCLE,
  GIMP_HANDLE_FILLED_CIRCLE,
  GIMP_HANDLE_DIAMOND,
  GIMP_HANDLE_FILLED_DIAMOND,
  GIMP_HANDLE_CROSS,
  GIMP_HANDLE_CROSSHAIR
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


#define GIMP_TYPE_PATH_STYLE (gimp_path_style_get_type ())

GType gimp_path_style_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_PATH_STYLE_DEFAULT,
  GIMP_PATH_STYLE_VECTORS,
  GIMP_PATH_STYLE_OUTLINE
} GimpPathStyle;


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

#define GIMP_TYPE_GUIDE_STYLE (gimp_guide_style_get_type ())

GType gimp_guide_style_get_type (void) G_GNUC_CONST;

typedef enum
{
    GIMP_GUIDE_STYLE_NONE,
    GIMP_GUIDE_STYLE_NORMAL,
    GIMP_GUIDE_STYLE_MIRROR
} GimpGuideStyle;

#endif /* __DISPLAY_ENUMS_H__ */
