/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppluginmanager-history.h
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

#ifndef __GIMP_PLUG_IN_MANAGER_HISTORY_H__
#define __GIMP_PLUG_IN_MANAGER_HISTORY_H__


guint                 gimp_plug_in_manager_history_size   (GimpPlugInManager   *manager);
guint                 gimp_plug_in_manager_history_length (GimpPlugInManager   *manager);
GimpPlugInProcedure * gimp_plug_in_manager_history_nth    (GimpPlugInManager   *manager,
                                                           guint                n);
void                  gimp_plug_in_manager_history_add    (GimpPlugInManager   *manager,
                                                           GimpPlugInProcedure *procedure);
void                  gimp_plug_in_manager_history_remove (GimpPlugInManager   *manager,
                                                           GimpPlugInProcedure *procedure);
void                  gimp_plug_in_manager_history_clear  (GimpPlugInManager   *manager);


#endif /* __GIMP_PLUG_IN_MANAGER_HISTORY_H__ */
