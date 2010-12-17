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
  GimpImageMapConfig  parent_instance;

  gint                n_cage_vertices;   /* vertices used by the cage */
  gint                max_cage_vertices; /* vertices allocated        */

  gint                offset_x;
  gint                offset_y;

  GimpVector2        *cage_vertices;   /* cage before deformation */
  GimpVector2        *cage_vertices_d; /* cage after deformation  */
  gdouble            *scaling_factor;
  GimpVector2        *normal_d;
};

struct _GimpCageConfigClass
{
  GimpImageMapConfigClass  parent_class;
};

GType         gimp_cage_config_get_type               (void) G_GNUC_CONST;

void          gimp_cage_config_add_cage_point         (GimpCageConfig  *gcc,
                                                       gdouble          x,
                                                       gdouble          y);
void          gimp_cage_config_remove_last_cage_point (GimpCageConfig  *gcc);
void          gimp_cage_config_move_cage_point        (GimpCageConfig  *gcc,
                                                       GimpCageMode     mode,
                                                       gint             point_number,
                                                       gdouble          x,
                                                       gdouble          y);
GeglRectangle gimp_cage_config_get_bounding_box       (GimpCageConfig  *gcc);
void          gimp_cage_config_reverse_cage_if_needed (GimpCageConfig  *gcc);
void          gimp_cage_config_reverse_cage           (GimpCageConfig  *gcc);
gboolean      gimp_cage_config_point_inside           (GimpCageConfig  *gcc,
                                                       gfloat           x,
                                                       gfloat           y);
GimpVector2*  gimp_cage_config_get_cage_point         (GimpCageConfig  *gcc,
                                                       GimpCageMode     mode);
void          gimp_cage_config_commit_cage_point      (GimpCageConfig  *gcc,
                                                       GimpCageMode     mode,
                                                       GimpVector2     *points);
#endif /* __GIMP_CAGE_CONFIG_H__ */
