/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpcontainer-filter.c
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_CONTAINER_FILTER_H__
#define __GIMP_CONTAINER_FILTER_H__


GimpContainer * gimp_container_filter         (GimpContainer        *container,
                                               GimpObjectFilterFunc  filter,
                                               gpointer              user_data);
GimpContainer * gimp_container_filter_by_name (GimpContainer        *container,
                                               const gchar          *regexp,
                                               GError              **error);

gchar        ** gimp_container_get_filtered_name_array
                                              (GimpContainer        *container,
                                               const gchar          *regexp);


#endif  /* __GIMP_CONTAINER_FILTER_H__ */
