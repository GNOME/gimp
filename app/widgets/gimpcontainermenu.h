/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainermenu.h
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

#ifndef __GIMP_CONTAINER_MENU_H__
#define __GIMP_CONTAINER_MENU_H__


/*  keep everything in sync with GimpContainerView so we can easily make
 *  and interface out of it with GTK+ 2.0
 */

#include <gtk/gtkmenu.h>


#define GIMP_TYPE_CONTAINER_MENU            (gimp_container_menu_get_type ())
#define GIMP_CONTAINER_MENU(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CONTAINER_MENU, GimpContainerMenu))
#define GIMP_CONTAINER_MENU_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONTAINER_MENU, GimpContainerMenuClass))
#define GIMP_IS_CONTAINER_MENU(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CONTAINER_MENU))
#define GIMP_IS_CONTAINER_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONTAINER_MENU))
#define GIMP_CONTAINER_MENU_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CONTAINER_MENU, GimpContainerMenuClass))


typedef struct _GimpContainerMenuClass  GimpContainerMenuClass;

struct _GimpContainerMenu
{
  GtkMenu              parent_instance;

  GimpContainer       *container;
  GimpContext         *context;

  GHashTable          *hash_table;

  gint                 preview_size;
  gint                 preview_border_width;

  GimpItemGetNameFunc  get_name_func;
};

struct _GimpContainerMenuClass
{
  GtkMenuClass  parent_class;

  /*  signals  */
  void     (* select_item)      (GimpContainerMenu *menu,
				 GimpViewable      *object,
				 gpointer           insert_data);
  void     (* activate_item)    (GimpContainerMenu *menu,
				 GimpViewable      *object,
				 gpointer           insert_data);
  void     (* context_item)     (GimpContainerMenu *menu,
				 GimpViewable      *object,
				 gpointer           insert_data);

  /*  virtual functions  */
  void     (* set_container)    (GimpContainerMenu *menu,
				 GimpContainer     *container);
  gpointer (* insert_item)      (GimpContainerMenu *menu,
				 GimpViewable      *object,
				 gint               index);
  void     (* remove_item)      (GimpContainerMenu *menu,
				 GimpViewable      *object,
				 gpointer           insert_data);
  void     (* reorder_item)     (GimpContainerMenu *menu,
				 GimpViewable      *object,
				 gint               new_index,
				 gpointer           insert_data);
  void     (* clear_items)      (GimpContainerMenu *menu);
  void     (* set_preview_size) (GimpContainerMenu *menu);
};


GType     gimp_container_menu_get_type         (void) G_GNUC_CONST;

void      gimp_container_menu_set_container    (GimpContainerMenu   *menu,
						GimpContainer       *container);
void      gimp_container_menu_set_context      (GimpContainerMenu   *menu,
						GimpContext         *context);
void      gimp_container_menu_set_preview_size (GimpContainerMenu   *menu,
						gint                 preview_size);
void      gimp_container_menu_set_name_func    (GimpContainerMenu   *menu,
						GimpItemGetNameFunc  get_name_func);

void      gimp_container_menu_select_item      (GimpContainerMenu   *menu,
						GimpViewable        *viewable);
void      gimp_container_menu_activate_item    (GimpContainerMenu   *menu,
						GimpViewable        *viewable);
void      gimp_container_menu_context_item     (GimpContainerMenu   *menu,
						GimpViewable        *viewable);


/*  private  */

void      gimp_container_menu_item_selected    (GimpContainerMenu *menu,
						GimpViewable      *item);
void      gimp_container_menu_item_activated   (GimpContainerMenu *menu,
						GimpViewable      *item);
void      gimp_container_menu_item_context     (GimpContainerMenu *menu,
						GimpViewable      *item);


#endif  /*  __GIMP_CONTAINER_MENU_H__  */
