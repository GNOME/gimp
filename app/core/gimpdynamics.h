/* GIMP - The GNU Image Manipulation Program
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

#pragma once

#include "gimpdata.h"


#define GIMP_TYPE_DYNAMICS (gimp_dynamics_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpDynamics,
                          gimp_dynamics,
                          GIMP, DYNAMICS,
                          GimpData)


struct _GimpDynamicsClass
{
  GimpDataClass  parent_class;
};


GimpData           * gimp_dynamics_new          (GimpContext            *context,
                                                 const gchar            *name);
GimpData           * gimp_dynamics_get_standard (GimpContext            *context);

GimpDynamicsOutput * gimp_dynamics_get_output   (GimpDynamics           *dynamics,
                                                 GimpDynamicsOutputType  type);

gboolean        gimp_dynamics_is_output_enabled (GimpDynamics           *dynamics,
                                                 GimpDynamicsOutputType  type);

gdouble         gimp_dynamics_get_linear_value  (GimpDynamics           *dynamics,
                                                 GimpDynamicsOutputType  type,
                                                 const GimpCoords       *coords,
                                                 GimpPaintOptions       *options,
                                                 gdouble                 fade_point);

gdouble         gimp_dynamics_get_angular_value (GimpDynamics           *dynamics,
                                                 GimpDynamicsOutputType  type,
                                                 const GimpCoords       *coords,
                                                 GimpPaintOptions       *options,
                                                 gdouble                 fade_point);

gdouble         gimp_dynamics_get_aspect_value  (GimpDynamics           *dynamics,
                                                 GimpDynamicsOutputType  type,
                                                 const GimpCoords       *coords,
                                                 GimpPaintOptions       *options,
                                                 gdouble                 fade_point);
