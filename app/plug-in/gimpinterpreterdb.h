/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpinterpreterdb.h
 * (C) 2005 Manish Singh <yosh@gimp.org>
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

#pragma once


#define GIMP_TYPE_INTERPRETER_DB            (gimp_interpreter_db_get_type ())
#define GIMP_INTERPRETER_DB(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_INTERPRETER_DB, GimpInterpreterDB))
#define GIMP_INTERPRETER_DB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_INTERPRETER_DB, GimpInterpreterDBClass))
#define GIMP_IS_INTERPRETER_DB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_INTERPRETER_DB))
#define GIMP_IS_INTERPRETER_DB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_INTERPRETER_DB))
#define GIMP_INTERPRETER_DB_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_INTERPRETER_DB, GimpInterpreterDBClass))


typedef struct _GimpInterpreterDBClass GimpInterpreterDBClass;

struct _GimpInterpreterDB
{
  GObject     parent_instance;

  gboolean    verbose;

  GHashTable *programs;

  GSList     *magics;
  GHashTable *magic_names;

  GHashTable *extensions;
  GHashTable *extension_names;
};

struct _GimpInterpreterDBClass
{
  GObjectClass  parent_class;
};


GType               gimp_interpreter_db_get_type (void) G_GNUC_CONST;
GimpInterpreterDB * gimp_interpreter_db_new      (gboolean            verbose);

void                gimp_interpreter_db_load     (GimpInterpreterDB  *db,
                                                  GList              *path);

void                gimp_interpreter_db_clear    (GimpInterpreterDB  *db);

gchar             * gimp_interpreter_db_resolve  (GimpInterpreterDB  *db,
                                                  const gchar        *program_path,
                                                  gchar             **interp_arg);
gchar       * gimp_interpreter_db_get_extensions (GimpInterpreterDB  *db);
