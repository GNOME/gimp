/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaplugin-proc.c
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"

#include "plug-in-types.h"

#include "core/ligmaparamspecs.h"

#include "pdb/ligmapdberror.h"

#include "ligmaplugin.h"
#include "ligmaplugin-proc.h"
#include "ligmaplugindef.h"
#include "ligmapluginmanager.h"
#include "ligmapluginmanager-file.h"
#include "ligmapluginprocedure.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static LigmaPlugInProcedure * ligma_plug_in_proc_find (LigmaPlugIn  *plug_in,
                                                     const gchar *proc_name);


/*  public functions  */

gboolean
ligma_plug_in_set_proc_image_types (LigmaPlugIn   *plug_in,
                                   const gchar  *proc_name,
                                   const gchar  *image_types,
                                   GError      **error)
{
  LigmaPlugInProcedure *proc;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = ligma_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register images types "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   ligma_object_get_name (plug_in),
                   ligma_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  ligma_plug_in_procedure_set_image_types (proc, image_types);

  return TRUE;
}

gboolean
ligma_plug_in_set_proc_sensitivity_mask (LigmaPlugIn   *plug_in,
                                        const gchar  *proc_name,
                                        gint          sensitivity_mask,
                                        GError      **error)
{
  LigmaPlugInProcedure *proc;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = ligma_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register the sensitivity mask \"%x\" "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   ligma_object_get_name (plug_in),
                   ligma_file_get_utf8_name (plug_in->file),
                   sensitivity_mask, proc_name);

      return FALSE;
    }

  ligma_plug_in_procedure_set_sensitivity_mask (proc, sensitivity_mask);

  return TRUE;
}

gboolean
ligma_plug_in_set_proc_menu_label (LigmaPlugIn   *plug_in,
                                  const gchar  *proc_name,
                                  const gchar  *menu_label,
                                  GError      **error)
{
  LigmaPlugInProcedure *proc;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);
  g_return_val_if_fail (menu_label != NULL && strlen (menu_label), FALSE);

  proc = ligma_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register the menu label \"%s\" "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   ligma_object_get_name (plug_in),
                   ligma_file_get_utf8_name (plug_in->file),
                   menu_label, proc_name);

      return FALSE;
    }

  return ligma_plug_in_procedure_set_menu_label (proc, menu_label, error);
}

gboolean
ligma_plug_in_add_proc_menu_path (LigmaPlugIn   *plug_in,
                                 const gchar  *proc_name,
                                 const gchar  *menu_path,
                                 GError      **error)
{
  LigmaPlugInProcedure *proc;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);
  g_return_val_if_fail (menu_path != NULL, FALSE);

  proc = ligma_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register the menu item \"%s\" "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   ligma_object_get_name (plug_in),
                   ligma_file_get_utf8_name (plug_in->file),
                   menu_path, proc_name);

      return FALSE;
    }

  return ligma_plug_in_procedure_add_menu_path (proc, menu_path, error);
}

gboolean
ligma_plug_in_set_proc_icon (LigmaPlugIn    *plug_in,
                            const gchar   *proc_name,
                            LigmaIconType   type,
                            const guint8  *data,
                            gint           data_length,
                            GError       **error)
{
  LigmaPlugInProcedure *proc;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = ligma_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to set the icon "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   ligma_object_get_name (plug_in),
                   ligma_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  return ligma_plug_in_procedure_set_icon (proc, type, data, data_length,
                                          error);
}

gboolean
ligma_plug_in_set_proc_help (LigmaPlugIn   *plug_in,
                            const gchar  *proc_name,
                            const gchar  *blurb,
                            const gchar  *help,
                            const gchar  *help_id,
                            GError      **error)
{
  LigmaPlugInProcedure *proc;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = ligma_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register help "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   ligma_object_get_name (plug_in),
                   ligma_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  ligma_procedure_set_help (LIGMA_PROCEDURE (proc),
                           blurb, help, help_id);

  return TRUE;
}

