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
#include <stdio.h>
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
#include "actionarea.h"

/* Bezier extensions made by Raphael FRANCOIS (fraph@ibm.net)

  BEZIER_EXTENDS VER 1.0 

  - work as the cut/copy/paste named functions.
  - allow to add/remove/replace bezier curves
  - allow to modify the control/anchor points even if the selection is made
  - allow to add/remove control/anchor points on a curve
  - allow to update a previous saved curve

  - cannot operate on open or multiple curves simultaneously
*/

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

/* bezier select type definitions */

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

/* The named paste dialog */
typedef struct _PasteNamedDlg PasteNamedDlg;

struct _PasteNamedDlg
{
  GtkWidget   *shell;
  GtkWidget   *list;
  GDisplay    *gdisp; 
};

/* The named buffer structure... */
typedef struct _named_buffer BezierNamedBuffer;

struct _named_buffer
{
  BezierSelect *sel;
  char *        name;
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
static GSList * bezier_insert_in_list      (GSList *, int);
static void  bezier_named_buffer_proc      (GDisplay *);

static int   add_point_on_segment          (BezierSelect *, BezierPoint *, int, int, int, int, int);

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
 
enum { EXTEND_EDIT, EXTEND_ADD, EXTEND_REMOVE };

static SelectionOptions *bezier_options = NULL;

/* Global Static Variable to maintain informations about rhe "contexte" */
static int dialog_open = 0;
static BezierSelect * curSel;
static Tool * curTool;
static GDisplay * curGdisp;
static DrawCore * curCore;
static PasteNamedDlg * curPndlg;
static int ModeEdit = EXTEND_EDIT;


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

  if (dialog_open == 2)
    {
      curCore = bezier_sel->core;
      curSel = bezier_sel;
      curTool = tool;
    }

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

  if (curPndlg)
    {
      gtk_widget_hide( curPndlg->shell );
      dialog_open = 2;
    }

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
  gtk_widget_activate (tool_info[BEZIER_SELECT].tool_widget);
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

  if (dialog_open == 2)
    {
      gtk_widget_show( curPndlg->shell );
      dialog_open = 1;
    }

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

  if (bezier_options->extend)
    bezier_named_buffer_proc( gdisp_ptr ); 
  curTool = active_tool;
  curSel = curTool->private;
  curGdisp = (GDisplay *) gdisp_ptr;
  curCore = bezier_sel->core;

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
	fatal_error ("tried to edit on open bezier curve in edit selection");

      /* erase the handles */
      bezier_sel->draw = BEZIER_DRAW_HANDLES;
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
	      start_pt = bezier_sel->points;

