/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpitemlistview.h
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_ITEM_LIST_VIEW_H__
#define __GIMP_ITEM_LIST_VIEW_H__


#include "gimpcontainerlistview.h"


typedef GimpContainer * (* GimpGetContainerFunc) (const GimpImage    *gimage);
typedef GimpViewable  * (* GimpGetItemFunc)      (const GimpImage    *gimage);
typedef void            (* GimpSetItemFunc)      (GimpImage          *gimage,
                                                  GimpViewable       *viewable);
typedef void            (* GimpReorderItemFunc)  (GimpImage          *gimage,
                                                  GimpViewable       *viewable,
                                                  gint                new_index,
                                                  gboolean            push_undo);
typedef void            (* GimpAddItemFunc)      (GimpImage          *gimage,
                                                  GimpViewable       *viewable,
                                                  gint                index);
typedef void            (* GimpRemoveItemFunc)   (GimpImage          *gimage,
                                                  GimpViewable       *viewable);
typedef GimpViewable  * (* GimpCopyItemFunc)     (GimpViewable       *viewable,
                                                  GType               new_type,
                                                  gboolean            add_alpha);
typedef GimpViewable  * (* GimpConvertItemFunc)  (GimpViewable       *viewable,
                                                  GimpImage          *dest_gimage);

typedef void            (* GimpNewItemFunc)      (GimpImage          *gimage,
                                                  GimpViewable       *template);
typedef void            (* GimpEditItemFunc)     (GimpViewable       *viewable);
typedef void            (* GimpActivateItemFunc) (GimpViewable       *viewable);


#define GIMP_TYPE_ITEM_LIST_VIEW            (gimp_item_list_view_get_type ())
#define GIMP_ITEM_LIST_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ITEM_LIST_VIEW, GimpItemListView))
#define GIMP_ITEM_LIST_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ITEM_LIST_VIEW, GimpItemListViewClass))
#define GIMP_IS_ITEM_LIST_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ITEM_LIST_VIEW))
#define GIMP_IS_ITEM_LIST_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ITEM_LIST_VIEW))
#define GIMP_ITEM_LIST_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ITEM_LIST_VIEW, GimpItemListViewClass))


typedef struct _GimpItemListViewClass  GimpItemListViewClass;

struct _GimpItemListView
{
  GimpContainerListView  parent_instance;

  GimpImage             *gimage;

  GType                  item_type;
  gchar                 *signal_name;

  GimpGetContainerFunc   get_container_func;
  GimpGetItemFunc        get_item_func;
  GimpSetItemFunc        set_item_func;
  GimpReorderItemFunc    reorder_item_func;
  GimpAddItemFunc        add_item_func;
  GimpRemoveItemFunc     remove_item_func;
  GimpCopyItemFunc       copy_item_func;
  GimpConvertItemFunc    convert_item_func;

  GimpNewItemFunc        new_item_func;
  GimpEditItemFunc       edit_item_func;
  GimpActivateItemFunc   activate_item_func;

  GimpItemFactory       *item_factory;

  GtkWidget             *new_button;
  GtkWidget             *raise_button;
  GtkWidget             *lower_button;
  GtkWidget             *duplicate_button;
  GtkWidget             *edit_button;
  GtkWidget             *delete_button;
};

struct _GimpItemListViewClass
{
  GimpContainerListViewClass  parent_class;

  void (* set_image) (GimpItemListView *view,
		      GimpImage        *gimage);
};


GType       gimp_item_list_view_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_item_list_view_new      (gint                  preview_size,
                                          GimpImage            *gimage,
                                          GType                 item_type,
                                          const gchar          *signal_name,
                                          GimpGetContainerFunc  get_container_func,
                                          GimpGetItemFunc       get_item_func,
                                          GimpSetItemFunc       set_item_func,
                                          GimpReorderItemFunc   reorder_item_func,
                                          GimpAddItemFunc       add_item_func,
                                          GimpRemoveItemFunc    remove_item_func,
                                          GimpCopyItemFunc      copy_item_func,
                                          GimpConvertItemFunc   convert_item_func,
                                          GimpNewItemFunc       new_item_func,
                                          GimpEditItemFunc      edit_item_func,
                                          GimpActivateItemFunc  activate_item_func,
                                          GimpItemFactory      *item_facotry);

void       gimp_item_list_view_set_image (GimpItemListView     *view,
                                          GimpImage            *gimage);


#endif  /*  __GIMP_ITEM_LIST_VIEW_H__  */
