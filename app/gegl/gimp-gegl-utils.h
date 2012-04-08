/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-gegl-utils.h
 * Copyright (C) 2007 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_GEGL_UTILS_H__
#define __GIMP_GEGL_UTILS_H__


#include <gdk-pixbuf/gdk-pixbuf.h> /* temp hack */


GimpImageType     gimp_babl_format_get_image_type (const Babl           *format);
GimpImageBaseType gimp_babl_format_get_base_type  (const Babl           *format);

const Babl  * gimp_bpp_to_babl_format            (guint                  bpp) G_GNUC_CONST;
const Babl  * gimp_bpp_to_babl_format_with_alpha (guint                  bpp) G_GNUC_CONST;

const gchar * gimp_interpolation_to_gegl_filter  (GimpInterpolationType  interpolation) G_GNUC_CONST;

GeglBuffer  * gimp_gegl_buffer_new               (const GeglRectangle   *rect,
                                                  const Babl            *format);
GeglBuffer  * gimp_gegl_buffer_dup               (GeglBuffer            *buffer);

GeglBuffer  * gimp_tile_manager_create_buffer    (TileManager           *tm,
                                                  const Babl            *format);
TileManager * gimp_gegl_buffer_get_tiles         (GeglBuffer            *buffer);

GimpTempBuf * gimp_gegl_buffer_get_temp_buf      (GeglBuffer            *buffer);

GeglBuffer  * gimp_pixbuf_create_buffer          (GdkPixbuf             *pixbuf);

void          gimp_gegl_buffer_refetch_tiles     (GeglBuffer            *buffer);

GeglColor   * gimp_gegl_color_new                (const GimpRGB         *rgb);

void          gimp_gegl_progress_connect         (GeglNode              *node,
                                                  GimpProgress          *progress,
                                                  const gchar           *text);

#endif /* __GIMP_GEGL_UTILS_H__ */
