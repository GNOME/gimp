/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * plug-ins-query.h
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

#ifndef __PLUG_INS_QUERY_H__
#define __PLUG_INS_QUERY_H__


gint   plug_ins_query (Gimp          *gimp,
                       const gchar   *search_str,
                       gchar       ***menu_strs,
                       gchar       ***accel_strs,
                       gchar       ***prog_strs,
                       gchar       ***types_strs,
                       gchar       ***realname_strs,
                       gint32       **time_ints);


#endif /* __PLUG_INS_QUERY_H__ */
