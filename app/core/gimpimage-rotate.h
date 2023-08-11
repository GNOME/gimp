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

#ifndef __GIMP_IMAGE_ROTATE_H__
#define __GIMP_IMAGE_ROTATE_H__


void   gimp_image_rotate                     (GimpImage        *image,
                                              GimpContext      *context,
                                              GimpRotationType  rotate_type,
                                              GimpProgress     *progress);

void   gimp_image_import_rotation_metadata   (GimpImage        *image,
                                              GimpContext      *context,
                                              GimpProgress     *progress,
                                              gboolean          interactive);

void   gimp_image_apply_metadata_orientation (GimpImage         *image,
                                              GimpContext       *context,
                                              GimpMetadata      *metadata,
                                              GimpProgress      *progress);


#endif /* __GIMP_IMAGE_ROTATE_H__ */
