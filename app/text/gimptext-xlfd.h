/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpText
 * Copyright (C) 2002-2003  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_TEXT_XLFD_H__
#define __GIMP_TEXT_XLFD_H__


/*  handle X Logical Font Descriptions for compat  */

gchar    * gimp_text_font_name_from_xlfd (const gchar  *xlfd);
gboolean   gimp_text_font_size_from_xlfd (const gchar  *xlfd,
                                          gdouble      *size,
                                          GimpUnit    **size_unit);

void       gimp_text_set_font_from_xlfd  (GimpText     *text,
                                          const gchar  *xlfd);


#endif /* __GIMP_TEXT_COMPAT_H__ */
