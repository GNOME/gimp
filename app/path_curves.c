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

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "core/core-types.h"
#include "tools/tools-types.h"

#include "path_curves.h"
#include "path_bezier.h"

#include "tools/gimpdrawtool.h"


/* only here temporarily */
PathSegment * path_split_segment   (PathSegment *, gdouble);

/* Here the different curves register their functions */

static CurveDescription CurveTypes[] =
{ 
    /* SEGMENT_LINE */
   {
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
    },

    /* SEGMENT_BEZIER */
   {
      NULL, /* path_bezier_get_points, */
      path_bezier_get_point,
      NULL, /* path_bezier_draw_handles, */
      NULL, /* path_bezier_draw_segment, */
      NULL, /* path_bezier_on_segment, */
      path_bezier_drag_segment,
      path_bezier_on_handles,
      path_bezier_drag_handles, 
      NULL, /* path_bezier_insert_anchor, */
      NULL, /* path_bezier_update_segment, */
      path_bezier_flip_segment,
      path_bezier_init_segment,
      NULL /* path_bezier_cleanup_segment */
    }
};


/*
 * these functions implement the dispatching for the different
 * curve-types. It implements default actions, which happen
 * to work with straight lines.
 */


guint
path_curve_get_points (PathSegment *segment,
		       gdouble *points,
		       guint npoints,
		       gdouble start,
		       gdouble end)
{
   gdouble pos;

   gint index=0;

   if (segment && segment->next) {
      if (CurveTypes[segment->type].get_points)
	 return (* CurveTypes[segment->type].get_points) (segment, points, npoints, start, end);
      else {

	 if (npoints > 1 && segment && segment->next) {
   	    for (pos = start; pos <= end; pos += (end - start) / (npoints -1)) {
   	       path_curve_get_point (segment, pos, &points[index*2],
			             &points[index*2+1]);
   	       index++;
   	    }
   	    return index;
     	 } else
       	    return 0;
      }
   }
#ifdef PATH_TOOL_DEBUG
   else
      fprintf (stderr, "path_curve_get_points called without valid curve");
#endif
   return 0;
}


void
path_curve_get_point (PathSegment *segment,
		      gdouble position,
		      gdouble *x,
		      gdouble *y)
{
   if (segment && segment->next) {
      if (CurveTypes[segment->type].get_point)
	 (* CurveTypes[segment->type].get_point) (segment, position, x, y);
      else {
#if 1
	 *x = segment->x + (segment->next->x - segment->x) * position;
         *y = segment->y + (segment->next->y - segment->y) * position;
#else
	 /* Only here for debugging purposes: A bezier curve with fixed tangents */

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
   }
#ifdef PATH_TOOL_DEBUG
   else
      fprintf (stderr, "path_curve_get_point called without valid curve");
#endif
   return;
}

void
path_curve_draw_handles (GimpDrawTool *tool,
			 PathSegment *segment)
{
   if (segment && CurveTypes[segment->type].draw_handles)
      (* CurveTypes[segment->type].draw_handles) (tool, segment);

   /* straight lines do not have handles... */
   return;
}
			  
void
path_curve_draw_segment (GimpDrawTool *tool,
			 PathSegment *segment)
{
   gint num_pts;

   if (segment && segment->next) {
      if (CurveTypes[segment->type].draw_segment) {
         (* CurveTypes[segment->type].draw_segment) (tool, segment);
         return;
      } else {
	 gdouble *coordinates = g_new (gdouble, 200);
         num_pts = path_curve_get_points (segment, coordinates, 100, 0, 1);
	 gimp_draw_tool_draw_lines (tool, coordinates, num_pts, FALSE);
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
path_curve_on_segment (PathSegment *segment,
		       gint x,
		       gint y,
		       gint halfwidth,
		       gint *distance)
{

   if (segment && CurveTypes[segment->type].on_segment)
      return (* CurveTypes[segment->type].on_segment) (segment, x, y, halfwidth, distance);
   else {
      if (segment && segment->next) {
#if 1
	 gint x1, y1, numpts, index;
	 gdouble *coordinates;
	 gint bestindex = -1;

	 coordinates = g_new (gdouble, 200);
	 *distance = halfwidth * halfwidth + 1;

         numpts = path_curve_get_points (segment, coordinates, 100, 0, 1);
	 for (index=0; index < numpts; index++) {
	    x1 = coordinates[2*index];
	    y1 = coordinates[2*index+1];
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
path_curve_drag_segment (PathSegment *segment,
			 gdouble position,
			 gdouble dx,
			 gdouble dy)
{
   if (segment && CurveTypes[segment->type].drag_segment)
      (* CurveTypes[segment->type].drag_segment) (segment, position, dx, dy);
   return;
}

gint
path_curve_on_handle (PathSegment *segment,
		      gdouble x,
		      gdouble y,
		      gdouble halfwidth)
{
   if (segment && CurveTypes[segment->type].on_handles)
      return (* CurveTypes[segment->type].on_handles) (segment, x, y, halfwidth);
   return FALSE;
}

void
path_curve_drag_handle (PathSegment *segment,
			gdouble dx,
			gdouble dy,
			gint handle_id)
{
   if (segment && CurveTypes[segment->type].drag_handle)
      (* CurveTypes[segment->type].drag_handle) (segment, dx, dy, handle_id);
}

PathSegment *
path_curve_insert_anchor (PathSegment *segment,
			  gdouble position)
{
   if (segment && CurveTypes[segment->type].insert_anchor)
      return (* CurveTypes[segment->type].insert_anchor) (segment, position);
   else {
      return path_split_segment (segment, position);
   }
}

void
path_curve_flip_segment (PathSegment *segment)
{
   if (segment && CurveTypes[segment->type].flip_segment)
      (* CurveTypes[segment->type].flip_segment) (segment);
   return;
}

void
path_curve_update_segment (PathSegment *segment)
{
   if (segment && CurveTypes[segment->type].update_segment)
      (* CurveTypes[segment->type].update_segment) (segment);
   return;
}

void
path_curve_init_segment (PathSegment *segment)
{
   if (segment && CurveTypes[segment->type].init_segment)
      (* CurveTypes[segment->type].init_segment) (segment);
   return;
}

void
path_curve_cleanup_segment (PathSegment *segment)
{
   if (segment && CurveTypes[segment->type].cleanup_segment)
      (* CurveTypes[segment->type].cleanup_segment) (segment);
   return;
}

