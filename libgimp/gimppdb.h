/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppdb.h
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

#ifndef __GIMP_PDB_H__
#define __GIMP_PDB_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_PDB (gimp_pdb_get_type ())
G_DECLARE_FINAL_TYPE (GimpPDB, gimp_pdb, GIMP, PDB, GObject)


gboolean             gimp_pdb_procedure_exists     (GimpPDB              *pdb,
                                                    const gchar          *procedure_name);

GimpProcedure      * gimp_pdb_lookup_procedure     (GimpPDB              *pdb,
                                                    const gchar          *procedure_name);

gchar              * gimp_pdb_temp_procedure_name  (GimpPDB              *pdb);

gboolean             gimp_pdb_dump_to_file         (GimpPDB              *pdb,
                                                    GFile                *file);
gchar             ** gimp_pdb_query_procedures     (GimpPDB              *pdb,
                                                    const gchar          *name,
                                                    const gchar          *blurb,
                                                    const gchar          *help,
                                                    const gchar          *help_id,
                                                    const gchar          *authors,
                                                    const gchar          *copyright,
                                                    const gchar          *date,
                                                    const gchar          *proc_type);

const gchar        * gimp_pdb_get_last_error       (GimpPDB              *pdb);
GimpPDBStatusType    gimp_pdb_get_last_status      (GimpPDB              *pdb);


/* Internal use */

G_GNUC_INTERNAL GimpValueArray * _gimp_pdb_run_procedure_array  (GimpPDB              *pdb,
                                                                 const gchar          *procedure_name,
                                                                 const GimpValueArray *arguments);


G_END_DECLS

#endif  /*  __GIMP_PDB_H__  */
