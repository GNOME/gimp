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

#ifndef __GIMP_IMAGE_MASK_H__
#define __GIMP_IMAGE_MASK_H__


gboolean      gimp_image_mask_boundary    (GimpImage    *gimage,
                                           BoundSeg    **segs_in,
                                           BoundSeg    **segs_out,
                                           gint         *num_segs_in,
                                           gint         *num_segs_out);

gboolean      gimp_image_mask_bounds      (GimpImage    *gimage,
                                           gint         *x1,
                                           gint         *y1,
                                           gint         *x2,
                                           gint         *y2);

void          gimp_image_mask_invalidate  (GimpImage    *gimage);

gint          gimp_image_mask_value       (GimpImage    *gimage,
                                           gint          x,
                                           gint          y);

gboolean      gimp_image_mask_is_empty    (GimpImage    *gimage);

TileManager * gimp_image_mask_extract     (GimpImage    *gimage,
                                           GimpDrawable *drawable,
                                           gboolean      cut_gimage,
                                           gboolean      keep_indexed,
                                           gboolean      add_alpha);

GimpLayer   * gimp_image_mask_float       (GimpImage    *gimage,
                                           GimpDrawable *drawable,
                                           gint          off_x,
                                           gint          off_y);

void          gimp_image_mask_push_undo   (GimpImage    *gimage);

void          gimp_image_mask_feather     (GimpImage    *gimage,
                                           gdouble       feather_radius_x,
                                           gdouble       feather_radius_y);
void          gimp_image_mask_sharpen     (GimpImage    *gimage);

void          gimp_image_mask_clear       (GimpImage    *gimage);
void          gimp_image_mask_all         (GimpImage    *gimage);
void          gimp_image_mask_invert      (GimpImage    *gimage);

void          gimp_image_mask_border      (GimpImage    *gimage,
                                           gint          border_radius_x,
                                           gint          border_radius_y);

void          gimp_image_mask_grow        (GimpImage    *gimage,
                                           gint          grow_pixels_x,
                                           gint          grow_pixels_y);

void          gimp_image_mask_shrink      (GimpImage    *gimage,
                                           gint          shrink_pixels_x,
                                           gint          shrink_pixels_y,
                                           gboolean      edge_lock);

void          gimp_image_mask_translate   (GimpImage    *gimage,
                                           gint          off_x,
                                           gint          off_y);

void          gimp_image_mask_load        (GimpImage    *gimage,
                                           GimpChannel  *channel);
void          gimp_image_mask_layer_alpha (GimpImage    *gimage,
                                           GimpLayer    *layer);
void          gimp_image_mask_layer_mask  (GimpImage    *gimage,
                                           GimpLayer    *layer);

GimpChannel * gimp_image_mask_save        (GimpImage    *gimage);

gboolean      gimp_image_mask_stroke      (GimpImage    *gimage,
                                           GimpDrawable *drawable,
                                           GimpContext  *context);


#endif  /*  __GIMP_IMAGE_MASK_H__  */
