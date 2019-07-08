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

#ifndef __GIMP_SHEAR_TOOL_H__
#define __GIMP_SHEAR_TOOL_H__


#include "gimptransformgridtool.h"


#define GIMP_TYPE_SHEAR_TOOL            (gimp_shear_tool_get_type ())
#define GIMP_SHEAR_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SHEAR_TOOL, GimpShearTool))
#define GIMP_SHEAR_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SHEAR_TOOL, GimpShearToolClass))
#define GIMP_IS_SHEAR_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SHEAR_TOOL))
#define GIMP_IS_SHEAR_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SHEAR_TOOL))
#define GIMP_SHEAR_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SHEAR_TOOL, GimpShearToolClass))


typedef struct _GimpShearTool      GimpShearTool;
typedef struct _GimpShearToolClass GimpShearToolClass;

struct _GimpShearTool
{
  GimpTransformGridTool  parent_instance;

  GtkAdjustment         *x_adj;
  GtkAdjustment         *y_adj;
};

struct _GimpShearToolClass
{
  GimpTransformGridToolClass  parent_class;
};


void    gimp_shear_tool_register (GimpToolRegisterCallback  callback,
                                  gpointer                  data);

GType   gimp_shear_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __GIMP_SHEAR_TOOL_H__  */
