/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_MODULE_H_INSIDE__) && !defined (GIMP_MODULE_COMPILATION)
#error "Only <libgimpmodule/gimpmodule.h> can be included directly."
#endif

#ifndef __GIMP_MODULE_DB_H__
#define __GIMP_MODULE_DB_H__

G_BEGIN_DECLS

#define GIMP_TYPE_MODULE_DB            (gimp_module_db_get_type ())
G_DECLARE_FINAL_TYPE (GimpModuleDB, gimp_module_db, GIMP, MODULE_DB, GObject)


GimpModuleDB * gimp_module_db_new              (gboolean      verbose);

void           gimp_module_db_set_verbose      (GimpModuleDB *db,
                                                gboolean      verbose);
gboolean       gimp_module_db_get_verbose      (GimpModuleDB *db);

void           gimp_module_db_set_load_inhibit (GimpModuleDB *db,
                                                const gchar  *load_inhibit);
const gchar  * gimp_module_db_get_load_inhibit (GimpModuleDB *db);

void           gimp_module_db_load             (GimpModuleDB *db,
                                                const gchar  *module_path);
void           gimp_module_db_refresh          (GimpModuleDB *db,
                                                const gchar  *module_path);


G_END_DECLS

#endif  /* __GIMP_MODULE_DB_H__ */
