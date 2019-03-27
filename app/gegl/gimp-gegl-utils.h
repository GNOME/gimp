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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_GEGL_UTILS_H__
#define __GIMP_GEGL_UTILS_H__


GType        gimp_gegl_get_op_enum_type              (const gchar         *operation,
                                                      const gchar         *property);

GeglColor  * gimp_gegl_color_new                     (const GimpRGB       *rgb);

void         gimp_gegl_progress_connect              (GeglNode            *node,
                                                      GimpProgress        *progress,
                                                      const gchar         *text);

gboolean     gimp_gegl_node_is_source_operation      (GeglNode            *node);
gboolean     gimp_gegl_node_is_point_operation       (GeglNode            *node);

const Babl * gimp_gegl_node_get_format               (GeglNode            *node,
                                                      const gchar         *pad_name);

void         gimp_gegl_node_set_underlying_operation (GeglNode           *node,
                                                      GeglNode           *operation);
GeglNode   * gimp_gegl_node_get_underlying_operation (GeglNode           *node);

gboolean     gimp_gegl_param_spec_has_key            (GParamSpec          *pspec,
                                                      const gchar         *key,
                                                      const gchar         *value);

void         gimp_gegl_rectangle_align_to_tile_grid  (GeglRectangle       *dest,
                                                      const GeglRectangle *src,
                                                      GeglBuffer          *buffer);


#endif /* __GIMP_GEGL_UTILS_H__ */
