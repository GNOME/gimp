/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationthresholdalpha.h
 * Copyright (C) 2012 Michael Natterer <mitch@gimp.org>
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
#include <operation/gegl-operation-point-filter.h>


#define GIMP_TYPE_OPERATION_THRESHOLD_ALPHA            (gimp_operation_threshold_alpha_get_type ())
#define GIMP_OPERATION_THRESHOLD_ALPHA(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPERATION_THRESHOLD_ALPHA, GimpOperationThresholdAlpha))
#define GIMP_OPERATION_THRESHOLD_ALPHA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_OPERATION_THRESHOLD_ALPHA, GimpOperationThresholdAlphaClass))
#define GIMP_IS_OPERATION_THRESHOLD_ALPHA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OPERATION_THRESHOLD_ALPHA))
#define GIMP_IS_OPERATION_THRESHOLD_ALPHA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_OPERATION_THRESHOLD_ALPHA))
#define GIMP_OPERATION_THRESHOLD_ALPHA_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_OPERATION_THRESHOLD_ALPHA, GimpOperationThresholdAlphaClass))


typedef struct _GimpOperationThresholdAlpha      GimpOperationThresholdAlpha;
typedef struct _GimpOperationThresholdAlphaClass GimpOperationThresholdAlphaClass;

struct _GimpOperationThresholdAlpha
{
  GeglOperationPointFilter  parent_instance;

  gdouble                   value;
};

struct _GimpOperationThresholdAlphaClass
{
  GeglOperationPointFilterClass  parent_class;
};


GType   gimp_operation_threshold_alpha_get_type (void) G_GNUC_CONST;
