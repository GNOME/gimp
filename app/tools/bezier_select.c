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
#include <stdlib.h>
#include <string.h>
#include "appenv.h"
#include "bezier_select.h"
#include "bezier_selectP.h"
#include "draw_core.h"
#include "edit_selection.h"
#include "errors.h"
#include "gdisplay.h"
#include "gimage_mask.h"
#include "rect_select.h"
#include "interface.h"

#define BEZIER_START     1
#define BEZIER_ADD       2
#define BEZIER_EDIT      4
#define BEZIER_DRAG      8

#define BEZIER_DRAW_CURVE   1
#define BEZIER_DRAW_CURRENT 2
#define BEZIER_DRAW_HANDLES 4
#define BEZIER_DRAW_ALL     (BEZIER_DRAW_CURVE | BEZIER_DRAW_HANDLES)

#define BEZIER_WIDTH     8
#define BEZIER_HALFWIDTH 4

#define SUBDIVIDE  1000

#define IMAGE_COORDS    1
#define AA_IMAGE_COORDS 2
#define SCREEN_COORDS   3

#define SUPERSAMPLE   3
#define SUPERSAMPLE2  9

#define NO  0
#define YES 1

/*  bezier select type definitions */

typedef struct _bezier_select BezierSelect;
typedef double BezierMatrix[4][4];
typedef void (*BezierPointsFunc) (BezierSelect *, GdkPoint *, int);

struct _bezier_select
{
  int state;                 /* start, add, edit or drag          */
  int draw;                  /* all or part                       */
  int closed;                /* is the curve closed               */
  DrawCore *core;            /* Core drawing object               */
  BezierPoint *points;       /* the curve                         */
  BezierPoint *cur_anchor;   /* the current active anchor point   */
  BezierPoint *cur_control;  /* the current active control point  */
  BezierPoint *last_point;   /* the last point on the curve       */
  int num_points;            /* number of points in the curve     */
  Channel *mask;             /* null if the curve is open         */
  GSList **scanlines;        /* used in converting a curve        */
};

static void  bezier_select_reset           (BezierSelect *);
static void  bezier_select_button_press    (Tool *, GdkEventButton *, gpointer);
static void  bezier_select_button_release  (Tool *, GdkEventButton *, gpointer);
static void  bezier_select_motion          (Tool *, GdkEventMotion *, gpointer);
static void  bezier_select_control         (Tool *, int, gpointer);
static void  bezier_select_draw            (Tool *);

static void  bezier_add_point              (BezierSelect *, int, int, int);
static void  bezier_offset_point           (BezierPoint *, int, int);
static int   bezier_check_point            (BezierPoint *, int, int, int);
static void  bezier_draw_curve             (BezierSelect *);
static void  bezier_draw_handles           (BezierSelect *);
static void  bezier_draw_current           (BezierSelect *);
static void  bezier_draw_point             (BezierSelect *, BezierPoint *, int);
static void  bezier_draw_line              (BezierSelect *, BezierPoint *, BezierPoint *);
static void  bezier_draw_segment           (BezierSelect *, BezierPoint *, int, int, BezierPointsFunc);
static void  bezier_draw_segment_points    (BezierSelect *, GdkPoint *, int);
static void  bezier_compose                (BezierMatrix, BezierMatrix, BezierMatrix);

static void  bezier_convert                (BezierSelect *, GDisplay *, int, int);
static void  bezier_convert_points         (BezierSelect *, GdkPoint *, int);
static void  bezier_convert_line           (GSList **, int, int, int, int);
static GSList *  bezier_insert_in_list     (GSList *, int);

static BezierMatrix basis =
{
  { -1,  3, -3,  1 },
  {  3, -6,  3,  0 },
  { -3,  3,  0,  0 },
  {  1,  0,  0,  0 },
};

static SelectionOptions *bezier_options = NULL;


