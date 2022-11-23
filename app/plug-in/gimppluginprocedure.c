/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapluginprocedure.c
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

#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libligmabase/ligmabase.h"

#include "plug-in-types.h"

#include "gegl/ligma-babl-compat.h"

#include "core/ligma.h"
#include "core/ligma-memsize.h"
#include "core/ligmadrawable.h"
#include "core/ligmaimage.h"
#include "core/ligmaparamspecs.h"

#include "file/file-utils.h"

#define __YES_I_NEED_LIGMA_PLUG_IN_MANAGER_CALL__
#include "ligmapluginmanager-call.h"

#include "ligmapluginerror.h"
#include "ligmapluginprocedure.h"
#include "plug-in-menu-path.h"

#include "ligma-intl.h"


enum
{
  MENU_PATH_ADDED,
  LAST_SIGNAL
};


static void     ligma_plug_in_procedure_finalize        (GObject        *object);

static gint64   ligma_plug_in_procedure_get_memsize     (LigmaObject     *object,
                                                        gint64         *gui_size);

static gchar  * ligma_plug_in_procedure_get_description (LigmaViewable   *viewable,
                                                        gchar         **tooltip);

static const gchar * ligma_plug_in_procedure_get_label  (LigmaProcedure  *procedure);
static const gchar * ligma_plug_in_procedure_get_menu_label
                                                       (LigmaProcedure  *procedure);
static const gchar * ligma_plug_in_procedure_get_blurb  (LigmaProcedure  *procedure);
static const gchar * ligma_plug_in_procedure_get_help_id(LigmaProcedure  *procedure);
static gboolean   ligma_plug_in_procedure_get_sensitive (LigmaProcedure  *procedure,
                                                        LigmaObject     *object,
                                                        const gchar  **reason);
static LigmaValueArray * ligma_plug_in_procedure_execute (LigmaProcedure  *procedure,
                                                        Ligma           *ligma,
                                                        LigmaContext    *context,
                                                        LigmaProgress   *progress,
                                                        LigmaValueArray *args,
                                                        GError        **error);
static void     ligma_plug_in_procedure_execute_async   (LigmaProcedure  *procedure,
                                                        Ligma           *ligma,
                                                        LigmaContext    *context,
                                                        LigmaProgress   *progress,
                                                        LigmaValueArray *args,
                                                        LigmaDisplay    *display);

static GFile  * ligma_plug_in_procedure_real_get_file   (LigmaPlugInProcedure *procedure);

static gboolean ligma_plug_in_procedure_validate_args   (LigmaPlugInProcedure *proc,
                                                        Ligma                *ligma,
                                                        LigmaValueArray      *args,
                                                        GError             **error);


G_DEFINE_TYPE (LigmaPlugInProcedure, ligma_plug_in_procedure,
               LIGMA_TYPE_PROCEDURE)

#define parent_class ligma_plug_in_procedure_parent_class

static guint ligma_plug_in_procedure_signals[LAST_SIGNAL] = { 0 };