gboolean
ligma_plug_in_set_proc_attribution (LigmaPlugIn   *plug_in,
                                   const gchar  *proc_name,
                                   const gchar  *authors,
                                   const gchar  *copyright,
                                   const gchar  *date,
                                   GError      **error)
{
  LigmaPlugInProcedure *proc;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = ligma_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register the attribution "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   ligma_object_get_name (plug_in),
                   ligma_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  ligma_procedure_set_attribution (LIGMA_PROCEDURE (proc),
                                  authors, copyright, date);

  return TRUE;
}

static inline gboolean
LIGMA_IS_PARAM_SPEC_RUN_MODE (GParamSpec *pspec)
{
  return (G_IS_PARAM_SPEC_ENUM (pspec) &&
          pspec->value_type == LIGMA_TYPE_RUN_MODE);
}

static inline gboolean
LIGMA_IS_PARAM_SPEC_FILE (GParamSpec *pspec)
{
  return (G_IS_PARAM_SPEC_OBJECT (pspec) &&
          pspec->value_type == G_TYPE_FILE);
}

gboolean
ligma_plug_in_set_file_proc_load_handler (LigmaPlugIn   *plug_in,
                                         const gchar  *proc_name,
                                         const gchar  *extensions,
                                         const gchar  *prefixes,
                                         const gchar  *magics,
                                         GError      **error)
{
  LigmaPlugInProcedure *proc;
  LigmaProcedure       *procedure;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = ligma_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register procedure \"%s\" "
                   "as load handler.\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   ligma_object_get_name (plug_in),
                   ligma_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  procedure = LIGMA_PROCEDURE (proc);

  if (((procedure->num_args   < 2)                        ||
       (procedure->num_values < 1)                        ||
       ! LIGMA_IS_PARAM_SPEC_RUN_MODE (procedure->args[0]) ||
       ! LIGMA_IS_PARAM_SPEC_FILE     (procedure->args[1]) ||
       (! proc->generic_file_proc &&
        ! LIGMA_IS_PARAM_SPEC_IMAGE (procedure->values[0]))))
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_FAILED,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register procedure \"%s\" "
                   "as load handler which does not take the standard "
                   "load procedure arguments: "
                   "(LigmaRunMode, GFile) -> (LigmaImage)",
                   ligma_object_get_name (plug_in),
                   ligma_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  ligma_plug_in_procedure_set_file_proc (proc, extensions, prefixes, magics);

  ligma_plug_in_manager_add_load_procedure (plug_in->manager, proc);

  return TRUE;
}

gboolean
ligma_plug_in_set_file_proc_save_handler (LigmaPlugIn   *plug_in,
                                         const gchar  *proc_name,
                                         const gchar  *extensions,
                                         const gchar  *prefixes,
                                         GError      **error)
{
  LigmaPlugInProcedure *proc;
  LigmaProcedure       *procedure;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = ligma_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register procedure \"%s\" "
                   "as save handler.\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   ligma_object_get_name (plug_in),
                   ligma_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  procedure = LIGMA_PROCEDURE (proc);

  if ((procedure->num_args < 5)                              ||
      ! LIGMA_IS_PARAM_SPEC_RUN_MODE     (procedure->args[0]) ||
      ! LIGMA_IS_PARAM_SPEC_IMAGE        (procedure->args[1]) ||
      ! G_IS_PARAM_SPEC_INT             (procedure->args[2]) ||
      ! LIGMA_IS_PARAM_SPEC_OBJECT_ARRAY (procedure->args[3]) ||
      ! LIGMA_IS_PARAM_SPEC_FILE         (procedure->args[4]))
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_FAILED,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register procedure \"%s\" "
                   "as save handler which does not take the standard "
                   "save procedure arguments:\n"
                   "(LigmaRunMode, LigmaImage, int [array size], LigmaDrawable Array, GFile)",
                   ligma_object_get_name (plug_in),
                   ligma_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  ligma_plug_in_procedure_set_file_proc (proc, extensions, prefixes, NULL);

  ligma_plug_in_manager_add_save_procedure (plug_in->manager, proc);

  return TRUE;
}

gboolean
ligma_plug_in_set_file_proc_priority (LigmaPlugIn   *plug_in,
                                     const gchar  *proc_name,
                                     gint          priority,
                                     GError      **error)
{
  LigmaPlugInProcedure *proc;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = ligma_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register the priority "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   ligma_object_get_name (plug_in),
                   ligma_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  ligma_plug_in_procedure_set_priority (proc, priority);

  return TRUE;
}