Tool*
tools_new_bezier_select ()
{
  Tool * tool;
  BezierSelect * bezier_sel;

  /*  The tool options  */
  if (!bezier_options)
    bezier_options = create_selection_options (BEZIER_SELECT);

  tool = g_malloc (sizeof (Tool));

  bezier_sel = g_malloc (sizeof (BezierSelect));

  bezier_sel->num_points = 0;
  bezier_sel->mask = NULL;
  bezier_sel->core = draw_core_new (bezier_select_draw);
  bezier_select_reset (bezier_sel);

  tool->type = BEZIER_SELECT;
  tool->state = INACTIVE;
  tool->scroll_lock = 1;   /*  Do not allow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->private = (void *) bezier_sel;
  tool->button_press_func = bezier_select_button_press;
  tool->button_release_func = bezier_select_button_release;
  tool->motion_func = bezier_select_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = rect_select_cursor_update;
  tool->control_func = bezier_select_control;
  tool->preserve = FALSE;

  return tool;
}

void
tools_free_bezier_select (Tool *tool)
{
  BezierSelect * bezier_sel;

  bezier_sel = tool->private;

  if (tool->state == ACTIVE)
    draw_core_stop (bezier_sel->core, tool);
  draw_core_free (bezier_sel->core);

  bezier_select_reset (bezier_sel);

  g_free (bezier_sel);
}

int
bezier_select_load (void        *gdisp_ptr,
		    BezierPoint *pts,
		    int          num_pts,
		    int          closed)
{
  GDisplay * gdisp;
  Tool * tool;
  BezierSelect * bezier_sel;

  gdisp = (GDisplay *) gdisp_ptr;

  /*  select the bezier tool  */
  gtk_widget_activate (tool_widgets[tool_info[BEZIER_SELECT].toolbar_position]);
  tool = active_tool;
  tool->state = ACTIVE;
  tool->gdisp_ptr = gdisp_ptr;
  bezier_sel = (BezierSelect *) tool->private;

  bezier_sel->points = pts;
  bezier_sel->last_point = pts->prev;
  bezier_sel->num_points = num_pts;
  bezier_sel->closed = closed;
  bezier_sel->state = BEZIER_EDIT;
  bezier_sel->draw = BEZIER_DRAW_ALL;

  bezier_convert (bezier_sel, tool->gdisp_ptr, SUBDIVIDE, NO);

  draw_core_start (bezier_sel->core, gdisp->canvas->window, tool);

  return 1;
}

static void
bezier_select_reset (BezierSelect *bezier_sel)
{
  BezierPoint * points;
  BezierPoint * start_pt;
  BezierPoint * temp_pt;

  if (bezier_sel->num_points > 0)
    {
      points = bezier_sel->points;
      start_pt = (bezier_sel->closed) ? (bezier_sel->points) : (NULL);

      do {
	temp_pt = points;
	points = points->next;

	g_free (temp_pt);
      } while (points != start_pt);
    }

  if (bezier_sel->mask)
    channel_delete (bezier_sel->mask);

  bezier_sel->state = BEZIER_START;    /* we are starting the curve */
  bezier_sel->draw = BEZIER_DRAW_ALL;  /* draw everything by default */
  bezier_sel->closed = 0;              /* the curve is initally open */
  bezier_sel->points = NULL;           /* initially there are no points */
  bezier_sel->cur_anchor = NULL;
  bezier_sel->cur_control = NULL;
  bezier_sel->last_point = NULL;
  bezier_sel->num_points = 0;          /* intially there are no points */
  bezier_sel->mask = NULL;             /* empty mask */
  bezier_sel->scanlines = NULL;
}

