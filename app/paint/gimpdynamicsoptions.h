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


#include "core/gimptooloptions.h"
#include "gimppaintoptions.h"
#include "gimpdata.h"


typedef struct _GimpDynamicOptions  GimpDynamicOptions;


struct _GimpDynamicOptions
{
  gboolean  opacity;
  gboolean  hardness;
  gboolean  rate;
  gboolean  size;
  gboolean  inverse_size;
  gboolean  aspect_ratio;
  gboolean  color;
  gboolean  angle;
  gdouble   prescale;
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
  GimpData                  parent_instance;

  GimpPaintInfo            *dynamics_info;
	
  gboolean                  dynamics_expanded;
  GimpDynamicOptions       *pressure_options;
  GimpDynamicOptions       *velocity_options;
  GimpDynamicOptions       *direction_options;
  GimpDynamicOptions       *tilt_options;
  GimpDynamicOptions       *random_options;
  GimpDynamicOptions       *fading_options;
};

struct _GimpDynamicsOptionsClass
{
  GimpPaintOptionsClass  parent_instance;
};


GType              gimp_dynamics_options_get_type (void) G_GNUC_CONST;

GimpData           * gimp_dynamics_options_new   (GimpPaintInfo    *dynamics_info);

gdouble gimp_dynamics_options_get_dynamic_opacity (GimpDynamicsOptions *dynamics_options,
                                                   const GimpCoords       *coords,
                                                   gdouble                 pixel_dist);

gdouble gimp_dynamics_options_get_dynamic_size   (GimpDynamicsOptions  *dynamics_options,
                                                  GimpPaintOptions       *paint_options,
                                                  const GimpCoords       *coords,
                                                  gboolean                use_dynamics,
                                                  gdouble                 pixel_dist);
 
gdouble gimp_dynamics_options_get_dynamic_aspect_ratio 
                                               (GimpDynamicsOptions    *dynamics_options,
                                                GimpPaintOptions       *paint_options,
                                                const GimpCoords       *coords,
                                                gdouble                 pixel_dist)

gdouble gimp_dynamics_options_get_dynamic_rate (GimpDynamicsOptions    *dynamics_options,
                                                const GimpCoords       *coords,
                                                gdouble                 pixel_dist);

gdouble gimp_dynamics_options_get_dynamic_color(GimpDynamicsOptions    *dynamics_options,
                                                const GimpCoords       *coords,
                                                gdouble                 pixel_dist);

gdouble gimp_dynamics_options_get_dynamic_angle (GimpDynamicsOptions   *dynamics_options,
                                                const GimpCoords       *coords,
                                                gdouble                 pixel_dist);

gdouble gimp_dynamics_options_get_dynamic_hardness(GimpDynamicsOptions *dynamics_options,
                                                const GimpCoords       *coords,
                                                gdouble                 pixel_dist);



#endif  /*  __GIMP_DYNAMICS_OPTIONS_H__  */
