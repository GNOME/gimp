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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __UNDO_H__
#define __UNDO_H__

/* forward declarations */
struct _GImage;
struct _GimpDrawable;


/*  Undo types  */
#define      IMAGE_UNDO              1
#define      IMAGE_MOD_UNDO          2
#define      MASK_UNDO               3
#define      LAYER_DISPLACE_UNDO     4
#define      TRANSFORM_UNDO          5
#define      PAINT_UNDO              6
#define      LAYER_UNDO              7
#define      LAYER_MOD               8
#define      LAYER_MASK_UNDO         9
#define      LAYER_CHANGE            10
#define      LAYER_POSITION          11
#define      CHANNEL_UNDO            12
#define      CHANNEL_MOD             13
#define      FS_TO_LAYER_UNDO        14
#define      GIMAGE_MOD              15
#define      FS_RIGOR                16
#define      FS_RELAX                17
#define      GUIDE_UNDO              18

/*  Aggregate undo types  */
#define      FLOAT_MASK_UNDO         20
#define      EDIT_PASTE_UNDO         21
#define      EDIT_CUT_UNDO           22
#define      TRANSFORM_CORE_UNDO     23
#define      PAINT_CORE_UNDO         24
#define      FLOATING_LAYER_UNDO     25
#define      LINKED_LAYER_UNDO       26
#define      LAYER_APPLY_MASK_UNDO   27
#define      LAYER_MERGE_UNDO        28
#define      FS_ANCHOR_UNDO          29
#define      GIMAGE_MOD_UNDO         30
#define      CROP_UNDO               31
#define      LAYER_SCALE_UNDO        32
#define      LAYER_RESIZE_UNDO       33
#define      MISC_UNDO               100

/*  Undo interface functions  */

int      undo_push_group_start       (struct _GImage *, int);
int      undo_push_group_end         (struct _GImage *);
int      undo_push_image             (struct _GImage *, struct _GimpDrawable *,
                                      int, int, int, int);
int      undo_push_image_mod         (struct _GImage *, struct _GimpDrawable *,
                                      int, int, int, int, void *);
int      undo_push_mask              (struct _GImage *, void *);
int      undo_push_layer_displace    (struct _GImage *, int);
int      undo_push_transform         (struct _GImage *, void *);
int      undo_push_paint             (struct _GImage *, void *);
int      undo_push_layer             (struct _GImage *, void *);
int      undo_push_layer_mod         (struct _GImage *, void *);
int      undo_push_layer_mask        (struct _GImage *, void *);
int      undo_push_layer_change      (struct _GImage *, int);
int      undo_push_layer_position    (struct _GImage *, int);
int      undo_push_channel           (struct _GImage *, void *);
int      undo_push_channel_mod       (struct _GImage *, void *);
int      undo_push_fs_to_layer       (struct _GImage *, void *);
int      undo_push_fs_rigor          (struct _GImage *, int);
int      undo_push_fs_relax          (struct _GImage *, int);
int      undo_push_gimage_mod        (struct _GImage *);
int      undo_push_guide             (struct _GImage *, void *);

int      undo_pop                    (struct _GImage *);
int      undo_redo                   (struct _GImage *);
void     undo_free                   (struct _GImage *);


#endif  /* __UNDO_H__ */
