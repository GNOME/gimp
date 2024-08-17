/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpplugin-proc.c
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

#include "libgimpbase/gimpbase.h"

#include "plug-in-types.h"

#include "core/gimpparamspecs.h"

#include "pdb/gimppdberror.h"

#include "gimpplugin.h"
#include "gimpplugin-proc.h"
#include "gimpplugindef.h"
#include "gimppluginmanager.h"
#include "gimppluginmanager-file.h"
#include "gimppluginprocedure.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static GimpPlugInProcedure * gimp_plug_in_proc_find (GimpPlugIn  *plug_in,
                                                     const gchar *proc_name);


/*  public functions  */

gboolean
gimp_plug_in_set_proc_image_types (GimpPlugIn   *plug_in,
                                   const gchar  *proc_name,
                                   const gchar  *image_types,
                                   GError      **error)
{
  GimpPlugInProcedure *proc;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = gimp_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register images types "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   gimp_object_get_name (plug_in),
                   gimp_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  gimp_plug_in_procedure_set_image_types (proc, image_types);

  return TRUE;
}

gboolean
gimp_plug_in_set_proc_sensitivity_mask (GimpPlugIn   *plug_in,
                                        const gchar  *proc_name,
                                        gint          sensitivity_mask,
                                        GError      **error)
{
  GimpPlugInProcedure *proc;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = gimp_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register the sensitivity mask \"%x\" "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   gimp_object_get_name (plug_in),
                   gimp_file_get_utf8_name (plug_in->file),
                   sensitivity_mask, proc_name);

      return FALSE;
    }

  gimp_plug_in_procedure_set_sensitivity_mask (proc, sensitivity_mask);

  return TRUE;
}

gboolean
gimp_plug_in_set_proc_menu_label (GimpPlugIn   *plug_in,
                                  const gchar  *proc_name,
                                  const gchar  *menu_label,
                                  GError      **error)
{
  GimpPlugInProcedure *proc;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);
  g_return_val_if_fail (menu_label != NULL && strlen (menu_label), FALSE);

  proc = gimp_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register the menu label \"%s\" "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   gimp_object_get_name (plug_in),
                   gimp_file_get_utf8_name (plug_in->file),
                   menu_label, proc_name);

      return FALSE;
    }

  return gimp_plug_in_procedure_set_menu_label (proc, menu_label, error);
}

gboolean
gimp_plug_in_add_proc_menu_path (GimpPlugIn   *plug_in,
                                 const gchar  *proc_name,
                                 const gchar  *menu_path,
                                 GError      **error)
{
  GimpPlugInProcedure *proc;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);
  g_return_val_if_fail (menu_path != NULL, FALSE);

  proc = gimp_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register the menu item \"%s\" "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   gimp_object_get_name (plug_in),
                   gimp_file_get_utf8_name (plug_in->file),
                   menu_path, proc_name);

      return FALSE;
    }

  return gimp_plug_in_procedure_add_menu_path (proc, menu_path, error);
}

gboolean
gimp_plug_in_set_proc_icon (GimpPlugIn    *plug_in,
                            const gchar   *proc_name,
                            GimpIconType   type,
                            const guint8  *data,
                            gint           data_length,
                            GError       **error)
{
  GimpPlugInProcedure *proc;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = gimp_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to set the icon "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   gimp_object_get_name (plug_in),
                   gimp_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  return gimp_plug_in_procedure_set_icon (proc, type, data, data_length,
                                          error);
}

gboolean
gimp_plug_in_set_proc_help (GimpPlugIn   *plug_in,
                            const gchar  *proc_name,
                            const gchar  *blurb,
                            const gchar  *help,
                            const gchar  *help_id,
                            GError      **error)
{
  GimpPlugInProcedure *proc;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = gimp_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register help "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   gimp_object_get_name (plug_in),
                   gimp_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  gimp_procedure_set_help (GIMP_PROCEDURE (proc),
                           blurb, help, help_id);

  return TRUE;
}

gboolean
gimp_plug_in_set_proc_attribution (GimpPlugIn   *plug_in,
                                   const gchar  *proc_name,
                                   const gchar  *authors,
                                   const gchar  *copyright,
                                   const gchar  *date,
                                   GError      **error)
{
  GimpPlugInProcedure *proc;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = gimp_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register the attribution "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   gimp_object_get_name (plug_in),
                   gimp_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  gimp_procedure_set_attribution (GIMP_PROCEDURE (proc),
                                  authors, copyright, date);

  return TRUE;
}

static inline gboolean
GIMP_IS_PARAM_SPEC_RUN_MODE (GParamSpec *pspec)
{
  return (G_IS_PARAM_SPEC_ENUM (pspec) &&
          pspec->value_type == GIMP_TYPE_RUN_MODE);
}

