/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaplugin-proc.h
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

#ifndef __LIGMA_PLUG_IN_PROC_H__
#define __LIGMA_PLUG_IN_PROC_H__


gboolean   ligma_plug_in_set_proc_image_types         (LigmaPlugIn    *plug_in,
                                                      const gchar   *proc_name,
                                                      const gchar   *image_types,
                                                      GError       **error);
gboolean   ligma_plug_in_set_proc_sensitivity_mask    (LigmaPlugIn   *plug_in,
                                                      const gchar  *proc_name,
                                                      gint          sensitivity_mask,
                                                      GError      **error);
gboolean   ligma_plug_in_set_proc_menu_label          (LigmaPlugIn    *plug_in,
                                                      const gchar   *proc_name,
                                                      const gchar   *menu_label,
                                                      GError       **error);
gboolean   ligma_plug_in_add_proc_menu_path           (LigmaPlugIn    *plug_in,
                                                      const gchar   *proc_name,
                                                      const gchar   *menu_path,
                                                      GError       **error);
gboolean   ligma_plug_in_set_proc_icon                (LigmaPlugIn    *plug_in,
                                                      const gchar   *proc_name,
                                                      LigmaIconType   type,
                                                      const guint8  *data,
                                                      gint           data_length,
                                                      GError       **error);
gboolean   ligma_plug_in_set_proc_help                (LigmaPlugIn    *plug_in,
                                                      const gchar   *proc_name,
                                                      const gchar   *blurb,
                                                      const gchar   *help,
                                                      const gchar   *help_id,
                                                      GError       **error);
gboolean   ligma_plug_in_set_proc_attribution         (LigmaPlugIn    *plug_in,
                                                      const gchar   *proc_name,
                                                      const gchar   *authors,
                                                      const gchar   *copyright,
                                                      const gchar   *date,
                                                      GError       **error);

gboolean   ligma_plug_in_set_file_proc_load_handler   (LigmaPlugIn    *plug_in,
                                                      const gchar   *proc_name,
                                                      const gchar   *extensions,
                                                      const gchar   *prefixes,
                                                      const gchar   *magics,
                                                      GError       **error);
gboolean   ligma_plug_in_set_file_proc_save_handler   (LigmaPlugIn    *plug_in,
                                                      const gchar   *proc_name,
                                                      const gchar   *extensions,
                                                      const gchar   *prefixes,
                                                      GError       **error);
gboolean   ligma_plug_in_set_file_proc_priority       (LigmaPlugIn    *plug_in,
                                                      const gchar   *proc_name,
                                                      gint           priority,
                                                      GError       **error);
gboolean   ligma_plug_in_set_file_proc_mime_types     (LigmaPlugIn    *plug_in,
                                                      const gchar   *proc_name,
                                                      const gchar   *mime_types,
                                                      GError       **error);
gboolean   ligma_plug_in_set_file_proc_handles_remote (LigmaPlugIn    *plug_in,
                                                      const gchar   *proc_name,
                                                      GError       **error);
gboolean   ligma_plug_in_set_file_proc_handles_raw    (LigmaPlugIn    *plug_in,
                                                      const gchar   *proc_name,
                                                      GError       **error);
gboolean   ligma_plug_in_set_file_proc_thumb_loader   (LigmaPlugIn    *plug_in,
                                                      const gchar   *proc_name,
                                                      const gchar   *thumb_proc,
                                                      GError       **error);
gboolean   ligma_plug_in_set_batch_interpreter        (LigmaPlugIn   *plug_in,
                                                      const gchar  *proc_name,
                                                      const gchar  *interpreter_name,
                                                      GError      **error);


#endif /* __LIGMA_PLUG_IN_PROC_H__ */
