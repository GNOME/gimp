/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_POLYGON_SELECT_TOOL_H__
#define __LIGMA_POLYGON_SELECT_TOOL_H__


#include "ligmaselectiontool.h"


#define LIGMA_TYPE_POLYGON_SELECT_TOOL            (ligma_polygon_select_tool_get_type ())
#define LIGMA_POLYGON_SELECT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_POLYGON_SELECT_TOOL, LigmaPolygonSelectTool))
#define LIGMA_POLYGON_SELECT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_POLYGON_SELECT_TOOL, LigmaPolygonSelectToolClass))
#define LIGMA_IS_POLYGON_SELECT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_POLYGON_SELECT_TOOL))
#define LIGMA_IS_POLYGON_SELECT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_POLYGON_SELECT_TOOL))
#define LIGMA_POLYGON_SELECT_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_POLYGON_SELECT_TOOL, LigmaPolygonSelectToolClass))


typedef struct _LigmaPolygonSelectTool        LigmaPolygonSelectTool;
typedef struct _LigmaPolygonSelectToolPrivate LigmaPolygonSelectToolPrivate;
typedef struct _LigmaPolygonSelectToolClass   LigmaPolygonSelectToolClass;

struct _LigmaPolygonSelectTool
{
  LigmaSelectionTool             parent_instance;

  LigmaPolygonSelectToolPrivate *priv;
};

struct _LigmaPolygonSelectToolClass
{
  LigmaSelectionToolClass  parent_class;

  /*  virtual functions  */
  void (* change_complete) (LigmaPolygonSelectTool *poly_sel,
                            LigmaDisplay           *display);
  void (* confirm)         (LigmaPolygonSelectTool *poly_sel,
                            LigmaDisplay           *display);
};


GType      ligma_polygon_select_tool_get_type   (void) G_GNUC_CONST;

gboolean   ligma_polygon_select_tool_is_closed  (LigmaPolygonSelectTool  *poly_sel);
void       ligma_polygon_select_tool_get_points (LigmaPolygonSelectTool  *poly_sel,
                                                const LigmaVector2     **points,
                                                gint                   *n_points);

/*  protected functions */
gboolean   ligma_polygon_select_tool_is_grabbed (LigmaPolygonSelectTool  *poly_sel);

void       ligma_polygon_select_tool_halt       (LigmaPolygonSelectTool  *poly_sel);


#endif  /*  __LIGMA_POLYGON_SELECT_TOOL_H__  */
