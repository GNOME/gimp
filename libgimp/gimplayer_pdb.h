/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimplayer_pdb.h
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

#ifndef __GIMP_LAYER_PDB_H__
#define __GIMP_LAYER_PDB_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* For information look into the C source or the html documentation */


gint32          gimp_layer_new                       (gint32        image_ID,
						      char         *name,
						      guint         width,
						      guint         height,
						      GimpImageType type,
						      gdouble       opacity,
						      GLayerMode    mode);
gint32          gimp_layer_copy                      (gint32        layer_ID);
void            gimp_layer_delete                    (gint32        layer_ID);
void            gimp_layer_add_alpha                 (gint32        layer_ID);
gint32          gimp_layer_create_mask               (gint32        layer_ID,
						      GimpAddMaskType mask_type);
void            gimp_layer_resize                    (gint32        layer_ID,
						      guint         new_width,
						      guint         new_height,
						      gint          offset_x,
						      gint          offset_y);
void            gimp_layer_scale                     (gint32        layer_ID,
						      guint         new_width,
						      guint         new_height,
						      gint          local_origin);
void            gimp_layer_translate                 (gint32        layer_ID,
						      gint          offset_x,
						      gint          offset_y);
gboolean        gimp_layer_is_floating_selection     (gint32        layer_ID);
gint32          gimp_layer_get_image_id              (gint32        layer_ID);
gint32          gimp_layer_get_mask_id               (gint32        layer_ID);
gboolean        gimp_layer_get_apply_mask            (gint32        layer_ID);
gboolean        gimp_layer_get_edit_mask             (gint32        layer_ID);
GLayerMode      gimp_layer_get_mode                  (gint32        layer_ID);
gchar         * gimp_layer_get_name                  (gint32        layer_ID);
gdouble         gimp_layer_get_opacity               (gint32        layer_ID);
gboolean        gimp_layer_get_preserve_transparency (gint32        layer_ID);
gint            gimp_layer_get_show_mask             (gint32        layer_ID);
gint            gimp_layer_get_visible               (gint32        layer_ID);
void            gimp_layer_set_apply_mask            (gint32        layer_ID,
						      gboolean      apply_mask);
void            gimp_layer_set_edit_mask             (gint32        layer_ID,
						      gboolean      edit_mask);
void            gimp_layer_set_mode                  (gint32        layer_ID,
						      GLayerMode    mode);
void            gimp_layer_set_name                  (gint32        layer_ID,
						      gchar        *name);
void            gimp_layer_set_offsets               (gint32        layer_ID,
						      gint          offset_x,
						      gint          offset_y);
void            gimp_layer_set_opacity               (gint32        layer_ID,
						      gdouble       opacity);
void            gimp_layer_set_preserve_transparency (gint32        layer_ID,
						      gboolean      preserve_transparency);
void            gimp_layer_set_show_mask             (gint32        layer_ID,
						      gboolean      show_mask);
void            gimp_layer_set_visible               (gint32        layer_ID,
						      gboolean      visible);
gint32          gimp_layer_get_tattoo                (gint32        layer_ID);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_LAYER_PDB_H__ */
