/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapluginmanager-file.h
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

#ifndef __LIGMA_PLUG_IN_MANAGER_FILE_H__
#define __LIGMA_PLUG_IN_MANAGER_FILE_H__


void       ligma_plug_in_manager_add_load_procedure    (LigmaPlugInManager      *manager,
                                                       LigmaPlugInProcedure    *proc);
void       ligma_plug_in_manager_add_save_procedure    (LigmaPlugInManager      *manager,
                                                       LigmaPlugInProcedure    *proc);

GSList   * ligma_plug_in_manager_get_file_procedures   (LigmaPlugInManager      *manager,
                                                       LigmaFileProcedureGroup  group);

LigmaPlugInProcedure *
ligma_plug_in_manager_file_procedure_find              (LigmaPlugInManager      *manager,
                                                       LigmaFileProcedureGroup  group,
                                                       GFile                  *file,
                                                       GError                **error);
LigmaPlugInProcedure *
ligma_plug_in_manager_file_procedure_find_by_prefix    (LigmaPlugInManager      *manager,
                                                       LigmaFileProcedureGroup  group,
                                                       GFile                  *file);
LigmaPlugInProcedure *
ligma_plug_in_manager_file_procedure_find_by_extension (LigmaPlugInManager      *manager,
                                                       LigmaFileProcedureGroup  group,
                                                       GFile                  *file);
LigmaPlugInProcedure *
ligma_plug_in_manager_file_procedure_find_by_mime_type (LigmaPlugInManager      *manager,
                                                       LigmaFileProcedureGroup  group,
                                                       const gchar            *mime_type);


#endif /* __LIGMA_PLUG_IN_MANAGER_FILE_H__ */