static void
bezier_select_button_press (Tool           *tool,
			    GdkEventButton *bevent,
			    gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  BezierSelect *bezier_sel;
  BezierPoint *points;
  BezierPoint *start_pt;
  int grab_pointer;
  int op, replace;
  int x, y;
  int halfwidth, dummy;

  gdisp = (GDisplay *) gdisp_ptr;
  bezier_sel = tool->private;
  grab_pointer = 0;

  /*  If the tool was being used in another image...reset it  */
  if (tool->state == ACTIVE && gdisp_ptr != tool->gdisp_ptr) {
    printf("Reset!\n");
    bezier_select_reset (bezier_sel);
  }
  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y, &x, &y, TRUE, 0);

  /* get halfwidth in image coord */
  gdisplay_untransform_coords (gdisp, bevent->x + BEZIER_HALFWIDTH, 0, &halfwidth, &dummy, TRUE, 0);
  halfwidth -= x;

  switch (bezier_sel->state)
    {
    case BEZIER_START:
      grab_pointer = 1;
      tool->state = ACTIVE;
      tool->gdisp_ptr = gdisp_ptr;

      if (bevent->state & GDK_MOD1_MASK)
	{
	  init_edit_selection (tool, gdisp_ptr, bevent, MaskTranslate);
	  break;
	}
      else if (!(bevent->state & GDK_SHIFT_MASK) && !(bevent->state & GDK_CONTROL_MASK))
	if (! (layer_is_floating_sel (gimage_get_active_layer (gdisp->gimage))) &&
	    gdisplay_mask_value (gdisp, bevent->x, bevent->y) > HALF_WAY)
	  {
	    init_edit_selection (tool, gdisp_ptr, bevent, MaskToLayerTranslate);
	    break;
	  }

      bezier_sel->state = BEZIER_ADD;
      bezier_sel->draw = BEZIER_DRAW_CURRENT | BEZIER_DRAW_HANDLES;

      bezier_add_point (bezier_sel, BEZIER_ANCHOR, x, y);
      bezier_add_point (bezier_sel, BEZIER_CONTROL, x, y);

      draw_core_start (bezier_sel->core, gdisp->canvas->window, tool);
      break;
    case BEZIER_ADD:
      grab_pointer = 1;

      if (bezier_sel->cur_anchor &&
	  bezier_check_point (bezier_sel->cur_anchor, x, y, halfwidth))
	{
	  break;
	}

      if (bezier_sel->cur_anchor->next &&
	  bezier_check_point (bezier_sel->cur_anchor->next, x, y, halfwidth))
	{
	  bezier_sel->cur_control = bezier_sel->cur_anchor->next;
	  break;
	}

      if (bezier_sel->cur_anchor->prev &&
	  bezier_check_point (bezier_sel->cur_anchor->prev, x, y, halfwidth))
	{
	  bezier_sel->cur_control = bezier_sel->cur_anchor->prev;
	  break;
	}

      if (bezier_check_point (bezier_sel->points, x, y, halfwidth))
	{
	  bezier_sel->draw = BEZIER_DRAW_ALL;
	  draw_core_pause (bezier_sel->core, tool);

	  bezier_add_point (bezier_sel, BEZIER_CONTROL, x, y);
	  bezier_sel->last_point->next = bezier_sel->points;
	  bezier_sel->points->prev = bezier_sel->last_point;
	  bezier_sel->cur_anchor = bezier_sel->points;
	  bezier_sel->cur_control = bezier_sel->points->next;

	  bezier_sel->closed = 1;
	  bezier_sel->state = BEZIER_EDIT;
	  bezier_sel->draw = BEZIER_DRAW_ALL;

	  draw_core_resume (bezier_sel->core, tool);
	}
      else
	{
	  bezier_sel->draw = BEZIER_DRAW_HANDLES;
	  draw_core_pause (bezier_sel->core, tool);

	  bezier_add_point (bezier_sel, BEZIER_CONTROL, x, y);
	  bezier_add_point (bezier_sel, BEZIER_ANCHOR, x, y);
	  bezier_add_point (bezier_sel, BEZIER_CONTROL, x, y);

	  bezier_sel->draw = BEZIER_DRAW_CURRENT | BEZIER_DRAW_HANDLES;
	  draw_core_resume (bezier_sel->core, tool);
	}
      break;
    case BEZIER_EDIT:
      if (!bezier_sel->closed)
	fatal_error ("tried to edit on open bezier curve");

      /* erase the handles */
      bezier_sel->draw = BEZIER_DRAW_HANDLES;
      draw_core_pause (bezier_sel->core, tool);

      /* unset the current anchor and control */
      bezier_sel->cur_anchor = NULL;
      bezier_sel->cur_control = NULL;

      points = bezier_sel->points;
      start_pt = bezier_sel->points;

      /* find if the button press occurred on a point */
      do {
	  if (bezier_check_point (points, x, y, halfwidth))
	    {
	      /* set the current anchor and control points */
	      switch (points->type)
		{
		case BEZIER_ANCHOR:
		  bezier_sel->cur_anchor = points;
		  bezier_sel->cur_control = bezier_sel->cur_anchor->next;
		  break;
		case BEZIER_CONTROL:
		  bezier_sel->cur_control = points;
		  if (bezier_sel->cur_control->next->type == BEZIER_ANCHOR)
		    bezier_sel->cur_anchor = bezier_sel->cur_control->next;
		  else
		    bezier_sel->cur_anchor = bezier_sel->cur_control->prev;
		  break;
		}
	      grab_pointer = 1;
	      break;
	    }

	  points = points->next;
	} while (points != start_pt);

      if (!grab_pointer && channel_value (bezier_sel->mask, x, y))
	{
	  /*  If we're antialiased, then recompute the
	   *  mask...
	   */
	  if (bezier_options->antialias)
	    bezier_convert (bezier_sel, tool->gdisp_ptr, SUBDIVIDE, YES);

	  tool->state = INACTIVE;
	  bezier_sel->draw = BEZIER_DRAW_CURVE;
	  draw_core_resume (bezier_sel->core, tool);

	  bezier_sel->draw = 0;
	  draw_core_stop (bezier_sel->core, tool);

	  replace = 0;
	  if ((bevent->state & GDK_SHIFT_MASK) && !(bevent->state & GDK_CONTROL_MASK))
	    op = ADD;
	  else if ((bevent->state & GDK_CONTROL_MASK) && !(bevent->state & GDK_SHIFT_MASK))
	    op = SUB;
	  else if ((bevent->state & GDK_CONTROL_MASK) && (bevent->state & GDK_SHIFT_MASK))
	    op = INTERSECT;
	  else
	    {
	      op = ADD;
	      replace = 1;
	    }

	  if (replace)
	    gimage_mask_clear (gdisp->gimage);
	  else
	    gimage_mask_undo (gdisp->gimage);

	  if (bezier_options->feather)
	    channel_feather (bezier_sel->mask,
			     gimage_get_mask (gdisp->gimage),
			     bezier_options->feather_radius, op, 0, 0);
	  else
	    channel_combine_mask (gimage_get_mask (gdisp->gimage),
				  bezier_sel->mask, op, 0, 0);

	  bezier_select_reset (bezier_sel);

	  /*  show selection on all views  */
	  gdisplays_flush ();
	}
      else
	{
	  /* draw the handles */
	  bezier_sel->draw = BEZIER_DRAW_HANDLES;
	  draw_core_resume (bezier_sel->core, tool);
	}
      break;
    }

  if (grab_pointer)
    gdk_pointer_grab (gdisp->canvas->window, FALSE,
		      GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
		      NULL, NULL, bevent->time);
}

