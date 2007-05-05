/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimplut.h: Copyright (C) 1999 Jay Cox <jaycox@gimp.org>
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

#ifndef __GIMP_LUT_H__
#define __GIMP_LUT_H__


struct _GimpLut
{
  guchar **luts;
  gint     nchannels;
};

/* TODO: the GimpLutFunc should really be passed the ColorModel of the
 * region, not just the number of channels
 */

/* GimpLutFuncs should assume that the input and output gamma are 1.0
 * and do no correction as this will be handled by gimp_lut_setup
 */
typedef gfloat (*GimpLutFunc) (gpointer user_data,
                               gint     nchannels,
                               gint     channel,
                               gfloat   value);


GimpLut * gimp_lut_new            (void);
void      gimp_lut_free           (GimpLut     *lut);

void      gimp_lut_setup          (GimpLut     *lut,
                                   GimpLutFunc  func,
                                   gpointer     user_data,
                                   gint         nchannels);

/* gimp_lut_setup_exact is currently identical to gimp_lut_setup.  It
 * however is guaranteed to never perform any interpolation or gamma
 * correction on the lut
 */
void      gimp_lut_setup_exact    (GimpLut     *lut,
                                   GimpLutFunc  func,
                                   gpointer     user_data,
                                   gint         nchannels);

void      gimp_lut_process        (GimpLut     *lut,
                                   PixelRegion *srcPR,
                                   PixelRegion *destPR);

void      gimp_lut_process_value  (GimpLut     *lut,
                                   PixelRegion *srcPR,
                                   PixelRegion *destPR);

/* gimp_lut_process_inline is like gimp_lut_process except it uses a
 * single PixelRegion as both the source and destination
 */
void      gimp_lut_process_inline (GimpLut     *lut,
                                   PixelRegion *src_destPR);


#endif /* __GIMP_LUT_H__ */
