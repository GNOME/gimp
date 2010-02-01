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


#endif /* __DISPLAY_ENUMS_H__ */
