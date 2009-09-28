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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_DYNAMICS_H__
#define __GIMP_DYNAMICS_H__

#include "core/gimpdata.h"
#include "gimpcurve.h"

typedef struct _GimpDynamicsOutput  GimpDynamicsOutput;


struct _GimpDynamicsOutput
{

  gboolean  pressure;
  gboolean  velocity;
  gboolean  direction;
  gboolean  tilt;
  gboolean  random;
  gboolean  fade;

  gdouble   fade_length;

  GimpCurve*  pressure_curve;
  GimpCurve*  velocity_curve;
  GimpCurve*  direction_curve;
  GimpCurve*  tilt_curve;
  GimpCurve*  random_curve;
  GimpCurve*  fade_curve;

};


#define GIMP_PAINT_PRESSURE_SCALE 1.5
#define GIMP_PAINT_VELOCITY_SCALE 1.0


#define GIMP_TYPE_DYNAMICS            (gimp_dynamics_get_type ())
#define GIMP_DYNAMICS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DYNAMICS, GimpDynamics))
#define GIMP_DYNAMICS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DYNAMICS, GimpDynamicsClass))
#define GIMP_IS_DYNAMICS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DYNAMICS))
#define GIMP_IS_DYNAMICS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DYNAMICS))
#define GIMP_DYNAMICS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DYNAMICS, GimpDynamicsClass))


typedef struct _GimpDynamics GimpDynamics;

struct _GimpDynamics
{
  GimpDataClass        parent_instance;

  gchar               *name;

  GimpDynamicsOutput*  opacity_dynamics;
  GimpDynamicsOutput*  hardness_dynamics;
  GimpDynamicsOutput*  rate_dynamics;
  GimpDynamicsOutput*  size_dynamics;
  GimpDynamicsOutput*  aspect_ratio_dynamics;
  GimpDynamicsOutput*  color_dynamics;
  GimpDynamicsOutput*  angle_dynamics;

};

typedef struct _GimpDynamicsClass GimpDynamicsClass;

struct _GimpDynamicsClass
{
  GimpDataClass  parent_instance;
};


GType              gimp_dynamics_get_type (void) G_GNUC_CONST;

GimpData           * gimp_dynamics_new   (const gchar *name);

GimpData           * gimp_dynamics_get_standard     (void);

gdouble            gimp_dynamics_get_output_val (GimpDynamicsOutput *output, GimpCoords coords);

#endif  /*  __GIMP_DYNAMICS_OPTIONS_H__  */
