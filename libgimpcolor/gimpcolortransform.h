/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpcolortransform.h
 * Copyright (C) 2014  Michael Natterer <mitch@gimp.org>
 *                     Elle Stone <ellestone@ninedegreesbelow.com>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_COLOR_H_INSIDE__) && !defined (GIMP_COLOR_COMPILATION)
#error "Only <libgimpcolor/gimpcolor.h> can be included directly."
#endif

#ifndef __GIMP_COLOR_TRANSFORM_H__
#define __GIMP_COLOR_TRANSFORM_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * GimpColorTransformFlags:
 * @GIMP_COLOR_TRANSFORM_FLAGS_NOOPTIMIZE: optimize for accuracy rather
 *   than for speed
 * @GIMP_COLOR_TRANSFORM_FLAGS_GAMUT_CHECK: mark out of gamut colors in the
 *   transform result
 * @GIMP_COLOR_TRANSFORM_FLAGS_BLACK_POINT_COMPENSATION: do black point
 *   compensation
 *
 * Flags for modifying #GimpColorTransform's behavior.
 **/
typedef enum
{
  GIMP_COLOR_TRANSFORM_FLAGS_NOOPTIMIZE               = 0x0100,
  GIMP_COLOR_TRANSFORM_FLAGS_GAMUT_CHECK              = 0x1000,
  GIMP_COLOR_TRANSFORM_FLAGS_BLACK_POINT_COMPENSATION = 0x2000,
} GimpColorTransformFlags;


#define GIMP_TYPE_COLOR_TRANSFORM (gimp_color_transform_get_type ())
G_DECLARE_FINAL_TYPE (GimpColorTransform, gimp_color_transform, GIMP, COLOR_TRANSFORM, GObject)


GimpColorTransform *
        gimp_color_transform_new              (GimpColorProfile         *src_profile,
                                               const Babl               *src_format,
                                               GimpColorProfile         *dest_profile,
                                               const Babl               *dest_format,
                                               GimpColorRenderingIntent  rendering_intent,
                                               GimpColorTransformFlags   flags);

GimpColorTransform *
        gimp_color_transform_new_proofing     (GimpColorProfile         *src_profile,
                                               const Babl               *src_format,
                                               GimpColorProfile         *dest_profile,
                                               const Babl               *dest_format,
                                               GimpColorProfile         *proof_profile,
                                               GimpColorRenderingIntent  proof_intent,
                                               GimpColorRenderingIntent  display_intent,
                                               GimpColorTransformFlags   flags);

void    gimp_color_transform_process_pixels   (GimpColorTransform       *transform,
                                               const Babl               *src_format,
                                               gconstpointer             src_pixels,
                                               const Babl               *dest_format,
                                               gpointer                  dest_pixels,
                                               gsize                     length);

void    gimp_color_transform_process_buffer   (GimpColorTransform       *transform,
                                               GeglBuffer               *src_buffer,
                                               const GeglRectangle      *src_rect,
                                               GeglBuffer               *dest_buffer,
                                               const GeglRectangle      *dest_rect);

gboolean gimp_color_transform_can_gegl_copy   (GimpColorProfile         *src_profile,
                                               GimpColorProfile         *dest_profile);


G_END_DECLS

#endif  /* __GIMP_COLOR_TRANSFORM_H__ */