static void
bezier_select_button_release (Tool           *tool,
			      GdkEventButton *bevent,
			      gpointer        gdisp_ptr)
{
  GDisplay * gdisp;
  BezierSelect *bezier_sel;

  gdisp = tool->gdisp_ptr;
  bezier_sel = tool->private;
  bezier_sel->state &= ~(BEZIER_DRAG);

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  if (bezier_sel->closed)
    bezier_convert (bezier_sel, tool->gdisp_ptr, SUBDIVIDE, NO);
}

static void
bezier_select_motion (Tool           *tool,
		      GdkEventMotion *mevent,
		      gpointer        gdisp_ptr)
{
  static int lastx, lasty;

  GDisplay * gdisp;
  BezierSelect * bezier_sel;
  BezierPoint * anchor;
  BezierPoint * opposite_control;
  int offsetx;
  int offsety;
  int x, y;

  if (tool->state != ACTIVE)
    return;

  gdisp = gdisp_ptr;
  bezier_sel = tool->private;

  if (!bezier_sel->cur_anchor || !bezier_sel->cur_control)
    return;

  bezier_sel->draw = BEZIER_DRAW_CURRENT | BEZIER_DRAW_HANDLES;
  draw_core_pause (bezier_sel->core, tool);

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &x, &y, TRUE, 0);

  /* If this is the first point then change the state and "remember" the point.
   */
  if (!(bezier_sel->state & BEZIER_DRAG))
    {
      bezier_sel->state |= BEZIER_DRAG;
      lastx = x;
      lasty = y;
    }

  if (mevent->state & GDK_CONTROL_MASK)
    {
      /* the control key is down ... move the current anchor point */
      /* we must also move the neighboring control points appropriately */

      offsetx = x - lastx;
      offsety = y - lasty;

      bezier_offset_point (bezier_sel->cur_anchor, offsetx, offsety);
      bezier_offset_point (bezier_sel->cur_anchor->next, offsetx, offsety);
      bezier_offset_point (bezier_sel->cur_anchor->prev, offsetx, offsety);
    }
  else
    {
      /* the control key is not down ... we move the current control point */

      offsetx = x - bezier_sel->cur_control->x;
      offsety = y - bezier_sel->cur_control->y;

      bezier_offset_point (bezier_sel->cur_control, offsetx, offsety);

      /* if the shift key is not down then we align the opposite control */
      /* point...ie the opposite control point acts like a mirror of the */
      /* current control point */

      if (!(mevent->state & GDK_SHIFT_MASK))
	{
	  anchor = NULL;
	  opposite_control = NULL;

	  if (bezier_sel->cur_control->next)
	    {
	      if (bezier_sel->cur_control->next->type == BEZIER_ANCHOR)
		{
		  anchor = bezier_sel->cur_control->next;
		  opposite_control = anchor->next;
		}
	    }
	  if (bezier_sel->cur_control->prev)
	    {
	      if (bezier_sel->cur_control->prev->type == BEZIER_ANCHOR)
		{
		  anchor = bezier_sel->cur_control->prev;
		  opposite_control = anchor->prev;
		}
	    }

	  if (!anchor)
	    fatal_error ("Encountered orphaned bezier control point");

	  if (opposite_control)
	    {
	      offsetx = bezier_sel->cur_control->x - anchor->x;
	      offsety = bezier_sel->cur_control->y - anchor->y;

	      opposite_control->x = anchor->x - offsetx;
	      opposite_control->y = anchor->y - offsety;
	    }
	}
    }

  bezier_sel->draw = BEZIER_DRAW_CURRENT | BEZIER_DRAW_HANDLES;
  draw_core_resume (bezier_sel->core, tool);

  lastx = x;
  lasty = y;
}

static void
bezier_select_control (Tool     *tool,
		       int       action,
		       gpointer  gdisp_ptr)
{
  BezierSelect * bezier_sel;

  bezier_sel = tool->private;

  switch (action)
    {
    case PAUSE :
      draw_core_pause (bezier_sel->core, tool);
      break;
    case RESUME :
      draw_core_resume (bezier_sel->core, tool);
      break;
    case HALT :
      draw_core_stop (bezier_sel->core, tool);
      bezier_select_reset (bezier_sel);
      break;
    }
}

