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


GList       * gimp_gegl_get_op_classes                (void);

GType         gimp_gegl_get_op_enum_type              (const gchar         *operation,
                                                       const gchar         *property);

void          gimp_gegl_progress_connect              (GeglNode            *node,
                                                       GimpProgress        *progress,
                                                       const gchar         *text);
void          gimp_gegl_progress_disconnect           (GeglNode            *node,
                                                       GimpProgress        *progress);

gboolean      gimp_gegl_node_is_source_operation      (GeglNode            *node);
gboolean      gimp_gegl_node_is_point_operation       (GeglNode            *node);
gboolean      gimp_gegl_node_is_area_filter_operation (GeglNode            *node);

const gchar * gimp_gegl_node_get_key                  (GeglNode            *node,
                                                       const gchar         *key);
gboolean      gimp_gegl_node_has_key                  (GeglNode            *node,
                                                       const gchar         *key);

const Babl  * gimp_gegl_node_get_format               (GeglNode            *node,
                                                       const gchar         *pad_name);

void          gimp_gegl_node_set_underlying_operation (GeglNode           *node,
                                                       GeglNode           *operation);
GeglNode    * gimp_gegl_node_get_underlying_operation (GeglNode           *node);

gboolean      gimp_gegl_param_spec_has_key            (GParamSpec          *pspec,
                                                       const gchar         *key,
                                                       const gchar         *value);

GeglBuffer  * gimp_gegl_buffer_dup                    (GeglBuffer          *buffer);
GeglBuffer  * gimp_gegl_buffer_resize                 (GeglBuffer          *buffer,
                                                       gint                 new_width,
                                                       gint                 new_height,
                                                       gint                 offset_x,
                                                       gint                 offset_y,
                                                       GeglColor           *color,
                                                       GimpPattern         *pattern,
                                                       gint                 pattern_offset_x,
                                                       gint                 pattern_offset_y);

gboolean      gimp_gegl_buffer_set_extent             (GeglBuffer          *buffer,
                                                       const GeglRectangle *extent);


#endif /* __GIMP_GEGL_UTILS_H__ */
