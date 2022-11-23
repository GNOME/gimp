/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
 *
 * ligmapluginmanager.c
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

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"

#include "plug-in-types.h"

#include "config/ligmacoreconfig.h"

#include "core/ligma.h"
#include "core/ligma-filter-history.h"
#include "core/ligma-memsize.h"
#include "core/ligmamarshal.h"

#include "pdb/ligmapdb.h"

#include "ligmaenvirontable.h"
#include "ligmainterpreterdb.h"
#include "ligmaplugin.h"
#include "ligmaplugindebug.h"
#include "ligmaplugindef.h"
#include "ligmapluginmanager.h"
#include "ligmapluginmanager-data.h"
#include "ligmapluginmanager-help-domain.h"
#include "ligmapluginmanager-menu-branch.h"
#include "ligmapluginshm.h"
#include "ligmatemporaryprocedure.h"

#include "ligma-intl.h"


enum
{
  PLUG_IN_OPENED,
  PLUG_IN_CLOSED,
  MENU_BRANCH_ADDED,
  LAST_SIGNAL
};


static void     ligma_plug_in_manager_finalize    (GObject    *object);

static gint64   ligma_plug_in_manager_get_memsize (LigmaObject *object,
                                                  gint64     *gui_size);


G_DEFINE_TYPE (LigmaPlugInManager, ligma_plug_in_manager, LIGMA_TYPE_OBJECT)

#define parent_class ligma_plug_in_manager_parent_class

static guint manager_signals[LAST_SIGNAL] = { 0, };


static void
ligma_plug_in_manager_class_init (LigmaPlugInManagerClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass *ligma_object_class = LIGMA_OBJECT_CLASS (klass);

  manager_signals[PLUG_IN_OPENED] =
    g_signal_new ("plug-in-opened",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaPlugInManagerClass,
                                   plug_in_opened),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_PLUG_IN);

  manager_signals[PLUG_IN_CLOSED] =
    g_signal_new ("plug-in-closed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaPlugInManagerClass,
                                   plug_in_closed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_PLUG_IN);

  manager_signals[MENU_BRANCH_ADDED] =
    g_signal_new ("menu-branch-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaPlugInManagerClass,
                                   menu_branch_added),
                  NULL, NULL,
                  ligma_marshal_VOID__OBJECT_STRING_STRING,
                  G_TYPE_NONE, 3,
                  G_TYPE_FILE,
                  G_TYPE_STRING,
                  G_TYPE_STRING);

  object_class->finalize         = ligma_plug_in_manager_finalize;

  ligma_object_class->get_memsize = ligma_plug_in_manager_get_memsize;
}

static void
ligma_plug_in_manager_init (LigmaPlugInManager *manager)
{
}

