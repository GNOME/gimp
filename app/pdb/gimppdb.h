/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_PDB_H__
#define __GIMP_PDB_H__


#include "core/gimpobject.h"


#define GIMP_TYPE_PDB            (gimp_pdb_get_type ())
#define GIMP_PDB(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PDB, GimpPDB))
#define GIMP_PDB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PDB, GimpPDBClass))
#define GIMP_IS_PDB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PDB))
#define GIMP_IS_PDB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PDB))
#define GIMP_PDB_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PDB, GimpPDBClass))


typedef struct _GimpPDBClass GimpPDBClass;

struct _GimpPDB
{
  GimpObject  parent_instance;

  Gimp       *gimp;

  GHashTable *procedures;
  GHashTable *compat_proc_names;
};

struct _GimpPDBClass
{
  GimpObjectClass parent_class;

  void (* register_procedure)   (GimpPDB       *pdb,
                                 GimpProcedure *procedure);
  void (* unregister_procedure) (GimpPDB       *pdb,
                                 GimpProcedure *procedure);
};


GType           gimp_pdb_get_type                       (void) G_GNUC_CONST;

GimpPDB       * gimp_pdb_new                            (Gimp          *gimp);

void            gimp_pdb_register_procedure             (GimpPDB       *pdb,
                                                         GimpProcedure *procedure);
void            gimp_pdb_unregister_procedure           (GimpPDB       *pdb,
                                                         GimpProcedure *procedure);

GimpProcedure * gimp_pdb_lookup_procedure               (GimpPDB       *pdb,
                                                         const gchar   *name);

void            gimp_pdb_register_compat_proc_name      (GimpPDB       *pdb,
                                                         const gchar   *old_name,
                                                         const gchar   *new_name);
const gchar   * gimp_pdb_lookup_compat_proc_name        (GimpPDB       *pdb,
                                                         const gchar   *old_name);

GValueArray   * gimp_pdb_execute_procedure_by_name_args (GimpPDB       *pdb,
                                                         GimpContext   *context,
                                                         GimpProgress  *progress,
                                                         const gchar   *name,
                                                         GValueArray   *args);
GValueArray   * gimp_pdb_execute_procedure_by_name      (GimpPDB       *pdb,
                                                         GimpContext   *context,
                                                         GimpProgress  *progress,
                                                         const gchar   *name,
                                                         ...);


#endif  /*  __GIMP_PDB_H__  */
