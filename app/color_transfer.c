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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "appenv.h"
#include "color_transfer.h"

#define  SQR(x) ((x) * (x))

/*  for lightening  */
double  highlights_add[256];
double  midtones_add[256];
double  shadows_add[256];

/*  for darkening  */
double  highlights_sub[256];
double  midtones_sub[256];
double  shadows_sub[256];

/*  color transfer functions  */
void
color_transfer_init ()
{
  int i;

  for (i = 0; i < 256; i++)
    {
      highlights_add[i] = shadows_sub[255 - i] = (1.075 - 1 / ((double) i / 16.0 + 1));
      midtones_add[i] = midtones_sub[i] = 0.667 * (1 - SQR (((double) i - 127.0) / 127.0));
      shadows_add[i] = highlights_sub[i] = 0.667 * (1 - SQR (((double) i - 127.0) / 127.0));
    }
}
