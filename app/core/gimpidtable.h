/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaidtable.h
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

#ifndef __LIGMA_ID_TABLE_H__
#define __LIGMA_ID_TABLE_H__


#include "ligmaobject.h"


#define LIGMA_TYPE_ID_TABLE            (ligma_id_table_get_type ())
#define LIGMA_ID_TABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_ID_TABLE, LigmaIdTable))
#define LIGMA_ID_TABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_ID_TABLE, LigmaIdTableClass))
#define LIGMA_IS_ID_TABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_ID_TABLE))
#define LIGMA_IS_ID_TABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_ID_TABLE))
#define LIGMA_ID_TABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_ID_TABLE, LigmaIdTableClass))


typedef struct _LigmaIdTableClass   LigmaIdTableClass;
typedef struct _LigmaIdTablePrivate LigmaIdTablePrivate;

struct _LigmaIdTable
{
  LigmaObject          parent_instance;

  LigmaIdTablePrivate *priv;
};

struct _LigmaIdTableClass
{
  LigmaObjectClass  parent_class;
};


GType          ligma_id_table_get_type       (void) G_GNUC_CONST;
LigmaIdTable *  ligma_id_table_new            (void);
gint           ligma_id_table_insert         (LigmaIdTable *id_table,
                                             gpointer     data);
gint           ligma_id_table_insert_with_id (LigmaIdTable *id_table,
                                             gint         id,
                                             gpointer     data);
void           ligma_id_table_replace        (LigmaIdTable *id_table,
                                             gint         id,
                                             gpointer     data);
gpointer       ligma_id_table_lookup         (LigmaIdTable *id_table,
                                             gint         id);
gboolean       ligma_id_table_remove         (LigmaIdTable *id_table,
                                             gint         id);


#endif  /*  __LIGMA_ID_TABLE_H__  */
