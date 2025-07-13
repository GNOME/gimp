/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationpointfilter.h
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

#pragma once

#include <gegl-plugin.h>
#include <operation/gegl-operation-point-filter.h>


enum
{
  GIMP_OPERATION_POINT_FILTER_PROP_0,
  GIMP_OPERATION_POINT_FILTER_PROP_TRC,
  GIMP_OPERATION_POINT_FILTER_PROP_CONFIG
};


#define GIMP_TYPE_OPERATION_POINT_FILTER            (gimp_operation_point_filter_get_type ())
#define GIMP_OPERATION_POINT_FILTER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPERATION_POINT_FILTER, GimpOperationPointFilter))
#define GIMP_OPERATION_POINT_FILTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_OPERATION_POINT_FILTER, GimpOperationPointFilterClass))
#define GIMP_IS_OPERATION_POINT_FILTER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OPERATION_POINT_FILTER))
#define GIMP_IS_OPERATION_POINT_FILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_OPERATION_POINT_FILTER))
#define GIMP_OPERATION_POINT_FILTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_OPERATION_POINT_FILTER, GimpOperationPointFilterClass))


typedef struct _GimpOperationPointFilterClass GimpOperationPointFilterClass;

struct _GimpOperationPointFilter
{
  GeglOperationPointFilter  parent_instance;

  GimpTRCType               trc;
  GObject                  *config;
};

struct _GimpOperationPointFilterClass
{
  GeglOperationPointFilterClass  parent_class;
};


GType   gimp_operation_point_filter_get_type     (void) G_GNUC_CONST;

void    gimp_operation_point_filter_get_property (GObject      *object,
                                                  guint         property_id,
                                                  GValue       *value,
                                                  GParamSpec   *pspec);
void    gimp_operation_point_filter_set_property (GObject      *object,
                                                  guint         property_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec);
