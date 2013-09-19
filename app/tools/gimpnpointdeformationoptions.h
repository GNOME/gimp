/* GIMP - The GNU Image Manipulation Program
 *
 * gimpnpointdeformationoptions.h
 * Copyright (C) 2013 Marek Dvoroznak <dvoromar@gmail.com>
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

#ifndef __GIMP_N_POINT_DEFORMATION_OPTIONS_H__
#define __GIMP_N_POINT_DEFORMATION_OPTIONS_H__


#include "core/gimptooloptions.h"


#define GIMP_TYPE_N_POINT_DEFORMATION_OPTIONS            (gimp_n_point_deformation_options_get_type ())
#define GIMP_N_POINT_DEFORMATION_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_N_POINT_DEFORMATION_OPTIONS, GimpNPointDeformationOptions))
#define GIMP_N_POINT_DEFORMATION_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_N_POINT_DEFORMATION_OPTIONS, GimpNPointDeformationOptionsClass))
#define GIMP_IS_N_POINT_DEFORMATION_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_N_POINT_DEFORMATION_OPTIONS))
#define GIMP_IS_N_POINT_DEFORMATION_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_N_POINT_DEFORMATION_OPTIONS))
#define GIMP_N_POINT_DEFORMATION_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_N_POINT_DEFORMATION_OPTIONS, GimpNPointDeformationOptionsClass))


typedef struct _GimpNPointDeformationOptions      GimpNPointDeformationOptions;
typedef struct _GimpNPointDeformationOptionsClass GimpNPointDeformationOptionsClass;

struct _GimpNPointDeformationOptions
{
  GimpToolOptions  parent_instance;

  gdouble          square_size;
  gdouble          rigidity;
  gboolean         ASAP_deformation;
  gboolean         MLS_weights;
  gdouble          MLS_weights_alpha;
  gboolean         mesh_visible;
  gboolean         deformation_is_paused;

  GtkWidget       *scale_square_size;
  GtkWidget       *scale_MLS_weights_alpha;
  GtkWidget       *check_mesh_visible;
  GtkWidget       *check_pause_deformation;
};

struct _GimpNPointDeformationOptionsClass
{
  GimpToolOptionsClass  parent_class;
};


GType       gimp_n_point_deformation_options_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_n_point_deformation_options_gui      (GimpToolOptions *tool_options);

gboolean    gimp_n_point_deformation_options_is_deformation_paused    (GimpNPointDeformationOptions *npd_options);
void        gimp_n_point_deformation_options_set_pause_deformation    (GimpNPointDeformationOptions *npd_options,
                                                                       gboolean                      is_active);
void        gimp_n_point_deformation_options_toggle_pause_deformation (GimpNPointDeformationOptions *npd_options);
void        gimp_n_point_deformation_options_init_some_widgets        (GimpNPointDeformationOptions *npd_options,
                                                                       gboolean                      is_tool_active);

#endif  /*  __GIMP_N_POINT_DEFORMATION_OPTIONS_H__  */
