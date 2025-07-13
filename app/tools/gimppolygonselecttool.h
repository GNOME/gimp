/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is polygon software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Polygon Software Foundation; either version 3 of the License, or
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

#include "gimpselectiontool.h"


#define GIMP_TYPE_POLYGON_SELECT_TOOL            (gimp_polygon_select_tool_get_type ())
#define GIMP_POLYGON_SELECT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_POLYGON_SELECT_TOOL, GimpPolygonSelectTool))
#define GIMP_POLYGON_SELECT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_POLYGON_SELECT_TOOL, GimpPolygonSelectToolClass))
#define GIMP_IS_POLYGON_SELECT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_POLYGON_SELECT_TOOL))
#define GIMP_IS_POLYGON_SELECT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_POLYGON_SELECT_TOOL))
#define GIMP_POLYGON_SELECT_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_POLYGON_SELECT_TOOL, GimpPolygonSelectToolClass))


typedef struct _GimpPolygonSelectTool        GimpPolygonSelectTool;
typedef struct _GimpPolygonSelectToolPrivate GimpPolygonSelectToolPrivate;
typedef struct _GimpPolygonSelectToolClass   GimpPolygonSelectToolClass;

struct _GimpPolygonSelectTool
{
  GimpSelectionTool             parent_instance;

  GimpPolygonSelectToolPrivate *priv;
};

struct _GimpPolygonSelectToolClass
{
  GimpSelectionToolClass  parent_class;

  /*  virtual functions  */
  void (* change_complete) (GimpPolygonSelectTool *poly_sel,
                            GimpDisplay           *display);
  void (* confirm)         (GimpPolygonSelectTool *poly_sel,
                            GimpDisplay           *display);
};


GType      gimp_polygon_select_tool_get_type   (void) G_GNUC_CONST;

gboolean   gimp_polygon_select_tool_is_closed  (GimpPolygonSelectTool  *poly_sel);
void       gimp_polygon_select_tool_get_points (GimpPolygonSelectTool  *poly_sel,
                                                const GimpVector2     **points,
                                                gint                   *n_points);

/*  protected functions */
gboolean   gimp_polygon_select_tool_is_grabbed (GimpPolygonSelectTool  *poly_sel);

void       gimp_polygon_select_tool_halt       (GimpPolygonSelectTool  *poly_sel);

