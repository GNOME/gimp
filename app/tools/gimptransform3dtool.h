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

#ifndef __GIMP_TRANSFORM_3D_TOOL_H__
#define __GIMP_TRANSFORM_3D_TOOL_H__


#include "gimptransformgridtool.h"


#define GIMP_TYPE_TRANSFORM_3D_TOOL           (gimp_transform_3d_tool_get_type ())
#define GIMP_TRANSFORM_3D_TOOL(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TRANSFORM_3D_TOOL, GimpTransform3DTool))
#define GIMP_TRANSFORM_3D_TOOL_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TRANSFORM_3D_TOOL, GimpTransform3DToolClass))
#define GIMP_IS_TRANSFORM_3D_TOOL(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TRANSFORM_3D_TOOL))
#define GIMP_TRANSFORM_3D_TOOL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TRANSFORM_3D_TOOL, GimpTransform3DToolClass))

#define GIMP_TRANSFORM_3D_TOOL_GET_OPTIONS(t) (GIMP_TRANSFORM_3D_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpTransform3DTool      GimpTransform3DTool;
typedef struct _GimpTransform3DToolClass GimpTransform3DToolClass;

struct _GimpTransform3DTool
{
  GimpTransformGridTool  parent_instance;

  gboolean               updating;

  GtkWidget             *notebook;
  GtkWidget             *vanishing_point_se;
  GtkWidget             *lens_mode_combo;
  GtkWidget             *focal_length_se;
  GtkWidget             *angle_of_view_scale;
  GtkAdjustment         *angle_of_view_adj;
  GtkWidget             *offset_se;
  GtkWidget             *rotation_order_buttons[3];
  GtkAdjustment         *angle_adj[3];
  GtkWidget             *pivot_selector;
};

struct _GimpTransform3DToolClass
{
  GimpTransformGridToolClass parent_class;
};


void    gimp_transform_3d_tool_register (GimpToolRegisterCallback  callback,
                                         gpointer                  data);

GType   gimp_transform_3d_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __GIMP_TRANSFORM_3D_TOOL_H__  */
