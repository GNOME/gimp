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
#ifndef __BEZIER_SELECTP_H__
#define __BEZIER_SELECTP_H__

#include "draw_core.h"

#define BEZIER_START     1
#define BEZIER_ADD       2
#define BEZIER_EDIT      4
#define BEZIER_DRAG      8

#define BEZIER_ANCHOR    1
#define BEZIER_CONTROL   2
#define BEZIER_MOVE      3

#define IMAGE_COORDS    1
#define AA_IMAGE_COORDS 2
#define SCREEN_COORDS   3

#define SUBDIVIDE  1000

enum { EXTEND_EDIT, EXTEND_ADD, EXTEND_REMOVE, EXTEND_NEW };

struct _BezierPoint
{
  gint         type;          /* type of point (anchor/control/move) */
  gdouble      x, y;          /* location of point in image space  */
  gint         sx, sy;        /* location of point in screen space */
  BezierPoint *next;          /* next point on curve               */
  BezierPoint *prev;          /* prev point on curve               */
  BezierPoint *next_curve;    /* Next curve segment                */
  gint         pointflags;    /* Status of point 0 = not selected 
			       * 1 = selected 
			       */ 
};

struct _BezierSelect
{
  gint          state;        /* start, add, edit or drag          */
  gint          draw;         /* all or part                       */
  gint          closed;       /* is the last curve closed          */
  DrawCore     *core;         /* Core drawing object               */
  BezierPoint  *points;       /* the curve                         */
  BezierPoint  *cur_anchor;   /* the current active anchor point   */
  BezierPoint  *cur_control;  /* the current active control point  */
  BezierPoint  *last_point;   /* the last point on the curve       */
  gint          num_points;   /* number of points in the curve     */
  Channel      *mask;         /* null if the curve is open         */
  GSList      **scanlines;    /* used in converting a curve        */
};

/* All udata that are passed to the bezier_draw_curve must
 * have this structure as the first element.
 */

typedef struct
{
  gint count;
} CountCurves;

typedef void (* BezierPointsFunc) (BezierSelect *, GdkPoint *, int,gpointer);

/*  Functions  */
int   bezier_select_load                   (void *,
					    BezierPoint *,
					    gint,
					    gint);
void  bezier_draw_curve                    (BezierSelect *,
					    BezierPointsFunc,
					    gint,
					    gpointer);
void  bezier_select_reset                  (BezierSelect *bezier_sel);
void  bezier_select_free                   (BezierSelect *bezier_sel);
void  bezier_add_point                     (BezierSelect *,
					    gint,
					    gdouble,
					    gdouble);
void  bezier_paste_bezierselect_to_current (GDisplay     *gdisp,
					    BezierSelect *bezier_sel);
void  bezier_select_mode                   (gint);
void  bezier_stroke 		           (BezierSelect *bezier_sel,
					    GDisplay     *gdisp,
					    gint,
					    gint);
void  bezier_to_selection                  (BezierSelect *bezier_sel,
					    GDisplay     *gdisp);
gint  bezier_distance_along                (BezierSelect *bezier_sel,
					    gint,
					    gdouble,
					    gint *,
					    gint *,
					    gdouble *);
void  bezier_draw                          (GDisplay     *gdisp,
					    BezierSelect *bezier_sel);

#endif /* __BEZIER_SELECTP_H__ */
