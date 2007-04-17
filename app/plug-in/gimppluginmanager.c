/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
 *
 * gimppluginmanager.c
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
#include "core/gimp-utils.h"
#include "core/gimpcontext.h"
#include "core/gimpmarshal.h"

#include "pdb/gimppdb.h"

#include "gimpenvirontable.h"
#include "gimpinterpreterdb.h"
#include "gimpplugin.h"
#include "gimpplugindebug.h"
#include "gimpplugindef.h"
#include "gimppluginmanager.h"
#define __YES_I_NEED_GIMP_PLUG_IN_MANAGER_CALL__
#include "gimppluginmanager-call.h"
#include "gimppluginmanager-data.h"
#include "gimppluginmanager-help-domain.h"
#include "gimppluginmanager-history.h"
#include "gimppluginmanager-locale-domain.h"
#include "gimppluginmanager-menu-branch.h"
#include "gimppluginshm.h"
#include "gimptemporaryprocedure.h"
#include "plug-in-rc.h"

#include "gimp-intl.h"


enum
{
  PLUG_IN_OPENED,
  PLUG_IN_CLOSED,
  MENU_BRANCH_ADDED,
  HISTORY_CHANGED,
  LAST_SIGNAL
};


static void     gimp_plug_in_manager_dispose     (GObject    *object);
static void     gimp_plug_in_manager_finalize    (GObject    *object);

static gint64   gimp_plug_in_manager_get_memsize (GimpObject *object,
                                                  gint64     *gui_size);

static void     gimp_plug_in_manager_add_from_file     (const GimpDatafileData *file_data,
                                                        gpointer                data);
static void     gimp_plug_in_manager_add_from_rc       (GimpPlugInManager      *manager,
                                                        GimpPlugInDef          *plug_in_def);
static void     gimp_plug_in_manager_add_to_db         (GimpPlugInManager      *manager,
                                                        GimpContext            *context,
                                                        GimpPlugInProcedure    *proc);
static gint     gimp_plug_in_manager_file_proc_compare (gconstpointer           a,
                                                        gconstpointer           b,
                                                        gpointer                data);


G_DEFINE_TYPE (GimpPlugInManager, gimp_plug_in_manager, GIMP_TYPE_OBJECT)

#define parent_class gimp_plug_in_manager_parent_class

static guint manager_signals[LAST_SIGNAL] = { 0, };


static void
gimp_plug_in_manager_class_init (GimpPlugInManagerClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);

  manager_signals[PLUG_IN_OPENED] =
    g_signal_new ("plug-in-opened",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpPlugInManagerClass,
                                   plug_in_opened),
                  NULL, NULL,
                  gimp_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_PLUG_IN);

  manager_signals[PLUG_IN_CLOSED] =
    g_signal_new ("plug-in-closed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpPlugInManagerClass,
                                   plug_in_closed),
                  NULL, NULL,
                  gimp_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_PLUG_IN);

  manager_signals[MENU_BRANCH_ADDED] =
    g_signal_new ("menu-branch-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpPlugInManagerClass,
                                   menu_branch_added),
                  NULL, NULL,
                  gimp_marshal_VOID__STRING_STRING_STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING,
                  G_TYPE_STRING,
                  G_TYPE_STRING);

  manager_signals[HISTORY_CHANGED] =
    g_signal_new ("history-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpPlugInManagerClass,
                                   history_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->dispose          = gimp_plug_in_manager_dispose;
  object_class->finalize         = gimp_plug_in_manager_finalize;

  gimp_object_class->get_memsize = gimp_plug_in_manager_get_memsize;
}

static void
gimp_plug_in_manager_init (GimpPlugInManager *manager)
{
  manager->gimp               = NULL;

  manager->plug_in_defs       = NULL;
  manager->write_pluginrc     = FALSE;

  manager->plug_in_procedures = NULL;
  manager->load_procs         = NULL;
  manager->save_procs         = NULL;

  manager->current_plug_in    = NULL;
  manager->open_plug_ins      = NULL;
  manager->plug_in_stack      = NULL;
  manager->history            = NULL;

  manager->shm                = NULL;
  manager->interpreter_db     = gimp_interpreter_db_new ();
  manager->environ_table      = gimp_environ_table_new ();
  manager->debug              = NULL;
  manager->data_list          = NULL;
}

