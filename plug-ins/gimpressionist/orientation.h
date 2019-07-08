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

#ifndef __ORIENTATION_H
#define __ORIENTATION_H

#define NUMORIENTRADIO 8

enum ORIENTATION_ENUM
{
    ORIENTATION_VALUE = 0,
    ORIENTATION_RADIUS = 1,
    ORIENTATION_RANDOM = 2,
    ORIENTATION_RADIAL = 3,
    ORIENTATION_FLOWING = 4,
    ORIENTATION_HUE = 5,
    ORIENTATION_ADAPTIVE = 6,
    ORIENTATION_MANUAL = 7,
};

void create_orientationpage (GtkNotebook *);
void orientation_restore (void);
int orientation_type_input (int in);

#endif /* #ifndef __ORIENTATION_H */
