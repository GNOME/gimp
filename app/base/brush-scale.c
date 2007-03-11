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

#include "config.h"

#include <glib-object.h>

#include "base-types.h"

#include "brush-scale.h"
#include "temp-buf.h"
#include "pixel-region.h"

#include "paint-funcs/scale-funcs.h"


MaskBuf *
brush_scale_buf (MaskBuf *brush_mask,
                 gint     dest_width,
                 gint     dest_height,
                 gint     bpp)
{
  PixelRegion  source_region;
  PixelRegion  dest_region;
  MaskBuf     *dest_brush_mask;

  /* Use existing scaling routines for brush scaling. */

  pixel_region_init_temp_buf (&source_region, brush_mask,
                              brush_mask->x,
                              brush_mask->y,
                              brush_mask->width,
                              brush_mask->height);

  dest_brush_mask = mask_buf_new (dest_width, dest_height, bpp);
  g_return_val_if_fail (dest_brush_mask != NULL, NULL);

  pixel_region_init_temp_buf (&dest_region, dest_brush_mask,
                              brush_mask->x,
                              brush_mask->y,
                              dest_width,
                              dest_height);

  scale_region (&source_region, &dest_region, GIMP_INTERPOLATION_LINEAR,
                NULL, NULL);

  return dest_brush_mask;
}