static void
gimp_plug_in_manager_dispose (GObject *object)
{
  GimpPlugInManager *manager = GIMP_PLUG_IN_MANAGER (object);

  gimp_plug_in_manager_history_clear (manager);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_plug_in_manager_finalize (GObject *object)
{
  GimpPlugInManager *manager = GIMP_PLUG_IN_MANAGER (object);

  if (manager->load_procs)
    {
      g_slist_free (manager->load_procs);
      manager->load_procs = NULL;
    }

  if (manager->save_procs)
    {
      g_slist_free (manager->save_procs);
      manager->save_procs = NULL;
    }

  if (manager->plug_in_procedures)
    {
      g_slist_foreach (manager->plug_in_procedures,
                       (GFunc) g_object_unref, NULL);
      g_slist_free (manager->plug_in_procedures);
      manager->plug_in_procedures = NULL;
    }

  if (manager->plug_in_defs)
    {
      g_slist_foreach (manager->plug_in_defs, (GFunc) g_object_unref, NULL);
      g_slist_free (manager->plug_in_defs);
      manager->plug_in_defs = NULL;
    }

  if (manager->shm)
    {
      gimp_plug_in_shm_free (manager->shm);
      manager->shm = NULL;
    }

  if (manager->environ_table)
    {
      g_object_unref (manager->environ_table);
      manager->environ_table = NULL;
    }

  if (manager->interpreter_db)
    {
      g_object_unref (manager->interpreter_db);
      manager->interpreter_db = NULL;
    }

  if (manager->debug)
    {
      gimp_plug_in_debug_free (manager->debug);
      manager->debug = NULL;
    }

  gimp_plug_in_manager_menu_branch_exit (manager);
  gimp_plug_in_manager_locale_domain_exit (manager);
  gimp_plug_in_manager_help_domain_exit (manager);
  gimp_plug_in_manager_data_free (manager);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_plug_in_manager_get_memsize (GimpObject *object,
                                  gint64     *gui_size)
{
  GimpPlugInManager *manager = GIMP_PLUG_IN_MANAGER (object);
  gint64             memsize = 0;

  memsize += gimp_g_slist_get_memsize_foreach (manager->plug_in_defs,
                                               (GimpMemsizeFunc)
                                               gimp_object_get_memsize,
                                               gui_size);

  memsize += gimp_g_slist_get_memsize (manager->plug_in_procedures, 0);
  memsize += gimp_g_slist_get_memsize (manager->load_procs, 0);
  memsize += gimp_g_slist_get_memsize (manager->save_procs, 0);

  memsize += gimp_g_slist_get_memsize (manager->menu_branches,  0 /* FIXME */);
  memsize += gimp_g_slist_get_memsize (manager->locale_domains, 0 /* FIXME */);
  memsize += gimp_g_slist_get_memsize (manager->help_domains,   0 /* FIXME */);

  memsize += gimp_g_slist_get_memsize_foreach (manager->open_plug_ins,
                                               (GimpMemsizeFunc)
                                               gimp_object_get_memsize,
                                               gui_size);
  memsize += gimp_g_slist_get_memsize (manager->plug_in_stack, 0 /* FIXME */);
  memsize += gimp_g_slist_get_memsize (manager->history,       0);

  memsize += 0; /* FIXME manager->shm */
  memsize += gimp_object_get_memsize (GIMP_OBJECT (manager->interpreter_db),
                                      gui_size);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (manager->environ_table),
                                      gui_size);
  memsize += 0; /* FIXME manager->plug_in_debug */
  memsize += gimp_g_list_get_memsize (manager->data_list, 0 /* FIXME */);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

GimpPlugInManager *
gimp_plug_in_manager_new (Gimp *gimp)
{
  GimpPlugInManager *manager;

  manager = g_object_new (GIMP_TYPE_PLUG_IN_MANAGER, NULL);

  manager->gimp = gimp;

  return manager;
}

void
gimp_plug_in_manager_initialize (GimpPlugInManager  *manager,
                                 GimpInitStatusFunc  status_callback)
{
  gchar *path;

  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (status_callback != NULL);

  status_callback (NULL, _("Plug-In Interpreters"), 0.8);

  path = gimp_config_path_expand (manager->gimp->config->interpreter_path,
                                  TRUE, NULL);
  gimp_interpreter_db_load (manager->interpreter_db, path);
  g_free (path);

  status_callback (NULL, _("Plug-In Environment"), 0.9);

  path = gimp_config_path_expand (manager->gimp->config->environ_path,
                                  TRUE, NULL);
  gimp_environ_table_load (manager->environ_table, path);
  g_free (path);

  /*  allocate a piece of shared memory for use in transporting tiles
   *  to plug-ins. if we can't allocate a piece of shared memory then
   *  we'll fall back on sending the data over the pipe.
   */
  if (manager->gimp->use_shm)
    manager->shm = gimp_plug_in_shm_new ();

  manager->debug = gimp_plug_in_debug_new ();
}

void
gimp_plug_in_manager_restore (GimpPlugInManager  *manager,
                              GimpContext        *context,
                              GimpInitStatusFunc  status_callback)
{
  Gimp   *gimp;
  gchar  *pluginrc;
  GSList *rc_defs;
  GSList *list;
  gint    n_plugins;
  gint    nth;
  GError *error = NULL;

  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (status_callback != NULL);

  gimp = manager->gimp;

  /* search for binaries in the plug-in directory path */
  {
    gchar *path;

    status_callback (_("Searching Plug-Ins"), "", 0.0);

    path = gimp_config_path_expand (gimp->config->plug_in_path, TRUE, NULL);

    gimp_datafiles_read_directories (path,
                                     G_FILE_TEST_IS_EXECUTABLE,
                                     gimp_plug_in_manager_add_from_file,
                                     manager);

    g_free (path);
  }

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
    g_print ("Parsing '%s'\n", gimp_filename_to_utf8 (pluginrc));

  rc_defs = plug_in_rc_parse (gimp, pluginrc, &error);

  if (rc_defs)
    {
      for (list = rc_defs; list; list = g_slist_next (list))
        gimp_plug_in_manager_add_from_rc (manager, list->data); /* consumes list->data */

      g_slist_free (rc_defs);
    }
  else if (error)
    {
      if (error->code != GIMP_CONFIG_ERROR_OPEN_ENOENT)
        gimp_message (gimp, NULL, GIMP_MESSAGE_ERROR, "%s", error->message);

      g_clear_error (&error);
    }

  /*  query any plug-ins that have changed since we last wrote out
   *  the pluginrc file
   */
  status_callback (_("Querying new Plug-ins"), "", 0.0);

  for (list = manager->plug_in_defs, n_plugins = 0; list; list = list->next)
    {
      GimpPlugInDef *plug_in_def = list->data;

      if (plug_in_def->needs_query)
        n_plugins++;
    }

  if (n_plugins)
    {
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

              if (gimp->be_verbose)
                g_print ("Querying plug-in: '%s'\n",
                         gimp_filename_to_utf8 (plug_in_def->prog));

              gimp_plug_in_manager_call_query (manager, context, plug_in_def);
            }
        }
    }

  /* initialize the plug-ins */
  status_callback (_("Initializing Plug-ins"), "", 0.0);

  for (list = manager->plug_in_defs, n_plugins = 0; list; list = list->next)
    {
      GimpPlugInDef *plug_in_def = list->data;

      if (plug_in_def->has_init)
        n_plugins++;
    }

  if (n_plugins)
    {
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

              if (gimp->be_verbose)
                g_print ("Initializing plug-in: '%s'\n",
                         gimp_filename_to_utf8 (plug_in_def->prog));

              gimp_plug_in_manager_call_init (manager, context, plug_in_def);
            }
        }
    }

  status_callback (NULL, "", 1.0);

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

  /* run automatically started extensions */
  {
    GList *extensions = NULL;
    gint   n_extensions;

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

        status_callback (_("Starting Extensions"), "", 0.0);

        for (list = extensions, nth = 0; list; list = g_list_next (list), nth++)
          {
            GimpPlugInProcedure *proc = list->data;
            GValueArray         *args;

            if (gimp->be_verbose)
              g_print ("Starting extension: '%s'\n",
                       GIMP_OBJECT (proc)->name);

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
}

void
gimp_plug_in_manager_exit (GimpPlugInManager *manager)
{
  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));

  while (manager->open_plug_ins)
    gimp_plug_in_close (manager->open_plug_ins->data, TRUE);
}

