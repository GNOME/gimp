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



/*  Undo types  */

/* NOTE: If you change this list, please update the textual mapping at
 * the bottom of undo.c as well. */
typedef enum
{
    /* Type 0 is special - in the gimpimage structure it means
     * there is no undo group currently being added to. */

    IMAGE_UNDO = 1,
    IMAGE_MOD_UNDO,
    MASK_UNDO,
    LAYER_DISPLACE_UNDO,
    TRANSFORM_UNDO,
    PAINT_UNDO,
    LAYER_ADD_UNDO,
    LAYER_REMOVE_UNDO,
    LAYER_MOD,
    LAYER_MASK_ADD_UNDO,
    LAYER_MASK_REMOVE_UNDO,
    LAYER_RENAME_UNDO,
    LAYER_POSITION,
    CHANNEL_UNDO,
    CHANNEL_MOD,
    FS_TO_LAYER_UNDO,
    GIMAGE_MOD,
    FS_RIGOR,
    FS_RELAX,
    GUIDE_UNDO,            /*  18 */

/*  Aggregate undo types  */
    FLOAT_MASK_UNDO = 20,
    EDIT_PASTE_UNDO,
    EDIT_CUT_UNDO,
    TRANSFORM_CORE_UNDO,

    PAINT_CORE_UNDO,
    FLOATING_LAYER_UNDO,
    LINKED_LAYER_UNDO,
    LAYER_APPLY_MASK_UNDO,
    LAYER_MERGE_UNDO,      /*  28  */
    FS_ANCHOR_UNDO,
    GIMAGE_MOD_UNDO,
    CROP_UNDO,
    LAYER_SCALE_UNDO,
    LAYER_RESIZE_UNDO,
    QMASK_UNDO,		   /*  34 */
    PARASITE_ATTACH_UNDO,
    PARASITE_REMOVE_UNDO,
    RESOLUTION_UNDO,

    MISC_UNDO = 100
} UndoType;


#endif /* __UNDO_TYPES_H__ */
