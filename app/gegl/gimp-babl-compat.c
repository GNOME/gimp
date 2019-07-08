/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-babl-compat.h
 * Copyright (C) 2012 Michael Natterer <mitch@gimp.org>
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

#include "config.h"

#include <string.h>

#include <gegl.h>

#include "gimp-gegl-types.h"

#include "gimp-babl.h"
#include "gimp-babl-compat.h"


GimpImageType
gimp_babl_format_get_image_type (const Babl *format)
{
  const gchar *name;

  g_return_val_if_fail (format != NULL, -1);

  name = babl_get_name (babl_format_get_model (format));

  if (! strcmp (name, "Y")  ||
      ! strcmp (name, "Y'") ||
      ! strcmp (name, "Y~"))
    {
      return GIMP_GRAY_IMAGE;
    }
  else if (! strcmp (name, "YA")   ||
           ! strcmp (name, "Y'A") ||
           ! strcmp (name, "Y~A"))
    {
      return GIMP_GRAYA_IMAGE;
    }
  else if (! strcmp (name, "RGB")    ||
           ! strcmp (name, "R'G'B'") ||
           ! strcmp (name, "R~G~B~"))
    {
      return GIMP_RGB_IMAGE;
    }
  else if (! strcmp (name, "RGBA")    ||
           ! strcmp (name, "R'G'B'A") ||
           ! strcmp (name, "R~G~B~A"))
    {
      return GIMP_RGBA_IMAGE;
    }
  else if (babl_format_is_palette (format))
    {
      if (babl_format_has_alpha (format))
        return GIMP_INDEXEDA_IMAGE;
      else
        return GIMP_INDEXED_IMAGE;
    }

  g_return_val_if_reached (-1);
}

const Babl *
gimp_babl_compat_u8_format (const Babl *format)
{
  g_return_val_if_fail (format != NULL, NULL);

  /*  indexed images only exist in u8, return the same format  */
  if (babl_format_is_palette (format))
    return format;

  return gimp_babl_format (gimp_babl_format_get_base_type (format),
                           GIMP_PRECISION_U8_NON_LINEAR,
                           babl_format_has_alpha (format),
                           babl_format_get_space (format));
}

const Babl *
gimp_babl_compat_u8_mask_format (const Babl *format)
{
  g_return_val_if_fail (format != NULL, NULL);

  return gimp_babl_format (gimp_babl_format_get_base_type (format),
                           GIMP_PRECISION_U8_LINEAR,
                           FALSE,
                           NULL);
}
