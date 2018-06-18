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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_ROTATE_TOOL_H__
#define __GIMP_ROTATE_TOOL_H__


#include "gimptransformgridtool.h"


#define GIMP_TYPE_ROTATE_TOOL            (gimp_rotate_tool_get_type ())
#define GIMP_ROTATE_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ROTATE_TOOL, GimpRotateTool))
#define GIMP_ROTATE_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ROTATE_TOOL, GimpRotateToolClass))
#define GIMP_IS_ROTATE_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ROTATE_TOOL))
#define GIMP_IS_ROTATE_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ROTATE_TOOL))
#define GIMP_ROTATE_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ROTATE_TOOL, GimpRotateToolClass))


typedef struct _GimpRotateTool      GimpRotateTool;
typedef struct _GimpRotateToolClass GimpRotateToolClass;

struct _GimpRotateTool
{
  GimpTransformGridTool  parent_instance;

  GtkAdjustment         *angle_adj;
  GtkWidget             *angle_spin_button;
  GtkWidget             *sizeentry;
};

struct _GimpRotateToolClass
{
  GimpTransformGridToolClass  parent_class;
};


void    gimp_rotate_tool_register (GimpToolRegisterCallback  callback,
                                   gpointer                  data);

GType   gimp_rotate_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __GIMP_ROTATE_TOOL_H__  */