static void
bezier_select_draw (Tool *tool)
{
  GDisplay * gdisp;
  BezierSelect * bezier_sel;
  BezierPoint * points;
  int num_points;
  int draw_curve;
  int draw_handles;
  int draw_current;

  gdisp = tool->gdisp_ptr;
  bezier_sel = tool->private;

  if (!bezier_sel->draw)
    return;

  draw_curve = bezier_sel->draw & BEZIER_DRAW_CURVE;
  draw_current = bezier_sel->draw & BEZIER_DRAW_CURRENT;
  draw_handles = bezier_sel->draw & BEZIER_DRAW_HANDLES;

  /* reset to the default drawing state of drawing the curve and handles */
  bezier_sel->draw = BEZIER_DRAW_ALL;

  /* transform the points from image space to screen space */
  points = bezier_sel->points;
  num_points = bezier_sel->num_points;

  while (points && num_points)
   {
      gdisplay_transform_coords (gdisp, points->x, points->y,
				 &points->sx, &points->sy, 0);
      points = points->next;
      num_points--;
    }

  if (draw_curve)
    bezier_draw_curve (bezier_sel);
  if (draw_handles)
    bezier_draw_handles (bezier_sel);
  if (draw_current)
    bezier_draw_current (bezier_sel);
}

static void
bezier_add_point (BezierSelect *bezier_sel,
		  int           type,
		  int           x,
		  int           y)
{
  BezierPoint *newpt;

  newpt = g_malloc (sizeof (BezierPoint));

  newpt->type = type;
  newpt->x = x;
  newpt->y = y;
  newpt->next = NULL;
  newpt->prev = NULL;

  if (bezier_sel->last_point)
    {
      bezier_sel->last_point->next = newpt;
      newpt->prev = bezier_sel->last_point;
      bezier_sel->last_point = newpt;
    }
  else
    {
      bezier_sel->points = newpt;
      bezier_sel->last_point = newpt;
    }

  switch (type)
    {
    case BEZIER_ANCHOR:
      bezier_sel->cur_anchor = newpt;
      break;
    case BEZIER_CONTROL:
      bezier_sel->cur_control = newpt;
      break;
    }

  bezier_sel->num_points += 1;
}

static void
bezier_offset_point (BezierPoint *pt,
		     int          x,
		     int          y)
{
  if (pt)
    {
      pt->x += x;
      pt->y += y;
    }
}

static int
bezier_check_point (BezierPoint *pt,
		    int          x,
		    int          y,
		    int		 halfwidth)
{
  int l, r, t, b;

  if (pt)
    {
      l = pt->x - halfwidth;
      r = pt->x + halfwidth;
      t = pt->y - halfwidth;
      b = pt->y + halfwidth;

      return ((x >= l) && (x <= r) && (y >= t) && (y <= b));
    }

  return 0;
}

static void
bezier_draw_curve (BezierSelect *bezier_sel)
{
  BezierPoint * points;
  BezierPoint * start_pt;
  int num_points;

  points = bezier_sel->points;

  if (bezier_sel->closed)
    {
      start_pt = bezier_sel->points;

      do {
	bezier_draw_segment (bezier_sel, points,
			     SUBDIVIDE, SCREEN_COORDS,
			     bezier_draw_segment_points);

	points = points->next;
	points = points->next;
	points = points->next;
      } while (points != start_pt);
    }
  else
    {
      num_points = bezier_sel->num_points;

      while (num_points >= 4)
	{
	  bezier_draw_segment (bezier_sel, points,
			       SUBDIVIDE, SCREEN_COORDS,
			       bezier_draw_segment_points);

	  points = points->next;
	  points = points->next;
	  points = points->next;
	  num_points -= 3;
	}
    }
}

static void
bezier_draw_handles (BezierSelect *bezier_sel)
{
  BezierPoint * points;
  int num_points;

  points = bezier_sel->points;
  num_points = bezier_sel->num_points;
  if (num_points <= 0)
    return;

  do {
    if (points == bezier_sel->cur_anchor)
      {
	bezier_draw_point (bezier_sel, points, 0);
	bezier_draw_point (bezier_sel, points->next, 0);
	bezier_draw_point (bezier_sel, points->prev, 0);
	bezier_draw_line (bezier_sel, points, points->next);
	bezier_draw_line (bezier_sel, points, points->prev);
      }
    else
      {
	bezier_draw_point (bezier_sel, points, 1);
      }

    if (points) points = points->next;
    if (points) points = points->next;
    if (points) points = points->next;
    num_points -= 3;
  } while (num_points > 0);
}

static void
bezier_draw_current (BezierSelect *bezier_sel)
{
  BezierPoint * points;

  points = bezier_sel->cur_anchor;

  if (points) points = points->prev;
  if (points) points = points->prev;
  if (points) points = points->prev;

  if (points)
    bezier_draw_segment (bezier_sel, points,
			 SUBDIVIDE, SCREEN_COORDS,
			 bezier_draw_segment_points);

  if (points != bezier_sel->cur_anchor)
    {
      points = bezier_sel->cur_anchor;

      if (points) points = points->next;
      if (points) points = points->next;
      if (points) points = points->next;

      if (points)
	bezier_draw_segment (bezier_sel, bezier_sel->cur_anchor,
			     SUBDIVIDE, SCREEN_COORDS,
			     bezier_draw_segment_points);
    }
}

