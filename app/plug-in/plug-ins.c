/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * plug-ins.c
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

#include <string.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpbase/gimpprotocol.h"
#include "libgimpconfig/gimpconfig.h"

#include "plug-in-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"

#include "pdb/gimp-pdb.h"
#include "pdb/gimptemporaryprocedure.h"

#include "plug-in.h"
#include "plug-in-data.h"
#include "plug-in-def.h"
#include "plug-in-help-domain.h"
#include "plug-in-locale-domain.h"
#include "plug-in-menu-branch.h"
#include "plug-in-rc.h"
#include "plug-ins.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   plug_ins_add_from_file     (const GimpDatafileData *file_data,
                                          gpointer                data);
static void   plug_ins_add_from_rc       (Gimp                   *gimp,
                                          PlugInDef              *plug_in_def);
static void   plug_ins_add_to_db         (Gimp                   *gimp,
                                          GimpContext            *context,
                                          GimpPlugInProcedure    *proc);
static gint   plug_ins_file_proc_compare (gconstpointer           a,
                                          gconstpointer           b,
                                          gpointer                data);


/*  public functions  */

void
plug_ins_init (Gimp               *gimp,
               GimpContext        *context,
               GimpInitStatusFunc  status_callback)
{
  gchar   *pluginrc;
  gchar   *path;
  GSList  *rc_defs;
  GSList  *list;
  GList   *extensions = NULL;
  gint     n_plugins;
  gint     n_extensions;
  gint     nth;
  GError  *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (status_callback != NULL);

  plug_in_init (gimp);

  /* search for binaries in the plug-in directory path */
  status_callback (_("Searching Plug-Ins"), "", 0.0);

  path = gimp_config_path_expand (gimp->config->plug_in_path, TRUE, NULL);

  gimp_datafiles_read_directories (path,
                                   G_FILE_TEST_IS_EXECUTABLE,
                                   plug_ins_add_from_file,
                                   &gimp->plug_in_defs);

  g_free (path);

  /* read the pluginrc file for cached data */
  if (gimp->config->plug_in_rc_path)
    {
      pluginrc = gimp_config_path_expand (gimp->config->plug_in_rc_path,
                                          TRUE, NULL);

      if (! g_path_is_absolute (pluginrc))
        {
          gchar *str = g_build_filename (gimp_directory (), pluginrc, NULL);

          g_free (pluginrc);
          pluginrc = str;
        }
    }
  else
    {
      pluginrc = gimp_personal_rc_file ("pluginrc");
    }

  status_callback (_("Resource configuration"),
                   gimp_filename_to_utf8 (pluginrc), 0.0);

  if (gimp->be_verbose)
    g_print (_("Parsing '%s'\n"), gimp_filename_to_utf8 (pluginrc));

  rc_defs = plug_in_rc_parse (gimp, pluginrc, &error);

  if (rc_defs)
    {
      for (list = rc_defs; list; list = g_slist_next (list))
        plug_ins_add_from_rc (gimp, list->data); /* consumes list->data */

      g_slist_free (rc_defs);
    }
  else
    {
      if (error->code != GIMP_CONFIG_ERROR_OPEN_ENOENT)
        g_message (error->message);

      g_clear_error (&error);
    }

  /*  query any plug-ins that have changed since we last wrote out
   *  the pluginrc file
   */
  status_callback (_("Querying new Plug-ins"), "", 0.0);

  for (list = gimp->plug_in_defs, n_plugins = 0; list; list = list->next)
    {
      PlugInDef *plug_in_def = list->data;

      if (plug_in_def->needs_query)
        n_plugins++;
    }

  if (n_plugins)
    {
      gimp->write_pluginrc = TRUE;

      for (list = gimp->plug_in_defs, nth = 0; list; list = list->next)
        {
          PlugInDef *plug_in_def = list->data;

          if (plug_in_def->needs_query)
            {
              gchar *basename;

              basename = g_filename_display_basename (plug_in_def->prog);
              status_callback (NULL, basename,
                               (gdouble) nth++ / (gdouble) n_plugins);
              g_free (basename);

              if (gimp->be_verbose)
                g_print (_("Querying plug-in: '%s'\n"),
                         gimp_filename_to_utf8 (plug_in_def->prog));

              plug_in_call_query (gimp, context, plug_in_def);
            }
        }
    }

  /* initialize the plug-ins */
  status_callback (_("Initializing Plug-ins"), "", 0.0);

  for (list = gimp->plug_in_defs, n_plugins = 0; list; list = list->next)
    {
      PlugInDef *plug_in_def = list->data;

      if (plug_in_def->has_init)
        n_plugins++;
    }

  if (n_plugins)
    {
      for (list = gimp->plug_in_defs, nth = 0; list; list = list->next)
        {
          PlugInDef *plug_in_def = list->data;

          if (plug_in_def->has_init)
            {
              gchar *basename;

              basename = g_filename_display_basename (plug_in_def->prog);
              status_callback (NULL, basename,
                               (gdouble) nth++ / (gdouble) n_plugins);
              g_free (basename);

              if (gimp->be_verbose)
                g_print (_("Initializing plug-in: '%s'\n"),
                         gimp_filename_to_utf8 (plug_in_def->prog));

              plug_in_call_init (gimp, context, plug_in_def);
            }
        }
    }

  status_callback (NULL, "", 1.0);

  /* add the procedures to gimp->plug_in_procedures */
  for (list = gimp->plug_in_defs; list; list = list->next)
    {
      PlugInDef *plug_in_def = list->data;
      GSList    *list2;

      for (list2 = plug_in_def->procedures; list2; list2 = list2->next)
        {
          plug_ins_procedure_add (gimp, list2->data);
        }
    }

  /* write the pluginrc file if necessary */
  if (gimp->write_pluginrc)
    {
      if (gimp->be_verbose)
        g_print (_("Writing '%s'\n"), gimp_filename_to_utf8 (pluginrc));

      if (! plug_in_rc_write (gimp->plug_in_defs, pluginrc, &error))
        {
          g_message ("%s", error->message);
          g_clear_error (&error);
        }

      gimp->write_pluginrc = FALSE;
    }

  g_free (pluginrc);

  /* add the plug-in procs to the procedure database */
  for (list = gimp->plug_in_procedures; list; list = list->next)
    {
      plug_ins_add_to_db (gimp, context, list->data);
    }

  /* create help_path and locale_domain lists */
  for (list = gimp->plug_in_defs; list; list = list->next)
    {
      PlugInDef *plug_in_def = list->data;

      if (plug_in_def->locale_domain_name)
        plug_in_locale_domain_add (gimp,
                                   plug_in_def->prog,
                                   plug_in_def->locale_domain_name,
                                   plug_in_def->locale_domain_path);

      if (plug_in_def->help_domain_name)
        plug_in_help_domain_add (gimp,
                                 plug_in_def->prog,
                                 plug_in_def->help_domain_name,
                                 plug_in_def->help_domain_uri);
    }

  if (! gimp->no_interface)
    {
      gimp->load_procs = g_slist_sort_with_data (gimp->load_procs,
                                                 plug_ins_file_proc_compare,
                                                 gimp);
      gimp->save_procs = g_slist_sort_with_data (gimp->save_procs,
                                                 plug_ins_file_proc_compare,
                                                 gimp);

      gimp_menus_init (gimp, gimp->plug_in_defs,
                       plug_in_standard_locale_domain ());
    }

  /* build list of automatically started extensions */
  for (list = gimp->plug_in_procedures, nth = 0; list; list = list->next, nth++)
    {
      GimpPlugInProcedure *proc = list->data;

      if (proc->prog                                         &&
          GIMP_PROCEDURE (proc)->proc_type == GIMP_EXTENSION &&
          GIMP_PROCEDURE (proc)->num_args  == 0)
        {
          extensions = g_list_prepend (extensions, proc);
        }
    }

  extensions   = g_list_reverse (extensions);
  n_extensions = g_list_length (extensions);

  /* run the available extensions */
  if (extensions)
    {
      GList *list;

      status_callback (_("Starting Extensions"), "", 0.0);

      for (list = extensions, nth = 0; list; list = g_list_next (list), nth++)
        {
          GimpPlugInProcedure *proc = list->data;
          GValueArray         *args;

          if (gimp->be_verbose)
            g_print (_("Starting extension: '%s'\n"),
                     GIMP_OBJECT (proc)->name);

          status_callback (NULL, GIMP_OBJECT (proc)->name,
                           (gdouble) nth / (gdouble) n_extensions);

          args = g_value_array_new (0);

          gimp_procedure_execute_async (GIMP_PROCEDURE (proc),
                                        gimp, context, NULL, args, -1);

          g_value_array_free (args);
        }

      g_list_free (extensions);
    }

  status_callback ("", "", 1.0);

  /* free up stuff */
  g_slist_foreach (gimp->plug_in_defs, (GFunc) plug_in_def_free, NULL);
  g_slist_free (gimp->plug_in_defs);
  gimp->plug_in_defs = NULL;
}

