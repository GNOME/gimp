/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmainterpreterdb.h
 * (C) 2005 Manish Singh <yosh@ligma.org>
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

#ifndef __LIGMA_INTERPRETER_DB_H__
#define __LIGMA_INTERPRETER_DB_H__


#define LIGMA_TYPE_INTERPRETER_DB            (ligma_interpreter_db_get_type ())
#define LIGMA_INTERPRETER_DB(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_INTERPRETER_DB, LigmaInterpreterDB))
#define LIGMA_INTERPRETER_DB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_INTERPRETER_DB, LigmaInterpreterDBClass))
#define LIGMA_IS_INTERPRETER_DB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_INTERPRETER_DB))
#define LIGMA_IS_INTERPRETER_DB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_INTERPRETER_DB))
#define LIGMA_INTERPRETER_DB_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_INTERPRETER_DB, LigmaInterpreterDBClass))


typedef struct _LigmaInterpreterDBClass LigmaInterpreterDBClass;

struct _LigmaInterpreterDB
{
  GObject     parent_instance;

  gboolean    verbose;

  GHashTable *programs;

  GSList     *magics;
  GHashTable *magic_names;

  GHashTable *extensions;
  GHashTable *extension_names;
};

struct _LigmaInterpreterDBClass
{
  GObjectClass  parent_class;
};


GType               ligma_interpreter_db_get_type (void) G_GNUC_CONST;
LigmaInterpreterDB * ligma_interpreter_db_new      (gboolean            verbose);

void                ligma_interpreter_db_load     (LigmaInterpreterDB  *db,
                                                  GList              *path);

void                ligma_interpreter_db_clear    (LigmaInterpreterDB  *db);

gchar             * ligma_interpreter_db_resolve  (LigmaInterpreterDB  *db,
                                                  const gchar        *program_path,
                                                  gchar             **interp_arg);
gchar       * ligma_interpreter_db_get_extensions (LigmaInterpreterDB  *db);


#endif /* __LIGMA_INTERPRETER_DB_H__ */
