/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
 *
 * gimppluginmanager-restore.c
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
#include "libgimpconfig/gimpconfig.h"

#include "plug-in-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"

#include "pdb/gimppdb.h"

#include "gimpinterpreterdb.h"
#include "gimpplugindef.h"
#include "gimppluginmanager.h"
#define __YES_I_NEED_GIMP_PLUG_IN_MANAGER_CALL__
#include "gimppluginmanager-call.h"
#include "gimppluginmanager-help-domain.h"
#include "gimppluginmanager-locale-domain.h"
#include "gimppluginmanager-restore.h"
#include "gimppluginprocedure.h"
#include "plug-in-rc.h"

#include "gimp-intl.h"


static void    gimp_plug_in_manager_search            (GimpPlugInManager      *manager,
                                                       GimpInitStatusFunc      status_callback);
static gchar * gimp_plug_in_manager_get_pluginrc      (GimpPlugInManager      *manager);
static void    gimp_plug_in_manager_read_pluginrc     (GimpPlugInManager      *manager,
                                                       const gchar            *pluginrc,
                                                       GimpInitStatusFunc      status_callback);
static void    gimp_plug_in_manager_query_new         (GimpPlugInManager      *manager,
                                                       GimpContext            *context,
                                                       GimpInitStatusFunc      status_callback);
static void    gimp_plug_in_manager_init_plug_ins     (GimpPlugInManager      *manager,
                                                       GimpContext            *context,
                                                       GimpInitStatusFunc      status_callback);
static void    gimp_plug_in_manager_run_extensions    (GimpPlugInManager      *manager,
                                                       GimpContext            *context,
                                                       GimpInitStatusFunc      status_callback);
static void    gimp_plug_in_manager_bind_text_domains (GimpPlugInManager      *manager);
static void    gimp_plug_in_manager_add_from_file     (const GimpDatafileData *file_data,
                                                       gpointer                data);
static void    gimp_plug_in_manager_add_from_rc       (GimpPlugInManager      *manager,
                                                       GimpPlugInDef          *plug_in_def);
static void     gimp_plug_in_manager_add_to_db         (GimpPlugInManager      *manager,
                                                        GimpContext            *context,
                                                        GimpPlugInProcedure    *proc);
static gint     gimp_plug_in_manager_file_proc_compare (gconstpointer           a,
                                                        gconstpointer           b,
                                                        gpointer                data);



void
gimp_plug_in_manager_restore (GimpPlugInManager  *manager,
                              GimpContext        *context,
                              GimpInitStatusFunc  status_callback)
{
  Gimp   *gimp;
  gchar  *pluginrc;
  GSList *list;
  GError *error = NULL;

  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (status_callback != NULL);

  gimp = manager->gimp;

  /* search for binaries in the plug-in directory path */
  gimp_plug_in_manager_search (manager, status_callback);

  /* read the pluginrc file for cached data */
  pluginrc = gimp_plug_in_manager_get_pluginrc (manager);

  gimp_plug_in_manager_read_pluginrc (manager, pluginrc, status_callback);

  /* query any plug-ins that changed since we last wrote out pluginrc */
  gimp_plug_in_manager_query_new (manager, context, status_callback);

  /* initialize the plug-ins */
  gimp_plug_in_manager_init_plug_ins (manager, context, status_callback);

  /* add the procedures to manager->plug_in_procedures */
  for (list = manager->plug_in_defs; list; list = list->next)
    {
      GimpPlugInDef *plug_in_def = list->data;
      GSList        *list2;

      for (list2 = plug_in_def->procedures; list2; list2 = list2->next)
        {
          gimp_plug_in_manager_add_procedure (manager, list2->data);
        }
    }

  /* write the pluginrc file if necessary */
  if (manager->write_pluginrc)
    {
      if (gimp->be_verbose)
        g_print ("Writing '%s'\n", gimp_filename_to_utf8 (pluginrc));

      if (! plug_in_rc_write (manager->plug_in_defs, pluginrc, &error))
        {
          gimp_message (gimp, NULL, GIMP_MESSAGE_ERROR, "%s", error->message);
          g_clear_error (&error);
        }

      manager->write_pluginrc = FALSE;
    }

  g_free (pluginrc);

  /* create locale and help domain lists */
  for (list = manager->plug_in_defs; list; list = list->next)
    {
      GimpPlugInDef *plug_in_def = list->data;

      if (plug_in_def->locale_domain_name)
        gimp_plug_in_manager_add_locale_domain (manager,
                                                plug_in_def->prog,
                                                plug_in_def->locale_domain_name,
                                                plug_in_def->locale_domain_path);
      else
        /* set the default plug-in locale domain */
        gimp_plug_in_def_set_locale_domain (plug_in_def,
                                            gimp_plug_in_manager_get_locale_domain (manager,
                                                                                    plug_in_def->prog,
                                                                                    NULL),
                                            NULL);

      if (plug_in_def->help_domain_name)
        gimp_plug_in_manager_add_help_domain (manager,
                                              plug_in_def->prog,
                                              plug_in_def->help_domain_name,
                                              plug_in_def->help_domain_uri);
    }

  /* we're done with the plug-in-defs */
  g_slist_foreach (manager->plug_in_defs, (GFunc) g_object_unref, NULL);
  g_slist_free (manager->plug_in_defs);
  manager->plug_in_defs = NULL;

  /* bind plug-in text domains  */
  gimp_plug_in_manager_bind_text_domains (manager);

  /* add the plug-in procs to the procedure database */
  for (list = manager->plug_in_procedures; list; list = list->next)
    {
      gimp_plug_in_manager_add_to_db (manager, context, list->data);
    }

  /* sort the load and save procedures  */
  manager->load_procs =
    g_slist_sort_with_data (manager->load_procs,
                            gimp_plug_in_manager_file_proc_compare, manager);
  manager->save_procs =
    g_slist_sort_with_data (manager->save_procs,
                            gimp_plug_in_manager_file_proc_compare, manager);

  gimp_plug_in_manager_run_extensions (manager, context, status_callback);
}