static void
bezier_draw_point (BezierSelect *bezier_sel,
		   BezierPoint  *pt,
		   int           fill)
{
  if (pt)
    {
      switch (pt->type)
	{
	case BEZIER_ANCHOR:
	  if (fill)
	    {
	      gdk_draw_arc (bezier_sel->core->win, bezier_sel->core->gc, 1,
			    pt->sx - BEZIER_HALFWIDTH, pt->sy - BEZIER_HALFWIDTH,
			    BEZIER_WIDTH, BEZIER_WIDTH, 0, 23040);
	    }
	  else
	    {
	      gdk_draw_arc (bezier_sel->core->win, bezier_sel->core->gc, 0,
			    pt->sx - BEZIER_HALFWIDTH, pt->sy - BEZIER_HALFWIDTH,
			    BEZIER_WIDTH, BEZIER_WIDTH, 0, 23040);
	    }
	  break;
	case BEZIER_CONTROL:
	  if (fill)
	    {
	      gdk_draw_rectangle (bezier_sel->core->win, bezier_sel->core->gc, 1,
				  pt->sx - BEZIER_HALFWIDTH, pt->sy - BEZIER_HALFWIDTH,
				  BEZIER_WIDTH, BEZIER_WIDTH);
	    }
	  else
	    {
	      gdk_draw_rectangle (bezier_sel->core->win, bezier_sel->core->gc, 0,
				  pt->sx - BEZIER_HALFWIDTH, pt->sy - BEZIER_HALFWIDTH,
				  BEZIER_WIDTH, BEZIER_WIDTH);
	    }
	  break;
	}
    }
}

static void
bezier_draw_line (BezierSelect *bezier_sel,
		  BezierPoint  *pt1,
		  BezierPoint  *pt2)
{
  if (pt1 && pt2)
    {
      gdk_draw_line (bezier_sel->core->win,
		     bezier_sel->core->gc,
		     pt1->sx, pt1->sy, pt2->sx, pt2->sy);
    }
}

static void
bezier_draw_segment (BezierSelect     *bezier_sel,
		     BezierPoint      *points,
		     int               subdivisions,
		     int               space,
		     BezierPointsFunc  points_func)
{
#define ROUND(x)  ((int) ((x) + 0.5))

  static GdkPoint gdk_points[256];
  static int npoints = 256;

  BezierMatrix geometry;
  BezierMatrix tmp1, tmp2;
  BezierMatrix deltas;
  double x, dx, dx2, dx3;
  double y, dy, dy2, dy3;
  double d, d2, d3;
  int lastx, lasty;
  int newx, newy;
  int index;
  int i;

  /* construct the geometry matrix from the segment */
  /* assumes that a valid segment containing 4 points is passed in */

  for (i = 0; i < 4; i++)
    {
      if (!points)
	fatal_error ("bad bezier segment");

      switch (space)
	{
	case IMAGE_COORDS:
	  geometry[i][0] = points->x;
	  geometry[i][1] = points->y;
	  break;
	case AA_IMAGE_COORDS:
	  geometry[i][0] = points->x * SUPERSAMPLE;
	  geometry[i][1] = points->y * SUPERSAMPLE;
	  break;
	case SCREEN_COORDS:
	  geometry[i][0] = points->sx;
	  geometry[i][1] = points->sy;
	  break;
	default:
	  fatal_error ("unknown coordinate space: %d", space);
	  break;
	}

      geometry[i][2] = 0;
      geometry[i][3] = 0;

      points = points->next;
    }

  /* subdivide the curve n times */
  /* n can be adjusted to give a finer or coarser curve */

  d = 1.0 / subdivisions;
  d2 = d * d;
  d3 = d * d * d;

  /* construct a temporary matrix for determining the forward diffencing deltas */

  tmp2[0][0] = 0;     tmp2[0][1] = 0;     tmp2[0][2] = 0;    tmp2[0][3] = 1;
  tmp2[1][0] = d3;    tmp2[1][1] = d2;    tmp2[1][2] = d;    tmp2[1][3] = 0;
  tmp2[2][0] = 6*d3;  tmp2[2][1] = 2*d2;  tmp2[2][2] = 0;    tmp2[2][3] = 0;
  tmp2[3][0] = 6*d3;  tmp2[3][1] = 0;     tmp2[3][2] = 0;    tmp2[3][3] = 0;

  /* compose the basis and geometry matrices */
  bezier_compose (basis, geometry, tmp1);

  /* compose the above results to get the deltas matrix */
  bezier_compose (tmp2, tmp1, deltas);

  /* extract the x deltas */
  x = deltas[0][0];
  dx = deltas[1][0];
  dx2 = deltas[2][0];
  dx3 = deltas[3][0];

  /* extract the y deltas */
  y = deltas[0][1];
  dy = deltas[1][1];
  dy2 = deltas[2][1];
  dy3 = deltas[3][1];

  lastx = x;
  lasty = y;

  gdk_points[0].x = lastx;
  gdk_points[0].y = lasty;
  index = 1;

  /* loop over the curve */
  for (i = 0; i < subdivisions; i++)
    {
      /* increment the x values */
      x += dx;
      dx += dx2;
      dx2 += dx3;

      /* increment the y values */
      y += dy;
      dy += dy2;
      dy2 += dy3;

      newx = ROUND (x);
      newy = ROUND (y);

      /* if this point is different than the last one...then draw it */
      if ((lastx != newx) || (lasty != newy))
	{
	  /* add the point to the point buffer */
	  gdk_points[index].x = newx;
	  gdk_points[index].y = newy;
	  index++;

	  /* if the point buffer is full put it to the screen and zero it out */
	  if (index >= npoints)
	    {
	      (* points_func) (bezier_sel, gdk_points, index);
	      index = 0;
	    }
	}

      lastx = newx;
      lasty = newy;
    }

  /* if there are points in the buffer, then put them on the screen */
  if (index)
    (* points_func) (bezier_sel, gdk_points, index);
}

