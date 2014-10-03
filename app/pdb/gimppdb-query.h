/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2003 Spencer Kimball and Peter Mattis
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_PDB_QUERY_H__
#define __GIMP_PDB_QUERY_H__


gboolean   gimp_pdb_dump      (GimpPDB          *pdb,
                               GFile            *file,
                               GError          **error);
gboolean   gimp_pdb_query     (GimpPDB          *pdb,
                               const gchar      *name,
                               const gchar      *blurb,
                               const gchar      *help,
                               const gchar      *author,
                               const gchar      *copyright,
                               const gchar      *date,
                               const gchar      *proc_type,
                               gint             *num_procs,
                               gchar          ***procs,
                               GError          **error);
gboolean   gimp_pdb_proc_info (GimpPDB          *pdb,
                               const gchar      *proc_name,
                               gchar           **blurb,
                               gchar           **help,
                               gchar           **author,
                               gchar           **copyright,
                               gchar           **date,
                               GimpPDBProcType  *proc_type,
                               gint             *num_args,
                               gint             *num_values,
                               GError          **error);


#endif /* __GIMP_PDB_QUERY_H__ */
