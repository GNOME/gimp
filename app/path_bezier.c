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

#include <math.h>

#include "path_bezier.h"

#define HANDLE_HALFWIDTH 3
#define HANDLE_WIDTH 6

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
		       gdouble pos,
		       gdouble *x,
		       gdouble *y)
{
   PathBezierData *data = (PathBezierData *) segment->data;
   
   if (segment->next) {

      *x = (1-pos) * (1-pos) * (1-pos) * segment->x
	 + 3 * pos * (1-pos) * (1-pos) * (segment->x + data->x1)
	 + 3 * pos * pos * (1-pos) * (segment->next->x + data->x2)
	 + pos * pos * pos * (segment->next->x);

      *y = (1-pos) * (1-pos) * (1-pos) * segment->y
	 + 3 * pos * (1-pos) * (1-pos) * (segment->y + data->y1)
	 + 3 * pos * pos * (1-pos) * (segment->next->y + data->y2)
	 + pos * pos * pos * (segment->next->y);
   }
#ifdef PATH_TOOL_DEBUG
   else fprintf (stderr, "FIXME: path_bezier_get_point called with endpoint-segment!!!\n");
#endif
		     
   return;
}

void
path_bezier_draw_handles (Tool *tool,
			  PathSegment *segment)
{
   PathTool *path_tool = (PathTool *) (tool->private);
   PathBezierData *data = (PathBezierData *) segment->data;
   GDisplay * gdisp = tool->gdisp_ptr;

   gint sx, sy, hx, hy;
   
   if (segment->next) {
      if (segment->flags & SEGMENT_ACTIVE) {
	 gdisplay_transform_coords (gdisp,
	       (gint) (segment->x), (gint) (segment->y), &sx, &sy, FALSE);
	 gdisplay_transform_coords (gdisp,
	       (gint) (segment->x + data->x1),
	       (gint) (segment->y + data->y1), &hx, &hy, FALSE);

	 gdk_draw_line (path_tool->core->win,
	       path_tool->core->gc, hx, hy, sx, sy);

	 gdk_draw_rectangle (path_tool->core->win, path_tool->core->gc, 0,
	       hx - HANDLE_HALFWIDTH, hy - HANDLE_HALFWIDTH,
	       HANDLE_WIDTH, HANDLE_WIDTH);
      }

      if (segment->next->flags & SEGMENT_ACTIVE) { 
	 gdisplay_transform_coords (gdisp,
	       (gint) (segment->next->x), (gint) (segment->next->y), &sx, &sy, FALSE);
	 gdisplay_transform_coords (gdisp,
	       (gint) (segment->next->x + data->x2),
	       (gint) (segment->next->y + data->y2), &hx, &hy, FALSE);

	 gdk_draw_line (path_tool->core->win,
	       path_tool->core->gc, hx, hy, sx, sy);

	 gdk_draw_rectangle (path_tool->core->win, path_tool->core->gc, 0,
	       hx - HANDLE_HALFWIDTH, hy - HANDLE_HALFWIDTH,
	       HANDLE_WIDTH, HANDLE_WIDTH);
      }
   }
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
			  gdouble pos,
			  gdouble dx,
			  gdouble dy)
{
   PathBezierData *data = (PathBezierData *) segment->data;
   gdouble feel_good;

   if (pos <= 0.5)
      feel_good = (pow(2 * pos, 3)) / 2;
   else
      feel_good = (1 - pow((1-pos)*2, 3)) / 2 + 0.5;
   
   data->x1 += (dx / (3*pos*(1-pos)*(1-pos)))*(1-feel_good);
   data->y1 += (dy / (3*pos*(1-pos)*(1-pos)))*(1-feel_good);
   data->x2 += (dx / (3*pos*pos*(1-pos)))*feel_good;
   data->y2 += (dy / (3*pos*pos*(1-pos)))*feel_good;

   return;
}

gint
path_bezier_on_handles (PathTool *path_tool,
			PathSegment *segment,
			gdouble x,
			gdouble y,
			gdouble halfwidth)
{
   PathBezierData *data = (PathBezierData *) segment->data;
       
   if (segment->flags & SEGMENT_ACTIVE &&
       fabs(segment->x + data->x1 - x) <= halfwidth &&
       fabs(segment->y + data->y1 - y) <= halfwidth)
      return 1;

   if (segment->next && segment->next->flags & SEGMENT_ACTIVE &&
       fabs(segment->next->x + data->x2 - x) <= halfwidth &&
       fabs(segment->next->y + data->y2 - y) <= halfwidth)
	 return 2;

   return 0;
}

void
path_bezier_drag_handles (PathTool *path_tool,
			  PathSegment *segment,
			  gdouble dx,
			  gdouble dy,
			  gint handle_id)
{
   PathBezierData *data = (PathBezierData *) segment->data;
   
   if (handle_id == 1) {
      data->x1 += dx;
      data->y1 += dy;
   } else {
      data->x2 += dx;
      data->y2 += dy;
   }
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
path_bezier_flip_segment (PathSegment *segment)
{
   PathBezierData *data = (PathBezierData *) segment->data;
   gdouble swap;
   
   swap = data->x1;
   data->x1 = data->x2;
   data->x2 = swap;
   
   swap = data->y1;
   data->y1 = data->y2;
   data->y2 = swap;

   return;
}


void
path_bezier_init_segment (PathSegment *segment)
{
   PathBezierData *data, *neardata;
   
   data = g_new(PathBezierData, 1);
   data->x1 = 0;
   data->y1 = 0;
   data->x2 = 0;
   data->y2 = 0;
   if (segment->prev && segment->prev->type == SEGMENT_BEZIER) {
      neardata = (PathBezierData *) segment->prev->data;
      data->x1 = - neardata->x2;
      data->y1 = - neardata->y2;
   }
   if (segment->next && segment->next->type == SEGMENT_BEZIER) {
      neardata = (PathBezierData *) segment->next->data;
      data->x2 = - neardata->x1;
      data->y2 = - neardata->y1;
   }

#ifdef PATH_TOOL_DEBUG
   if (segment->data)
      fprintf(stderr, "Warning: path_bezier_init_segment called with already initialized segment\n");
#endif
   segment->data = data;

   return;
}


void
path_bezier_cleanup_segment (PathSegment *segment)
{
   g_free(segment->data);
   segment->data = NULL;

   return;
}


