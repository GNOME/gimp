/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattisbvf
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

#ifndef __GIMP_IMAGE_SCALE_H__
#define __GIMP_IMAGE_SCALE_H__


void   gimp_image_scale         (GimpImage             *image,
                                 gint                   new_width,
                                 gint                   new_height,
                                 GimpInterpolationType  interpolation_type,
                                 GimpProgress          *progress);

GimpImageScaleCheckType
       gimp_image_scale_check   (GimpImage             *image,
                                 gint                   new_width,
                                 gint                   new_height,
                                 gint64                 max_memsize,
                                 gint64                *new_memsize);


#endif /* __GIMP_IMAGE_SCALE_H__ */
