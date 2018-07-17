/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
 *
 * gimppluginmanager.c
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
#include "libgimpconfig/gimpconfig.h"

#include "plug-in-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimp-filter-history.h"
#include "core/gimp-memsize.h"
#include "core/gimpmarshal.h"

#include "pdb/gimppdb.h"

#include "gimpenvirontable.h"
#include "gimpinterpreterdb.h"
#include "gimpplugin.h"
#include "gimpplugindebug.h"
#include "gimpplugindef.h"
#include "gimppluginmanager.h"
#include "gimppluginmanager-data.h"
#include "gimppluginmanager-help-domain.h"
#include "gimppluginmanager-locale-domain.h"
#include "gimppluginmanager-menu-branch.h"
#include "gimppluginshm.h"
#include "gimptemporaryprocedure.h"

#include "gimp-intl.h"


enum
{
  PLUG_IN_OPENED,
  PLUG_IN_CLOSED,
  MENU_BRANCH_ADDED,
  LAST_SIGNAL
};


static void     gimp_plug_in_manager_finalize    (GObject    *object);

static gint64   gimp_plug_in_manager_get_memsize (GimpObject *object,
                                                  gint64     *gui_size);


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
                  gimp_marshal_VOID__OBJECT_STRING_STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_FILE,
                  G_TYPE_STRING,
                  G_TYPE_STRING);

  object_class->finalize         = gimp_plug_in_manager_finalize;

  gimp_object_class->get_memsize = gimp_plug_in_manager_get_memsize;
}

static void
gimp_plug_in_manager_init (GimpPlugInManager *manager)
{
}

static void
gimp_plug_in_manager_finalize (GObject *object)
{
  GimpPlugInManager *manager = GIMP_PLUG_IN_MANAGER (object);

  g_clear_pointer (&manager->load_procs,     g_slist_free);
  g_clear_pointer (&manager->save_procs,     g_slist_free);
  g_clear_pointer (&manager->export_procs,   g_slist_free);
  g_clear_pointer (&manager->raw_load_procs, g_slist_free);

  g_clear_pointer (&manager->display_load_procs,     g_slist_free);
  g_clear_pointer (&manager->display_save_procs,     g_slist_free);
  g_clear_pointer (&manager->display_export_procs,   g_slist_free);
  g_clear_pointer (&manager->display_raw_load_procs, g_slist_free);

  if (manager->plug_in_procedures)
    {
      g_slist_free_full (manager->plug_in_procedures,
                         (GDestroyNotify) g_object_unref);
      manager->plug_in_procedures = NULL;
    }

  if (manager->plug_in_defs)
    {
      g_slist_free_full (manager->plug_in_defs,
                         (GDestroyNotify) g_object_unref);
      manager->plug_in_defs = NULL;
    }

  g_clear_object (&manager->environ_table);
  g_clear_object (&manager->interpreter_db);

  g_clear_pointer (&manager->debug, gimp_plug_in_debug_free);

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
  memsize += gimp_g_slist_get_memsize (manager->export_procs, 0);
  memsize += gimp_g_slist_get_memsize (manager->raw_load_procs, 0);
  memsize += gimp_g_slist_get_memsize (manager->display_load_procs, 0);
  memsize += gimp_g_slist_get_memsize (manager->display_save_procs, 0);
  memsize += gimp_g_slist_get_memsize (manager->display_export_procs, 0);
  memsize += gimp_g_slist_get_memsize (manager->display_raw_load_procs, 0);

  memsize += gimp_g_slist_get_memsize (manager->menu_branches,  0 /* FIXME */);
  memsize += gimp_g_slist_get_memsize (manager->locale_domains, 0 /* FIXME */);
  memsize += gimp_g_slist_get_memsize (manager->help_domains,   0 /* FIXME */);

  memsize += gimp_g_slist_get_memsize_foreach (manager->open_plug_ins,
                                               (GimpMemsizeFunc)
                                               gimp_object_get_memsize,
                                               gui_size);
  memsize += gimp_g_slist_get_memsize (manager->plug_in_stack, 0);

  memsize += 0; /* FIXME manager->shm */
  memsize += /* FIXME */ gimp_g_object_get_memsize (G_OBJECT (manager->interpreter_db));
  memsize += /* FIXME */ gimp_g_object_get_memsize (G_OBJECT (manager->environ_table));
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

  manager->gimp           = gimp;
  manager->interpreter_db = gimp_interpreter_db_new (gimp->be_verbose);
  manager->environ_table  = gimp_environ_table_new (gimp->be_verbose);

  return manager;
}

