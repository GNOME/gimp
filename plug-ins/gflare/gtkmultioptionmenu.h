/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
  Modified 97-06-26	Eiichi Takamori <taka@ma1.seikyou.ne.jp>
  GtkMultiOptionMenu, taken from GtkOptionMenu, can work with
  hierarchal menus.
 */

#ifndef __GTK_MULTI_OPTION_MENU_H__
#define __GTK_MULTI_OPTION_MENU_H__


#include <gdk/gdk.h>
#include <gtk/gtkbutton.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GTK_MULTI_OPTION_MENU(obj)          GTK_CHECK_CAST (obj, gtk_multi_option_menu_get_type (), GtkMultiOptionMenu)
#define GTK_MULTI_OPTION_MENU_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, gtk_multi_option_menu_get_type (), GtkMultiOptionMenuClass)
#define GTK_IS_MULTI_OPTION_MENU(obj)       GTK_CHECK_TYPE (obj, gtk_multi_option_menu_get_type ())


typedef struct _GtkMultiOptionMenu       GtkMultiOptionMenu;
typedef struct _GtkMultiOptionMenuClass  GtkMultiOptionMenuClass;

struct _GtkMultiOptionMenu
{
  GtkButton button;

  GtkWidget *menu;
  GtkWidget *menu_item;

  guint16 width;
  guint16 height;
};

struct _GtkMultiOptionMenuClass
{
  GtkButtonClass parent_class;
};


guint      gtk_multi_option_menu_get_type    (void);
GtkWidget* gtk_multi_option_menu_new         (void);
GtkWidget* gtk_multi_option_menu_get_menu    (GtkMultiOptionMenu *multi_option_menu);
void       gtk_multi_option_menu_set_menu    (GtkMultiOptionMenu *multi_option_menu,
					      GtkWidget     *menu);
void       gtk_multi_option_menu_remove_menu (GtkMultiOptionMenu *multi_option_menu);
void       gtk_multi_option_menu_set_history (GtkMultiOptionMenu *multi_option_menu,
					      gint           index);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_MULTI_OPTION_MENU_H__ */
