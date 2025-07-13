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

#pragma once

#include "gimpdrawtool.h"


#define GIMP_TYPE_PAINT_SELECT_TOOL            (gimp_paint_select_tool_get_type ())
#define GIMP_PAINT_SELECT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PAINT_SELECT_TOOL, GimpPaintSelectTool))
#define GIMP_PAINT_SELECT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PAINT_SELECT_TOOL, GimpPaintSelectToolClass))
#define GIMP_IS_PAINT_SELECT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PAINT_SELECT_TOOL))
#define GIMP_IS_PAINT_SELECT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PAINT_SELECT_TOOL))
#define GIMP_PAINT_SELECT_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PAINT_SELECT_TOOL, GimpPaintSelectToolClass))

#define GIMP_PAINT_SELECT_TOOL_GET_OPTIONS(t)  (GIMP_PAINT_SELECT_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpPaintSelectTool      GimpPaintSelectTool;
typedef struct _GimpPaintSelectToolClass GimpPaintSelectToolClass;

struct _GimpPaintSelectTool
{
  GimpDrawTool           parent_instance;

  GimpPaintSelectMode    saved_mode;     /*  saved tool options state  */

  GeglBuffer            *trimap;
  GeglBuffer            *image_mask;
  GeglBuffer            *drawable;
  GeglBuffer            *scribble;

  gint                   drawable_off_x;
  gint                   drawable_off_y;
  gint                   drawable_width;
  gint                   drawable_height;

  GeglNode              *graph;
  GeglNode              *ps_node;
  GeglNode              *threshold_node;
  GeglNode              *render_node;

  GimpVector2            last_pos;
};

struct _GimpPaintSelectToolClass
{
  GimpDrawToolClass  parent_class;
};


void    gimp_paint_select_tool_register (GimpToolRegisterCallback  callback,
                                         gpointer                  data);

GType   gimp_paint_select_tool_get_type (void) G_GNUC_CONST;
