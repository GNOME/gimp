/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Recent File Storage,
 * see http://freedesktop.org/Standards/recent-file-spec/
 *
 * This code is taken from libegg and has been adapted to the GIMP needs.
 * The original author is James Willcox <jwillcox@cs.indiana.edu>,
 * responsible for bugs in this version is Sven Neumann <sven@gimp.org>.
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

#ifndef __GIMP_RECENT_LIST_H__
#define __GIMP_RECENT_LIST_H__


gboolean  gimp_recent_list_add_uri (const gchar *uri,
                                    const gchar *mime_type);


#endif /* __GIMP_RECENT_LIST_H__ */
