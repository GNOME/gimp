/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoperationgradient.h
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

#ifndef __LIGMA_OPERATION_GRADIENT_H__
#define __LIGMA_OPERATION_GRADIENT_H__


#include <gegl-plugin.h>


#define LIGMA_TYPE_OPERATION_GRADIENT            (ligma_operation_gradient_get_type ())
#define LIGMA_OPERATION_GRADIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_OPERATION_GRADIENT, LigmaOperationGradient))
#define LIGMA_OPERATION_GRADIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LIGMA_TYPE_OPERATION_GRADIENT, LigmaOperationGradientClass))
#define LIGMA_IS_OPERATION_GRADIENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_OPERATION_GRADIENT))
#define LIGMA_IS_OPERATION_GRADIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LIGMA_TYPE_OPERATION_GRADIENT))
#define LIGMA_OPERATION_GRADIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LIGMA_TYPE_OPERATION_GRADIENT, LigmaOperationGradientClass))


typedef struct _LigmaOperationGradient      LigmaOperationGradient;
typedef struct _LigmaOperationGradientClass LigmaOperationGradientClass;

struct _LigmaOperationGradient
{
  GeglOperationFilter          parent_instance;

  LigmaContext                 *context;

  LigmaGradient                *gradient;
  gdouble                      start_x, start_y, end_x, end_y;
  LigmaGradientType             gradient_type;
  LigmaRepeatMode               gradient_repeat;
  gdouble                      offset;
  gboolean                     gradient_reverse;
  LigmaGradientBlendColorSpace  gradient_blend_color_space;

  gboolean                     supersample;
  gint                         supersample_depth;
  gdouble                      supersample_threshold;

  gboolean                     dither;

  LigmaRGB                     *gradient_cache;
  gint                         gradient_cache_size;
  GMutex                       gradient_cache_mutex;
};

struct _LigmaOperationGradientClass
{
  GeglOperationFilterClass  parent_class;
};


GType   ligma_operation_gradient_get_type (void) G_GNUC_CONST;


#endif /* __LIGMA_OPERATION_GRADIENT_H__ */
