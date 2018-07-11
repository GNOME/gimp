/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 2009 Martin Nordholts <martinn@src.gnome.org>
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

#ifndef __GIMP_TEST_SESSION_UTILS_H__
#define __GIMP_TEST_SESSION_UTILS_H__


void gimp_test_session_load_and_write_session_files (const gchar *loaded_sessionrc,
                                                     const gchar *loaded_dockrc,
                                                     const gchar *expected_sessionrc,
                                                     const gchar *expected_dockrc,
                                                     gboolean     single_window_mode);


#endif /* __GIMP_TEST_SESSION_UTILS_H__ */