void
plug_ins_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  plug_in_exit (gimp);

  plug_in_menu_branch_exit (gimp);
  plug_in_locale_domain_exit (gimp);
  plug_in_help_domain_exit (gimp);
  plug_in_data_free (gimp);

  g_slist_foreach (gimp->plug_in_procedures, (GFunc) g_object_unref, NULL);
  g_slist_free (gimp->plug_in_procedures);
  gimp->plug_in_procedures = NULL;
}

void
plug_ins_procedure_add (Gimp                *gimp,
                        GimpPlugInProcedure *proc)
{
  GSList *list;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc));

  for (list = gimp->plug_in_procedures; list; list = list->next)
    {
      GimpPlugInProcedure *tmp_proc = list->data;

      if (strcmp (GIMP_OBJECT (proc)->name,
                  GIMP_OBJECT (tmp_proc)->name) == 0)
        {
          GSList *list2;

          list->data = g_object_ref (proc);

          g_printerr ("removing duplicate PDB procedure \"%s\" "
                      "registered by '%s'\n",
                      GIMP_OBJECT (tmp_proc)->name,
                      gimp_filename_to_utf8 (tmp_proc->prog));

          /* search the plugin list to see if any plugins had references to
           * the tmp_proc.
           */
          for (list2 = gimp->plug_in_defs; list2; list2 = list2->next)
            {
              PlugInDef *plug_in_def = list2->data;

              if (g_slist_find (plug_in_def->procedures, tmp_proc))
                plug_in_def_remove_procedure (plug_in_def, tmp_proc);
            }

          /* also remove it from the lists of load and save procs */
          gimp->load_procs = g_slist_remove (gimp->load_procs, tmp_proc);
          gimp->save_procs = g_slist_remove (gimp->save_procs, tmp_proc);

          g_object_unref (tmp_proc);

          return;
        }
    }

  gimp->plug_in_procedures = g_slist_prepend (gimp->plug_in_procedures,
                                              g_object_ref (proc));
}

