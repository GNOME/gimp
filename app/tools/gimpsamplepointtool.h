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

#ifndef __GIMP_SAMPLE_POINT_TOOL_H__
#define __GIMP_SAMPLE_POINT_TOOL_H__


#include "gimpdrawtool.h"


#define GIMP_TYPE_SAMPLE_POINT_TOOL            (gimp_sample_point_tool_get_type ())
#define GIMP_SAMPLE_POINT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SAMPLE_POINT_TOOL, GimpSamplePointTool))
#define GIMP_SAMPLE_POINT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SAMPLE_POINT_TOOL, GimpSamplePointToolClass))
#define GIMP_IS_SAMPLE_POINT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SAMPLE_POINT_TOOL))
#define GIMP_IS_SAMPLE_POINT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SAMPLE_POINT_TOOL))
#define GIMP_SAMPLE_POINT_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SAMPLE_POINT_TOOL, GimpSamplePointToolClass))


typedef struct _GimpSamplePointTool      GimpSamplePointTool;
typedef struct _GimpSamplePointToolClass GimpSamplePointToolClass;

struct _GimpSamplePointTool
{
  GimpDrawTool     parent_instance;

  GimpSamplePoint *sample_point;
  gint             sample_point_old_x;
  gint             sample_point_old_y;
  gint             sample_point_x;
  gint             sample_point_y;
};

struct _GimpSamplePointToolClass
{
  GimpDrawToolClass  parent_class;
};


GType   gimp_sample_point_tool_get_type   (void) G_GNUC_CONST;

void    gimp_sample_point_tool_start_new  (GimpTool        *parent_tool,
                                           GimpDisplay     *display);
void    gimp_sample_point_tool_start_edit (GimpTool        *parent_tool,
                                           GimpDisplay     *display,
                                           GimpSamplePoint *sample_point);


#endif  /*  __GIMP_SAMPLE_POINT_TOOL_H__  */
