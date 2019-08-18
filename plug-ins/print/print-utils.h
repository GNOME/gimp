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

GKeyFile * print_utils_key_file_load_from_rcfile   (const gchar *basename);
GKeyFile * print_utils_key_file_load_from_parasite (GimpImage   *image,
                                                    const gchar *parasite_name);

void       print_utils_key_file_save_as_rcfile     (GKeyFile    *key_file,
                                                    const gchar *basename);
void       print_utils_key_file_save_as_parasite   (GKeyFile    *key_file,
                                                    GimpImage   *image,
                                                    const gchar *parasite_name);
