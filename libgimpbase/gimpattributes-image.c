/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpattributes-image.c
 * Copyright (C) 2014  Hartmut Kuhse <hatti@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>

#include <glib-object.h>
#include <gexiv2/gexiv2.h>

#ifdef G_OS_WIN32
#endif

#include "libgimpmath/gimpmath.h"

#include "gimprational.h"
#include "gimpbasetypes.h"
#include "gimpattribute.h"
#include "gimpattributes.h"
#include "gimplimits.h"
#include "gimpmetadata.h"
#include "gimpunit.h"
#include "gimpattributes-image.h"

/**
 * SECTION: gimpattributes-image
 * @title: gimpattributes-image
 * @short_description: handle image data.
 * @see_also: GimpAttribute, GimpAttributes
 *
 * handle image data.
 **/


/**
 * gimp_metadata_set_resolution:
 * @metadata: A #GimpMetadata instance.
 * @xres:     The image's X Resolution, in ppi
 * @yres:     The image's Y Resolution, in ppi
 * @unit:     The image's unit
 *
 * Sets Exif.Image.XResolution, Exif.Image.YResolution and
 * Exif.Image.ResolutionUnit @metadata.
 *
 * Since: GIMP 2.10
 */
void
gimp_attributes_set_resolution (GimpAttributes *attributes,
                                gdouble         xres,
                                gdouble         yres,
                                GimpUnit        unit)
{
  gchar buffer[64];
  gint  exif_unit;
  gint  factor;

  g_return_if_fail (GIMP_IS_ATTRIBUTES (attributes));

  if (gimp_unit_is_metric (unit))
    {
      xres /= 2.54;
      yres /= 2.54;

      exif_unit = 3;
    }
  else
    {
      exif_unit = 2;
    }

  for (factor = 1; factor <= 100 /* arbitrary */; factor++)
    {
      if (fabs (xres * factor - ROUND (xres * factor)) < 0.01 &&
          fabs (yres * factor - ROUND (yres * factor)) < 0.01)
        break;
    }

  g_snprintf (buffer, sizeof (buffer), "%d/%d", ROUND (xres * factor), factor);
  gimp_attributes_new_attribute (attributes, "Exif.Image.XResolution", buffer, TYPE_RATIONAL);

  g_snprintf (buffer, sizeof (buffer), "%d/%d", ROUND (yres * factor), factor);
  gimp_attributes_new_attribute (attributes, "Exif.Image.YResolution", buffer, TYPE_RATIONAL);

  g_snprintf (buffer, sizeof (buffer), "%d", exif_unit);
  gimp_attributes_new_attribute (attributes, "Exif.Image.ResolutionUnit", buffer, TYPE_SHORT);
}

/**
 * gimp_attributes_get_resolution:
 * @metadata: A #GimpMetadata instance.
 * @xres:     Return location for the X Resolution, in ppi
 * @yres:     Return location for the Y Resolution, in ppi
 * @unit:     Return location for the unit unit
 *
 * Returns values based on Exif.Image.XResolution,
 * Exif.Image.YResolution and Exif.Image.ResolutionUnit of @metadata.
 *
 * Return value: %TRUE on success, %FALSE otherwise.
 *
 * Since: GIMP 2.10
 */
