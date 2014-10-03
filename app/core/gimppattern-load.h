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

#ifndef __GIMP_PATTERN_LOAD_H__
#define __GIMP_PATTERN_LOAD_H__


#define GIMP_PATTERN_FILE_EXTENSION ".pat"


GList * gimp_pattern_load        (GimpContext   *context,
                                  GFile         *file,
                                  GInputStream  *input,
                                  GError       **error);
GList * gimp_pattern_load_pixbuf (GimpContext   *context,
                                  GFile         *file,
                                  GInputStream  *input,
                                  GError       **error);


#endif /* __GIMP_PATTERN_LOAD_H__ */
