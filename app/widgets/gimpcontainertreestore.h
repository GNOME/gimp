/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacontainertreestore.h
 * Copyright (C) 2010 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_CONTAINER_TREE_STORE_H__
#define __LIGMA_CONTAINER_TREE_STORE_H__


enum
{
  LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER,
  LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME,
  LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME_ATTRIBUTES,
  LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME_SENSITIVE,
  LIGMA_CONTAINER_TREE_STORE_COLUMN_USER_DATA,
  LIGMA_CONTAINER_TREE_STORE_N_COLUMNS
};


#define LIGMA_TYPE_CONTAINER_TREE_STORE            (ligma_container_tree_store_get_type ())
#define LIGMA_CONTAINER_TREE_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CONTAINER_TREE_STORE, LigmaContainerTreeStore))
#define LIGMA_CONTAINER_TREE_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CONTAINER_TREE_STORE, LigmaContainerTreeStoreClass))
#define LIGMA_IS_CONTAINER_TREE_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CONTAINER_TREE_STORE))
#define LIGMA_IS_CONTAINER_TREE_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CONTAINER_TREE_STORE))
#define LIGMA_CONTAINER_TREE_STORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CONTAINER_TREE_STORE, LigmaContainerTreeStoreClass))


typedef struct _LigmaContainerTreeStoreClass LigmaContainerTreeStoreClass;

struct _LigmaContainerTreeStore
{
  GtkTreeStore  parent_instance;
};

struct _LigmaContainerTreeStoreClass
{
  GtkTreeStoreClass  parent_class;
};


GType          ligma_container_tree_store_get_type      (void) G_GNUC_CONST;

void           ligma_container_tree_store_columns_init  (GType                  *types,
                                                        gint                   *n_types);
gint           ligma_container_tree_store_columns_add   (GType                  *types,
                                                        gint                   *n_types,
                                                        GType                   type);

GtkTreeModel * ligma_container_tree_store_new           (LigmaContainerView      *container_view,
                                                        gint                    n_columns,
                                                        GType                  *types);

void       ligma_container_tree_store_add_renderer_cell (LigmaContainerTreeStore *store,
                                                        GtkCellRenderer        *cell,
                                                        gint                    column_number);
LigmaViewRenderer *
               ligma_container_tree_store_get_renderer  (LigmaContainerTreeStore *store,
                                                        GtkTreeIter            *iter);

void           ligma_container_tree_store_set_use_name  (LigmaContainerTreeStore *store,
                                                        gboolean                use_name);
gboolean       ligma_container_tree_store_get_use_name  (LigmaContainerTreeStore *store);

void           ligma_container_tree_store_set_context   (LigmaContainerTreeStore *store,
                                                        LigmaContext            *context);
GtkTreeIter *  ligma_container_tree_store_insert_item   (LigmaContainerTreeStore *store,
                                                        LigmaViewable           *viewable,
                                                        GtkTreeIter            *parent,
                                                        gint                    index);
void           ligma_container_tree_store_remove_item   (LigmaContainerTreeStore *store,
                                                        LigmaViewable           *viewable,
                                                        GtkTreeIter            *iter);
void           ligma_container_tree_store_reorder_item  (LigmaContainerTreeStore *store,
                                                        LigmaViewable           *viewable,
                                                        gint                    new_index,
                                                        GtkTreeIter            *iter);
gboolean       ligma_container_tree_store_rename_item   (LigmaContainerTreeStore *store,
                                                        LigmaViewable           *viewable,
                                                        GtkTreeIter            *iter);
void           ligma_container_tree_store_clear_items   (LigmaContainerTreeStore *store);
void           ligma_container_tree_store_set_view_size (LigmaContainerTreeStore *store);


#endif  /*  __LIGMA_CONTAINER_TREE_STORE_H__  */
