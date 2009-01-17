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
void   color_balance_create_lookup_tables (ColorBalance     *cb);
void   color_balance                      (ColorBalance     *cb,
                                           PixelRegion      *srcPR,
                                           PixelRegion      *destPR);


#endif  /*  __COLOR_BALANCE_H__  */
