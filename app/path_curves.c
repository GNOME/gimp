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
#include "path_curves.h"
#include "path_bezier.h"

#ifdef PATH_TOOL_DEBUG
#include <stdio.h>
#endif

/* only here temporarily */
PathSegment * path_split_segment   (PathSegment *, gdouble);

/* Here the different curves register their functions */

static CurveDescription CurveTypes[] =
{ 
    /* SEGMENT_LINE */
   {
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
    },

    /* SEGMENT_BEZIER */
   {
      path_bezier_get_points,
      path_bezier_get_point,
      path_bezier_draw_handles,
      path_bezier_draw_segment,
      path_bezier_on_segment,
      path_bezier_drag_segment,
      path_bezier_on_handles,
      path_bezier_drag_handles,
      path_bezier_insert_anchor,
      path_bezier_update_segment,
      path_bezier_flip_segment
    }
};


/*
 * these functions implement the dispatching for the different
 * curve-types. It implements default actions, which happen
 * to work with straight lines.
 */


guint
path_curve_get_points (PathTool *path_tool,
                       PathSegment *segment,
		       GdkPoint *points,
		       guint npoints,
		       gdouble start,
		       gdouble end)
{
   gdouble pos, x, y;
   gint index=0;

   if (segment && CurveTypes[segment->type].get_points)
      return (* CurveTypes[segment->type].get_points) (path_tool, segment, points, npoints, start, end);
   else {
      if (npoints > 1 && segment && segment->next) {
	 for (pos = start; pos <= end; pos += (end - start) / (npoints -1)) {
	    path_curve_get_point (path_tool, segment, pos, &x, &y);
	    points[index].x = (guint) (x + 0.5);
	    points[index].y = (guint) (y + 0.5);
	    index++;
	 }
	 return index;
      } else
         return 0;
   }
}


void
path_curve_get_point (PathTool *path_tool,
		       		  PathSegment *segment,
		       		  gdouble position,
		       		  gdouble *x,
		       		  gdouble *y)
{
   if (segment && CurveTypes[segment->type].get_point)
      (* CurveTypes[segment->type].get_point) (path_tool, segment, position, x, y);
   else {
      if (segment && segment->next) {
#if 0
         *x = segment->x + (segment->next->x - segment->x) * position;
         *y = segment->y + (segment->next->y - segment->y) * position;
#else
	/* Only here for debugging purposes: A bezier curve fith fixed tangents */
	 *x = (1-position)*(1-position)*(1-position) * segment->x +
		3 * position *(1-position)*(1-position) * (segment->x - 60) +
		3 * position * position *(1-position) * (segment->next->x + 60) +
		position * position * position * (segment->next->x);
	 *y = (1-position)*(1-position)*(1-position) * segment->y +
		3 * position *(1-position)*(1-position) * (segment->y + 60) +
		3 * position * position *(1-position) * (segment->next->y + 60) +
		position * position * position * (segment->next->y);
#endif
      }
#ifdef PATH_TOOL_DEBUG
      else
	 fprintf (stderr, "path_curve_get_point called without valid curve");
#endif
   }
   return;
}

void
path_curve_draw_handles (Tool *tool,
			 PathSegment *segment)
{
   if (segment && CurveTypes[segment->type].draw_handles)
      (* CurveTypes[segment->type].draw_handles) (tool, segment);

   /* straight lines do not have handles... */
   return;
}
			  
void
path_curve_draw_segment (Tool *tool,
			 PathSegment *segment)
{
   gint x, y, numpts, index;

   if (segment && segment->next) {
      if (CurveTypes[segment->type].draw_segment) {
         (* CurveTypes[segment->type].draw_segment) (tool, segment);
         return;
      } else {
	 GdkPoint *coordinates = g_new (GdkPoint, 100);
         numpts = path_curve_get_points (((PathTool *) tool->private), segment,
				         coordinates, 100, 0, 1);
	 for (index=0; index < numpts; index++) {
	    gdisplay_transform_coords (tool->gdisp_ptr,
		                       coordinates[index].x,
				       coordinates[index].y,
				       &x, &y, FALSE);
	    coordinates[index].x = x;
	    coordinates[index].y = y;
	 }
	 gdk_draw_lines (((PathTool *) tool->private)->core->win,
	                 ((PathTool *) tool->private)->core->gc,
			 coordinates, numpts);
	 g_free (coordinates);
      }

#ifdef PATH_TOOL_DEBUG
   } else {
      fprintf (stderr, "Fatal Error: path_curve_draw_segment called without valid segment *\n");
#endif
      
   }

   return;
}
			  