static inline gboolean
GIMP_IS_PARAM_SPEC_FILE (GParamSpec *pspec)
{
  return (G_IS_PARAM_SPEC_OBJECT (pspec) &&
          pspec->value_type == G_TYPE_FILE);
}

gboolean
gimp_plug_in_set_file_proc_load_handler (GimpPlugIn   *plug_in,
                                         const gchar  *proc_name,
                                         const gchar  *extensions,
                                         const gchar  *prefixes,
                                         const gchar  *magics,
                                         GError      **error)
{
  GimpPlugInProcedure *proc;
  GimpProcedure       *procedure;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = gimp_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register procedure \"%s\" "
                   "as load handler.\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   gimp_object_get_name (plug_in),
                   gimp_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  procedure = GIMP_PROCEDURE (proc);

  if (((procedure->num_args   < 2)                        ||
       (procedure->num_values < 1)                        ||
       ! GIMP_IS_PARAM_SPEC_RUN_MODE (procedure->args[0]) ||
       ! GIMP_IS_PARAM_SPEC_FILE     (procedure->args[1]) ||
       (! proc->generic_file_proc &&
        ! GIMP_IS_PARAM_SPEC_IMAGE (procedure->values[0]))))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_FAILED,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register procedure \"%s\" "
                   "as load handler which does not take the standard "
                   "load procedure arguments: "
                   "(GimpRunMode, GFile) -> (GimpImage)",
                   gimp_object_get_name (plug_in),
                   gimp_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  gimp_plug_in_procedure_set_file_proc (proc, extensions, prefixes, magics);

  gimp_plug_in_manager_add_load_procedure (plug_in->manager, proc);

  return TRUE;
}

gboolean
gimp_plug_in_set_file_proc_save_handler (GimpPlugIn   *plug_in,
                                         const gchar  *proc_name,
                                         const gchar  *extensions,
                                         const gchar  *prefixes,
                                         GError      **error)
{
  GimpPlugInProcedure *proc;
  GimpProcedure       *procedure;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = gimp_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register procedure \"%s\" "
                   "as save handler.\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   gimp_object_get_name (plug_in),
                   gimp_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  procedure = GIMP_PROCEDURE (proc);

  if ((procedure->num_args < 4)                                ||
      ! GIMP_IS_PARAM_SPEC_RUN_MODE       (procedure->args[0]) ||
      ! GIMP_IS_PARAM_SPEC_IMAGE          (procedure->args[1]) ||
      ! GIMP_IS_PARAM_SPEC_FILE           (procedure->args[2]) ||
      ! GIMP_IS_PARAM_SPEC_EXPORT_OPTIONS (procedure->args[3]))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_FAILED,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register procedure \"%s\" "
                   "as save handler which does not take the standard "
                   "save procedure arguments:\n"
                   "(GimpRunMode, GimpImage, GFile, GimpExportOptions)",
                   gimp_object_get_name (plug_in),
                   gimp_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  gimp_plug_in_procedure_set_file_proc (proc, extensions, prefixes, NULL);

  gimp_plug_in_manager_add_save_procedure (plug_in->manager, proc);

  return TRUE;
}

gboolean
gimp_plug_in_set_file_proc_priority (GimpPlugIn   *plug_in,
                                     const gchar  *proc_name,
                                     gint          priority,
                                     GError      **error)
{
  GimpPlugInProcedure *proc;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = gimp_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register the priority "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   gimp_object_get_name (plug_in),
                   gimp_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  gimp_plug_in_procedure_set_priority (proc, priority);

  return TRUE;
}

gboolean
gimp_plug_in_set_file_proc_mime_types (GimpPlugIn   *plug_in,
                                       const gchar  *proc_name,
                                       const gchar  *mime_types,
                                       GError      **error)
{
  GimpPlugInProcedure *proc;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = gimp_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register mime types "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   gimp_object_get_name (plug_in),
                   gimp_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  gimp_plug_in_procedure_set_mime_types (proc, mime_types);

  return TRUE;
}

gboolean
gimp_plug_in_set_file_proc_handles_remote (GimpPlugIn   *plug_in,
                                           const gchar  *proc_name,
                                           GError      **error)
{
  GimpPlugInProcedure *proc;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = gimp_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register 'handles remote' "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   gimp_object_get_name (plug_in),
                   gimp_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  gimp_plug_in_procedure_set_handles_remote (proc);

  return TRUE;
}