static void
ligma_plug_in_procedure_class_init (LigmaPlugInProcedureClass *klass)
{
  GObjectClass       *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass    *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaViewableClass  *viewable_class    = LIGMA_VIEWABLE_CLASS (klass);
  LigmaProcedureClass *proc_class        = LIGMA_PROCEDURE_CLASS (klass);

  ligma_plug_in_procedure_signals[MENU_PATH_ADDED] =
    g_signal_new ("menu-path-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaPlugInProcedureClass, menu_path_added),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  object_class->finalize            = ligma_plug_in_procedure_finalize;

  ligma_object_class->get_memsize    = ligma_plug_in_procedure_get_memsize;

  viewable_class->default_icon_name = "system-run";
  viewable_class->get_description   = ligma_plug_in_procedure_get_description;

  proc_class->get_label             = ligma_plug_in_procedure_get_label;
  proc_class->get_menu_label        = ligma_plug_in_procedure_get_menu_label;
  proc_class->get_blurb             = ligma_plug_in_procedure_get_blurb;
  proc_class->get_help_id           = ligma_plug_in_procedure_get_help_id;
  proc_class->get_sensitive         = ligma_plug_in_procedure_get_sensitive;
  proc_class->execute               = ligma_plug_in_procedure_execute;
  proc_class->execute_async         = ligma_plug_in_procedure_execute_async;

  klass->get_file                   = ligma_plug_in_procedure_real_get_file;
  klass->menu_path_added            = NULL;
}

static void
ligma_plug_in_procedure_init (LigmaPlugInProcedure *proc)
{
  LIGMA_PROCEDURE (proc)->proc_type = LIGMA_PDB_PROC_TYPE_PLUGIN;

  proc->icon_data_length = -1;
}

static void
ligma_plug_in_procedure_finalize (GObject *object)
{
  LigmaPlugInProcedure *proc = LIGMA_PLUG_IN_PROCEDURE (object);

  g_object_unref (proc->file);
  g_free (proc->menu_label);

  g_list_free_full (proc->menu_paths, (GDestroyNotify) g_free);

  g_free (proc->help_id_with_domain);

  g_free (proc->icon_data);
  g_free (proc->image_types);
  g_free (proc->insensitive_reason);

  g_free (proc->extensions);
  g_free (proc->prefixes);
  g_free (proc->magics);
  g_free (proc->mime_types);

  g_slist_free_full (proc->extensions_list, (GDestroyNotify) g_free);
  g_slist_free_full (proc->prefixes_list, (GDestroyNotify) g_free);
  g_slist_free_full (proc->magics_list, (GDestroyNotify) g_free);
  g_slist_free_full (proc->mime_types_list, (GDestroyNotify) g_free);

  g_free (proc->thumb_loader);
  g_free (proc->batch_interpreter_name);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
ligma_plug_in_procedure_get_memsize (LigmaObject *object,
                                    gint64     *gui_size)
{
  LigmaPlugInProcedure *proc    = LIGMA_PLUG_IN_PROCEDURE (object);
  gint64               memsize = 0;
  GList               *list;
  GSList              *slist;

  memsize += ligma_g_object_get_memsize (G_OBJECT (proc->file));
  memsize += ligma_string_get_memsize (proc->menu_label);

  for (list = proc->menu_paths; list; list = g_list_next (list))
    memsize += sizeof (GList) + ligma_string_get_memsize (list->data);

  switch (proc->icon_type)
    {
    case LIGMA_ICON_TYPE_ICON_NAME:
    case LIGMA_ICON_TYPE_IMAGE_FILE:
      memsize += ligma_string_get_memsize ((const gchar *) proc->icon_data);
      break;

    case LIGMA_ICON_TYPE_PIXBUF:
      memsize += proc->icon_data_length;
      break;
    }

  memsize += ligma_string_get_memsize (proc->extensions);
  memsize += ligma_string_get_memsize (proc->prefixes);
  memsize += ligma_string_get_memsize (proc->magics);
  memsize += ligma_string_get_memsize (proc->mime_types);
  memsize += ligma_string_get_memsize (proc->thumb_loader);
  memsize += ligma_string_get_memsize (proc->batch_interpreter_name);

  for (slist = proc->extensions_list; slist; slist = g_slist_next (slist))
    memsize += sizeof (GSList) + ligma_string_get_memsize (slist->data);

  for (slist = proc->prefixes_list; slist; slist = g_slist_next (slist))
    memsize += sizeof (GSList) + ligma_string_get_memsize (slist->data);

  for (slist = proc->magics_list; slist; slist = g_slist_next (slist))
    memsize += sizeof (GSList) + ligma_string_get_memsize (slist->data);

  for (slist = proc->mime_types_list; slist; slist = g_slist_next (slist))
    memsize += sizeof (GSList) + ligma_string_get_memsize (slist->data);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gchar *
ligma_plug_in_procedure_get_description (LigmaViewable  *viewable,
                                        gchar        **tooltip)
{
  LigmaProcedure *procedure = LIGMA_PROCEDURE (viewable);

  if (tooltip)
    *tooltip = g_strdup (ligma_procedure_get_blurb (procedure));

  return g_strdup (ligma_procedure_get_label (procedure));
}

static const gchar *
ligma_plug_in_procedure_get_label (LigmaProcedure *procedure)
{
  LigmaPlugInProcedure *proc = LIGMA_PLUG_IN_PROCEDURE (procedure);

  if (! proc->menu_label)
    return NULL;

  return LIGMA_PROCEDURE_CLASS (parent_class)->get_label (procedure);
}

static const gchar *
ligma_plug_in_procedure_get_menu_label (LigmaProcedure *procedure)
{
  LigmaPlugInProcedure *proc = LIGMA_PLUG_IN_PROCEDURE (procedure);

  if (proc->menu_label)
    return proc->menu_label;

  return LIGMA_PROCEDURE_CLASS (parent_class)->get_menu_label (procedure);
}

static const gchar *
ligma_plug_in_procedure_get_blurb (LigmaProcedure *procedure)
{
  if (procedure->blurb)
    return procedure->blurb;

  return NULL;
}

static const gchar *
ligma_plug_in_procedure_get_help_id (LigmaProcedure *procedure)
{
  LigmaPlugInProcedure *proc = LIGMA_PLUG_IN_PROCEDURE (procedure);
  const gchar         *domain;
  const gchar         *help_id;

  if (proc->help_id_with_domain)
    return proc->help_id_with_domain;

  domain = ligma_plug_in_procedure_get_help_domain (proc);

  if (procedure->help_id)
    help_id = procedure->help_id;
  else
    help_id = ligma_object_get_name (procedure);

  if (domain)
    proc->help_id_with_domain = g_strconcat (domain, "?", help_id, NULL);
  else
    proc->help_id_with_domain = g_strdup (help_id);

  return proc->help_id_with_domain;
}

static gboolean
ligma_plug_in_procedure_get_sensitive (LigmaProcedure  *procedure,
                                      LigmaObject     *object,
                                      const gchar   **reason)
{
  LigmaPlugInProcedure *proc       = LIGMA_PLUG_IN_PROCEDURE (procedure);
  LigmaImage           *image;
  GList               *drawables  = NULL;
  LigmaImageType        image_type = -1;
  gboolean             sensitive  = FALSE;

  g_return_val_if_fail (object == NULL || LIGMA_IS_IMAGE (object), FALSE);

  image = LIGMA_IMAGE (object);
  if (image)
    drawables = ligma_image_get_selected_drawables (image);

  if (drawables)
    {
      const Babl *format = ligma_drawable_get_format (drawables->data);

      image_type = ligma_babl_format_get_image_type (format);
    }

  switch (image_type)
    {
    case LIGMA_RGB_IMAGE:
      sensitive = proc->image_types_val & LIGMA_PLUG_IN_RGB_IMAGE;
      break;
    case LIGMA_RGBA_IMAGE:
      sensitive = proc->image_types_val & LIGMA_PLUG_IN_RGBA_IMAGE;
      break;
    case LIGMA_GRAY_IMAGE:
      sensitive = proc->image_types_val & LIGMA_PLUG_IN_GRAY_IMAGE;
      break;
    case LIGMA_GRAYA_IMAGE:
      sensitive = proc->image_types_val & LIGMA_PLUG_IN_GRAYA_IMAGE;
      break;
    case LIGMA_INDEXED_IMAGE:
      sensitive = proc->image_types_val & LIGMA_PLUG_IN_INDEXED_IMAGE;
      break;
    case LIGMA_INDEXEDA_IMAGE:
      sensitive = proc->image_types_val & LIGMA_PLUG_IN_INDEXEDA_IMAGE;
      break;
    default:
      break;
    }

  if (! image &&
      (proc->sensitivity_mask & LIGMA_PROCEDURE_SENSITIVE_NO_IMAGE) != 0)
    sensitive = TRUE;
  else if (g_list_length (drawables) == 1 && proc->sensitivity_mask != 0 &&
           (proc->sensitivity_mask & LIGMA_PROCEDURE_SENSITIVE_DRAWABLE) == 0)
    sensitive = FALSE;
  else if (g_list_length (drawables) == 0 &&
           (proc->sensitivity_mask & LIGMA_PROCEDURE_SENSITIVE_NO_DRAWABLES) == 0)
    sensitive = FALSE;
  else if (g_list_length (drawables) > 1 &&
           (proc->sensitivity_mask & LIGMA_PROCEDURE_SENSITIVE_DRAWABLES) == 0)
    sensitive = FALSE;

  g_list_free (drawables);

  if (! sensitive)
    *reason = proc->insensitive_reason;

  return sensitive ? TRUE : FALSE;
}

static LigmaValueArray *
ligma_plug_in_procedure_execute (LigmaProcedure  *procedure,
                                Ligma           *ligma,
                                LigmaContext    *context,
                                LigmaProgress   *progress,
                                LigmaValueArray *args,
                                GError        **error)
{
  LigmaPlugInProcedure *plug_in_procedure = LIGMA_PLUG_IN_PROCEDURE (procedure);
  GError              *pdb_error         = NULL;

  if (! ligma_plug_in_procedure_validate_args (plug_in_procedure, ligma,
                                              args, &pdb_error))
    {
      LigmaValueArray *return_vals;

      return_vals = ligma_procedure_get_return_values (procedure, FALSE,
                                                      pdb_error);
      g_propagate_error (error, pdb_error);

      return return_vals;
    }

  if (procedure->proc_type == LIGMA_PDB_PROC_TYPE_INTERNAL)
    return LIGMA_PROCEDURE_CLASS (parent_class)->execute (procedure, ligma,
                                                         context, progress,
                                                         args, error);

  return ligma_plug_in_manager_call_run (ligma->plug_in_manager,
                                        context, progress,
                                        LIGMA_PLUG_IN_PROCEDURE (procedure),
                                        args, TRUE, NULL);
}

static void
ligma_plug_in_procedure_execute_async (LigmaProcedure  *procedure,
                                      Ligma           *ligma,
                                      LigmaContext    *context,
                                      LigmaProgress   *progress,
                                      LigmaValueArray *args,
                                      LigmaDisplay    *display)
{
  LigmaPlugInProcedure *plug_in_procedure = LIGMA_PLUG_IN_PROCEDURE (procedure);
  GError              *error             = NULL;

  if (ligma_plug_in_procedure_validate_args (plug_in_procedure, ligma,
                                            args, &error))
    {
      LigmaValueArray *return_vals;

      return_vals = ligma_plug_in_manager_call_run (ligma->plug_in_manager,
                                                   context, progress,
                                                   plug_in_procedure,
                                                   args, FALSE, display);

      if (return_vals)
        {
          ligma_plug_in_procedure_handle_return_values (plug_in_procedure,
                                                       ligma, progress,
                                                       return_vals);
          ligma_value_array_unref (return_vals);
        }
    }
  else
    {
      ligma_message_literal (ligma, G_OBJECT (progress), LIGMA_MESSAGE_ERROR,
                            error->message);
      g_error_free (error);
    }
}

static GFile *
ligma_plug_in_procedure_real_get_file (LigmaPlugInProcedure *procedure)
{
  return procedure->file;
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

static gboolean
ligma_plug_in_procedure_validate_args (LigmaPlugInProcedure *proc,
                                      Ligma                *ligma,
                                      LigmaValueArray      *args,
                                      GError             **error)
{
#if 0
  LigmaProcedure *procedure = LIGMA_PROCEDURE (proc);
  GValue        *uri_value = NULL;

  if (! proc->file_proc)
    return TRUE;

  /*  make sure that the passed strings are actually URIs, not just a
   *  file path (bug 758685)
   */

  if ((procedure->num_args   >= 3)                     &&
      (procedure->num_values >= 1)                     &&
      LIGMA_IS_PARAM_SPEC_RUN_MODE (procedure->args[0]) &&
      LIGMA_IS_PARAM_SPEC_FILE     (procedure->args[1]) &&
      LIGMA_IS_PARAM_SPEC_IMAGE    (procedure->values[0]))
    {
      uri_value = ligma_value_array_index (args, 1);
    }
  else if ((procedure->num_args >= 5)                       &&
           LIGMA_IS_PARAM_SPEC_RUN_MODE (procedure->args[0]) &&
           LIGMA_IS_PARAM_SPEC_IMAGE    (procedure->args[1]) &&
           LIGMA_IS_PARAM_SPEC_DRAWABLE (procedure->args[2]) &&
           LIGMA_IS_PARAM_SPEC_FILE     (procedure->args[3]))
    {
      uri_value = ligma_value_array_index (args, 3);
    }

  if (uri_value)
    {
      GFile *file;

      file = file_utils_filename_to_file (ligma,
                                          g_value_get_string (uri_value),
                                          error);

      if (! file)
        return FALSE;

      g_value_take_string (uri_value, g_file_get_uri (file));
      g_object_unref (file);
    }
#endif

  return TRUE;
}


/*  public functions  */

LigmaProcedure *
ligma_plug_in_procedure_new (LigmaPDBProcType  proc_type,
                            GFile           *file)
{
  LigmaPlugInProcedure *proc;

  g_return_val_if_fail (proc_type == LIGMA_PDB_PROC_TYPE_PLUGIN ||
                        proc_type == LIGMA_PDB_PROC_TYPE_EXTENSION, NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);

  proc = g_object_new (LIGMA_TYPE_PLUG_IN_PROCEDURE, NULL);

  proc->file = g_object_ref (file);

  LIGMA_PROCEDURE (proc)->proc_type = proc_type;

  return LIGMA_PROCEDURE (proc);
}

LigmaPlugInProcedure *
ligma_plug_in_procedure_find (GSList      *list,
                             const gchar *proc_name)
{
  GSList *l;

  for (l = list; l; l = g_slist_next (l))
    {
      LigmaObject *object = l->data;

      if (! strcmp (proc_name, ligma_object_get_name (object)))
        return LIGMA_PLUG_IN_PROCEDURE (object);
    }

  return NULL;
}

GFile *
ligma_plug_in_procedure_get_file (LigmaPlugInProcedure *proc)
{
  g_return_val_if_fail (LIGMA_IS_PLUG_IN_PROCEDURE (proc), NULL);

  return LIGMA_PLUG_IN_PROCEDURE_GET_CLASS (proc)->get_file (proc);
}

void
ligma_plug_in_procedure_set_help_domain (LigmaPlugInProcedure *proc,
                                        const gchar         *help_domain)
{
  g_return_if_fail (LIGMA_IS_PLUG_IN_PROCEDURE (proc));

  proc->help_domain = help_domain ? g_quark_from_string (help_domain) : 0;
}

const gchar *
ligma_plug_in_procedure_get_help_domain (LigmaPlugInProcedure *proc)
{
  g_return_val_if_fail (LIGMA_IS_PLUG_IN_PROCEDURE (proc), NULL);

  return g_quark_to_string (proc->help_domain);
}

gboolean
ligma_plug_in_procedure_set_menu_label (LigmaPlugInProcedure  *proc,
                                       const gchar          *menu_label,
                                       GError              **error)
{
  g_return_val_if_fail (LIGMA_IS_PLUG_IN_PROCEDURE (proc), FALSE);
  g_return_val_if_fail (menu_label != NULL && strlen (menu_label), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (menu_label[0] == '<')
    {
      gchar *basename = g_path_get_basename (ligma_file_get_utf8_name (proc->file));

      g_set_error (error, LIGMA_PLUG_IN_ERROR, LIGMA_PLUG_IN_FAILED,
                   "Plug-in \"%s\"\n(%s)\n\n"
                   "attempted to install procedure \"%s\" with a full "
                   "menu path \"%s\" as menu label, this is not supported "
                   "any longer.",
                   basename, ligma_file_get_utf8_name (proc->file),
                   ligma_object_get_name (proc),
                   menu_label);

      g_free (basename);

      return FALSE;
    }

  g_clear_pointer (&LIGMA_PROCEDURE (proc)->label, g_free);

  g_free (proc->menu_label);
  proc->menu_label = g_strdup (menu_label);

  return TRUE;
}

gboolean
ligma_plug_in_procedure_add_menu_path (LigmaPlugInProcedure  *proc,
                                      const gchar          *menu_path,
                                      GError              **error)
{
  LigmaProcedure *procedure;
  gchar         *basename = NULL;
  const gchar   *required = NULL;
  gchar         *p;
  gchar         *mapped_path;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN_PROCEDURE (proc), FALSE);
  g_return_val_if_fail (menu_path != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  procedure = LIGMA_PROCEDURE (proc);

  if (! proc->menu_label)
    {
      basename = g_path_get_basename (ligma_file_get_utf8_name (proc->file));

      g_set_error (error, LIGMA_PLUG_IN_ERROR, LIGMA_PLUG_IN_FAILED,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register the procedure \"%s\" "
                   "in the menu \"%s\", but the procedure has no label. "
                   "This is not allowed.",
                   basename, ligma_file_get_utf8_name (proc->file),
                   ligma_object_get_name (proc),
                   menu_path);

      g_free (basename);

      return FALSE;
    }

  p = strchr (menu_path, '>');
  if (p == NULL || (*(++p) && *p != '/'))
    {
      basename = g_path_get_basename (ligma_file_get_utf8_name (proc->file));

      g_set_error (error, LIGMA_PLUG_IN_ERROR, LIGMA_PLUG_IN_FAILED,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to install procedure \"%s\"\n"
                   "in the invalid menu location \"%s\".\n"
                   "The menu path must look like either \"<Prefix>\" "
                   "or \"<Prefix>/path/to/item\".",
                   basename, ligma_file_get_utf8_name (proc->file),
                   ligma_object_get_name (proc),
                   menu_path);
      goto failure;
    }

  if (g_str_has_prefix (menu_path, "<Image>"))
    {
      if ((procedure->num_args < 1) ||
          ! LIGMA_IS_PARAM_SPEC_RUN_MODE (procedure->args[0]))
        {
          required = "LigmaRunMode";
          goto failure;
        }
    }
  else if (g_str_has_prefix (menu_path, "<Layers>"))
    {
      if ((procedure->num_args < 3)                          ||
          ! LIGMA_IS_PARAM_SPEC_RUN_MODE (procedure->args[0]) ||
          ! LIGMA_IS_PARAM_SPEC_IMAGE    (procedure->args[1]) ||
          ! (G_TYPE_FROM_INSTANCE (procedure->args[2])
                               == LIGMA_TYPE_PARAM_LAYER      ||
             G_TYPE_FROM_INSTANCE (procedure->args[2])
                               == LIGMA_TYPE_PARAM_DRAWABLE))
        {
          required = "LigmaRunMode, LigmaImage, (LigmaLayer | LigmaDrawable)";
          goto failure;
        }
    }
  else if (g_str_has_prefix (menu_path, "<Channels>"))
    {
      if ((procedure->num_args < 3)                          ||
          ! LIGMA_IS_PARAM_SPEC_RUN_MODE (procedure->args[0]) ||
          ! LIGMA_IS_PARAM_SPEC_IMAGE    (procedure->args[1]) ||
          ! (G_TYPE_FROM_INSTANCE (procedure->args[2])
                               == LIGMA_TYPE_PARAM_CHANNEL    ||
             G_TYPE_FROM_INSTANCE (procedure->args[2])
                               == LIGMA_TYPE_PARAM_DRAWABLE))
        {
          required = "LigmaRunMode, LigmaImage, (LigmaChannel | LigmaDrawable)";
          goto failure;
        }
    }
  else if (g_str_has_prefix (menu_path, "<Vectors>"))
    {
      if ((procedure->num_args < 3)                          ||
          ! LIGMA_IS_PARAM_SPEC_RUN_MODE (procedure->args[0]) ||
          ! LIGMA_IS_PARAM_SPEC_IMAGE    (procedure->args[1]) ||
          ! LIGMA_IS_PARAM_SPEC_VECTORS  (procedure->args[2]))
        {
          required = "LigmaRunMode, LigmaImage, LigmaVectors";
          goto failure;
        }
    }
  else if (g_str_has_prefix (menu_path, "<Colormap>"))
    {
      if ((procedure->num_args < 2)                          ||
          ! LIGMA_IS_PARAM_SPEC_RUN_MODE (procedure->args[0]) ||
          ! LIGMA_IS_PARAM_SPEC_IMAGE    (procedure->args[1]))
        {
          required = "LigmaRunMode, LigmaImage";
          goto failure;
        }
    }
  else if (g_str_has_prefix (menu_path, "<Brushes>")        ||
           g_str_has_prefix (menu_path, "<Dynamics>")       ||
           g_str_has_prefix (menu_path, "<MyPaintBrushes>") ||
           g_str_has_prefix (menu_path, "<Gradients>")      ||
           g_str_has_prefix (menu_path, "<Palettes>")       ||
           g_str_has_prefix (menu_path, "<Patterns>")       ||
           g_str_has_prefix (menu_path, "<ToolPresets>")    ||
           g_str_has_prefix (menu_path, "<Fonts>")          ||
           g_str_has_prefix (menu_path, "<Buffers>"))
    {
      if ((procedure->num_args < 1) ||
          ! LIGMA_IS_PARAM_SPEC_RUN_MODE (procedure->args[0]))
        {
          required = "LigmaRunMode";
          goto failure;
        }
    }
  else
    {
      basename = g_path_get_basename (ligma_file_get_utf8_name (proc->file));

      g_set_error (error, LIGMA_PLUG_IN_ERROR, LIGMA_PLUG_IN_FAILED,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to install procedure \"%s\" "
                   "in the invalid menu location \"%s\".\n"
                   "Use either \"<Image>\", "
                   "\"<Layers>\", \"<Channels>\", \"<Vectors>\", "
                   "\"<Colormap>\", \"<Brushes>\", \"<Dynamics>\", "
                   "\"<MyPaintBrushes>\", \"<Gradients>\", \"<Palettes>\", "
                   "\"<Patterns>\", \"<ToolPresets>\", \"<Fonts>\" "
                   "or \"<Buffers>\".",
                   basename, ligma_file_get_utf8_name (proc->file),
                   ligma_object_get_name (proc),
                   menu_path);
      goto failure;
    }

  g_free (basename);

  mapped_path = plug_in_menu_path_map (menu_path, NULL);

  proc->menu_paths = g_list_append (proc->menu_paths, mapped_path);

  g_signal_emit (proc, ligma_plug_in_procedure_signals[MENU_PATH_ADDED], 0,
                 mapped_path);

  return TRUE;

 failure:
  if (required)
    {
      gchar *prefix = g_strdup (menu_path);

      p = strchr (prefix, '>') + 1;
      *p = '\0';

      basename = g_path_get_basename (ligma_file_get_utf8_name (proc->file));

      g_set_error (error, LIGMA_PLUG_IN_ERROR, LIGMA_PLUG_IN_FAILED,
                   "Plug-in \"%s\"\n(%s)\n\n"
                   "attempted to install %s procedure \"%s\" "
                   "which does not take the standard %s plug-in's "
                   "arguments: (%s).",
                   basename, ligma_file_get_utf8_name (proc->file),
                   prefix, ligma_object_get_name (proc), prefix,
                   required);

      g_free (prefix);
    }

  g_free (basename);

  return FALSE;
}

gboolean
ligma_plug_in_procedure_set_icon (LigmaPlugInProcedure  *proc,
                                 LigmaIconType          icon_type,
                                 const guint8         *icon_data,
                                 gint                  icon_data_length,
                                 GError              **error)
{
  return ligma_plug_in_procedure_take_icon (proc, icon_type,
                                           g_memdup2 (icon_data, icon_data_length),
                                           icon_data_length, error);
}

gboolean
ligma_plug_in_procedure_take_icon (LigmaPlugInProcedure  *proc,
                                  LigmaIconType          icon_type,
                                  guint8               *icon_data,
                                  gint                  icon_data_length,
                                  GError              **error)
{
  const gchar *icon_name   = NULL;
  GdkPixbuf   *icon_pixbuf = NULL;
  gboolean     success     = TRUE;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN_PROCEDURE (proc), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (proc->icon_data)
    {
      g_free (proc->icon_data);
      proc->icon_data_length = -1;
      proc->icon_data        = NULL;
    }

  proc->icon_type = icon_type;

  switch (proc->icon_type)
    {
      GdkPixbufLoader *loader;

    case LIGMA_ICON_TYPE_ICON_NAME:
      proc->icon_data_length = -1;
      proc->icon_data        = icon_data;

      icon_name = (const gchar *) proc->icon_data;
      break;

    case LIGMA_ICON_TYPE_PIXBUF:
      proc->icon_data_length = icon_data_length;
      proc->icon_data        = icon_data;

      loader = gdk_pixbuf_loader_new ();

      if (! gdk_pixbuf_loader_write (loader,
                                     proc->icon_data,
                                     proc->icon_data_length,
                                     error))
        {
          gdk_pixbuf_loader_close (loader, NULL);
          success = FALSE;
        }
      else if (! gdk_pixbuf_loader_close (loader, error))
        {
          success = FALSE;
        }

      if (success)
        {
          icon_pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);

          if (icon_pixbuf)
            g_object_ref (icon_pixbuf);
        }

      g_object_unref (loader);
      break;

    case LIGMA_ICON_TYPE_IMAGE_FILE:
      proc->icon_data_length = -1;
      proc->icon_data        = icon_data;

      icon_pixbuf = gdk_pixbuf_new_from_file ((gchar *) proc->icon_data,
                                              error);

      if (! icon_pixbuf)
        success = FALSE;
      break;
    }

  ligma_viewable_set_icon_name (LIGMA_VIEWABLE (proc), icon_name);
  g_object_set (proc, "icon-pixbuf", icon_pixbuf, NULL);

  if (icon_pixbuf)
    g_object_unref (icon_pixbuf);

  return success;
}

static LigmaPlugInImageType
image_types_parse (const gchar *name,
                   const gchar *image_types)
{
  const gchar         *type_spec = image_types;
  LigmaPlugInImageType  types     = 0;

  /*  If the plug_in registers with image_type == NULL or "", return 0
   *  By doing so it won't be touched by plug_in_set_menu_sensitivity()
   */
  if (! image_types)
    return types;

  while (*image_types)
    {
      while (*image_types &&
             ((*image_types == ' ') ||
              (*image_types == '\t') ||
              (*image_types == ',')))
        image_types++;

      if (*image_types)
        {
          if (g_str_has_prefix (image_types, "RGBA"))
            {
              types |= LIGMA_PLUG_IN_RGBA_IMAGE;
              image_types += strlen ("RGBA");
            }
          else if (g_str_has_prefix (image_types, "RGB*"))
            {
              types |= LIGMA_PLUG_IN_RGB_IMAGE | LIGMA_PLUG_IN_RGBA_IMAGE;
              image_types += strlen ("RGB*");
            }
          else if (g_str_has_prefix (image_types, "RGB"))
            {
              types |= LIGMA_PLUG_IN_RGB_IMAGE;
              image_types += strlen ("RGB");
            }
          else if (g_str_has_prefix (image_types, "GRAYA"))
            {
              types |= LIGMA_PLUG_IN_GRAYA_IMAGE;
              image_types += strlen ("GRAYA");
            }
          else if (g_str_has_prefix (image_types, "GRAY*"))
            {
              types |= LIGMA_PLUG_IN_GRAY_IMAGE | LIGMA_PLUG_IN_GRAYA_IMAGE;
              image_types += strlen ("GRAY*");
            }
          else if (g_str_has_prefix (image_types, "GRAY"))
            {
              types |= LIGMA_PLUG_IN_GRAY_IMAGE;
              image_types += strlen ("GRAY");
            }
          else if (g_str_has_prefix (image_types, "INDEXEDA"))
            {
              types |= LIGMA_PLUG_IN_INDEXEDA_IMAGE;
              image_types += strlen ("INDEXEDA");
            }
          else if (g_str_has_prefix (image_types, "INDEXED*"))
            {
              types |= LIGMA_PLUG_IN_INDEXED_IMAGE | LIGMA_PLUG_IN_INDEXEDA_IMAGE;
              image_types += strlen ("INDEXED*");
            }
          else if (g_str_has_prefix (image_types, "INDEXED"))
            {
              types |= LIGMA_PLUG_IN_INDEXED_IMAGE;
              image_types += strlen ("INDEXED");
            }
          else if (g_str_has_prefix (image_types, "*"))
            {
              types |= (LIGMA_PLUG_IN_RGB_IMAGE     | LIGMA_PLUG_IN_RGBA_IMAGE  |
                        LIGMA_PLUG_IN_GRAY_IMAGE    | LIGMA_PLUG_IN_GRAYA_IMAGE |
                        LIGMA_PLUG_IN_INDEXED_IMAGE | LIGMA_PLUG_IN_INDEXEDA_IMAGE);
              image_types += strlen ("*");
            }
          else
            {
              g_printerr ("%s: image-type contains unrecognizable parts:"
                          "'%s'\n", name, type_spec);

              /* skip to next token */
              while (*image_types &&
                     *image_types != ' '  &&
                     *image_types != '\t' &&
                     *image_types != ',')
                {
                  image_types++;
                }
            }
        }
    }

  return types;
}

void
ligma_plug_in_procedure_set_image_types (LigmaPlugInProcedure *proc,
                                        const gchar         *image_types)
{
  GList *types = NULL;

  g_return_if_fail (LIGMA_IS_PLUG_IN_PROCEDURE (proc));

  if (proc->image_types)
    g_free (proc->image_types);

  proc->image_types     = g_strdup (image_types);
  proc->image_types_val = image_types_parse (ligma_object_get_name (proc),
                                             proc->image_types);

  g_clear_pointer (&proc->insensitive_reason, g_free);

  if (proc->image_types_val &
      (LIGMA_PLUG_IN_RGB_IMAGE | LIGMA_PLUG_IN_RGBA_IMAGE))
    {
      if ((proc->image_types_val & LIGMA_PLUG_IN_RGB_IMAGE) &&
          (proc->image_types_val & LIGMA_PLUG_IN_RGBA_IMAGE))
        {
          types = g_list_prepend (types, _("RGB"));
        }
      else if (proc->image_types_val & LIGMA_PLUG_IN_RGB_IMAGE)
        {
          types = g_list_prepend (types, _("RGB without alpha"));
        }
      else
        {
          types = g_list_prepend (types, _("RGB with alpha"));
        }
    }

  if (proc->image_types_val &
      (LIGMA_PLUG_IN_GRAY_IMAGE | LIGMA_PLUG_IN_GRAYA_IMAGE))
    {
      if ((proc->image_types_val & LIGMA_PLUG_IN_GRAY_IMAGE) &&
          (proc->image_types_val & LIGMA_PLUG_IN_GRAYA_IMAGE))
        {
          types = g_list_prepend (types, _("Grayscale"));
        }
      else if (proc->image_types_val & LIGMA_PLUG_IN_GRAY_IMAGE)
        {
          types = g_list_prepend (types, _("Grayscale without alpha"));
        }
      else
        {
          types = g_list_prepend (types, _("Grayscale with alpha"));
        }
    }

  if (proc->image_types_val &
      (LIGMA_PLUG_IN_INDEXED_IMAGE | LIGMA_PLUG_IN_INDEXEDA_IMAGE))
    {
      if ((proc->image_types_val & LIGMA_PLUG_IN_INDEXED_IMAGE) &&
          (proc->image_types_val & LIGMA_PLUG_IN_INDEXEDA_IMAGE))
        {
          types = g_list_prepend (types, _("Indexed"));
        }
      else if (proc->image_types_val & LIGMA_PLUG_IN_INDEXED_IMAGE)
        {
          types = g_list_prepend (types, _("Indexed without alpha"));
        }
      else
        {
          types = g_list_prepend (types, _("Indexed with alpha"));
        }
    }

  if (types)
    {
      GString *string;
      GList   *list;

      types = g_list_reverse (types);

      string = g_string_new (_("This plug-in only works on the "
                               "following layer types:"));
      g_string_append (string, " ");

      for (list = types; list; list = g_list_next (list))
        {
          g_string_append (string, list->data);

          if (list->next)
            g_string_append (string, ", ");
          else
            g_string_append (string, ".");
        }

      g_list_free (types);

      proc->insensitive_reason = g_string_free (string, FALSE);
    }
}

void
ligma_plug_in_procedure_set_sensitivity_mask (LigmaPlugInProcedure *proc,
                                             gint                 sensitivity_mask)
{
  g_return_if_fail (LIGMA_IS_PLUG_IN_PROCEDURE (proc));

  proc->sensitivity_mask = sensitivity_mask;
}

static GSList *
extensions_parse (gchar *extensions)
{
  GSList *list = NULL;

  /*  extensions can be NULL.  Avoid calling strtok if it is.  */
  if (extensions)
    {
      gchar *extension;
      gchar *next_token;

      /*  work on a copy  */
      extensions = g_strdup (extensions);

      next_token = extensions;
      extension = strtok (next_token, " \t,");

      while (extension)
        {
          list = g_slist_prepend (list, g_strdup (extension));
          extension = strtok (NULL, " \t,");
        }

      g_free (extensions);
    }

  return g_slist_reverse (list);
}

void
ligma_plug_in_procedure_set_file_proc (LigmaPlugInProcedure *proc,
                                      const gchar         *extensions,
                                      const gchar         *prefixes,
                                      const gchar         *magics)
{
  GSList *list;

  g_return_if_fail (LIGMA_IS_PLUG_IN_PROCEDURE (proc));

  proc->file_proc = TRUE;

  /*  extensions  */

  if (proc->extensions != extensions)
    {
      if (proc->extensions)
        g_free (proc->extensions);

      proc->extensions = g_strdup (extensions);
    }

  if (proc->extensions_list)
    g_slist_free_full (proc->extensions_list, (GDestroyNotify) g_free);

  proc->extensions_list = extensions_parse (proc->extensions);

  /*  prefixes  */

  if (proc->prefixes != prefixes)
    {
      if (proc->prefixes)
        g_free (proc->prefixes);

      proc->prefixes = g_strdup (prefixes);
    }

  if (proc->prefixes_list)
    g_slist_free_full (proc->prefixes_list, (GDestroyNotify) g_free);

  proc->prefixes_list = extensions_parse (proc->prefixes);

  /* don't allow "file:" to be registered as prefix */
  for (list = proc->prefixes_list; list; list = g_slist_next (list))
    {
      const gchar *prefix = list->data;

      if (prefix && strcmp (prefix, "file:") == 0)
        {
          g_free (list->data);
          proc->prefixes_list = g_slist_delete_link (proc->prefixes_list, list);
          break;
        }
    }

  /*  magics  */

  if (proc->magics != magics)
    {
      if (proc->magics)
        g_free (proc->magics);

      proc->magics = g_strdup (magics);
    }

  if (proc->magics_list)
    g_slist_free_full (proc->magics_list, (GDestroyNotify) g_free);

  proc->magics_list = extensions_parse (proc->magics);
}

void
ligma_plug_in_procedure_set_generic_file_proc (LigmaPlugInProcedure *proc,
                                              gboolean             is_generic_file_proc)
{
  proc->generic_file_proc = is_generic_file_proc;
}

void
ligma_plug_in_procedure_set_priority (LigmaPlugInProcedure *proc,
                                     gint                 priority)
{
  g_return_if_fail (LIGMA_IS_PLUG_IN_PROCEDURE (proc));

  proc->priority = priority;
}

void
ligma_plug_in_procedure_set_mime_types (LigmaPlugInProcedure *proc,
                                       const gchar         *mime_types)
{
  g_return_if_fail (LIGMA_IS_PLUG_IN_PROCEDURE (proc));

  if (proc->mime_types != mime_types)
    {
      if (proc->mime_types)
        g_free (proc->mime_types);

      proc->mime_types = g_strdup (mime_types);
    }

  if (proc->mime_types_list)
    g_slist_free_full (proc->mime_types_list, (GDestroyNotify) g_free);

  proc->mime_types_list = extensions_parse (proc->mime_types);
}

void
ligma_plug_in_procedure_set_handles_remote (LigmaPlugInProcedure *proc)
{
  g_return_if_fail (LIGMA_IS_PLUG_IN_PROCEDURE (proc));

  proc->handles_remote = TRUE;
}

void
ligma_plug_in_procedure_set_handles_raw (LigmaPlugInProcedure *proc)
{
  g_return_if_fail (LIGMA_IS_PLUG_IN_PROCEDURE (proc));

  proc->handles_raw = TRUE;
}

void
ligma_plug_in_procedure_set_thumb_loader (LigmaPlugInProcedure *proc,
                                         const gchar         *thumb_loader)
{
  g_return_if_fail (LIGMA_IS_PLUG_IN_PROCEDURE (proc));

  if (proc->thumb_loader)
    g_free (proc->thumb_loader);

  proc->thumb_loader = g_strdup (thumb_loader);
}

void
ligma_plug_in_procedure_set_batch_interpreter (LigmaPlugInProcedure *proc,
                                              const gchar         *name)
{
  g_return_if_fail (LIGMA_IS_PLUG_IN_PROCEDURE (proc));
  g_return_if_fail (name != NULL);

  if (proc->batch_interpreter_name)
    g_free (proc->batch_interpreter_name);

  proc->batch_interpreter      = TRUE;
  proc->batch_interpreter_name = g_strdup (name);
}

void
ligma_plug_in_procedure_handle_return_values (LigmaPlugInProcedure *proc,
                                             Ligma                *ligma,
                                             LigmaProgress        *progress,
                                             LigmaValueArray      *return_vals)
{
  g_return_if_fail (LIGMA_IS_PLUG_IN_PROCEDURE (proc));
  g_return_if_fail (return_vals != NULL);

  if (ligma_value_array_length (return_vals) == 0 ||
      G_VALUE_TYPE (ligma_value_array_index (return_vals, 0)) !=
      LIGMA_TYPE_PDB_STATUS_TYPE)
    {
      return;
    }

  switch (g_value_get_enum (ligma_value_array_index (return_vals, 0)))
    {
    case LIGMA_PDB_SUCCESS:
      break;

    case LIGMA_PDB_CALLING_ERROR:
      if (ligma_value_array_length (return_vals) > 1 &&
          G_VALUE_HOLDS_STRING (ligma_value_array_index (return_vals, 1)))
        {
          ligma_message (ligma, G_OBJECT (progress), LIGMA_MESSAGE_ERROR,
                        _("Calling error for '%s':\n"
                          "%s"),
                        ligma_procedure_get_label (LIGMA_PROCEDURE (proc)),
                        g_value_get_string (ligma_value_array_index (return_vals, 1)));
        }
      break;

    case LIGMA_PDB_EXECUTION_ERROR:
      if (ligma_value_array_length (return_vals) > 1 &&
          G_VALUE_HOLDS_STRING (ligma_value_array_index (return_vals, 1)))
        {
          ligma_message (ligma, G_OBJECT (progress), LIGMA_MESSAGE_ERROR,
                        _("Execution error for '%s':\n"
                          "%s"),
                        ligma_procedure_get_label (LIGMA_PROCEDURE (proc)),
                        g_value_get_string (ligma_value_array_index (return_vals, 1)));
        }
      break;
    }
}
