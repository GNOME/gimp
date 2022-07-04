/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimppluginmanager.h
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

#ifndef __GIMP_PLUG_IN_MANAGER_H__
#define __GIMP_PLUG_IN_MANAGER_H__


#include "core/gimpobject.h"


#define GIMP_TYPE_PLUG_IN_MANAGER            (gimp_plug_in_manager_get_type ())
#define GIMP_PLUG_IN_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PLUG_IN_MANAGER, GimpPlugInManager))
#define GIMP_PLUG_IN_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PLUG_IN_MANAGER, GimpPlugInManagerClass))
#define GIMP_IS_PLUG_IN_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PLUG_IN_MANAGER))
#define GIMP_IS_PLUG_IN_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PLUG_IN_MANAGER))


typedef struct _GimpPlugInManagerClass GimpPlugInManagerClass;

struct _GimpPlugInManager
{
  GimpObject         parent_instance;

  Gimp              *gimp;

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

  GimpPlugIn        *current_plug_in;
  GSList            *open_plug_ins;
  GSList            *plug_in_stack;

  GimpPlugInShm     *shm;
  GimpInterpreterDB *interpreter_db;
  GimpEnvironTable  *environ_table;
  GimpPlugInDebug   *debug;
  GList             *data_list;
};

struct _GimpPlugInManagerClass
{
  GimpObjectClass  parent_class;

  void (* plug_in_opened)    (GimpPlugInManager *manager,
                              GimpPlugIn        *plug_in);
  void (* plug_in_closed)    (GimpPlugInManager *manager,
                              GimpPlugIn        *plug_in);

  void (* menu_branch_added) (GimpPlugInManager *manager,
                              GFile             *file,
                              const gchar       *menu_path,
                              const gchar       *menu_label);
};


GType               gimp_plug_in_manager_get_type (void) G_GNUC_CONST;

GimpPlugInManager * gimp_plug_in_manager_new      (Gimp                *gimp);

void    gimp_plug_in_manager_initialize           (GimpPlugInManager   *manager,
                                                   GimpInitStatusFunc   status_callback);
void    gimp_plug_in_manager_exit                 (GimpPlugInManager   *manager);

/* Register a plug-in. This function is public for file load-save
 * handlers, which are organized around the plug-in data structure.
 * This could all be done a little better, but oh well.  -josh
 */
void    gimp_plug_in_manager_add_procedure        (GimpPlugInManager   *manager,
                                                   GimpPlugInProcedure *procedure);

void    gimp_plug_in_manager_add_batch_procedure  (GimpPlugInManager      *manager,
                                                   GimpPlugInProcedure    *proc);
GSList * gimp_plug_in_manager_get_batch_procedures (GimpPlugInManager      *manager);

void    gimp_plug_in_manager_add_temp_proc        (GimpPlugInManager      *manager,
                                                   GimpTemporaryProcedure *procedure);
void    gimp_plug_in_manager_remove_temp_proc     (GimpPlugInManager      *manager,
                                                   GimpTemporaryProcedure *procedure);

void    gimp_plug_in_manager_add_open_plug_in     (GimpPlugInManager   *manager,
                                                   GimpPlugIn          *plug_in);
void    gimp_plug_in_manager_remove_open_plug_in  (GimpPlugInManager   *manager,
                                                   GimpPlugIn          *plug_in);

void    gimp_plug_in_manager_plug_in_push         (GimpPlugInManager   *manager,
                                                   GimpPlugIn          *plug_in);
void    gimp_plug_in_manager_plug_in_pop          (GimpPlugInManager   *manager);


#endif  /* __GIMP_PLUG_IN_MANAGER_H__ */
