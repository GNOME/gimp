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

#ifndef __UNDO_TYPES_H__
#define __UNDO_TYPES_H__


/*  Undo types which actually do something, unlike the ones in
 *  core/core-types.h, which are only for pushing groups
 */

typedef enum
{
  IMAGE_UNDO = LAST_UNDO_GROUP + 1,
  IMAGE_MOD_UNDO,
  IMAGE_TYPE_UNDO,
  IMAGE_SIZE_UNDO,
  IMAGE_RESOLUTION_UNDO,
  IMAGE_MASK_UNDO,
  IMAGE_QMASK_UNDO,	
  IMAGE_GUIDE_UNDO,
  LAYER_ADD_UNDO,
  LAYER_REMOVE_UNDO,
  LAYER_MOD_UNDO,
  LAYER_MASK_ADD_UNDO,
  LAYER_MASK_REMOVE_UNDO,
  LAYER_RENAME_UNDO,
  LAYER_REPOSITION_UNDO,
  LAYER_DISPLACE_UNDO,
  CHANNEL_ADD_UNDO,
  CHANNEL_REMOVE_UNDO,
  CHANNEL_MOD_UNDO,
  CHANNEL_REPOSITION_UNDO,
  VECTORS_ADD_UNDO,
  VECTORS_REMOVE_UNDO,
  VECTORS_MOD_UNDO,
  VECTORS_REPOSITION_UNDO,
  FS_TO_LAYER_UNDO,
  FS_RIGOR_UNDO,
  FS_RELAX_UNDO,
  TRANSFORM_UNDO,
  PAINT_UNDO,
  PARASITE_ATTACH_UNDO,
  PARASITE_REMOVE_UNDO,

  CANT_UNDO
} UndoImplType;


#endif /* __UNDO_TYPES_H__ */
