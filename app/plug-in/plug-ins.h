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


void              plug_ins_init                 (Gimp               *gimp,
                                                 GimpContext        *context,
                                                 GimpInitStatusFunc  status_callback);
void              plug_ins_exit                 (Gimp          *gimp);

/* Register an internal plug-in.  This is for file load-save
 * handlers, which are organized around the plug-in data structure.
 * This could all be done a little better, but oh well.  -josh
 */
void              plug_ins_add_internal         (Gimp          *gimp,
                                                 PlugInProcDef *proc_def);

/* Add in the file load/save handler fields procedure. */
PlugInProcDef   * plug_ins_file_register_magic  (Gimp          *gimp,
                                                 const gchar   *name,
                                                 const gchar   *extensions,
                                                 const gchar   *prefixes,
                                                 const gchar   *magics);

PlugInProcDef   * plug_ins_file_register_mime   (Gimp          *gimp,
                                                 const gchar   *name,
                                                 const gchar   *mime_type);

PlugInProcDef   * plug_ins_file_register_thumb_loader
                                                (Gimp          *gimp,
                                                 const gchar   *load_proc,
                                                 const gchar   *thumb_proc);


/* Add a plug-in definition. */
void              plug_ins_def_add_from_rc      (Gimp          *gimp,
                                                 PlugInDef     *plug_in_def);

/* Add/Remove temporary procedures. */
void              plug_ins_temp_proc_def_add    (Gimp          *gimp,
                                                 PlugInProcDef *proc_def);
void              plug_ins_temp_proc_def_remove (Gimp          *gimp,
                                                 PlugInProcDef *proc_def);

/* Retrieve a plug-ins locale domain */
const gchar     * plug_ins_locale_domain        (Gimp          *gimp,
                                                 const gchar   *prog_name,
                                                 const gchar  **locale_path);

/* Retrieve a plug-ins help domain */
const gchar     * plug_ins_help_domain          (Gimp          *gimp,
                                                 const gchar   *prog_name,
                                                 const gchar  **help_uri);

/* Retrieve all help domains */
gint              plug_ins_help_domains         (Gimp          *gimp,
                                                 gchar       ***help_domains,
                                                 gchar       ***help_uris);

/* Retreive a plug-ins proc_def from its ProcRecord */
PlugInProcDef   * plug_ins_proc_def_find        (Gimp          *gimp,
                                                 ProcRecord    *proc_rec);


GSList          * plug_ins_extensions_parse     (gchar         *extensions);
PlugInImageType   plug_ins_image_types_parse    (gchar         *image_types);


#endif /* __PLUG_INS_H__ */
