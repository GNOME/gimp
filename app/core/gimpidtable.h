/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpidtable.h
 * Copyright (C) 2011 Martin Nordholts <martinn@src.gnome.org>
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

#ifndef __GIMP_ID_TABLE_H__
#define __GIMP_ID_TABLE_H__


#include "gimpobject.h"


#define GIMP_TYPE_ID_TABLE            (gimp_id_table_get_type ())
#define GIMP_ID_TABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ID_TABLE, GimpIdTable))
#define GIMP_ID_TABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ID_TABLE, GimpIdTableClass))
#define GIMP_IS_ID_TABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ID_TABLE))
#define GIMP_IS_ID_TABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ID_TABLE))
#define GIMP_ID_TABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ID_TABLE, GimpIdTableClass))


typedef struct _GimpIdTableClass   GimpIdTableClass;
typedef struct _GimpIdTablePrivate GimpIdTablePrivate;

struct _GimpIdTable
{
  GimpObject          parent_instance;

  GimpIdTablePrivate *priv;
};

struct _GimpIdTableClass
{
  GimpObjectClass  parent_class;
};


GType          gimp_id_table_get_type       (void) G_GNUC_CONST;
GimpIdTable *  gimp_id_table_new            (void);
gint           gimp_id_table_insert         (GimpIdTable *id_table,
                                             gpointer     data);
gint           gimp_id_table_insert_with_id (GimpIdTable *id_table,
                                             gint         id,
                                             gpointer     data);
void           gimp_id_table_replace        (GimpIdTable *id_table,
                                             gint         id,
                                             gpointer     data);
gpointer       gimp_id_table_lookup         (GimpIdTable *id_table,
                                             gint         id);
gboolean       gimp_id_table_remove         (GimpIdTable *id_table,
                                             gint         id);


#endif  /*  __GIMP_ID_TABLE_H__  */
