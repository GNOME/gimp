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

/*
 * Complete new path-tool by Simon Budig <Simon.Budig@unix-ag.org>
 * 
 * Currently vapor-ware.
 * 
 * a path manipulation core independent of the underlying formula:
 * implement bezier-curves, intelligent scissors-curves, splines...
 * 
 * A Path is a collection of curves, which are constructed from
 * segments between two anchors.
 */

#include <math.h>
/* #include "appenv.h"
 */

#include "draw_core.h"
#include "cursorutil.h"
#include "path_tool.h"
#include "path_toolP.h"

#include <stdio.h>

#include "libgimp/gimpintl.h"

/*
 * Every new curve-type has to have a parameter between 0 and 1, and
 * should go from a starting to a target point.
 */



/* Some defines... */

#define PATH_TOOL_WIDTH 8
#define PATH_TOOL_HALFWIDTH 4

/*  local function prototypes  */


/* Small functions to determine coordinates, iterate over path/curve/segment */

static void    path_segment_get_coordinates (PathSegment *, gdouble, gint *, gint *);
static void    path_traverse_path           (Path *, PathTraverseFunc, CurveTraverseFunc, SegmentTraverseFunc, gpointer);
static void    path_traverse_curve          (Path *, PathCurve *, CurveTraverseFunc, SegmentTraverseFunc, gpointer);
static void    path_traverse_segment        (Path *, PathCurve *, PathSegment *, SegmentTraverseFunc, gpointer);
static gdouble path_locate_point            (Path *, PathCurve **, PathSegment **, gint, gint, gint, gint, gint);

/* Tools to manipulate paths, curves, segments */

static PathCurve   * path_add_curve       (Path *, gint, gint);
static inline PathSegment * path_append_segment  (Path *, PathCurve *, SegmentType, gint, gint);
static PathSegment * path_prepend_segment (Path *, PathCurve *, SegmentType, gint, gint);
static PathSegment * path_split_segment   (PathSegment *, gdouble);
static void          path_free_path       (Path *);
static void          path_free_curve      (PathCurve *);
static void          path_free_segment    (PathSegment *);
static void          path_print           (Path *);
static void          path_offset_active   (Path *, gdouble, gdouble);
static void          path_clear_active    (Path *, PathCurve *, PathSegment *);

/* High level image-manipulation functions */

static void path_stroke (PathTool *, Path *);
static void path_to_selection (PathTool *, Path *);

/* Functions necessary for the tool */

static void path_tool_button_press    (Tool *, GdkEventButton *, gpointer);
static void path_tool_button_release  (Tool *, GdkEventButton *, gpointer);
static void path_tool_motion          (Tool *, GdkEventMotion *, gpointer);
static void path_tool_cursor_update   (Tool *, GdkEventMotion *, gpointer);
static void path_tool_control         (Tool *, ToolAction,       gpointer);
static void path_tool_draw            (Tool *);
static void path_tool_draw_curve      (Tool *, PathCurve *);
static void path_tool_draw_segment    (Tool *, PathSegment *);
static gboolean path_tool_on_anchors      (Tool *, gint, gint, gint, Path**, PathCurve**, PathSegment**);
static gboolean path_tool_on_handles      (Tool *, gint, gint, gint);
static gboolean path_tool_on_curve        (Tool *, gint, gint, gint);

static gint path_tool_button_press_canvas (Tool *, GdkEventButton *, GDisplay *);
static gint path_tool_button_press_anchor (Tool *, GdkEventButton *, GDisplay *);
static void path_tool_motion_anchor       (Tool *, GdkEventMotion *, GDisplay *);
static void path_tool_motion_curve        (Tool *, GdkEventMotion *, GDisplay *);


/*  the path tool options  */
static ToolOptions *path_options = NULL;


/* 
 * 
 * 
 * Here we go!
 * 
 * 
 */

/*
 * These functions are for applying a function over a complete path/curve/segment
 * they can pass information to each other with a arbitrary data structure
 *
 * The idea behind the three different functions is:
 *  if pathfunc != NULL
 *     call pathfunc for every curve
 *  else
 *     if curvefunc != NULL
 *        call curvefunc for every segment
 *     else
 *        call segmentfunc for every point
 *
 */

