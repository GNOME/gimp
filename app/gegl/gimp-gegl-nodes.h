/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-gegl-nodes.h
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_GEGL_NODES_H__
#define __GIMP_GEGL_NODES_H__


GeglNode * gimp_gegl_create_flatten_node       (const GimpRGB *background);
GeglNode * gimp_gegl_create_apply_opacity_node (GeglBuffer    *mask,
                                                gdouble        opacity,
                                                /* offsets *into* the mask */
                                                gint           mask_offset_x,
                                                gint           mask_offset_y);


#endif /* __GIMP_GEGL_NODES_H__ */
