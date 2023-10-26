/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppdb-private.h
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

#ifndef __GIMP_PDB_PRIVATE_H__
#define __GIMP_PDB_PRIVATE_H__

G_BEGIN_DECLS


typedef enum
{
  GIMP_PDB_ERROR_FAILED,  /* generic error condition */
  GIMP_PDB_ERROR_CANCELLED,
  GIMP_PDB_ERROR_PDB_NOT_FOUND,
  GIMP_PDB_ERROR_INVALID_ARGUMENT,
  GIMP_PDB_ERROR_INVALID_RETURN_VALUE,
  GIMP_PDB_ERROR_INTERNAL_ERROR
} GimpPdbErrorCode;


#define GIMP_PDB_ERROR (_gimp_pdb_error_quark ())

GQuark _gimp_pdb_error_quark (void) G_GNUC_CONST;


GimpPDB    * _gimp_pdb_new         (GimpPlugIn   *plug_in);

GimpPlugIn * _gimp_pdb_get_plug_in (GimpPDB      *pdb);

gboolean     gimp_pdb_get_data     (const gchar  *identifier,
                                    GBytes      **data);
gboolean     gimp_pdb_set_data     (const gchar  *identifier,
                                    GBytes       *data);

G_END_DECLS

#endif  /*  __GIMP_PDB_PRIVATE_H__  */
