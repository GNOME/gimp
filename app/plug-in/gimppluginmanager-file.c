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

#include "plug-in-types.h"

#include "core/gimp.h"
#include "core/gimpparamspecs.h"

#include "gimpplugin.h"
#include "gimpplugindef.h"
#include "gimppluginmanager.h"
#include "gimppluginmanager-file.h"
#include "gimppluginmanager-file-procedure.h"
#include "gimppluginprocedure.h"


static gboolean   file_procedure_in_group (GimpPlugInProcedure    *file_proc,
                                           GimpFileProcedureGroup  group);


/*  public functions  */

gboolean
gimp_plug_in_manager_register_load_handler (GimpPlugInManager *manager,
                                            const gchar       *name,
                                            const gchar       *extensions,
                                            const gchar       *prefixes,
                                            const gchar       *magics)
{
  GimpPlugInProcedure *file_proc;
  GimpProcedure       *procedure;
  GSList              *list;

  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), FALSE);
  g_return_val_if_fail (name != NULL, FALSE);

  if (manager->current_plug_in && manager->current_plug_in->plug_in_def)
    list = manager->current_plug_in->plug_in_def->procedures;
  else
    list = manager->plug_in_procedures;

  file_proc = gimp_plug_in_procedure_find (list, name);

  if (! file_proc)
    {
      gimp_message (manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "attempt to register nonexistent load handler \"%s\"",
                    name);
      return FALSE;
    }

  procedure = GIMP_PROCEDURE (file_proc);

  if ((procedure->num_args   < 3)                        ||
      (procedure->num_values < 1)                        ||
      ! GIMP_IS_PARAM_SPEC_INT32    (procedure->args[0]) ||
      ! G_IS_PARAM_SPEC_STRING      (procedure->args[1]) ||
      ! G_IS_PARAM_SPEC_STRING      (procedure->args[2]) ||
      ! GIMP_IS_PARAM_SPEC_IMAGE_ID (procedure->values[0]))
    {
      gimp_message (manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "load handler \"%s\" does not take the standard "
                    "load handler args", name);
      return FALSE;
    }

  gimp_plug_in_procedure_set_file_proc (file_proc,
                                        extensions, prefixes, magics);

  if (! g_slist_find (manager->load_procs, file_proc))
    manager->load_procs = g_slist_prepend (manager->load_procs, file_proc);

  return TRUE;
}

gboolean
gimp_plug_in_manager_register_save_handler (GimpPlugInManager *manager,
                                            const gchar       *name,
                                            const gchar       *extensions,
                                            const gchar       *prefixes)
{
  GimpPlugInProcedure *file_proc;
  GimpProcedure       *procedure;
  GSList              *list;

  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), FALSE);
  g_return_val_if_fail (name != NULL, FALSE);

  if (manager->current_plug_in && manager->current_plug_in->plug_in_def)
    list = manager->current_plug_in->plug_in_def->procedures;
  else
    list = manager->plug_in_procedures;

  file_proc = gimp_plug_in_procedure_find (list, name);

  if (! file_proc)
    {
      gimp_message (manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "attempt to register nonexistent save handler \"%s\"",
                    name);
      return FALSE;
    }

  procedure = GIMP_PROCEDURE (file_proc);

  if ((procedure->num_args < 5)                             ||
      ! GIMP_IS_PARAM_SPEC_INT32       (procedure->args[0]) ||
      ! GIMP_IS_PARAM_SPEC_IMAGE_ID    (procedure->args[1]) ||
      ! GIMP_IS_PARAM_SPEC_DRAWABLE_ID (procedure->args[2]) ||
      ! G_IS_PARAM_SPEC_STRING         (procedure->args[3]) ||
      ! G_IS_PARAM_SPEC_STRING         (procedure->args[4]))
    {
      gimp_message (manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "save handler \"%s\" does not take the standard "
                    "save handler args", name);
      return FALSE;
    }

  gimp_plug_in_procedure_set_file_proc (file_proc,
                                        extensions, prefixes, NULL);

  if (file_procedure_in_group (file_proc, GIMP_FILE_PROCEDURE_GROUP_SAVE))
    {
      if (! g_slist_find (manager->save_procs, file_proc))
        manager->save_procs = g_slist_prepend (manager->save_procs, file_proc);
    }

  if (file_procedure_in_group (file_proc, GIMP_FILE_PROCEDURE_GROUP_EXPORT))
    {
      if (! g_slist_find (manager->export_procs, file_proc))
        manager->export_procs = g_slist_prepend (manager->export_procs, file_proc);
    }

  return TRUE;
}

