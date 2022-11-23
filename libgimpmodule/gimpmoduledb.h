/* LIBLIGMA - The LIGMA Library
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

#if !defined (__LIGMA_MODULE_H_INSIDE__) && !defined (LIGMA_MODULE_COMPILATION)
#error "Only <libligmamodule/ligmamodule.h> can be included directly."
#endif

#ifndef __LIGMA_MODULE_DB_H__
#define __LIGMA_MODULE_DB_H__

G_BEGIN_DECLS


#define LIGMA_TYPE_MODULE_DB            (ligma_module_db_get_type ())
#define LIGMA_MODULE_DB(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_MODULE_DB, LigmaModuleDB))
#define LIGMA_MODULE_DB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_MODULE_DB, LigmaModuleDBClass))
#define LIGMA_IS_MODULE_DB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_MODULE_DB))
#define LIGMA_IS_MODULE_DB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_MODULE_DB))
#define LIGMA_MODULE_DB_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_MODULE_DB, LigmaModuleDBClass))


typedef struct _LigmaModuleDBPrivate LigmaModuleDBPrivate;
typedef struct _LigmaModuleDBClass   LigmaModuleDBClass;

struct _LigmaModuleDB
{
  GObject              parent_instance;

  LigmaModuleDBPrivate *priv;
};

struct _LigmaModuleDBClass
{
  GObjectClass  parent_class;

  void (* add)             (LigmaModuleDB *db,
                            LigmaModule   *module);
  void (* remove)          (LigmaModuleDB *db,
                            LigmaModule   *module);
  void (* module_modified) (LigmaModuleDB *db,
                            LigmaModule   *module);

  /* Padding for future expansion */
  void (* _ligma_reserved1) (void);
  void (* _ligma_reserved2) (void);
  void (* _ligma_reserved3) (void);
  void (* _ligma_reserved4) (void);
  void (* _ligma_reserved5) (void);
  void (* _ligma_reserved6) (void);
  void (* _ligma_reserved7) (void);
  void (* _ligma_reserved8) (void);
};


GType          ligma_module_db_get_type         (void) G_GNUC_CONST;
LigmaModuleDB * ligma_module_db_new              (gboolean      verbose);

GList        * ligma_module_db_get_modules      (LigmaModuleDB *db);

void           ligma_module_db_set_verbose      (LigmaModuleDB *db,
                                                gboolean      verbose);
gboolean       ligma_module_db_get_verbose      (LigmaModuleDB *db);

void           ligma_module_db_set_load_inhibit (LigmaModuleDB *db,
                                                const gchar  *load_inhibit);
const gchar  * ligma_module_db_get_load_inhibit (LigmaModuleDB *db);

void           ligma_module_db_load             (LigmaModuleDB *db,
                                                const gchar  *module_path);
void           ligma_module_db_refresh          (LigmaModuleDB *db,
                                                const gchar  *module_path);


G_END_DECLS

#endif  /* __LIGMA_MODULE_DB_H__ */
