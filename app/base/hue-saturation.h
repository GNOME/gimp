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

#ifndef __HUE_SATURATION_H__
#define __HUE_SATURATION_H__


struct _HueSaturation
{
  gdouble hue[7];
  gdouble lightness[7];
  gdouble saturation[7];
  gdouble overlap;

  gint    hue_transfer[6][256];
  gint    lightness_transfer[6][256];
  gint    saturation_transfer[6][256];
};


void   hue_saturation_init                (HueSaturation *hs);
void   hue_saturation_partition_reset     (HueSaturation *hs,
                                           GimpHueRange   partition);
void   hue_saturation_calculate_transfers (HueSaturation *hs);
void   hue_saturation                     (HueSaturation *hs,
                                           PixelRegion   *srcPR,
                                           PixelRegion   *destPR);


#endif  /*  __HUE_SATURATION_H__  */
