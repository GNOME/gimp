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

#ifndef __PATH_CURVES_H__
#define __PATH_CURVES_H__

#include <gdk/gdk.h>
#include "path_toolP.h"


/*
 * This function is to get a set of npoints different coordinates for
 * the range from start to end (each in the range from 0 to 1 and
 * start < end).
 * returns the number of created coords. Make sure that the points-
 * Array is allocated.
 */

typedef guint (*PathGetPointsFunc) (PathTool *path_tool,
                        	    PathSegment *segment,
				    GdkPoint *points,
				    guint npoints,
				    gdouble start,
				    gdouble end);

typedef void (*PathGetPointFunc) (PathTool *path_tool,
		       		  PathSegment *segment,
		       		  gdouble position,
		       		  gdouble *x,
		       		  gdouble *y);

typedef void (*PathDrawHandlesFunc) (Tool *tool,
			  	     PathSegment *segment);
			  
typedef void (*PathDrawSegmentFunc) (Tool *tool,
			  	     PathSegment *segment);
			  

typedef gdouble (*PathOnSegmentFunc) (Tool *tool,
				      PathSegment *segment,
				      gint x,
				      gint y,
				      gint halfwidth,
				      gint *distance);

typedef void (*PathDragSegmentFunc) (PathTool *path_tool,
				     PathSegment *segment,
				     gdouble position,
				     gint x,
				     gint y
				     );

typedef gboolean (*PathOnHandlesFunc) (PathTool *path_tool,
				       PathSegment *segment,
				       gint x,
				       gint y,
				       gint halfwidth);

typedef void (*PathDragHandleFunc) (PathTool *path_tool,
				    PathSegment *segment,
				    gint x,
				    gint y
				    );

typedef PathSegment * (*PathInsertAnchorFunc) (PathTool *path_tool,
			   	      PathSegment *segment,
			   	      gdouble position);

typedef void (*PathUpdateSegmentFunc) (PathTool *path_tool,
			    	       PathSegment *segment);

typedef void (*PathFlipSegmentFunc) (PathTool *path_tool,
			    	       PathSegment *segment);

typedef struct {
   PathGetPointsFunc     get_points;
   PathGetPointFunc      get_point;
   PathDrawHandlesFunc   draw_handles;
   PathDrawSegmentFunc   draw_segment;
   PathOnSegmentFunc     on_segment;
   PathDragSegmentFunc   drag_segment;
   PathOnHandlesFunc     on_handles;
   PathDragHandleFunc    drag_handle;
   PathInsertAnchorFunc  insert_anchor;
   PathUpdateSegmentFunc update_segment;
   PathFlipSegmentFunc   flip_segment;
} CurveDescription;


guint
path_curve_get_points (PathTool *path_tool,
                        	    PathSegment *segment,
				    GdkPoint *points,
				    guint npoints,
				    gdouble start,
				    gdouble end);

void
path_curve_get_point (PathTool *path_tool,
		       		  PathSegment *segment,
		       		  gdouble position,
		       		  gdouble *x,
		       		  gdouble *y);

void
path_curve_draw_handles (Tool *tool,
			  	     PathSegment *segment);
			  
void
path_curve_draw_segment (Tool *tool,
			  	     PathSegment *segment);
			  

gdouble
path_curve_on_segment (Tool *tool,
				     PathSegment *segment,
				     gint x,
				     gint y,
				     gint halfwidth,
				     gint *distance);

void
path_curve_drag_segment (PathTool *path_tool,
				       PathSegment *segment,
				       gdouble position,
				       gint x,
				       gint y
				       );

gboolean
path_curve_on_handle (PathTool *path_tool,
		      PathSegment *segment,
		      gint x,
		      gint y,
		      gint halfwidth);

void
path_curve_drag_handle (PathTool *path_tool,
			PathSegment *segment,
			gint x,
			gint y);

PathSegment *
path_curve_insert_anchor (PathTool *path_tool,
			  PathSegment *segment,
			  gdouble position);

void
path_curve_update_segment (PathTool *path_tool,
			   PathSegment *segment);

void
path_curve_flip_segment (PathTool *path_tool,
			 PathSegment *segment);










/* This is, what Soleil (Olofs little daughter) has to say to this:

fc fc g hgvfvv  drrrrrrtcc jctfcz w   sdzs d bx   cv^[ ^[c^[f c
vffvcccccccccccccggfc fvx^[c^[x^[x^[x^[x^[x^[x^[ v       xbvcbvcxv cxxc xxxx^[x
xz^[c^[x^[x^[x^[x^[x^[x^[xxxxxxcccccccxxxxxxxxxxxxxxxvä"åp'
hj^[[24~^[[4~^[[1~^[[4~^[[1~^[[4~ ^[[D^[[Bk^[[B,,,,,
,^[[2~^[[4~^[[6~^[[4~l^[[6~,l' .holg^[[B^[[B n,,klmj ^[[B^[[1~j ^[[P^[[B
^[[D^[[4~^[[6~nb ^[[A^[[C ^[[Akj^[[B            ^[[A^[[C^[[A


...^[[1~^[[D^[[4~^[[2~^[[C^[[B,^[[A^[[2~^[[C^[[2~^[[A^[[3~^[[A^[[4~ ^[[2~
^[[2~pö-  ., å^[[Aöpl.,  k,km ,
m,^[[5~^[[6~^[[2~^[[C^[[3~p^[[A^[[Bö^[[2~^[[B^[[6~^[[1~, .^[[D^[[4~^[[2~^[[Db
.l, .,.,m ^[[2~pöl. ik
^[[20~kl9i^[[20~^[[20~^[[20~^[[21~^[[21~^[[21~^[[21~^[[21~^[[21~^[[20~m +
^[[A^[[5~^[[G^[[D ^[[5~^[[1+^[[C

*/

#endif /* __PATH_CURVES_H__ */

