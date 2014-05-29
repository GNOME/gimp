/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_IMAGE_PROFILE_H__
#define __GIMP_IMAGE_PROFILE_H__


#define GIMP_ICC_PROFILE_PARASITE_NAME "icc-profile"


gboolean             gimp_image_validate_icc_profile (GimpImage           *image,
                                                      const GimpParasite  *icc_profile,
                                                      GError             **error);
const GimpParasite * gimp_image_get_icc_profile      (GimpImage           *image);
void                 gimp_image_set_icc_profile      (GimpImage           *image,
                                                      const GimpParasite  *icc_profile);

GimpColorProfile     gimp_image_get_profile          (GimpImage           *image,
                                                      GError             **error);


#endif /* __GIMP_IMAGE_PROFILE_H__ */
