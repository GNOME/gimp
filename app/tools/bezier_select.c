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
#include "config.h"

#include <string.h>

#include "appenv.h"
#include "cursorutil.h"
#include "draw_core.h"
#include "edit_selection.h"
#include "errors.h"
#include "gdisplay.h"
#include "gimage_mask.h"
#include "rect_select.h"
#include "bezier_select.h"
#include "bezier_selectP.h"
#include "paths_dialogP.h"
#include "selection_options.h"
#include "undo.h"

#include "libgimp/gimpmath.h"

#include "libgimp/gimpintl.h"

/* Bezier extensions made by Raphael FRANCOIS (fraph@ibm.net)

  BEZIER_EXTENDS VER 1.0 

  - work as the cut/copy/paste named functions.
  - allow to add/remove/replace bezier curves
  - allow to modify the control/anchor points even if the selection is made
  - allow to add/remove control/anchor points on a curve
  - allow to update a previous saved curve

  - cannot operate on open or multiple curves simultaneously
*/

#define BEZIER_DRAW_CURVE   1
#define BEZIER_DRAW_CURRENT 2
#define BEZIER_DRAW_HANDLES 4
#define BEZIER_DRAW_ALL     (BEZIER_DRAW_CURVE | BEZIER_DRAW_HANDLES)

#define BEZIER_WIDTH     8
#define BEZIER_HALFWIDTH 4


#define SUPERSAMPLE   3
#define SUPERSAMPLE2  9

#define NO  0
#define YES 1

/*  the bezier select structures  */

typedef double BezierMatrix[4][4];

/*  The named paste dialog  */
typedef struct _PasteNamedDlg PasteNamedDlg;
struct _PasteNamedDlg
{
  GtkWidget   *shell;
  GtkWidget   *list;
  GDisplay    *gdisp; 
};

/*  The named buffer structure...  */
typedef struct _named_buffer BezierNamedBuffer;
struct _named_buffer
{
  BezierSelect *sel;
  char *        name;
};


typedef struct {
  CountCurves  curve_count;         /* Must be the first element */
  gdouble *stroke_points;
  gint	   num_stroke_points;	/* num of valid points */
  gint	   len_stroke_points;	/* allocated length */
  void    *next_curve;          /* Next curve in list -- we draw all curves 
				 * separately.
				 */
} BezierRenderPnts;

typedef struct {
  CountCurves  curve_count;         /* Must be the first element */
  gboolean firstpnt;
  gdouble  curdist;
  gdouble  dist;
  gdouble  *gradient;
  gint     *x;
  gint     *y;
  gdouble  lastx;
  gdouble  lasty;
  gboolean found;
} BezierDistance;

typedef struct {
  CountCurves  curve_count;         /* Must be the first element */
  gint         x;
  gint         y;
  gint         halfwidth;
  gint         found;
} BezierCheckPnts;

/*  the bezier selection tool options  */ 
static SelectionOptions *bezier_options = NULL;

/*  local variables  */
static BezierMatrix basis =
{
  { -1,  3, -3,  1 },
  {  3, -6,  3,  0 },
  { -3,  3,  0,  0 },
  {  1,  0,  0,  0 },
};

/*
static BezierMatrix basis =
{
  { -1/6.0,  3/6.0, -3/6.0,  1/6.0 },
  {  3/6.0, -6/6.0,  3/6.0,  0 },
  { -3/6.0,  0,  3/6.0,  0 },
  {  1/6.0,  4/6.0,  1,  0 },
};
*/

/*  The named buffer list  */
GSList *bezier_named_buffers = NULL;

/*  Global Static Variable to maintain informations about the "contexte"  */
static BezierSelect * curSel;
static Tool * curTool;
static GDisplay * curGdisp;
static DrawCore * curCore;
static int ModeEdit = EXTEND_NEW;


/*  local function prototypes  */
static void  bezier_select_button_press    (Tool *, GdkEventButton *, gpointer);
static void  bezier_select_button_release  (Tool *, GdkEventButton *, gpointer);
static void  bezier_select_motion          (Tool *, GdkEventMotion *, gpointer);
static void  bezier_select_control         (Tool *, ToolAction,       gpointer);
static void  bezier_select_cursor_update   (Tool *, GdkEventMotion *, gpointer);
static void  bezier_select_draw            (Tool *);

static void  bezier_offset_point           (BezierPoint *, int, int);
static int   bezier_check_point            (BezierPoint *, int, int, int);
static void  bezier_draw_handles           (BezierSelect *, gint);
static void  bezier_draw_current           (BezierSelect *);
static void  bezier_draw_point             (BezierSelect *, BezierPoint *, int);
static void  bezier_draw_line              (BezierSelect *, BezierPoint *, BezierPoint *);
static void  bezier_draw_segment           (BezierSelect *, BezierPoint *, int, int, BezierPointsFunc,gpointer);
static void  bezier_draw_segment_points    (BezierSelect *, GdkPoint *, int, gpointer);
static void  bezier_compose                (BezierMatrix, BezierMatrix, BezierMatrix);

static void  bezier_convert                (BezierSelect *, GDisplay *, int, int);
static void  bezier_convert_points         (BezierSelect *, GdkPoint *, int, gpointer);
static void  bezier_convert_line           (GSList **, int, int, int, int);
static GSList * bezier_insert_in_list      (GSList *, int);

static int   test_add_point_on_segment     (BezierSelect *, BezierPoint *, int, int, int, int, int);
static void  bezier_to_sel_internal        (BezierSelect *, Tool *, GDisplay *, gint, gint);
static void  bezier_stack_points	   (BezierSelect *, GdkPoint *, int, gpointer);
static gboolean stroke_interpolatable    (int, int, int, int, gdouble);
static void bezier_stack_points_aux      (GdkPoint *, int, int, gdouble, BezierRenderPnts *);
static gint count_points_on_curve(BezierPoint *);


/*  functions  */

static void
bezier_select_options_reset ()
{
  selection_options_reset (bezier_options);
}

