/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * plug-in-proc-def.h
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

#ifndef __PLUG_IN_PROC_DEF_H__
#define __PLUG_IN_PROC_DEF_H__

#include <time.h>      /* time_t */

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "pdb/procedural_db.h"  /* ProcRecord */


struct _PlugInProcDef
{
  /*  common members  */
  gchar           *prog;
  gchar           *menu_label;
  GList           *menu_paths;
  GimpIconType     icon_type;
  gint             icon_data_length;
  gchar           *icon_data;
  gchar           *image_types;
  PlugInImageType  image_types_val;
  time_t           mtime;
  gboolean	   installed_during_init;
  ProcRecord       db_info;

  /*  file proc specific members  */
  gchar           *extensions;
  gchar           *prefixes;
  gchar           *magics;
  gchar           *mime_type;
  GSList          *extensions_list;
  GSList          *prefixes_list;
  GSList          *magics_list;
  gchar           *thumb_loader;
};


PlugInProcDef    * plug_in_proc_def_new          (void);
void               plug_in_proc_def_free         (PlugInProcDef       *proc_def);

const ProcRecord * plug_in_proc_def_get_proc     (const PlugInProcDef *proc_def);
const gchar      * plug_in_proc_def_get_progname (const PlugInProcDef *proc_def);
gchar            * plug_in_proc_def_get_label    (const PlugInProcDef *proc_def,
                                                  const gchar         *locale_domain);
const gchar      * plug_in_proc_def_get_stock_id (const PlugInProcDef *proc_def);
GdkPixbuf        * plug_in_proc_def_get_pixbuf   (const PlugInProcDef *proc_def);
gchar            * plug_in_proc_def_get_help_id  (const PlugInProcDef *proc_def,
                                                  const gchar         *help_domain);
gboolean          plug_in_proc_def_get_sensitive (const PlugInProcDef *proc_def,
                                                  GimpImageType        image_type);


#endif /* __PLUG_IN_PROC_DEF_H__ */
