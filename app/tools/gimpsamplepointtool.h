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

#ifndef __LIGMA_SAMPLE_POINT_TOOL_H__
#define __LIGMA_SAMPLE_POINT_TOOL_H__


#include "ligmadrawtool.h"


#define LIGMA_TYPE_SAMPLE_POINT_TOOL            (ligma_sample_point_tool_get_type ())
#define LIGMA_SAMPLE_POINT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_SAMPLE_POINT_TOOL, LigmaSamplePointTool))
#define LIGMA_SAMPLE_POINT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_SAMPLE_POINT_TOOL, LigmaSamplePointToolClass))
#define LIGMA_IS_SAMPLE_POINT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_SAMPLE_POINT_TOOL))
#define LIGMA_IS_SAMPLE_POINT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_SAMPLE_POINT_TOOL))
#define LIGMA_SAMPLE_POINT_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_SAMPLE_POINT_TOOL, LigmaSamplePointToolClass))


typedef struct _LigmaSamplePointTool      LigmaSamplePointTool;
typedef struct _LigmaSamplePointToolClass LigmaSamplePointToolClass;

struct _LigmaSamplePointTool
{
  LigmaDrawTool     parent_instance;

  LigmaSamplePoint *sample_point;
  gint             sample_point_old_x;
  gint             sample_point_old_y;
  gint             sample_point_x;
  gint             sample_point_y;
};

struct _LigmaSamplePointToolClass
{
  LigmaDrawToolClass  parent_class;
};


GType   ligma_sample_point_tool_get_type   (void) G_GNUC_CONST;

void    ligma_sample_point_tool_start_new  (LigmaTool        *parent_tool,
                                           LigmaDisplay     *display);
void    ligma_sample_point_tool_start_edit (LigmaTool        *parent_tool,
                                           LigmaDisplay     *display,
                                           LigmaSamplePoint *sample_point);


#endif  /*  __LIGMA_SAMPLE_POINT_TOOL_H__  */