static void
path_traverse_path (Path *path, PathTraverseFunc pathfunc, CurveTraverseFunc curvefunc, SegmentTraverseFunc segmentfunc, gpointer data)
{
   PathCurve *cur_curve;

   if (path && path->curves)
   {
      cur_curve = path->curves;
      if (pathfunc)
         do {
            (* pathfunc) (path, cur_curve, data);
            cur_curve = cur_curve->next;
         } while (cur_curve && cur_curve != path->curves);
      else
         do {
            path_traverse_curve (path, cur_curve, curvefunc, segmentfunc, data);
            cur_curve = cur_curve->next;
         } while (cur_curve && cur_curve != path->curves);
   }
}
     

static void
path_traverse_curve (Path *path, PathCurve *curve, CurveTraverseFunc curvefunc, SegmentTraverseFunc segmentfunc, gpointer data)
{
   PathSegment *cur_segment;

   if (curve && curve->segments)
   {
      cur_segment = curve->segments;
      if (curvefunc)
         do {
            (* curvefunc) (path, curve, cur_segment, data);
            cur_segment = cur_segment->next;
         } while (cur_segment && cur_segment != curve->segments);
      else
         do {
            path_traverse_segment (path, curve, cur_segment, segmentfunc, data);
            cur_segment = cur_segment->next;
         } while (cur_segment && cur_segment != curve->segments);
   }
}

static void
path_traverse_segment (Path *path, PathCurve *curve, PathSegment *segment, SegmentTraverseFunc function, gpointer data)
{
   fprintf(stderr, "path_traverse_segment\n");

   /* Something like:
    * for i = 1 to subsamples {
    *   (x,y) = get_coordinates(i / subsamples)
    *   (* function) (....)
    * }
    */
}

/**************************************************************
 * Helper functions for manipulating the data-structures:
 */

static PathCurve   * path_add_curve       (Path * cur_path, gint x, gint y)
{
   PathCurve * tmp = cur_path->curves;
   PathCurve * new_curve;

   fprintf(stderr, "path_add_curve\n");
   
   new_curve = g_new (PathCurve, 1);
   
   new_curve->next = tmp;
   new_curve->prev = NULL;
   new_curve->cur_segment = NULL;
   new_curve->segments = NULL;

   if (tmp) tmp->prev = new_curve;
   
   cur_path->curves = cur_path->cur_curve = new_curve;
   
   new_curve->segments = path_append_segment (cur_path, new_curve, SEGMENT_LINE, x, y);
   return new_curve;
}
   
   
static inline PathSegment * path_append_segment  (Path * cur_path, PathCurve * cur_curve, SegmentType type, gint x, gint y)
{
   PathSegment * tmp = cur_curve->segments;
   PathSegment * new_segment = NULL;
   
   if (cur_curve) {
      tmp = cur_curve->segments;
      while (tmp && tmp->next && tmp->next != cur_curve->segments) {
         tmp = tmp->next;
      }
   
      if (tmp == NULL || tmp->next == NULL) {
         new_segment = g_new (PathSegment, 1);
         
         new_segment->type = type;
         new_segment->x = x;
         new_segment->y = y;
         new_segment->flags = SEGMENT_ACTIVE;
         new_segment->next = NULL;
         new_segment->prev = tmp;
         new_segment->data = NULL;
         
         if (tmp) 
         {
            tmp->next = new_segment;
         }

         cur_curve->cur_segment = new_segment;
      }
      else
         fprintf(stderr, "Fatal Error: path_append_segment called with a closed curve\n");
   }
   else
      fprintf(stderr, "Fatal Error: path_append_segment called without valid curve\n");
   
   return new_segment;
}


/* static PathSegment * path_prepend_segment (Path *, PathCurve *, gint, gint);
 * static PathSegment * path_split_segment   (PathSegment *, gdouble);
 */

static void
path_free_path (Path * path)
{
   PathCurve *tmp1, *tmp2;
   
   if (path)
   {
      tmp2 = path->curves;
      g_string_free(path->name, TRUE);
      g_free(path);

      while ((tmp1 = tmp2) != NULL)
      {
         tmp2 = tmp1->next;
	 path_free_curve (tmp1);
      } 
   }
}

static void
path_free_curve (PathCurve *curve)
{
   PathSegment *tmp1, *tmp2;
   
   if (curve)
   {
      tmp2 = curve->segments;
      g_free(curve);

      while ((tmp1 = tmp2) != NULL)
      {
         tmp2 = tmp1->next;
	 path_free_segment (tmp1);
      } 
   }
}

