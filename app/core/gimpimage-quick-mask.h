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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once


/*  don't change this string, it's used to identify the Quick Mask
 *  when opening files.
 */
#define GIMP_IMAGE_QUICK_MASK_NAME "Qmask"


void          gimp_image_set_quick_mask_state    (GimpImage     *image,
                                                  gboolean       active);
gboolean      gimp_image_get_quick_mask_state    (GimpImage     *image);

void          gimp_image_set_quick_mask_color    (GimpImage     *image,
                                                  GeglColor     *color);
GeglColor   * gimp_image_get_quick_mask_color    (GimpImage     *image);

GimpChannel * gimp_image_get_quick_mask          (GimpImage     *image);

void          gimp_image_quick_mask_invert       (GimpImage     *image);
gboolean      gimp_image_get_quick_mask_inverted (GimpImage     *image);
