/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaenvirontable.h
 * (C) 2002 Manish Singh <yosh@ligma.org>
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

#ifndef __LIGMA_ENVIRON_TABLE_H__
#define __LIGMA_ENVIRON_TABLE_H__


#define LIGMA_TYPE_ENVIRON_TABLE            (ligma_environ_table_get_type ())
#define LIGMA_ENVIRON_TABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_ENVIRON_TABLE, LigmaEnvironTable))
#define LIGMA_ENVIRON_TABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_ENVIRON_TABLE, LigmaEnvironTableClass))
#define LIGMA_IS_ENVIRON_TABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_ENVIRON_TABLE))
#define LIGMA_IS_ENVIRON_TABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_ENVIRON_TABLE))
#define LIGMA_ENVIRON_TABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_ENVIRON_TABLE, LigmaEnvironTableClass))


typedef struct _LigmaEnvironTableClass LigmaEnvironTableClass;

struct _LigmaEnvironTable
{
  GObject      parent_instance;

  gboolean     verbose;

  GHashTable  *vars;
  GHashTable  *internal;

  gchar      **envp;
};

struct _LigmaEnvironTableClass
{
  GObjectClass  parent_class;
};


GType               ligma_environ_table_get_type  (void) G_GNUC_CONST;
LigmaEnvironTable  * ligma_environ_table_new       (gboolean          verbose);

void                ligma_environ_table_load      (LigmaEnvironTable *environ_table,
                                                  GList            *path);

void                ligma_environ_table_add       (LigmaEnvironTable *environ_table,
                                                  const gchar      *name,
                                                  const gchar      *value,
                                                  const gchar      *separator);
void                ligma_environ_table_remove    (LigmaEnvironTable *environ_table,
                                                  const gchar      *name);

void                ligma_environ_table_clear     (LigmaEnvironTable *environ_table);
void                ligma_environ_table_clear_all (LigmaEnvironTable *environ_table);

gchar            ** ligma_environ_table_get_envp  (LigmaEnvironTable *environ_table);


#endif /* __LIGMA_ENVIRON_TABLE_H__ */
