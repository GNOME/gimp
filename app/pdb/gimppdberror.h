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

#ifndef __LIGMA_PDB_ERROR_H__
#define __LIGMA_PDB_ERROR_H__


typedef enum
{
  LIGMA_PDB_ERROR_FAILED,  /* generic error condition */
  LIGMA_PDB_ERROR_CANCELLED,
  LIGMA_PDB_ERROR_PROCEDURE_NOT_FOUND,
  LIGMA_PDB_ERROR_INVALID_ARGUMENT,
  LIGMA_PDB_ERROR_INVALID_RETURN_VALUE,
  LIGMA_PDB_ERROR_INTERNAL_ERROR
} LigmaPdbErrorCode;


#define LIGMA_PDB_ERROR (ligma_pdb_error_quark ())

GQuark  ligma_pdb_error_quark (void) G_GNUC_CONST;


#endif /* __LIGMA_PDB_ERROR_H__ */
