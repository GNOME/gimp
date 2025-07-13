/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationgradient.h
 * Copyright (C) 2014 Michael Henning <drawoc@darkrefraction.com>
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

#include <gegl-plugin.h>


#define GIMP_TYPE_OPERATION_GRADIENT            (gimp_operation_gradient_get_type ())
#define GIMP_OPERATION_GRADIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPERATION_GRADIENT, GimpOperationGradient))
#define GIMP_OPERATION_GRADIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_OPERATION_GRADIENT, GimpOperationGradientClass))
#define GIMP_IS_OPERATION_GRADIENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OPERATION_GRADIENT))
#define GIMP_IS_OPERATION_GRADIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_OPERATION_GRADIENT))
#define GIMP_OPERATION_GRADIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_OPERATION_GRADIENT, GimpOperationGradientClass))


typedef struct _GimpOperationGradient      GimpOperationGradient;
typedef struct _GimpOperationGradientClass GimpOperationGradientClass;

struct _GimpOperationGradient
{
  GeglOperationFilter          parent_instance;

  GimpContext                 *context;

  GimpGradient                *gradient;
  gdouble                      start_x, start_y, end_x, end_y;
  GimpGradientType             gradient_type;
  GimpRepeatMode               gradient_repeat;
  gdouble                      offset;
  gboolean                     gradient_reverse;
  GimpGradientBlendColorSpace  gradient_blend_color_space;

  gboolean                     supersample;
  gint                         supersample_depth;
  gdouble                      supersample_threshold;

  gboolean                     dither;

  gdouble                     *gradient_cache;
  gint                         gradient_cache_size;
  GMutex                       gradient_cache_mutex;
};

struct _GimpOperationGradientClass
{
  GeglOperationFilterClass  parent_class;
};


GType   gimp_operation_gradient_get_type (void) G_GNUC_CONST;

