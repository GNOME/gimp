/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#ifndef __LIGMA_DYNAMICS_H__
#define __LIGMA_DYNAMICS_H__


#include "ligmadata.h"


#define LIGMA_TYPE_DYNAMICS            (ligma_dynamics_get_type ())
#define LIGMA_DYNAMICS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DYNAMICS, LigmaDynamics))
#define LIGMA_DYNAMICS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DYNAMICS, LigmaDynamicsClass))
#define LIGMA_IS_DYNAMICS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DYNAMICS))
#define LIGMA_IS_DYNAMICS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DYNAMICS))
#define LIGMA_DYNAMICS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DYNAMICS, LigmaDynamicsClass))


typedef struct _LigmaDynamicsClass LigmaDynamicsClass;

struct _LigmaDynamics
{
  LigmaData  parent_instance;
};

struct _LigmaDynamicsClass
{
  LigmaDataClass  parent_class;
};


GType                ligma_dynamics_get_type     (void) G_GNUC_CONST;

LigmaData           * ligma_dynamics_new          (LigmaContext            *context,
                                                 const gchar            *name);
LigmaData           * ligma_dynamics_get_standard (LigmaContext            *context);

LigmaDynamicsOutput * ligma_dynamics_get_output   (LigmaDynamics           *dynamics,
                                                 LigmaDynamicsOutputType  type);

gboolean        ligma_dynamics_is_output_enabled (LigmaDynamics           *dynamics,
                                                 LigmaDynamicsOutputType  type);

gdouble         ligma_dynamics_get_linear_value  (LigmaDynamics           *dynamics,
                                                 LigmaDynamicsOutputType  type,
                                                 const LigmaCoords       *coords,
                                                 LigmaPaintOptions       *options,
                                                 gdouble                 fade_point);

gdouble         ligma_dynamics_get_angular_value (LigmaDynamics           *dynamics,
                                                 LigmaDynamicsOutputType  type,
                                                 const LigmaCoords       *coords,
                                                 LigmaPaintOptions       *options,
                                                 gdouble                 fade_point);

gdouble         ligma_dynamics_get_aspect_value  (LigmaDynamics           *dynamics,
                                                 LigmaDynamicsOutputType  type,
                                                 const LigmaCoords       *coords,
                                                 LigmaPaintOptions       *options,
                                                 gdouble                 fade_point);


#endif  /*  __LIGMA_DYNAMICS_H__  */
