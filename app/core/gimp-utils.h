/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __APP_GIMP_UTILS_H__
#define __APP_GIMP_UTILS_H__


gint64       gimp_g_type_instance_get_memsize      (GTypeInstance   *instance);
gint64       gimp_g_object_get_memsize             (GObject         *object);
gint64       gimp_g_hash_table_get_memsize         (GHashTable      *hash,
                                                    gint64           data_size);
gint64       gimp_g_hash_table_get_memsize_foreach (GHashTable      *hash,
                                                    GimpMemsizeFunc  func,
                                                    gint64          *gui_size);
gint64       gimp_g_slist_get_memsize              (GSList          *slist,
                                                    gint64           data_size);
gint64       gimp_g_slist_get_memsize_foreach      (GSList          *slist,
                                                    GimpMemsizeFunc  func,
                                                    gint64          *gui_size);
gint64       gimp_g_list_get_memsize               (GList           *list,
                                                    gint64           data_size);
gint64       gimp_g_list_get_memsize_foreach       (GList            *slist,
                                                    GimpMemsizeFunc  func,
                                                    gint64          *gui_size);
gint64       gimp_g_value_get_memsize              (GValue          *value);
gint64       gimp_g_param_spec_get_memsize         (GParamSpec      *pspec);

gint64       gimp_parasite_get_memsize             (GimpParasite    *parasite,
                                                    gint64          *gui_size);

gchar      * gimp_get_default_language             (const gchar     *category);
GimpUnit     gimp_get_default_unit                 (void);

GParameter * gimp_parameters_append                (GType            object_type,
                                                    GParameter      *params,
                                                    gint            *n_params,
                                                    ...) G_GNUC_NULL_TERMINATED;
GParameter * gimp_parameters_append_valist         (GType            object_type,
                                                    GParameter      *params,
                                                    gint            *n_params,
                                                    va_list          args);
void         gimp_parameters_free                  (GParameter      *params,
                                                    gint             n_params);

void         gimp_value_array_truncate             (GValueArray     *args,
                                                    gint             n_values);

gchar      * gimp_get_temp_filename                (Gimp            *gimp,
                                                    const gchar     *extension);

GimpObject * gimp_container_get_neighbor_of_active (GimpContainer   *container,
                                                    GimpContext     *context,
                                                    GimpObject      *active);


#endif /* __APP_GIMP_UTILS_H__ */