static void
path_free_segment (PathSegment *segment)
{
   if (segment)
   {
      if (segment->data)
         g_free(segment->data);
      g_free (segment);
   }
}



/*
 * A function to determine, which object is hit by the cursor
 */

static gint
path_tool_cursor_position (Tool *tool, gint x, gint y, gint halfwidth, Path **pathP, PathCurve **curveP, PathSegment **segmentP)
{
   gint location;

   if (path_tool_on_anchors (tool, x, y, halfwidth, pathP, curveP, segmentP))
      return ON_ANCHOR;

   return ON_CANVAS;
}
      
     
/**************************************************************
 * The click-callbacks for the tool
 */

static void
path_tool_button_press (Tool           *tool,
			GdkEventButton *bevent,
			gpointer        gdisp_ptr)
{
   GDisplay * gdisp;
   PathTool * path_tool;
   gint grab_pointer=0;
   gint x, y, halfwidth, dummy;

   fprintf (stderr, "path_tool_button_press\n");

   gdisp = (GDisplay *) gdisp_ptr;
   path_tool = (PathTool *) tool->private;
   tool->gdisp_ptr = gdisp_ptr;
  
   /* Transform window-coordinates to canvas-coordinates */
   gdisplay_untransform_coords (gdisp, bevent->x, bevent->y, &x, &y, TRUE, 0);
   fprintf(stderr, "Clickcoordinates %d, %d\n",x,y);
   path_tool->click_x = x;
   path_tool->click_y = y;
   path_tool->click_modifier = bevent->state;
   /* get halfwidth in image coord */
   gdisplay_untransform_coords (gdisp, bevent->x + PATH_TOOL_HALFWIDTH, 0, &halfwidth, &dummy, TRUE, 0);
   halfwidth -= x;
   path_tool->click_halfwidth = halfwidth;
   
   if (!path_tool->cur_path->curves)
      draw_core_start (path_tool->core, gdisp->canvas->window, tool);

   /* determine point, where clicked,
    * switch accordingly.
    */
  
   path_tool->click_pos = path_tool_cursor_position(tool, x, y, halfwidth,
                                                    &(path_tool->click_path),
						    &(path_tool->click_curve),
						    &(path_tool->click_segment));
  
   switch (path_tool->click_pos)
   {
   case ON_CANVAS:
      grab_pointer = path_tool_button_press_canvas(tool, bevent, gdisp);
      break;

   case ON_ANCHOR:
      grab_pointer = path_tool_button_press_anchor(tool, bevent, gdisp);
      break;

   default:
      g_message("Huh? Whats happening here? (button_press_*)");
   }
  
   if (grab_pointer)
      gdk_pointer_grab (gdisp->canvas->window, FALSE,
		        GDK_POINTER_MOTION_HINT_MASK |
			GDK_BUTTON1_MOTION_MASK |
			GDK_BUTTON_RELEASE_MASK,
			NULL, NULL, bevent->time);

   tool->state = ACTIVE;

}

static gint
path_tool_button_press_anchor (Tool *tool,
                               GdkEventButton *bevent,
                               GDisplay *gdisp)
{
   PathTool *path_tool = tool->private;
   
   Path * cur_path = path_tool->cur_path;
   gint grab_pointer;
   
   fprintf(stderr, "path_tool_button_press_anchor:\n");
   
   grab_pointer = 1;

   if (!cur_path) {
      fprintf (stderr, "Fatal error: No current Path\n");
      return 0;
   }
   
   draw_core_pause (path_tool->core, tool);
  
   /* The user pressed on an anchor:
    *  normally this activates this anchor
    *  + SHIFT toggles the activity of an anchor.
    *  if this anchor is at the end of an open curve and the other
    *  end is active, close the curve.
    */

   if (path_tool->click_modifier & GDK_SHIFT_MASK)
      path_tool->click_segment->flags ^= SEGMENT_ACTIVE;
   else {
      if (!(path_tool->click_segment->flags & SEGMENT_ACTIVE))
	 path_clear_active (cur_path, NULL, NULL);
      path_tool->click_segment->flags |= SEGMENT_ACTIVE;
   }

   /* Action goes here */

   draw_core_resume(path_tool->core, tool);

   return grab_pointer;
}

