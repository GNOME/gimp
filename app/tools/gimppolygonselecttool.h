/* GIMP - The GNU Image Manipulation Program
 *
 * A polygonal selection tool for GIMP
 * Copyright (C) 2007 Martin Nordholts
 *
 * Based on gimpfreeselecttool.h which is
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_POLYGON_SELECT_TOOL_H__
#define __GIMP_POLYGON_SELECT_TOOL_H__


#include "gimpselectiontool.h"


#define GIMP_TYPE_POLYGON_SELECT_TOOL            (gimp_polygon_select_tool_get_type ())
#define GIMP_POLYGON_SELECT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_POLYGON_SELECT_TOOL, GimpPolygonSelectTool))
#define GIMP_POLYGON_SELECT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_POLYGON_SELECT_TOOL, GimpPolygonSelectToolClass))
#define GIMP_IS_POLYGON_SELECT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_POLYGON_SELECT_TOOL))
#define GIMP_IS_POLYGON_SELECT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_POLYGON_SELECT_TOOL))
#define GIMP_POLYGON_SELECT_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_POLYGON_SELECT_TOOL, GimpPolygonSelectToolClass))


typedef struct _GimpPolygonSelectTool        GimpPolygonSelectTool;
typedef struct _GimpPolygonSelectToolClass   GimpPolygonSelectToolClass;

struct _GimpPolygonSelectToolClass
{
  GimpSelectionToolClass  parent_class;

  /*  virtual function  */

  void (* select) (GimpPolygonSelectTool *poly_select_tool,
                   GimpDisplay           *display);
};


void    gimp_polygon_select_tool_register (GimpToolRegisterCallback callback,
                                           gpointer                 data);

GType   gimp_polygon_select_tool_get_type (void) G_GNUC_CONST;

void    gimp_polygon_select_tool_select   (GimpPolygonSelectTool *poly_sel,
                                           GimpDisplay           *display);


#endif /* __GIMP_POLYGON_SELECT_TOOL_H__ */
