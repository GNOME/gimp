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


typedef enum
{
  GIMP_AUTO_SHRINK_SHRINK,
  GIMP_AUTO_SHRINK_EMPTY,
  GIMP_AUTO_SHRINK_UNSHRINKABLE
} GimpAutoShrink;


GimpAutoShrink   gimp_pickable_auto_shrink (GimpPickable *pickable,
                                            gint         x,
                                            gint         y,
                                            gint         width,
                                            gint         height,
                                            gint        *shrunk_x,
                                            gint        *shrunk_y,
                                            gint        *shrunk_width,
                                            gint        *shrunk_height);
