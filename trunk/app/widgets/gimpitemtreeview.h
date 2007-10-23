/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpitemtreeview.h
 * Copyright (C) 2001-2003 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_ITEM_TREE_VIEW_H__
#define __GIMP_ITEM_TREE_VIEW_H__


#include "gimpcontainertreeview.h"


typedef GimpContainer * (* GimpGetContainerFunc) (const GimpImage *image);
typedef GimpItem      * (* GimpGetItemFunc)      (const GimpImage *image);
typedef void            (* GimpSetItemFunc)      (GimpImage       *image,
                                                  GimpItem        *item);
typedef void            (* GimpReorderItemFunc)  (GimpImage       *image,
                                                  GimpItem        *item,
                                                  gint             new_index,
                                                  gboolean         push_undo,
                                                  const gchar     *undo_desc);
typedef void            (* GimpAddItemFunc)      (GimpImage       *image,
                                                  GimpItem        *item,
                                                  gint             index);
typedef void            (* GimpRemoveItemFunc)   (GimpImage       *image,
                                                  GimpItem        *item);
typedef GimpItem      * (* GimpNewItemFunc)      (GimpImage       *image);


#define GIMP_TYPE_ITEM_TREE_VIEW            (gimp_item_tree_view_get_type ())
#define GIMP_ITEM_TREE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ITEM_TREE_VIEW, GimpItemTreeView))
#define GIMP_ITEM_TREE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ITEM_TREE_VIEW, GimpItemTreeViewClass))
#define GIMP_IS_ITEM_TREE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ITEM_TREE_VIEW))
#define GIMP_IS_ITEM_TREE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ITEM_TREE_VIEW))
#define GIMP_ITEM_TREE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ITEM_TREE_VIEW, GimpItemTreeViewClass))


typedef struct _GimpItemTreeViewClass  GimpItemTreeViewClass;

struct _GimpItemTreeView
{
  GimpContainerTreeView  parent_instance;

  GimpImage             *image;

  GtkWidget             *edit_button;
  GtkWidget             *new_button;
  GtkWidget             *raise_button;
  GtkWidget             *lower_button;
  GtkWidget             *duplicate_button;
  GtkWidget             *delete_button;

  gint                   model_column_visible;
  gint                   model_column_linked;
  GtkCellRenderer       *eye_cell;
  GtkCellRenderer       *chain_cell;

  /*< private >*/
  GQuark                 visible_changed_handler_id;
  GQuark                 linked_changed_handler_id;
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
  GimpGetItemFunc       get_active_item;
  GimpSetItemFunc       set_active_item;
  GimpReorderItemFunc   reorder_item;
  GimpAddItemFunc       add_item;
  GimpRemoveItemFunc    remove_item;
  GimpNewItemFunc       new_item;

  /*  action names  */
  const gchar          *action_group;
  const gchar          *activate_action;
  const gchar          *edit_action;
  const gchar          *new_action;
  const gchar          *new_default_action;
  const gchar          *raise_action;
  const gchar          *raise_top_action;
  const gchar          *lower_action;
  const gchar          *lower_bottom_action;
  const gchar          *duplicate_action;
  const gchar          *delete_action;

  /*  undo descriptions  */
  const gchar          *reorder_desc;
};


GType       gimp_item_tree_view_get_type  (void) G_GNUC_CONST;

GtkWidget * gimp_item_tree_view_new       (GType             view_type,
                                           gint              view_size,
                                           gint              view_border_width,
                                           GimpImage        *image,
                                           GimpMenuFactory  *menu_facotry,
                                           const gchar      *menu_identifier,
                                           const gchar      *ui_identifier);

void        gimp_item_tree_view_set_image (GimpItemTreeView *view,
                                           GimpImage        *image);
GimpImage * gimp_item_tree_view_get_image (GimpItemTreeView *view);


#endif  /*  __GIMP_ITEM_TREE_VIEW_H__  */