	      do {
		  if (add_point_on_segment (bezier_sel,       
					    points,
					    SUBDIVIDE, 
					    SCREEN_COORDS,
					    x, y,
					    halfwidth))
		    {
		      bezier_sel->draw = BEZIER_DRAW_CURVE; 
		      draw_core_resume (bezier_sel->core, tool);
		      bezier_sel->draw = 0;
		      draw_core_stop ( bezier_sel->core, tool );
                   
		      bezier_sel->draw = BEZIER_DRAW_CURVE;
		      draw_core_start (bezier_sel->core, gdisp->canvas->window, tool);

		      bezier_sel->draw = BEZIER_DRAW_ALL; 

		      draw_core_pause( bezier_sel->core, tool);               
		      draw_core_resume( bezier_sel->core, tool);

		      break;
		    }

		  points = points->next;
		  points = points->next;
		  points = points->next;
	        } while (points != start_pt);
	    }
	}

      /* find if the button press occurred on a point */
      do {
	  if (bezier_check_point (points, x, y, halfwidth))
	    {
	      {
		BezierPoint *finded=points;
		BezierPoint *start_op; 
		BezierPoint *end_op; 

		if (ModeEdit== EXTEND_REMOVE)
		  {
		    if (bezier_sel->num_points <= 7)
		      {
			/* If we've got less then 7 points ie: 2 anchors points 4 controls 
			   Then the curve is minimal closed curve.
			   I've decided to not operate on this kind of curve because it    
			   implies opening the curve and change some drawing states        
                           Removing 1 point of curve that contains 2 point is something    
                           similare to reconstruct the curve !!!
                          */
                        goto end;
		      }

		    bezier_sel->draw = BEZIER_DRAW_CURVE; 
		    draw_core_resume (bezier_sel->core, tool);
		    bezier_sel->draw = 0;

		    draw_core_stop ( bezier_sel->core, tool );
                   
		    if ( finded->type == BEZIER_CONTROL )
		      {
			if (finded->next->type == BEZIER_CONTROL)
			  finded = finded->prev;
			else
			  finded = finded->next;
		      }

		    start_op = finded->prev->prev;
		    end_op = finded->next->next;

		    start_op->next = end_op;
		    end_op->prev = start_op;

		    if ( (bezier_sel->last_point == finded) || 
			 (bezier_sel->last_point == finded->next) || 
			 (bezier_sel->last_point  == finded->prev))
		      {
			 bezier_sel->last_point = start_op->prev;
			 bezier_sel->points = start_op->prev;
		      }

		    bezier_sel->num_points -= 3;

		    g_free( finded->prev );
		    g_free( finded->next );
		    g_free( finded );

		    bezier_sel->draw = BEZIER_DRAW_CURVE;       
		    draw_core_start (bezier_sel->core, gdisp->canvas->window, tool);
		  end:
		    bezier_sel->draw = BEZIER_DRAW_ALL; 

		    draw_core_pause( bezier_sel->core, tool);               
		    draw_core_resume( bezier_sel->core, tool);
		  }
	      }

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

	  if (!bezier_options->extend)
	    {
	      tool->state = INACTIVE;
	      bezier_sel->draw = BEZIER_DRAW_CURVE;
	      draw_core_resume (bezier_sel->core, tool);

	      bezier_sel->draw = 0;
	      draw_core_stop (bezier_sel->core, tool);
	    }

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

	  if (!bezier_options->extend)
	    {
	      bezier_select_reset (bezier_sel);
	    }
	  else
	    {
	      /*  show selection on all views  */
	      bezier_sel->draw = BEZIER_DRAW_HANDLES;
	      draw_core_resume (bezier_sel->core, tool); 
	    }
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
      int i;
      BezierPoint *tmp = bezier_sel->points;

      offsetx = x - lastx;
      offsety = y - lasty;

      for (i=0; i< bezier_sel->num_points; i++)
       {
         bezier_offset_point (tmp, offsetx, offsety);
         tmp = tmp->next;
       }
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
/*
          if (!list)
	    g_message ("cannot properly scanline convert bezier curve: %d", i);
          else
*/
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
/*********************************************/
/*        Named buffer operations            */
/*********************************************/

static void
set_list_of_named_buffers (GtkWidget *list_widget)
{
  GSList *list;
  BezierNamedBuffer *nb;
  GtkWidget *list_item;

  gtk_list_clear_items (GTK_LIST (list_widget), 0, -1);
  list = bezier_named_buffers;

  while (list)
    {
      nb = (BezierNamedBuffer *) list->data;
      list =  g_slist_next(list);

      list_item = gtk_list_item_new_with_label (nb->name);
      gtk_container_add (GTK_CONTAINER (list_widget), list_item);
      gtk_widget_show (list_item);
      gtk_object_set_user_data (GTK_OBJECT (list_item), (gpointer) nb);
    }
}

static void
bezier_buffer_paste_foreach (GtkWidget *w,
			    gpointer   client_data)
{
  PasteNamedDlg *pn_dlg;
  BezierNamedBuffer *nb;
  BezierSelect *sel;
  BezierPoint *pts;
  
  if (w->state == GTK_STATE_SELECTED)
    {
      int i;

      pn_dlg = (PasteNamedDlg *) client_data;
      nb = (BezierNamedBuffer *) gtk_object_get_user_data (GTK_OBJECT (w));

      sel = (BezierSelect *) nb->sel;
      pts = (BezierPoint *) nb->sel->points;

      curSel->draw = BEZIER_DRAW_ALL; 
      draw_core_resume (curSel->core, curTool);
      curSel->draw = 0;
      draw_core_stop ( curSel->core, curTool );
  
      bezier_select_reset( curSel );
      
      curTool->state  = ACTIVE;
      
      for (i=0; i< nb->sel->num_points; i++)
	{
	  bezier_add_point( curSel, pts->type, pts->x, pts->y);
	  pts = pts->next;
	}
      
      if ( sel->closed )
	{
	  curSel->last_point->next = curSel->points;
	  curSel->points->prev = curSel->last_point;
	  curSel->cur_anchor = curSel->points;
	  curSel->cur_control = curSel-> points->next;
	  curSel->closed = 1;
	  if (curTool->gdisp_ptr)
	    bezier_convert(curSel, curTool->gdisp_ptr, SUBDIVIDE, NO);
	}

      curSel->core = curCore;
      curSel->state = nb->sel->state;
      curSel->draw = BEZIER_DRAW_ALL;
    }
 
 
  draw_core_start (curSel->core, curGdisp->canvas->window, curTool);

  curSel->draw = BEZIER_DRAW_ALL;
  draw_core_pause( curSel->core, curTool);
  draw_core_resume( curSel->core, curTool);
} 



static void bezier_buffer_paste_callback (GtkWidget *w,
			     gpointer   client_data)
{
  PasteNamedDlg *pn_dlg;

  pn_dlg = (PasteNamedDlg *) client_data;

  gtk_container_foreach ((GtkContainer*) pn_dlg->list,
			 bezier_buffer_paste_foreach, client_data);
}

static void
bezier_buffer_delete_foreach (GtkWidget *w,
			     gpointer   client_data)
{
  BezierNamedBuffer * nb;
  
  if (w->state == GTK_STATE_SELECTED)
    {
      
      nb = (BezierNamedBuffer *) gtk_object_get_user_data (GTK_OBJECT (w));

      if (nb==NULL)
	{
	  return;
	}
      bezier_select_reset(nb->sel);
      g_free (nb->sel);
      g_free (nb->name);
      g_free (nb);
      bezier_named_buffers = g_slist_remove (bezier_named_buffers, (void *) nb);
    } 
} 

static void
bezier_buffer_delete_callback (GtkWidget *w,
			      gpointer   client_data)
{
  PasteNamedDlg *pn_dlg;

  pn_dlg = (PasteNamedDlg *) client_data;
  gtk_container_foreach ((GtkContainer*) pn_dlg->list,
			 bezier_buffer_delete_foreach, client_data);
  set_list_of_named_buffers (pn_dlg->list);
}

static void bezier_replace_foreach (GtkWidget *w,
				     gpointer   client_data)
{
  BezierNamedBuffer * nb;
  BezierPoint *pts;
  int i;
  
  if (w->state == GTK_STATE_SELECTED)
    {
      
      nb = (BezierNamedBuffer *) gtk_object_get_user_data (GTK_OBJECT (w));
      
      if (nb==NULL)
	{
	  return;
	}
      bezier_select_reset(nb->sel);

      if (nb->sel == NULL) 
	{
	  return;
	}
      
      nb->sel->closed = 0;
      nb->sel->points = NULL;           /* initially there are no points */
      nb->sel->cur_anchor = NULL;
      nb->sel->cur_control = NULL;
      nb->sel->last_point = NULL;
      nb->sel->num_points = 0;          /* intially there are no points */
      nb->sel->mask = NULL;             /* empty mask */
      nb->sel->scanlines = NULL;
      pts = curSel->points;
      
      for (i=0; i< curSel->num_points; i++)
	{
	  bezier_add_point( nb->sel, pts->type, pts->x, pts->y);
	  pts = pts->next;
	}
      
      if ( curSel->closed )
	{
	  nb->sel->last_point->next = nb->sel->points;
	  nb->sel->points->prev = nb->sel->last_point;
	  nb->sel->cur_anchor = nb->sel->points;
	  nb->sel->cur_control = nb->sel->points->next;
	  
	  nb->sel->closed = 1;
	}
      
      nb->sel->draw =  curSel->draw;
      nb->sel->state = curSel->state;
      
      
    } 
}

static void bezier_replace_callback (GtkWidget *w,
				     gpointer   client_data)
{
  PasteNamedDlg *pn_dlg;
  
  pn_dlg = (PasteNamedDlg *) client_data;
  gtk_container_foreach ((GtkContainer*) pn_dlg->list,
			 bezier_replace_foreach, client_data);
  
}

static GtkWidget *file_dlg = 0;
static int load_store;
FILE *f;


static void bezier_buffer_save_foreach(GtkWidget *w,
				       gpointer   client_data)
		
{
  PasteNamedDlg *pn_dlg;
  BezierNamedBuffer * nb;
  BezierPoint *pt;
  int i;
      
  pn_dlg = (PasteNamedDlg *) client_data;
  nb = (BezierNamedBuffer *) gtk_object_get_user_data (GTK_OBJECT (w));


  fprintf(f, "Name: %s\n", nb->name);
  fprintf(f, "#POINTS: %d\n", nb->sel->num_points);
  fprintf(f, "CLOSED: %d\n", nb->sel->closed==1?1:0);
  fprintf(f, "DRAW: %d\n", nb->sel->draw);
  fprintf(f, "STATE: %d\n", nb->sel->state);


  pt = nb->sel->points;
  for(i=0; i< nb->sel->num_points; i++)
    {
      fprintf(f,"TYPE: %d X: %d Y: %d\n", pt->type, pt->x, pt->y);
      pt = pt->next;
    }
}

static void file_ok_callback(GtkWidget * widget, gpointer client_data) 
{
  GtkFileSelection *fs; 
  char* filename;
  fs = GTK_FILE_SELECTION (file_dlg);
  filename = gtk_file_selection_get_filename (fs);
  
  if (load_store) 
    {
      PasteNamedDlg *pn_dlg;
      BezierNamedBuffer * nb;
      
      pn_dlg = (PasteNamedDlg *) client_data;

      f = fopen(filename, "r");
      printf("reading %s\n", filename);
      
      while(!feof(f))
	{
	  char txt[30];
	  int val, x, y, type, closed, i, draw, state;

	  /*
	    fscanf(f, "%s", txt);
	    if (strcmp("BEZIER_EXTENDS",txt))
	    {
	    GString *s;
	    
	    fclose(f);
	    s = g_string_new ("Open failed: ");
	    
	    g_string_append (s, "Bezier Load");
	    
	    g_message (s->str);
	      
	    g_string_free (s, TRUE);
	    return;
	    }
	    */	  
	  fscanf(f, "Name: %s\n", txt);
	  fscanf(f, "#POINTS: %d\n", &val);
 	  fscanf(f, "CLOSED: %d\n", &closed);
	  fscanf(f, "DRAW: %d\n", &draw);
	  fscanf(f, "STATE: %d\n", &state);

	  nb = (BezierNamedBuffer *) g_malloc (sizeof(BezierNamedBuffer));
	  nb->name = g_strdup ((char *) txt);
	  nb->sel = (BezierSelect *) g_malloc( sizeof (BezierSelect));
	  
	  if (nb->sel == NULL) 
	    {
	      return;
	    }
	  
	  nb->sel->closed = 0;
	  nb->sel->points = NULL;           /* initially there are no points */
	  nb->sel->cur_anchor = NULL;
	  nb->sel->cur_control = NULL;
	  nb->sel->last_point = NULL;
	  nb->sel->num_points = 0;          /* intially there are no points */
	  nb->sel->mask = NULL;             /* empty mask */
	  nb->sel->scanlines = NULL; 
	  
	  for(i=0; i< val; i++)
	    {
	      fscanf(f,"TYPE: %d X: %d Y: %d\n", &type, &x, &y);
	      bezier_add_point( nb->sel, type, x, y);
	    }
  
	  if ( closed )
	    {
	      nb->sel->last_point->next = nb->sel->points;
	      nb->sel->points->prev = nb->sel->last_point;
	      nb->sel->cur_anchor = nb->sel->points;
	      nb->sel->cur_control = nb->sel-> points->next;
	      nb->sel->closed = 1;

	    }
	  
	  nb->sel->state = state;
	  nb->sel->draw = draw;
	  
	  bezier_named_buffers = g_slist_append (bezier_named_buffers, (void *) nb);
	  set_list_of_named_buffers (pn_dlg->list);
	}
      fclose(f);      
    } 
  else 
    {
      PasteNamedDlg *pn_dlg;
      f = fopen(filename, "w");
      if (NULL == f) 
	{
	  perror(filename);
	  return;
	}
      
      pn_dlg = (PasteNamedDlg *) client_data;
      /*
	fprintf(f,"BEZIER_EXTENDS\n");
	*/
      gtk_container_foreach ((GtkContainer*) pn_dlg->list,
			     bezier_buffer_save_foreach, 
			     client_data);
      fclose(f);
    }
  gtk_widget_hide (file_dlg);  
}


static void file_cancel_callback(GtkWidget * widget, gpointer data) 
{
  gtk_widget_hide (file_dlg);
}

static void make_file_dlg(gpointer data) 
{
  file_dlg = gtk_file_selection_new ("Load/Store Bezier Curves");
  gtk_window_position (GTK_WINDOW (file_dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect(GTK_OBJECT (GTK_FILE_SELECTION (file_dlg)->cancel_button),
		     "clicked", (GtkSignalFunc) file_cancel_callback, data);
  gtk_signal_connect(GTK_OBJECT (GTK_FILE_SELECTION (file_dlg)->ok_button),
		     "clicked", (GtkSignalFunc) file_ok_callback, data);
}


static void load_callback(GtkWidget * widget, gpointer data) 
{
  if (!file_dlg) 
    {
      make_file_dlg(data);
    } 
  else 
    {
      if (GTK_WIDGET_VISIBLE(file_dlg))
	return;
    }
  gtk_window_set_title(GTK_WINDOW (file_dlg), "Load Bezier Curves");
  load_store = 1;
  gtk_widget_show (file_dlg);
}

static void store_callback(GtkWidget * widget, gpointer data) 
{
  if (!file_dlg) 
    {
      make_file_dlg(data);
    } 
  else 
    {
      if (GTK_WIDGET_VISIBLE(file_dlg))
	return;
    }

  gtk_window_set_title(GTK_WINDOW (file_dlg), "Store Bezier Curves");
  load_store = 0;
  gtk_widget_show (file_dlg);
}

static void new_bezier_buffer_callback (GtkWidget *w,
					gpointer   client_data,
					gpointer   call_data)
{
  BezierNamedBuffer *nb; 
  PasteNamedDlg *pn_dlg;
  int i;
  BezierPoint *pts = curSel->points;


  if (curSel == NULL || pts==NULL)
    {
      return;
    }


  pn_dlg = (PasteNamedDlg *) client_data;

  nb = (BezierNamedBuffer *) g_malloc (sizeof (BezierNamedBuffer));

  if (nb==NULL)
    {
      return;
    }


  nb->name = g_strdup ((char *) call_data);
  nb->sel = (BezierSelect *) g_malloc( sizeof (BezierSelect));

  if (nb->sel == NULL) 
    {
      return;
    }

  nb->sel->closed = 0;
  nb->sel->points = NULL;           /* initially there are no points */
  nb->sel->cur_anchor = NULL;
  nb->sel->cur_control = NULL;
  nb->sel->last_point = NULL;
  nb->sel->num_points = 0;          /* intially there are no points */
  nb->sel->mask = NULL;             /* empty mask */
  nb->sel->scanlines = NULL;
  
  for (i=0; i< curSel->num_points; i++)
    {
      bezier_add_point( nb->sel, pts->type, pts->x, pts->y);
      pts = pts->next;
    }
  
  if ( curSel->closed )
    {
      nb->sel->last_point->next = nb->sel->points;
      nb->sel->points->prev = nb->sel->last_point;
      nb->sel->cur_anchor = nb->sel->points;
      nb->sel->cur_control = nb->sel->points->next;
      
      nb->sel->closed = 1;
    }
   
  nb->sel->draw =  curSel->draw;
  nb->sel->state = curSel->state;
  
  
  bezier_named_buffers = g_slist_append (bezier_named_buffers, (void *) nb);
  set_list_of_named_buffers (pn_dlg->list);
}


static void bezier_add_callback (GtkWidget *w,
				 gpointer   client_data)
{
  PasteNamedDlg *pn_dlg;

  pn_dlg = (PasteNamedDlg *) client_data;

  query_string_box ("Named Bezier Buffer", 
		    "Enter a name for this buffer", 
		    NULL,
		    new_bezier_buffer_callback, 
		    pn_dlg);
}

static void bezier_new_curve_callback(GtkWidget *w,
				      gpointer   client_data)
{
  gchar *name;

  name = gtk_widget_get_name( w ); 

  curSel->draw = BEZIER_DRAW_ALL; 
  draw_core_resume (curSel->core, curTool);
  curSel->draw = 0;
  draw_core_stop ( curSel->core, curTool );
  
  bezier_select_reset( curSel );
  draw_core_start (curSel->core, curGdisp->canvas->window, curTool);

  curSel->draw = BEZIER_DRAW_ALL;
  draw_core_pause( curSel->core, curTool);
  draw_core_resume( curSel->core, curTool);
}

static void menu_cb(GtkWidget * widget, gpointer data) 
{
  ModeEdit = (int) data;
}

void create_choice(GtkWidget *box)
{
  static struct {
    char *name;
    int value;
  } menu_items[] = {
    {"Edit Curve", EXTEND_EDIT },
    {"Add Point", EXTEND_ADD },
    {"Remove Point", EXTEND_REMOVE},
    { NULL, 0},
  };

  GtkWidget *hbox;
  GtkWidget *option_menu;
  GtkWidget *menu, *w;
  int i;
  
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start(GTK_BOX(box), hbox, TRUE, FALSE, 10);
  gtk_widget_show(hbox);
  
  w = gtk_label_new("Mode :");
  gtk_misc_set_alignment(GTK_MISC(w), 0.0, 0.5);
  gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 0);
  gtk_widget_show(w);
  
  option_menu = gtk_option_menu_new ();
  gtk_box_pack_start(GTK_BOX(hbox), option_menu, FALSE, FALSE, 0);
  i = 0;
  menu = gtk_menu_new ();
  while (menu_items[i].name) {
    GtkWidget *menu_item;
    menu_item = gtk_menu_item_new_with_label(menu_items[i].name);
    gtk_container_add (GTK_CONTAINER (menu), menu_item);
    gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
			(GtkSignalFunc) menu_cb,
			(gpointer) menu_items[i].value);
    gtk_widget_show (menu_item);
    i++;
  }
  gtk_menu_set_active (GTK_MENU (menu), EXTEND_EDIT);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
  gtk_widget_show (option_menu);
}

static void bezier_named_buffer_proc (GDisplay *gdisp)
{
  int i;
  static ActionAreaItem action_items[] =
  {
    { "New", bezier_new_curve_callback, NULL, NULL },
    { "Add", bezier_add_callback, NULL, NULL },
    { "Replace", bezier_replace_callback, NULL, NULL },
    { "Paste", bezier_buffer_paste_callback, NULL, NULL },
    { "Delete", bezier_buffer_delete_callback, NULL, NULL },
    { "Load", load_callback, NULL, NULL },
    { "Save", store_callback, NULL, NULL }
  };
  PasteNamedDlg *pn_dlg;
  GtkWidget *vbox;
  GtkWidget *label; 
  GtkWidget *listbox;

  if (dialog_open==1)
    return;

  dialog_open = 1;

  pn_dlg = (PasteNamedDlg *) g_malloc (sizeof (PasteNamedDlg));
  pn_dlg->gdisp = gdisp;

  pn_dlg->shell = gtk_dialog_new ();

  curPndlg = pn_dlg;

  gtk_window_set_title (GTK_WINDOW (pn_dlg->shell), "Paste Bezier Named Buffer");
  gtk_window_position (GTK_WINDOW (pn_dlg->shell), GTK_WIN_POS_MOUSE);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (pn_dlg->shell)->vbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  label = gtk_label_new ("Select a buffer to operate:");
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, FALSE, 0);
  gtk_widget_show (label);

  listbox = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (listbox),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), listbox, TRUE, TRUE, 0);
  gtk_widget_set_usize (listbox, 125, 100);
  gtk_widget_show (listbox);

  pn_dlg->list = gtk_list_new ();
  gtk_list_set_selection_mode (GTK_LIST (pn_dlg->list), GTK_SELECTION_BROWSE);
  gtk_container_add (GTK_CONTAINER (listbox), pn_dlg->list);
  set_list_of_named_buffers (pn_dlg->list);
  gtk_widget_show (pn_dlg->list);

  create_choice( vbox );

  for (i=0; i < sizeof( action_items)/sizeof( ActionAreaItem); i++)
    action_items[i].user_data = pn_dlg;

  build_action_area (GTK_DIALOG (pn_dlg->shell), 
		     action_items,  
		     sizeof( action_items)/sizeof( ActionAreaItem), 
		     0);

  gtk_widget_show (pn_dlg->shell);
}


