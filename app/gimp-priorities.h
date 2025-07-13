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

#pragma once


/* #define G_PRIORITY_HIGH -100 */

/* #define G_PRIORITY_DEFAULT 0 */

/* #define G_PRIORITY_HIGH_IDLE 100 */

/* #define GTK_PRIORITY_REDRAW (G_PRIORITY_HIGH_IDLE + 20) */

/*  a bit higher than projection construction  */
#define GIMP_PRIORITY_DISPLAY_SHELL_FILL_IDLE             (G_PRIORITY_HIGH_IDLE + 21)
#define GIMP_PRIORITY_IMAGE_WINDOW_UPDATE_UI_MANAGER_IDLE (G_PRIORITY_HIGH_IDLE + 21)

/*  just a bit less than GDK_PRIORITY_REDRAW   */
#define GIMP_PRIORITY_PROJECTION_IDLE (G_PRIORITY_HIGH_IDLE + 22)

/* #define G_PRIORITY_DEFAULT_IDLE 200 */

#define GIMP_PRIORITY_VIEWABLE_IDLE (G_PRIORITY_LOW)

/* #define G_PRIORITY_LOW 300 */
