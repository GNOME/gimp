/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
#ifndef __PATH_TOOLP_H__
#define __PATH_TOOLP_H__


#define IMAGE_COORDS    1
#define AA_IMAGE_COORDS 2
#define SCREEN_COORDS   3

#define SEGMENT_ACTIVE  1

#define PATH_TOOL_DRAG  1

#define PATH_TOOL_REDRAW_ALL    1
#define PATH_TOOL_REDRAW_ACTIVE   2
#define PATH_TOOL_REDRAW_HANDLES  4

#define SUBDIVIDE  1000


enum { ON_ANCHOR, ON_HANDLE, ON_CURVE, ON_CANVAS };

typedef enum { SEGMENT_LINE, SEGMENT_BEZIER } SegmentType;


typedef struct _path_segment PathSegment;
typedef struct _path_curve PathCurve;
typedef struct _path Path;

typedef struct _path_tool PathTool;

struct _path_segment
{
   SegmentType type;          /* What type of segment */
   gdouble x, y;              /* location of starting-point in image space  */
   gpointer data;             /* Additional data, dependant of segment-type */
   
   guint32 flags;             /* Various Flags: Is the Segment active? */

   PathCurve *parent;         /* the parent Curve */
   PathSegment *next;         /* Next Segment or NULL */
   PathSegment *prev;         /* Previous Segment or NULL */

};


struct _path_curve
{
   PathSegment * segments;    /* The segments of the curve */
   PathSegment * cur_segment; /* the current segment */
   Path * parent;             /* the parent Path */
   PathCurve * next;          /* Next Curve or NULL */
   PathCurve * prev;          /* Previous Curve or NULL */
};


struct _path
{
   PathCurve * curves;        /* the curves */
   PathCurve * cur_curve;     /* the current curve */
   GString * name;            /* the name of the path */
   guint32 state;             /* is the path locked? */
   PathTool * path_tool;      /* The parent Path Tool */
};


struct _path_tool
{
   gint click_pos;            /* where did the user click?         */
   gint click_x;              /* X-coordinate of the click         */
   gint click_y;              /* Y-coordinate of the click         */
   gint click_halfwidth;
   guint click_modifier;      /* what modifiers were pressed?      */
   Path *click_path;          /* On which Path/Curve/Segment       */
   PathCurve *click_curve;    /* was the click?                    */
   PathSegment *click_segment;

   gint active_count;         /* How many segments are active?     */
   /*
    * WARNING: single_active_segment may contain non NULL Values
    * which point to the nirvana. But they are important!
    * The pointer is garantueed to be valid, when active_count==1
    */
   PathSegment *single_active_segment;  /* The only active segment */

   gint state;                /* state of tool                     */
   gint draw;                 /* all or part                       */
   DrawCore *core;            /* Core drawing object               */
   Path *cur_path;            /* the current active path           */
   GSList **scanlines;        /* used in converting a path         */
};

typedef void (*PathTraverseFunc) (Path *, PathCurve *, gpointer);
typedef void (*CurveTraverseFunc) (Path *, PathCurve *, PathSegment *, gpointer);
typedef void (*SegmentTraverseFunc) (Path *, PathCurve *, PathSegment *, gint, gint, gpointer);

/* typedef void (*SegmentTraverseFunc) (PathTool *, GdkPoint *, gint, gpointer);*/

#endif /* __PATH_TOOLP_H__ */