Tool*
tools_new_bezier_select ()
{
  Tool * tool;
  BezierSelect * private;

  /*  The tool options  */
  if (! bezier_options)
    {
      bezier_options =
	selection_options_new (BEZIER_SELECT, bezier_select_options_reset);
      tools_register (BEZIER_SELECT, (ToolOptions *) bezier_options);
    }


  tool = tools_new_tool (BEZIER_SELECT);
  private = g_new0 (BezierSelect, 1);

  private->num_points = 0;
  private->mask = NULL;
  private->core = draw_core_new (bezier_select_draw);
  bezier_select_reset (private);

  tool->scroll_lock = TRUE;   /*  Disallow scrolling  */
  tool->preserve    = FALSE;  /*  Don't preserve on drawable change  */

  tool->private = (void *) private;

  tool->button_press_func   = bezier_select_button_press;
  tool->button_release_func = bezier_select_button_release;
  tool->motion_func         = bezier_select_motion;
  tool->cursor_update_func  = bezier_select_cursor_update;
  tool->control_func        = bezier_select_control;

  curCore = private->core;
  curSel  = private;
  curTool = tool;

  paths_new_bezier_select_tool ();

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
  gimp_context_set_tool (gimp_context_get_user (), BEZIER_SELECT);
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

static BezierPoint *
valid_curve_segment(BezierPoint *points)
{
  /* Valid curve segment is made up of four points */
  if(points && 
     points->next && 
     points->next->next && 
     points->next->next->next )
    return(points);

  return (NULL);
}

static BezierPoint *
next_anchor(BezierPoint *points,BezierPoint **next_curve)
{
  int loop;

  *next_curve = NULL;
 
  if(!points)
    return (NULL);

  for(loop = 0; loop < 3; loop++)
  {
   	points = points->next; 
   	if (!points) 
	  return (NULL);
   	if(points->next_curve)
   	{
	  *next_curve = points->next_curve;
   	}
  }
  return valid_curve_segment(points);
}

void
bezier_draw_curve (BezierSelect    *bezier_sel,
		   BezierPointsFunc func, 
		   gint             coord,
		   gpointer         udata)
{
  BezierPoint * points;
  BezierPoint * start_pt;
  BezierPoint * next_curve;
  CountCurves * cnt = (CountCurves *)udata;
  gint point_counts = 0;

/*   printSel(bezier_sel); */

  if(cnt)
    cnt->count = 0;
  points = bezier_sel->points;
  start_pt = bezier_sel->points;

  if(bezier_sel->num_points >= 4)
    {
      do {
	point_counts = count_points_on_curve(points);
	if(point_counts >= 4)
	  {
	    do {
	      bezier_draw_segment (bezier_sel, points,
				   SUBDIVIDE, coord,
				   func,
				   udata);
	      
	      points = next_anchor(points,&next_curve);
	      /* 	  g_print ("next_anchor = %p\n",points); */
	    } while (points != start_pt && points);
	    if(cnt)
	      cnt->count++;
	    start_pt = next_curve;
	    points = next_curve;
	  }
	else
	  break; /* must be last curve since only this one is allowed < 4
		  * points.
		  */
      } while(next_curve);
    }
}

void
bezier_select_reset (BezierSelect *bezier_sel)
{
  BezierPoint * points;
  BezierPoint * start_pt;
  BezierPoint * temp_pt;
  BezierPoint * next_curve;

  if (bezier_sel->num_points > 0)
    {
      points = bezier_sel->points;
      start_pt = bezier_sel->points;

      do {
	do {
	  temp_pt = points;
	  next_curve = points->next_curve;
	  points = points->next;
	  if(points != start_pt && next_curve)
	    {
	      g_warning("Curve points out of sync");
	    }
	  g_free (temp_pt);
	} while (points != start_pt && points);
	points = next_curve;
	start_pt = next_curve;
      } while (next_curve);
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

void
bezier_select_free (BezierSelect *bezier_sel)
{
  bezier_select_reset (bezier_sel);
  g_free (bezier_sel);
}

/* Find the curve that points to this curve. This makes to_check point
 * the start of a curve. 
 */
static BezierPoint * 
check_for_next_curve(BezierSelect *bezier_sel,
		     BezierPoint  *to_check)
{
  BezierPoint *points = bezier_sel->points;
  gint num_points = bezier_sel->num_points;

  while (points && num_points)
   {
     if(points->next_curve == to_check)
       return (points);

     if(points->next_curve)
       points = points->next_curve;
     else
       points = points->next;
     num_points--;
   }
  return (NULL);
}

static gint 
count_points_on_curve(BezierPoint *points)
{
  BezierPoint *start = points;
  gint count = 0;
  
  while(points && points->next != start)
    {
      points = points->next;
      count++;
    }

  return (count);
}

/* Find the start of the last open curve, if curve already closed
 * this is an error..
 */

static BezierPoint *
find_start_open_curve(BezierSelect *bsel)
{
  BezierPoint *start_pnt = NULL;
  BezierPoint *this_pnt = bsel->last_point;

  /* Could be one of the first points */
  if(!bsel->last_point)
    {
      return NULL;
    }

  if(bsel->closed)
    {
      g_message(_("Bezier path already closed."));
      return NULL;
    }

  /* Step backwards until the prev point is null.
   * in this case this is the start of the open curve.
   * The start_pnt stuff is to stop us going around forever.
   * If we even have a closed curve then we are in seroius 
   * trouble.
   */

  while(this_pnt->prev && start_pnt != this_pnt)
    {
      if(!start_pnt)
	start_pnt = this_pnt;
      this_pnt = this_pnt->prev;
    }

  /* Must be an anchor to be the start */
  if(start_pnt == this_pnt || this_pnt->type != BEZIER_ANCHOR)
    {
      g_message(_("Corrupt curve"));
      return NULL;
    }

/*   g_print ("Returned start pnt of curve %p is %p\n",bsel->last_point,this_pnt); */
  return (this_pnt);
}

/* Delete a whole curve. Watch out for special cases.
 * start_pnt must always be the start point if the curve to delete.
 */

static void
delete_whole_curve(BezierSelect   *bezier_sel,
		   BezierPoint    *start_pnt)
{
  BezierPoint    *tmppnt;
  BezierPoint    *next_curve = NULL; /* Next curve this one 
				      * points at (if any) 
				      */
  BezierPoint    *prev_curve; /* Does a curve point to this one? */
  gint            cnt_pnts = 0; /* Count how many pnts deleted */
  gint            reset_last = FALSE;

  /* shift and del means delete whole curve */
  /* Three cases, this is first curve, middle curve 
   * or end curve.
   */
  
  /* Does this curve have another chained on the end? 
   * or is this curve pointed to another one?
   */

/*   g_print ("delete_whole_curve::\n"); */

  tmppnt = start_pnt;
  do {
    if(tmppnt->next_curve)
      {
	next_curve = tmppnt->next_curve;
	break;
      }
    tmppnt = tmppnt->next;
  } while (tmppnt != start_pnt && tmppnt);

  prev_curve = check_for_next_curve(bezier_sel,start_pnt);
  
  /* First curve ?*/
  if(bezier_sel->points == start_pnt)
    bezier_sel->points = next_curve;
  else 
    {
      /* better have a previous curve else how did we get here? */
      prev_curve->next_curve = next_curve;
    }
     
  /* start_pnt points to the curve we should free .. ignoring the next_curve */

  tmppnt = start_pnt;
  do {
    BezierPoint *fpnt;
    fpnt = tmppnt;
    tmppnt = tmppnt->next;
    if(fpnt == bezier_sel->last_point)
      reset_last = TRUE;
    g_free (fpnt);
    cnt_pnts++;
  } while (tmppnt != start_pnt && tmppnt);

  bezier_sel->num_points -= cnt_pnts;
  
  /* if deleted curve was unclosed then must have been the last curve 
   * and thus this curve becomes closed.
   */
  if(!tmppnt && bezier_sel->num_points > 0)
    {
      bezier_sel->closed = 1;
      bezier_sel->state = BEZIER_EDIT;
    }

  if(bezier_sel->num_points <= 0)
    {
      bezier_select_reset(bezier_sel);
    }

  bezier_sel->cur_anchor = NULL;
  bezier_sel->cur_control = NULL;

  /* The last point could have been on this curve as well... */
  if(reset_last)
    {
      BezierPoint *points = bezier_sel->points;
      BezierPoint *l_pnt = NULL;
      gint num_points = bezier_sel->num_points;
      
      while (points && num_points)
	{
	  l_pnt = points;
	  if(points->next_curve)
	    points = points->next_curve;
	  else
	    points = points->next;
	  num_points--;
	}
      bezier_sel->last_point = l_pnt;
    }
}

static gint
bezier_edit_point_on_curve(int             x,
			   int             y,
			   int             halfwidth,
			   GDisplay       *gdisp,
			   BezierSelect   *bezier_sel,
			   Tool           *tool,
			   GdkEventButton *bevent)
{
  gint grab_pointer = 0;
  BezierPoint  *next_curve;
  BezierPoint  *points = bezier_sel->points;
  BezierPoint  *start_pt = bezier_sel->points;
  BezierPoint  *last_curve = NULL;
  gint point_counts = 0;

  /* find if the button press occurred on a point */
  do {
    point_counts = count_points_on_curve(points);
    do {
      if (bezier_check_point (points, x, y, halfwidth))
	{
	  BezierPoint *finded=points;
	  BezierPoint *start_op; 
	  BezierPoint *end_op; 
	  
	  if (ModeEdit== EXTEND_REMOVE)
	    {

	      if((bevent->state & GDK_SHIFT_MASK) || (point_counts <= 7))
		{
		  /* Case 1: GDK_SHIFT_MASK - The user explicitly wishes the present 
                     curve to go away.
                     Case 2: The current implementation cannot cope with less than
                     7 points ie: 2 anchors points and 4 controls: the minimal closed curve.
                     Since the user wishes less than this implementation minimum,
                     we take this for an implicit wish that the entire curve go away.
                     G'bye dear curve. 
		  */

		  delete_whole_curve(bezier_sel,start_pt);
		  return (1);
		}
	      else if(!finded->prev || !finded->prev->prev) 
		{
		  /* This is the first point on the curve */
		  /* Clear current anchor and control */
		  bezier_sel->cur_anchor = NULL;
		  bezier_sel->cur_control = NULL;
		  /* Where are we?  reset to first point... */
		  /*if(last_curve == NULL)*/
		  if(start_pt == bezier_sel->points)
		    {
		      finded = bezier_sel->points;
		      bezier_sel->points = finded->next->next->next;
		      bezier_sel->points->prev = NULL;
		    }
		  else
		    {
 		      finded = last_curve->next_curve; 
 		      last_curve->next_curve = finded->next->next->next; 
 		      last_curve->next_curve->prev = NULL; 
		    }
		  
		  bezier_sel->num_points -= 3;
		  
		  g_free( finded->next->next );
		  g_free( finded->next );
		  g_free( finded );
		}
	      else if(!bezier_sel->closed && 
		      (finded == bezier_sel->last_point ||
		       finded == bezier_sel->last_point->prev || 
		       finded == bezier_sel->last_point->prev->prev))
		{
		  /* This is the last point on the curve */
		  /* Clear current anchor and control */
		  /* Where are we?  reset to last point... */
		  finded =  bezier_sel->last_point->prev;
		  bezier_sel->last_point = finded->prev->prev;
		  bezier_sel->last_point->next = NULL;
		  
		  bezier_sel->cur_anchor = NULL;
		  bezier_sel->cur_control = NULL;
		 

		  bezier_sel->num_points -= 3;
		  
		  g_free( finded->prev );
		  g_free( finded->next );
		  g_free( finded );
		}
	      else
		{
		  if ( finded->type == BEZIER_CONTROL )
		    {
		      if (finded->next->type == BEZIER_CONTROL)
			finded = finded->prev;
		      else
			finded = finded->next;
		    }
		  
		  start_op = finded->prev->prev;
		  end_op = finded->next->next;

		  /* we can use next_curve here since we are going to 
		   * drop out the bottom anyways.
		   */
		  next_curve = 
		    check_for_next_curve(bezier_sel,finded);

		  if(next_curve)
		    {
		      /* Deleteing first point of next curve*/
		      next_curve->next_curve = finded->prev->prev->prev;
		    }
		  else /* Can't be both first and a next curve!*/
		    {
		      if (bezier_sel->points == finded)
			{
/* 			  g_print ("Deleting first point %p\n",finded); */
			  bezier_sel->points = finded->prev->prev->prev;
			}
		    }

		  /* Make sure the chain of curves is preserved */
		  if(finded->prev->next_curve)
		    {
/* 		      g_print ("Moving curve on next_curve %p\n",finded->prev->next_curve); */
		      /* last point on closed multi-path */
		      finded->prev->prev->prev->prev->next_curve = finded->prev->next_curve;
		    }
		  
		  if(bezier_sel->last_point == finded->prev)
		    {
/* 		      g_print ("Deleting last point %p\n",finded->prev); */
		      bezier_sel->last_point = bezier_sel->last_point->prev->prev->prev;
		    }

		  start_op->next = end_op;
		  end_op->prev = start_op;

/* 		  if ( (bezier_sel->last_point == finded) ||  */
/* 		       (bezier_sel->last_point == finded->next) ||  */
/* 		       (bezier_sel->last_point  == finded->prev)) */
/* 		    { */
/* 		      bezier_sel->last_point = start_op->prev->prev; */
/* 		      bezier_sel->points = start_op->prev; */
/* 		    } */
		  
		  bezier_sel->num_points -= 3;
		  
		  g_free( finded->prev );
		  g_free( finded->next );
		  g_free( finded );
		  /* Clear current anchor and control */
		  bezier_sel->cur_anchor = NULL;
		  bezier_sel->cur_control = NULL;
		}
	    }
	  else
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
		  if (bezier_sel->cur_control->next &&
		      bezier_sel->cur_control->next->type == BEZIER_ANCHOR)
		    bezier_sel->cur_anchor = bezier_sel->cur_control->next;
		  else
		    bezier_sel->cur_anchor = bezier_sel->cur_control->prev;
		  break;
		}
	    }
	  return(1);
	}

      next_curve = points->next_curve;
      if(next_curve)
	last_curve = points;
      points = points->next;
    } while (points != start_pt && points);
    start_pt = next_curve;
    points = next_curve;
  } while (next_curve);
    
  return grab_pointer;
}

static gint
bezier_add_point_on_segment(int           x,
			    int           y,
			    int           halfwidth,
			    GDisplay     *gdisp,
			    BezierSelect *bezier_sel,
			    Tool         *tool)
{
  BezierPoint  *points = bezier_sel->points;
  BezierPoint  *start_pt = bezier_sel->points;
  BezierPoint  *next_curve;

  do {
    do {
      if (test_add_point_on_segment (bezier_sel,       
				     points,
				     SUBDIVIDE, 
				     IMAGE_COORDS,
				     x, y,
				     halfwidth))
	{
	  return 1;
	}
      
      points = points->next;
      
      if(!points)
	return 0;
      
      points = points->next;
      
      if(!points)
	return 0;
      
      next_curve = points->next_curve;
      points = points->next;
    } while (points != start_pt && points);
    start_pt = next_curve;
    points = next_curve;
  } while (next_curve);
  return 0;
}


static void
bezier_start_new_segment(BezierSelect *bezier_sel,gint x,gint y)
{
  /* Must be closed to do this! */
  if(!bezier_sel->closed)
    return;
  bezier_sel->closed = 0; /* End is no longer closed !*/
  bezier_sel->state = BEZIER_ADD;
  bezier_add_point (bezier_sel, BEZIER_MOVE, (gdouble)x, (gdouble)y);
  bezier_add_point (bezier_sel, BEZIER_CONTROL, (gdouble)x, (gdouble)y);
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
  BezierPoint *curve_start;
  int grab_pointer;
  int op, replace;
  int x, y;
  int halfwidth, dummy;

  gdisp = (GDisplay *) gdisp_ptr;

  tool->drawable = gimage_active_drawable (gdisp->gimage);

  bezier_sel = tool->private;
  grab_pointer = 0;

  if (bezier_options->extend)
    {
      tool->gdisp_ptr = gdisp_ptr;
    }
  else
    {
      /*  If the tool was being used in another image...reset it  */
      if (tool->state == ACTIVE && gdisp_ptr != tool->gdisp_ptr) {
        draw_core_stop(bezier_sel->core, tool);
        bezier_select_reset (bezier_sel);
      }
    }

  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y, &x, &y, TRUE, 0);

  /* get halfwidth in image coord */
  gdisplay_untransform_coords (gdisp, bevent->x + BEZIER_HALFWIDTH, 0, &halfwidth, &dummy, TRUE, 0);
  halfwidth -= x;

  curTool = active_tool;
  curSel = curTool->private;
  curGdisp = (GDisplay *) gdisp_ptr;
  active_tool->gdisp_ptr = gdisp_ptr;
  curCore = bezier_sel->core;

  switch (bezier_sel->state)
    {
    case BEZIER_START:
      if(ModeEdit != EXTEND_NEW)
	break;
      grab_pointer = 1;
      tool->state = ACTIVE;
      tool->gdisp_ptr = gdisp_ptr;

/*       if (bevent->state & GDK_MOD1_MASK) */
/* 	{ */
/* 	  init_edit_selection (tool, gdisp_ptr, bevent, EDIT_MASK_TRANSLATE); */
/* 	  break; */
/* 	} */
/*       else if (!(bevent->state & GDK_SHIFT_MASK) && !(bevent->state & GDK_CONTROL_MASK)) */
/* 	if (! (layer_is_floating_sel (gimage_get_active_layer (gdisp->gimage))) && */
/* 	    gdisplay_mask_value (gdisp, bevent->x, bevent->y) > HALF_WAY) */
/* 	  { */
/* 	    init_edit_selection (tool, gdisp_ptr, bevent, EDIT_MASK_TO_LAYER_TRANSLATE); */
/* 	    break; */
/* 	  } */

      bezier_sel->state = BEZIER_ADD;
      /*bezier_sel->draw = BEZIER_DRAW_CURVE; | BEZIER_DRAW_HANDLES;*/
      bezier_sel->draw = BEZIER_DRAW_CURRENT;

      bezier_add_point (bezier_sel, BEZIER_ANCHOR, (gdouble)x, (gdouble)y);
      bezier_add_point (bezier_sel, BEZIER_CONTROL, (gdouble)x, (gdouble)y);

      draw_core_start (bezier_sel->core, gdisp->canvas->window, tool);
      break;
    case BEZIER_ADD:

      grab_pointer = 1;

      if(ModeEdit == EXTEND_EDIT)
	{ 
	  /* erase the handles */
	  if(bezier_sel->closed)
	    bezier_sel->draw = BEZIER_DRAW_ALL;  
	  else
	    bezier_sel->draw = BEZIER_DRAW_CURVE;  
	  draw_core_pause (bezier_sel->core, tool);
	  /* unset the current anchor and control */
	  bezier_sel->cur_anchor = NULL;
	  bezier_sel->cur_control = NULL;

	  grab_pointer = bezier_edit_point_on_curve(x,y,halfwidth,gdisp,bezier_sel,tool,bevent);

	  bezier_sel->draw = BEZIER_DRAW_ALL;  
	  draw_core_resume (bezier_sel->core, tool);

	  if (grab_pointer)
	    gdk_pointer_grab (gdisp->canvas->window, FALSE,
			      GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			      NULL, NULL, bevent->time);
	  else
	    {
	      paths_dialog_set_default_op();
	      /* recursive call */
	      bezier_select_button_press(tool,bevent,gdisp_ptr);
	    }
	  return;
	}

      if(ModeEdit == EXTEND_REMOVE)
	{ 
/* 	  if(bezier_sel->num_points < 6) */
/* 	    return; */

	  /* erase the handles */
	  bezier_sel->draw = BEZIER_DRAW_ALL; 
	  draw_core_pause (bezier_sel->core, tool);
	  /* unset the current anchor and control */
	  bezier_sel->cur_anchor = NULL;
	  bezier_sel->cur_control = NULL;

	  /*kkk*/
	  bezier_sel->draw = BEZIER_DRAW_ALL; 
	  draw_core_resume (bezier_sel->core, tool);
	  bezier_sel->draw = BEZIER_DRAW_ALL; 
	  draw_core_pause (bezier_sel->core, tool);
	  /*kkk*/

	  grab_pointer = bezier_edit_point_on_curve(x,y,halfwidth,gdisp,bezier_sel,tool,bevent);

	  bezier_sel->draw = BEZIER_DRAW_ALL; 
	  draw_core_resume (bezier_sel->core, tool);
	  if(bezier_sel->num_points == 0)
	    {
	      draw_core_pause(bezier_sel->core, tool);
	      paths_dialog_set_default_op();
	    }

	  if(!grab_pointer)
	    {
	      paths_dialog_set_default_op();
	      /* recursive call */
	      bezier_select_button_press(tool,bevent,gdisp_ptr);
	    }
	  return;
	}

      if(ModeEdit == EXTEND_ADD)
	{
	  if(bezier_sel->num_points < 5)
	    return;
	  bezier_sel->draw = BEZIER_DRAW_ALL; 
	  draw_core_pause (bezier_sel->core, tool); 

	  grab_pointer = bezier_add_point_on_segment(x,y,halfwidth,gdisp,bezier_sel,tool);

	  bezier_sel->draw = BEZIER_DRAW_ALL;
	  draw_core_resume (bezier_sel->core, tool);
	  if (grab_pointer)
	    gdk_pointer_grab (gdisp->canvas->window, FALSE,
			      GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			      NULL, NULL, bevent->time);
	  else
	    {
	      paths_dialog_set_default_op();
	      /* recursive call */
	      bezier_select_button_press(tool,bevent,gdisp_ptr);
	    }
	  return;
	}

      if(bezier_sel->cur_anchor)
	{
	  if (bezier_check_point (bezier_sel->cur_anchor, x, y, halfwidth))
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
	}

      curve_start = find_start_open_curve(bezier_sel);

      if (curve_start && bezier_check_point (curve_start, x, y, halfwidth))
	{
	  bezier_sel->draw = BEZIER_DRAW_ALL;
	  draw_core_pause (bezier_sel->core, tool);

	  bezier_add_point (bezier_sel, BEZIER_CONTROL, (gdouble)x, (gdouble)y);
	  bezier_sel->last_point->next = curve_start;
	  curve_start->prev = bezier_sel->last_point;
 	  bezier_sel->cur_anchor = curve_start;
 	  bezier_sel->cur_control = curve_start->next;

	  bezier_sel->closed = 1;
	  bezier_sel->state = BEZIER_EDIT;
	  bezier_sel->draw = BEZIER_DRAW_ALL;

	  draw_core_resume (bezier_sel->core, tool);
	}
      else
	{
	  if(bezier_sel->cur_anchor)
	    bezier_sel->cur_anchor->pointflags = 1;
 	  bezier_sel->draw = BEZIER_DRAW_HANDLES; 
 	  draw_core_pause (bezier_sel->core, tool); 

	  bezier_add_point (bezier_sel, BEZIER_CONTROL, (gdouble)x, (gdouble)y);
	  bezier_add_point (bezier_sel, BEZIER_ANCHOR, (gdouble)x, (gdouble)y);
	  bezier_add_point (bezier_sel, BEZIER_CONTROL, (gdouble)x, (gdouble)y);

	  bezier_sel->draw = BEZIER_DRAW_CURRENT | BEZIER_DRAW_HANDLES;

	  draw_core_resume (bezier_sel->core, tool);
	}
      break;
    case BEZIER_EDIT:
      if (!bezier_sel->closed)
	gimp_fatal_error ("bezier_select_button_press(): Tried to edit on open bezier curve in edit selection");

      /* erase the handles */
      bezier_sel->draw = BEZIER_DRAW_ALL; 
      draw_core_pause (bezier_sel->core, tool);

      /* unset the current anchor and control */
      bezier_sel->cur_anchor = NULL;
      bezier_sel->cur_control = NULL;

      points = bezier_sel->points;
      start_pt = bezier_sel->points;

      if (ModeEdit == EXTEND_ADD)
	{
	  if (bezier_sel->closed)
	    {
	      grab_pointer = bezier_add_point_on_segment(x,y,halfwidth,gdisp,bezier_sel,tool);
	    }
	}
      else
	{
	  grab_pointer = bezier_edit_point_on_curve(x,y,halfwidth,gdisp,bezier_sel,tool,bevent);
	  if(grab_pointer && bezier_sel->num_points == 0)
	    {
	      draw_core_pause(bezier_sel->core, tool);
	      paths_dialog_set_default_op();
	    }
	}

      if (!grab_pointer && channel_value (bezier_sel->mask, x, y))
	{
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
	  bezier_to_sel_internal(bezier_sel,tool,gdisp,op,replace);
	  draw_core_resume (bezier_sel->core, tool);
	}
      else
	{
	  /* draw the handles */
	  if(!grab_pointer)
	    {
	      paths_dialog_set_default_op();
	      bezier_start_new_segment(bezier_sel,x,y);
	    }
 	  bezier_sel->draw = BEZIER_DRAW_ALL; 

	  draw_core_resume (bezier_sel->core, tool);
	}
      break;
    }

  if (grab_pointer)
    gdk_pointer_grab (gdisp->canvas->window, FALSE,
		      GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
		      NULL, NULL, bevent->time);

  /* Don't bother doing this if we don't have any points */
  if(bezier_sel->num_points > 0)
    paths_first_button_press(bezier_sel,gdisp);
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

  /* Here ?*/
  paths_newpoint_current(bezier_sel,gdisp);
}


