/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpimage_pdb.h
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */         

#ifndef __GIMP_IMAGE_PDB_H__
#define __GIMP_IMAGE_PDB_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* For information look into the C source or the html documentation */


gint32         gimp_image_new                   (guint          width,
						 guint          height,
						 GimpImageBaseType type);
gint32         gimp_image_duplicate             (gint32         image_ID);
void           gimp_image_delete                (gint32         image_ID);
guint          gimp_image_width                 (gint32         image_ID);
guint          gimp_image_height                (gint32         image_ID);
GimpImageBaseType  gimp_image_base_type         (gint32         image_ID);
gint32         gimp_image_floating_selection    (gint32         image_ID);
void           gimp_image_add_channel           (gint32         image_ID,
						 gint32         channel_ID,
						 gint           position);
void           gimp_image_add_layer             (gint32         image_ID,
						 gint32         layer_ID,
						 gint           position);
void           gimp_image_add_layer_mask        (gint32         image_ID,
						 gint32         layer_ID,
						 gint32         mask_ID);
void           gimp_image_clean_all             (gint32         image_ID);
void           gimp_image_undo_disable          (gint32         image_ID);
void           gimp_image_undo_enable           (gint32         image_ID);
void           gimp_image_undo_freeze           (gint32         image_ID);
void           gimp_image_undo_thaw             (gint32         image_ID);
void           gimp_undo_push_group_start       (gint32         image_ID);
void           gimp_undo_push_group_end         (gint32         image_ID);
void           gimp_image_clean_all             (gint32         image_ID);
gint32         gimp_image_flatten               (gint32         image_ID);
void           gimp_image_lower_channel         (gint32         image_ID,
						 gint32         channel_ID);
void           gimp_image_lower_layer           (gint32         image_ID,
						 gint32         layer_ID);
gint32         gimp_image_merge_visible_layers  (gint32         image_ID,
						 GimpMergeType  merge_type);
gint32         gimp_image_pick_correlate_layer  (gint32         image_ID,
						 gint           x,
						 gint           y);
void           gimp_image_raise_channel         (gint32         image_ID,
						 gint32         channel_ID);
void           gimp_image_raise_layer           (gint32         image_ID,
						 gint32         layer_ID);
void           gimp_image_remove_channel        (gint32         image_ID,
						 gint32         channel_ID);
void           gimp_image_remove_layer          (gint32         image_ID,
						 gint32         layer_ID);
void           gimp_image_remove_layer_mask     (gint32         image_ID,
						 gint32         layer_ID,
						 gint           mode);
void           gimp_image_resize                (gint32         image_ID,
						 guint          new_width,
						 guint          new_height,
						 gint           offset_x,
						 gint           offset_y);
gint32         gimp_image_get_active_channel    (gint32         image_ID);
gint32         gimp_image_get_active_layer      (gint32         image_ID);
gint32       * gimp_image_get_channels          (gint32         image_ID,
						 gint          *nchannels);
guchar       * gimp_image_get_cmap              (gint32         image_ID,
						 gint          *ncolors);
gboolean       gimp_image_get_component_active  (gint32         image_ID,
						 gint           component);
gboolean       gimp_image_get_component_visible (gint32         image_ID,
						 gint           component);
gchar        * gimp_image_get_filename          (gint32         image_ID);
gint32       * gimp_image_get_layers            (gint32         image_ID,
						 gint          *nlayers);
gint32         gimp_image_get_selection         (gint32         image_ID);
void           gimp_image_set_active_channel    (gint32         image_ID,
						 gint32         channel_ID);
void           gimp_image_set_active_layer      (gint32         image_ID,
						 gint32         layer_ID);
void           gimp_image_set_cmap              (gint32         image_ID,
						 guchar        *cmap,
						 gint           ncolors);
void           gimp_image_set_component_active  (gint32         image_ID,
						 gint           component,
						 gboolean       active);
void           gimp_image_set_component_visible (gint32         image_ID,
						 gint           component,
						 gboolean       visible);
void           gimp_image_set_filename          (gint32         image_ID,
						 gchar         *name);
GimpParasite * gimp_image_parasite_find         (gint32         image_ID,
						 const gchar   *name);
void           gimp_image_parasite_attach       (gint32         image_ID,
						 const GimpParasite *parasite);
void           gimp_image_attach_new_parasite   (gint32         image_ID,
						 const gchar   *name, 
						 gint           flags,
						 gint           size, 
						 const gpointer data);
void           gimp_image_parasite_detach       (gint32         image_ID,
						 const gchar   *name);
void           gimp_image_set_resolution        (gint32         image_ID,
						 gdouble        xresolution,
						 gdouble        yresolution);
void           gimp_image_get_resolution        (gint32         image_ID,
						 gdouble       *xresolution,
						 gdouble       *yresolution);
void           gimp_image_set_unit              (gint32         image_ID,
						 GimpUnit       unit);
GimpUnit       gimp_image_get_unit              (gint32         image_ID);
gint32         gimp_image_get_layer_by_tattoo   (gint32         image_ID,
						 gint32         tattoo);
gint32         gimp_image_get_channel_by_tattoo (gint32         image_ID,
						 gint32         tattoo);

guchar       * gimp_image_get_thumbnail_data    (gint32         image_ID,
						 gint          *width,
						 gint          *height,
						 gint          *bytes);
void           gimp_image_convert_rgb           (gint32         image_ID);
void           gimp_image_convert_grayscale     (gint32         image_ID);
void           gimp_image_convert_indexed       (gint32         image_ID,
						 GimpConvertDitherType  dither_type,
						 GimpConvertPaletteType palette_type,
						 gint           num_colors,
						 gint           alpha_dither,
						 gint           remove_unused,
						 gchar         *palette);

/****************************************
 *              Guides                  *
 ****************************************/

gint32        gimp_image_add_hguide            (gint32     image_ID,
						gint32     yposition);
gint32        gimp_image_add_vguide            (gint32     image_ID,
						gint32     xposition);
void          gimp_image_delete_guide          (gint32     image_ID,
						gint32     guide_ID);
gint32        gimp_image_find_next_guide       (gint32     image_ID,
						gint32     guide_ID);
GOrientation  gimp_image_get_guide_orientation (gint32     image_ID,
						gint32     guide_ID);
gint32        gimp_image_get_guide_position    (gint32     image_ID,
						gint32     guide_ID);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_IMAGE_PDB_H__ */
