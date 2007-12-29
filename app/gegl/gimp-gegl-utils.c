/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-gegl-utils.h
 * Copyright (C) 2007 Michael Natterer <mitch@gimp.org>
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

#include <gegl.h>

#include "gegl-types.h"

#include "gimp-gegl-utils.h"


const Babl *
gimp_bpp_to_babl_format (guint bpp)
{
  g_return_val_if_fail (bpp > 0 && bpp <= 4, NULL);

  switch (bpp)
    {
    case 1:
      return babl_format ("Y' u8");
    case 2:
      return babl_format ("Y'A u8");
    case 3:
      return babl_format ("R'G'B' u8");
    case 4:
      return babl_format ("R'G'B'A u8");
    }

  return NULL;
}
