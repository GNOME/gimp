/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpchannellistitem.h
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

#ifndef __GIMP_CHANNEL_LIST_ITEM_H__
#define __GIMP_CHANNEL_LIST_ITEM_H__


#include "gimpdrawablelistitem.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GIMP_TYPE_CHANNEL_LIST_ITEM            (gimp_channel_list_item_get_type ())
#define GIMP_CHANNEL_LIST_ITEM(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_CHANNEL_LIST_ITEM, GimpChannelListItem))
#define GIMP_CHANNEL_LIST_ITEM_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CHANNEL_LIST_ITEM, GimpChannelListItemClass))
#define GIMP_IS_CHANNEL_LIST_ITEM(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_CHANNEL_LIST_ITEM))
#define GIMP_IS_CHANNEL_LIST_ITEM_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CHANNEL_LIST_ITEM))


typedef struct _GimpChannelListItemClass  GimpChannelListItemClass;

struct _GimpChannelListItem
{
  GimpDrawableListItem  parent_instance;
};

struct _GimpChannelListItemClass
{
  GimpDrawableListItemClass  parent_class;
};


GtkType   gimp_channel_list_item_get_type (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GIMP_CHANNEL_LIST_ITEM_H__ */
