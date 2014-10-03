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

#ifndef __GIMP_PICKABLE_AUTO_SHRINK_H__
#define __GIMP_PICKABLE_AUTO_SHRINK_H__


typedef enum
{
  GIMP_AUTO_SHRINK_SHRINK,
  GIMP_AUTO_SHRINK_EMPTY,
  GIMP_AUTO_SHRINK_UNSHRINKABLE
} GimpAutoShrink;


GimpAutoShrink   gimp_pickable_auto_shrink (GimpPickable *pickable,
                                            gint         x1,
                                            gint         y1,
                                            gint         x2,
                                            gint         y2,
                                            gint        *shrunk_x1,
                                            gint        *shrunk_y1,
                                            gint        *shrunk_x2,
                                            gint        *shrunk_y2);


#endif  /* __GIMP_PICKABLE_AUTO_SHRINK_H__ */
