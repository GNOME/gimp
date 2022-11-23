/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 2009 Martin Nordholts <martinn@src.gnome.org>
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

#ifndef  __LIGMA_APP_TEST_UTILS_H__
#define  __LIGMA_APP_TEST_UTILS_H__


void            ligma_test_utils_set_env_to_subdir    (const gchar *root_env_var,
                                                      const gchar *subdir,
                                                      const gchar *target_env_var);
void            ligma_test_utils_set_env_to_subpath   (const gchar *root_env_var1,
                                                      const gchar *root_env_var2,
                                                      const gchar *subdir,
                                                      const gchar *target_env_var);
void            ligma_test_utils_set_ligma3_directory  (const gchar *root_env_var,
                                                      const gchar *subdir);
void            ligma_test_utils_setup_menus_path     (void);
void            ligma_test_utils_create_image         (Ligma        *ligma,
                                                      gint         width,
                                                      gint         height);
void            ligma_test_utils_synthesize_key_event (GtkWidget   *widget,
                                                      guint        keyval);
LigmaUIManager * ligma_test_utils_get_ui_manager       (Ligma        *ligma);
LigmaImage     * ligma_test_utils_create_image_from_dialog
                                                     (Ligma        *ligma);


#endif /* __LIGMA_APP_TEST_UTILS_H__ */
