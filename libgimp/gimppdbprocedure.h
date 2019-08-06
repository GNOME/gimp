/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppdbprocedure.h
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

#ifndef __GIMP_PDB_PROCEDURE_H__
#define __GIMP_PDB_PROCEDURE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_PDB_PROCEDURE            (_gimp_pdb_procedure_get_type ())
#define GIMP_PDB_PROCEDURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PDB_PROCEDURE, GimpPDBProcedure))
#define GIMP_PDB_PROCEDURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PDB_PROCEDURE, GimpPDBProcedureClass))
#define GIMP_IS_PDB_PROCEDURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PDB_PROCEDURE))
#define GIMP_IS_PDB_PROCEDURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PDB_PROCEDURE))
#define GIMP_PDB_PROCEDURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PDB_PROCEDURE, GimpPDBProcedureClass))


typedef struct _GimpPDBProcedure        GimpPDBProcedure;
typedef struct _GimpPDBProcedureClass   GimpPDBProcedureClass;
typedef struct _GimpPDBProcedurePrivate GimpPDBProcedurePrivate;

struct _GimpPDBProcedure
{
  GimpProcedure            parent_instance;

  GimpPDBProcedurePrivate *priv;
};

struct _GimpPDBProcedureClass
{
  GimpProcedureClass parent_class;
};


GType           _gimp_pdb_procedure_get_type (void) G_GNUC_CONST;

GimpProcedure * _gimp_pdb_procedure_new      (GimpPDB     *pdb,
                                              const gchar *name);


G_END_DECLS

#endif  /*  __GIMP_PDB_PROCEDURE_H__  */
