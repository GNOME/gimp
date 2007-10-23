/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppluginmanager-query.h
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

#ifndef __GIMP_PLUG_IN_MANAGER_QUERY_H__
#define __GIMP_PLUG_IN_MANAGER_QUERY_H__


gint   gimp_plug_in_manager_query (GimpPlugInManager   *manager,
                                   const gchar         *search_str,
                                   gchar             ***menu_strs,
                                   gchar             ***accel_strs,
                                   gchar             ***prog_strs,
                                   gchar             ***types_strs,
                                   gchar             ***realname_strs,
                                   gint32             **time_ints);


#endif /* __GIMP_PLUG_IN_MANAGER_QUERY_H__ */