gboolean
ligma_plug_in_set_file_proc_mime_types (LigmaPlugIn   *plug_in,
                                       const gchar  *proc_name,
                                       const gchar  *mime_types,
                                       GError      **error)
{
  LigmaPlugInProcedure *proc;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = ligma_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register mime types "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   ligma_object_get_name (plug_in),
                   ligma_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  ligma_plug_in_procedure_set_mime_types (proc, mime_types);

  return TRUE;
}

gboolean
ligma_plug_in_set_file_proc_handles_remote (LigmaPlugIn   *plug_in,
                                           const gchar  *proc_name,
                                           GError      **error)
{
  LigmaPlugInProcedure *proc;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = ligma_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register 'handles remote' "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   ligma_object_get_name (plug_in),
                   ligma_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  ligma_plug_in_procedure_set_handles_remote (proc);

  return TRUE;
}

gboolean
ligma_plug_in_set_file_proc_handles_raw (LigmaPlugIn   *plug_in,
                                        const gchar  *proc_name,
                                        GError      **error)
{
  LigmaPlugInProcedure *proc;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = ligma_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register 'handles raw' "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   ligma_object_get_name (plug_in),
                   ligma_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  ligma_plug_in_procedure_set_handles_raw (proc);

  return TRUE;
}

gboolean
ligma_plug_in_set_file_proc_thumb_loader (LigmaPlugIn   *plug_in,
                                         const gchar  *proc_name,
                                         const gchar  *thumb_proc_name,
                                         GError      **error)
{
  LigmaPlugInProcedure *proc;
  LigmaPlugInProcedure *thumb_proc;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);
  g_return_val_if_fail (thumb_proc_name != NULL, FALSE);

  proc       = ligma_plug_in_proc_find (plug_in, proc_name);
  thumb_proc = ligma_plug_in_proc_find (plug_in, thumb_proc_name);

  if (! proc)
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register a thumbnail loader "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   ligma_object_get_name (plug_in),
                   ligma_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  if (! thumb_proc)
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register a procedure \"%s\" "
                   "as thumbnail loader.\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   ligma_object_get_name (plug_in),
                   ligma_file_get_utf8_name (plug_in->file),
                   thumb_proc_name);

      return FALSE;
    }

  ligma_plug_in_procedure_set_thumb_loader (proc, thumb_proc_name);

  return TRUE;
}

gboolean
ligma_plug_in_set_batch_interpreter (LigmaPlugIn   *plug_in,
                                    const gchar  *proc_name,
                                    const gchar  *interpreter_name,
                                    GError      **error)
{
  LigmaPlugInProcedure *proc;
  LigmaProcedure       *procedure;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = ligma_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register procedure \"%s\" "
                   "as a 'batch interpreter'.\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   ligma_object_get_name (plug_in),
                   ligma_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  procedure = LIGMA_PROCEDURE (proc);

  if (procedure->num_args < 2                            ||
      ! LIGMA_IS_PARAM_SPEC_RUN_MODE (procedure->args[0]) ||
      ! G_IS_PARAM_SPEC_STRING   (procedure->args[1]))
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_FAILED,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register procedure \"%s\" "
                   "as a batch interpreter which does not take the standard "
                   "batch interpreter procedure arguments: "
                   "(LigmaRunMode, gchar *) -> ()",
                   ligma_object_get_name (plug_in),
                   ligma_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  ligma_plug_in_procedure_set_batch_interpreter (proc, interpreter_name);
  ligma_plug_in_manager_add_batch_procedure (plug_in->manager, proc);

  return TRUE;
}


/*  private functions  */

static LigmaPlugInProcedure *
ligma_plug_in_proc_find (LigmaPlugIn  *plug_in,
                        const gchar *proc_name)
{
  LigmaPlugInProcedure *proc = NULL;

  if (plug_in->plug_in_def)
    proc = ligma_plug_in_procedure_find (plug_in->plug_in_def->procedures,
                                        proc_name);

  if (! proc)
    proc = ligma_plug_in_procedure_find (plug_in->temp_procedures, proc_name);

  return proc;
}
