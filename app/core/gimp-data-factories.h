/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
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

#ifndef __LIGMA_DATA_FACTORIES_H__
#define __LIGMA_DATA_FACTORIES_H__


void      ligma_data_factories_init        (Ligma               *ligma);
void      ligma_data_factories_add_builtin (Ligma               *ligma);
void      ligma_data_factories_clear       (Ligma               *ligma);
void      ligma_data_factories_exit        (Ligma               *ligma);

gint64    ligma_data_factories_get_memsize (Ligma   *ligma,
                                           gint64 *gui_size);
void      ligma_data_factories_data_clean  (Ligma   *ligma);

void      ligma_data_factories_load        (Ligma               *ligma,
                                           LigmaInitStatusFunc  status_callback);
void      ligma_data_factories_save        (Ligma               *ligma);


#endif /* __LIGMA_DATA_FACTORIES_H__ */
