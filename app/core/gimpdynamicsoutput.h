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

#ifndef __LIGMA_DYNAMICS_OUTPUT_H__
#define __LIGMA_DYNAMICS_OUTPUT_H__


#include "ligmaobject.h"


#define LIGMA_TYPE_DYNAMICS_OUTPUT            (ligma_dynamics_output_get_type ())
#define LIGMA_DYNAMICS_OUTPUT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DYNAMICS_OUTPUT, LigmaDynamicsOutput))
#define LIGMA_DYNAMICS_OUTPUT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DYNAMICS_OUTPUT, LigmaDynamicsOutputClass))
#define LIGMA_IS_DYNAMICS_OUTPUT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DYNAMICS_OUTPUT))
#define LIGMA_IS_DYNAMICS_OUTPUT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DYNAMICS_OUTPUT))
#define LIGMA_DYNAMICS_OUTPUT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DYNAMICS_OUTPUT, LigmaDynamicsOutputClass))


typedef struct _LigmaDynamicsOutputClass LigmaDynamicsOutputClass;

struct _LigmaDynamicsOutput
{
  LigmaObject  parent_instance;
};

struct _LigmaDynamicsOutputClass
{
  LigmaObjectClass  parent_class;
};


GType                ligma_dynamics_output_get_type (void) G_GNUC_CONST;

LigmaDynamicsOutput * ligma_dynamics_output_new      (const gchar            *name,
                                                    LigmaDynamicsOutputType  type);

gboolean   ligma_dynamics_output_is_enabled         (LigmaDynamicsOutput *output);

gdouble    ligma_dynamics_output_get_linear_value   (LigmaDynamicsOutput *output,
                                                    const LigmaCoords   *coords,
                                                    LigmaPaintOptions   *options,
                                                    gdouble             fade_point);

gdouble    ligma_dynamics_output_get_angular_value  (LigmaDynamicsOutput *output,
                                                    const LigmaCoords   *coords,
                                                    LigmaPaintOptions   *options,
                                                    gdouble             fade_point);
gdouble    ligma_dynamics_output_get_aspect_value   (LigmaDynamicsOutput *output,
                                                    const LigmaCoords   *coords,
                                                    LigmaPaintOptions   *options,
                                                    gdouble             fade_point);


#endif  /*  __LIGMA_DYNAMICS_OUTPUT_H__  */
