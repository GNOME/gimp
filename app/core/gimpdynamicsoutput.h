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

#include "gimpobject.h"


#define GIMP_TYPE_DYNAMICS_OUTPUT (gimp_dynamics_output_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpDynamicsOutput,
                          gimp_dynamics_output,
                          GIMP, DYNAMICS_OUTPUT,
                          GimpObject)


struct _GimpDynamicsOutputClass
{
  GimpObjectClass  parent_class;
};


GimpDynamicsOutput * gimp_dynamics_output_new     (const gchar            *name,
                                                   GimpDynamicsOutputType  type);

gboolean   gimp_dynamics_output_is_enabled        (GimpDynamicsOutput *output);

gdouble    gimp_dynamics_output_get_linear_value  (GimpDynamicsOutput *output,
                                                   const GimpCoords   *coords,
                                                   GimpPaintOptions   *options,
                                                   gdouble             fade_point);

gdouble    gimp_dynamics_output_get_angular_value (GimpDynamicsOutput *output,
                                                   const GimpCoords   *coords,
                                                   GimpPaintOptions   *options,
                                                   gdouble             fade_point);
gdouble    gimp_dynamics_output_get_aspect_value  (GimpDynamicsOutput *output,
                                                   const GimpCoords   *coords,
                                                   GimpPaintOptions   *options,
                                                   gdouble             fade_point);
