/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcomponentlistitem.h
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

#ifndef __GIMP_COMPONENT_LIST_ITEM_H__
#define __GIMP_COMPONENT_LIST_ITEM_H__


#include "gimplistitem.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GIMP_TYPE_COMPONENT_LIST_ITEM            (gimp_component_list_item_get_type ())
#define GIMP_COMPONENT_LIST_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COMPONENT_LIST_ITEM, GimpComponentListItem))
#define GIMP_COMPONENT_LIST_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COMPONENT_LIST_ITEM, GimpComponentListItemClass))
#define GIMP_IS_COMPONENT_LIST_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COMPONENT_LIST_ITEM))
#define GIMP_IS_COMPONENT_LIST_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COMPONENT_LIST_ITEM))
#define GIMP_COMPONENT_LIST_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COMPONENT_LIST_ITEM, GimpComponentListItemClass))


typedef struct _GimpComponentListItemClass  GimpComponentListItemClass;

struct _GimpComponentListItem
{
  GimpListItem  parent_instance;

  ChannelType   channel;

  GtkWidget    *eye_button;
};

struct _GimpComponentListItemClass
{
  GimpListItemClass  parent_class;
};


GType       gimp_component_list_item_get_type (void);

GtkWidget * gimp_component_list_item_new      (GimpImage   *gimage,
					       gint         preview_size,
					       ChannelType  channel);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GIMP_COMPONENT_LIST_ITEM_H__ */
