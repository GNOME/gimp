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

#ifndef __PLUG_INS_H__
#define __PLUG_INS_H__


void              plug_ins_init              (Gimp               *gimp,
                                              GimpInitStatusFunc  status_callback);
void              plug_ins_exit              (Gimp          *gimp);

/* Get the "image_types" the plug-in works on. */
gchar           * plug_ins_image_types       (gchar         *name);

/* Add in the file load/save handler fields procedure. */
PlugInProcDef   * plug_ins_file_handler      (gchar         *name,
					      gchar         *extensions,
					      gchar         *prefixes,
					      gchar         *magics);

/* Add a plug-in definition. */
void              plug_ins_def_add           (PlugInDef     *plug_in_def);

void              plug_ins_proc_def_remove   (PlugInProcDef *proc_def,
                                              Gimp          *gimp);

/* Retrieve a plug-ins menu path */
gchar           * plug_ins_menu_path         (gchar         *name);

/* Retrieve a plug-ins help path */
gchar           * plug_ins_help_path         (gchar         *prog_name);


/* Register an internal plug-in.  This is for file load-save
 * handlers, which are organized around the plug-in data structure.
 * This could all be done a little better, but oh well.  -josh
 */
void              plug_ins_add_internal      (PlugInProcDef *proc_def);
GSList          * plug_ins_extensions_parse  (gchar         *extensions);
PlugInImageType   plug_ins_image_types_parse (gchar         *image_types);


extern GSList *proc_defs;
extern gchar  *std_plugins_domain;


#endif /* __PLUG_INS_H__ */