void
plug_ins_temp_procedure_add (Gimp                   *gimp,
                             GimpTemporaryProcedure *proc)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_TEMPORARY_PROCEDURE (proc));

  if (! gimp->no_interface)
    {
      GimpPlugInProcedure *plug_in_proc = GIMP_PLUG_IN_PROCEDURE (proc);

      if (plug_in_proc->menu_label || plug_in_proc->menu_paths)
        gimp_menus_create_item (gimp, plug_in_proc, NULL);
    }

  /*  Register the procedural database entry  */
  gimp_pdb_register (gimp, GIMP_PROCEDURE (proc));

  /*  Add the definition to the global list  */
  gimp->plug_in_procedures = g_slist_prepend (gimp->plug_in_procedures,
                                              g_object_ref (proc));
}

void
plug_ins_temp_procedure_remove (Gimp                   *gimp,
                                GimpTemporaryProcedure *proc)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_TEMPORARY_PROCEDURE (proc));

  if (! gimp->no_interface)
    {
      GimpPlugInProcedure *plug_in_proc = GIMP_PLUG_IN_PROCEDURE (proc);

      if (plug_in_proc->menu_label || plug_in_proc->menu_paths)
        gimp_menus_delete_item (gimp, plug_in_proc);
    }

  /*  Remove the definition from the global list  */
  gimp->plug_in_procedures = g_slist_remove (gimp->plug_in_procedures, proc);

  /*  Unregister the procedural database entry  */
  gimp_pdb_unregister (gimp, GIMP_OBJECT (proc)->name);

  g_object_unref (proc);
}


/*  private functions  */

static void
plug_ins_add_from_file (const GimpDatafileData *file_data,
                        gpointer                data)
{
  PlugInDef  *plug_in_def;
  GSList    **plug_in_defs = data;
  GSList     *list;

  for (list = *plug_in_defs; list; list = list->next)
    {
      gchar *plug_in_name;

      plug_in_def  = list->data;
      plug_in_name = g_path_get_basename (plug_in_def->prog);

      if (g_ascii_strcasecmp (file_data->basename, plug_in_name) == 0)
        {
          g_printerr ("skipping duplicate plug-in: '%s'\n",
                      gimp_filename_to_utf8 (file_data->filename));

          g_free (plug_in_name);

          return;
        }

      g_free (plug_in_name);
    }

  plug_in_def = plug_in_def_new (file_data->filename);

  plug_in_def_set_mtime (plug_in_def, file_data->mtime);
  plug_in_def_set_needs_query (plug_in_def, TRUE);

  *plug_in_defs = g_slist_prepend (*plug_in_defs, plug_in_def);
}

