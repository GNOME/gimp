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


/**
 * gimp_bpp_to_babl_format:
 * @bpp: bytes per pixel
 *
 * Return the Babl format to use for a given number of bytes per pixel.
 * This function assumes that the data is 8bit gamma-corrected data.
 *
 * Return value: the Babl format to use
 **/
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

/**
 * gimp_bpp_to_babl_format_linear:
 * @bpp: bytes per pixel
 *
 * Return the Babl format to use for a given number of bytes per pixel.
 * This function assumes that the data is 8bit linear.
 *
 * Return value: the Babl format to use
 **/
const Babl *
gimp_bpp_to_babl_format_linear (guint bpp)
{
  g_return_val_if_fail (bpp > 0 && bpp <= 4, NULL);

  switch (bpp)
    {
    case 1:
      return babl_format ("Y u8");
    case 2:
      return babl_format ("YA u8");
    case 3:
      return babl_format ("RGB u8");
    case 4:
      return babl_format ("RGBA u8");
    }

  return NULL;
}

/**
 * gimp_gegl_check_version:
 * @required_major: the required major version.
 * @required_minor: the required minor version.
 * @required_micro: the required micro version.
 * 
 * Return value: %TRUE if the GEGL library in use is the given
 *               version or newer, %FALSE otherwise
 */
gboolean
gimp_gegl_check_version (guint required_major,
			 guint required_minor,
			 guint required_micro)
{
  gint major, minor, micro;

  if (required_major != GEGL_MAJOR_VERSION)
    return FALSE; /* major mismatch */

  gegl_get_version (&major, &minor, &micro);

  return (100 * minor + micro >= 100 * required_minor + required_micro);
}