/* unused
static void
bez_copy_points(BezierSelect *tobez,
		BezierSelect *frombez)
{
  BezierPoint *pts;
  gint i;
  BezierPoint  *bpnt = NULL;
  int need_move = 0;

  pts = (BezierPoint *) frombez->points;

  for (i=0; i< frombez->num_points; i++)
    {
      if(need_move)
	{
	  bezier_add_point( tobez, BEZIER_MOVE, pts->x, pts->y);
	  need_move = 0;
	}
      else
	bezier_add_point( tobez, pts->type, pts->x, pts->y);

      if(pts == frombez->cur_anchor)
	tobez->cur_anchor = tobez->last_point;
      else if(pts == frombez->cur_control)
	tobez->cur_control = tobez->last_point;

      if(bpnt == NULL)
	bpnt = tobez->last_point;

      if(pts->next_curve)
	{
	  tobez->last_point->next = bpnt;
	  bpnt->prev = tobez->last_point;
	  bpnt = NULL;
	  need_move = 1;
	  pts = pts->next_curve;
	}
      else
	{
	  pts = pts->next;
	}
    }
      
  if ( frombez->closed )
    {
      tobez->last_point->next = bpnt;
      bpnt->prev = tobez->last_point;
      tobez->closed = 1;
    }
}
*/

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

  if (mevent->state & GDK_MOD1_MASK)
    {
      bezier_sel->draw = BEZIER_DRAW_ALL;
    }
  else
    {
      bezier_sel->draw = BEZIER_DRAW_CURRENT | BEZIER_DRAW_HANDLES;
    }


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

  /* The Alt key is down... Move all the points of the bezier curve */

  if (mevent->state & GDK_MOD1_MASK)
    {
      BezierPoint *tmp = bezier_sel->points;
      gint num_points = bezier_sel->num_points;
      offsetx = x - lastx;
      offsety = y - lasty;

      if(mevent->state & GDK_SHIFT_MASK)
	{
	  /* Only move this curve */
	  BezierPoint *start_pt = bezier_sel->cur_anchor;

/* 	  g_print ("moving only one curve\n"); */

	  tmp = start_pt;
	  
	  do {
	    bezier_offset_point (tmp, offsetx, offsety);
	    tmp = tmp->next;
	  } while (tmp != start_pt && tmp);
	  /* Check if need to go backwards because curve is open */
	  if(!tmp && bezier_sel->cur_anchor->prev)
	    {
	      BezierPoint *start_pt = bezier_sel->cur_anchor->prev;
	      tmp = start_pt;
	      
	      do {
		bezier_offset_point (tmp, offsetx, offsety);
		tmp = tmp->prev;
	      } while (tmp != start_pt && tmp);
	    }
	}
      else
	{
	  while (tmp && num_points)
	    {
	      bezier_offset_point (tmp, offsetx, offsety);
	      if(tmp->next_curve)
		tmp = tmp->next_curve;
	      else
		tmp = tmp->next;
	      num_points--;
	    }
	}
    }
  else
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
	    gimp_fatal_error ("bezier_select_motion(): Encountered orphaned bezier control point");

	  if (opposite_control)
	    {
	      offsetx = bezier_sel->cur_control->x - anchor->x;
	      offsety = bezier_sel->cur_control->y - anchor->y;

	      opposite_control->x = anchor->x - offsetx;
	      opposite_control->y = anchor->y - offsety;
	    }
	}
    }

  /* As we're moving all the control points of the curve,
     we have to redraw all !!!
     */

  if ( mevent->state & GDK_MOD1_MASK)
    bezier_sel->draw = BEZIER_DRAW_ALL;
  else
    bezier_sel->draw = BEZIER_DRAW_CURRENT | BEZIER_DRAW_HANDLES;

  draw_core_resume (bezier_sel->core, tool);

  lastx = x;
  lasty = y;
}

