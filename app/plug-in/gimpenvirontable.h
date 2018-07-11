/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpenvirontable.h
 * (C) 2002 Manish Singh <yosh@gimp.org>
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

#ifndef __GIMP_ENVIRON_TABLE_H__
#define __GIMP_ENVIRON_TABLE_H__


#define GIMP_TYPE_ENVIRON_TABLE            (gimp_environ_table_get_type ())
#define GIMP_ENVIRON_TABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ENVIRON_TABLE, GimpEnvironTable))
#define GIMP_ENVIRON_TABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ENVIRON_TABLE, GimpEnvironTableClass))
#define GIMP_IS_ENVIRON_TABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ENVIRON_TABLE))
#define GIMP_IS_ENVIRON_TABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ENVIRON_TABLE))
#define GIMP_ENVIRON_TABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ENVIRON_TABLE, GimpEnvironTableClass))


typedef struct _GimpEnvironTableClass GimpEnvironTableClass;

struct _GimpEnvironTable
{
  GObject      parent_instance;

  gboolean     verbose;

  GHashTable  *vars;
  GHashTable  *internal;

  gchar      **envp;
};

struct _GimpEnvironTableClass
{
  GObjectClass  parent_class;
};


GType               gimp_environ_table_get_type  (void) G_GNUC_CONST;
GimpEnvironTable  * gimp_environ_table_new       (gboolean          verbose);

void                gimp_environ_table_load      (GimpEnvironTable *environ_table,
                                                  GList            *path);

void                gimp_environ_table_add       (GimpEnvironTable *environ_table,
                                                  const gchar      *name,
                                                  const gchar      *value,
                                                  const gchar      *separator);
void                gimp_environ_table_remove    (GimpEnvironTable *environ_table,
                                                  const gchar      *name);

void                gimp_environ_table_clear     (GimpEnvironTable *environ_table);
void                gimp_environ_table_clear_all (GimpEnvironTable *environ_table);

gchar            ** gimp_environ_table_get_envp  (GimpEnvironTable *environ_table);


#endif /* __GIMP_ENVIRON_TABLE_H__ */
