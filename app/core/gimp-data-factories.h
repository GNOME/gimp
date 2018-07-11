/* GIMP - The GNU Image Manipulation Program
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

#ifndef __GIMP_DATA_FACTORIES_H__
#define __GIMP_DATA_FACTORIES_H__


void      gimp_data_factories_init        (Gimp               *gimp);
void      gimp_data_factories_add_builtin (Gimp               *gimp);
void      gimp_data_factories_clear       (Gimp               *gimp);
void      gimp_data_factories_exit        (Gimp               *gimp);

gint64    gimp_data_factories_get_memsize (Gimp   *gimp,
                                           gint64 *gui_size);
void      gimp_data_factories_data_clean  (Gimp   *gimp);

void      gimp_data_factories_load        (Gimp               *gimp,
                                           GimpInitStatusFunc  status_callback);
void      gimp_data_factories_save        (Gimp               *gimp);


#endif /* __GIMP_DATA_FACTORIES_H__ */
