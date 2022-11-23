/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapdbprocedure.h
 * Copyright (C) 2019 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_PDB_PROCEDURE_H__
#define __LIGMA_PDB_PROCEDURE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define LIGMA_TYPE_PDB_PROCEDURE            (_ligma_pdb_procedure_get_type ())
#define LIGMA_PDB_PROCEDURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PDB_PROCEDURE, LigmaPDBProcedure))
#define LIGMA_PDB_PROCEDURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PDB_PROCEDURE, LigmaPDBProcedureClass))
#define LIGMA_IS_PDB_PROCEDURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PDB_PROCEDURE))
#define LIGMA_IS_PDB_PROCEDURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PDB_PROCEDURE))
#define LIGMA_PDB_PROCEDURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PDB_PROCEDURE, LigmaPDBProcedureClass))


typedef struct _LigmaPDBProcedure        LigmaPDBProcedure;
typedef struct _LigmaPDBProcedureClass   LigmaPDBProcedureClass;
typedef struct _LigmaPDBProcedurePrivate LigmaPDBProcedurePrivate;

struct _LigmaPDBProcedure
{
  LigmaProcedure            parent_instance;

  LigmaPDBProcedurePrivate *priv;
};

struct _LigmaPDBProcedureClass
{
  LigmaProcedureClass parent_class;
};


GType           _ligma_pdb_procedure_get_type (void) G_GNUC_CONST;

LigmaProcedure * _ligma_pdb_procedure_new      (LigmaPDB     *pdb,
                                              const gchar *name);


G_END_DECLS

#endif  /*  __LIGMA_PDB_PROCEDURE_H__  */
