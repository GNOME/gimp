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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __APP_GIMP_UTILS_H__
#define __APP_GIMP_UTILS_H__


#define GIMP_TIMER_START() \
  { GTimer *_timer = g_timer_new ();

#define GIMP_TIMER_END(message) \
  g_printerr ("%s: " message " took %0.2f seconds\n", \
              G_STRFUNC, g_timer_elapsed (_timer, NULL)); \
  g_timer_destroy (_timer); }


#define MIN4(a,b,c,d) MIN (MIN ((a), (b)), MIN ((c), (d)))
#define MAX4(a,b,c,d) MAX (MAX ((a), (b)), MAX ((c), (d)))


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

gint64       gimp_string_get_memsize               (const gchar     *string);
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

gchar      * gimp_markup_extract_text              (const gchar     *markup);

const gchar* gimp_enum_get_value_name              (GType            enum_type,
                                                    gint             value);

/* Common values for the n_snap_lines parameter of
 * gimp_constrain_line.
 */
#define GIMP_CONSTRAIN_LINE_90_DEGREES 2
#define GIMP_CONSTRAIN_LINE_45_DEGREES 4
#define GIMP_CONSTRAIN_LINE_15_DEGREES 12

void         gimp_constrain_line                   (gdouble          start_x,
                                                    gdouble          start_y,
                                                    gdouble         *end_x,
                                                    gdouble         *end_y,
                                                    gint             n_snap_lines);


#endif /* __APP_GIMP_UTILS_H__ */
