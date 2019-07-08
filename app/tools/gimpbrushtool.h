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

#ifndef __GIMP_BRUSH_TOOL_H__
#define __GIMP_BRUSH_TOOL_H__


#include "gimppainttool.h"


#define GIMP_TYPE_BRUSH_TOOL            (gimp_brush_tool_get_type ())
#define GIMP_BRUSH_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_BRUSH_TOOL, GimpBrushTool))
#define GIMP_BRUSH_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_BRUSH_TOOL, GimpBrushToolClass))
#define GIMP_IS_BRUSH_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_BRUSH_TOOL))
#define GIMP_IS_BRUSH_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_BRUSH_TOOL))
#define GIMP_BRUSH_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_BRUSH_TOOL, GimpBrushToolClass))


typedef struct _GimpBrushToolClass GimpBrushToolClass;

struct _GimpBrushTool
{
  GimpPaintTool   parent_instance;

  GimpBezierDesc *boundary;
  gint            boundary_width;
  gint            boundary_height;
  gdouble         boundary_scale;
  gdouble         boundary_aspect_ratio;
  gdouble         boundary_angle;
  gboolean        boundary_reflect;
  gdouble         boundary_hardness;
};

struct _GimpBrushToolClass
{
  GimpPaintToolClass  parent_class;
};


GType            gimp_brush_tool_get_type       (void) G_GNUC_CONST;

GimpCanvasItem * gimp_brush_tool_create_outline (GimpBrushTool *brush_tool,
                                                 GimpDisplay   *display,
                                                 gdouble        x,
                                                 gdouble        y);


#endif  /*  __GIMP_BRUSH_TOOL_H__  */
