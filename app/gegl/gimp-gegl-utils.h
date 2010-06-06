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


const Babl  * gimp_bpp_to_babl_format           (guint                  bpp,
                                                 gboolean               linear) G_GNUC_CONST;

TileManager * gimp_buffer_to_tiles              (GeglBuffer            *buffer);

const gchar * gimp_layer_mode_to_gegl_operation (GimpLayerModeEffects   mode) G_GNUC_CONST;
const gchar * gimp_interpolation_to_gegl_filter (GimpInterpolationType  interpolation) G_GNUC_CONST;


#endif /* __GIMP_GEGL_UTILS_H__ */