/* returns 0 if not on control point, else BEZIER_ANCHOR or BEZIER_CONTROL */
static gint
bezier_on_control_point(GDisplay     *gdisp,
			BezierSelect *bezier_sel,
			gint         x,
			gint         y,
			gint         halfwidth)
{
  BezierPoint * points;
  gint num_points;

  /* transform the points from image space to screen space */
  points = bezier_sel->points;
  num_points = bezier_sel->num_points;

  while (points && num_points)
   {
     if(bezier_check_point(points,x,y,halfwidth))
       return points->type;

     if(points->next_curve)
       points = points->next_curve;
     else
       points = points->next;
     num_points--;
    }

  return FALSE;
}

static void
bezier_check_points (BezierSelect *bezier_sel,
		     GdkPoint     *points,
		     int           npoints,
		     gpointer      udata)
{
  gint loop;
  int l, r, t, b;
  BezierCheckPnts *chkpnts = udata;
  gint halfwidth = chkpnts->halfwidth;

  /* Quick exit if already found */

  if(chkpnts->found)
    return;

  for(loop = 0 ; loop < npoints; loop++)
    {
      l = points[loop].x - halfwidth;
      r = points[loop].x + halfwidth;
      t = points[loop].y - halfwidth;
      b = points[loop].y + halfwidth;

/*       g_print ("x,y = [%d,%d] halfwidth %d l,r,t,d [%d,%d,%d,%d]\n", */
/* 	     points[loop].x, */
/* 	     points[loop].y, */
/* 	     halfwidth, */
/* 	     l,r,t,b); */

      if ((chkpnts->x >= l) && 
	  (chkpnts->x <= r) && 
	  (chkpnts->y >= t) && 
	  (chkpnts->y <= b))
	{
	  chkpnts->found = TRUE;
	}
    }
}

static gboolean 
points_in_box(BezierPoint *points,
	      gint         x,
	      gint         y)
{
  /* below code adapted from Wm. Randolph Franklin <wrf@ecse.rpi.edu>
   */
  int i, j, c = 0;
  double yp[4],xp[4];

  for (i = 0; i < 4; i++)
    {
      xp[i] = points->x;
      yp[i] = points->y;
      points = points->next;
    }

  /* Check if straight line ..below don't work if it is! */
  if((xp[0] == xp[1] && yp[0] == yp[1]) || 
     (xp[2] == xp[3] && yp[0] == yp[1]))
    return TRUE;

  for (i = 0, j = 3; i < 4; j = i++) {
    if ((((yp[i]<=y) && (y<yp[j])) ||
	 ((yp[j]<=y) && (y<yp[i]))) &&
	(x < (xp[j] - xp[i]) * (y - yp[i]) / (yp[j] - yp[i]) + xp[i]))
      c = !c;
  }

  return c;
}

