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

GKeyFile * print_utils_key_file_load_from_rcfile   (const gchar *basename);
GKeyFile * print_utils_key_file_load_from_parasite (gint32       image_ID,
                                                    const gchar *parasite_name);

void       print_utils_key_file_save_as_rcfile     (GKeyFile    *key_file,
                                                    const gchar *basename);
void       print_utils_key_file_save_as_parasite   (GKeyFile    *key_file,
                                                    gint32       image_ID,
                                                    const gchar *parasite_name);
