/* The GIMP -- an image manipulation program
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

#ifndef __GIMP_BY_COLOR_SELECT_TOOL_H__
#define __GIMP_BY_COLOR_SELECT_TOOL_H__


#include "gimpselectiontool.h"


#define GIMP_TYPE_BY_COLOR_SELECT_TOOL            (gimp_by_color_select_tool_get_type ())
#define GIMP_BY_COLOR_SELECT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_BY_COLOR_SELECT_TOOL, GimpByColorSelectTool))
#define GIMP_IS_BY_COLOR_SELECT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_BY_COLOR_SELECT_TOOL))
#define GIMP_BY_COLOR_SELECT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_BY_COLOR_SELECT_TOOL, GimpByColorSelectToolClass))
#define GIMP_IS_BY_COLOR_SELECT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_BY_COLOR_SELECT_TOOL))


/*  the by color selection structures  */

typedef struct _GimpByColorSelectTool GimpByColorSelectTool;
typedef struct _GimpByColorSelectToolClass GimpByColorSelectToolClass;

struct _GimpByColorSelectTool
{
  GimpSelectionTool parent_instance;

  gint       x, y;       /*  Point from which to execute seed fill  */
  SelectOps  operation;  /*  add, subtract, normal color selection  */
};


struct _GimpByColorSelectToolClass
{
  GimpSelectionToolClass klass;
};



void    gimp_by_color_select_tool_register            (Gimp         *gimp);

GtkType gimp_by_color_select_tool_get_type            (void);

void    gimp_by_color_select_tool_select              (GimpImage    *gimage,
						       GimpDrawable *drawable,
						       guchar       *color,
						       gint          threshold,
						       SelectOps     op,
						       gboolean      antialias,
						       gboolean      feather,
						       gdouble       feather_radius,
						       gboolean      sample_merged);


#endif  /*  __GIMP_BY_COLOR_SELECT_TOOL_H__  */
