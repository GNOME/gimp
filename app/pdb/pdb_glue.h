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

#ifndef __PDB_GLUE_H__
#define __PDB_GLUE_H__


#define gimp_drawable_layer      GIMP_IS_LAYER
#define gimp_drawable_layer_mask GIMP_IS_LAYER_MASK
#define gimp_drawable_channel    GIMP_IS_CHANNEL

#define gimp_layer_set_name(l,n)       gimp_object_set_name(GIMP_OBJECT(l),(n))
#define gimp_layer_get_name(l)         gimp_object_get_name(GIMP_OBJECT(l))
#define gimp_layer_get_visible(l)      gimp_drawable_get_visible(GIMP_DRAWABLE(l))
#define gimp_layer_set_visible(l,v)    gimp_drawable_set_visible(GIMP_DRAWABLE(l),(v))
#define gimp_layer_set_tattoo(l,t)     gimp_item_set_tattoo(GIMP_ITEM(l),(t))
#define gimp_layer_get_tattoo(l)       gimp_item_get_tattoo(GIMP_ITEM(l))

#define gimp_channel_set_name(c,n)     gimp_object_set_name(GIMP_OBJECT(c),(n))
#define gimp_channel_get_name(c)       gimp_object_get_name(GIMP_OBJECT(c))
#define gimp_channel_get_visible(c)    gimp_drawable_get_visible(GIMP_DRAWABLE(c))
#define gimp_channel_set_visible(c,v)  gimp_drawable_set_visible(GIMP_DRAWABLE(c),(v))
#define gimp_channel_set_tattoo(c,t)   gimp_item_set_tattoo(GIMP_ITEM(c),(t))
#define gimp_channel_get_tattoo(c)     gimp_item_get_tattoo(GIMP_ITEM(c))

#define gimp_layer_get_apply_mask(l)   (l)->mask ? gimp_layer_mask_get_apply((l)->mask) : FALSE;
#define gimp_layer_get_show_mask(l)    (l)->mask ? gimp_layer_mask_get_show((l)->mask) : FALSE;
#define gimp_layer_get_edit_mask(l)    (l)->mask ? gimp_layer_mask_get_edit((l)->mask) : FALSE;

#define gimp_layer_set_apply_mask(l,a) { if((l)->mask) gimp_layer_mask_set_apply((l)->mask,(a)); else success = FALSE; }
#define gimp_layer_set_show_mask(l,s)  { if((l)->mask) gimp_layer_mask_set_show((l)->mask,(s)); else success = FALSE; }
#define gimp_layer_set_edit_mask(l,e)  { if((l)->mask) gimp_layer_mask_set_edit((l)->mask,(e)); else success = FALSE; }

#define gimp_drawable_parasite_attach(d,p) gimp_item_parasite_attach(GIMP_ITEM(d),p)
#define gimp_drawable_parasite_detach(d,p) gimp_item_parasite_detach(GIMP_ITEM(d),p)
#define gimp_drawable_parasite_list(d,c)   gimp_item_parasite_list(GIMP_ITEM(d),c)
#define gimp_drawable_parasite_find(d,p)   gimp_item_parasite_find(GIMP_ITEM(d),p)


#endif /* __PDB_GLUE_H__ */
