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
