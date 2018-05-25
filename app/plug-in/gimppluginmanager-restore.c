/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
 *
 * gimppluginmanager-restore.c
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

#include "config.h"

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "plug-in-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimp-utils.h"

#include "pdb/gimppdb.h"
#include "pdb/gimppdbcontext.h"

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


static void    gimp_plug_in_manager_search            (GimpPlugInManager    *manager,
                                                       GimpInitStatusFunc    status_callback);
static void    gimp_plug_in_manager_search_directory  (GimpPlugInManager    *manager,
                                                       GFile                *directory);
static GFile * gimp_plug_in_manager_get_pluginrc      (GimpPlugInManager    *manager);
static void    gimp_plug_in_manager_read_pluginrc     (GimpPlugInManager    *manager,
                                                       GFile                *file,
                                                       GimpInitStatusFunc    status_callback);
static void    gimp_plug_in_manager_query_new         (GimpPlugInManager    *manager,
                                                       GimpContext          *context,
                                                       GimpInitStatusFunc    status_callback);
static void    gimp_plug_in_manager_init_plug_ins     (GimpPlugInManager    *manager,
                                                       GimpContext          *context,
                                                       GimpInitStatusFunc    status_callback);
static void    gimp_plug_in_manager_run_extensions    (GimpPlugInManager    *manager,
                                                       GimpContext          *context,
                                                       GimpInitStatusFunc    status_callback);
static void    gimp_plug_in_manager_bind_text_domains (GimpPlugInManager    *manager);
static void    gimp_plug_in_manager_add_from_file     (GimpPlugInManager    *manager,
                                                       GFile                *file,
                                                       guint64               mtime);
static void    gimp_plug_in_manager_add_from_rc       (GimpPlugInManager    *manager,
                                                       GimpPlugInDef        *plug_in_def);
static void    gimp_plug_in_manager_add_to_db         (GimpPlugInManager    *manager,
                                                       GimpContext          *context,
                                                       GimpPlugInProcedure  *proc);
static void    gimp_plug_in_manager_sort_file_procs   (GimpPlugInManager    *manager);
static gint    gimp_plug_in_manager_file_proc_compare (gconstpointer         a,
                                                       gconstpointer         b,
                                                       gpointer              data);



void
gimp_plug_in_manager_restore (GimpPlugInManager  *manager,
                              GimpContext        *context,
                              GimpInitStatusFunc  status_callback)
{
  Gimp   *gimp;
  GFile  *pluginrc;
  GSList *list;
  GError *error = NULL;

  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (status_callback != NULL);

  gimp = manager->gimp;

  /* need a GimpPDBContext for calling gimp_plug_in_manager_run_foo() */
  context = gimp_pdb_context_new (gimp, context, TRUE);

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
        g_print ("Writing '%s'\n", gimp_file_get_utf8_name (pluginrc));

      if (! plug_in_rc_write (manager->plug_in_defs, pluginrc, &error))
        {
          gimp_message_literal (gimp,
                                NULL, GIMP_MESSAGE_ERROR, error->message);
          g_clear_error (&error);
        }

      manager->write_pluginrc = FALSE;
    }

  g_object_unref (pluginrc);

  /* create locale and help domain lists */
  for (list = manager->plug_in_defs; list; list = list->next)
    {
      GimpPlugInDef *plug_in_def = list->data;

      if (plug_in_def->locale_domain_name)
        gimp_plug_in_manager_add_locale_domain (manager,
                                                plug_in_def->file,
                                                plug_in_def->locale_domain_name,
                                                plug_in_def->locale_domain_path);
      else
        /* set the default plug-in locale domain */
        gimp_plug_in_def_set_locale_domain (plug_in_def,
                                            gimp_plug_in_manager_get_locale_domain (manager,
                                                                                    plug_in_def->file,
                                                                                    NULL),
                                            NULL);

      if (plug_in_def->help_domain_name)
        gimp_plug_in_manager_add_help_domain (manager,
                                              plug_in_def->file,
                                              plug_in_def->help_domain_name,
                                              plug_in_def->help_domain_uri);
    }

  /* we're done with the plug-in-defs */
  g_slist_free_full (manager->plug_in_defs, (GDestroyNotify) g_object_unref);
  manager->plug_in_defs = NULL;

  /* bind plug-in text domains  */
  gimp_plug_in_manager_bind_text_domains (manager);

  /* add the plug-in procs to the procedure database */
  for (list = manager->plug_in_procedures; list; list = list->next)
    {
      gimp_plug_in_manager_add_to_db (manager, context, list->data);
    }

  /* sort the load, save and export procedures, make the raw handler list */
  gimp_plug_in_manager_sort_file_procs (manager);

  gimp_plug_in_manager_run_extensions (manager, context, status_callback);

  g_object_unref (context);
}


