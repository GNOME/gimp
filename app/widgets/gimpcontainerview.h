/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_CONTAINER_VIEW_H__
#define __GIMP_CONTAINER_VIEW_H__


#include <gtk/gtkvbox.h>


#define GIMP_TYPE_CONTAINER_VIEW            (gimp_container_view_get_type ())
#define GIMP_CONTAINER_VIEW(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_CONTAINER_VIEW, GimpContainerView))
#define GIMP_CONTAINER_VIEW_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONTAINER_VIEW, GimpContainerViewClass))
#define GIMP_IS_CONTAINER_VIEW(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_CONTAINER_VIEW))
#define GIMP_IS_CONTAINER_VIEW_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONTAINER_VIEW))


typedef struct _GimpContainerViewClass  GimpContainerViewClass;

struct _GimpContainerView
{
  GtkVBox        parent_instance;

  GimpContainer *container;
  GimpContext   *context;

  GHashTable    *hash_table;

  gint           preview_size;
};

struct _GimpContainerViewClass
{
  GtkVBoxClass  parent_class;

  void     (* set_container)    (GimpContainerView *view,
				 GimpContainer     *container);
  gpointer (* insert_item)      (GimpContainerView *view,
				 GimpViewable      *object,
				 gint               index);
  void     (* remove_item)      (GimpContainerView *view,
				 GimpViewable      *object,
				 gpointer           insert_data);
  void     (* reorder_item)     (GimpContainerView *view,
				 GimpViewable      *object,
				 gint               new_index,
				 gpointer           insert_data);
  void     (* select_item)      (GimpContainerView *view,
				 GimpViewable      *object,
				 gpointer           insert_data);
  void     (* activate_item)    (GimpContainerView *view,
				 GimpViewable      *object,
				 gpointer           insert_data);
  void     (* clear_items)      (GimpContainerView *view);
  void     (* set_preview_size) (GimpContainerView *view);
};


GtkType   gimp_container_view_get_type         (void);

void      gimp_container_view_set_container    (GimpContainerView *view,
						GimpContainer     *container);
void      gimp_container_view_set_context      (GimpContainerView *view,
						GimpContext       *context);
void      gimp_container_view_set_preview_size (GimpContainerView *view,
						gint               preview_size);
void      gimp_container_view_select_item      (GimpContainerView *view,
						GimpViewable      *viewable);
void      gimp_container_view_activate_item    (GimpContainerView *view,
						GimpViewable      *viewable);


/*  private  */

void      gimp_container_view_item_selected    (GimpContainerView *view,
						GimpViewable      *item);
void      gimp_container_view_item_activated   (GimpContainerView *view,
						GimpViewable      *item);


#endif  /*  __GIMP_CONTAINER_VIEW_H__  */
