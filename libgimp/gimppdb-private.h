/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapdb-private.h
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

#ifndef __LIGMA_PDB_PRIVATE_H__
#define __LIGMA_PDB_PRIVATE_H__

G_BEGIN_DECLS


typedef enum
{
  LIGMA_PDB_ERROR_FAILED,  /* generic error condition */
  LIGMA_PDB_ERROR_CANCELLED,
  LIGMA_PDB_ERROR_PDB_NOT_FOUND,
  LIGMA_PDB_ERROR_INVALID_ARGUMENT,
  LIGMA_PDB_ERROR_INVALID_RETURN_VALUE,
  LIGMA_PDB_ERROR_INTERNAL_ERROR
} LigmaPdbErrorCode;


#define LIGMA_PDB_ERROR (_ligma_pdb_error_quark ())

GQuark _ligma_pdb_error_quark (void) G_GNUC_CONST;


LigmaPDB    * _ligma_pdb_new         (LigmaPlugIn *plug_in);

LigmaPlugIn * _ligma_pdb_get_plug_in (LigmaPDB    *pdb);


G_END_DECLS

#endif  /*  __LIGMA_PDB_PRIVATE_H__  */