static void
bezier_draw_segment_points (BezierSelect *bezier_sel,
			    GdkPoint     *points,
			    int           npoints)
{
  gdk_draw_points (bezier_sel->core->win,
		   bezier_sel->core->gc, points, npoints);
}

static void
bezier_compose (BezierMatrix a,
		BezierMatrix b,
		BezierMatrix ab)
{
  int i, j;

  for (i = 0; i < 4; i++)
    {
      for (j = 0; j < 4; j++)
        {
          ab[i][j] = (a[i][0] * b[0][j] +
                      a[i][1] * b[1][j] +
                      a[i][2] * b[2][j] +
                      a[i][3] * b[3][j]);
        }
    }
}

static int start_convert;
static int width, height;
static int lastx;
static int lasty;

static void
bezier_convert (BezierSelect *bezier_sel,
		GDisplay     *gdisp,
		int           subdivisions,
		int           antialias)
{
  PixelRegion maskPR;
  BezierPoint * points;
  BezierPoint * start_pt;
  GSList * list;
  unsigned char *buf, *b;
  int draw_type;
  int * vals, val;
  int start, end;
  int x, x2, w;
  int i, j;

  if (!bezier_sel->closed)
    fatal_error ("tried to convert an open bezier curve");

  /* destroy previous mask */
  if (bezier_sel->mask)
    {
      channel_delete (bezier_sel->mask);
      bezier_sel->mask = NULL;
    }

  /* get the new mask's maximum extents */
  if (antialias)
    {
      buf = (unsigned char *) g_malloc (width);
      width = gdisp->gimage->width * SUPERSAMPLE;
      height = gdisp->gimage->height * SUPERSAMPLE;
      draw_type = AA_IMAGE_COORDS;
      /* allocate value array  */
      vals = (int *) g_malloc (sizeof (int) * width);
    }
  else
    {
      buf = NULL;
      width = gdisp->gimage->width;
      height = gdisp->gimage->height;
      draw_type = IMAGE_COORDS;
      vals = NULL;
    }

  /* create a new mask */
  bezier_sel->mask = channel_ref (channel_new_mask (gdisp->gimage->ID, 
						    gdisp->gimage->width,
						    gdisp->gimage->height));

  /* allocate room for the scanlines */
  bezier_sel->scanlines = g_malloc (sizeof (GSList *) * height);

  /* zero out the scanlines */
  for (i = 0; i < height; i++)
    bezier_sel->scanlines[i] = NULL;

  /* scan convert the curve */
  points = bezier_sel->points;
  start_pt = bezier_sel->points;
  start_convert = 1;

  do {
    bezier_draw_segment (bezier_sel, points,
			 subdivisions, draw_type,
			 bezier_convert_points);

    /*  advance to the next segment  */
    points = points->next;
    points = points->next;
    points = points->next;
  } while (points != start_pt);

  if (antialias)
    bezier_convert_line (bezier_sel->scanlines, lastx, lasty,
			 bezier_sel->points->x * SUPERSAMPLE,
			 bezier_sel->points->y * SUPERSAMPLE);
  else
    bezier_convert_line (bezier_sel->scanlines, lastx, lasty,
			 bezier_sel->points->x, bezier_sel->points->y);

  pixel_region_init (&maskPR, drawable_data (GIMP_DRAWABLE(bezier_sel->mask)), 
		     0, 0,
		     drawable_width (GIMP_DRAWABLE(bezier_sel->mask)),
		     drawable_height (GIMP_DRAWABLE(bezier_sel->mask)), TRUE);
  for (i = 0; i < height; i++)
    {
      list = bezier_sel->scanlines[i];

      /*  zero the vals array  */
      if (antialias && !(i % SUPERSAMPLE))
	memset (vals, 0, width * sizeof (int));

      while (list)
        {
          x = (long) list->data;
          list = list->next;
          if (!list)
	    warning ("cannot properly scanline convert bezier curve: %d", i);
          else
            {
	      /*  bounds checking  */
	      x = BOUNDS (x, 0, width);
	      x2 = BOUNDS ((long) list->data, 0, width);

	      w = x2 - x;

	      if (!antialias)
		channel_add_segment (bezier_sel->mask, x, i, w, 255);
	      else
		for (j = 0; j < w; j++)
		  vals[j + x] += 255;

              list = g_slist_next (list);
            }
        }

      if (antialias && !((i+1) % SUPERSAMPLE))
	{
	  b = buf;
	  start = 0;
	  end = width;
	  for (j = start; j < end; j += SUPERSAMPLE)
	    {
	      val = 0;
	      for (x = 0; x < SUPERSAMPLE; x++)
		val += vals[j + x];

	      *b++ = (unsigned char) (val / SUPERSAMPLE2);
	    }

	  pixel_region_set_row (&maskPR, 0, (i / SUPERSAMPLE), 
				drawable_width (GIMP_DRAWABLE(bezier_sel->mask)), buf);
	}

      g_slist_free (bezier_sel->scanlines[i]);
    }

  if (antialias)
    {
      g_free (vals);
      g_free (buf);
    }

  g_free (bezier_sel->scanlines);
  bezier_sel->scanlines = NULL;

  channel_invalidate_bounds (bezier_sel->mask);
}