gboolean
gimp_plug_in_set_file_proc_handles_raw (GimpPlugIn   *plug_in,
                                        const gchar  *proc_name,
                                        GError      **error)
{
  GimpPlugInProcedure *proc;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = gimp_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register 'handles raw' "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   gimp_object_get_name (plug_in),
                   gimp_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  if (proc->handles_vector)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_FAILED,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register 'handles raw' "
                   "for procedure \"%s\" which already 'handles vector'.\n"
                   "It cannot handle both.",
                   gimp_object_get_name (plug_in),
                   gimp_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }
  gimp_plug_in_procedure_set_handles_raw (proc);

  return TRUE;
}

gboolean
gimp_plug_in_set_file_proc_handles_vector (GimpPlugIn   *plug_in,
                                           const gchar  *proc_name,
                                           GError      **error)
{
  GimpPlugInProcedure *proc;
  GimpProcedure       *procedure;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = gimp_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register 'handles vector' "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   gimp_object_get_name (plug_in),
                   gimp_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  procedure = GIMP_PROCEDURE (proc);

  if (procedure->num_args < 4                            ||
      procedure->num_values < 1                          ||
      ! GIMP_IS_PARAM_SPEC_RUN_MODE (procedure->args[0]) ||
      ! GIMP_IS_PARAM_SPEC_FILE     (procedure->args[1]) ||
      ! G_IS_PARAM_SPEC_INT         (procedure->args[2]) ||
      ! G_IS_PARAM_SPEC_INT         (procedure->args[3]) ||
      ! GIMP_IS_PARAM_SPEC_IMAGE (procedure->values[0]))
    {

      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_FAILED,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register procedure \"%s\" "
                   "as a vector load procedure which does not take the "
                   "standard load procedure procedure arguments: "
                   "(GimpRunMode, file, int, int) -> (image)",
                   gimp_object_get_name (plug_in),
                   gimp_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  if (proc->handles_raw)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_FAILED,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register 'handles vector' "
                   "for procedure \"%s\" which already 'handles raw'.\n"
                   "It cannot handle both.",
                   gimp_object_get_name (plug_in),
                   gimp_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  gimp_plug_in_procedure_set_handles_vector (proc);

  return TRUE;
}

gboolean
gimp_plug_in_set_file_proc_thumb_loader (GimpPlugIn   *plug_in,
                                         const gchar  *proc_name,
                                         const gchar  *thumb_proc_name,
                                         GError      **error)
{
  GimpPlugInProcedure *proc;
  GimpPlugInProcedure *thumb_proc;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);
  g_return_val_if_fail (thumb_proc_name != NULL, FALSE);

  proc       = gimp_plug_in_proc_find (plug_in, proc_name);
  thumb_proc = gimp_plug_in_proc_find (plug_in, thumb_proc_name);

  if (! proc)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register a thumbnail loader "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   gimp_object_get_name (plug_in),
                   gimp_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  if (! thumb_proc)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register a procedure \"%s\" "
                   "as thumbnail loader.\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   gimp_object_get_name (plug_in),
                   gimp_file_get_utf8_name (plug_in->file),
                   thumb_proc_name);

      return FALSE;
    }

  gimp_plug_in_procedure_set_thumb_loader (proc, thumb_proc_name);

  return TRUE;
}

gboolean
gimp_plug_in_set_batch_interpreter (GimpPlugIn   *plug_in,
                                    const gchar  *proc_name,
                                    const gchar  *interpreter_name,
                                    GError      **error)
{
  GimpPlugInProcedure *proc;
  GimpProcedure       *procedure;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = gimp_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register procedure \"%s\" "
                   "as a 'batch interpreter'.\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   gimp_object_get_name (plug_in),
                   gimp_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  procedure = GIMP_PROCEDURE (proc);

  if (procedure->num_args < 2                            ||
      ! GIMP_IS_PARAM_SPEC_RUN_MODE (procedure->args[0]) ||
      ! G_IS_PARAM_SPEC_STRING   (procedure->args[1]))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_FAILED,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register procedure \"%s\" "
                   "as a batch interpreter which does not take the standard "
                   "batch interpreter procedure arguments: "
                   "(GimpRunMode, gchar *) -> ()",
                   gimp_object_get_name (plug_in),
                   gimp_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  gimp_plug_in_procedure_set_batch_interpreter (proc, interpreter_name);
  gimp_plug_in_manager_add_batch_procedure (plug_in->manager, proc);

  return TRUE;
}


/*  private functions  */

static GimpPlugInProcedure *
gimp_plug_in_proc_find (GimpPlugIn  *plug_in,
                        const gchar *proc_name)
{
  GimpPlugInProcedure *proc = NULL;

  if (plug_in->plug_in_def)
    proc = gimp_plug_in_procedure_find (plug_in->plug_in_def->procedures,
                                        proc_name);

  if (! proc)
    proc = gimp_plug_in_procedure_find (plug_in->temp_procedures, proc_name);

  return proc;
}
