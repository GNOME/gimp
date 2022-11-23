/* LIGMA - The GNU Image Manipulation Program
 *
 * ligmacageconfig.h
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

#ifndef __LIGMA_CAGE_CONFIG_H__
#define __LIGMA_CAGE_CONFIG_H__


#include "ligmaoperationsettings.h"


struct _LigmaCagePoint
{
  LigmaVector2 src_point;
  LigmaVector2 dest_point;
  LigmaVector2 edge_normal;
  gdouble     edge_scaling_factor;
  gboolean    selected;
};


#define LIGMA_TYPE_CAGE_CONFIG            (ligma_cage_config_get_type ())
#define LIGMA_CAGE_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CAGE_CONFIG, LigmaCageConfig))
#define LIGMA_CAGE_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CAGE_CONFIG, LigmaCageConfigClass))
#define LIGMA_IS_CAGE_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CAGE_CONFIG))
#define LIGMA_IS_CAGE_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CAGE_CONFIG))
#define LIGMA_CAGE_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CAGE_CONFIG, LigmaCageConfigClass))


typedef struct _LigmaCageConfigClass LigmaCageConfigClass;

struct _LigmaCageConfig
{
  LigmaOperationSettings  parent_instance;

  GArray                *cage_points;

  gdouble                displacement_x;
  gdouble                displacement_y;
  LigmaCageMode           cage_mode;  /* Cage mode, used to commit displacement */
};

struct _LigmaCageConfigClass
{
  LigmaOperationSettingsClass  parent_class;
};


GType           ligma_cage_config_get_type               (void) G_GNUC_CONST;

guint           ligma_cage_config_get_n_points           (LigmaCageConfig  *gcc);
void            ligma_cage_config_add_cage_point         (LigmaCageConfig  *gcc,
                                                         gdouble          x,
                                                         gdouble          y);
void            ligma_cage_config_insert_cage_point      (LigmaCageConfig  *gcc,
                                                         gint             point_number,
                                                         gdouble          x,
                                                         gdouble          y);
void            ligma_cage_config_remove_last_cage_point (LigmaCageConfig  *gcc);
void            ligma_cage_config_remove_cage_point      (LigmaCageConfig  *gcc,
                                                         gint             point_number);
void            ligma_cage_config_remove_selected_points (LigmaCageConfig  *gcc);
LigmaVector2     ligma_cage_config_get_point_coordinate   (LigmaCageConfig  *gcc,
                                                         LigmaCageMode     mode,
                                                         gint             point_number);
void            ligma_cage_config_add_displacement       (LigmaCageConfig  *gcc,
                                                         LigmaCageMode     mode,
                                                         gdouble          x,
                                                         gdouble          y);
void            ligma_cage_config_commit_displacement    (LigmaCageConfig  *gcc);
void            ligma_cage_config_reset_displacement     (LigmaCageConfig  *gcc);
GeglRectangle   ligma_cage_config_get_bounding_box       (LigmaCageConfig  *gcc);
void            ligma_cage_config_reverse_cage_if_needed (LigmaCageConfig  *gcc);
void            ligma_cage_config_reverse_cage           (LigmaCageConfig  *gcc);
gboolean        ligma_cage_config_point_inside           (LigmaCageConfig  *gcc,
                                                         gfloat           x,
                                                         gfloat           y);
void            ligma_cage_config_select_point           (LigmaCageConfig  *gcc,
                                                         gint             point_number);
void            ligma_cage_config_select_area            (LigmaCageConfig  *gcc,
                                                         LigmaCageMode     mode,
                                                         GeglRectangle    area);
void            ligma_cage_config_select_add_area        (LigmaCageConfig  *gcc,
                                                         LigmaCageMode     mode,
                                                         GeglRectangle    area);
void            ligma_cage_config_toggle_point_selection (LigmaCageConfig  *gcc,
                                                         gint             point_number);
void            ligma_cage_config_deselect_points        (LigmaCageConfig  *gcc);
gboolean        ligma_cage_config_point_is_selected      (LigmaCageConfig  *gcc,
                                                         gint             point_number);


#endif /* __LIGMA_CAGE_CONFIG_H__ */
