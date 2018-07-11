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

#ifndef __GIMP_DYNAMICS_OUTPUT_H__
#define __GIMP_DYNAMICS_OUTPUT_H__


#include "gimpobject.h"


#define GIMP_TYPE_DYNAMICS_OUTPUT            (gimp_dynamics_output_get_type ())
#define GIMP_DYNAMICS_OUTPUT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DYNAMICS_OUTPUT, GimpDynamicsOutput))
#define GIMP_DYNAMICS_OUTPUT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DYNAMICS_OUTPUT, GimpDynamicsOutputClass))
#define GIMP_IS_DYNAMICS_OUTPUT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DYNAMICS_OUTPUT))
#define GIMP_IS_DYNAMICS_OUTPUT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DYNAMICS_OUTPUT))
#define GIMP_DYNAMICS_OUTPUT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DYNAMICS_OUTPUT, GimpDynamicsOutputClass))


typedef struct _GimpDynamicsOutputClass GimpDynamicsOutputClass;

struct _GimpDynamicsOutput
{
  GimpObject  parent_instance;
};

struct _GimpDynamicsOutputClass
{
  GimpObjectClass  parent_class;
};


GType                gimp_dynamics_output_get_type (void) G_GNUC_CONST;

GimpDynamicsOutput * gimp_dynamics_output_new      (const gchar            *name,
                                                    GimpDynamicsOutputType  type);

gboolean   gimp_dynamics_output_is_enabled         (GimpDynamicsOutput *output);

gdouble    gimp_dynamics_output_get_linear_value   (GimpDynamicsOutput *output,
                                                    const GimpCoords   *coords,
                                                    GimpPaintOptions   *options,
                                                    gdouble             fade_point);

gdouble    gimp_dynamics_output_get_angular_value  (GimpDynamicsOutput *output,
                                                    const GimpCoords   *coords,
                                                    GimpPaintOptions   *options,
                                                    gdouble             fade_point);
gdouble    gimp_dynamics_output_get_aspect_value   (GimpDynamicsOutput *output,
                                                    const GimpCoords   *coords,
                                                    GimpPaintOptions   *options,
                                                    gdouble             fade_point);


#endif  /*  __GIMP_DYNAMICS_OUTPUT_H__  */