void
gimp_plug_in_manager_add_procedure (GimpPlugInManager   *manager,
                                    GimpPlugInProcedure *procedure)
{
  GSList *list;

  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (procedure));

  for (list = manager->plug_in_procedures; list; list = list->next)
    {
      GimpPlugInProcedure *tmp_proc = list->data;

      if (strcmp (GIMP_OBJECT (procedure)->name,
                  GIMP_OBJECT (tmp_proc)->name) == 0)
        {
          GSList *list2;

          list->data = g_object_ref (procedure);

          g_printerr ("Removing duplicate PDB procedure '%s' "
                      "registered by '%s'\n",
                      GIMP_OBJECT (tmp_proc)->name,
                      gimp_filename_to_utf8 (tmp_proc->prog));

          /* search the plugin list to see if any plugins had references to
           * the tmp_proc.
           */
          for (list2 = manager->plug_in_defs; list2; list2 = list2->next)
            {
              GimpPlugInDef *plug_in_def = list2->data;

              if (g_slist_find (plug_in_def->procedures, tmp_proc))
                gimp_plug_in_def_remove_procedure (plug_in_def, tmp_proc);
            }

          /* also remove it from the lists of load and save procs */
          manager->load_procs = g_slist_remove (manager->load_procs, tmp_proc);
          manager->save_procs = g_slist_remove (manager->save_procs, tmp_proc);

          /* and from the history */
          gimp_plug_in_manager_history_remove (manager, tmp_proc);

          g_object_unref (tmp_proc);

          return;
        }
    }

  manager->plug_in_procedures = g_slist_prepend (manager->plug_in_procedures,
                                                 g_object_ref (procedure));
}

