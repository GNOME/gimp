/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmacontainer-filter.c
 * Copyright (C) 2003  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_CONTAINER_FILTER_H__
#define __LIGMA_CONTAINER_FILTER_H__


LigmaContainer * ligma_container_filter         (LigmaContainer        *container,
                                               LigmaObjectFilterFunc  filter,
                                               gpointer              user_data);
LigmaContainer * ligma_container_filter_by_name (LigmaContainer        *container,
                                               const gchar          *regexp,
                                               GError              **error);

gchar        ** ligma_container_get_filtered_name_array
                                              (LigmaContainer        *container,
                                               const gchar          *regexp);


#endif  /* __LIGMA_CONTAINER_FILTER_H__ */
