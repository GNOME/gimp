/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __SCRIPT_FU_SCRIPT_H__
#define __SCRIPT_FU_SCRIPT_H__


SFScript * script_fu_script_new                     (const gchar          *name,
                                                     const gchar          *menu_label,
                                                     const gchar          *blurb,
                                                     const gchar          *authors,
                                                     const gchar          *copyright,
                                                     const gchar          *date,
                                                     const gchar          *image_types,
                                                     gint                  n_args);
void       script_fu_script_free                    (SFScript             *script);

void       script_fu_script_install_proc            (GimpPlugIn           *plug_in,
                                                     SFScript             *script);
void       script_fu_script_uninstall_proc          (GimpPlugIn           *plug_in,
                                                     SFScript             *script);

gchar    * script_fu_script_get_title               (SFScript             *script);
void       script_fu_script_reset                   (SFScript             *script,
                                                     gboolean              reset_ids);

gint       script_fu_script_collect_standard_args   (SFScript             *script,
                                                     GParamSpec          **pspecs,
                                                     guint                 n_pspecs,
                                                     GimpProcedureConfig  *config);

gchar    * script_fu_script_get_command             (SFScript             *script);
gchar    * script_fu_script_get_command_from_params (SFScript             *script,
                                                     GParamSpec          **pspecs,
                                                     guint                 n_pspecs,
                                                     GimpProcedureConfig  *config);
gchar    * script_fu_script_get_command_for_image_proc (
                                                     SFScript             *script,
                                                     GimpImage            *image,
                                                     GimpDrawable        **drawables,
                                                     GimpProcedureConfig  *config);
gchar    * script_fu_script_get_command_for_regular_proc (
                                                     SFScript             *script,
                                                     GimpProcedureConfig  *config);
GimpProcedure * script_fu_script_create_PDB_procedure (GimpPlugIn         *plug_in,
                                                       SFScript           *script,
                                                       GimpPDBProcType     plug_in_type);

void       script_fu_script_infer_drawable_arity      (SFScript           *script);
void       script_fu_script_set_drawable_arity_none   (SFScript           *script);
void       script_fu_script_set_is_old_style          (SFScript           *script);
gboolean   script_fu_script_get_is_old_style          (SFScript           *script);

void       script_fu_script_set_i18n                  (SFScript           *script,
                                                       gchar              *domain,
                                                       gchar              *catalog);
void       script_fu_script_get_i18n                  (SFScript           *script,
                                                       gchar             **domain,
                                                       gchar             **catalog);

#endif /*  __SCRIPT_FU_SCRIPT__  */
