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

#ifndef __GIMP_DYNAMICS_OPTIONS_H__
#define __GIMP_DYNAMICS_OPTIONS_H__


#include "gimppaintoptions.h"
#include "core/gimpdata.h"

typedef struct _GimpDynamicOutputOptions  GimpDynamicOutputOptions;


struct _GimpDynamicOutputOptions
{
  gboolean  pressure;
  gboolean  velocity;
  gboolean  direction;
  gboolean  tilt;
  gboolean  random;
  gboolean  fade;

  gdouble  fade_length;

  GimpCurve*  pressure_curve;
  GimpCurve*  velocity_curve;
  GimpCurve*  direction_curve;
  GimpCurve*  tilt_curve;
  GimpCurve*  random_curve;
  GimpCurve*  fade_curve;

};


#define GIMP_PAINT_PRESSURE_SCALE 1.5
#define GIMP_PAINT_VELOCITY_SCALE 1.0


#define GIMP_TYPE_DYNAMICS_OPTIONS            (gimp_dynamics_options_get_type ())
#define GIMP_DYNAMICS_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DYNAMICS_OPTIONS, GimpDynamicsOptions))
#define GIMP_DYNAMICS_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DYNAMICS_OPTIONS, GimpDynamicsOptionsClass))
#define GIMP_IS_DYNAMICS_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DYNAMICS_OPTIONS))
#define GIMP_IS_DYNAMICS_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DYNAMICS_OPTIONS))
#define GIMP_DYNAMICS_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DYNAMICS_OPTIONS, GimpDynamicsOptionsClass))


typedef struct _GimpDynamicsOptionsClass GimpDynamicsOptionsClass;

struct _GimpDynamicsOptions
{
   GimpDataClass          parent_instance;

  GimpDynamicOutputOptions*  opacity_dynamics;
  GimpDynamicOutputOptions*  hardness_dynamics;
  GimpDynamicOutputOptions*  rate_dynamics;
  GimpDynamicOutputOptions*  size_dynamics;
  GimpDynamicOutputOptions*  aspect_ratio_dynamics;
  GimpDynamicOutputOptions*  color_dynamics;
  GimpDynamicOutputOptions*  angle_dynamics;

};

struct _GimpDynamicsOptionsClass
{
  GimpDataClass  parent_instance;
};


GType              gimp_dynamics_options_get_type (void) G_GNUC_CONST;

GimpData           * gimp_dynamics_options_new   (const gchar *name);

GimpData           * gimp_dynamics_get_standard     (void);

gdouble            gimp_dynamics_options_get_output_val (GimpDynamicOutputOptions *output, GimpCoords *coords);

#endif  /*  __GIMP_DYNAMICS_OPTIONS_H__  */