static gint
bezier_point_on_curve (GDisplay     *gdisp,
		       BezierSelect *bezier_sel,
		       gint          x,
		       gint          y,
		       gint          halfwidth)
{
  BezierCheckPnts chkpnts;
  BezierPoint * points;
  BezierPoint * start_pt;
  BezierPoint * next_curve;
  CountCurves * cnt = (CountCurves *)&chkpnts;
  gint point_counts = 0;

  chkpnts.x = x;
  chkpnts.y = y;
  chkpnts.halfwidth = halfwidth;
  chkpnts.found = FALSE;

  if (cnt)
    cnt->count = 0;
  points = bezier_sel->points;
  start_pt = bezier_sel->points;

  if (bezier_sel->num_points >= 4)
    {
      do
	{
	  point_counts = count_points_on_curve(points);
	  if (point_counts >= 4)
	    {
	      do
		{
		  if (points_in_box (points, x, y))
		    {
		      bezier_draw_segment (bezier_sel, points,
					   SUBDIVIDE, IMAGE_COORDS,
					   bezier_check_points,
					   &chkpnts);
		    }
		  points = next_anchor (points, &next_curve);
		  /* 	  g_print ("next_anchor = %p\n",points); */
		}
	      while (points != start_pt && points);
	      if (cnt)
		cnt->count++;
	      start_pt = next_curve;
	      points = next_curve;
	    }
	  else
	    break; /* must be last curve since only this one is allowed < 4
		    * points.
		    */
	}
      while (next_curve);
    }

  return chkpnts.found;
}

static void
bezier_select_cursor_update (Tool           *tool,
			     GdkEventMotion *mevent,
			     gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  BezierSelect * bezier_sel;
  gboolean on_curve;
  gboolean on_control_pnt;
  gboolean in_selection_area;
  gint halfwidth,dummy;
  gint x,y;

  gdisp = (GDisplay *) gdisp_ptr;

  bezier_sel = tool->private;

  if (gdisp != tool->gdisp_ptr || bezier_sel->core->draw_state == INVISIBLE)
    {
      gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_CURSOR,
				    BEZIER_SELECT,
				    CURSOR_MODIFIER_NONE,
				    FALSE);
      return;
    }

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &x, &y, TRUE, 0);

  /* get halfwidth in image coord */
  gdisplay_untransform_coords (gdisp, mevent->x + BEZIER_HALFWIDTH, 0,
			       &halfwidth, &dummy, TRUE, 0);
  halfwidth -= x;

  on_control_pnt = bezier_on_control_point (gdisp, bezier_sel, x, y, halfwidth);

  on_curve = bezier_point_on_curve (gdisp, bezier_sel, x, y, halfwidth);

  if (bezier_sel->mask && bezier_sel->closed &&
      channel_value(bezier_sel->mask, x, y) && 
      !on_control_pnt &&
      (!on_curve || ModeEdit != EXTEND_ADD))
    {
      in_selection_area = TRUE;
      if ((mevent->state & GDK_SHIFT_MASK) &&
	  !(mevent->state & GDK_CONTROL_MASK))
	{
	  gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_CURSOR,
					RECT_SELECT,
					CURSOR_MODIFIER_PLUS,
					FALSE);
	}
      else if ((mevent->state & GDK_CONTROL_MASK) &&
	       !(mevent->state & GDK_SHIFT_MASK))
	{
	  gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_CURSOR,
					RECT_SELECT,
					CURSOR_MODIFIER_MINUS,
					FALSE);
	}
      else if ((mevent->state & GDK_CONTROL_MASK) &&
	       (mevent->state & GDK_SHIFT_MASK))
	{
	  gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_CURSOR,
					RECT_SELECT,
					CURSOR_MODIFIER_INTERSECT,
					FALSE);
	}
      else
	{
	  gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_CURSOR,
					RECT_SELECT,
					CURSOR_MODIFIER_NONE,
					FALSE);
	}
      return;
    }

  if (mevent->state & GDK_MOD1_MASK)
    {
      /* Moving curve */
      if (mevent->state & GDK_SHIFT_MASK)
	{
	  /* moving on 1 curve */
	  gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_CURSOR,
					BEZIER_SELECT,
					CURSOR_MODIFIER_MOVE,
					FALSE);
	}
      else
	{
	  gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_CURSOR,
					BEZIER_SELECT,
					CURSOR_MODIFIER_MOVE,
					FALSE);
	}
    }
  else
    {
      switch (ModeEdit)
	{
	case EXTEND_NEW:
	  if (on_control_pnt && bezier_sel->closed)
	    {
	      gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_CURSOR,
					    BEZIER_SELECT,
					    CURSOR_MODIFIER_CONTROL,
					    FALSE);
/* 	      g_print ("add to curve cursor\n"); */
	    }
	  else if (on_curve)
	    {
	      gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_CURSOR,
					    BEZIER_SELECT,
					    CURSOR_MODIFIER_NONE,
					    FALSE);
/* 	      g_print ("edit control point cursor\n"); */
	    }
	  else
	    {
	      gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_CURSOR,
					    BEZIER_SELECT,
					    CURSOR_MODIFIER_NONE,
					    FALSE);
	    }
	  break;
	case EXTEND_ADD:
	  if (on_curve)
	    {
	      gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_CURSOR,
					    BEZIER_SELECT,
					    CURSOR_MODIFIER_PLUS,
					    FALSE);
/* 	      g_print ("add to curve cursor\n"); */
	    }
	  else
	    {
	      gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_CURSOR,
					    BEZIER_SELECT,
					    CURSOR_MODIFIER_NONE,
					    FALSE);
/* 	      g_print ("default no action cursor\n"); */
	    }
	  break;
	case EXTEND_EDIT:
	  if (on_control_pnt)
	    {
	      gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_CURSOR,
					    BEZIER_SELECT,
					    CURSOR_MODIFIER_CONTROL,
					    FALSE);
/* 	      g_print ("edit control point cursor\n"); */
	    }
	  else
	    {
	      gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_CURSOR,
					    BEZIER_SELECT,
					    CURSOR_MODIFIER_NONE,
					    FALSE);
/* 	      g_print ("default no action cursor\n"); */
	    }
	  break;
	case EXTEND_REMOVE:
	  if (on_control_pnt && mevent->state & GDK_SHIFT_MASK)
	    {
	      gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_CURSOR,
					    BEZIER_SELECT,
					    CURSOR_MODIFIER_MINUS,
					    FALSE);
/*            g_print ("delete whole curve cursor\n"); */
	    }
	  else if (on_control_pnt)
	    {
	      gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_CURSOR,
					    BEZIER_SELECT,
					    CURSOR_MODIFIER_MINUS,
					    FALSE);
/* 	      g_print ("remove point cursor\n"); */
	    }
	  else
	    {
	      gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_CURSOR,
					    BEZIER_SELECT,
					    CURSOR_MODIFIER_NONE,
					    FALSE);
/* 	      g_print ("default no action cursor\n"); */
	    }
	  break;
	default:
	  g_print ("In default\n");
	  gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_CURSOR,
					BEZIER_SELECT,
					CURSOR_MODIFIER_NONE,
					FALSE);
	  break;
	}
    }
}

static void
bezier_select_control (Tool       *tool,
		       ToolAction  action,
		       gpointer    gdisp_ptr)
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

    default:
      break;
    }
}

void 
bezier_draw(GDisplay * gdisp,BezierSelect * bezier_sel)
{
  BezierPoint * points;
  int num_points;
  int draw_curve;
  int draw_handles;
  int draw_current;


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
      if(points->next_curve)
	points = points->next_curve;
      else
	points = points->next;
      num_points--;
    }

  if (draw_curve)
    {
      bezier_draw_curve (bezier_sel,bezier_draw_segment_points,SCREEN_COORDS,NULL);
      bezier_draw_handles (bezier_sel,1);
    }
  else if (draw_current)
    {
      bezier_draw_current (bezier_sel);
      bezier_draw_handles (bezier_sel,0);
    }
  else if (draw_handles) 
    {
      bezier_draw_handles (bezier_sel,0); 
    }

}

static void
bezier_select_draw (Tool *tool)
{
  GDisplay * gdisp;
  BezierSelect * bezier_sel;

  gdisp = tool->gdisp_ptr;
  bezier_sel = tool->private;

  bezier_draw(gdisp,bezier_sel);
}



