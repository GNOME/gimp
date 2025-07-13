/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainertreestore.h
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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


enum
{
  GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER,
  GIMP_CONTAINER_TREE_STORE_COLUMN_NAME,
  GIMP_CONTAINER_TREE_STORE_COLUMN_NAME_ATTRIBUTES,
  GIMP_CONTAINER_TREE_STORE_COLUMN_NAME_SENSITIVE,
  GIMP_CONTAINER_TREE_STORE_COLUMN_USER_DATA,
  GIMP_CONTAINER_TREE_STORE_N_COLUMNS
};


#define GIMP_TYPE_CONTAINER_TREE_STORE            (gimp_container_tree_store_get_type ())
#define GIMP_CONTAINER_TREE_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CONTAINER_TREE_STORE, GimpContainerTreeStore))
#define GIMP_CONTAINER_TREE_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONTAINER_TREE_STORE, GimpContainerTreeStoreClass))
#define GIMP_IS_CONTAINER_TREE_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CONTAINER_TREE_STORE))
#define GIMP_IS_CONTAINER_TREE_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONTAINER_TREE_STORE))
#define GIMP_CONTAINER_TREE_STORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CONTAINER_TREE_STORE, GimpContainerTreeStoreClass))


typedef struct _GimpContainerTreeStoreClass GimpContainerTreeStoreClass;

struct _GimpContainerTreeStore
{
  GtkTreeStore  parent_instance;
};

struct _GimpContainerTreeStoreClass
{
  GtkTreeStoreClass  parent_class;
};


GType          gimp_container_tree_store_get_type      (void) G_GNUC_CONST;

void           gimp_container_tree_store_columns_init  (GType                  *types,
                                                        gint                   *n_types);
gint           gimp_container_tree_store_columns_add   (GType                  *types,
                                                        gint                   *n_types,
                                                        GType                   type);

GtkTreeModel * gimp_container_tree_store_new           (GimpContainerView      *container_view,
                                                        gint                    n_columns,
                                                        GType                  *types);

void       gimp_container_tree_store_add_renderer_cell (GimpContainerTreeStore *store,
                                                        GtkCellRenderer        *cell,
                                                        gint                    column_number);
GimpViewRenderer *
               gimp_container_tree_store_get_renderer  (GimpContainerTreeStore *store,
                                                        GtkTreeIter            *iter);

void           gimp_container_tree_store_set_use_name  (GimpContainerTreeStore *store,
                                                        gboolean                use_name);
gboolean       gimp_container_tree_store_get_use_name  (GimpContainerTreeStore *store);

void           gimp_container_tree_store_set_context   (GimpContainerTreeStore *store,
                                                        GimpContext            *context);
GtkTreeIter *  gimp_container_tree_store_insert_item   (GimpContainerTreeStore *store,
                                                        GimpViewable           *viewable,
                                                        GtkTreeIter            *parent,
                                                        gint                    index);
void           gimp_container_tree_store_remove_item   (GimpContainerTreeStore *store,
                                                        GimpViewable           *viewable,
                                                        GtkTreeIter            *iter);
void           gimp_container_tree_store_reorder_item  (GimpContainerTreeStore *store,
                                                        GimpViewable           *viewable,
                                                        gint                    new_index,
                                                        GtkTreeIter            *iter);
gboolean       gimp_container_tree_store_rename_item   (GimpContainerTreeStore *store,
                                                        GimpViewable           *viewable,
                                                        GtkTreeIter            *iter);
void           gimp_container_tree_store_clear_items   (GimpContainerTreeStore *store);
void           gimp_container_tree_store_set_view_size (GimpContainerTreeStore *store);