static gint
path_tool_button_press_canvas (Tool *tool,
                               GdkEventButton *bevent,
                               GDisplay *gdisp)
{
   PathTool *path_tool = tool->private;
   
   Path * cur_path = path_tool->cur_path;
   PathCurve * cur_curve;
   PathSegment * cur_segment;
   gint grab_pointer;
   
   fprintf(stderr, "path_tool_button_press_canvas:\n");
   
   grab_pointer = 1;
   
   if (!cur_path) {
      fprintf (stderr, "Fatal error: No current Path\n");
      return 0;
   }
   
   draw_core_pause (path_tool->core, tool);
  
   path_clear_active (cur_path, NULL, NULL);

   if (!(cur_curve = cur_path->cur_curve)) {
      cur_path->cur_curve = path_add_curve(cur_path, path_tool->click_x, path_tool->click_y);
      draw_core_resume(path_tool->core, tool);
      return 0;
   }
   
   cur_curve->cur_segment = path_append_segment(cur_path, cur_curve, SEGMENT_LINE, path_tool->click_x, path_tool->click_y);

   draw_core_resume(path_tool->core, tool);

   return 0;
}

	       


static void
path_tool_button_release (Tool           *tool,
			  GdkEventButton *bevent,
			  gpointer        gdisp_ptr)
{
   GDisplay * gdisp;
   PathTool * path_tool;
 
   fprintf (stderr, "path_tool_button_release\n");
 
   gdisp = (GDisplay *) gdisp_ptr;
   path_tool = (PathTool *) tool->private;
   
   path_tool->state &= ~PATH_TOOL_DRAG;
   path_tool->state = 0;

   gdk_pointer_ungrab (bevent->time);
   gdk_flush ();
}


/**************************************************************
 * The motion-callbacks for the tool
 */

static void
path_tool_motion (Tool           *tool,
		  GdkEventMotion *mevent,
		  gpointer        gdisp_ptr)
{
   GDisplay * gdisp;
   PathTool * path_tool;

   gdisp = (GDisplay *) gdisp_ptr;
   path_tool = (PathTool *) tool->private;

   switch (path_tool->click_pos) {
   case ON_ANCHOR:
      path_tool_motion_anchor(tool, mevent, gdisp);
      break;
   default:
      return;
   }

}

static void
path_tool_motion_anchor (Tool           *tool,
		         GdkEventMotion *mevent,
		         GDisplay       *gdisp)
{
   PathTool * path_tool;
   gdouble dx, dy;
   gint x,y;
   static gint lastx = 0;
   static gint lasty = 0;
   
   if (gtk_events_pending()) return; /* Is this OK? I want to ignore just the motion-events... */
   
   path_tool = (PathTool *) tool->private;
   
   if (!(path_tool->state & PATH_TOOL_DRAG))
   {
      path_tool->state |= PATH_TOOL_DRAG;
      lastx = path_tool->click_x;
      lasty = path_tool->click_y;
   }

   gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &x, &y, TRUE, 0);
   
   dx = x - lastx;
   dy = y - lasty;
   
   path_tool->draw |= PATH_TOOL_REDRAW_ACTIVE;
   
   draw_core_pause(path_tool->core, tool);

   path_offset_active (path_tool->cur_path, dx, dy);

   draw_core_resume (path_tool->core, tool);
   
   path_tool->draw &= ~PATH_TOOL_REDRAW_ACTIVE;

   lastx = x;
   lasty = y;
}
   

static void
path_tool_cursor_update (Tool           *tool,
			 GdkEventMotion *mevent,
			 gpointer        gdisp_ptr)
{
   PathTool *path_tool;
   GDisplay *gdisp;
   gint     x, y, halfwidth, dummy, cursor_location;
  
   /* fprintf (stderr, "path_tool_cursor_update\n");
    */

   gdisp = (GDisplay *) gdisp_ptr;
   path_tool = (PathTool *) tool->private;

   gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &x, &y, TRUE, 0);
   /* get halfwidth in image coord */
   gdisplay_untransform_coords (gdisp, mevent->x + PATH_TOOL_HALFWIDTH, 0, &halfwidth, &dummy, TRUE, 0);
   halfwidth -= x;

   cursor_location = path_tool_cursor_position(tool, x, y, halfwidth, NULL, NULL, NULL);

   switch (cursor_location) {
   case ON_CANVAS:
      gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE1AP_CURSOR);
      break;
   case ON_ANCHOR:
      gdisplay_install_tool_cursor (gdisp, GDK_TCROSS);
      break;
   default:
      gdisplay_install_tool_cursor (gdisp, GDK_QUESTION_ARROW);
      break;
   }
}