/* search for binaries in the plug-in directory path */
static void
gimp_plug_in_manager_search (GimpPlugInManager  *manager,
                             GimpInitStatusFunc  status_callback)
{
  gchar       *path;
  const gchar *pathext = g_getenv ("PATHEXT");

  /*  If PATHEXT is set, we are likely on Windows and need to add
   *  the known file extensions.
   */
  if (pathext)
    {
      gchar *exts;

      exts = gimp_interpreter_db_get_extensions (manager->interpreter_db);

      if (exts)
        {
          gchar *value;

          value = g_strconcat (pathext, G_SEARCHPATH_SEPARATOR_S, exts, NULL);

          g_setenv ("PATHEXT", value, TRUE);

          g_free (value);
          g_free (exts);
        }
    }

  status_callback (_("Searching Plug-Ins"), "", 0.0);

  path = gimp_config_path_expand (manager->gimp->config->plug_in_path,
                                  TRUE, NULL);

  gimp_datafiles_read_directories (path,
                                   G_FILE_TEST_IS_EXECUTABLE,
                                   gimp_plug_in_manager_add_from_file,
                                   manager);

  g_free (path);
}

static gchar *
gimp_plug_in_manager_get_pluginrc (GimpPlugInManager *manager)
{
  Gimp  *gimp = manager->gimp;
  gchar *pluginrc;

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

  return pluginrc;
}

/* read the pluginrc file for cached data */
static void
gimp_plug_in_manager_read_pluginrc (GimpPlugInManager  *manager,
                                    const gchar        *pluginrc,
                                    GimpInitStatusFunc  status_callback)
{
  GSList *rc_defs;
  GError *error = NULL;

  status_callback (_("Resource configuration"),
                   gimp_filename_to_utf8 (pluginrc), 0.0);

  if (manager->gimp->be_verbose)
    g_print ("Parsing '%s'\n", gimp_filename_to_utf8 (pluginrc));

  rc_defs = plug_in_rc_parse (manager->gimp, pluginrc, &error);

  if (rc_defs)
    {
      GSList *list;

      for (list = rc_defs; list; list = g_slist_next (list))
        gimp_plug_in_manager_add_from_rc (manager, list->data); /* consumes list->data */

      g_slist_free (rc_defs);
    }
  else if (error)
    {
      if (error->code != GIMP_CONFIG_ERROR_OPEN_ENOENT)
        gimp_message (manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                      "%s", error->message);

      g_clear_error (&error);
    }
}

/* query any plug-ins that changed since we last wrote out pluginrc */
static void
gimp_plug_in_manager_query_new (GimpPlugInManager  *manager,
                                GimpContext        *context,
                                GimpInitStatusFunc  status_callback)
{
  GSList *list;
  gint    n_plugins;

  status_callback (_("Querying new Plug-ins"), "", 0.0);

  for (list = manager->plug_in_defs, n_plugins = 0; list; list = list->next)
    {
      GimpPlugInDef *plug_in_def = list->data;

      if (plug_in_def->needs_query)
        n_plugins++;
    }

  if (n_plugins)
    {
      gint nth;

      manager->write_pluginrc = TRUE;

      for (list = manager->plug_in_defs, nth = 0; list; list = list->next)
        {
          GimpPlugInDef *plug_in_def = list->data;

          if (plug_in_def->needs_query)
            {
              gchar *basename;

              basename = g_filename_display_basename (plug_in_def->prog);
              status_callback (NULL, basename,
                               (gdouble) nth++ / (gdouble) n_plugins);
              g_free (basename);

              if (manager->gimp->be_verbose)
                g_print ("Querying plug-in: '%s'\n",
                         gimp_filename_to_utf8 (plug_in_def->prog));

              gimp_plug_in_manager_call_query (manager, context, plug_in_def);
            }
        }
    }

  status_callback (NULL, "", 1.0);
}

