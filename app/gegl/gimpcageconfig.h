/* GIMP - The GNU Image Manipulation Program
 * 
 * gimpcageconfig.h
 * Copyright (C) 2010 Michael Muré <batolettre@gmail.com>
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

#ifndef __GIMP_CAGE_CONFIG_H__
#define __GIMP_CAGE_CONFIG_H__

#include "core/gimpimagemapconfig.h"

#define GIMP_TYPE_CAGE_CONFIG            (gimp_cage_config_get_type ())
#define GIMP_CAGE_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CAGE_CONFIG, GimpCageConfig))
#define GIMP_CAGE_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CAGE_CONFIG, GimpCageConfigClass))
#define GIMP_IS_CAGE_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CAGE_CONFIG))
#define GIMP_IS_CAGE_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CAGE_CONFIG))
#define GIMP_CAGE_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CAGE_CONFIG, GimpCageConfigClass))


typedef struct _GimpCageConfigClass GimpCageConfigClass;

struct _GimpCageConfig
{
  GimpImageMapConfig        parent_instance;
  
  gint                      cage_vertice_number; /* number of vertices used by the cage */
  gint                      cage_vertices_max; /* number of vertices allocated in memory */
  
  GimpVector2              *cage_vertices; /* cage before deformation */
  GimpVector2              *cage_vertices_d; /* cage after deformation */
};


struct _GimpCageConfigClass
{
  GimpImageMapConfigClass parent_class;
};

GType         gimp_cage_config_get_type          (void) G_GNUC_CONST;

/**
 * gimp_cage_config_add_cage_point:
 * @gcc: the cage config
 * @x: x value of the new point
 * @y: y value of the new point
 * 
 * Add a new point in the polygon of the cage, and make allocation if needed.
 * Point is added in both source and destination cage
 */
void          gimp_cage_config_add_cage_point         (GimpCageConfig  *gcc,
                                                       gdouble          x,
                                                       gdouble          y);

/**
 * gimp_cage_config_remove_last_cage_point:
 * @gcc: the cage config
 * 
 * Remove the last point of the cage, in both source and destination cage
 */
void          gimp_cage_config_remove_last_cage_point (GimpCageConfig  *gcc);

/**
 * gimp_cage_config_is_on_handle:
 * @gcc: the cage config
 * @mode: the actual mode of the cage, GIMP_CAGE_MODE_CAGE_CHANGE or GIMP_CAGE_MODE_DEFORM
 * @x: x value to check
 * @y: y value to check
 * @handle_size: size of the handle, in pixels
 * 
 * Check if a given point is on a handle of the cage, and return his number if yes.
 * 
 * Returns: the number of the handle if the point is on a handle, or -1 if not.
 */
gint          gimp_cage_config_is_on_handle           (GimpCageConfig  *gcc,
                                                       GimpCageMode     mode,
                                                       gdouble          x,
                                                       gdouble          y,
                                                       gint             handle_size);

/**
 * gimp_cage_config_move_cage_point:
 * @gcc: the cage config
 * @mode: the actual mode of the cage, GIMP_CAGE_MODE_CAGE_CHANGE or GIMP_CAGE_MODE_DEFORM
 * @point_number: the point of the cage to move
 * @x: new x value
 * @y: new y value
 * 
 * Move a point of the source or destination cage, according to the cage mode provided
 */ 
void          gimp_cage_config_move_cage_point        (GimpCageConfig  *gcc,
                                                       GimpCageMode     mode,
                                                       gint             point_number,
                                                       gdouble          x,
                                                       gdouble          y);

/**
 * gimp_cage_config_get_edge_normal:
 * @gcc: the cage config
 * @edge_index: the index of the edge considered
 * 
 * Compute the normal vector to an edge of the cage
 * 
 * Returns: The normal vector to the specified edge of the cage. This vector is normalized (length = 1.0)
 */
GimpVector2   gimp_cage_config_get_edge_normal        (GimpCageConfig  *gcc,
                                                       gint             edge_index);

/**
 * gimp_cage_config_get_bounding_box:
 * @gcc: the cage config
 * 
 * Compute the bounding box of the destination cage
 * 
 * Returns: the bounding box of the destination cage, as a GeglRectangle
 */
GeglRectangle gimp_cage_config_get_bounding_box       (GimpCageConfig  *gcc);

/**
 * gimp_cage_config_reverse_cage_if_needed:
 * @gcc: the cage config
 * 
 * Since the cage need to be defined counter-clockwise to have the topological inside in the actual 'physical' inside of the cage,
 * this function compute if the cage is clockwise or not, and reverse the cage if needed.
 */
void          gimp_cage_config_reverse_cage_if_needed (GimpCageConfig  *gcc);

/**
 * gimp_cage_config_reverse_cage:
 * @gcc: the cage config
 * 
 * When using non-simple cage (like a cage in 8), user may want to manually inverse inside and outside of the cage.
 * This function reverse the cage
 */
void          gimp_cage_config_reverse_cage           (GimpCageConfig *gcc);

#endif /* __GIMP_CAGE_CONFIG_H__ */