static void
plug_ins_add_from_rc (Gimp      *gimp,
                      PlugInDef *plug_in_def)
{
  GSList *list;
  gchar  *basename1;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (plug_in_def != NULL);
  g_return_if_fail (plug_in_def->prog != NULL);

  if (! g_path_is_absolute (plug_in_def->prog))
    {
      g_warning ("plug_ins_def_add_from_rc: filename not absolute (skipping)");
      plug_in_def_free (plug_in_def);
      return;
    }

  basename1 = g_path_get_basename (plug_in_def->prog);

  /*  If this is a file load or save plugin, make sure we have
   *  something for one of the extensions, prefixes, or magic number.
   *  Other bits of code rely on detecting file plugins by the presence
   *  of one of these things, but Nick Lamb's alien/unknown format
   *  loader needs to be able to register no extensions, prefixes or
   *  magics. -- austin 13/Feb/99
   */
  for (list = plug_in_def->procedures; list; list = list->next)
    {
      GimpPlugInProcedure *proc = list->data;

      if (! proc->extensions &&
          ! proc->prefixes   &&
          ! proc->magics     &&
          proc->menu_paths   &&
          (! strncmp (proc->menu_paths->data, "<Load>", 6) ||
           ! strncmp (proc->menu_paths->data, "<Save>", 6)))
        {
          proc->extensions = g_strdup ("");
        }
    }

  /*  Check if the entry mentioned in pluginrc matches an executable
   *  found in the plug_in_path.
   */
  for (list = gimp->plug_in_defs; list; list = list->next)
    {
      PlugInDef *ondisk_plug_in_def = list->data;
      gchar     *basename2;

      basename2 = g_path_get_basename (ondisk_plug_in_def->prog);

      if (! strcmp (basename1, basename2))
        {
          if (! g_ascii_strcasecmp (plug_in_def->prog,
                                    ondisk_plug_in_def->prog) &&
              (plug_in_def->mtime == ondisk_plug_in_def->mtime))
            {
              /* Use pluginrc entry, deleting ondisk entry */
              list->data = plug_in_def;
              plug_in_def_free (ondisk_plug_in_def);
            }
          else
            {
              /* Use ondisk entry, deleting pluginrc entry */
              plug_in_def_free (plug_in_def);
            }

          g_free (basename2);
          g_free (basename1);

          return;
        }

      g_free (basename2);
    }

  g_free (basename1);

  gimp->write_pluginrc = TRUE;
  g_printerr ("executable not found: '%s'\n",
              gimp_filename_to_utf8 (plug_in_def->prog));
  plug_in_def_free (plug_in_def);
}

static void
plug_ins_add_to_db (Gimp                *gimp,
                    GimpContext         *context,
                    GimpPlugInProcedure *proc)
{
  gimp_pdb_register (gimp, GIMP_PROCEDURE (proc));

  if (proc->file_proc)
    {
      GValueArray *return_vals;

      if (proc->image_types)
        {
          return_vals =
            gimp_pdb_run_proc (gimp, context, NULL,
                               "gimp-register-save-handler",
                               G_TYPE_STRING, GIMP_OBJECT (proc)->name,
                               G_TYPE_STRING, proc->extensions,
                               G_TYPE_STRING, proc->prefixes,
                               G_TYPE_NONE);
        }
      else
        {
          return_vals =
            gimp_pdb_run_proc (gimp, context, NULL,
                               "gimp-register-magic-load-handler",
                               G_TYPE_STRING, GIMP_OBJECT (proc)->name,
                               G_TYPE_STRING, proc->extensions,
                               G_TYPE_STRING, proc->prefixes,
                               G_TYPE_STRING, proc->magics,
                               G_TYPE_NONE);
        }

      g_value_array_free (return_vals);
    }
}

static gint
plug_ins_file_proc_compare (gconstpointer a,
                            gconstpointer b,
                            gpointer      data)
{
  Gimp                      *gimp   = data;
  const GimpPlugInProcedure *proc_a = a;
  const GimpPlugInProcedure *proc_b = b;
  const gchar               *domain_a;
  const gchar               *domain_b;
  gchar                     *label_a;
  gchar                     *label_b;
  gint                       retval = 0;

  if (strncmp (proc_a->prog, "gimp-xcf", 8) == 0)
    return -1;

  if (strncmp (proc_b->prog, "gimp-xcf", 8) == 0)
    return 1;

  domain_a = plug_in_locale_domain (gimp, proc_a->prog, NULL);
  domain_b = plug_in_locale_domain (gimp, proc_b->prog, NULL);

  label_a = gimp_plug_in_procedure_get_label (proc_a, domain_a);
  label_b = gimp_plug_in_procedure_get_label (proc_b, domain_b);

  if (label_a && label_b)
    retval = g_utf8_collate (label_a, label_b);

  g_free (label_a);
  g_free (label_b);

  return retval;
}
