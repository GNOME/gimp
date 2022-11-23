/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_TRANSFORM_3D_TOOL_H__
#define __LIGMA_TRANSFORM_3D_TOOL_H__


#include "ligmatransformgridtool.h"


#define LIGMA_TYPE_TRANSFORM_3D_TOOL           (ligma_transform_3d_tool_get_type ())
#define LIGMA_TRANSFORM_3D_TOOL(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TRANSFORM_3D_TOOL, LigmaTransform3DTool))
#define LIGMA_TRANSFORM_3D_TOOL_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TRANSFORM_3D_TOOL, LigmaTransform3DToolClass))
#define LIGMA_IS_TRANSFORM_3D_TOOL(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TRANSFORM_3D_TOOL))
#define LIGMA_TRANSFORM_3D_TOOL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TRANSFORM_3D_TOOL, LigmaTransform3DToolClass))

#define LIGMA_TRANSFORM_3D_TOOL_GET_OPTIONS(t) (LIGMA_TRANSFORM_3D_OPTIONS (ligma_tool_get_options (LIGMA_TOOL (t))))


typedef struct _LigmaTransform3DTool      LigmaTransform3DTool;
typedef struct _LigmaTransform3DToolClass LigmaTransform3DToolClass;

struct _LigmaTransform3DTool
{
  LigmaTransformGridTool  parent_instance;

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

struct _LigmaTransform3DToolClass
{
  LigmaTransformGridToolClass parent_class;
};


void    ligma_transform_3d_tool_register (LigmaToolRegisterCallback  callback,
                                         gpointer                  data);

GType   ligma_transform_3d_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __LIGMA_TRANSFORM_3D_TOOL_H__  */
