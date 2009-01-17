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

#ifndef __GIMP_DRAWABLE_LEVELS_H__
#define __GIMP_DRAWABLE_LEVELS_H__


void   gimp_drawable_levels         (GimpDrawable *drawable,
                                     GimpProgress *progress,
                                     gint32        channel,
                                     gint32        low_input,
                                     gint32        high_input,
                                     gdouble       gamma,
                                     gint32        low_output,
                                     gint32        high_output);

void   gimp_drawable_levels_stretch (GimpDrawable *drawable,
                                     GimpProgress *progress);


#endif  /*  __GIMP_DRAWABLE_LEVELS_H__  */
