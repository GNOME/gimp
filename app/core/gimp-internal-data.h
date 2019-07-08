/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
 *
 * gimp-internal-data.h
 * Copyright (C) 2017 Ell
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

#ifndef __GIMP_INTERNAL_DATA__
#define __GIMP_INTERNAL_DATA__


gboolean   gimp_internal_data_load  (Gimp    *gimp,
                                     GError **error);
gboolean   gimp_internal_data_save  (Gimp    *gimp,
                                     GError **error);

gboolean   gimp_internal_data_clear (Gimp    *gimp,
                                     GError **error);


#endif /* __GIMP_INTERNAL_DATA__ */
