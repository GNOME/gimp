/* The GIMP -- an image manipulation program
 *
 * This file Copyright (C) 1999 Simon Budig
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

#include "path_bezier.h"

/*
 * This function is to get a set of npoints different coordinates for
 * the range from start to end (each in the range from 0 to 1 and
 * start < end.
 * returns the number of created coords. Make sure that the points-
 * Array is allocated.
 */

guint
path_bezier_get_points (PathTool *path_tool,
                        PathSegment *segment,
			GdkPoint *points,
			guint npoints,
			gdouble start,
			gdouble end)
{
   return 0;
}

void
path_bezier_get_point (PathTool *path_tool,
		       PathSegment *segment,
		       gdouble position,
		       gdouble *x,
		       gdouble *y)
{
   return;
}

void
path_bezier_draw_handles (Tool *tool,
			  PathSegment *segment)
{
   return;
}
			  
void
path_bezier_draw_segment (Tool *tool,
			  PathSegment *segment)
{
   return;
}
			  

gdouble
path_bezier_on_segment (Tool *tool,
			PathSegment *segment,
			gint x,
			gint y,
			gint halfwidth,
			gint *distance)
{
   return -1;
}

void
path_bezier_drag_segment (PathTool *path_tool,
			  PathSegment *segment,
			  gdouble position,
			  gint x,
			  gint y)
{
   return;
}

gboolean
path_bezier_on_handles (PathTool *path_tool,
			PathSegment *segment,
			gint x,
			gint y,
			gint halfwidth)
{
   return FALSE;
}

void
path_bezier_drag_handles (PathTool *path_tool,
			  PathSegment *segment,
			  gint x,
			  gint y)
{
   return;
}

			
PathSegment *
path_bezier_insert_anchor (PathTool *path_tool,
			   PathSegment *segment,
			   gdouble position)
{
   return NULL;
}

void
path_bezier_update_segment (PathTool *path_tool,
			    PathSegment *segment)
{
    return;
}


void
path_bezier_flip_segment (PathTool *path_tool,
			  PathSegment *segment)
{
    return;
}



			