void
bezier_add_point (BezierSelect *bezier_sel,
		  int           type,
		  gdouble       x,
		  gdouble       y)
{
  BezierPoint *newpt;

  newpt = g_new0 (BezierPoint,1);

  newpt->type = type;
  newpt->x = x;
  newpt->y = y;
  newpt->next = NULL;
  newpt->prev = NULL;
  newpt->next_curve = NULL;

  if(type == BEZIER_MOVE &&
     bezier_sel->last_point)
    {
/*       g_print ("Adding move point\n"); */
      newpt->type = BEZIER_ANCHOR;
      bezier_sel->last_point->next_curve = newpt;
      bezier_sel->last_point = newpt;
      bezier_sel->cur_anchor = newpt;
    }
  else
    {
      if(type == BEZIER_MOVE) 
	{
	  newpt->type = BEZIER_ANCHOR;
/* 	  g_print ("Adding MOVE point to null curve\n"); */
	}

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
bezier_draw_handles (BezierSelect *bezier_sel,gint doAll)
{
  BezierPoint * points;
  int num_points;

  points = bezier_sel->points;
  num_points = bezier_sel->num_points;

  if (num_points <= 0)
    return;

  while (num_points && num_points > 0)
  {
      if (points == bezier_sel->cur_anchor)
	{
/* 	g_print ("bezier_draw_handles:: found cur_anchor %p\n",points); */
	  bezier_draw_point (bezier_sel, points, 0);
	  bezier_draw_point (bezier_sel, points->next, 0);
	  bezier_draw_point (bezier_sel, points->prev, 0);
	  bezier_draw_line (bezier_sel, points, points->next);
	  bezier_draw_line (bezier_sel, points, points->prev);
	}
      else
	{
/* 	g_print ("bezier_draw_handles:: not found cur_anchor %p\n",points); */
	  if(doAll || points->pointflags == 1)
	    {
	      bezier_draw_point (bezier_sel, points, 1);
	      points->pointflags = 0;
	    }
	}

      if(points)
      {
        if(points->next_curve)
          points = points->next_curve;
        else
          points = points->next;
      }

      if(points)
      {
        if(points->next_curve)
          points = points->next_curve;
        else
          points = points->next;
      }

      if(points)
      {
        if(points->next_curve)
          points = points->next_curve;
        else
          points = points->next;
      }

      num_points -= 3;
   }
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
			 bezier_draw_segment_points,
			 NULL);

  if (points != bezier_sel->cur_anchor)
    {
      points = bezier_sel->cur_anchor;

      if (points) points = points->next;
      if (points) points = points->next;
      if (points) points = points->next;

      if (points)
	bezier_draw_segment (bezier_sel, bezier_sel->cur_anchor,
			     SUBDIVIDE, SCREEN_COORDS,
			     bezier_draw_segment_points,
			     NULL);
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
		     BezierPointsFunc  points_func,
		     gpointer          udata)
{
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
	gimp_fatal_error ("bezier_draw_segment(): Bad bezier segment");

      switch (space)
	{
	case IMAGE_COORDS:
	  geometry[i][0] = points->x;
	  geometry[i][1] = points->y;
	  break;
	case AA_IMAGE_COORDS:
	  geometry[i][0] = (points->x * SUPERSAMPLE); 
	  geometry[i][1] = (points->y * SUPERSAMPLE); 
	  break;
	case SCREEN_COORDS:
	  geometry[i][0] = points->sx;
	  geometry[i][1] = points->sy;
	  break;
	default:
	  gimp_fatal_error ("bezier_draw_segment(): Unknown coordinate space: %d", space);
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

  gdk_points[0].x = (lastx); 
  gdk_points[0].y = (lasty); 
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
	  gdk_points[index].x = (newx); 
	  gdk_points[index].y = (newy); 
	  index++;

	  /* if the point buffer is full put it to the screen and zero it out */
	  if (index >= npoints)
	    {
	      (* points_func) (bezier_sel, gdk_points, index, udata);
	      index = 0;
	    }
	}

      lastx = newx;
      lasty = newy;
    }

  /* if there are points in the buffer, then put them on the screen */
  if (index)
    (* points_func) (bezier_sel, gdk_points, index, udata);
}

static void
bezier_draw_segment_points (BezierSelect *bezier_sel,
			    GdkPoint     *points,
			    int           npoints,
			    gpointer      udata)
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
  BezierPoint * next_curve;
  GSList * list;
  unsigned char *buf, *b;
  int draw_type;
  int * vals, val;
  int start, end;
  int x, x2, w;
  int i, j;

  if (!bezier_sel->closed)
    gimp_fatal_error ("bezier_convert(): tried to convert an open bezier curve");

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
  bezier_sel->mask = channel_ref (channel_new_mask (gdisp->gimage, 
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

  do {
    start_convert = 1;
    do {
      bezier_draw_segment (bezier_sel, points,
			   subdivisions, draw_type,
			   bezier_convert_points,
			   NULL);

      /*  advance to the next segment  */
      points = next_anchor(points,&next_curve);
    } while (points != start_pt && points);
    start_pt = next_curve;
    points = next_curve;
  } while (next_curve);

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
/*
          if (!list)
	    g_message ("cannot properly scanline convert bezier curve: %d", i);
          else
*/
            {
	      /*  bounds checking  */
	      x = CLAMP (x, 0, width);
	      x2 = CLAMP ((long) list->data, 0, width);

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
		       int           npoints,
		       gpointer      udata)
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
  int    dx, dy; 
  int    error, inc; 
  int    tmp;
  double slope;

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
	  slope = (double) (y2 - y1) / (double) (x2 - x1); 
	  x1 = x2 + (int)(0.5 + (double)(0 - y2) / slope);
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
	  slope = (double) (y2 - y1) / (double) (x2 - x1); 
	  x2 = x1 + (int)(0.5 + (double)(height - y1) / slope);
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

gboolean 
bezier_tool_selected()
{
  return(active_tool &&
	 active_tool->type == BEZIER_SELECT &&
	 active_tool->state == ACTIVE);
}

void
bezier_paste_bezierselect_to_current(GDisplay *gdisp,BezierSelect *bsel)
{
  BezierPoint *pts;
  gint i;
  Tool * tool;
  BezierPoint  *bpnt = NULL;
  int need_move = 0;

/*   g_print ("bezier_paste_bezierselect_to_current::\n"); */
/*   printSel(bsel); */

  /*  If the tool was being used before clear it */
  if (active_tool &&
      active_tool->type == BEZIER_SELECT &&
      active_tool->state == ACTIVE)
    {
      BezierSelect *bezier_sel = (BezierSelect*)active_tool->private;
      if(bezier_sel)
	{
	  draw_core_stop ( curSel->core, active_tool );
	  bezier_select_reset (bezier_sel);
	}
    }

  gimp_context_set_tool (gimp_context_get_user (), BEZIER_SELECT);
  active_tool->paused_count = 0;
  active_tool->gdisp_ptr = gdisp;
  active_tool->drawable = gimage_active_drawable (gdisp->gimage);  

  tool = active_tool;

  bezier_select_reset( curSel );

  draw_core_start (curSel->core, gdisp->canvas->window, tool);

  tool->state = ACTIVE;

  pts = (BezierPoint *) bsel->points;

  for (i=0; i< bsel->num_points; i++)
    {
      if(need_move)
	{
	  bezier_add_point( curSel, BEZIER_MOVE, pts->x, pts->y);
	  need_move = 0;
	}
      else
	bezier_add_point( curSel, pts->type, pts->x, pts->y);

      if(bpnt == NULL)
	bpnt = curSel->last_point;

      if(pts->next_curve)
	{
/* 	  g_print ("bezier_paste_bezierselect_to_current:: Close last curve off \n"); */
	  curSel->last_point->next = bpnt;
	  bpnt->prev = curSel->last_point;
	  curSel->cur_anchor = NULL;
	  curSel->cur_control = NULL;
	  bpnt = NULL;
	  need_move = 1;
	  pts = pts->next_curve;
	}
      else
	{
	  pts = pts->next;
	}
    }
      
  if ( bsel->closed )
    {
      curSel->last_point->next = bpnt;
      bpnt->prev = curSel->last_point;
      curSel->cur_anchor = curSel->points;
      curSel->cur_control = curSel->points->next;
      curSel->closed = 1;
      if (curTool->gdisp_ptr)
	bezier_convert(curSel, curTool->gdisp_ptr, SUBDIVIDE, NO);
    }

/*   g_print ("After pasting...\n"); */
/*   printSel(curSel); */

  if(bsel->num_points == 0)
    {
      curSel->state = BEZIER_START;
      curSel->draw = 0;
      draw_core_stop( curSel->core, curTool);
    }
  else
    {
      curSel->state = bsel->state;
      curSel->draw = BEZIER_DRAW_ALL;
      draw_core_resume( curSel->core, curTool);
    }
}

static void
bezier_to_sel_internal(BezierSelect  *bezier_sel,
		       Tool          *tool,
		       GDisplay      *gdisp,
		       gint           op, 
		       gint           replace)
{
  /*  If we're antialiased, then recompute the
   *  mask...
   */
  if (bezier_options->antialias)
    bezier_convert (bezier_sel, tool->gdisp_ptr, SUBDIVIDE, YES);
  
/*   if (!bezier_options->extend) */
/*     { */
/*       tool->state = INACTIVE; */
/*       bezier_sel->draw = BEZIER_DRAW_CURVE; */
/*       draw_core_resume (bezier_sel->core, tool); */
      
/*       bezier_sel->draw = 0; */
/*       draw_core_stop (bezier_sel->core, tool); */
/*     } */

   if (replace)
    gimage_mask_clear (gdisp->gimage);
  else
    gimage_mask_undo (gdisp->gimage);
  
  if (bezier_options->feather)
    channel_feather (bezier_sel->mask,
		     gimage_get_mask (gdisp->gimage),
		     bezier_options->feather_radius, 
		     bezier_options->feather_radius, 
		     op, 0, 0);
  else
    channel_combine_mask (gimage_get_mask (gdisp->gimage),
			  bezier_sel->mask, op, 0, 0);
  
  /*  show selection on all views  */
  gdisplays_flush ();
}

static int
test_add_point_on_segment (BezierSelect     *bezier_sel,
			    BezierPoint      *pt,
			    int               subdivisions,
			    int               space,
			    int xpos, 
			    int ypos, 
			    int halfwidth)

{
  BezierPoint *points;
  BezierMatrix geometry;
  BezierMatrix tmp1, tmp2;
  BezierMatrix deltas;
  double x, dx, dx2, dx3;
  double y, dy, dy2, dy3;
  double d, d2, d3;
  int lastx, lasty;
  int newx, newy;
  int i;
  double ratio;

  /* construct the geometry matrix from the segment */
  /* assumes that a valid segment containing 4 points is passed in */
  /* ALT ignore invalid segments since we might be working on an open curve */

  points = pt;

  ratio = -1.0;

  for (i = 0; i < 4; i++)
    {
      if (!points)
	return 0;

      switch (space)
	{
	case IMAGE_COORDS:
	  geometry[i][0] = points->x;
	  geometry[i][1] = points->y;
	  break;
	case AA_IMAGE_COORDS:
	  geometry[i][0] = RINT(points->x * SUPERSAMPLE);
	  geometry[i][1] = RINT(points->y * SUPERSAMPLE);
	  break;
	case SCREEN_COORDS:
	  geometry[i][0] = points->sx;
	  geometry[i][1] = points->sy;
	  break;
	default:
	  gimp_fatal_error ("test_add_point_on_segment(): Unknown coordinate space: %d", space);
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

      /* if this point is different than the last one...then test it */
      if ((lastx != newx) || (lasty != newy))
	{
	  int l, r, b, t;
	  
	  l = newx - halfwidth;
	  r = newx + halfwidth;
	  t = newy - halfwidth;
	  b = newy + halfwidth;
	  
	  if  ((xpos >= l) && (xpos <= r) && (ypos >= t) && (ypos <= b))
	    {
	      /* so we found one point in the square hit */
	      
	      ratio = (double)i/(double)subdivisions;

	      /* We found the exact point on the curve, so take it ...*/

	      if ((xpos==newx) && (ypos==newy)) break;

	      /* to Implement :

		 keep each time the nearest point of the curve from where we've clicked
		 in the case where we haven't click exactely on the curve.
	      */

	    }
	}
      lastx = newx;
      lasty = newy;
    }

  /* we found a point on the curve */

  if (ratio >= 0.0)
    {
      BezierPoint *pts, *pt1, *pt2, *pt3;
      BezierPoint *P00, *P01, *P02, *P03;
      BezierPoint P10, P11, P12;
      BezierPoint P20, P21;
      BezierPoint P30;

      pts = pt;

      P00 = pts;
      pts = pts->next;
      P01 = pts;
      pts = pts->next;
      P02 = pts;
      pts = pts->next;
      P03 = pts;

      /* De Casteljau algorithme 
	 [Advanced Animation & Randering Technics / Alan & Mark WATT]
	 [ADDISON WESLEY ref 54412]
	 Iteratif way of drawing a Bezier curve by geometrical approch 
	 
	 P0x represent the four controls points ( anchor / control /control /anchor ) 
	 P30 represent the new anchor point to add on the curve 
	 P2x represent the new control points of P30
	 P1x represent the new values of the control points P01 and P02

	 so if we moves ratio from 0 to 1 we draw the all curve between P00 and P03
      */
	 
      P10.x = (int)((1-ratio)*P00->x + ratio * P01->x);
      P10.y = (int)((1-ratio)*P00->y + ratio * P01->y);

      P11.x = (1-ratio)*P01->x + ratio * P02->x;
      P11.y = (1-ratio)*P01->y + ratio * P02->y;

      P12.x = (1-ratio)*P02->x + ratio * P03->x;
      P12.y = (1-ratio)*P02->y + ratio * P03->y;

      P20.x = (1-ratio)*P10.x + ratio * P11.x;
      P20.y = (1-ratio)*P10.y + ratio * P11.y;

      P21.x = (1-ratio)*P11.x + ratio * P12.x;
      P21.y = (1-ratio)*P11.y + ratio * P12.y;

      P30.x = (1-ratio)*P20.x + ratio * P21.x;
      P30.y = (1-ratio)*P20.y + ratio * P21.y;
      
      P01->x = P10.x;
      P01->y = P10.y;

      P02->x = P12.x;
      P02->y = P12.y;

      /* All the computes are done, let's insert the new point on the curve */

      pt1 = g_new0(BezierPoint,1);
      pt2 = g_new0(BezierPoint,1);
      pt3 = g_new0(BezierPoint,1);
      
      pt1->type = BEZIER_CONTROL;
      pt2->type = BEZIER_ANCHOR;
      pt3->type = BEZIER_CONTROL;
      
      pt1->x = P20.x; pt1->y = P20.y;    
      pt2->x = P30.x; pt2->y = P30.y;
      pt3->x = P21.x; pt3->y = P21.y;
      
      pt3->next_curve = P01->next_curve;
      P01->next  = pt1;
      
      pt1->prev = P01;
      pt1->next = pt2;
      
      pt2->prev = pt1;
      pt2->next = pt3;
      
      pt3->prev = pt2; 
      pt3->next = P02;
      
      P02->prev = pt3;
      
      bezier_sel->num_points += 3;

      bezier_sel->cur_anchor = pt2;
      bezier_sel->cur_control = pt1;

      return 1;
    }

  return 0;
  
}

void bezier_select_mode(gint mode)
{
  ModeEdit = mode;
}

/* The curve has to be closed to do a selection. */

void
bezier_to_selection(BezierSelect *bezier_sel,
		    GDisplay     *gdisp)
{
  /* Call the internal function */
  if(!bezier_sel->closed)
    {
      g_message(_("Curve not closed!"));
      return;
    }

  /* force the passed selection to be the current selection..*/
  /* This loads it into curSel for this image */
  bezier_paste_bezierselect_to_current(gdisp,bezier_sel);
  bezier_to_sel_internal(curSel,curTool,gdisp,ADD,1);
}

void printSel( BezierSelect *sel)
{
  BezierPoint *pt;
  BezierPoint *start_pt;
  int i;

  pt = sel->points;
  start_pt = pt;

  g_print ("\n");

  for(i=0; i<sel->num_points; i++)
    {
      g_print ("%d(%p) x,y=%f,%f type=%d next=%p prev=%p next_curve=%p\n", i,pt, pt->x, pt->y,pt->type,pt->next,pt->prev,pt->next_curve);
      if(pt->next != start_pt && pt->next_curve)
	g_print ("Curve out a sync!!\n");
      if(pt->next_curve)
	{
	  pt = pt->next_curve;
	  start_pt = pt;
	}
      else
	pt = pt->next;
    }

  g_print ("core : %p\n", sel->core);
  g_print ("closed : %d\n", sel->closed);
  g_print ("draw : %d\n", sel->draw);
  g_print ("state: %d\n", sel->state);
}

/* check whether vectors (offx, offy), (l_offx, l_offy) have the same angle. */
static gboolean
stroke_interpolatable (int offx, int offy, int l_offx, int l_offy, gdouble error)
{
  if      ((offx == l_offx) & (offy == l_offy))
    return TRUE;
  else if ((offx == 0) | (l_offx == 0))
    if (offx == l_offx)
      return ((0 <= offy) & (0 <= offy)) | ((offy < 0) & (l_offy < 0));
    else
      return FALSE;
  else if ((offy == 0) | (l_offy == 0))
    if (offy == l_offy)
      return ((0 < offx) & (0 < l_offx)) | ((offx < 0) & (l_offx < 0));
    else
      return FALSE;
  /* At this point, we can assert: offx, offy, l_offx, l_offy != 0 */
  else if (((0 < offx) & (0 < l_offx)) | ((offx < 0) & (l_offx < 0)))
    {
      gdouble	grad1, grad2;

      if (ABS (offy) < ABS (offx))
	{
	  grad1 = (gdouble) offy / (gdouble) offx;
	  grad2 = (gdouble) l_offy / (gdouble) l_offx;
	}
      else
	{
	  grad1 = (gdouble) offx / (gdouble) offy;
	  grad2 = (gdouble) l_offx / (gdouble) l_offy;
	}
      /* printf ("error: %f / %f\n", ABS (grad1 - grad2), error); */
      return (ABS (grad1 - grad2) <= error);
    }
  else
    return FALSE;
}

static void
bezier_stack_points_aux (GdkPoint *points,
			 int      start,
			 int	  end,
			 gdouble  error,
			 BezierRenderPnts *rpnts)
{
  const gint	expand_size = 32;
  gint		med;
  gint		offx, offy, l_offx, l_offy;

  if (rpnts->stroke_points == NULL)
    return;

  /* BASE CASE: stack the end point */
  if (end - start <= 1)
    {
      if ((rpnts->stroke_points[rpnts->num_stroke_points * 2 - 2] == points[end].x)
	  && (rpnts->stroke_points[rpnts->num_stroke_points * 2 - 1] == points[end].y))
	return;

      rpnts->num_stroke_points++;
      if (rpnts->len_stroke_points <= rpnts->num_stroke_points)
	{
	  rpnts->len_stroke_points += expand_size;
	  rpnts->stroke_points = g_renew (double, rpnts->stroke_points, 2 * rpnts->len_stroke_points);
	  if (rpnts->stroke_points == NULL)
	    {
	      rpnts->len_stroke_points = rpnts->num_stroke_points = 0;
	      return;
	    }
	}
      rpnts->stroke_points[rpnts->num_stroke_points * 2 - 2] = points[end].x;
      rpnts->stroke_points[rpnts->num_stroke_points * 2 - 1] = points[end].y;
      return;
    }

  if (end - start <= 32)
    {
      gint i;

      for (i = start+ 1; i <= end; i++)
	bezier_stack_points_aux (points, i, i, 0,rpnts);
      return;
    }
  /* Otherwise, check whether to divide the segment recursively */
  offx = points[end].x - points[start].x;
  offy = points[end].y - points[start].y;
  med = (end + start) / 2;

  l_offx = points[med].x - points[start].x;
  l_offy = points[med].y - points[start].y;

  if (! stroke_interpolatable (offx, offy, l_offx, l_offy, error))
    {
      bezier_stack_points_aux (points, start, med, error, rpnts);
      bezier_stack_points_aux (points, med, end, error, rpnts);
      return;
    }

  l_offx = points[end].x - points[med].x;
  l_offy = points[end].y - points[med].y;

  if (! stroke_interpolatable (offx, offy, l_offx, l_offy, error))
    {
      bezier_stack_points_aux (points, start, med, error, rpnts);
      bezier_stack_points_aux (points, med, end, error, rpnts);
      return;
    }
  /* Now, the curve can be represented by a points pair: (start, end).
     So, add the last point to stroke_points. */
  bezier_stack_points_aux (points, end, end, 0, rpnts);
}

static void
bezier_stack_points (BezierSelect *bezier_sel,
		     GdkPoint     *points,
		     int           npoints,
		     gpointer      udata)
{
  gint	i;
  gint	expand_size = 32;
  gint	minx, maxx, miny, maxy;
  gdouble error;
  BezierRenderPnts *rpnts = udata;

  if (npoints < 2)		/* Does this happen? */
    return;

  if (rpnts->stroke_points == NULL)	/*  initialize it here */
    {
      rpnts->num_stroke_points = 0;
      rpnts->len_stroke_points = expand_size;
      rpnts->stroke_points = g_new (double, 2 * rpnts->len_stroke_points);
    }

  maxx = minx = points[0].x;
  maxy = miny = points[0].y;
  for (i = 1; i < npoints; i++)
    {
      if (points[i].x < minx)
	minx = points[i].x;
      else if (maxx < points[i].x)
	maxx = points[i].x;
      if (points[i].y < miny)
	miny = points[i].x;
      else if (maxy < points[i].y)
	maxy = points[i].y;
    }
  /* allow one pixel fluctuation */
  error = 2.0 / MAX(maxx - minx, maxy - miny);
  error = 0.0; /* ALT */

  /* add the start point */
  bezier_stack_points_aux (points, 0, 0, 0, rpnts);

  /* divide segments recursively */
  bezier_stack_points_aux (points, 0, npoints - 1, error, rpnts);

  /* printf ("npoints: %d\n", npoints); */
}

static gint
bezier_gen_points(BezierSelect     *bezier_sel,
		  int	            open_path,
		  BezierRenderPnts *rpnts)
{
  BezierPoint * points;
  BezierPoint * start_pt;
  BezierPoint * next_curve;
  BezierRenderPnts *next_rpnts = rpnts;
  gint point_counts = 0; 

  /* stack points */
  points = bezier_sel->points;
  start_pt = bezier_sel->points;
      
  if(bezier_sel->num_points >= 4)
    {
      do {
 	point_counts = count_points_on_curve(points); 
	if(point_counts < 4)
	  return(TRUE);
	do {
	  bezier_draw_segment (bezier_sel, points,
			       SUBDIVIDE, AA_IMAGE_COORDS,
			       bezier_stack_points,
			       (gpointer)next_rpnts);
	  
	  points = next_anchor(points,&next_curve);
	} while (points != start_pt && points);
	start_pt = next_curve;
	points = next_curve;
	if(next_curve)
	  {
	    next_rpnts->next_curve = g_new0(BezierRenderPnts,1);
	    next_rpnts = next_rpnts->next_curve;
	  }
      } while (next_curve);
    }
  
  return (TRUE);
}

void
bezier_stroke (BezierSelect *bezier_sel,
	       GDisplay     *gdisp,
	       int          subdivisions,
	       int	    open_path)
{
  Argument *return_vals;
  int nreturn_vals;
  BezierRenderPnts *next_rpnts;
  BezierRenderPnts *rpnts = g_new0(BezierRenderPnts,1);

  /*  Start an undo group  */
  undo_push_group_start (gdisp->gimage, PAINT_CORE_UNDO);

  bezier_gen_points(bezier_sel,open_path,rpnts);
  do
    {
  if (rpnts->stroke_points)
    {
      GimpDrawable *drawable;
      int offset_x, offset_y;
      gdouble *ptr;
    
      drawable = gimage_active_drawable (gdisp->gimage);
      gimp_drawable_offsets (drawable, &offset_x, &offset_y);

      ptr = rpnts->stroke_points;
      while (ptr < rpnts->stroke_points + (rpnts->num_stroke_points * 2))
	{
	  *ptr /= SUPERSAMPLE;
	  *ptr++ -= offset_x;
	  *ptr /= SUPERSAMPLE;
	  *ptr++ -= offset_y;
	}

      /* Stroke with the correct tool */
      return_vals = procedural_db_run_proc (tool_active_PDB_string(),
					    &nreturn_vals,
					    PDB_DRAWABLE, drawable_ID (drawable),
					    PDB_INT32, (gint32) rpnts->num_stroke_points * 2,
					    PDB_FLOATARRAY, rpnts->stroke_points,
					    PDB_END);

      if (return_vals && return_vals[0].value.pdb_int != PDB_SUCCESS)
	g_message (_("Paintbrush operation failed."));

      procedural_db_destroy_args (return_vals, nreturn_vals);

      g_free (rpnts->stroke_points);
    }
    next_rpnts = rpnts->next_curve;
    rpnts->stroke_points = NULL;
    rpnts->len_stroke_points = rpnts->num_stroke_points = 0;
    g_free(rpnts);
    rpnts = next_rpnts;
    } while (rpnts);
  /*  End an undo group  */
  undo_push_group_end (gdisp->gimage);
  gdisplays_flush ();
}

static void
bezier_draw_segment_for_distance (BezierSelect     *bezier_sel,
				  BezierPoint      *points,
				  int               subdivisions,
				  BezierDistance   *bdist)
{
  BezierMatrix geometry;
  BezierMatrix tmp1, tmp2;
  BezierMatrix deltas;
  double x, dx, dx2, dx3;
  double y, dy, dy2, dy3;
  double d, d2, d3;
  int index;
  int i;

  /* construct the geometry matrix from the segment */
  /* assumes that a valid segment containing 4 points is passed in */

  if(bdist->found)
    return;

  for (i = 0; i < 4; i++)
    {
      if (!points)
	gimp_fatal_error ("bezier_draw_segment_for_distance(): Bad bezier segment");

      geometry[i][0] = points->x;
      geometry[i][1] = points->y;
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

/*       g_print ("x = %g, y = %g\n",x,y); */

      /* if this point is different than the last one...then draw it */
      /* Note :
       * It assumes the udata is the place we want the 
       * floating version of the coords to be stuffed.
       * These are needed when we calculate the gradient of the 
       * curve.
       */

      if(!bdist->firstpnt)
	{
	  gdouble rx = x;
	  gdouble ry = y;
 	  gdouble dx = bdist->lastx - rx; 
 	  gdouble dy = bdist->lasty - ry; 

 	  bdist->curdist += sqrt((dx*dx)+(dy*dy)); 
 	  if(bdist->curdist >= bdist->dist) 
 	    { 
 	      *(bdist->x) = ROUND((rx + dx/2)); 
 	      *(bdist->y) = ROUND((ry + dy/2)); 
 	      if(dx == 0.0) 
 		*(bdist->gradient) = G_MAXDOUBLE; 
 	      else 
 		*(bdist->gradient) = dy/dx; 

/* 	      g_print ("found x = %d, y = %d\n",*(bdist->x),*(bdist->y)); */
	      bdist->found = TRUE;
 	      break; 
 	    } 
 	  bdist->lastx = rx; 
 	  bdist->lasty = ry; 
	}
      else
	{
	  bdist->firstpnt = FALSE;
	  bdist->lastx = x;
	  bdist->lasty = y;
	}
    }
}

static void
bezier_draw_curve_for_distance (BezierSelect    *bezier_sel,
				BezierDistance  *udata)
{
  BezierPoint * points;
  BezierPoint * start_pt;
  BezierPoint * next_curve;

  points = bezier_sel->points;
  start_pt = bezier_sel->points;

  if(bezier_sel->num_points >= 4)
    {
      do {
	do {
	  bezier_draw_segment_for_distance (bezier_sel, points,
					    SUBDIVIDE, 
					    udata);
	  points = next_anchor(points,&next_curve);
	} while (points != start_pt && points);
	start_pt = next_curve;
	points = next_curve;
      } while (next_curve);
    }
}

gint
bezier_distance_along(BezierSelect *bezier_sel,
		      int	    open_path,
		      gdouble       dist,
		      gint         *x,
		      gint         *y,
		      gdouble      *gradient)
{
  /* Render the curve as points then walk along it... */
  BezierDistance *bdist = g_new0(BezierDistance,1);
  gint ret;

  bdist->firstpnt = TRUE;
  bdist->curdist = 0.0;
  bdist->lastx = 0.0;
  bdist->lasty = 0.0;
  bdist->dist = dist;
  bdist->x = x;
  bdist->y = y;
  bdist->gradient = gradient;
  bdist->found = FALSE;

  bezier_draw_curve_for_distance (bezier_sel,bdist);
  ret = bdist->found;

  g_free(bdist);
  return (ret);
}
