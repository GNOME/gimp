/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpitemtree.h
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

#ifndef __GIMP_ITEM_TREE_H__
#define __GIMP_ITEM_TREE_H__


#include "gimpobject.h"


#define GIMP_TYPE_ITEM_TREE            (gimp_item_tree_get_type ())
#define GIMP_ITEM_TREE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ITEM_TREE, GimpItemTree))
#define GIMP_ITEM_TREE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ITEM_TREE, GimpItemTreeClass))
#define GIMP_IS_ITEM_TREE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ITEM_TREE))
#define GIMP_IS_ITEM_TREE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ITEM_TREE))


typedef struct _GimpItemTreeClass GimpItemTreeClass;

struct _GimpItemTree
{
  GimpObject     parent_instance;

  GimpContainer *container;
};

struct _GimpItemTreeClass
{
  GimpObjectClass  parent_class;
};


GType          gimp_item_tree_get_type         (void) G_GNUC_CONST;
GimpItemTree * gimp_item_tree_new              (GimpImage     *image,
                                                GType          container_type,
                                                GType          item_type);

GimpItem     * gimp_item_tree_get_active_item  (GimpItemTree  *tree);
void           gimp_item_tree_set_active_item  (GimpItemTree  *tree,
                                                GimpItem      *item);

GList        * gimp_item_tree_get_selected_items (GimpItemTree  *tree);
void           gimp_item_tree_set_selected_items (GimpItemTree  *tree,
                                                  GList         *items);

GimpItem     * gimp_item_tree_get_item_by_name (GimpItemTree  *tree,
                                                const gchar   *name);

gboolean       gimp_item_tree_get_insert_pos   (GimpItemTree  *tree,
                                                GimpItem      *item,
                                                GimpItem     **parent,
                                                gint          *position);

void           gimp_item_tree_add_item         (GimpItemTree  *tree,
                                                GimpItem      *item,
                                                GimpItem      *parent,
                                                gint           position);
GList        * gimp_item_tree_remove_item      (GimpItemTree  *tree,
                                                GimpItem      *item,
                                                GList         *new_selected);

gboolean       gimp_item_tree_reorder_item     (GimpItemTree  *tree,
                                                GimpItem      *item,
                                                GimpItem      *new_parent,
                                                gint           new_index,
                                                gboolean       push_undo,
                                                const gchar   *undo_desc);

void           gimp_item_tree_rename_item      (GimpItemTree  *tree,
                                                GimpItem      *item,
                                                const gchar   *new_name,
                                                gboolean       push_undo,
                                                const gchar   *undo_desc);


#endif  /*  __GIMP_ITEM_TREE_H__  */
