/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimplistitem.h
 * Copyright (C) 2001 Michael Natterer
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

#ifndef __GIMP_LIST_ITEM_H__
#define __GIMP_LIST_ITEM_H__


#include <gtk/gtklistitem.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GIMP_TYPE_LIST_ITEM              (gimp_list_item_get_type ())
#define GIMP_LIST_ITEM(obj)              (GTK_CHECK_CAST ((obj), GIMP_TYPE_LIST_ITEM, GimpListItem))
#define GIMP_LIST_ITEM_CLASS(klass)      (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_LIST_ITEM, GimpListItemClass))
#define GIMP_IS_LIST_ITEM(obj)           (GTK_CHECK_TYPE ((obj), GIMP_TYPE_LIST_ITEM))
#define GIMP_IS_LIST_ITEM_CLASS(klass)   (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_LIST_ITEM))


typedef struct _GimpListItemClass  GimpListItemClass;

struct _GimpListItem
{
  GtkListItem    parent_instance;

  GtkWidget     *hbox;

  GtkWidget     *preview;
  GtkWidget     *name_label;

  /*< protected >*/
  gint           preview_size;

  /*< private >*/
  gboolean       reorderable;
  GimpContainer *container;
  GimpDropType   drop_type;
};

struct _GimpListItemClass
{
  GtkListItemClass  parent_class;

  void (* set_viewable) (GimpListItem *list_item,
                         GimpViewable *viewable);
};


GtkType     gimp_list_item_get_type        (void);
GtkWidget * gimp_list_item_new             (GimpViewable   *viewable,
                                            gint            preview_size);

void        gimp_list_item_set_viewable    (GimpListItem   *list_item,
					    GimpViewable   *viewable);
void        gimp_list_item_set_reorderable (GimpListItem   *list_item,
                                            gboolean        reorderable,
                                            GimpContainer  *container);


/*  protected  */

gboolean    gimp_list_item_check_drag           (GimpListItem   *list_item,
						 GdkDragContext *context,
						 gint            x,
						 gint            y,
						 GimpViewable  **src_viewable,
						 gint           *dest_index,
						 GdkDragAction  *drag_action,
						 GimpDropType   *drop_type);

void        gimp_list_item_button_realize       (GtkWidget      *widget,
						 gpointer        data);
void        gimp_list_item_button_state_changed (GtkWidget      *widget,
						 GtkStateType    previous_state,
						 gpointer        data);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GIMP_LIST_ITEM_H__ */