void
gimp_plug_in_manager_add_temp_proc (GimpPlugInManager      *manager,
                                    GimpTemporaryProcedure *procedure)
{
  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (GIMP_IS_TEMPORARY_PROCEDURE (procedure));

  gimp_pdb_register_procedure (manager->gimp->pdb, GIMP_PROCEDURE (procedure));

  manager->plug_in_procedures = g_slist_prepend (manager->plug_in_procedures,
                                                 g_object_ref (procedure));
}

void
gimp_plug_in_manager_remove_temp_proc (GimpPlugInManager      *manager,
                                       GimpTemporaryProcedure *procedure)
{
  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (GIMP_IS_TEMPORARY_PROCEDURE (procedure));

  manager->plug_in_procedures = g_slist_remove (manager->plug_in_procedures,
                                                procedure);

  gimp_plug_in_manager_history_remove (manager,
                                       GIMP_PLUG_IN_PROCEDURE (procedure));

  gimp_pdb_unregister_procedure (manager->gimp->pdb,
                                 GIMP_PROCEDURE (procedure));

  g_object_unref (procedure);
}

void
gimp_plug_in_manager_add_open_plug_in (GimpPlugInManager *manager,
                                       GimpPlugIn        *plug_in)
{
  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  manager->open_plug_ins = g_slist_prepend (manager->open_plug_ins,
                                            g_object_ref (plug_in));

  g_signal_emit (manager, manager_signals[PLUG_IN_OPENED], 0,
                 plug_in);
}

void
gimp_plug_in_manager_remove_open_plug_in (GimpPlugInManager *manager,
                                          GimpPlugIn        *plug_in)
{
  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  manager->open_plug_ins = g_slist_remove (manager->open_plug_ins, plug_in);

  g_signal_emit (manager, manager_signals[PLUG_IN_CLOSED], 0,
                 plug_in);

  g_object_unref (plug_in);
}

void
gimp_plug_in_manager_plug_in_push (GimpPlugInManager *manager,
                                   GimpPlugIn        *plug_in)
{
  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  manager->current_plug_in = plug_in;

  manager->plug_in_stack = g_slist_prepend (manager->plug_in_stack,
                                            manager->current_plug_in);
}

void
gimp_plug_in_manager_plug_in_pop (GimpPlugInManager *manager)
{
  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));

  if (manager->current_plug_in)
    manager->plug_in_stack = g_slist_remove (manager->plug_in_stack,
                                             manager->plug_in_stack->data);

  if (manager->plug_in_stack)
    manager->current_plug_in = manager->plug_in_stack->data;
  else
    manager->current_plug_in = NULL;
}

void
gimp_plug_in_manager_history_changed (GimpPlugInManager *manager)
{
  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));

  g_signal_emit (manager, manager_signals[HISTORY_CHANGED], 0);
}


/*  private functions  */

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
              /* Use pluginrc entry, deleting ondisk entry */
              list->data = plug_in_def;
              g_object_unref (ondisk_plug_in_def);
            }
          else
            {
              /* Use ondisk entry, deleting pluginrc entry */
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
  g_printerr ("Executable not found: '%s'\n",
              gimp_filename_to_utf8 (plug_in_def->prog));
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
