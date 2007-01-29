/* GIMP - The GNU Image Manipulation Program
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

#ifndef __GIMP_IMAGE_UNDO_PUSH_H__
#define __GIMP_IMAGE_UNDO_PUSH_H__


/*  image undos  */

GimpUndo * gimp_image_undo_push_image_type          (GimpImage     *image,
                                                     const gchar   *undo_desc);
GimpUndo * gimp_image_undo_push_image_size          (GimpImage     *image,
                                                     const gchar   *undo_desc);
GimpUndo * gimp_image_undo_push_image_resolution    (GimpImage     *image,
                                                     const gchar   *undo_desc);
GimpUndo * gimp_image_undo_push_image_guide         (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpGuide     *guide);
GimpUndo * gimp_image_undo_push_image_grid          (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpGrid      *grid);
GimpUndo * gimp_image_undo_push_image_sample_point  (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpSamplePoint *sample_point);
GimpUndo * gimp_image_undo_push_image_colormap      (GimpImage     *image,
                                                     const gchar   *undo_desc);


/*  drawable undo  */

GimpUndo * gimp_image_undo_push_drawable            (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpDrawable  *drawable,
                                                     TileManager   *tiles,
                                                     gboolean       sparse,
                                                     gint           x,
                                                     gint           y,
                                                     gint           width,
                                                     gint           height);
GimpUndo * gimp_image_undo_push_drawable_mod        (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpDrawable  *drawable);


/*  mask undo  */

GimpUndo * gimp_image_undo_push_mask                (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpChannel   *mask);


/*  item undos  */

GimpUndo * gimp_image_undo_push_item_rename         (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpItem      *item);
GimpUndo * gimp_image_undo_push_item_displace       (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpItem      *item);
GimpUndo * gimp_image_undo_push_item_visibility     (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpItem      *item);
GimpUndo * gimp_image_undo_push_item_linked         (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpItem      *item);


/*  layer undos  */

GimpUndo * gimp_image_undo_push_layer_add           (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpLayer     *layer,
                                                     gint           prev_position,
                                                     GimpLayer     *prev_layer);
GimpUndo * gimp_image_undo_push_layer_remove        (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpLayer     *layer,
                                                     gint           prev_position,
                                                     GimpLayer     *prev_layer);
GimpUndo * gimp_image_undo_push_layer_reposition    (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpLayer     *layer);
GimpUndo * gimp_image_undo_push_layer_mode          (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpLayer     *layer);
GimpUndo * gimp_image_undo_push_layer_opacity       (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpLayer     *layer);
GimpUndo * gimp_image_undo_push_layer_lock_alpha    (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpLayer     *layer);


/*  text layer undos  */

GimpUndo * gimp_image_undo_push_text_layer          (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpTextLayer *layer,
                                                     const GParamSpec *pspec);
GimpUndo * gimp_image_undo_push_text_layer_modified (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpTextLayer *layer);


/*  layer mask undos  */

GimpUndo * gimp_image_undo_push_layer_mask_add      (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpLayer     *layer,
                                                     GimpLayerMask *mask);
GimpUndo * gimp_image_undo_push_layer_mask_remove   (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpLayer     *layer,
                                                     GimpLayerMask *mask);
GimpUndo * gimp_image_undo_push_layer_mask_apply    (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpLayerMask *mask);
GimpUndo * gimp_image_undo_push_layer_mask_show     (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpLayerMask *mask);


/*  channel undos  */

GimpUndo * gimp_image_undo_push_channel_add         (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpChannel   *channel,
                                                     gint           prev_position,
                                                     GimpChannel   *prev_channel);
GimpUndo * gimp_image_undo_push_channel_remove      (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpChannel   *channel,
                                                     gint           prev_position,
                                                     GimpChannel   *prev_channel);
GimpUndo * gimp_image_undo_push_channel_reposition  (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpChannel   *channel);
GimpUndo * gimp_image_undo_push_channel_color       (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpChannel   *channel);


/*  vectors undos  */

GimpUndo * gimp_image_undo_push_vectors_add         (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpVectors   *vectors,
                                                     gint           prev_position,
                                                     GimpVectors   *prev_vectors);
GimpUndo * gimp_image_undo_push_vectors_remove      (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpVectors   *channel,
                                                     gint           prev_position,
                                                     GimpVectors   *prev_vectors);
GimpUndo * gimp_image_undo_push_vectors_mod         (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpVectors   *vectors);
GimpUndo * gimp_image_undo_push_vectors_reposition  (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpVectors   *vectors);


/*  floating selection undos  */

GimpUndo * gimp_image_undo_push_fs_to_layer         (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpLayer     *floating_layer,
                                                     GimpDrawable  *drawable);
GimpUndo * gimp_image_undo_push_fs_rigor            (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpLayer     *floating_layer);
GimpUndo * gimp_image_undo_push_fs_relax            (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpLayer     *floating_layer);


/*  parasite undos  */

GimpUndo * gimp_image_undo_push_image_parasite      (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     const GimpParasite *parasite);
GimpUndo * gimp_image_undo_push_image_parasite_remove (GimpImage   *image,
                                                     const gchar   *undo_desc,
                                                     const gchar   *name);

GimpUndo * gimp_image_undo_push_item_parasite       (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpItem      *item,
                                                     const GimpParasite *parasite);
GimpUndo * gimp_image_undo_push_item_parasite_remove(GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpItem      *item,
                                                     const gchar   *name);


/*  EEK undo  */

GimpUndo * gimp_image_undo_push_cantundo            (GimpImage     *image,
                                                     const gchar   *undo_desc);


#endif  /* __GIMP_IMAGE_UNDO_PUSH_H__ */