void
gimp_plug_in_manager_initialize (GimpPlugInManager  *manager,
                                 GimpInitStatusFunc  status_callback)
{
  GimpCoreConfig *config;
  GList          *path;

  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (status_callback != NULL);

  config = manager->gimp->config;

  status_callback (NULL, _("Plug-in Interpreters"), 0.8);

  path = gimp_config_path_expand_to_files (config->interpreter_path, NULL);
  gimp_interpreter_db_load (manager->interpreter_db, path);
  g_list_free_full (path, (GDestroyNotify) g_object_unref);

  status_callback (NULL, _("Plug-in Environment"), 0.9);

  path = gimp_config_path_expand_to_files (config->environ_path, NULL);
  gimp_environ_table_load (manager->environ_table, path);
  g_list_free_full (path, (GDestroyNotify) g_object_unref);

  /*  allocate a piece of shared memory for use in transporting tiles
   *  to plug-ins. if we can't allocate a piece of shared memory then
   *  we'll fall back on sending the data over the pipe.
   */
  if (manager->gimp->use_shm)
    manager->shm = gimp_plug_in_shm_new ();

  manager->debug = gimp_plug_in_debug_new ();
}

void
gimp_plug_in_manager_exit (GimpPlugInManager *manager)
{
  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));

  while (manager->open_plug_ins)
    gimp_plug_in_close (manager->open_plug_ins->data, TRUE);

  /*  need to detach from shared memory, we can't rely on exit()
   *  cleaning up behind us (see bug #609026)
   */
  if (manager->shm)
    {
      gimp_plug_in_shm_free (manager->shm);
      manager->shm = NULL;
    }
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

      if (strcmp (gimp_object_get_name (procedure),
                  gimp_object_get_name (tmp_proc)) == 0)
        {
          GSList *list2;

          list->data = g_object_ref (procedure);

          g_printerr ("Removing duplicate PDB procedure '%s' "
                      "registered by '%s'\n",
                      gimp_object_get_name (tmp_proc),
                      gimp_file_get_utf8_name (tmp_proc->file));

          /* search the plugin list to see if any plugins had references to
           * the tmp_proc.
           */
          for (list2 = manager->plug_in_defs; list2; list2 = list2->next)
            {
              GimpPlugInDef *plug_in_def = list2->data;

              if (g_slist_find (plug_in_def->procedures, tmp_proc))
                gimp_plug_in_def_remove_procedure (plug_in_def, tmp_proc);
            }

          /* also remove it from the lists of load, save and export procs */
          manager->load_procs             = g_slist_remove (manager->load_procs,             tmp_proc);
          manager->save_procs             = g_slist_remove (manager->save_procs,             tmp_proc);
          manager->export_procs           = g_slist_remove (manager->export_procs,           tmp_proc);
          manager->raw_load_procs         = g_slist_remove (manager->raw_load_procs,         tmp_proc);
          manager->display_load_procs     = g_slist_remove (manager->display_load_procs,     tmp_proc);
          manager->display_save_procs     = g_slist_remove (manager->display_save_procs,     tmp_proc);
          manager->display_export_procs   = g_slist_remove (manager->display_export_procs,   tmp_proc);
          manager->display_raw_load_procs = g_slist_remove (manager->display_raw_load_procs, tmp_proc);

          /* and from the history */
          gimp_filter_history_remove (manager->gimp, GIMP_PROCEDURE (tmp_proc));

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

  gimp_filter_history_remove (manager->gimp,
                              GIMP_PROCEDURE (procedure));

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
