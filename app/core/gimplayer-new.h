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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once


GimpLayer * gimp_layer_new                  (GimpImage        *image,
                                             gint              width,
                                             gint              height,
                                             const Babl       *format,
                                             const gchar      *name,
                                             gdouble           opacity,
                                             GimpLayerMode     mode);

GimpLayer * gimp_layer_new_from_buffer      (GimpBuffer       *buffer,
                                             GimpImage        *dest_image,
                                             const Babl       *format,
                                             const gchar      *name,
                                             gdouble           opacity,
                                             GimpLayerMode     mode);
GimpLayer * gimp_layer_new_from_gegl_buffer (GeglBuffer       *buffer,
                                             GimpImage        *dest_image,
                                             const Babl       *format,
                                             const gchar      *name,
                                             gdouble           opacity,
                                             GimpLayerMode     mode,
                                             GimpColorProfile *buffer_profile);
GimpLayer * gimp_layer_new_from_pixbuf      (GdkPixbuf        *pixbuf,
                                             GimpImage        *dest_image,
                                             const Babl       *format,
                                             const gchar      *name,
                                             gdouble           opacity,
                                             GimpLayerMode     mode);
