/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimplayerlistitem.h
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

#ifndef __GIMP_LAYER_LIST_ITEM_H__
#define __GIMP_LAYER_LIST_ITEM_H__

#include "gimpdrawablelistitem.h"


#define GIMP_TYPE_LAYER_LIST_ITEM            (gimp_layer_list_item_get_type ())
#define GIMP_LAYER_LIST_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_LAYER_LIST_ITEM, GimpLayerListItem))
#define GIMP_LAYER_LIST_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_LAYER_LIST_ITEM, GimpLayerListItemClass))
#define GIMP_IS_LAYER_LIST_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_LAYER_LIST_ITEM))
#define GIMP_IS_LAYER_LIST_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_LAYER_LIST_ITEM))
#define GIMP_LAYER_LIST_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_LAYER_LIST_ITEM, GimpLayerListItemClass))


typedef struct _GimpLayerListItemClass  GimpLayerListItemClass;

struct _GimpLayerListItem
{
  GimpDrawableListItem  parent_instance;

  GtkWidget            *linked_button;
  GtkWidget            *mask_preview;
};

struct _GimpLayerListItemClass
{
  GimpDrawableListItemClass  parent_class;
};


GType   gimp_layer_list_item_get_type (void) G_GNUC_CONST;


#endif /* __GIMP_LAYER_LIST_ITEM_H__ */