/* initialize the plug-ins */
static void
gimp_plug_in_manager_init_plug_ins (GimpPlugInManager  *manager,
                                    GimpContext        *context,
                                    GimpInitStatusFunc  status_callback)
{
  GSList *list;
  gint    n_plugins;

  status_callback (_("Initializing Plug-ins"), "", 0.0);

  for (list = manager->plug_in_defs, n_plugins = 0; list; list = list->next)
    {
      GimpPlugInDef *plug_in_def = list->data;

      if (plug_in_def->has_init)
        n_plugins++;
    }

  if (n_plugins)
    {
      gint nth;

      for (list = manager->plug_in_defs, nth = 0; list; list = list->next)
        {
          GimpPlugInDef *plug_in_def = list->data;

          if (plug_in_def->has_init)
            {
              gchar *basename;

              basename = g_filename_display_basename (plug_in_def->prog);
              status_callback (NULL, basename,
                               (gdouble) nth++ / (gdouble) n_plugins);
              g_free (basename);

              if (manager->gimp->be_verbose)
                g_print ("Initializing plug-in: '%s'\n",
                         gimp_filename_to_utf8 (plug_in_def->prog));

              gimp_plug_in_manager_call_init (manager, context, plug_in_def);
            }
        }
    }

  status_callback (NULL, "", 1.0);
}

/* run automatically started extensions */
static void
gimp_plug_in_manager_run_extensions (GimpPlugInManager  *manager,
                                     GimpContext        *context,
                                     GimpInitStatusFunc  status_callback)
{
  Gimp   *gimp = manager->gimp;
  GSList *list;
  GList  *extensions = NULL;
  gint    n_extensions;

  /* build list of automatically started extensions */
  for (list = manager->plug_in_procedures; list; list = list->next)
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
      gint   nth;

      status_callback (_("Starting Extensions"), "", 0.0);

      for (list = extensions, nth = 0; list; list = g_list_next (list), nth++)
        {
          GimpPlugInProcedure *proc = list->data;
          GValueArray         *args;

          if (gimp->be_verbose)
            g_print ("Starting extension: '%s'\n", GIMP_OBJECT (proc)->name);

          status_callback (NULL, GIMP_OBJECT (proc)->name,
                           (gdouble) nth / (gdouble) n_extensions);

          args = g_value_array_new (0);

          gimp_procedure_execute_async (GIMP_PROCEDURE (proc),
                                        gimp, context, NULL, args, NULL);

          g_value_array_free (args);
        }

      g_list_free (extensions);

      status_callback (NULL, "", 1.0);
    }
}

static void
gimp_plug_in_manager_bind_text_domains (GimpPlugInManager *manager)
{
  gchar **locale_domains;
  gchar **locale_paths;
  gint    n_domains;
  gint    i;

  n_domains = gimp_plug_in_manager_get_locale_domains (manager,
                                                       &locale_domains,
                                                       &locale_paths);

  for (i = 0; i < n_domains; i++)
    {
      bindtextdomain (locale_domains[i], locale_paths[i]);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
      bind_textdomain_codeset (locale_domains[i], "UTF-8");
#endif
    }

  g_strfreev (locale_domains);
  g_strfreev (locale_paths);
}

static void
gimp_plug_in_manager_add_from_file (const GimpDatafileData *file_data,
                                    gpointer                data)
{
  GimpPlugInManager *manager = data;
  GimpPlugInDef     *plug_in_def;
  GSList            *list;

  for (list = manager->plug_in_defs; list; list = list->next)
    {
      gchar *plug_in_name;

      plug_in_def  = list->data;
      plug_in_name = g_path_get_basename (plug_in_def->prog);

      if (g_ascii_strcasecmp (file_data->basename, plug_in_name) == 0)
        {
          g_printerr ("Skipping duplicate plug-in: '%s'\n",
                      gimp_filename_to_utf8 (file_data->filename));

          g_free (plug_in_name);

          return;
        }

      g_free (plug_in_name);
    }

  plug_in_def = gimp_plug_in_def_new (file_data->filename);

  gimp_plug_in_def_set_mtime (plug_in_def, file_data->mtime);
  gimp_plug_in_def_set_needs_query (plug_in_def, TRUE);

  manager->plug_in_defs = g_slist_prepend (manager->plug_in_defs, plug_in_def);
}