static void
bezier_convert_points (BezierSelect *bezier_sel,
		       GdkPoint     *points,
		       int           npoints)
{
  int i;

  if (start_convert)
    start_convert = 0;
  else
    bezier_convert_line (bezier_sel->scanlines, lastx, lasty, points[0].x, points[0].y);

  for (i = 0; i < (npoints - 1); i++)
    {
      bezier_convert_line (bezier_sel->scanlines,
			   points[i].x, points[i].y,
			   points[i+1].x, points[i+1].y);
    }

  lastx = points[npoints-1].x;
  lasty = points[npoints-1].y;
}

static void
bezier_convert_line (GSList ** scanlines,
		     int       x1,
		     int       y1,
		     int       x2,
		     int       y2)
{
  int dx, dy;
  int error, inc;
  int tmp;
  float slope;

  if (y1 == y2)
    return;

  if (y1 > y2)
    {
      tmp = y2; y2 = y1; y1 = tmp;
      tmp = x2; x2 = x1; x1 = tmp;
    }

  if (y1 < 0)
    {
      if (y2 < 0)
	return;

      if (x2 == x1)
	{
	  y1 = 0;
	}
      else
	{
	  slope = (float) (y2 - y1) / (float) (x2 - x1);
	  x1 = x2 + (0 - y2) / slope;
	  y1 = 0;
	}
    }

  if (y2 >= height)
    {
      if (y1 >= height)
	return;

      if (x2 == x1)
	{
	  y2 = height;
	}
      else
	{
	  slope = (float) (y2 - y1) / (float) (x2 - x1);
	  x2 = x1 + (height - y1) / slope;
	  y2 = height;
	}
    }

  if (y1 == y2)
    return;

  dx = x2 - x1;
  dy = y2 - y1;

  scanlines = &scanlines[y1];

  if (((dx < 0) ? -dx : dx) > ((dy < 0) ? -dy : dy))
    {
      if (dx < 0)
        {
          inc = -1;
          dx = -dx;
        }
      else
        {
          inc = 1;
        }

      error = -dx /2;
      while (x1 != x2)
        {
          error += dy;
          if (error > 0)
            {
              error -= dx;
	      *scanlines = bezier_insert_in_list (*scanlines, x1);
	      scanlines++;
            }

          x1 += inc;
        }
    }
  else
    {
      error = -dy /2;
      if (dx < 0)
        {
          dx = -dx;
          inc = -1;
        }
      else
        {
          inc = 1;
        }

      while (y1++ < y2)
        {
	  *scanlines = bezier_insert_in_list (*scanlines, x1);
	  scanlines++;

          error += dx;
          if (error > 0)
            {
              error -= dy;
              x1 += inc;
            }
        }
    }
}

static GSList *
bezier_insert_in_list (GSList * list,
		       int      x)
{
  GSList * orig = list;
  GSList * rest;

  if (!list)
    return g_slist_prepend (list, (void *) ((long) x));

  while (list)
    {
      rest = g_slist_next (list);
      if (x < (long) list->data)
        {
          rest = g_slist_prepend (rest, list->data);
          list->next = rest;
          list->data = (void *) ((long) x);
          return orig;
        }
      else if (!rest)
        {
          g_slist_append (list, (void *) ((long) x));
          return orig;
        }
      list = g_slist_next (list);
    }

  return orig;
}
