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
  g_printerr ("%s: %s took %0.4f seconds\n", \
              G_STRFUNC, message, g_timer_elapsed (_timer, NULL)); \
  g_timer_destroy (_timer); }


#define MIN4(a,b,c,d) MIN (MIN ((a), (b)), MIN ((c), (d)))
#define MAX4(a,b,c,d) MAX (MAX ((a), (b)), MAX ((c), (d)))


gint         gimp_get_pid                          (void);
guint64      gimp_get_physical_memory_size         (void);
gchar      * gimp_get_backtrace                    (void);
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

gchar      * gimp_markup_extract_text              (const gchar     *markup);

const gchar* gimp_enum_get_value_name              (GType            enum_type,
                                                    gint             value);

gboolean     gimp_get_fill_params                  (GimpContext      *context,
                                                    GimpFillType      fill_type,
                                                    GimpRGB          *color,
                                                    GimpPattern     **pattern,
                                                    GError          **error);

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

gint         gimp_file_compare                     (GFile           *file1,
                                                    GFile           *file2);
gboolean     gimp_file_is_executable               (GFile           *file);

void         gimp_create_image_from_buffer         (Gimp            *gimp,
                                                    GeglBuffer      *buffer);


#endif /* __APP_GIMP_UTILS_H__ */
