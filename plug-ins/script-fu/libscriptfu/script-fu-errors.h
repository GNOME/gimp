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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __SCRIPT_FU_ERRORS_H__
#define __SCRIPT_FU_ERRORS_H__

pointer registration_error (scheme        *sc,
                            const gchar   *error_message);

pointer script_error (scheme        *sc,
                      const gchar   *error_message,
                      const pointer  a);

pointer script_type_error (scheme       *sc,
                           const gchar  *expected_type,
                           const guint   arg_index,
                           const gchar  *proc_name);

pointer script_type_error_in_container (scheme       *sc,
                                        const gchar  *expected_type,
                                        const guint   arg_index,
                                        const guint   element_index,
                                        const gchar  *proc_name,
                                        const pointer a);

pointer script_length_error_in_vector (scheme       *sc,
                                       const guint   arg_index,
                                       const gchar  *proc_name,
                                       const guint   expected_length,
                                       const pointer vector);

pointer script_int_range_error        (scheme       *sc,
                                       const guint   arg_index,
                                       const gchar  *proc_name,
                                       const gint    expected_min,
                                       const gint    expected_max,
                                       const gint    value);

pointer script_float_range_error      (scheme       *sc,
                                       const guint   arg_index,
                                       const gchar  *proc_name,
                                       const gdouble expected_min,
                                       const gdouble expected_max,
                                       const gdouble value);

pointer implementation_error (scheme        *sc,
                              const gchar   *error_message,
                              const pointer  a);


void debug_vector (scheme        *sc,
                   const pointer  vector,
                   const gchar   *format);

void debug_list (scheme       *sc,
                 pointer       list,
                 const char   *format,
                 const guint   num_elements);

void debug_in_arg(scheme           *sc,
                  const pointer     a,
                  const guint       arg_index,
                  const gchar      *type_name );

void debug_gvalue(const GValue     *value);

#endif /*  __SCRIPT_FU_ERRORS_H__  */