gdouble
path_curve_on_segment (Tool *tool,
		       PathSegment *segment,
		       gint x,
		       gint y,
		       gint halfwidth,
		       gint *distance)
{

   if (segment && CurveTypes[segment->type].on_segment)
      return (* CurveTypes[segment->type].on_segment) (tool, segment, x, y, halfwidth, distance);
   else {
      if (segment && segment->next) {
#if 1
	 gint x1, y1, numpts, index;
	 GdkPoint *coordinates = g_new (GdkPoint, 100);
	 gint bestindex = -1;

	 *distance = halfwidth * halfwidth + 1;

         numpts = path_curve_get_points (((PathTool *) tool->private), segment,
				         coordinates, 100, 0, 1);
	 for (index=0; index < numpts; index++) {
	    x1 = coordinates[index].x;
	    y1 = coordinates[index].y;
	    if (((x - x1) * (x - x1) + (y - y1) * (y - y1)) < *distance) {
	       *distance = (x - x1) * (x - x1) + (y - y1) * (y - y1);
	       bestindex = index;
	    }
	 }
	 g_free (coordinates);
	 if (numpts == 1) {
	    *distance = (gint) sqrt ((gdouble) *distance);
	    return bestindex;
	 }
	 if (bestindex >= 0 && (*distance <= halfwidth * halfwidth)) {
	    *distance = (gint) sqrt ((gdouble) *distance);
	    return bestindex == 0 ? 0 : ((gdouble) bestindex) / (numpts - 1);
	 }

#else

         /* Special case for lines */
         gdouble Ax, Ay, Bx, By, r, d;
         Ax = segment->x;
         Ay = segment->y;
         Bx = segment->next->x;
         By = segment->next->y;

         r = (( x - Ax)*(Bx - Ax) + ( y - Ay)* (By - Ay)) /
             ((Bx - Ax)*(Bx - Ax) + (By - Ay)* (By - Ay)) ;

         if (r >= 0 && r <= 1) {
            d = (((Ax + (Bx - Ax) * r) - x) * ((Ax + (Bx - Ax) * r) - x) +
	         ((Ay + (By - Ay) * r) - y) * ((Ay + (By - Ay) * r) - y));
	    if (d <= halfwidth * halfwidth) {
	       *distance = (gint) (d + 0.5);
               return r;
	    }
	 }
#endif
      }
   }
      
   return -1;
}

void
path_curve_drag_segment (PathTool *path_tool,
				       PathSegment *segment,
				       gdouble position,
				       gint x,
				       gint y
				       )
{
   if (segment && CurveTypes[segment->type].drag_segment)
      (* CurveTypes[segment->type].drag_segment) (path_tool, segment, position, x, y);
   return;
}

gboolean
path_curve_on_handle (PathTool *path_tool,
				      PathSegment *segment,
				      gint x,
				      gint y,
				      gint halfwidth)
{
   if (segment && CurveTypes[segment->type].on_handles)
      return (* CurveTypes[segment->type].on_handles) (path_tool, segment, x, y, halfwidth);
   return FALSE;
}

void
path_curve_drag_handle (PathTool *path_tool,
				      PathSegment *segment,
				      gint x,
				      gint y
				      )
{
   if (segment && CurveTypes[segment->type].drag_handle)
      (* CurveTypes[segment->type].drag_handle) (path_tool, segment, x, y);
}

PathSegment *
path_curve_insert_anchor (PathTool *path_tool,
			   	      PathSegment *segment,
			   	      gdouble position)
{
   if (segment && CurveTypes[segment->type].insert_anchor)
      return (* CurveTypes[segment->type].insert_anchor) (path_tool, segment, position);
   else {
      return path_split_segment (segment, position);
   }
}

void
path_curve_flip_segment (PathTool *path_tool,
			    	       PathSegment *segment)
{
   if (segment && CurveTypes[segment->type].flip_segment)
      (* CurveTypes[segment->type].flip_segment) (path_tool, segment);
   return;
}

void
path_curve_update_segment (PathTool *path_tool,
			    	       PathSegment *segment)
{
   if (segment && CurveTypes[segment->type].update_segment)
      (* CurveTypes[segment->type].update_segment) (path_tool, segment);
   return;
}
