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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __SCRIPT_FU_SCRIPT_H__
#define __SCRIPT_FU_SCRIPT_H__


SFScript * script_fu_script_new                     (const gchar     *name,
                                                     const gchar     *menu_label,
                                                     const gchar     *blurb,
                                                     const gchar     *author,
                                                     const gchar     *copyright,
                                                     const gchar     *date,
                                                     const gchar     *image_types,
                                                     gint             n_args);
void       script_fu_script_free                    (SFScript        *script);

void       script_fu_script_install_proc            (SFScript        *script,
                                                     GimpRunProc      run_proc);
void       script_fu_script_uninstall_proc          (SFScript        *script);

gchar    * script_fu_script_get_title               (SFScript        *script);
void       script_fu_script_reset                   (SFScript        *script,
                                                     gboolean         reset_ids);

gint       script_fu_script_collect_standard_args   (SFScript        *script,
                                                     gint             n_params,
                                                     const GimpParam *params);

gchar    * script_fu_script_get_command             (SFScript        *script);
gchar    * script_fu_script_get_command_from_params (SFScript        *script,
                                                     const GimpParam *params);


#endif /*  __SCRIPT_FU_SCRIPT__  */
