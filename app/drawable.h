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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __DRAWABLE_H__
#define __DRAWABLE_H__

#include "gimage.h"

/*  Drawable access functions  */
void             drawable_apply_image        (int, int, int, int, int, TileManager *, int);
void             drawable_merge_shadow       (int, int);
void             drawable_fill               (int, int);
void             drawable_update             (int, int, int, int, int);
int              drawable_mask_bounds        (int, int *, int *, int *, int *);
void             drawable_invalidate_preview (int);
int              drawable_dirty              (int);
int              drawable_clean              (int);
GImage *         drawable_gimage             (int);
int              drawable_type               (int);
int              drawable_has_alpha          (int);
int              drawable_type_with_alpha    (int);
int              drawable_color              (int);
int              drawable_gray               (int);
int              drawable_indexed            (int);
TileManager *    drawable_data               (int);
TileManager *    drawable_shadow             (int);
int              drawable_bytes              (int);
int              drawable_width              (int);
int              drawable_height             (int);
void             drawable_offsets            (int, int *, int *);
unsigned char *  drawable_cmap               (int);
Layer *          drawable_layer              (int);
Channel *        drawable_layer_mask         (int);
Channel *        drawable_channel            (int);

#endif /* __DRAWABLE_H__ */
