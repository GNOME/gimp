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

#ifndef __PDB_TYPES_H__
#define __PDB_TYPES_H__


#include "core/core-types.h"


typedef struct _GimpPDB                GimpPDB;
typedef struct _GimpProcedure          GimpProcedure;
typedef struct _GimpPlugInProcedure    GimpPlugInProcedure;
typedef struct _GimpTemporaryProcedure GimpTemporaryProcedure;


typedef enum
{
  GIMP_PDB_COMPAT_OFF,
  GIMP_PDB_COMPAT_ON,
  GIMP_PDB_COMPAT_WARN
} GimpPDBCompatMode;


typedef enum
{
  GIMP_PDB_ITEM_CONTENT  = 1 << 0,
  GIMP_PDB_ITEM_POSITION = 1 << 1
} GimpPDBItemModify;


typedef enum
{
  GIMP_PDB_DATA_ACCESS_READ   = 0,
  GIMP_PDB_DATA_ACCESS_WRITE  = 1 << 0,
  GIMP_PDB_DATA_ACCESS_RENAME = 1 << 1
} GimpPDBDataAccess;


#endif /* __PDB_TYPES_H__ */
