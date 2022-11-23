/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaimage-convert-precision.h
 * Copyright (C) 2012 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_IMAGE_CONVERT_PRECISION_H__
#define __LIGMA_IMAGE_CONVERT_PRECISION_H__


void   ligma_image_convert_precision (LigmaImage        *image,
                                     LigmaPrecision     precision,
                                     GeglDitherMethod  layer_dither_type,
                                     GeglDitherMethod  text_layer_dither_type,
                                     GeglDitherMethod  mask_dither_type,
                                     LigmaProgress     *progress);

void   ligma_image_convert_dither_u8 (LigmaImage        *image,
                                     LigmaProgress     *progress);


#endif  /*  __LIGMA_IMAGE_CONVERT_PRECISION_H__  */