static void
gimp_plug_in_manager_add_from_rc (GimpPlugInManager *manager,
                                  GimpPlugInDef     *plug_in_def)
{
  GSList *list;
  gchar  *basename1;

  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (plug_in_def != NULL);
  g_return_if_fail (plug_in_def->prog != NULL);

  if (! g_path_is_absolute (plug_in_def->prog))
    {
      g_warning ("plug_ins_def_add_from_rc: filename not absolute (skipping)");
      g_object_unref (plug_in_def);
      return;
    }

  basename1 = g_path_get_basename (plug_in_def->prog);

  /*  If this is a file load or save plugin, make sure we have
   *  something for one of the extensions, prefixes, or magic number.
   *  Other bits of code rely on detecting file plugins by the
   *  presence of one of these things, but the raw plug-in needs to be
   *  able to register no extensions, prefixes or magics.
   */
  for (list = plug_in_def->procedures; list; list = list->next)
    {
      GimpPlugInProcedure *proc = list->data;

      if (! proc->extensions &&
          ! proc->prefixes   &&
          ! proc->magics     &&
          proc->menu_paths   &&
          (g_str_has_prefix (proc->menu_paths->data, "<Load>") ||
           g_str_has_prefix (proc->menu_paths->data, "<Save>")))
        {
          proc->extensions = g_strdup ("");
        }
    }

  /*  Check if the entry mentioned in pluginrc matches an executable
   *  found in the plug_in_path.
   */
  for (list = manager->plug_in_defs; list; list = list->next)
    {
      GimpPlugInDef *ondisk_plug_in_def = list->data;
      gchar         *basename2;

      basename2 = g_path_get_basename (ondisk_plug_in_def->prog);

      if (! strcmp (basename1, basename2))
        {
          if (! g_ascii_strcasecmp (plug_in_def->prog,
                                    ondisk_plug_in_def->prog) &&
              (plug_in_def->mtime == ondisk_plug_in_def->mtime))
            {
              /* Use pluginrc entry, deleting on-disk entry */
              list->data = plug_in_def;
              g_object_unref (ondisk_plug_in_def);
            }
          else
            {
              /* Use on-disk entry, deleting pluginrc entry */
              g_object_unref (plug_in_def);
            }

          g_free (basename2);
          g_free (basename1);

          return;
        }

      g_free (basename2);
    }

  g_free (basename1);

  manager->write_pluginrc = TRUE;

  if (manager->gimp->be_verbose)
    {
      g_printerr ("pluginrc lists '%s', but it wasn't found\n",
                  gimp_filename_to_utf8 (plug_in_def->prog));
    }

  g_object_unref (plug_in_def);
}


static void
gimp_plug_in_manager_add_to_db (GimpPlugInManager   *manager,
                                GimpContext         *context,
                                GimpPlugInProcedure *proc)
{
  gimp_pdb_register_procedure (manager->gimp->pdb, GIMP_PROCEDURE (proc));

  if (proc->file_proc)
    {
      GValueArray *return_vals;

      if (proc->image_types)
        {
          return_vals =
            gimp_pdb_execute_procedure_by_name (manager->gimp->pdb,
                                                context, NULL,
                                                "gimp-register-save-handler",
                                                G_TYPE_STRING, GIMP_OBJECT (proc)->name,
                                                G_TYPE_STRING, proc->extensions,
                                                G_TYPE_STRING, proc->prefixes,
                                                G_TYPE_NONE);
        }
      else
        {
          return_vals =
            gimp_pdb_execute_procedure_by_name (manager->gimp->pdb,
                                                context, NULL,
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
gimp_plug_in_manager_file_proc_compare (gconstpointer a,
                                        gconstpointer b,
                                        gpointer      data)
{
  GimpPlugInProcedure *proc_a = GIMP_PLUG_IN_PROCEDURE (a);
  GimpPlugInProcedure *proc_b = GIMP_PLUG_IN_PROCEDURE (b);
  const gchar         *label_a;
  const gchar         *label_b;
  gint                 retval = 0;

  if (g_str_has_prefix (proc_a->prog, "gimp-xcf"))
    return -1;

  if (g_str_has_prefix (proc_b->prog, "gimp-xcf"))
    return 1;

  label_a = gimp_plug_in_procedure_get_label (proc_a);
  label_b = gimp_plug_in_procedure_get_label (proc_b);

  if (label_a && label_b)
    retval = g_utf8_collate (label_a, label_b);

  return retval;
}
