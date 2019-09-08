/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpplugin-proc.h
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

#ifndef __GIMP_PLUG_IN_PROC_H__
#define __GIMP_PLUG_IN_PROC_H__


gboolean   gimp_plug_in_add_proc_menu_path (GimpPlugIn             *plug_in,
                                            const gchar            *proc_name,
                                            const gchar            *menu_path);
gboolean   gimp_plug_in_set_proc_icon      (GimpPlugIn             *plug_in,
                                            const gchar            *proc_name,
                                            GimpIconType            type,
                                            const guint8           *data,
                                            gint                    data_length);


#endif /* __GIMP_PLUG_IN_PROC_H__ */
