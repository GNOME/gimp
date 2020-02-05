/* GIMP - The GNU Image Manipulation Program
 *
 * gimpwarptool.h
 * Copyright (C) 2011 Michael Muré <batolettre@gmail.com>
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

#ifndef __GIMP_WARP_TOOL_H__
#define __GIMP_WARP_TOOL_H__


#include "gimpdrawtool.h"


#define GIMP_TYPE_WARP_TOOL            (gimp_warp_tool_get_type ())
#define GIMP_WARP_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_WARP_TOOL, GimpWarpTool))
#define GIMP_WARP_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_WARP_TOOL, GimpWarpToolClass))
#define GIMP_IS_WARP_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_WARP_TOOL))
#define GIMP_IS_WARP_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_WARP_TOOL))
#define GIMP_WARP_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_WARP_TOOL, GimpWarpToolClass))

#define GIMP_WARP_TOOL_GET_OPTIONS(t)  (GIMP_WARP_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpWarpTool      GimpWarpTool;
typedef struct _GimpWarpToolClass GimpWarpToolClass;

struct _GimpWarpTool
{
  GimpDrawTool        parent_instance;

  gboolean            show_cursor;
  gboolean            draw_brush;
  gboolean            snap_brush;

  GimpVector2         cursor_pos;    /* Hold the cursor position */

  GeglBuffer         *coords_buffer; /* Buffer where coordinates are stored */

  GeglNode           *graph;         /* Top level GeglNode */
  GeglNode           *render_node;   /* Node to render the transformation */

  GeglPath           *current_stroke;
  guint               stroke_timer;

  GimpVector2         last_pos;
  gdouble             total_dist;

  GimpDrawableFilter *filter;

  GList              *redo_stack;
};

struct _GimpWarpToolClass
{
  GimpDrawToolClass parent_class;
};


void    gimp_warp_tool_register (GimpToolRegisterCallback  callback,
                                 gpointer                  data);

GType   gimp_warp_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __GIMP_WARP_TOOL_H__  */
