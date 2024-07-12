/* GIMP - The GNU Image Manipulation Program
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

#ifndef __GIMP_IMAGE_UNDO_PUSH_H__
#define __GIMP_IMAGE_UNDO_PUSH_H__


/*  image undos  */

GimpUndo * gimp_image_undo_push_image_type          (GimpImage     *image,
                                                     const gchar   *undo_desc);
GimpUndo * gimp_image_undo_push_image_precision     (GimpImage     *image,
                                                     const gchar   *undo_desc);
GimpUndo * gimp_image_undo_push_image_size          (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     gint           previous_origin_x,
                                                     gint           previous_origin_y,
                                                     gint           previous_width,
                                                     gint           prevoius_height);
GimpUndo * gimp_image_undo_push_image_resolution    (GimpImage     *image,
                                                     const gchar   *undo_desc);
GimpUndo * gimp_image_undo_push_image_grid          (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpGrid      *grid);
GimpUndo * gimp_image_undo_push_image_colormap      (GimpImage     *image,
                                                     const gchar   *undo_desc);
GimpUndo * gimp_image_undo_push_image_hidden_profile(GimpImage     *image,
                                                     const gchar   *undo_desc);
GimpUndo * gimp_image_undo_push_image_metadata      (GimpImage     *image,
                                                     const gchar   *undo_desc);
GimpUndo * gimp_image_undo_push_image_parasite      (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     const GimpParasite *parasite);
GimpUndo * gimp_image_undo_push_image_parasite_remove (GimpImage   *image,
                                                     const gchar   *undo_desc,
                                                     const gchar   *name);


/*  guide & sample point undos  */

GimpUndo * gimp_image_undo_push_guide               (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpGuide     *guide);
GimpUndo * gimp_image_undo_push_sample_point        (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpSamplePoint *sample_point);


/*  drawable undos  */

GimpUndo * gimp_image_undo_push_drawable            (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpDrawable  *drawable,
                                                     GeglBuffer    *buffer,
                                                     gint           x,
                                                     gint           y);
GimpUndo * gimp_image_undo_push_drawable_mod        (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpDrawable  *drawable,
                                                     gboolean       copy_buffer);
GimpUndo * gimp_image_undo_push_drawable_format     (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpDrawable  *drawable);


/*  drawable filter undos  */
GimpUndo * gimp_image_undo_push_filter_add          (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpDrawable  *drawable,
                                                     GimpDrawableFilter
                                                                   *filter);

GimpUndo * gimp_image_undo_push_filter_remove       (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpDrawable  *drawable,
                                                     GimpDrawableFilter
                                                                   *filter);

GimpUndo * gimp_image_undo_push_filter_reorder      (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpDrawable  *drawable,
                                                     GimpDrawableFilter
                                                                   *filter);


/*  mask undos  */

GimpUndo * gimp_image_undo_push_mask                (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpChannel   *mask);
GimpUndo * gimp_image_undo_push_mask_precision      (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpChannel   *mask);


/*  item undos  */

GimpUndo * gimp_image_undo_push_item_reorder        (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpItem      *item);
GimpUndo * gimp_image_undo_push_item_rename         (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpItem      *item);
GimpUndo * gimp_image_undo_push_item_displace       (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpItem      *item);
GimpUndo * gimp_image_undo_push_item_visibility     (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpItem      *item);
GimpUndo * gimp_image_undo_push_item_color_tag      (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpItem      *item);
GimpUndo * gimp_image_undo_push_item_lock_content   (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpItem      *item);
GimpUndo * gimp_image_undo_push_item_lock_position  (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpItem      *item);
GimpUndo * gimp_image_undo_push_item_lock_visibility (GimpImage    *image,
                                                      const gchar  *undo_desc,
                                                      GimpItem     *item);
GimpUndo * gimp_image_undo_push_item_parasite       (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpItem      *item,
                                                     const GimpParasite *parasite);
