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

#include "gimptransformgridtool.h"


#define GIMP_TYPE_GENERIC_TRANSFORM_TOOL            (gimp_generic_transform_tool_get_type ())
#define GIMP_GENERIC_TRANSFORM_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_GENERIC_TRANSFORM_TOOL, GimpGenericTransformTool))
#define GIMP_GENERIC_TRANSFORM_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_GENERIC_TRANSFORM_TOOL, GimpGenericTransformToolClass))
#define GIMP_IS_GENERIC_TRANSFORM_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_GENERIC_TRANSFORM_TOOL))
#define GIMP_IS_GENERIC_TRANSFORM_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_GENERIC_TRANSFORM_TOOL))
#define GIMP_GENERIC_TRANSFORM_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_GENERIC_TRANSFORM_TOOL, GimpGenericTransformToolClass))


typedef struct _GimpGenericTransformToolClass GimpGenericTransformToolClass;

struct _GimpGenericTransformTool
{
  GimpTransformGridTool  parent_instance;

  GimpVector2            input_points[4];
  GimpVector2            output_points[4];

  GtkWidget             *matrix_label;
  GtkWidget             *invalid_label;
};

struct _GimpGenericTransformToolClass
{
  GimpTransformGridToolClass  parent_class;

  /*  virtual functions  */
  void   (* info_to_points) (GimpGenericTransformTool *generic);
};


GType   gimp_generic_transform_tool_get_type (void) G_GNUC_CONST;
