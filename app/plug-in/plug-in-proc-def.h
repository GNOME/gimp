/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __PLUG_IN_PROC_H__
#define __PLUG_IN_PROC_H__

#include <time.h>      /* time_t */

#include "pdb/procedural_db.h"  /* ProcRecord */


struct _PlugInProcDef
{
  gchar           *prog;
  GList           *menu_paths;
  gchar           *accelerator;
  gchar           *extensions;
  gchar           *prefixes;
  gchar           *magics;
  gchar           *image_types;
  PlugInImageType  image_types_val;
  ProcRecord       db_info;
  GSList          *extensions_list;
  GSList          *prefixes_list;
  GSList          *magics_list;
  time_t           mtime;
  gboolean	   installed_during_init;
};


PlugInProcDef    * plug_in_proc_def_new          (void);
void               plug_in_proc_def_free         (PlugInProcDef       *proc_def);

const ProcRecord * plug_in_proc_def_get_proc     (const PlugInProcDef *proc_def);
const gchar      * plug_in_proc_def_get_progname (const PlugInProcDef *proc_def);
gchar            * plug_in_proc_def_get_help_id  (const PlugInProcDef *proc_def,
                                                  const gchar         *help_domain);

gint          plug_in_proc_def_compare_menu_path (gconstpointer        a,
                                                  gconstpointer        b,
                                                  gpointer             user_data);


#endif /* __PLUG_IN_PROC_H__ */