/**************************************************************
 * Tool-control functions
 */

static void
path_tool_control (Tool       *tool,
		   ToolAction  action,
		   gpointer    gdisp_ptr)
{
   GDisplay * gdisp;
   PathTool * path_tool;

   fprintf (stderr, "path_tool_control\n");

   gdisp = (GDisplay *) tool->gdisp_ptr;
   path_tool = (PathTool *) tool->private;

   switch (action)
     {
     case PAUSE:
        draw_core_pause (path_tool->core, tool);
        break;

     case RESUME:
        draw_core_resume (path_tool->core, tool);
        break;

     case HALT:
        draw_core_stop (path_tool->core, tool);
        tool->state = INACTIVE;
        break;

     default:
        break;
     }
}

Tool *
tools_new_path_tool (void)
{
   Tool * tool;
   PathTool * private;

   /*  The tool options  */
   if (! path_options)
      {
         path_options = tool_options_new (_("Path Tool Options"));
         tools_register (PATH_TOOL, (ToolOptions *) path_options);
      }

   tool = tools_new_tool (PATH_TOOL);
   private = g_new (PathTool, 1);

   private->click_pos       = ON_CANVAS;
   private->click_x         = 0;
   private->click_y         = 0;
   private->click_halfwidth = 0;
   private->click_modifier  = 0;
   private->state           = 0;
   private->draw            = PATH_TOOL_REDRAW_ALL;
   private->core            = draw_core_new (path_tool_draw);
   private->cur_path        = g_new0(Path, 1);
   private->scanlines       = NULL;


   tool->private = (void *) private;
 
   tool->button_press_func   = path_tool_button_press;
   tool->button_release_func = path_tool_button_release;
   tool->motion_func         = path_tool_motion;
   tool->cursor_update_func  = path_tool_cursor_update;
   tool->control_func        = path_tool_control;

   private->cur_path->curves    = NULL;
   private->cur_path->cur_curve = NULL;
   private->cur_path->name      = g_string_new("Path 0");
   private->cur_path->state     = 0;
   
   return tool;
}

void
tools_free_path_tool (Tool *tool)
{
   GDisplay * gdisp;
   PathTool * path_tool;

   path_tool = (PathTool *) tool->private;
   gdisp = (GDisplay *) tool->gdisp_ptr;

   if (tool->state == ACTIVE)
   {
      draw_core_stop (path_tool->core, tool);
   }
   
   path_free_path (path_tool->cur_path);

   draw_core_free (path_tool->core);
   
   g_free (path_tool);
}


/**************************************************************
 * Set of tools to determine, if the click was on an anchor
 */

typedef struct {
   Path *path;
   PathCurve *curve;
   PathSegment *segment;
   gint testx;
   gint testy;
   gint distance;
   gboolean found;
} Path_on_anchors_type;

/* This is a CurveTraverseFunc */
static void
path_tool_on_anchors_helper (Path *path, PathCurve *curve, PathSegment *segment, gpointer ptr)
{
   gint distance;
   Path_on_anchors_type *data = (Path_on_anchors_type *) ptr;

   if (segment && data && data->distance > 0)
   {
      distance = ((data->testx - segment->x) * (data->testx - segment->x) +
                  (data->testy - segment->y) * (data->testy - segment->y));

      if ( distance < data->distance )
      {
         data->path = path;
         data->curve = curve;
         data->segment = segment;
	 data->distance = distance;
	 data->found = TRUE;
      }
   }
}

static gboolean
path_tool_on_anchors (Tool *tool, gint x, gint y, gint halfwidth, Path **ret_pathP, PathCurve **ret_curveP, PathSegment **ret_segmentP)
{
   Path_on_anchors_type *data = g_new (Path_on_anchors_type, 1);
   gboolean ret_found;

   data->path = NULL;
   data->segment = NULL;
   data->segment = NULL;
   data->testx = x;
   data->testy = y;
   data->distance = halfwidth * halfwidth + 1;
   data->found = FALSE;

   path_traverse_path (((PathTool *) tool->private)->cur_path, NULL, path_tool_on_anchors_helper, NULL, data);

   if (ret_pathP) *ret_pathP = data->path;
   if (ret_curveP) *ret_curveP = data->curve;
   if (ret_segmentP) *ret_segmentP = data->segment;

   ret_found = data->found;

   g_free(data);

   return ret_found;

}


