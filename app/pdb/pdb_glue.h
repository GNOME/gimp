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

#define gimp_layer_set_name(l,n)   gimp_object_set_name(GIMP_OBJECT(l),(n))
#define gimp_layer_get_name(l)     gimp_object_get_name(GIMP_OBJECT(l))
#define gimp_layer_set_tattoo(l,t) gimp_drawable_set_tattoo(GIMP_DRAWABLE(l),(t))
#define gimp_layer_get_tattoo(l)   gimp_drawable_get_tattoo(GIMP_DRAWABLE(l))

#define gimp_channel_set_name(c,n)   gimp_object_set_name(GIMP_OBJECT(c),(n))
#define gimp_channel_get_name(c)     gimp_object_get_name(GIMP_OBJECT(c))
#define gimp_channel_set_tattoo(c,t) gimp_drawable_set_tattoo(GIMP_DRAWABLE(c),(t))
#define gimp_channel_get_tattoo(c)   gimp_drawable_get_tattoo(GIMP_DRAWABLE(c))


#endif /* __PDB_GLUE_H__ */