gboolean
gimp_attributes_get_resolution (GimpAttributes *attributes,
                                gdouble        *xres,
                                gdouble        *yres,
                                GimpUnit       *unit)
{
  GimpAttribute *x_attribute;
  GimpAttribute *y_attribute;
  GimpAttribute *res_attribute;

  gint   xnom, xdenom;
  gint   ynom, ydenom;
  gint   exif_unit = 2;

  g_return_val_if_fail (GIMP_IS_ATTRIBUTES (attributes), FALSE);

  x_attribute = gimp_attributes_get_attribute (attributes, "Exif.Image.XResolution");
  y_attribute = gimp_attributes_get_attribute (attributes, "Exif.Image.YResolution");
  res_attribute = gimp_attributes_get_attribute (attributes, "Exif.Image.ResolutionUnit");

  if (x_attribute)
    {
      GValue    value;
      Rational *rats;
      gint     *_nom;
      gint     *_denom;
      gint      l;

      value = gimp_attribute_get_value (x_attribute);

      rats = g_value_get_boxed (&value);

      rational_to_int (rats, &_nom, &_denom, &l);

      if (l > 0)
        {
          xnom = _nom[0];
          xdenom = _denom[0];
        }
      rational_free (rats);
    }
  else
    return FALSE;

  if (y_attribute)
    {
      GValue    value;
      Rational *rats;
      gint     *_nom;
      gint     *_denom;
      gint      l;

      value = gimp_attribute_get_value (y_attribute);

      rats = g_value_get_boxed (&value);

      rational_to_int (rats, &_nom, &_denom, &l);

      if (l > 0)
        {
          ynom = _nom[0];
          ydenom = _denom[0];
        }
      rational_free (rats);
    }
  else
    return FALSE;

  if (res_attribute)
    {
      GValue value = gimp_attribute_get_value (res_attribute);

      exif_unit = (gint) g_value_get_uint (&value);
    }

  if (xnom != 0 && xdenom != 0 &&
      ynom != 0 && ydenom != 0)
    {
      gdouble xresolution = (gdouble) xnom / (gdouble) xdenom;
      gdouble yresolution = (gdouble) ynom / (gdouble) ydenom;

      if (exif_unit == 3)
        {
          xresolution *= 2.54;
          yresolution *= 2.54;
        }

      if (xresolution >= GIMP_MIN_RESOLUTION &&
          xresolution <= GIMP_MAX_RESOLUTION &&
          yresolution >= GIMP_MIN_RESOLUTION &&
          yresolution <= GIMP_MAX_RESOLUTION)
        {
          if (xres)
            *xres = xresolution;

          if (yres)
            *yres = yresolution;

          if (unit)
            {
              if (exif_unit == 3)
                *unit = GIMP_UNIT_MM;
              else
                *unit = GIMP_UNIT_INCH;
            }

          return TRUE;
        }
    }
  return FALSE;
}

/**
 * gimp_attributes_set_bits_per_sample:
 * @metadata: A #GimpMetadata instance.
 * @bps:      Bytes per pixel, per component
 *
 * Sets Exif.Image.BitsPerSample on @metadata.
 *
 * Since: GIMP 2.10
 */
void
gimp_attributes_set_bits_per_sample (GimpAttributes *attributes,
                                     gint            bps)
{
  gchar buffer[32];

  g_return_if_fail (GIMP_IS_ATTRIBUTES (attributes));

  g_snprintf (buffer, sizeof (buffer), "%d", bps);
  gimp_attributes_new_attribute (attributes, "Exif.Image.BitsPerSample", buffer, TYPE_SHORT);
}

/**
 * gimp_attributes_set_pixel_size:
 * @metadata: A #GimpMetadata instance.
 * @width:    Width in pixels
 * @height:   Height in pixels
 *
 * Sets Exif.Image.ImageWidth and Exif.Image.ImageLength on @metadata.
 *
 * Since: GIMP 2.10
 */
void
gimp_attributes_set_pixel_size (GimpAttributes *attributes,
                                gint            width,
                                gint            height)
{
  gchar buffer[32];

  g_return_if_fail (GIMP_IS_ATTRIBUTES (attributes));

  g_snprintf (buffer, sizeof (buffer), "%d", width);
  gimp_attributes_new_attribute (attributes, "Exif.Image.ImageWidth", buffer, TYPE_LONG);

  g_snprintf (buffer, sizeof (buffer), "%d", height);
  gimp_attributes_new_attribute (attributes, "Exif.Image.ImageLength", buffer, TYPE_LONG);
}
