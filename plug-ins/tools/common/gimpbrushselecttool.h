/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
 *
 * Brush selection tool by Nathan Summers
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

#ifndef __GIMP_BRUSH_SELECT_TOOL_H__
#define __GIMP_BRUSH_SELECT_TOOL_H__


#include "gimpselectiontool.h"


/* GTypeModules use a static variable instead of a function call
to get the GType.  See http://www.gtk.org/~otaylor/gtk/2.0/theme-engines.html
for the rationale. */

extern GType gimp_type_brush_select_tool;


#define GIMP_TYPE_BRUSH_SELECT_TOOL            gimp_type_brush_select_tool
#define GIMP_BRUSH_SELECT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_BRUSH_SELECT_TOOL, GimpBrushSelectTool))
#define GIMP_BRUSH_SELECT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_BRUSH_SELECT_TOOL, GimpBrushSelectToolClass))
#define GIMP_IS_BRUSH_SELECT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_BRUSH_SELECT_TOOL))
#define GIMP_IS_BRUSH_SELECT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_BRUSH_SELECT_TOOL))
#define GIMP_BRUSH_SELECT_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_BRUSH_SELECT_TOOL, GimpBrushSelectToolClass))


typedef struct _GimpBrushSelectTool      GimpBrushSelectTool;
typedef struct _GimpBrushSelectToolClass GimpBrushSelectToolClass;

struct _GimpBrushSelectTool
{
  GimpSelectionTool  parent_instance;

  GimpVector2       *points;
  gint               num_points;
  gint               max_segs;
};

struct _GimpBrushSelectToolClass
{
  GimpSelectionToolClass  parent_class;
};


void    gimp_brush_select_tool_register (Gimp                    *gimp,
                                        GimpToolRegisterCallback  callback);

GType   gimp_brush_select_tool_get_type (GTypeModule		 *module);


#endif  /*  __GIMP_BRUSH_SELECT_TOOL_H__  */