/**************************************************************
 * Set of tools to offset all active anchors
 */

typedef struct {
   gdouble dx;
   gdouble dy;
} Path_offset_active_type;

/* This is a CurveTraverseFunc */
static void
path_offset_active_helper (Path *path, PathCurve *curve, PathSegment *segment, gpointer ptr)
{
   Path_offset_active_type *data = (Path_offset_active_type *) ptr;

   if (segment && data && (segment->flags & SEGMENT_ACTIVE)) {
      segment->x += data->dx;
      segment->y += data->dy;
   }
}

static void
path_offset_active (Path *path, gdouble dx, gdouble dy)
{
   Path_offset_active_type *data = g_new (Path_offset_active_type, 1);
   data->dx = dx;
   data->dy = dy;
   
   if (path)
      path_traverse_path (path, NULL, path_offset_active_helper, NULL, data);
   
   g_free(data);
}


/**************************************************************
 * Set of tools to set the state of all anchors to inactive
 */

/* This is a CurveTraverseFunc */
static void
path_clear_active_helper (Path *path, PathCurve *curve, PathSegment *segment, gpointer ptr)
{
   gint distance;

   if (segment)
      segment->flags &= ~SEGMENT_ACTIVE;
}

static void
path_clear_active (Path *path, PathCurve *curve, PathSegment *segment)
{
   fprintf (stderr, "path_clear_active\n");
   if (segment)
      path_clear_active_helper (path, curve, segment, NULL);
   else if (curve)
      path_traverse_curve (path, curve, path_clear_active_helper, NULL, NULL);
   else if (path)
      path_traverse_path (path, NULL, path_clear_active_helper, NULL, NULL);
}


/**************************************************************
 * Set of tools to draw the segments to the window
 */

/* This is a CurveTraverseFunc */
static void
path_tool_draw_helper (Path *path, PathCurve *curve, PathSegment * segment, gpointer tool_ptr)
{
   Tool     * tool = (Tool *) tool_ptr;
   GDisplay * gdisp;
   PathTool * path_tool;
   DrawCore * core;
   gint x1, y1, x2, y2, draw;
   
   if (!tool) {
      fprintf (stderr, "Fatal Error: path_tool_draw_segment called without valid tool *\n");
      return;
   }
   
   gdisp = tool->gdisp_ptr;

   path_tool = (PathTool *) tool->private;
   core = path_tool->core;
   
   draw = 1;
   
   if (path_tool->draw & PATH_TOOL_REDRAW_ACTIVE)
   {
      if (segment->flags & SEGMENT_ACTIVE || (segment->next && segment->next->flags & SEGMENT_ACTIVE))
	 draw=1;
      else
	 draw=0;
   }

   if (segment && draw) 
   {
      gdisplay_transform_coords (gdisp, (gint) segment->x, (gint) segment->y, &x1, &y1, FALSE);
      if (segment->flags & SEGMENT_ACTIVE)
         gdk_draw_arc (core->win, core->gc, 0,
	               x1 - PATH_TOOL_HALFWIDTH, y1 - PATH_TOOL_HALFWIDTH,
		       PATH_TOOL_WIDTH, PATH_TOOL_WIDTH, 0, 23040);
      else
         gdk_draw_arc (core->win, core->gc, 1,
	               x1 - PATH_TOOL_HALFWIDTH, y1 - PATH_TOOL_HALFWIDTH,
		       PATH_TOOL_WIDTH, PATH_TOOL_WIDTH, 0, 23040);
   
      if (segment->next)
      {
         gdisplay_transform_coords (gdisp, (gint) segment->next->x, (gint) segment->next->y, &x2, &y2, FALSE);
         gdk_draw_line (core->win, core->gc, x1, y1, x2, y2);
      }
   }
   else if (!segment)
      fprintf(stderr, "path_tool_draw_segment: no segment to draw\n");
}

static void
path_tool_draw (Tool *tool)
{
   GDisplay * gdisp;
   Path * cur_path;
   PathTool * path_tool;
   PathCurve * cur_curve;
  
   fprintf (stderr, "path_tool_draw\n");
   
   gdisp = tool->gdisp_ptr;
   path_tool = tool->private;
   cur_path = path_tool->cur_path;
   
   path_traverse_path (cur_path, NULL, path_tool_draw_helper, NULL, tool);
}
