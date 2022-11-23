/* LIGMA - The GNU Image Manipulation Program
 *
 * ligmanpointdeformationoptions.h
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __LIGMA_N_POINT_DEFORMATION_OPTIONS_H__
#define __LIGMA_N_POINT_DEFORMATION_OPTIONS_H__


#include "core/ligmatooloptions.h"


#define LIGMA_TYPE_N_POINT_DEFORMATION_OPTIONS            (ligma_n_point_deformation_options_get_type ())
#define LIGMA_N_POINT_DEFORMATION_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_N_POINT_DEFORMATION_OPTIONS, LigmaNPointDeformationOptions))
#define LIGMA_N_POINT_DEFORMATION_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_N_POINT_DEFORMATION_OPTIONS, LigmaNPointDeformationOptionsClass))
#define LIGMA_IS_N_POINT_DEFORMATION_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_N_POINT_DEFORMATION_OPTIONS))
#define LIGMA_IS_N_POINT_DEFORMATION_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_N_POINT_DEFORMATION_OPTIONS))
#define LIGMA_N_POINT_DEFORMATION_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_N_POINT_DEFORMATION_OPTIONS, LigmaNPointDeformationOptionsClass))


typedef struct _LigmaNPointDeformationOptions      LigmaNPointDeformationOptions;
typedef struct _LigmaNPointDeformationOptionsClass LigmaNPointDeformationOptionsClass;

struct _LigmaNPointDeformationOptions
{
  LigmaToolOptions  parent_instance;

  gdouble          square_size;
  gdouble          rigidity;
  gboolean         asap_deformation;
  gboolean         mls_weights;
  gdouble          mls_weights_alpha;
  gboolean         mesh_visible;

  GtkWidget       *scale_square_size;
  GtkWidget       *check_mesh_visible;
};

struct _LigmaNPointDeformationOptionsClass
{
  LigmaToolOptionsClass  parent_class;
};


GType       ligma_n_point_deformation_options_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_n_point_deformation_options_gui      (LigmaToolOptions *tool_options);

void        ligma_n_point_deformation_options_set_sensitivity (LigmaNPointDeformationOptions *npd_options,
                                                              gboolean                      tool_active);


#endif  /*  __LIGMA_N_POINT_DEFORMATION_OPTIONS_H__  */
