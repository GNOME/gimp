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

#ifndef __SIZE_H
#define __SIZE_H

enum SIZE_TYPE_ENUM
{
    SIZE_TYPE_VALUE = 0,
    SIZE_TYPE_RADIUS = 1,
    SIZE_TYPE_RANDOM = 2,
    SIZE_TYPE_RADIAL = 3,
    SIZE_TYPE_FLOWING = 4,
    SIZE_TYPE_HUE = 5,
    SIZE_TYPE_ADAPTIVE = 6,
    SIZE_TYPE_MANUAL = 7,
};

void size_restore (void);

void create_sizepage (GtkNotebook *);

int size_type_input (int in);
void size_map_free_resources (void);

#endif /* #ifndef __SIZE_H */
