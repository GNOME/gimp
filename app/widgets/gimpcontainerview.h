/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainerview.h
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

#ifndef __GIMP_CONTAINER_VIEW_H__
#define __GIMP_CONTAINER_VIEW_H__


#include "gimpeditor.h"


#define GIMP_TYPE_CONTAINER_VIEW            (gimp_container_view_get_type ())
#define GIMP_CONTAINER_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CONTAINER_VIEW, GimpContainerView))
#define GIMP_CONTAINER_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONTAINER_VIEW, GimpContainerViewClass))
#define GIMP_IS_CONTAINER_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CONTAINER_VIEW))
#define GIMP_IS_CONTAINER_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONTAINER_VIEW))
#define GIMP_CONTAINER_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CONTAINER_VIEW, GimpContainerViewClass))


typedef struct _GimpContainerViewClass  GimpContainerViewClass;

struct _GimpContainerView
{
  GimpEditor     parent_instance;

  GimpContainer *container;
  GimpContext   *context;

  GHashTable    *hash_table;

  gint           preview_size;
  gint           preview_border_width;
  gboolean       reorderable;

  GtkWidget     *scrolled_win;

  /*  initialized by subclass  */
  GtkWidget     *dnd_widget;

  GQuark         name_changed_handler_id;
};

struct _GimpContainerViewClass
{
  GimpEditorClass  parent_class;

  /*  signals  */
  gboolean (* select_item)      (GimpContainerView *view,
				 GimpViewable      *object,
				 gpointer           insert_data);
  void     (* activate_item)    (GimpContainerView *view,
				 GimpViewable      *object,
				 gpointer           insert_data);
  void     (* context_item)     (GimpContainerView *view,
				 GimpViewable      *object,
				 gpointer           insert_data);

  /*  virtual functions  */
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
  void     (* rename_item)      (GimpContainerView *view,
				 GimpViewable      *object,
				 gpointer           insert_data);
  void     (* clear_items)      (GimpContainerView *view);
  void     (* set_preview_size) (GimpContainerView *view);

  /*  the destroy notifier for view->hash_table's values  */
  GDestroyNotify  insert_data_free;
};


GType     gimp_container_view_get_type         (void) G_GNUC_CONST;

void      gimp_container_view_construct        (GimpContainerView   *view,
                                                GimpContainer       *container,
                                                GimpContext         *context,
                                                gint                 preview_size,
                                                gint                 preview_border_width,
                                                gboolean             reorderable);

void      gimp_container_view_set_container    (GimpContainerView   *view,
						GimpContainer       *container);
void      gimp_container_view_set_context      (GimpContainerView   *view,
						GimpContext         *context);
void      gimp_container_view_set_preview_size (GimpContainerView   *view,
						gint                 preview_size,
                                                gint                 preview_border_width);

void      gimp_container_view_enable_dnd       (GimpContainerView   *editor,
						GtkButton           *button,
						GType                children_type);
void      gimp_container_view_set_size_request (GimpContainerView   *view,
                                                gint                 width,
                                                gint                 height);

gboolean  gimp_container_view_select_item      (GimpContainerView   *view,
						GimpViewable        *viewable);
void      gimp_container_view_activate_item    (GimpContainerView   *view,
						GimpViewable        *viewable);
void      gimp_container_view_context_item     (GimpContainerView   *view,
						GimpViewable        *viewable);


/*  protected  */

gboolean  gimp_container_view_item_selected    (GimpContainerView   *view,
						GimpViewable        *item);
void      gimp_container_view_item_activated   (GimpContainerView   *view,
						GimpViewable        *item);
void      gimp_container_view_item_context     (GimpContainerView   *view,
						GimpViewable        *item);


#endif  /*  __GIMP_CONTAINER_VIEW_H__  */
