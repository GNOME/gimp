/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __LIGMA_IMAGE_UNDO_PUSH_H__
#define __LIGMA_IMAGE_UNDO_PUSH_H__


/*  image undos  */

LigmaUndo * ligma_image_undo_push_image_type          (LigmaImage     *image,
                                                     const gchar   *undo_desc);
LigmaUndo * ligma_image_undo_push_image_precision     (LigmaImage     *image,
                                                     const gchar   *undo_desc);
LigmaUndo * ligma_image_undo_push_image_size          (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     gint           previous_origin_x,
                                                     gint           previous_origin_y,
                                                     gint           previous_width,
                                                     gint           prevoius_height);
LigmaUndo * ligma_image_undo_push_image_resolution    (LigmaImage     *image,
                                                     const gchar   *undo_desc);
LigmaUndo * ligma_image_undo_push_image_grid          (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaGrid      *grid);
LigmaUndo * ligma_image_undo_push_image_colormap      (LigmaImage     *image,
                                                     const gchar   *undo_desc);
LigmaUndo * ligma_image_undo_push_image_hidden_profile(LigmaImage     *image,
                                                     const gchar   *undo_desc);
LigmaUndo * ligma_image_undo_push_image_metadata      (LigmaImage     *image,
                                                     const gchar   *undo_desc);
LigmaUndo * ligma_image_undo_push_image_parasite      (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     const LigmaParasite *parasite);
LigmaUndo * ligma_image_undo_push_image_parasite_remove (LigmaImage   *image,
                                                     const gchar   *undo_desc,
                                                     const gchar   *name);


/*  guide & sample point undos  */

LigmaUndo * ligma_image_undo_push_guide               (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaGuide     *guide);
LigmaUndo * ligma_image_undo_push_sample_point        (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaSamplePoint *sample_point);


/*  drawable undos  */

LigmaUndo * ligma_image_undo_push_drawable            (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaDrawable  *drawable,
                                                     GeglBuffer    *buffer,
                                                     gint           x,
                                                     gint           y);
LigmaUndo * ligma_image_undo_push_drawable_mod        (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaDrawable  *drawable,
                                                     gboolean       copy_buffer);
LigmaUndo * ligma_image_undo_push_drawable_format     (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaDrawable  *drawable);


/*  mask undos  */

LigmaUndo * ligma_image_undo_push_mask                (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaChannel   *mask);
LigmaUndo * ligma_image_undo_push_mask_precision      (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaChannel   *mask);


/*  item undos  */

LigmaUndo * ligma_image_undo_push_item_reorder        (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaItem      *item);
LigmaUndo * ligma_image_undo_push_item_rename         (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaItem      *item);
LigmaUndo * ligma_image_undo_push_item_displace       (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaItem      *item);
LigmaUndo * ligma_image_undo_push_item_visibility     (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaItem      *item);
LigmaUndo * ligma_image_undo_push_item_color_tag      (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaItem      *item);
LigmaUndo * ligma_image_undo_push_item_lock_content   (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaItem      *item);
LigmaUndo * ligma_image_undo_push_item_lock_position  (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaItem      *item);
LigmaUndo * ligma_image_undo_push_item_lock_visibility (LigmaImage    *image,
                                                      const gchar  *undo_desc,
                                                      LigmaItem     *item);
LigmaUndo * ligma_image_undo_push_item_parasite       (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaItem      *item,
                                                     const LigmaParasite *parasite);
LigmaUndo * ligma_image_undo_push_item_parasite_remove(LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaItem      *item,
                                                     const gchar   *name);


/*  layer undos  */

LigmaUndo * ligma_image_undo_push_layer_add           (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaLayer     *layer,
                                                     GList         *prev_layers);
LigmaUndo * ligma_image_undo_push_layer_remove        (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaLayer     *layer,
                                                     LigmaLayer     *prev_parent,
                                                     gint           prev_position,
                                                     GList         *prev_layers);