gboolean
gimp_plug_in_manager_register_priority (GimpPlugInManager *manager,
                                        const gchar       *name,
                                        gint               priority)
{
  GimpPlugInProcedure *file_proc;
  GSList              *list;

  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), FALSE);
  g_return_val_if_fail (name != NULL, FALSE);

  if (manager->current_plug_in && manager->current_plug_in->plug_in_def)
    list = manager->current_plug_in->plug_in_def->procedures;
  else
    list = manager->plug_in_procedures;

  file_proc = gimp_plug_in_procedure_find (list, name);

  if (! file_proc)
    return FALSE;

  gimp_plug_in_procedure_set_priority (file_proc, priority);

  return TRUE;
}

gboolean
gimp_plug_in_manager_register_mime_types (GimpPlugInManager *manager,
                                          const gchar       *name,
                                          const gchar       *mime_types)
{
  GimpPlugInProcedure *file_proc;
  GSList              *list;

  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), FALSE);
  g_return_val_if_fail (name != NULL, FALSE);
  g_return_val_if_fail (mime_types != NULL, FALSE);

  if (manager->current_plug_in && manager->current_plug_in->plug_in_def)
    list = manager->current_plug_in->plug_in_def->procedures;
  else
    list = manager->plug_in_procedures;

  file_proc = gimp_plug_in_procedure_find (list, name);

  if (! file_proc)
    return FALSE;

  gimp_plug_in_procedure_set_mime_types (file_proc, mime_types);

  return TRUE;
}

gboolean
gimp_plug_in_manager_register_handles_uri (GimpPlugInManager *manager,
                                           const gchar       *name)
{
  GimpPlugInProcedure *file_proc;
  GSList              *list;

  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), FALSE);
  g_return_val_if_fail (name != NULL, FALSE);

  if (manager->current_plug_in && manager->current_plug_in->plug_in_def)
    list = manager->current_plug_in->plug_in_def->procedures;
  else
    list = manager->plug_in_procedures;

  file_proc = gimp_plug_in_procedure_find (list, name);

  if (! file_proc)
    return FALSE;

  gimp_plug_in_procedure_set_handles_uri (file_proc);

  return TRUE;
}

gboolean
gimp_plug_in_manager_register_handles_raw (GimpPlugInManager *manager,
                                           const gchar       *name)
{
  GimpPlugInProcedure *file_proc;
  GSList              *list;

  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), FALSE);
  g_return_val_if_fail (name != NULL, FALSE);

  if (manager->current_plug_in && manager->current_plug_in->plug_in_def)
    list = manager->current_plug_in->plug_in_def->procedures;
  else
    list = manager->plug_in_procedures;

  file_proc = gimp_plug_in_procedure_find (list, name);

  if (! file_proc)
    return FALSE;

  gimp_plug_in_procedure_set_handles_raw (file_proc);

  return TRUE;
}

gboolean
gimp_plug_in_manager_register_thumb_loader (GimpPlugInManager *manager,
                                            const gchar       *load_proc,
                                            const gchar       *thumb_proc)
{
  GimpPlugInProcedure *file_proc;
  GSList              *list;

  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), FALSE);
  g_return_val_if_fail (load_proc, FALSE);
  g_return_val_if_fail (thumb_proc, FALSE);

  if (manager->current_plug_in && manager->current_plug_in->plug_in_def)
    list = manager->current_plug_in->plug_in_def->procedures;
  else
    list = manager->plug_in_procedures;

  file_proc = gimp_plug_in_procedure_find (list, load_proc);

  if (! file_proc)
    return FALSE;

  gimp_plug_in_procedure_set_thumb_loader (file_proc, thumb_proc);

  return TRUE;
}

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


/*  private functions  */

static gboolean
file_procedure_in_group (GimpPlugInProcedure    *file_proc,
                         GimpFileProcedureGroup  group)
{
  const gchar *name        = gimp_object_get_name (file_proc);
  gboolean     is_xcf_save = FALSE;
  gboolean     is_filter   = FALSE;

  is_xcf_save = (strcmp (name, "gimp-xcf-save") == 0);

  is_filter   = (strcmp (name, "file-gz-save")  == 0 ||
                 strcmp (name, "file-bz2-save") == 0 ||
                 strcmp (name, "file-xz-save")  == 0);

  switch (group)
    {
    case GIMP_FILE_PROCEDURE_GROUP_NONE:
      return FALSE;

    case GIMP_FILE_PROCEDURE_GROUP_SAVE:
      /* Only .xcf shall pass */
      return is_xcf_save || is_filter;

    case GIMP_FILE_PROCEDURE_GROUP_EXPORT:
      /* Anything but .xcf shall pass */
      return ! is_xcf_save;

    case GIMP_FILE_PROCEDURE_GROUP_OPEN:
      /* No filter applied for Open */
      return TRUE;

    default:
    case GIMP_FILE_PROCEDURE_GROUP_ANY:
      return TRUE;
    }
}
