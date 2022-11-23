/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmaitemtree.h
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

#ifndef __LIGMA_ITEM_TREE_H__
#define __LIGMA_ITEM_TREE_H__


#include "ligmaobject.h"


#define LIGMA_TYPE_ITEM_TREE            (ligma_item_tree_get_type ())
#define LIGMA_ITEM_TREE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_ITEM_TREE, LigmaItemTree))
#define LIGMA_ITEM_TREE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_ITEM_TREE, LigmaItemTreeClass))
#define LIGMA_IS_ITEM_TREE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_ITEM_TREE))
#define LIGMA_IS_ITEM_TREE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_ITEM_TREE))


typedef struct _LigmaItemTreeClass LigmaItemTreeClass;

struct _LigmaItemTree
{
  LigmaObject     parent_instance;

  LigmaContainer *container;
};

struct _LigmaItemTreeClass
{
  LigmaObjectClass  parent_class;
};


GType          ligma_item_tree_get_type         (void) G_GNUC_CONST;
LigmaItemTree * ligma_item_tree_new              (LigmaImage     *image,
                                                GType          container_type,
                                                GType          item_type);

LigmaItem     * ligma_item_tree_get_active_item  (LigmaItemTree  *tree);
void           ligma_item_tree_set_active_item  (LigmaItemTree  *tree,
                                                LigmaItem      *item);

GList        * ligma_item_tree_get_selected_items (LigmaItemTree  *tree);
void           ligma_item_tree_set_selected_items (LigmaItemTree  *tree,
                                                  GList         *items);

LigmaItem     * ligma_item_tree_get_item_by_name (LigmaItemTree  *tree,
                                                const gchar   *name);

gboolean       ligma_item_tree_get_insert_pos   (LigmaItemTree  *tree,
                                                LigmaItem      *item,
                                                LigmaItem     **parent,
                                                gint          *position);

void           ligma_item_tree_add_item         (LigmaItemTree  *tree,
                                                LigmaItem      *item,
                                                LigmaItem      *parent,
                                                gint           position);
GList        * ligma_item_tree_remove_item      (LigmaItemTree  *tree,
                                                LigmaItem      *item,
                                                GList         *new_selected);

gboolean       ligma_item_tree_reorder_item     (LigmaItemTree  *tree,
                                                LigmaItem      *item,
                                                LigmaItem      *new_parent,
                                                gint           new_index,
                                                gboolean       push_undo,
                                                const gchar   *undo_desc);

void           ligma_item_tree_rename_item      (LigmaItemTree  *tree,
                                                LigmaItem      *item,
                                                const gchar   *new_name,
                                                gboolean       push_undo,
                                                const gchar   *undo_desc);


#endif  /*  __LIGMA_ITEM_TREE_H__  */
