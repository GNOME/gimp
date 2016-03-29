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

#ifndef __SCREENSHOT_H__
#define __SCREENSHOT_H__


typedef enum
{
  SCREENSHOT_BACKEND_NONE,
  SCREENSHOT_BACKEND_OSX,
  SCREENSHOT_BACKEND_GNOME_SHELL,
  SCREENSHOT_BACKEND_X11
} ScreenshotBackend;

typedef enum
{
  SCREENSHOT_CAN_SHOOT_DECORATIONS     = 0x1 << 0,
  SCREENSHOT_CAN_SHOOT_POINTER         = 0x1 << 1,
  SCREENSHOT_CAN_PICK_NONINTERACTIVELY = 0x1 << 2
} ScreenshotCapabilities;

typedef enum
{
  SHOOT_ROOT,
  SHOOT_REGION,
  SHOOT_WINDOW
} ShootType;

typedef struct
{
  ShootType  shoot_type;
  gboolean   decorate;
  guint      window_id;
  guint      select_delay;
  gint       x1;
  gint       y1;
  gint       x2;
  gint       y2;
  gboolean   show_cursor;
} ScreenshotValues;


void   screenshot_delay (gint seconds);


#endif /* __SCREENSHOT_H__ */
