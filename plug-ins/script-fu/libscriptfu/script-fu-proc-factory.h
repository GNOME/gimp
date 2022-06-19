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

#ifndef __SCRIPT_FU_PDB_PROC_FACTORY_H__
#define __SCRIPT_FU_PDB_PROC_FACTORY_H__

GimpProcedure *script_fu_proc_factory_make_PLUGIN (GimpPlugIn  *plug_in,
                                                   GList       *paths,
                                                   const gchar *name);
GList         *script_fu_proc_factory_list_names  (GimpPlugIn  *plug_in,
                                                   GList       *paths);

#endif /*  __SCRIPT_FU_PDB_PROC_FACTORY__  */
