/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppluginmanager-file.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "plug-in-types.h"

#include "core/gimp.h"
#include "core/gimpparamspecs.h"

#include "gimpplugin.h"
#include "gimpplugindef.h"
#include "gimppluginmanager.h"
#include "gimppluginmanager-file.h"
#include "gimppluginprocedure.h"


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

  if (! g_slist_find (manager->save_procs, file_proc))
    manager->save_procs = g_slist_prepend (manager->save_procs, file_proc);

  return TRUE;
}

gboolean
gimp_plug_in_manager_register_mime_type (GimpPlugInManager *manager,
                                         const gchar       *name,
                                         const gchar       *mime_type)
{
  GimpPlugInProcedure *file_proc;
  GSList              *list;

  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), FALSE);
  g_return_val_if_fail (name != NULL, FALSE);
  g_return_val_if_fail (mime_type != NULL, FALSE);

  if (manager->current_plug_in && manager->current_plug_in->plug_in_def)
    list = manager->current_plug_in->plug_in_def->procedures;
  else
    list = manager->plug_in_procedures;

  file_proc = gimp_plug_in_procedure_find (list, name);

  if (! file_proc)
    return FALSE;

  gimp_plug_in_procedure_set_mime_type (file_proc, mime_type);

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
