#/* The GIMP -- an image manipulation program
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


/*  pure wrappers around the resp. GimpChannel::foo() functions:  */

gboolean      gimp_image_mask_boundary    (GimpImage       *gimage,
                                           const BoundSeg **segs_in,
                                           const BoundSeg **segs_out,
                                           gint            *num_segs_in,
                                           gint            *num_segs_out);
gboolean      gimp_image_mask_bounds      (GimpImage       *gimage,
                                           gint            *x1,
                                           gint            *y1,
                                           gint            *x2,
                                           gint            *y2);
gint          gimp_image_mask_value       (GimpImage       *gimage,
                                           gint             x,
                                           gint             y);
gboolean      gimp_image_mask_is_empty    (GimpImage       *gimage);


/*  pure wrappers around the resp. GimpSelection functions:  */

void          gimp_image_mask_push_undo   (GimpImage       *gimage,
                                           const gchar     *undo_desc);
void          gimp_image_mask_invalidate  (GimpImage       *gimage);


/*  really implemented here:  */

TileManager * gimp_image_mask_extract     (GimpImage       *gimage,
                                           GimpDrawable    *drawable,
                                           gboolean         cut_image,
                                           gboolean         keep_indexed,
                                           gboolean         add_alpha);

GimpLayer   * gimp_image_mask_float       (GimpImage       *gimage,
                                           GimpDrawable    *drawable,
                                           gboolean         cut_image,
                                           gint             off_x,
                                           gint             off_y);

void          gimp_image_mask_load        (GimpImage       *gimage,
                                           GimpChannel     *channel);
GimpChannel * gimp_image_mask_save        (GimpImage       *gimage);


#endif  /*  __GIMP_IMAGE_MASK_H__  */
