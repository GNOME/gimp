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
  SCREENSHOT_BACKEND_WIN32,
  SCREENSHOT_BACKEND_FREEDESKTOP,
  SCREENSHOT_BACKEND_GNOME_SHELL,
  SCREENSHOT_BACKEND_KWIN,
  SCREENSHOT_BACKEND_X11
} ScreenshotBackend;

typedef enum
{
  SCREENSHOT_CAN_SHOOT_DECORATIONS     = 0x1 << 0,
  SCREENSHOT_CAN_SHOOT_POINTER         = 0x1 << 1,
  SCREENSHOT_CAN_PICK_NONINTERACTIVELY = 0x1 << 2,
  SCREENSHOT_CAN_SHOOT_REGION          = 0x1 << 3,
  /* SHOOT_WINDOW mode only: it window selection requires active click. */
  SCREENSHOT_CAN_PICK_WINDOW           = 0x1 << 4,
  /* SHOOT_WINDOW + SCREENSHOT_CAN_PICK_WINDOW only: if a delay can be
   * inserted in-between selection click and actual snapshot. */
  SCREENSHOT_CAN_DELAY_WINDOW_SHOT     = 0x1 << 5,
  SCREENSHOT_CAN_SHOOT_WINDOW          = 0x1 << 6
} ScreenshotCapabilities;

typedef enum
{
  SCREENSHOT_PROFILE_POLICY_MONITOR,
  SCREENSHOT_PROFILE_POLICY_SRGB
} ScreenshotProfilePolicy;

typedef enum
{
  SHOOT_ROOT,
  SHOOT_REGION,
  SHOOT_WINDOW
} ShootType;

typedef struct
{
  ShootType               shoot_type;
  gboolean                decorate;
  guint                   window_id;
  GdkMonitor             *monitor;
  guint                   select_delay;
  guint                   screenshot_delay;
  gint                    x1;
  gint                    y1;
  gint                    x2;
  gint                    y2;
  gboolean                show_cursor;
  ScreenshotProfilePolicy profile_policy;
} ScreenshotValues;


void   screenshot_delay (gint seconds);


#endif /* __SCREENSHOT_H__ */