static int
add_point_on_segment (BezierSelect     *bezier_sel,
			    BezierPoint      *pt,
			    int               subdivisions,
			    int               space,
			    int xpos, 
			    int ypos, 
			    int halfwidth)

{
#define ROUND(x)  ((int) ((x) + 0.5))

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

  points = pt;

  ratio = -1.0;

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

      pt1 = g_malloc (sizeof (BezierPoint));
      pt2 = g_malloc (sizeof (BezierPoint));
      pt3 = g_malloc (sizeof (BezierPoint));
      
      pt1->type = BEZIER_CONTROL;
      pt2->type = BEZIER_ANCHOR;
      pt3->type = BEZIER_CONTROL;
      
      pt1->x = P20.x; pt1->y = P20.y;    
      pt2->x = P30.x; pt2->y = P30.y;
      pt3->x = P21.x; pt3->y = P21.y;
      
      
      P01->next  = pt1;
      
      pt1->prev = P01;
      pt1->next = pt2;
      
      pt2->prev = pt1;
      pt2->next = pt3;
      
      pt3->prev = pt2; 
      pt3->next = P02;
      
      P02->prev = pt3;
      
      bezier_sel->num_points += 3;

      return 1;
    }

  return 0;
  
}

void printSel( BezierSelect *sel)
{
  BezierPoint *pt;
  int i;

  pt = sel->points;

  printf("\n");

  for(i=0; i<sel->num_points; i++)
    {
      printf("%d %d %d\n", i, pt->x, pt->y);
      pt = pt->next;
    }

  printf("core : %p\n", sel->core);
  printf("closed : %d\n", sel->closed);
  printf("draw : %d\n", sel->draw);
  printf("state: %d\n", sel->state);
}

