/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-procedure.h
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

#ifndef __FILE_PROCEDURE_H__
#define __FILE_PROCEDURE_H__


typedef enum
{
  FILE_PROCEDURE_GROUP_ANY,
  FILE_PROCEDURE_GROUP_OPEN,
  FILE_PROCEDURE_GROUP_SAVE,
  FILE_PROCEDURE_GROUP_EXPORT
} FileProcedureGroup;


GimpPlugInProcedure *file_procedure_find              (GSList               *procs,
                                                       const gchar          *filename,
                                                       GError              **error);
GimpPlugInProcedure *file_procedure_find_by_prefix    (GSList               *procs,
                                                       const gchar          *uri);
GimpPlugInProcedure *file_procedure_find_by_extension (GSList               *procs,
                                                       const gchar          *uri);
gboolean             file_procedure_in_group          (GimpPlugInProcedure  *file_proc,
                                                       FileProcedureGroup    group);


#endif /* __FILE_PROCEDURE_H__ */
