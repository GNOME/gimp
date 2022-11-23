/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligma-gegl-utils.h
 * Copyright (C) 2007 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_GEGL_UTILS_H__
#define __LIGMA_GEGL_UTILS_H__


GType         ligma_gegl_get_op_enum_type              (const gchar         *operation,
                                                       const gchar         *property);

GeglColor   * ligma_gegl_color_new                     (const LigmaRGB       *rgb,
                                                       const Babl          *space);

void          ligma_gegl_progress_connect              (GeglNode            *node,
                                                       LigmaProgress        *progress,
                                                       const gchar         *text);

gboolean      ligma_gegl_node_is_source_operation      (GeglNode            *node);
gboolean      ligma_gegl_node_is_point_operation       (GeglNode            *node);
gboolean      ligma_gegl_node_is_area_filter_operation (GeglNode            *node);

const gchar * ligma_gegl_node_get_key                  (GeglNode            *node,
                                                       const gchar         *key);
gboolean      ligma_gegl_node_has_key                  (GeglNode            *node,
                                                       const gchar         *key);

const Babl  * ligma_gegl_node_get_format               (GeglNode            *node,
                                                       const gchar         *pad_name);

void          ligma_gegl_node_set_underlying_operation (GeglNode           *node,
                                                       GeglNode           *operation);
GeglNode    * ligma_gegl_node_get_underlying_operation (GeglNode           *node);

gboolean      ligma_gegl_param_spec_has_key            (GParamSpec          *pspec,
                                                       const gchar         *key,
                                                       const gchar         *value);

GeglBuffer  * ligma_gegl_buffer_dup                    (GeglBuffer          *buffer);

gboolean      ligma_gegl_buffer_set_extent             (GeglBuffer          *buffer,
                                                       const GeglRectangle *extent);


#endif /* __LIGMA_GEGL_UTILS_H__ */
