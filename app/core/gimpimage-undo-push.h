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

#ifndef __GIMP_IMAGE_UNDO_PUSH_H__
#define __GIMP_IMAGE_UNDO_PUSH_H__


/*  image undos  */

gboolean gimp_image_undo_push_image_type            (GimpImage     *gimage,
                                                     const gchar   *undo_desc);
gboolean gimp_image_undo_push_image_size            (GimpImage     *gimage,
                                                     const gchar   *undo_desc);
gboolean gimp_image_undo_push_image_resolution      (GimpImage     *gimage,
                                                     const gchar   *undo_desc);
gboolean gimp_image_undo_push_image_grid            (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpGrid      *grid);
gboolean gimp_image_undo_push_image_guide           (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpGuide     *guide);
gboolean gimp_image_undo_push_image_colormap        (GimpImage     *gimage,
                                                     const gchar   *undo_desc);


/*  drawable undo  */

gboolean gimp_image_undo_push_drawable              (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpDrawable  *drawable,
                                                     TileManager   *tiles,
                                                     gboolean       sparse,
                                                     gint           x,
                                                     gint           y,
                                                     gint           width,
                                                     gint           height);
gboolean gimp_image_undo_push_drawable_mod          (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpDrawable  *drawable);


/*  mask undo  */

gboolean gimp_image_undo_push_mask                  (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpChannel   *mask);


/*  item undos  */

gboolean gimp_image_undo_push_item_rename           (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpItem      *item);
gboolean gimp_image_undo_push_item_displace         (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpItem      *item);
gboolean gimp_image_undo_push_item_visibility       (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpItem      *item);
gboolean gimp_image_undo_push_item_linked           (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpItem      *item);


/*  layer undos  */

gboolean gimp_image_undo_push_layer_add             (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpLayer     *layer,
                                                     gint           prev_position,
                                                     GimpLayer     *prev_layer);
gboolean gimp_image_undo_push_layer_remove          (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpLayer     *layer,
                                                     gint           prev_position,
                                                     GimpLayer     *prev_layer);
gboolean gimp_image_undo_push_layer_mask_add        (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpLayer     *layer,
                                                     GimpLayerMask *mask);
gboolean gimp_image_undo_push_layer_mask_remove     (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpLayer     *layer,
                                                     GimpLayerMask *mask);
gboolean gimp_image_undo_push_layer_reposition      (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpLayer     *layer);
gboolean gimp_image_undo_push_layer_mode            (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpLayer     *layer);
gboolean gimp_image_undo_push_layer_opacity         (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpLayer     *layer);
gboolean gimp_image_undo_push_layer_preserve_trans  (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpLayer     *layer);


/*  text layer undos  */

gboolean gimp_image_undo_push_text_layer            (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpTextLayer *layer,
                                                     const GParamSpec *pspec);
gboolean gimp_image_undo_push_text_layer_modified   (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpTextLayer *layer);


/*  channel undos  */

gboolean gimp_image_undo_push_channel_add           (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpChannel   *channel,
                                                     gint           prev_position,
                                                     GimpChannel   *prev_channel);
gboolean gimp_image_undo_push_channel_remove        (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpChannel   *channel,
                                                     gint           prev_position,
                                                     GimpChannel   *prev_channel);
gboolean gimp_image_undo_push_channel_reposition    (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpChannel   *channel);
gboolean gimp_image_undo_push_channel_color         (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpChannel   *channel);


/*  vectors undos  */

gboolean gimp_image_undo_push_vectors_add           (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpVectors   *vectors,
                                                     gint           prev_position,
                                                     GimpVectors   *prev_vectors);
gboolean gimp_image_undo_push_vectors_remove        (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpVectors   *channel,
                                                     gint           prev_position,
                                                     GimpVectors   *prev_vectors);
gboolean gimp_image_undo_push_vectors_mod           (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpVectors   *vectors);
gboolean gimp_image_undo_push_vectors_reposition    (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpVectors   *vectors);


/*  floating selection undos  */

gboolean gimp_image_undo_push_fs_to_layer           (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpLayer     *floating_layer,
                                                     GimpDrawable  *drawable);
gboolean gimp_image_undo_push_fs_rigor              (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpLayer     *floating_layer);
gboolean gimp_image_undo_push_fs_relax              (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpLayer     *floating_layer);


/*  parasite undos  */

gboolean gimp_image_undo_push_image_parasite        (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     gpointer       parasite);
gboolean gimp_image_undo_push_image_parasite_remove (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     const gchar   *name);

gboolean gimp_image_undo_push_item_parasite         (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpItem      *item,
                                                     gpointer       parasite);
gboolean gimp_image_undo_push_item_parasite_remove  (GimpImage     *gimage,
                                                     const gchar   *undo_desc,
                                                     GimpItem      *item,
                                                     const gchar   *name);


/*  EEK undo  */

gboolean gimp_image_undo_push_cantundo              (GimpImage     *gimage,
                                                     const gchar   *undo_desc);


#endif  /* __GIMP_IMAGE_UNDO_PUSH_H__ */
