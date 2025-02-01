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

#ifndef __SCRIPT_FU_SCRIPTS_H__
#define __SCRIPT_FU_SCRIPTS_H__

void      script_fu_find_scripts  (GimpPlugIn *plug_in,
                                   GList      *path);
pointer   script_fu_add_script        (scheme     *sc,
                                       pointer     a);
pointer   script_fu_add_script_filter (scheme     *sc,
                                       pointer     a);
pointer  script_fu_add_script_regular (scheme     *sc,
                                       pointer     a);
pointer   script_fu_add_menu      (scheme     *sc,
                                   pointer     a);
pointer   script_fu_add_i18n      (scheme     *sc,
                                   pointer     a);

GTree          * script_fu_scripts_load_into_tree (GimpPlugIn  *plug_in,
                                                   GList       *path);
gboolean         script_fu_scripts_are_loaded     (void);
SFScript       * script_fu_find_script            (const gchar *name);
GList          * script_fu_get_menu_list          (void);

gboolean         script_fu_is_defined             (const gchar          *name);

#endif /*  __SCRIPT_FU_SCRIPTS__  */