GimpUndo * gimp_image_undo_push_item_parasite_remove(GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpItem      *item,
                                                     const gchar   *name);


/*  layer undos  */

GimpUndo * gimp_image_undo_push_layer_add           (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpLayer     *layer,
                                                     GList         *prev_layers);
GimpUndo * gimp_image_undo_push_layer_remove        (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpLayer     *layer,
                                                     GimpLayer     *prev_parent,
                                                     gint           prev_position,
                                                     GList         *prev_layers);
GimpUndo * gimp_image_undo_push_layer_mode          (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpLayer     *layer);
GimpUndo * gimp_image_undo_push_layer_opacity       (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpLayer     *layer);
GimpUndo * gimp_image_undo_push_layer_lock_alpha    (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpLayer     *layer);


/*  group layer undos  */

GimpUndo *
    gimp_image_undo_push_group_layer_suspend_resize (GimpImage      *image,
                                                     const gchar    *undo_desc,
                                                     GimpGroupLayer *group);
GimpUndo *
     gimp_image_undo_push_group_layer_resume_resize (GimpImage      *image,
                                                     const gchar    *undo_desc,
                                                     GimpGroupLayer *group);
GimpUndo *
      gimp_image_undo_push_group_layer_suspend_mask (GimpImage      *image,
                                                     const gchar    *undo_desc,
                                                     GimpGroupLayer *group);
GimpUndo *
       gimp_image_undo_push_group_layer_resume_mask (GimpImage      *image,
                                                     const gchar    *undo_desc,
                                                     GimpGroupLayer *group);
GimpUndo *
   gimp_image_undo_push_group_layer_start_transform (GimpImage      *image,
                                                     const gchar    *undo_desc,
                                                     GimpGroupLayer *group);
GimpUndo *
     gimp_image_undo_push_group_layer_end_transform (GimpImage      *image,
                                                     const gchar    *undo_desc,
                                                     GimpGroupLayer *group);
GimpUndo * gimp_image_undo_push_group_layer_convert (GimpImage      *image,
                                                     const gchar    *undo_desc,
                                                     GimpGroupLayer *group);


/*  text layer undos  */

GimpUndo * gimp_image_undo_push_text_layer          (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpTextLayer *layer,
                                                     const GParamSpec *pspec);
GimpUndo * gimp_image_undo_push_text_layer_modified (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpTextLayer *layer);
GimpUndo * gimp_image_undo_push_text_layer_convert  (GimpImage     *image,
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
                                                     GimpLayer     *layer);
GimpUndo * gimp_image_undo_push_layer_mask_show     (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpLayer     *layer);


/*  channel undos  */

GimpUndo * gimp_image_undo_push_channel_add         (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpChannel   *channel,
                                                     GList         *prev_channels);
GimpUndo * gimp_image_undo_push_channel_remove      (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpChannel   *channel,
                                                     GimpChannel   *prev_parent,
                                                     gint           prev_position,
                                                     GList         *prev_channels);
GimpUndo * gimp_image_undo_push_channel_color       (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpChannel   *channel);


/*  path undos  */

GimpUndo * gimp_image_undo_push_path_add            (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpPath      *path,
                                                     GList         *prev_paths);
GimpUndo * gimp_image_undo_push_path_remove         (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpPath      *path,
                                                     GimpPath      *prev_parent,
                                                     gint           prev_position,
                                                     GList         *prev_paths);
GimpUndo * gimp_image_undo_push_path_mod            (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpPath      *path);


/*  floating selection undos  */

GimpUndo * gimp_image_undo_push_fs_to_layer         (GimpImage     *image,
                                                     const gchar   *undo_desc,
                                                     GimpLayer     *floating_layer);


/*  EEK undo  */

GimpUndo * gimp_image_undo_push_cantundo            (GimpImage     *image,
                                                     const gchar   *undo_desc);


#endif  /* __GIMP_IMAGE_UNDO_PUSH_H__ */
