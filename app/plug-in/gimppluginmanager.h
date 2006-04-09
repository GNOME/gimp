/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * plug-ins.h
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


struct _PlugInMenuBranch
{
  gchar *prog_name;
  gchar *menu_path;
  gchar *menu_label;
};


void              plug_ins_init                    (Gimp               *gimp,
                                                    GimpContext        *context,
                                                    GimpInitStatusFunc  status_callback);
void              plug_ins_exit                    (Gimp          *gimp);

/* Register an internal plug-in.  This is for file load-save
 * handlers, which are organized around the plug-in data structure.
 * This could all be done a little better, but oh well.  -josh
 */
void              plug_ins_add_internal            (Gimp                *gimp,
                                                    GimpPlugInProcedure *proc);

/* Add in the file load/save handler fields procedure. */
GimpPlugInProcedure * plug_ins_file_register_magic (Gimp          *gimp,
                                                    const gchar   *name,
                                                    const gchar   *extensions,
                                                    const gchar   *prefixes,
                                                    const gchar   *magics);

GimpPlugInProcedure * plug_ins_file_register_mime  (Gimp          *gimp,
                                                    const gchar   *name,
                                                    const gchar   *mime_type);

GimpPlugInProcedure * plug_ins_file_register_thumb_loader
                                                   (Gimp          *gimp,
                                                    const gchar   *load_proc,
                                                    const gchar   *thumb_proc);

/* Add/Remove temporary procedures. */
void              plug_ins_temp_procedure_add    (Gimp                   *gimp,
                                                  GimpTemporaryProcedure *proc);
void              plug_ins_temp_procedure_remove (Gimp                   *gimp,
                                                  GimpTemporaryProcedure *proc);

/* Add a menu branch */
void              plug_ins_menu_branch_add         (Gimp          *gimp,
                                                    const gchar   *prog_name,
                                                    const gchar   *menu_path,
                                                    const gchar   *menu_label);


#endif /* __PLUG_INS_H__ */
