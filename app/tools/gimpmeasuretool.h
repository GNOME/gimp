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

#pragma once

#include "gimptransformtool.h"


#define GIMP_TYPE_MEASURE_TOOL            (gimp_measure_tool_get_type ())
#define GIMP_MEASURE_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_MEASURE_TOOL, GimpMeasureTool))
#define GIMP_MEASURE_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_MEASURE_TOOL, GimpMeasureToolClass))
#define GIMP_IS_MEASURE_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_MEASURE_TOOL))
#define GIMP_IS_MEASURE_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_MEASURE_TOOL))
#define GIMP_MEASURE_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_MEASURE_TOOL, GimpMeasureToolClass))

#define GIMP_MEASURE_TOOL_GET_OPTIONS(t)  (GIMP_MEASURE_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpMeasureTool      GimpMeasureTool;
typedef struct _GimpMeasureToolClass GimpMeasureToolClass;

struct _GimpMeasureTool
{
  GimpTransformTool  parent_instance;

  GimpToolWidget    *widget;
  GimpToolWidget    *grab_widget;

  gboolean           supress_guides;

  gint               n_points;
  gint               x[3];
  gint               y[3];

  GimpToolGui       *gui;
  GtkWidget         *distance_label[2];
  GtkWidget         *angle_label[2];
  GtkWidget         *width_label[2];
  GtkWidget         *height_label[2];
  GtkWidget         *unit_label[4];
};

struct _GimpMeasureToolClass
{
  GimpTransformToolClass  parent_class;
};


void    gimp_measure_tool_register (GimpToolRegisterCallback  callback,
                                    gpointer                  data);

GType   gimp_measure_tool_get_type (void) G_GNUC_CONST;
