/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainermenuimpl.h
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

#ifndef __GIMP_CONTAINER_MENU_IMPL_H__
#define __GIMP_CONTAINER_MENU_IMPL_H__


#include "gimpcontainermenu.h"


#define GIMP_TYPE_CONTAINER_MENU_IMPL            (gimp_container_menu_impl_get_type ())
#define GIMP_CONTAINER_MENU_IMPL(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_CONTAINER_MENU_IMPL, GimpContainerMenuImpl))
#define GIMP_CONTAINER_MENU_IMPL_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONTAINER_MENU_IMPL, GimpContainerMenuImplClass))
#define GIMP_IS_CONTAINER_MENU_IMPL(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_CONTAINER_MENU_IMPL))
#define GIMP_IS_CONTAINER_MENU_IMPL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONTAINER_MENU_IMPL))


typedef struct _GimpContainerMenuImplClass  GimpContainerMenuImplClass;

struct _GimpContainerMenuImpl
{
  GimpContainerMenu  parent_instance;

  GtkWidget         *empty_item;
};

struct _GimpContainerMenuImplClass
{
  GimpContainerMenuClass  parent_class;
};


GtkType     gimp_container_menu_impl_get_type (void);

GtkWidget * gimp_container_menu_new           (GimpContainer *container,
					       GimpContext   *context,
					       gint           preview_size);


#endif  /*  __GIMP_CONTAINER_MENU_IMPL_H__  */
