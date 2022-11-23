/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_IMAGE_ROTATE_H__
#define __LIGMA_IMAGE_ROTATE_H__


void   ligma_image_rotate                   (LigmaImage        *image,
                                            LigmaContext      *context,
                                            LigmaRotationType  rotate_type,
                                            LigmaProgress     *progress);

void   ligma_image_import_rotation_metadata (LigmaImage        *image,
                                            LigmaContext      *context,
                                            LigmaProgress     *progress,
                                            gboolean          interactive);

#endif /* __LIGMA_IMAGE_ROTATE_H__ */