/* search for binaries in the plug-in directory path */
static void
gimp_plug_in_manager_search (GimpPlugInManager  *manager,
                             GimpInitStatusFunc  status_callback)
{
  const gchar *path_str;
  GList       *path;
  GList       *list;

#ifdef G_OS_WIN32
  const gchar *pathext = g_getenv ("PATHEXT");

  /*  On Windows, we need to add the known file extensions in PATHEXT. */
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
#endif /* G_OS_WIN32 */

  status_callback (_("Searching plug-ins"), "", 0.0);

  /* Give automatic tests a chance to use plug-ins from the build
   * dir
   */
  path_str = g_getenv ("GIMP_TESTING_PLUGINDIRS");
  if (! path_str)
    path_str = manager->gimp->config->plug_in_path;

  path = gimp_config_path_expand_to_files (path_str, NULL);

  for (list = path; list; list = g_list_next (list))
    {
      gimp_plug_in_manager_search_directory (manager, list->data);
    }

  g_list_free_full (path, (GDestroyNotify) g_object_unref);
}

static void
gimp_plug_in_manager_search_directory (GimpPlugInManager *manager,
                                       GFile             *directory)
{
  GFileEnumerator *enumerator;

  enumerator = g_file_enumerate_children (directory,
                                          G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                          G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN ","
                                          G_FILE_ATTRIBUTE_TIME_MODIFIED,
                                          G_FILE_QUERY_INFO_NONE,
                                          NULL, NULL);
  if (enumerator)
    {
      GFileInfo *info;

      while ((info = g_file_enumerator_next_file (enumerator, NULL, NULL)))
        {
          GFile *child;

          if (g_file_info_get_is_hidden (info))
            {
              g_object_unref (info);
              continue;
            }

          child = g_file_enumerator_get_child (enumerator, info);

          if (g_file_query_file_type (child,
                                      G_FILE_QUERY_INFO_NONE,
                                      NULL) == G_FILE_TYPE_DIRECTORY)
            {
              /* Search in subdirectory the first executable file with
               * the same name as the directory (except extension).
               * We don't search recursively, but only at a single
               * level and assume that there can be only 1 plugin
               * inside a directory.
               */
              GFileEnumerator *enumerator2;

              enumerator2 = g_file_enumerate_children (child,
                                                       G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                                       G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN ","
                                                       G_FILE_ATTRIBUTE_TIME_MODIFIED,
                                                       G_FILE_QUERY_INFO_NONE,
                                                       NULL, NULL);
              if (enumerator2)
                {
                  GFileInfo *info2;

                  while ((info2 = g_file_enumerator_next_file (enumerator2, NULL, NULL)))
                    {
                      GFile *child2;
                      gchar *file_name;
                      char  *ext;

                      if (g_file_info_get_is_hidden (info2))
                        {
                          g_object_unref (info2);
                          continue;
                        }

                      child2 = g_file_enumerator_get_child (enumerator2, info2);
                      file_name = g_strdup (g_file_info_get_name (info2));

                      ext = strrchr (file_name, '.');
                      if (ext)
                        *ext = '\0';

                      if (g_strcmp0 (file_name, g_file_info_get_name (info)) == 0 &&
                          gimp_file_is_executable (child2))
                        {
                          guint64 mtime;

                          mtime = g_file_info_get_attribute_uint64 (info2,
                                                                    G_FILE_ATTRIBUTE_TIME_MODIFIED);

                          gimp_plug_in_manager_add_from_file (manager, child2, mtime);

                          g_free (file_name);
                          g_object_unref (child2);
                          g_object_unref (info2);
                          break;
                        }

                      g_free (file_name);
                      g_object_unref (child2);
                      g_object_unref (info2);
                    }

                  g_object_unref (enumerator2);
                }
            }
          else if (gimp_file_is_executable (child))
            {
              g_printerr (_("Skipping potential plug-in '%s': "
                            "plug-ins must be installed in subdirectories.\n"),
                          g_file_peek_path (child));
            }
          else
            {
              g_printerr (_("Skipping unknown file '%s' in plug-in directory.\n"),
                          g_file_peek_path (child));
            }

          g_object_unref (child);
          g_object_unref (info);
        }

      g_object_unref (enumerator);
    }
}

