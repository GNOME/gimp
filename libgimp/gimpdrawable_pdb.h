/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpdrawable_pdb.h
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

#ifndef __GIMP_DRAWABLE_PDB_H__
#define __GIMP_DRAWABLE_PDB_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* For information look into the C source or the html documentation */


void            gimp_drawable_update              (gint32        drawable_ID,
						   gint          x,
						   gint          y,
						   guint         width,
						   guint         height);
void            gimp_drawable_merge_shadow        (gint32        drawable_ID,
						   gboolean      undoable);
gint32          gimp_drawable_image_id            (gint32        drawable_ID);
guint           gimp_drawable_width               (gint32        drawable_ID);
guint           gimp_drawable_height              (gint32        drawable_ID);
guint           gimp_drawable_bpp                 (gint32        drawable_ID);
GimpImageType   gimp_drawable_type                (gint32        drawable_ID);
gboolean        gimp_drawable_is_channel          (gint32        drawable_ID);
gboolean        gimp_drawable_is_rgb              (gint32        drawable_ID);
gboolean        gimp_drawable_is_gray             (gint32        drawable_ID);
gboolean        gimp_drawable_has_alpha           (gint32        drawable_ID);
gboolean        gimp_drawable_is_indexed          (gint32        drawable_ID);
gboolean        gimp_drawable_is_layer            (gint32        drawable_ID);
gboolean        gimp_drawable_is_layer_mask       (gint32        drawable_ID);
gboolean        gimp_drawable_mask_bounds         (gint32        drawable_ID,
						   gint         *x1,
						   gint         *y1,
						   gint         *x2,
						   gint         *y2);
void            gimp_drawable_offsets             (gint32        drawable_ID,
						   gint         *offset_x,
						   gint         *offset_y);
void            gimp_drawable_fill                (gint32        drawable_ID,
						   GimpFillType  fill_type);
void            gimp_drawable_set_visible         (gint32        drawable_ID,
						   gint          visible);
guchar        * gimp_drawable_get_thumbnail_data  (gint32        drawable_ID,
						   gint         *width,
						   gint         *height,
						   gint         *bytes);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_DRAWABLE_PDB_H__ */
