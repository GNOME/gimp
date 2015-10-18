/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpattributes-image.h
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


#ifndef __GIMPATTRIBUTES_IMAGE_H__
#define __GIMPATTRIBUTES_IMAGE_H__

G_BEGIN_DECLS

typedef enum
{
  GIMP_ATTRIBUTES_COLORSPACE_UNSPECIFIED,
  GIMP_ATTRIBUTES_COLORSPACE_UNCALIBRATED,
  GIMP_ATTRIBUTES_COLORSPACE_SRGB,
  GIMP_ATTRIBUTES_COLORSPACE_ADOBERGB
} GimpAttributesColorspace;

gboolean                 gimp_attributes_get_resolution            (GimpAttributes           *attributes,
                                                                    gdouble                  *xres,
                                                                    gdouble                  *yres,
                                                                    GimpUnit                 *unit);

void                     gimp_attributes_set_resolution            (GimpAttributes           *attributes,
                                                                    gdouble                   xres,
                                                                    gdouble                   yres,
                                                                    GimpUnit                  unit);

void                     gimp_attributes_set_bits_per_sample       (GimpAttributes           *attributes,
                                                                    gint                      bps);

void                     gimp_attributes_set_pixel_size            (GimpAttributes           *attributes,
                                                                    gint                      width,
                                                                    gint                      height);

GimpAttributesColorspace gimp_attributes_get_colorspace            (GimpAttributes           *attributes);

void                     gimp_attributes_set_colorspace            (GimpAttributes           *attributes,
                                                                    GimpAttributesColorspace  colorspace);
G_END_DECLS

#endif