static void
ligma_plug_in_manager_finalize (GObject *object)
{
  LigmaPlugInManager *manager = LIGMA_PLUG_IN_MANAGER (object);

  g_clear_pointer (&manager->load_procs,     g_slist_free);
  g_clear_pointer (&manager->save_procs,     g_slist_free);
  g_clear_pointer (&manager->export_procs,   g_slist_free);
  g_clear_pointer (&manager->raw_load_procs, g_slist_free);
  g_clear_pointer (&manager->batch_procs,    g_slist_free);

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

  g_clear_pointer (&manager->debug, ligma_plug_in_debug_free);

  ligma_plug_in_manager_menu_branch_exit (manager);
  ligma_plug_in_manager_help_domain_exit (manager);
  ligma_plug_in_manager_data_free (manager);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
ligma_plug_in_manager_get_memsize (LigmaObject *object,
                                  gint64     *gui_size)
{
  LigmaPlugInManager *manager = LIGMA_PLUG_IN_MANAGER (object);
  gint64             memsize = 0;

  memsize += ligma_g_slist_get_memsize_foreach (manager->plug_in_defs,
                                               (LigmaMemsizeFunc)
                                               ligma_object_get_memsize,
                                               gui_size);

  memsize += ligma_g_slist_get_memsize (manager->plug_in_procedures, 0);
  memsize += ligma_g_slist_get_memsize (manager->load_procs, 0);
  memsize += ligma_g_slist_get_memsize (manager->save_procs, 0);
  memsize += ligma_g_slist_get_memsize (manager->export_procs, 0);
  memsize += ligma_g_slist_get_memsize (manager->raw_load_procs, 0);
  memsize += ligma_g_slist_get_memsize (manager->batch_procs, 0);
  memsize += ligma_g_slist_get_memsize (manager->display_load_procs, 0);
  memsize += ligma_g_slist_get_memsize (manager->display_save_procs, 0);
  memsize += ligma_g_slist_get_memsize (manager->display_export_procs, 0);
  memsize += ligma_g_slist_get_memsize (manager->display_raw_load_procs, 0);

  memsize += ligma_g_slist_get_memsize (manager->menu_branches,  0 /* FIXME */);
  memsize += ligma_g_slist_get_memsize (manager->help_domains,   0 /* FIXME */);

  memsize += ligma_g_slist_get_memsize_foreach (manager->open_plug_ins,
                                               (LigmaMemsizeFunc)
                                               ligma_object_get_memsize,
                                               gui_size);
  memsize += ligma_g_slist_get_memsize (manager->plug_in_stack, 0);

  memsize += 0; /* FIXME manager->shm */
  memsize += /* FIXME */ ligma_g_object_get_memsize (G_OBJECT (manager->interpreter_db));
  memsize += /* FIXME */ ligma_g_object_get_memsize (G_OBJECT (manager->environ_table));
  memsize += 0; /* FIXME manager->plug_in_debug */
  memsize += ligma_g_list_get_memsize (manager->data_list, 0 /* FIXME */);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

LigmaPlugInManager *
ligma_plug_in_manager_new (Ligma *ligma)
{
  LigmaPlugInManager *manager;

  manager = g_object_new (LIGMA_TYPE_PLUG_IN_MANAGER, NULL);

  manager->ligma           = ligma;
  manager->interpreter_db = ligma_interpreter_db_new (ligma->be_verbose);
  manager->environ_table  = ligma_environ_table_new (ligma->be_verbose);

  return manager;
}

void
ligma_plug_in_manager_initialize (LigmaPlugInManager  *manager,
                                 LigmaInitStatusFunc  status_callback)
{
  LigmaCoreConfig *config;
  GList          *path;

  g_return_if_fail (LIGMA_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (status_callback != NULL);

  config = manager->ligma->config;

  status_callback (NULL, _("Plug-in Interpreters"), 0.8);

  path = ligma_config_path_expand_to_files (config->interpreter_path, NULL);
  ligma_interpreter_db_load (manager->interpreter_db, path);
  g_list_free_full (path, (GDestroyNotify) g_object_unref);

  status_callback (NULL, _("Plug-in Environment"), 0.9);

  path = ligma_config_path_expand_to_files (config->environ_path, NULL);
  ligma_environ_table_load (manager->environ_table, path);
  g_list_free_full (path, (GDestroyNotify) g_object_unref);

  /*  allocate a piece of shared memory for use in transporting tiles
   *  to plug-ins. if we can't allocate a piece of shared memory then
   *  we'll fall back on sending the data over the pipe.
   */
  if (manager->ligma->use_shm)
    manager->shm = ligma_plug_in_shm_new ();

  manager->debug = ligma_plug_in_debug_new ();
}

void
ligma_plug_in_manager_exit (LigmaPlugInManager *manager)
{
  g_return_if_fail (LIGMA_IS_PLUG_IN_MANAGER (manager));

  while (manager->open_plug_ins)
    ligma_plug_in_close (manager->open_plug_ins->data, TRUE);

  /*  need to detach from shared memory, we can't rely on exit()
   *  cleaning up behind us (see bug #609026)
   */
  if (manager->shm)
    {
      ligma_plug_in_shm_free (manager->shm);
      manager->shm = NULL;
    }
}

void
ligma_plug_in_manager_add_procedure (LigmaPlugInManager   *manager,
                                    LigmaPlugInProcedure *procedure)
{
  GSList *list;

  g_return_if_fail (LIGMA_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (LIGMA_IS_PLUG_IN_PROCEDURE (procedure));

  for (list = manager->plug_in_procedures; list; list = list->next)
    {
      LigmaPlugInProcedure *tmp_proc = list->data;

      if (strcmp (ligma_object_get_name (procedure),
                  ligma_object_get_name (tmp_proc)) == 0)
        {
          GSList *list2;

          list->data = g_object_ref (procedure);

          g_printerr ("Removing duplicate PDB procedure '%s' "
                      "registered by '%s'\n",
                      ligma_object_get_name (tmp_proc),
                      ligma_file_get_utf8_name (tmp_proc->file));

          /* search the plugin list to see if any plugins had references to
           * the tmp_proc.
           */
          for (list2 = manager->plug_in_defs; list2; list2 = list2->next)
            {
              LigmaPlugInDef *plug_in_def = list2->data;

              if (g_slist_find (plug_in_def->procedures, tmp_proc))
                ligma_plug_in_def_remove_procedure (plug_in_def, tmp_proc);
            }

          /* also remove it from the lists of load, save and export procs */
          manager->load_procs             = g_slist_remove (manager->load_procs,             tmp_proc);
          manager->save_procs             = g_slist_remove (manager->save_procs,             tmp_proc);
          manager->export_procs           = g_slist_remove (manager->export_procs,           tmp_proc);
          manager->raw_load_procs         = g_slist_remove (manager->raw_load_procs,         tmp_proc);
          manager->batch_procs            = g_slist_remove (manager->batch_procs,            tmp_proc);
          manager->display_load_procs     = g_slist_remove (manager->display_load_procs,     tmp_proc);
          manager->display_save_procs     = g_slist_remove (manager->display_save_procs,     tmp_proc);
          manager->display_export_procs   = g_slist_remove (manager->display_export_procs,   tmp_proc);
          manager->display_raw_load_procs = g_slist_remove (manager->display_raw_load_procs, tmp_proc);

          /* and from the history */
          ligma_filter_history_remove (manager->ligma, LIGMA_PROCEDURE (tmp_proc));

          g_object_unref (tmp_proc);

          return;
        }
    }

  manager->plug_in_procedures = g_slist_prepend (manager->plug_in_procedures,
                                                 g_object_ref (procedure));
}

void
ligma_plug_in_manager_add_batch_procedure (LigmaPlugInManager   *manager,
                                          LigmaPlugInProcedure *proc)
{
  g_return_if_fail (LIGMA_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (LIGMA_IS_PLUG_IN_PROCEDURE (proc));

  if (! g_slist_find (manager->batch_procs, proc))
    manager->batch_procs = g_slist_prepend (manager->batch_procs, proc);
}

GSList *
ligma_plug_in_manager_get_batch_procedures (LigmaPlugInManager *manager)
{
  g_return_val_if_fail (LIGMA_IS_PLUG_IN_MANAGER (manager), NULL);

  return manager->batch_procs;
}

void
ligma_plug_in_manager_add_temp_proc (LigmaPlugInManager      *manager,
                                    LigmaTemporaryProcedure *procedure)
{
  g_return_if_fail (LIGMA_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (LIGMA_IS_TEMPORARY_PROCEDURE (procedure));

  ligma_pdb_register_procedure (manager->ligma->pdb, LIGMA_PROCEDURE (procedure));

  manager->plug_in_procedures = g_slist_prepend (manager->plug_in_procedures,
                                                 g_object_ref (procedure));
}

void
ligma_plug_in_manager_remove_temp_proc (LigmaPlugInManager      *manager,
                                       LigmaTemporaryProcedure *procedure)
{
  g_return_if_fail (LIGMA_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (LIGMA_IS_TEMPORARY_PROCEDURE (procedure));

  manager->plug_in_procedures = g_slist_remove (manager->plug_in_procedures,
                                                procedure);

  ligma_filter_history_remove (manager->ligma,
                              LIGMA_PROCEDURE (procedure));

  ligma_pdb_unregister_procedure (manager->ligma->pdb,
                                 LIGMA_PROCEDURE (procedure));

  g_object_unref (procedure);
}

void
ligma_plug_in_manager_add_open_plug_in (LigmaPlugInManager *manager,
                                       LigmaPlugIn        *plug_in)
{
  g_return_if_fail (LIGMA_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (LIGMA_IS_PLUG_IN (plug_in));

  manager->open_plug_ins = g_slist_prepend (manager->open_plug_ins,
                                            g_object_ref (plug_in));

  g_signal_emit (manager, manager_signals[PLUG_IN_OPENED], 0,
                 plug_in);
}

void
ligma_plug_in_manager_remove_open_plug_in (LigmaPlugInManager *manager,
                                          LigmaPlugIn        *plug_in)
{
  g_return_if_fail (LIGMA_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (LIGMA_IS_PLUG_IN (plug_in));

  manager->open_plug_ins = g_slist_remove (manager->open_plug_ins, plug_in);

  g_signal_emit (manager, manager_signals[PLUG_IN_CLOSED], 0,
                 plug_in);

  g_object_unref (plug_in);
}

void
ligma_plug_in_manager_plug_in_push (LigmaPlugInManager *manager,
                                   LigmaPlugIn        *plug_in)
{
  g_return_if_fail (LIGMA_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (LIGMA_IS_PLUG_IN (plug_in));

  manager->current_plug_in = plug_in;

  manager->plug_in_stack = g_slist_prepend (manager->plug_in_stack,
                                            manager->current_plug_in);
}

void
ligma_plug_in_manager_plug_in_pop (LigmaPlugInManager *manager)
{
  g_return_if_fail (LIGMA_IS_PLUG_IN_MANAGER (manager));

  if (manager->current_plug_in)
    manager->plug_in_stack = g_slist_remove (manager->plug_in_stack,
                                             manager->plug_in_stack->data);

  if (manager->plug_in_stack)
    manager->current_plug_in = manager->plug_in_stack->data;
  else
    manager->current_plug_in = NULL;
}