LigmaUndo * ligma_image_undo_push_layer_mode          (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaLayer     *layer);
LigmaUndo * ligma_image_undo_push_layer_opacity       (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaLayer     *layer);
LigmaUndo * ligma_image_undo_push_layer_lock_alpha    (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaLayer     *layer);


/*  group layer undos  */

LigmaUndo *
    ligma_image_undo_push_group_layer_suspend_resize (LigmaImage      *image,
                                                     const gchar    *undo_desc,
                                                     LigmaGroupLayer *group);
LigmaUndo *
     ligma_image_undo_push_group_layer_resume_resize (LigmaImage      *image,
                                                     const gchar    *undo_desc,
                                                     LigmaGroupLayer *group);
LigmaUndo *
      ligma_image_undo_push_group_layer_suspend_mask (LigmaImage      *image,
                                                     const gchar    *undo_desc,
                                                     LigmaGroupLayer *group);
LigmaUndo *
       ligma_image_undo_push_group_layer_resume_mask (LigmaImage      *image,
                                                     const gchar    *undo_desc,
                                                     LigmaGroupLayer *group);
LigmaUndo *
   ligma_image_undo_push_group_layer_start_transform (LigmaImage      *image,
                                                     const gchar    *undo_desc,
                                                     LigmaGroupLayer *group);
LigmaUndo *
     ligma_image_undo_push_group_layer_end_transform (LigmaImage      *image,
                                                     const gchar    *undo_desc,
                                                     LigmaGroupLayer *group);
LigmaUndo * ligma_image_undo_push_group_layer_convert (LigmaImage      *image,
                                                     const gchar    *undo_desc,
                                                     LigmaGroupLayer *group);


/*  text layer undos  */

LigmaUndo * ligma_image_undo_push_text_layer          (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaTextLayer *layer,
                                                     const GParamSpec *pspec);
LigmaUndo * ligma_image_undo_push_text_layer_modified (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaTextLayer *layer);
LigmaUndo * ligma_image_undo_push_text_layer_convert  (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaTextLayer *layer);


/*  layer mask undos  */

LigmaUndo * ligma_image_undo_push_layer_mask_add      (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaLayer     *layer,
                                                     LigmaLayerMask *mask);
LigmaUndo * ligma_image_undo_push_layer_mask_remove   (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaLayer     *layer,
                                                     LigmaLayerMask *mask);
LigmaUndo * ligma_image_undo_push_layer_mask_apply    (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaLayer     *layer);
LigmaUndo * ligma_image_undo_push_layer_mask_show     (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaLayer     *layer);


/*  channel undos  */

LigmaUndo * ligma_image_undo_push_channel_add         (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaChannel   *channel,
                                                     GList         *prev_channels);
LigmaUndo * ligma_image_undo_push_channel_remove      (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaChannel   *channel,
                                                     LigmaChannel   *prev_parent,
                                                     gint           prev_position,
                                                     GList         *prev_channels);
LigmaUndo * ligma_image_undo_push_channel_color       (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaChannel   *channel);


/*  vectors undos  */

LigmaUndo * ligma_image_undo_push_vectors_add         (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaVectors   *vectors,
                                                     GList         *prev_vectors);
LigmaUndo * ligma_image_undo_push_vectors_remove      (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaVectors   *vectors,
                                                     LigmaVectors   *prev_parent,
                                                     gint           prev_position,
                                                     GList         *prev_vectors);
LigmaUndo * ligma_image_undo_push_vectors_mod         (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaVectors   *vectors);


/*  floating selection undos  */

LigmaUndo * ligma_image_undo_push_fs_to_layer         (LigmaImage     *image,
                                                     const gchar   *undo_desc,
                                                     LigmaLayer     *floating_layer);


/*  EEK undo  */

LigmaUndo * ligma_image_undo_push_cantundo            (LigmaImage     *image,
                                                     const gchar   *undo_desc);


#endif  /* __LIGMA_IMAGE_UNDO_PUSH_H__ */
