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

#ifndef __GIMP_IMAGE_CROP_H__
#define __GIMP_IMAGE_CROP_H__


void       gimp_image_crop             (GimpImage   *image,
                                        GimpContext *context,
                                        gint         x1,
                                        gint         y1,
                                        gint         x2,
                                        gint         y2,
                                        gboolean     active_layer_only,
                                        gboolean     crop_layers);

gboolean   gimp_image_crop_auto_shrink (GimpImage   *image,
                                        gint         x1,
                                        gint         y1,
                                        gint         x2,
                                        gint         y2,
                                        gboolean     active_drawable_only,
                                        gint        *shrunk_x1,
                                        gint        *shrunk_y1,
                                        gint        *shrunk_x2,
                                        gint        *shrunk_y2);


#endif  /* __GIMP_IMAGE_CROP_H__ */
