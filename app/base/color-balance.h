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

#ifndef __COLOR_BALANCE_H__
#define __COLOR_BALANCE_H__


struct _ColorBalance
{
  gboolean preserve_luminosity;

  gdouble  cyan_red[3];
  gdouble  magenta_green[3];
  gdouble  yellow_blue[3];

  guchar   r_lookup[256];
  guchar   g_lookup[256];
  guchar   b_lookup[256];
};


void   color_balance_init                 (ColorBalance     *cb);
void   color_balance_range_reset          (ColorBalance     *cb,
                                           GimpTransferMode  range);
void   color_balance_create_lookup_tables (ColorBalance     *cb);
void   color_balance                      (ColorBalance     *cb,
                                           PixelRegion      *srcPR,
                                           PixelRegion      *destPR);


#endif  /*  __COLOR_BALANCE_H__  */
