/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_UTILS_H__
#define __GIMP_UTILS_H__


gchar       * gimp_utf8_strtrim      (const gchar *str,
                                      gint         max_chars);
gchar       * gimp_any_to_utf8       (const gchar *str,
                                      gssize       len,
                                      const gchar *warning_format,
                                      ...) G_GNUC_PRINTF (3, 4);
const gchar * gimp_filename_to_utf8  (const gchar *filename);

gchar       * gimp_memsize_to_string (guint64      memsize);

gchar       * gimp_strip_uline       (const gchar *str);
gchar       * gimp_escape_uline      (const gchar *str);


#endif  /* __GIMP_UTILS_H__ */
