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

#ifndef __SCRIPT_FU_LIB_H__
#define __SCRIPT_FU_LIB_H__

gboolean     script_fu_extension_is_busy         (void);

GList *      script_fu_search_path               (void);
void         script_fu_find_and_register_scripts (GimpPlugIn     *plugin,
                                                  GList          *paths);

void         script_fu_set_run_mode              (GimpRunMode     run_mode);
void         script_fu_init_embedded_interpreter (GList          *paths,
                                                  gboolean        allow_register,
                                                  GimpRunMode     run_mode);

void         script_fu_set_print_flag            (gboolean        should_print);
void         script_fu_redirect_output_to_gstr   (GString        *output);
void         script_fu_redirect_output_to_stdout (void);
void         script_fu_print_welcome             (void);

gboolean     script_fu_interpret_string          (const gchar     *text);
const gchar *script_fu_get_success_msg           (void);
const gchar *script_fu_get_error_msg             (void);
GError      *script_fu_get_gerror                (void);

void         script_fu_run_read_eval_print_loop  (void);

void         script_fu_register_quit_callback         (void (*func) (void));
void         script_fu_register_post_command_callback (void (*func) (void));

GimpProcedure *script_fu_find_scripts_create_PDB_proc_plugin (GimpPlugIn  *plug_in,
                                                              GList       *paths,
                                                              const gchar *name);
GList         *script_fu_find_scripts_list_proc_names        (GimpPlugIn  *plug_in,
                                                              GList       *paths);

#endif /* __SCRIPT_FU_LIB_H__ */
