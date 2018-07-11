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
#define GIMP_MODULE_DB(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_MODULE_DB, GimpModuleDB))
#define GIMP_MODULE_DB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_MODULE_DB, GimpModuleDBClass))
#define GIMP_IS_MODULE_DB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_MODULE_DB))
#define GIMP_IS_MODULE_DB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_MODULE_DB))
#define GIMP_MODULE_DB_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_MODULE_DB, GimpModuleDBClass))


typedef struct _GimpModuleDBClass GimpModuleDBClass;

struct _GimpModuleDB
{
  GObject   parent_instance;

  /*< private >*/
  GList    *modules;

  gchar    *load_inhibit;
  gboolean  verbose;
};

struct _GimpModuleDBClass
{
  GObjectClass  parent_class;

  void (* add)             (GimpModuleDB *db,
                            GimpModule   *module);
  void (* remove)          (GimpModuleDB *db,
                            GimpModule   *module);
  void (* module_modified) (GimpModuleDB *db,
                            GimpModule   *module);

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
};


GType          gimp_module_db_get_type         (void) G_GNUC_CONST;
GimpModuleDB * gimp_module_db_new              (gboolean      verbose);

void           gimp_module_db_set_load_inhibit (GimpModuleDB *db,
                                                const gchar  *load_inhibit);
const gchar  * gimp_module_db_get_load_inhibit (GimpModuleDB *db);

void           gimp_module_db_load             (GimpModuleDB *db,
                                                const gchar  *module_path);
void           gimp_module_db_refresh          (GimpModuleDB *db,
                                                const gchar  *module_path);


G_END_DECLS

#endif  /* __GIMP_MODULE_DB_H__ */
