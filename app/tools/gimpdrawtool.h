/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others.
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

#ifndef __GIMP_DRAW_TOOL_H__
#define __GIMP_DRAW_TOOL_H__


#include "gimptool.h"


/*  draw states  */
#define INVISIBLE   0
#define VISIBLE     1


#define GIMP_TYPE_DRAW_TOOL            (gimp_draw_tool_get_type ())
#define GIMP_DRAW_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DRAW_TOOL, GimpDrawTool))
#define GIMP_DRAW_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DRAW_TOOL, GimpDrawToolClass))
#define GIMP_IS_DRAW_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DRAW_TOOL))
#define GIMP_IS_DRAW_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DRAW_TOOL))
#define GIMP_DRAW_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DRAW_TOOL, GimpDrawToolClass))


typedef struct _GimpDrawToolClass GimpDrawToolClass;

struct _GimpDrawTool
{
  GimpTool    parent_instance;

  GdkGC      *gc;           /*  Graphics context for draw functions  */
  GdkWindow  *win;          /*  Window to draw draw operation to      */

  gint        draw_state;   /*  Current state in the draw process    */

  gint        line_width;   /**/
  gint        line_style;   /**/
  gint        cap_style;    /*  line attributes                         */
  gint        join_style;   /**/

  gint        paused_count; /*  count to keep track of multiple pauses  */
};

struct _GimpDrawToolClass
{
  GimpToolClass   parent_class;

  void (* draw) (GimpDrawTool *draw_tool);
};


GType   gimp_draw_tool_get_type    (void);

void    gimp_draw_tool_start       (GimpDrawTool *draw_tool,
				    GdkWindow    *window);
void    gimp_draw_tool_stop        (GimpDrawTool *draw_tool);
void    gimp_draw_tool_pause       (GimpDrawTool *draw_tool);
void    gimp_draw_tool_resume      (GimpDrawTool *draw_tool);

void    gimp_draw_tool_draw_handle (GimpDrawTool *draw_tool, 
				    gdouble       x,
				    gdouble       y,
				    gint          size,
				    gint          type);

void   gimp_draw_tool_draw_lines   (GimpDrawTool *draw_tool, 
				    gdouble      *points,
				    gint          npoints,
				    gint          filled);


#endif  /*  __GIMP_DRAW_TOOL_H__  */
