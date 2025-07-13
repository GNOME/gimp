/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimage-symmetry.h
 * Copyright (C) 2015 Jehan <jehan@gimp.org>
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


GList        * gimp_image_symmetry_list       (void);

GimpSymmetry * gimp_image_symmetry_new        (GimpImage    *image,
                                               GType         type);
void           gimp_image_symmetry_add        (GimpImage    *image,
                                               GimpSymmetry *sym);
void           gimp_image_symmetry_remove     (GimpImage    *image,
                                               GimpSymmetry *sym);
GList        * gimp_image_symmetry_get        (GimpImage    *image);

gboolean       gimp_image_set_active_symmetry (GimpImage    *image,
                                               GType         type);
GimpSymmetry * gimp_image_get_active_symmetry (GimpImage    *image);
