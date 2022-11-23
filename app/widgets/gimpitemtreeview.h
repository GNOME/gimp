/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaitemtreeview.h
 * Copyright (C) 2001-2003 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_ITEM_TREE_VIEW_H__
#define __LIGMA_ITEM_TREE_VIEW_H__


#include "ligmacontainertreeview.h"


typedef LigmaContainer * (* LigmaGetContainerFunc) (LigmaImage *image);
typedef LigmaItem      * (* LigmaGetItemFunc)      (LigmaImage *image);
typedef void            (* LigmaSetItemFunc)      (LigmaImage *image,
                                                  LigmaItem  *item);
typedef GList         * (* LigmaGetItemsFunc)     (LigmaImage *image);
typedef void            (* LigmaSetItemsFunc)     (LigmaImage *image,
                                                  GList     *items);
typedef void            (* LigmaAddItemFunc)      (LigmaImage *image,
                                                  LigmaItem  *item,
                                                  LigmaItem  *parent,
                                                  gint       index,
                                                  gboolean   push_undo);
typedef void            (* LigmaRemoveItemFunc)   (LigmaImage *image,
                                                  LigmaItem  *item,
                                                  gboolean   push_undo,
                                                  LigmaItem  *new_active);
typedef LigmaItem      * (* LigmaNewItemFunc)      (LigmaImage *image);


typedef gboolean        (* LigmaIsLockedFunc)     (LigmaItem  *item);
typedef gboolean        (* LigmaCanLockFunc)      (LigmaItem  *item);
typedef void            (* LigmaSetLockFunc)      (LigmaItem  *item,
                                                  gboolean   lock,
                                                  gboolean   push_undo);
typedef LigmaUndo      * (*LigmaUndoLockPush)      (LigmaImage     *image,
                                                  const gchar   *undo_desc,
                                                  LigmaItem      *item);


#define LIGMA_TYPE_ITEM_TREE_VIEW            (ligma_item_tree_view_get_type ())
#define LIGMA_ITEM_TREE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_ITEM_TREE_VIEW, LigmaItemTreeView))
#define LIGMA_ITEM_TREE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_ITEM_TREE_VIEW, LigmaItemTreeViewClass))
#define LIGMA_IS_ITEM_TREE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_ITEM_TREE_VIEW))
#define LIGMA_IS_ITEM_TREE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_ITEM_TREE_VIEW))
#define LIGMA_ITEM_TREE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_ITEM_TREE_VIEW, LigmaItemTreeViewClass))


typedef struct _LigmaItemTreeViewClass   LigmaItemTreeViewClass;
typedef struct _LigmaItemTreeViewPrivate LigmaItemTreeViewPrivate;

struct _LigmaItemTreeView
{
  LigmaContainerTreeView    parent_instance;

  LigmaItemTreeViewPrivate *priv;
};

struct _LigmaItemTreeViewClass
{
  LigmaContainerTreeViewClass  parent_class;

  /*  signals  */
  void (* set_image) (LigmaItemTreeView *view,
                      LigmaImage        *image);

  GType                 item_type;
  const gchar          *signal_name;

  /*  virtual functions for manipulating the image's item tree  */
  LigmaGetContainerFunc  get_container;
  LigmaGetItemsFunc      get_selected_items;
  LigmaSetItemsFunc      set_selected_items;
  LigmaAddItemFunc       add_item;
  LigmaRemoveItemFunc    remove_item;
  LigmaNewItemFunc       new_item;

  /*  action names  */
  const gchar          *action_group;
  const gchar          *activate_action;
  const gchar          *new_action;
  const gchar          *new_default_action;
  const gchar          *raise_action;
  const gchar          *raise_top_action;
  const gchar          *lower_action;
  const gchar          *lower_bottom_action;
  const gchar          *duplicate_action;
  const gchar          *delete_action;

  /*  lock content button appearance  */
  const gchar          *lock_content_icon_name;
  const gchar          *lock_content_tooltip;
  const gchar          *lock_content_help_id;

  /* lock position (translation and transformation) button appearance */
  const gchar          *lock_position_icon_name;
  const gchar          *lock_position_tooltip;
  const gchar          *lock_position_help_id;

  /* lock visibility button appearance */
  const gchar          *lock_visibility_icon_name;
  const gchar          *lock_visibility_tooltip;
  const gchar          *lock_visibility_help_id;
};


GType       ligma_item_tree_view_get_type          (void) G_GNUC_CONST;

GtkWidget * ligma_item_tree_view_new               (GType             view_type,
                                                   gint              view_size,
                                                   gint              view_border_width,
                                                   gboolean          multiple_selection,
                                                   LigmaImage        *image,
                                                   LigmaMenuFactory  *menu_facotry,
                                                   const gchar      *menu_identifier,
                                                   const gchar      *ui_identifier);

void        ligma_item_tree_view_set_image         (LigmaItemTreeView *view,
                                                   LigmaImage        *image);
LigmaImage * ligma_item_tree_view_get_image         (LigmaItemTreeView *view);

void        ligma_item_tree_view_add_options       (LigmaItemTreeView *view,
                                                   const gchar      *label,
                                                   GtkWidget        *options);
void        ligma_item_tree_view_add_lock          (LigmaItemTreeView *view,
                                                   const gchar      *icon_name,
                                                   LigmaIsLockedFunc  is_locked,
                                                   LigmaCanLockFunc   can_lock,
                                                   LigmaSetLockFunc   lock,
                                                   LigmaUndoLockPush  undo_push,
                                                   const gchar      *signal_name,
                                                   LigmaUndoType      undo_type,
                                                   LigmaUndoType      group_undo_type,
                                                   const gchar      *undo_lock_label,
                                                   const gchar      *undo_unlock_label,
                                                   const gchar      *undo_exclusive_desc,
                                                   const gchar      *tooltip,
                                                   const gchar      *help_id);
void        ligma_item_tree_view_blink_lock        (LigmaItemTreeView *view,
                                                   LigmaItem         *item);

GtkWidget * ligma_item_tree_view_get_new_button    (LigmaItemTreeView *view);
GtkWidget * ligma_item_tree_view_get_delete_button (LigmaItemTreeView *view);

gint        ligma_item_tree_view_get_drop_index    (LigmaItemTreeView *view,
                                                   LigmaViewable     *dest_viewable,
                                                   GtkTreeViewDropPosition drop_pos,
                                                   LigmaViewable    **parent);


#endif  /*  __LIGMA_ITEM_TREE_VIEW_H__  */
