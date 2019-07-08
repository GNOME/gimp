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

#ifndef __GIMP_PDB_COMPAT_H__
#define __GIMP_PDB_COMPAT_H__


GParamSpec     * gimp_pdb_compat_param_spec          (Gimp           *gimp,
                                                      GimpPDBArgType  arg_type,
                                                      const gchar    *name,
                                                      const gchar    *desc);

GType            gimp_pdb_compat_arg_type_to_gtype   (GimpPDBArgType  type);
GimpPDBArgType   gimp_pdb_compat_arg_type_from_gtype (GType           type);
gchar          * gimp_pdb_compat_arg_type_to_string  (GimpPDBArgType  type);

void             gimp_pdb_compat_procs_register (GimpPDB           *pdb,
                                                 GimpPDBCompatMode  compat_mode);


#endif  /*  __GIMP_PDB_COMPAT_H__  */
