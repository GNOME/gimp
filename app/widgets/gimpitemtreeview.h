/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpitemtreeview.h
 * Copyright (C) 2001-2003 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_ITEM_TREE_VIEW_H__
#define __GIMP_ITEM_TREE_VIEW_H__


#include "gimpcontainertreeview.h"


typedef GimpContainer * (* GimpGetContainerFunc) (GimpImage *image);
typedef GimpItem      * (* GimpGetItemFunc)      (GimpImage *image);
typedef void            (* GimpSetItemFunc)      (GimpImage *image,
                                                  GimpItem  *item);
typedef GList         * (* GimpGetItemsFunc)     (GimpImage *image);
typedef void            (* GimpSetItemsFunc)     (GimpImage *image,
                                                  GList     *items);
typedef void            (* GimpAddItemFunc)      (GimpImage *image,
                                                  GimpItem  *item,
                                                  GimpItem  *parent,
                                                  gint       index,
                                                  gboolean   push_undo);
typedef void            (* GimpRemoveItemFunc)   (GimpImage *image,
                                                  GimpItem  *item,
                                                  gboolean   push_undo,
                                                  GimpItem  *new_active);
typedef GimpItem      * (* GimpNewItemFunc)      (GimpImage *image);


typedef gboolean        (* GimpIsLockedFunc)     (GimpItem  *item);
typedef gboolean        (* GimpCanLockFunc)      (GimpItem  *item);
typedef void            (* GimpSetLockFunc)      (GimpItem  *item,
                                                  gboolean   lock,
                                                  gboolean   push_undo);
typedef GimpUndo      * (*GimpUndoLockPush)      (GimpImage     *image,
                                                  const gchar   *undo_desc,
                                                  GimpItem      *item);


#define GIMP_TYPE_ITEM_TREE_VIEW            (gimp_item_tree_view_get_type ())
#define GIMP_ITEM_TREE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ITEM_TREE_VIEW, GimpItemTreeView))
#define GIMP_ITEM_TREE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ITEM_TREE_VIEW, GimpItemTreeViewClass))
#define GIMP_IS_ITEM_TREE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ITEM_TREE_VIEW))
#define GIMP_IS_ITEM_TREE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ITEM_TREE_VIEW))
#define GIMP_ITEM_TREE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ITEM_TREE_VIEW, GimpItemTreeViewClass))


typedef struct _GimpItemTreeViewClass   GimpItemTreeViewClass;
typedef struct _GimpItemTreeViewPrivate GimpItemTreeViewPrivate;

struct _GimpItemTreeView
{
  GimpContainerTreeView    parent_instance;

  GimpItemTreeViewPrivate *priv;
};

struct _GimpItemTreeViewClass
{
  GimpContainerTreeViewClass  parent_class;

  /*  signals  */
  void (* set_image) (GimpItemTreeView *view,
                      GimpImage        *image);

  GType                 item_type;
  const gchar          *signal_name;

  /*  virtual functions for manipulating the image's item tree  */
  GimpGetContainerFunc  get_container;
  GimpGetItemsFunc      get_selected_items;
  GimpSetItemsFunc      set_selected_items;
  GimpAddItemFunc       add_item;
  GimpRemoveItemFunc    remove_item;
  GimpNewItemFunc       new_item;

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

  /* actions */

  const gchar          *move_cursor_up_action;
  const gchar          *move_cursor_down_action;
  const gchar          *move_cursor_up_flat_action;
  const gchar          *move_cursor_down_flat_action;
  const gchar          *move_cursor_start_action;
  const gchar          *move_cursor_end_action;
};


GType       gimp_item_tree_view_get_type          (void) G_GNUC_CONST;

GtkWidget * gimp_item_tree_view_new               (GType             view_type,
                                                   gint              view_size,
                                                   gint              view_border_width,
                                                   gboolean          multiple_selection,
                                                   GimpImage        *image,
                                                   GimpMenuFactory  *menu_facotry,
                                                   const gchar      *menu_identifier,
                                                   const gchar      *ui_identifier);

void        gimp_item_tree_view_set_image         (GimpItemTreeView *view,
                                                   GimpImage        *image);
GimpImage * gimp_item_tree_view_get_image         (GimpItemTreeView *view);

void        gimp_item_tree_view_add_options       (GimpItemTreeView *view,
                                                   const gchar      *label,
                                                   GtkWidget        *options);
void        gimp_item_tree_view_add_lock          (GimpItemTreeView *view,
                                                   const gchar      *icon_name,
                                                   GimpIsLockedFunc  is_locked,
                                                   GimpCanLockFunc   can_lock,
                                                   GimpSetLockFunc   lock,
                                                   GimpUndoLockPush  undo_push,
                                                   const gchar      *signal_name,
                                                   GimpUndoType      undo_type,
                                                   GimpUndoType      group_undo_type,
                                                   const gchar      *undo_lock_label,
                                                   const gchar      *undo_unlock_label,
                                                   const gchar      *undo_exclusive_desc,
                                                   const gchar      *tooltip,
                                                   const gchar      *help_id);
void        gimp_item_tree_view_blink_lock        (GimpItemTreeView *view,
                                                   GimpItem         *item);

GtkWidget * gimp_item_tree_view_get_new_button    (GimpItemTreeView *view);
GtkWidget * gimp_item_tree_view_get_delete_button (GimpItemTreeView *view);

gint        gimp_item_tree_view_get_drop_index    (GimpItemTreeView *view,
                                                   GimpViewable     *dest_viewable,
                                                   GtkTreeViewDropPosition drop_pos,
                                                   GimpViewable    **parent);


#endif  /*  __GIMP_ITEM_TREE_VIEW_H__  */
