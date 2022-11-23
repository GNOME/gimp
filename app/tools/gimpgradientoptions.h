/* LIGMA - The GNU Image Manipulation Program
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

#ifndef  __LIGMA_GRADIENT_OPTIONS_H__
#define  __LIGMA_GRADIENT_OPTIONS_H__


#include "paint/ligmapaintoptions.h"


#define LIGMA_TYPE_GRADIENT_OPTIONS            (ligma_gradient_options_get_type ())
#define LIGMA_GRADIENT_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_GRADIENT_OPTIONS, LigmaGradientOptions))
#define LIGMA_GRADIENT_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_GRADIENT_OPTIONS, LigmaGradientOptionsClass))
#define LIGMA_IS_GRADIENT_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_GRADIENT_OPTIONS))
#define LIGMA_IS_GRADIENT_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_GRADIENT_OPTIONS))
#define LIGMA_GRADIENT_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_GRADIENT_OPTIONS, LigmaGradientOptionsClass))


typedef struct _LigmaGradientOptions   LigmaGradientOptions;
typedef struct _LigmaPaintOptionsClass LigmaGradientOptionsClass;

struct _LigmaGradientOptions
{
  LigmaPaintOptions   paint_options;

  gdouble            offset;
  LigmaGradientType   gradient_type;
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


GType       ligma_gradient_options_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_gradient_options_gui      (LigmaToolOptions *tool_options);


#endif  /*  __LIGMA_GRADIENT_OPTIONS_H__  */
