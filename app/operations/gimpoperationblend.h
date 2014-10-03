/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationblend.h
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_OPERATION_BLEND_H__
#define __GIMP_OPERATION_BLEND_H__


#include <gegl-plugin.h>


#define GIMP_TYPE_OPERATION_BLEND            (gimp_operation_blend_get_type ())
#define GIMP_OPERATION_BLEND(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPERATION_BLEND, GimpOperationBlend))
#define GIMP_OPERATION_BLEND_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_OPERATION_BLEND, GimpOperationBlendClass))
#define GIMP_IS_OPERATION_BLEND(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OPERATION_BLEND))
#define GIMP_IS_OPERATION_BLEND_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_OPERATION_BLEND))
#define GIMP_OPERATION_BLEND_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_OPERATION_BLEND, GimpOperationBlendClass))


typedef struct _GimpOperationBlend      GimpOperationBlend;
typedef struct _GimpOperationBlendClass GimpOperationBlendClass;

struct _GimpOperationBlend
{
  GeglOperationSource  parent_instance;

  GimpContext         *context;

  GimpGradient        *gradient;
  gdouble              start_x, start_y, end_x, end_y;
  GimpGradientType     gradient_type;
  GimpRepeatMode       gradient_repeat;
  gdouble              offset;
  gboolean             gradient_reverse;

  gboolean             supersample;
  gint                 supersample_depth;
  gdouble              supersample_threshold;

  gboolean             dither;
};

struct _GimpOperationBlendClass
{
  GeglOperationSourceClass  parent_class;
};


GType   gimp_operation_blend_get_type (void) G_GNUC_CONST;


#endif /* __GIMP_OPERATION_BLEND_H__ */
