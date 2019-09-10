/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppluginmanager-file.c
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

#include "config.h"

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "plug-in-types.h"

#include "core/gimp.h"

#include "gimpplugin.h"
#include "gimpplugindef.h"
#include "gimppluginmanager.h"
#include "gimppluginmanager-file.h"
#include "gimppluginmanager-file-procedure.h"
#include "gimppluginprocedure.h"


/*  public functions  */

GSList *
gimp_plug_in_manager_get_file_procedures (GimpPlugInManager      *manager,
                                          GimpFileProcedureGroup  group)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), NULL);

  switch (group)
    {
    case GIMP_FILE_PROCEDURE_GROUP_NONE:
      return NULL;

    case GIMP_FILE_PROCEDURE_GROUP_OPEN:
      return manager->display_load_procs;

    case GIMP_FILE_PROCEDURE_GROUP_SAVE:
      return manager->display_save_procs;

    case GIMP_FILE_PROCEDURE_GROUP_EXPORT:
      return manager->display_export_procs;

    default:
      g_return_val_if_reached (NULL);
    }
}

GimpPlugInProcedure *
gimp_plug_in_manager_file_procedure_find (GimpPlugInManager      *manager,
                                          GimpFileProcedureGroup  group,
                                          GFile                  *file,
                                          GError                **error)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  switch (group)
    {
    case GIMP_FILE_PROCEDURE_GROUP_OPEN:
      return file_procedure_find (manager->load_procs, file, error);

    case GIMP_FILE_PROCEDURE_GROUP_SAVE:
      return file_procedure_find (manager->save_procs, file, error);

    case GIMP_FILE_PROCEDURE_GROUP_EXPORT:
      return file_procedure_find (manager->export_procs, file, error);

    default:
      g_return_val_if_reached (NULL);
    }
}

GimpPlugInProcedure *
gimp_plug_in_manager_file_procedure_find_by_prefix (GimpPlugInManager      *manager,
                                                    GimpFileProcedureGroup  group,
                                                    GFile                  *file)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);

  switch (group)
    {
    case GIMP_FILE_PROCEDURE_GROUP_OPEN:
      return file_procedure_find_by_prefix (manager->load_procs, file);

    case GIMP_FILE_PROCEDURE_GROUP_SAVE:
      return file_procedure_find_by_prefix (manager->save_procs, file);

    case GIMP_FILE_PROCEDURE_GROUP_EXPORT:
      return file_procedure_find_by_prefix (manager->export_procs, file);

    default:
      g_return_val_if_reached (NULL);
    }
}

GimpPlugInProcedure *
gimp_plug_in_manager_file_procedure_find_by_extension (GimpPlugInManager      *manager,
                                                       GimpFileProcedureGroup  group,
                                                       GFile                  *file)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);

  switch (group)
    {
    case GIMP_FILE_PROCEDURE_GROUP_OPEN:
      return file_procedure_find_by_extension (manager->load_procs, file);

    case GIMP_FILE_PROCEDURE_GROUP_SAVE:
      return file_procedure_find_by_extension (manager->save_procs, file);

    case GIMP_FILE_PROCEDURE_GROUP_EXPORT:
      return file_procedure_find_by_extension (manager->export_procs, file);

    default:
      g_return_val_if_reached (NULL);
    }
}

GimpPlugInProcedure *
gimp_plug_in_manager_file_procedure_find_by_mime_type (GimpPlugInManager      *manager,
                                                       GimpFileProcedureGroup  group,
                                                       const gchar            *mime_type)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), NULL);
  g_return_val_if_fail (mime_type != NULL, NULL);

  switch (group)
    {
    case GIMP_FILE_PROCEDURE_GROUP_OPEN:
      return file_procedure_find_by_mime_type (manager->load_procs, mime_type);

    case GIMP_FILE_PROCEDURE_GROUP_SAVE:
      return file_procedure_find_by_mime_type (manager->save_procs, mime_type);

    case GIMP_FILE_PROCEDURE_GROUP_EXPORT:
      return file_procedure_find_by_mime_type (manager->export_procs, mime_type);

    default:
      g_return_val_if_reached (NULL);
    }
}
