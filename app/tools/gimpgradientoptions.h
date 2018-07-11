/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef  __GIMP_GRADIENT_OPTIONS_H__
#define  __GIMP_GRADIENT_OPTIONS_H__


#include "paint/gimppaintoptions.h"


#define GIMP_TYPE_GRADIENT_OPTIONS            (gimp_gradient_options_get_type ())
#define GIMP_GRADIENT_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_GRADIENT_OPTIONS, GimpGradientOptions))
#define GIMP_GRADIENT_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_GRADIENT_OPTIONS, GimpGradientOptionsClass))
#define GIMP_IS_GRADIENT_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_GRADIENT_OPTIONS))
#define GIMP_IS_GRADIENT_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_GRADIENT_OPTIONS))
#define GIMP_GRADIENT_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_GRADIENT_OPTIONS, GimpGradientOptionsClass))


typedef struct _GimpGradientOptions   GimpGradientOptions;
typedef struct _GimpPaintOptionsClass GimpGradientOptionsClass;

struct _GimpGradientOptions
{
  GimpPaintOptions   paint_options;

  gdouble            offset;
  GimpGradientType   gradient_type;
  GeglDistanceMetric distance_metric;

  gboolean           supersample;
  gint               supersample_depth;
  gdouble            supersample_threshold;

  gboolean           dither;

  gboolean           instant;
  gboolean           modify_active;

  /*  options gui  */
  GtkWidget         *instant_toggle;
  GtkWidget         *modify_active_frame;
  GtkWidget         *modify_active_hint;
};


GType       gimp_gradient_options_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_gradient_options_gui      (GimpToolOptions *tool_options);


#endif  /*  __GIMP_GRADIENT_OPTIONS_H__  */
