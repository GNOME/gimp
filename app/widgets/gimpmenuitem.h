/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmenuitem.h
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

#ifndef __GIMP_MENU_ITEM_H__
#define __GIMP_MENU_ITEM_H__

#include <gtk/gtkmenuitem.h>


#define GIMP_TYPE_MENU_ITEM              (gimp_menu_item_get_type ())
#define GIMP_MENU_ITEM(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_MENU_ITEM, GimpMenuItem))
#define GIMP_MENU_ITEM_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_MENU_ITEM, GimpMenuItemClass))
#define GIMP_IS_MENU_ITEM(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_MENU_ITEM))
#define GIMP_IS_MENU_ITEM_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_MENU_ITEM))
#define GIMP_MENU_ITEM_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_MENU_ITEM, GimpMenuItemClass))


typedef struct _GimpMenuItemClass  GimpMenuItemClass;

struct _GimpMenuItem
{
  GtkMenuItem          parent_instance;

  GtkWidget           *hbox;

  GtkWidget           *preview;
  GtkWidget           *name_label;

  /*< protected >*/
  guint                preview_size;

  /*< private >*/
  GimpItemGetNameFunc  get_name_func;
};

struct _GimpMenuItemClass
{
  GtkMenuItemClass  parent_class;

  /*  virtual functions  */
  void (* set_viewable) (GimpMenuItem *menu_item,
                         GimpViewable *viewable);
};


GType       gimp_menu_item_get_type      (void) G_GNUC_CONST;

GtkWidget * gimp_menu_item_new           (GimpViewable        *viewable,
					  gint                 preview_size);

void        gimp_menu_item_set_name_func (GimpMenuItem        *menu_item,
					  GimpItemGetNameFunc  get_name_func);


#endif /* __GIMP_MENU_ITEM_H__ */
