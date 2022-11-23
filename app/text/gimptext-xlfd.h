/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaText
 * Copyright (C) 2002-2003  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_TEXT_XLFD_H__
#define __LIGMA_TEXT_XLFD_H__


/*  handle X Logical Font Descriptions for compat  */

gchar    * ligma_text_font_name_from_xlfd (const gchar *xlfd);
gboolean   ligma_text_font_size_from_xlfd (const gchar *xlfd,
                                          gdouble     *size,
                                          LigmaUnit    *size_unit);

void       ligma_text_set_font_from_xlfd  (LigmaText    *text,
                                          const gchar *xlfd);


#endif /* __LIGMA_TEXT_COMPAT_H__ */
