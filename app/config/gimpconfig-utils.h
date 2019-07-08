/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * Utitility functions for GimpConfig.
 * Copyright (C) 2001-2003  Sven Neumann <sven@gimp.org>
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

#ifndef __APP_GIMP_CONFIG_UTILS_H__
#define __APP_GIMP_CONFIG_UTILS_H__


void   gimp_config_connect      (GObject     *a,
                                 GObject     *b,
                                 const gchar *property_name);
void   gimp_config_connect_full (GObject     *a,
                                 GObject     *b,
                                 const gchar *property_name_a,
                                 const gchar *property_name_b);
void   gimp_config_disconnect   (GObject     *a,
                                 GObject     *b);


#endif  /* __APP_GIMP_CONFIG_UTILS_H__ */
