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

#ifndef  __GIMP_BLEND_TOOL_H__
#define  __GIMP_BLEND_TOOL_H__


#include "gimpdrawtool.h"

typedef enum
{
  /* POINT_NONE evaluates to FALSE */
  POINT_NONE = 0,
  POINT_START,
  POINT_END,
  POINT_INIT_MODE,
  POINT_FILL_MODE
} GimpBlendToolPoint;

#define GIMP_TYPE_BLEND_TOOL            (gimp_blend_tool_get_type ())
#define GIMP_BLEND_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_BLEND_TOOL, GimpBlendTool))
#define GIMP_BLEND_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_BLEND_TOOL, GimpBlendToolClass))
#define GIMP_IS_BLEND_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_BLEND_TOOL))
#define GIMP_IS_BLEND_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_BLEND_TOOL))
#define GIMP_BLEND_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_BLEND_TOOL, GimpBlendToolClass))

#define GIMP_BLEND_TOOL_GET_OPTIONS(t)  (GIMP_BLEND_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpBlendTool      GimpBlendTool;
typedef struct _GimpBlendToolClass GimpBlendToolClass;

struct _GimpBlendTool
{
  GimpDrawTool    parent_instance;

  GimpBlendToolPoint grabbed_point;

  GimpGradient   *gradient;

  gdouble         start_x;    /*  starting x coord  */
  gdouble         start_y;    /*  starting y coord  */
  gdouble         end_x;      /*  ending x coord    */
  gdouble         end_y;      /*  ending y coord    */

  gdouble         mouse_x;    /*  pointer x coord   */
  gdouble         mouse_y;    /*  pointer y coord   */

  GimpCanvasItem *line;
  GimpCanvasItem *start_handle_circle;
  GimpCanvasItem *start_handle_cross;
  GimpCanvasItem *end_handle_circle;
  GimpCanvasItem *end_handle_cross;

  GeglNode       *graph;
  GeglNode       *render_node;
  GeglNode       *subtract_node;
  GeglNode       *divide_node;
  GeglNode       *dist_node;
  GeglNode       *translate_node;
  GeglBuffer     *dist_buffer;
  gint           dist_buffer_off_x, dist_buffer_off_y;
  GimpImageMap   *image_map;
};

struct _GimpBlendToolClass
{
  GimpDrawToolClass  parent_class;
};


void    gimp_blend_tool_register (GimpToolRegisterCallback  callback,
                                  gpointer                  data);

GType   gimp_blend_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __GIMP_BLEND_TOOL_H__  */
