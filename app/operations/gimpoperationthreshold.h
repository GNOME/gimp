/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationthreshold.h
 * Copyright (C) 2007 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_OPERATION_THRESHOLD_H__
#define __GIMP_OPERATION_THRESHOLD_H__


#include "gimpoperationpointfilter.h"


#define GIMP_TYPE_OPERATION_THRESHOLD            (gimp_operation_threshold_get_type ())
#define GIMP_OPERATION_THRESHOLD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPERATION_THRESHOLD, GimpOperationThreshold))
#define GIMP_OPERATION_THRESHOLD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_OPERATION_THRESHOLD, GimpOperationThresholdClass))
#define GIMP_IS_OPERATION_THRESHOLD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OPERATION_THRESHOLD))
#define GIMP_IS_OPERATION_THRESHOLD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_OPERATION_THRESHOLD))
#define GIMP_OPERATION_THRESHOLD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_OPERATION_THRESHOLD, GimpOperationThresholdClass))


typedef struct _GimpOperationThreshold      GimpOperationThreshold;
typedef struct _GimpOperationThresholdClass GimpOperationThresholdClass;

struct _GimpOperationThreshold
{
  GimpOperationPointFilter  parent_instance;

  GimpHistogramChannel      channel;
  gdouble                   low;
  gdouble                   high;
};

struct _GimpOperationThresholdClass
{
  GimpOperationPointFilterClass  parent_class;
};


GType   gimp_operation_threshold_get_type (void) G_GNUC_CONST;


#endif /* __GIMP_OPERATION_THRESHOLD_H__ */