static GFile *
gimp_plug_in_manager_get_pluginrc (GimpPlugInManager *manager)
{
  Gimp  *gimp = manager->gimp;
  GFile *pluginrc;

  if (gimp->config->plug_in_rc_path)
    {
      gchar *path = gimp_config_path_expand (gimp->config->plug_in_rc_path,
                                             TRUE, NULL);

      if (g_path_is_absolute (path))
        pluginrc = g_file_new_for_path (path);
      else
        pluginrc = gimp_directory_file (path, NULL);

      g_free (path);
    }
  else
    {
      pluginrc = gimp_directory_file ("pluginrc", NULL);
    }

  return pluginrc;
}

/* read the pluginrc file for cached data */
static void
gimp_plug_in_manager_read_pluginrc (GimpPlugInManager  *manager,
                                    GFile              *pluginrc,
                                    GimpInitStatusFunc  status_callback)
{
  GSList *rc_defs;
  GError *error = NULL;

  status_callback (_("Resource configuration"),
                   gimp_file_get_utf8_name (pluginrc), 0.0);

  if (manager->gimp->be_verbose)
    g_print ("Parsing '%s'\n", gimp_file_get_utf8_name (pluginrc));

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
        gimp_message_literal (manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                              error->message);

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

              basename =
                g_path_get_basename (gimp_file_get_utf8_name (plug_in_def->file));
              status_callback (NULL, basename,
                               (gdouble) nth++ / (gdouble) n_plugins);
              g_free (basename);

              if (manager->gimp->be_verbose)
                g_print ("Querying plug-in: '%s'\n",
                         gimp_file_get_utf8_name (plug_in_def->file));

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

              basename =
                g_path_get_basename (gimp_file_get_utf8_name (plug_in_def->file));
              status_callback (NULL, basename,
                               (gdouble) nth++ / (gdouble) n_plugins);
              g_free (basename);

              if (manager->gimp->be_verbose)
                g_print ("Initializing plug-in: '%s'\n",
                         gimp_file_get_utf8_name (plug_in_def->file));

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

      if (proc->file                                         &&
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
          GimpValueArray      *args;
          GError              *error = NULL;

          if (gimp->be_verbose)
            g_print ("Starting extension: '%s'\n", gimp_object_get_name (proc));

          status_callback (NULL, gimp_object_get_name (proc),
                           (gdouble) nth / (gdouble) n_extensions);

          args = gimp_value_array_new (0);

          gimp_procedure_execute_async (GIMP_PROCEDURE (proc),
                                        gimp, context, NULL,
                                        args, NULL, &error);

          gimp_value_array_unref (args);

          if (error)
            {
              gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR,
                                    error->message);
              g_clear_error (&error);
            }
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

/**
 * gimp_plug_in_manager_ignore_plugin_basename:
 * @basename: Basename to test with
 *
 * Checks the environment variable
 * GIMP_TESTING_PLUGINDIRS_BASENAME_IGNORES for file basenames.
 *
 * Returns: %TRUE if @basename was in GIMP_TESTING_PLUGINDIRS_BASENAME_IGNORES
 **/
static gboolean
gimp_plug_in_manager_ignore_plugin_basename (const gchar *plugin_basename)
{
  const gchar *ignore_basenames_string;
  GList       *ignore_basenames;
  GList       *iter;
  gboolean     ignore = FALSE;

  ignore_basenames_string = g_getenv ("GIMP_TESTING_PLUGINDIRS_BASENAME_IGNORES");
  ignore_basenames        = gimp_path_parse (ignore_basenames_string,
                                             256 /*max_paths*/,
                                             FALSE /*check*/,
                                             NULL /*check_failed*/);

  for (iter = ignore_basenames; iter; iter = g_list_next (iter))
    {
      const gchar *ignore_basename = iter->data;

      if (g_ascii_strcasecmp (ignore_basename, plugin_basename) == 0)
        {
          ignore = TRUE;
          break;
        }
    }

  gimp_path_free (ignore_basenames);

  return ignore;
}

static void
gimp_plug_in_manager_add_from_file (GimpPlugInManager *manager,
                                    GFile             *file,
                                    guint64            mtime)
{
  GimpPlugInDef *plug_in_def;
  GSList        *list;
  gchar         *filename;
  gchar         *basename;

  filename = g_file_get_path (file);
  basename = g_path_get_basename (filename);
  g_free (filename);

  /* When we scan build dirs for plug-ins, there will be some
   * executable files that are not plug-ins that we want to ignore,
   * for example plug-ins/common/mkgen.pl if
   * GIMP_TESTING_PLUGINDIRS=plug-ins/common
   */
  if (gimp_plug_in_manager_ignore_plugin_basename (basename))
    {
      g_free (basename);
      return;
    }

  for (list = manager->plug_in_defs; list; list = list->next)
    {
      gchar *path;
      gchar *plug_in_name;

      plug_in_def = list->data;

      path = g_file_get_path (plug_in_def->file);
      plug_in_name = g_path_get_basename (path);
      g_free (path);

      if (g_ascii_strcasecmp (basename, plug_in_name) == 0)
        {
          g_printerr ("Skipping duplicate plug-in: '%s'\n",
                      gimp_file_get_utf8_name (file));

          g_free (plug_in_name);
          g_free (basename);

          return;
        }

      g_free (plug_in_name);
    }

  g_free (basename);

  plug_in_def = gimp_plug_in_def_new (file);

  gimp_plug_in_def_set_mtime (plug_in_def, mtime);
  gimp_plug_in_def_set_needs_query (plug_in_def, TRUE);

  manager->plug_in_defs = g_slist_prepend (manager->plug_in_defs, plug_in_def);
}

static void
gimp_plug_in_manager_add_from_rc (GimpPlugInManager *manager,
                                  GimpPlugInDef     *plug_in_def)
{
  GSList *list;
  gchar  *path1;
  gchar  *basename1;

  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (plug_in_def != NULL);
  g_return_if_fail (plug_in_def->file != NULL);

  path1 = g_file_get_path (plug_in_def->file);

  if (! g_path_is_absolute (path1))
    {
      g_warning ("plug_ins_def_add_from_rc: filename not absolute (skipping)");
      g_object_unref (plug_in_def);
      g_free (path1);
      return;
    }

  basename1 = g_path_get_basename (path1);

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
      gchar         *path2;
      gchar         *basename2;

      path2 = g_file_get_path (ondisk_plug_in_def->file);

      basename2 = g_path_get_basename (path2);

      g_free (path2);

      if (! strcmp (basename1, basename2))
        {
          if (g_file_equal (plug_in_def->file,
                            ondisk_plug_in_def->file) &&
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
          g_free (path1);

          return;
        }

      g_free (basename2);
    }

  g_free (basename1);
  g_free (path1);

  manager->write_pluginrc = TRUE;

  if (manager->gimp->be_verbose)
    {
      g_printerr ("pluginrc lists '%s', but it wasn't found\n",
                  gimp_file_get_utf8_name (plug_in_def->file));
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
      GimpValueArray *return_vals;
      GError         *error = NULL;

      if (proc->image_types)
        {
          return_vals =
            gimp_pdb_execute_procedure_by_name (manager->gimp->pdb,
                                                context, NULL, &error,
                                                "gimp-register-save-handler",
                                                G_TYPE_STRING, gimp_object_get_name (proc),
                                                G_TYPE_STRING, proc->extensions,
                                                G_TYPE_STRING, proc->prefixes,
                                                G_TYPE_NONE);
        }
      else
        {
          return_vals =
            gimp_pdb_execute_procedure_by_name (manager->gimp->pdb,
                                                context, NULL, &error,
                                                "gimp-register-magic-load-handler",
                                                G_TYPE_STRING, gimp_object_get_name (proc),
                                                G_TYPE_STRING, proc->extensions,
                                                G_TYPE_STRING, proc->prefixes,
                                                G_TYPE_STRING, proc->magics,
                                                G_TYPE_NONE);
        }

      gimp_value_array_unref (return_vals);

      if (error)
        {
          gimp_message_literal (manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                                error->message);
          g_error_free (error);
        }
    }
}

static void
gimp_plug_in_manager_sort_file_procs (GimpPlugInManager *manager)
{
  GimpCoreConfig *config         = manager->gimp->config;
  GFile          *config_plug_in = NULL;
  GFile          *raw_plug_in    = NULL;
  GSList         *list;

  manager->load_procs =
    g_slist_sort_with_data (manager->load_procs,
                            gimp_plug_in_manager_file_proc_compare, manager);
  manager->save_procs =
    g_slist_sort_with_data (manager->save_procs,
                            gimp_plug_in_manager_file_proc_compare, manager);
  manager->export_procs =
    g_slist_sort_with_data (manager->export_procs,
                            gimp_plug_in_manager_file_proc_compare, manager);


  if (config->import_raw_plug_in)
    {
      /* remember the configured raw loader, unless it's the placeholder */
      if (! strstr (config->import_raw_plug_in, "file-raw-placeholder"))
        config_plug_in =
          gimp_file_new_for_config_path (config->import_raw_plug_in,
                                         NULL);
    }

  /* make the list of raw loaders, and remember the one configured in
   * config if found
   */
  for (list = manager->load_procs; list; list = g_slist_next (list))
    {
      GimpPlugInProcedure *file_proc = list->data;

      if (file_proc->handles_raw)
        {
          GFile *file;

          manager->raw_load_procs = g_slist_append (manager->raw_load_procs,
                                                    file_proc);

          file = gimp_plug_in_procedure_get_file (file_proc);

          if (! raw_plug_in  &&
              config_plug_in &&
              g_file_equal (config_plug_in, file))
            {
              raw_plug_in = file;
            }
        }
    }

  if (config_plug_in)
    g_object_unref (config_plug_in);

  /* if no raw loader was configured, or the configured raw loader
   * wasn't found, default to the first loader that is not the
   * placeholder, if any
   */
  if (! raw_plug_in && manager->raw_load_procs)
    {
      gchar *path;

      for (list = manager->raw_load_procs; list; list = g_slist_next (list))
        {
          GimpPlugInProcedure *file_proc = list->data;

          raw_plug_in = gimp_plug_in_procedure_get_file (file_proc);

          path = gimp_file_get_config_path (raw_plug_in, NULL);

          if (! strstr (path, "file-raw-placeholder"))
            break;

          g_free (path);
          path = NULL;

          raw_plug_in = NULL;
        }

      if (! raw_plug_in)
        {
          raw_plug_in =
            gimp_plug_in_procedure_get_file (manager->raw_load_procs->data);

          path = gimp_file_get_config_path (raw_plug_in, NULL);
        }

      g_object_set (config,
                    "import-raw-plug-in", path,
                    NULL);

      g_free (path);
    }

  /* finally, remove all raw loaders except the configured one from
   * the list of load_procs
   */
  list = manager->load_procs;
  while (list)
    {
      GimpPlugInProcedure *file_proc = list->data;

      list = g_slist_next (list);

      if (file_proc->handles_raw &&
          ! g_file_equal (gimp_plug_in_procedure_get_file (file_proc),
                          raw_plug_in))
        {
          manager->load_procs = g_slist_remove (manager->load_procs,
                                                file_proc);
        }
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

  if (g_str_has_prefix (gimp_file_get_utf8_name (proc_a->file), "gimp-xcf"))
    return -1;

  if (g_str_has_prefix (gimp_file_get_utf8_name (proc_b->file), "gimp-xcf"))
    return 1;

  label_a = gimp_procedure_get_label (GIMP_PROCEDURE (proc_a));
  label_b = gimp_procedure_get_label (GIMP_PROCEDURE (proc_b));

  if (label_a && label_b)
    retval = g_utf8_collate (label_a, label_b);

  return retval;
}
