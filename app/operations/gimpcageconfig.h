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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "gimpoperationsettings.h"


struct _GimpCagePoint
{
  GimpVector2 src_point;
  GimpVector2 dest_point;
  GimpVector2 edge_normal;
  gdouble     edge_scaling_factor;
  gboolean    selected;
};


#define GIMP_TYPE_CAGE_CONFIG            (gimp_cage_config_get_type ())
#define GIMP_CAGE_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CAGE_CONFIG, GimpCageConfig))
#define GIMP_CAGE_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CAGE_CONFIG, GimpCageConfigClass))
#define GIMP_IS_CAGE_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CAGE_CONFIG))
#define GIMP_IS_CAGE_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CAGE_CONFIG))
#define GIMP_CAGE_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CAGE_CONFIG, GimpCageConfigClass))


typedef struct _GimpCageConfigClass GimpCageConfigClass;

struct _GimpCageConfig
{
  GimpOperationSettings  parent_instance;

  GArray                *cage_points;

  gdouble                displacement_x;
  gdouble                displacement_y;
  GimpCageMode           cage_mode;  /* Cage mode, used to commit displacement */
};

struct _GimpCageConfigClass
{
  GimpOperationSettingsClass  parent_class;
};


GType           gimp_cage_config_get_type               (void) G_GNUC_CONST;

guint           gimp_cage_config_get_n_points           (GimpCageConfig  *gcc);
void            gimp_cage_config_add_cage_point         (GimpCageConfig  *gcc,
                                                         gdouble          x,
                                                         gdouble          y);
void            gimp_cage_config_insert_cage_point      (GimpCageConfig  *gcc,
                                                         gint             point_number,
                                                         gdouble          x,
                                                         gdouble          y);
void            gimp_cage_config_remove_last_cage_point (GimpCageConfig  *gcc);
void            gimp_cage_config_remove_cage_point      (GimpCageConfig  *gcc,
                                                         gint             point_number);
void            gimp_cage_config_remove_selected_points (GimpCageConfig  *gcc);
GimpVector2     gimp_cage_config_get_point_coordinate   (GimpCageConfig  *gcc,
                                                         GimpCageMode     mode,
                                                         gint             point_number);
void            gimp_cage_config_add_displacement       (GimpCageConfig  *gcc,
                                                         GimpCageMode     mode,
                                                         gdouble          x,
                                                         gdouble          y);
void            gimp_cage_config_commit_displacement    (GimpCageConfig  *gcc);
void            gimp_cage_config_reset_displacement     (GimpCageConfig  *gcc);
GeglRectangle   gimp_cage_config_get_bounding_box       (GimpCageConfig  *gcc);
void            gimp_cage_config_reverse_cage_if_needed (GimpCageConfig  *gcc);
void            gimp_cage_config_reverse_cage           (GimpCageConfig  *gcc);
gboolean        gimp_cage_config_point_inside           (GimpCageConfig  *gcc,
                                                         gfloat           x,
                                                         gfloat           y);
void            gimp_cage_config_select_point           (GimpCageConfig  *gcc,
                                                         gint             point_number);
void            gimp_cage_config_select_area            (GimpCageConfig  *gcc,
                                                         GimpCageMode     mode,
                                                         GeglRectangle    area);
void            gimp_cage_config_select_add_area        (GimpCageConfig  *gcc,
                                                         GimpCageMode     mode,
                                                         GeglRectangle    area);
void            gimp_cage_config_toggle_point_selection (GimpCageConfig  *gcc,
                                                         gint             point_number);
void            gimp_cage_config_deselect_points        (GimpCageConfig  *gcc);
gboolean        gimp_cage_config_point_is_selected      (GimpCageConfig  *gcc,
                                                         gint             point_number);
