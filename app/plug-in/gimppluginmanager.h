/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmapluginmanager.h
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

#ifndef __LIGMA_PLUG_IN_MANAGER_H__
#define __LIGMA_PLUG_IN_MANAGER_H__


#include "core/ligmaobject.h"


#define LIGMA_TYPE_PLUG_IN_MANAGER            (ligma_plug_in_manager_get_type ())
#define LIGMA_PLUG_IN_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PLUG_IN_MANAGER, LigmaPlugInManager))
#define LIGMA_PLUG_IN_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PLUG_IN_MANAGER, LigmaPlugInManagerClass))
#define LIGMA_IS_PLUG_IN_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PLUG_IN_MANAGER))
#define LIGMA_IS_PLUG_IN_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PLUG_IN_MANAGER))


typedef struct _LigmaPlugInManagerClass LigmaPlugInManagerClass;

struct _LigmaPlugInManager
{
  LigmaObject         parent_instance;

  Ligma              *ligma;

  GSList            *plug_in_defs;
  gboolean           write_pluginrc;

  GSList            *plug_in_procedures;

  GSList            *load_procs;
  GSList            *save_procs;
  GSList            *export_procs;
  GSList            *raw_load_procs;
  GSList            *batch_procs;

  GSList            *display_load_procs;
  GSList            *display_save_procs;
  GSList            *display_export_procs;
  GSList            *display_raw_load_procs;

  GSList            *menu_branches;
  GSList            *help_domains;

  LigmaPlugIn        *current_plug_in;
  GSList            *open_plug_ins;
  GSList            *plug_in_stack;

  LigmaPlugInShm     *shm;
  LigmaInterpreterDB *interpreter_db;
  LigmaEnvironTable  *environ_table;
  LigmaPlugInDebug   *debug;
  GList             *data_list;
};

struct _LigmaPlugInManagerClass
{
  LigmaObjectClass  parent_class;

  void (* plug_in_opened)    (LigmaPlugInManager *manager,
                              LigmaPlugIn        *plug_in);
  void (* plug_in_closed)    (LigmaPlugInManager *manager,
                              LigmaPlugIn        *plug_in);

  void (* menu_branch_added) (LigmaPlugInManager *manager,
                              GFile             *file,
                              const gchar       *menu_path,
                              const gchar       *menu_label);
};


GType               ligma_plug_in_manager_get_type (void) G_GNUC_CONST;

LigmaPlugInManager * ligma_plug_in_manager_new      (Ligma                *ligma);

void    ligma_plug_in_manager_initialize           (LigmaPlugInManager   *manager,
                                                   LigmaInitStatusFunc   status_callback);
void    ligma_plug_in_manager_exit                 (LigmaPlugInManager   *manager);

/* Register a plug-in. This function is public for file load-save
 * handlers, which are organized around the plug-in data structure.
 * This could all be done a little better, but oh well.  -josh
 */
void    ligma_plug_in_manager_add_procedure        (LigmaPlugInManager   *manager,
                                                   LigmaPlugInProcedure *procedure);

void    ligma_plug_in_manager_add_batch_procedure  (LigmaPlugInManager      *manager,
                                                   LigmaPlugInProcedure    *proc);
GSList * ligma_plug_in_manager_get_batch_procedures (LigmaPlugInManager      *manager);

void    ligma_plug_in_manager_add_temp_proc        (LigmaPlugInManager      *manager,
                                                   LigmaTemporaryProcedure *procedure);
void    ligma_plug_in_manager_remove_temp_proc     (LigmaPlugInManager      *manager,
                                                   LigmaTemporaryProcedure *procedure);

void    ligma_plug_in_manager_add_open_plug_in     (LigmaPlugInManager   *manager,
                                                   LigmaPlugIn          *plug_in);
void    ligma_plug_in_manager_remove_open_plug_in  (LigmaPlugInManager   *manager,
                                                   LigmaPlugIn          *plug_in);

void    ligma_plug_in_manager_plug_in_push         (LigmaPlugInManager   *manager,
                                                   LigmaPlugIn          *plug_in);
void    ligma_plug_in_manager_plug_in_pop          (LigmaPlugInManager   *manager);


#endif  /* __LIGMA_PLUG_IN_MANAGER_H__ */
