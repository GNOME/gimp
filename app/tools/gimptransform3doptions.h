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

#include "gimptransformgridoptions.h"


#define GIMP_TYPE_TRANSFORM_3D_OPTIONS            (gimp_transform_3d_options_get_type ())
#define GIMP_TRANSFORM_3D_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TRANSFORM_3D_OPTIONS, GimpTransform3DOptions))
#define GIMP_TRANSFORM_3D_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TRANSFORM_3D_OPTIONS, GimpTransform3DOptionsClass))
#define GIMP_IS_TRANSFORM_3D_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TRANSFORM_3D_OPTIONS))
#define GIMP_IS_TRANSFORM_3D_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TRANSFORM_3D_OPTIONS))
#define GIMP_TRANSFORM_3D_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TRANSFORM_3D_OPTIONS, GimpTransform3DOptionsClass))


typedef struct _GimpTransform3DOptions      GimpTransform3DOptions;
typedef struct _GimpTransform3DOptionsClass GimpTransform3DOptionsClass;

struct _GimpTransform3DOptions
{
  GimpTransformGridOptions  parent_instance;

  GimpTransform3DMode       mode;
  gboolean                  unified;

  gboolean                  constrain_axis;
  gboolean                  z_axis;
  gboolean                  local_frame;
};

struct _GimpTransform3DOptionsClass
{
  GimpTransformGridOptionsClass  parent_class;
};


GType       gimp_transform_3d_options_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_transform_3d_options_gui      (GimpToolOptions *tool_options);
