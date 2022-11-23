/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_PDB_H__
#define __LIGMA_PDB_H__


#include "core/ligmaobject.h"


#define LIGMA_TYPE_PDB            (ligma_pdb_get_type ())
#define LIGMA_PDB(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PDB, LigmaPDB))
#define LIGMA_PDB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PDB, LigmaPDBClass))
#define LIGMA_IS_PDB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PDB))
#define LIGMA_IS_PDB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PDB))
#define LIGMA_PDB_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PDB, LigmaPDBClass))


typedef struct _LigmaPDBClass LigmaPDBClass;

struct _LigmaPDB
{
  LigmaObject  parent_instance;

  Ligma       *ligma;

  GHashTable *procedures;
  GHashTable *compat_proc_names;
};

struct _LigmaPDBClass
{
  LigmaObjectClass parent_class;

  void (* register_procedure)   (LigmaPDB       *pdb,
                                 LigmaProcedure *procedure);
  void (* unregister_procedure) (LigmaPDB       *pdb,
                                 LigmaProcedure *procedure);
};


GType            ligma_pdb_get_type                       (void) G_GNUC_CONST;

LigmaPDB        * ligma_pdb_new                            (Ligma           *ligma);

void             ligma_pdb_register_procedure             (LigmaPDB        *pdb,
                                                          LigmaProcedure  *procedure);
void             ligma_pdb_unregister_procedure           (LigmaPDB        *pdb,
                                                          LigmaProcedure  *procedure);

LigmaProcedure  * ligma_pdb_lookup_procedure               (LigmaPDB        *pdb,
                                                          const gchar    *name);

void             ligma_pdb_register_compat_proc_name      (LigmaPDB        *pdb,
                                                          const gchar    *old_name,
                                                          const gchar    *new_name);
const gchar    * ligma_pdb_lookup_compat_proc_name        (LigmaPDB        *pdb,
                                                          const gchar    *old_name);

LigmaValueArray * ligma_pdb_execute_procedure_by_name_args (LigmaPDB        *pdb,
                                                          LigmaContext    *context,
                                                          LigmaProgress   *progress,
                                                          GError        **error,
                                                          const gchar    *name,
                                                          LigmaValueArray *args);
LigmaValueArray * ligma_pdb_execute_procedure_by_name      (LigmaPDB        *pdb,
                                                          LigmaContext    *context,
                                                          LigmaProgress   *progress,
                                                          GError        **error,
                                                          const gchar    *name,
                                                          ...);

GList          * ligma_pdb_get_deprecated_procedures      (LigmaPDB        *pdb);


#endif  /*  __LIGMA_PDB_H__  */
