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

#undef PATH_TOOL_DEBUG

#include <math.h>
/* #include "appenv.h"
 */

#include "draw_core.h"
#include "cursorutil.h"
#include "path_tool.h"
#include "path_toolP.h"

#ifdef PATH_TOOL_DEBUG
#include <stdio.h>
#endif PATH_TOOL_DEBUG

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
static PathSegment * path_append_segment  (Path *, PathCurve *, SegmentType, gint, gint);
static PathSegment * path_prepend_segment (Path *, PathCurve *, SegmentType, gint, gint);
static PathSegment * path_split_segment   (PathSegment *, gdouble);
static void          path_join_curves     (PathSegment *, PathSegment *);
static void          path_flip_curve      (PathCurve *);
static void          path_free_path       (Path *);
static void          path_free_curve      (PathCurve *);
static void          path_free_segment    (PathSegment *);
static void          path_delete_segment  (PathSegment *);
static void          path_print           (Path *);
static void          path_offset_active   (Path *, gdouble, gdouble);
static void          path_set_flags       (PathTool *, Path *, PathCurve *, PathSegment *, guint32, guint32);

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
 * These functions are for applying a function over a complete
 * path/curve/segment. They can pass information to each other
 * with a arbitrary data structure
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
#ifdef PATH_TOOL_DEBUG
   fprintf(stderr, "path_traverse_segment\n");
#endif PATH_TOOL_DEBUG

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

static PathCurve *path_add_curve (Path * cur_path, gint x, gint y)
{
   PathCurve * tmp = cur_path->curves;
   PathCurve * new_curve = NULL;

#ifdef PATH_TOOL_DEBUG
   fprintf(stderr, "path_add_curve\n");
#endif PATH_TOOL_DEBUG
   
   if (cur_path) {
      new_curve = g_new (PathCurve, 1);
   
      new_curve->parent = cur_path;
      new_curve->next = tmp;
      new_curve->prev = NULL;
      new_curve->cur_segment = NULL;
      new_curve->segments = NULL;

      if (tmp) tmp->prev = new_curve;
   
      cur_path->curves = cur_path->cur_curve = new_curve;
   
      new_curve->segments = path_prepend_segment (cur_path, new_curve, SEGMENT_LINE, x, y);
   }
#ifdef PATH_TOOL_DEBUG
   else
      fprintf (stderr, "Fatal Error: path_add_curve called without valid path\n");
#endif PATH_TOOL_DEBUG

   return new_curve;
}
   
   
static PathSegment * path_append_segment  (Path * cur_path, PathCurve * cur_curve, SegmentType type, gint x, gint y)
{
   PathSegment * tmp = cur_curve->segments;
   PathSegment * new_segment = NULL;
   
#ifdef PATH_TOOL_DEBUG
      fprintf(stderr, "path_append_segment\n");
#endif PATH_TOOL_DEBUG

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
         new_segment->flags = 0;
         new_segment->parent = cur_curve;
         new_segment->next = NULL;
         new_segment->prev = tmp;
         new_segment->data = NULL;
         
         if (tmp) 
            tmp->next = new_segment;

         cur_curve->cur_segment = new_segment;
      }
#ifdef PATH_TOOL_DEBUG
      else
         fprintf(stderr, "Fatal Error: path_append_segment called with a closed curve\n");
#endif PATH_TOOL_DEBUG
   }
#ifdef PATH_TOOL_DEBUG
   else
      fprintf(stderr, "Fatal Error: path_append_segment called without valid curve\n");
#endif PATH_TOOL_DEBUG
   
   return new_segment;
}


static PathSegment * path_prepend_segment  (Path * cur_path, PathCurve * cur_curve, SegmentType type, gint x, gint y)
{
   PathSegment * tmp = cur_curve->segments;
   PathSegment * new_segment = NULL;
   
#ifdef PATH_TOOL_DEBUG
      fprintf(stderr, "path_prepend_segment\n");
#endif PATH_TOOL_DEBUG

   if (cur_curve) {
      tmp = cur_curve->segments;
   
      if (tmp == NULL || tmp->prev == NULL) {
         new_segment = g_new (PathSegment, 1);
         
         new_segment->type = type;
         new_segment->x = x;
         new_segment->y = y;
         new_segment->flags = 0;
         new_segment->parent = cur_curve;
         new_segment->next = tmp;
         new_segment->prev = NULL;
         new_segment->data = NULL;
         
         if (tmp) 
            tmp->prev = new_segment;

         cur_curve->segments = new_segment;
         cur_curve->cur_segment = new_segment;
      }
#ifdef PATH_TOOL_DEBUG
      else
         fprintf(stderr, "Fatal Error: path_prepend_segment called with a closed curve\n");
#endif PATH_TOOL_DEBUG
   }
#ifdef PATH_TOOL_DEBUG
   else
      fprintf(stderr, "Fatal Error: path_prepend_segment called without valid curve\n");
#endif PATH_TOOL_DEBUG
   
   return new_segment;
}

/* static PathSegment * path_split_segment   (PathSegment *, gdouble);
 */

/*
 * Join two arbitrary endpoints and free the parent from the second
 * segment, if it differs from the first parents.
 */

static void
path_join_curves (PathSegment *segment1, PathSegment *segment2) {
   PathCurve *curve1, *curve2;
   PathSegment *tmp;

   if ((segment1->next && segment1->prev) || (segment2->next && segment2->prev)) {
#ifdef PATH_TOOL_DEBUG
      fprintf(stderr, "Fatal Error: path_join_curves called with a closed segment\n");
#endif PATH_TOOL_DEBUG
      return;
   }
   if (segment1->parent == segment2->parent) {
#ifdef PATH_TOOL_DEBUG
      fprintf (stderr, "Joining beginning and end of the same curve...\n");
#endif PATH_TOOL_DEBUG
      if (segment2->next == NULL) {
	 segment2->next = segment1;
	 segment1->prev = segment2;
      } else {
	 segment2->prev = segment1;
	 segment1->next = segment2;
      }
      return;
   }

   if (segment1->next == NULL && segment2->next == NULL) {
#ifdef PATH_TOOL_DEBUG
      fprintf (stderr, "Flipping second curve (next, next)...\n");
#endif PATH_TOOL_DEBUG
      path_flip_curve (segment2->parent);
      /* segment2 = segment2->parent->segments;
       */
   }
      
   if (segment1->prev == NULL && segment2->prev == NULL) {
#ifdef PATH_TOOL_DEBUG
      fprintf (stderr, "Flipping second curve (prev, prev)...\n");
#endif PATH_TOOL_DEBUG
      path_flip_curve (segment2->parent);
      /* segment2 = segment2->parent->segments;
       * while (segment2->next)
       * segment2 = segment2->next;
       */
   }
      
   if (segment1->next == NULL && segment2->prev == NULL) {
#ifdef PATH_TOOL_DEBUG
      fprintf (stderr, "Appending second to first curve...\n");
#endif PATH_TOOL_DEBUG
      curve1 = segment1->parent;
      curve2 = segment2->parent;
      
      segment1->next = segment2;
      segment2->prev = segment1;
      
      curve2->segments = NULL;
      
      if (curve2->prev)
	 curve2->prev->next = curve2->next;
      if (curve2->next)
	 curve2->next->prev = curve2->prev;
      
      if (curve2->parent->curves == curve2)
	 curve2->parent->curves = curve2->next;
      
      path_free_curve (curve2);
      
      tmp = segment2;
      
      while (tmp) {
	 tmp->parent = curve1;
	 tmp = tmp->next;
      }
      return;
   }

   if (segment1->prev == NULL && segment2->next == NULL) {
#ifdef PATH_TOOL_DEBUG
      fprintf (stderr, "Prepending second to first curve...\n");
#endif PATH_TOOL_DEBUG
      curve1 = segment1->parent;
      curve2 = segment2->parent;
      
      segment1->prev = segment2;
      segment2->next = segment1;
      
      curve2->segments = NULL;
      if (curve2->prev)
	 curve2->prev->next = curve2->next;
      if (curve2->next)
	 curve2->next->prev = curve2->prev;
      if (curve2->parent->curves == curve2)
	 curve2->parent->curves = curve2->next;
      path_free_curve (curve2);
      
      tmp = segment2;
      
      while (tmp) {
	 tmp->parent = curve1;
	 curve1->segments = tmp;
	 tmp = tmp->prev;
      }
      return;
   }

#ifdef PATH_TOOL_DEBUG
   fprintf (stderr, "Cant join these curves yet...\n");
   return;
#endif PATH_TOOL_DEBUG

}

/*
 * This function reverses the order of the anchors. This is
 * necessary for some joining operations.
 */
static void
path_flip_curve (PathCurve *curve)
{
   gpointer *end_data;
   SegmentType end_type;

   PathSegment *tmp, *tmp2;
   
   if (!curve && !curve->segments) {
#ifdef PATH_TOOL_DEBUG
      fprintf (stderr, "path_flip_curve: No curve o no segments to flip!\n");
#endif PATH_TOOL_DEBUG
      return;
   }
   
   tmp = curve->segments;

   while (tmp->next)
      tmp = tmp->next;
   
   end_data = tmp->data;
   end_type = tmp->type;

   tmp->parent->segments = tmp;
   
   while (tmp) {
      tmp2 = tmp->next;
      tmp->next = tmp->prev;
      tmp->prev = tmp2;
      if (tmp->next) {
	 tmp->type = tmp->next->type;
	 tmp->data = tmp->next->data;
      } else {
	 tmp->type = end_type;
	 tmp->data = end_data;
      }
      tmp = tmp->next;
   }
}


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

      /* break closed curves */
      if (tmp2 && tmp2->prev)
	 tmp2->prev->next = NULL;

      while ((tmp1 = tmp2) != NULL)
      {
         tmp2 = tmp1->next;
	 path_free_segment (tmp1);
      } 
      g_free(curve);
   }
}

static void
path_free_segment (PathSegment *segment)
{
   if (segment)
   {
      /* Clear the active flag to keep path_tool->single_active_segment consistent */
      path_set_flags (segment->parent->parent->path_tool, segment->parent->parent,
	              segment->parent, segment, 0, SEGMENT_ACTIVE);
      if (segment->data)
         g_free(segment->data);
      g_free (segment);
   }
}

static void
path_delete_curve (PathCurve *curve)
{
   if (curve)
   {
      if (curve->next)
	 curve->next->prev = curve->prev;
      if (curve->prev)
	 curve->prev->next = curve->next;
      
      if (curve == curve->parent->curves) {
	 curve->parent->curves = curve->next;
      }
      
      path_free_curve (curve);
   }
}

static void
path_delete_segment (PathSegment *segment)
{
   if (segment)
   {
      if (segment->next)
	 segment->next->prev = segment->prev;
      if (segment->prev)
	 segment->prev->next = segment->next;
      
      /* If the remaining curve is closed and has a
       * single point only, open it.
       */
      if (segment->next == segment->prev && segment->next)
	 segment->next->next = segment->next->prev = NULL;

      if (segment == segment->parent->segments)
	 segment->parent->segments = segment->next;

      if (segment->parent->segments == NULL)
   	 path_delete_curve (segment->parent);
      
      path_free_segment (segment);

      /*
       * here we have to update the surrounding segments
       */
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

#ifdef PATH_TOOL_DEBUG
   fprintf (stderr, "path_tool_button_press\n");
#endif PATH_TOOL_DEBUG

   gdisp = (GDisplay *) gdisp_ptr;
   path_tool = (PathTool *) tool->private;
   tool->gdisp_ptr = gdisp_ptr;
  
   /* Transform window-coordinates to canvas-coordinates */
   gdisplay_untransform_coords (gdisp, bevent->x, bevent->y, &x, &y, TRUE, 0);
#ifdef PATH_TOOL_DEBUG
   fprintf(stderr, "Clickcoordinates %d, %d\n",x,y);
#endif PATH_TOOL_DEBUG
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
   static guint32 last_click_time=0;
   gboolean doubleclick=FALSE;
   PathTool *path_tool = tool->private;
   
   Path * cur_path = path_tool->cur_path;
   PathSegment *p_sas;
   gint grab_pointer;
   
#ifdef PATH_TOOL_DEBUG
   fprintf(stderr, "path_tool_button_press_anchor:\n");
#endif PATH_TOOL_DEBUG
   
   grab_pointer = 1;

   if (!cur_path) {
#ifdef PATH_TOOL_DEBUG
      fprintf (stderr, "Fatal error: No current Path\n");
#endif PATH_TOOL_DEBUG
      return 0;
   }
   
   /* 
    * We have to determine, if this was a doubleclick for ourself, because
    * disp_callback.c ignores the GDK_[23]BUTTON_EVENT's and adding them to
    * the switch statement confuses some tools.
    */
   if (bevent->time - last_click_time < 250) {
      doubleclick=TRUE;
#ifdef PATH_TOOL_DEBUG
      fprintf (stderr, "Doppelclick!\n");
#endif PATH_TOOL_DEBUG
   } else
      doubleclick=FALSE;
   last_click_time = bevent->time;


   draw_core_pause (path_tool->core, tool);
  
   /* The user pressed on an anchor:
    *  normally this activates this anchor
    *  + SHIFT toggles the activity of an anchor.
    *  if this anchor is at the end of an open curve and the other
    *  end is active, close the curve.
    *  
    *  Doubleclick (de)activates the whole curve (not Path!).
    */

   p_sas = path_tool->single_active_segment;

#ifdef PATH_TOOL_DEBUG
   fprintf (stderr, "p_sas: %p\n", p_sas);
#endif PATH_TOOL_DEBUG

   if (path_tool->click_modifier & GDK_SHIFT_MASK) {
      if (path_tool->active_count == 1 && p_sas && p_sas != path_tool->click_segment &&
          (p_sas->next == NULL || p_sas->prev == NULL) &&
	  (path_tool->click_segment->next == NULL || path_tool->click_segment->prev == NULL)) {
	 /*
	  * if this is the end of an open curve and the single active segment was another
	  * open end, connect those ends.
	  */
	 path_join_curves (path_tool->click_segment, p_sas);
	 path_set_flags (path_tool, path_tool->click_path, path_tool->click_curve,
	                 NULL, 0, SEGMENT_ACTIVE);
      }

      if (doubleclick)
	 /*
	  * Doubleclick set the whole curve to the same state, depending on the
	  * state of the clicked anchor.
	  */
	 if (path_tool->click_segment->flags & SEGMENT_ACTIVE)
	    path_set_flags (path_tool, path_tool->click_path, path_tool->click_curve,
		            NULL, SEGMENT_ACTIVE, 0);
	 else
	    path_set_flags (path_tool, path_tool->click_path, path_tool->click_curve,
     		            NULL, 0, SEGMENT_ACTIVE);
      else
	 /*
	  * Toggle the state of the clicked anchor.
	  */
	 if (path_tool->click_segment->flags & SEGMENT_ACTIVE)
	    path_set_flags (path_tool, path_tool->click_path, path_tool->click_curve,
	 	            path_tool->click_segment, 0, SEGMENT_ACTIVE);
	 else
	    path_set_flags (path_tool, path_tool->click_path, path_tool->click_curve,
		            path_tool->click_segment, SEGMENT_ACTIVE, 0);
   }
   /*
    * Delete anchors, when CONTROL is pressed
    */
   else if (path_tool->click_modifier & GDK_CONTROL_MASK)
   {
      if (path_tool->click_segment->flags & SEGMENT_ACTIVE)
      {
	 if (path_tool->click_segment->prev)
	    path_set_flags (path_tool, path_tool->click_path, path_tool->click_curve,
   		            path_tool->click_segment->prev, SEGMENT_ACTIVE, 0);
     	 else if (path_tool->click_segment->next)
   	    path_set_flags (path_tool, path_tool->click_path, path_tool->click_curve,
		            path_tool->click_segment->next, SEGMENT_ACTIVE, 0);
      }

      path_delete_segment (path_tool->click_segment);
      path_tool->click_segment = NULL;
   }
   else if (!(path_tool->click_segment->flags & SEGMENT_ACTIVE))
   {
      path_set_flags (path_tool, cur_path, NULL, NULL, 0, SEGMENT_ACTIVE);
      path_set_flags (path_tool, cur_path, path_tool->click_curve, path_tool->click_segment, SEGMENT_ACTIVE, 0);
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
   
#ifdef PATH_TOOL_DEBUG
   fprintf(stderr, "path_tool_button_press_canvas:\n");
#endif PATH_TOOL_DEBUG
   
   grab_pointer = 1;
   
   if (!cur_path) {
#ifdef PATH_TOOL_DEBUG
      fprintf (stderr, "Fatal error: No current Path\n");
#endif PATH_TOOL_DEBUG
      return 0;
   }
   
   draw_core_pause (path_tool->core, tool);
   
   if (path_tool->active_count == 1 && path_tool->single_active_segment != NULL
       && (path_tool->single_active_segment->prev == NULL || path_tool->single_active_segment->next == NULL)) {
      cur_segment = path_tool->single_active_segment;
      cur_curve = cur_segment->parent;

      path_set_flags (path_tool, cur_path, NULL, NULL, 0, SEGMENT_ACTIVE);

      if (cur_segment->next == NULL)
	 cur_curve->cur_segment = path_append_segment(cur_path, cur_curve, SEGMENT_LINE, path_tool->click_x, path_tool->click_y);
      else
	 cur_curve->cur_segment = path_prepend_segment(cur_path, cur_curve, SEGMENT_LINE, path_tool->click_x, path_tool->click_y);
      if (cur_curve->cur_segment) {
	 path_set_flags (path_tool, cur_path, cur_curve, cur_curve->cur_segment, SEGMENT_ACTIVE, 0);
      }
   } else {
      path_set_flags (path_tool, cur_path, NULL, NULL, 0, SEGMENT_ACTIVE);
      cur_path->cur_curve = path_add_curve(cur_path, path_tool->click_x, path_tool->click_y);
      path_set_flags (path_tool, cur_path, cur_path->cur_curve, cur_path->cur_curve->segments, SEGMENT_ACTIVE, 0);

   }
   
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
 
#ifdef PATH_TOOL_DEBUG
   fprintf (stderr, "path_tool_button_release\n");
#endif PATH_TOOL_DEBUG
 
   gdisp = (GDisplay *) gdisp_ptr;
   path_tool = (PathTool *) tool->private;
   
   path_tool->state &= ~PATH_TOOL_DRAG;

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

   if (gtk_events_pending()) return;
   
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
   gdouble dx, dy, d;
   gint x,y;
   static gint dxsum = 0;
   static gint dysum = 0;
   
   path_tool = (PathTool *) tool->private;
   
   /*
    * Dont do anything, if the user clicked with pressed CONTROL-Key,
    * because he deleted an anchor.
    */
   if (path_tool->click_modifier & GDK_CONTROL_MASK)
      return;
   
   if (!(path_tool->state & PATH_TOOL_DRAG))
   {
      path_tool->state |= PATH_TOOL_DRAG;
      dxsum = 0;
      dysum = 0;
   }

   gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &x, &y, TRUE, 0);
   
   dx = x - path_tool->click_x - dxsum;
   dy = y - path_tool->click_y - dysum;
   
   /* restrict to horizontal/vertical lines, if modifiers are pressed
    * I'm not sure, if this is intuitive for the user. Esp. When moving
    * an endpoint of an curve I'd expect, that the *line* is
    * horiz/vertical - not the delta to the point, where the point was
    * originally...
    */
   if (mevent->state & GDK_MOD1_MASK)
   {
      if (mevent->state & GDK_CONTROL_MASK)
      {
	 d  = (fabs(dx) + fabs(dy)) / 2;  
	 d  = (fabs(x - path_tool->click_x) + fabs(y - path_tool->click_y)) / 2;  
	 
	 dx = ((x < path_tool->click_x) ? -d : d ) - dxsum;
	 dy = ((y < path_tool->click_y) ? -d : d ) - dysum;
      }
      else
	 dx = - dxsum;
   }
   else if (mevent->state & GDK_CONTROL_MASK)
      dy = - dysum;
   

   path_tool->draw |= PATH_TOOL_REDRAW_ACTIVE;
   
   draw_core_pause(path_tool->core, tool);

   path_offset_active (path_tool->cur_path, dx, dy);

   dxsum += dx;
   dysum += dy;

   draw_core_resume (path_tool->core, tool);
   
   path_tool->draw &= ~PATH_TOOL_REDRAW_ACTIVE;

}
   

static void
path_tool_cursor_update (Tool           *tool,
			 GdkEventMotion *mevent,
			 gpointer        gdisp_ptr)
{
   PathTool *path_tool;
   GDisplay *gdisp;
   gint     x, y, halfwidth, dummy, cursor_location;
  
#ifdef PATH_TOOL_DEBUG
   /* fprintf (stderr, "path_tool_cursor_update\n");
    */
#endif PATH_TOOL_DEBUG

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

#ifdef PATH_TOOL_DEBUG
   fprintf (stderr, "path_tool_control\n");
#endif PATH_TOOL_DEBUG

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

   private->active_count    = 0;
   private->single_active_segment = NULL;

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
   private->cur_path->path_tool = private;
   
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
 * Set of function to determine, if the click was on an anchor
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
 * Set of function to offset all active anchors
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
 * Set of function to set the state of all anchors to inactive
 */

typedef struct {
   guint32 bits_set;
   guint32 bits_clear;
   PathTool *path_tool;
} Path_set_flags_type;

/* This is a CurveTraverseFunc */
static void
path_set_flags_helper (Path *path, PathCurve *curve, PathSegment *segment, gpointer ptr)
{
   Path_set_flags_type *tmp = (Path_set_flags_type *) ptr;
   guint32 oldflags;
   guint tmp_uint;

   if (segment) {
      oldflags = segment->flags;
      segment->flags &= ~(tmp->bits_clear);
      segment->flags |= tmp->bits_set;

      /* 
       * Some black magic: We try to remember, which is the single active segment.
       * We count, how many segments are active (in path_tool->active_count) and
       * XOR path_tool->single_active_segment every time we select or deselect
       * an anchor. So if exactly one anchor is active, path_tool->single_active_segment
       * points to it.
       */

      /* If SEGMENT_ACTIVE state has changed change the PathTool data accordingly.*/
      if (((segment->flags ^ oldflags) & SEGMENT_ACTIVE) && tmp && tmp->path_tool) {
	 if (segment->flags & SEGMENT_ACTIVE)
	    tmp->path_tool->active_count++;
	 else
	    tmp->path_tool->active_count--;

	 /* Does this work on all (16|32|64)-bit Machines? */

	 tmp_uint = GPOINTER_TO_UINT(tmp->path_tool->single_active_segment);
         tmp_uint ^= GPOINTER_TO_UINT(segment);
	 tmp->path_tool->single_active_segment = GUINT_TO_POINTER(tmp_uint);
      }
   }
}

static void
path_set_flags (PathTool *path_tool, Path *path, PathCurve *curve, PathSegment *segment, guint32 bits_set, guint32 bits_clear)
{
   Path_set_flags_type *tmp = g_new (Path_set_flags_type, 1);
   tmp->bits_set=bits_set;
   tmp->bits_clear=bits_clear;
   tmp->path_tool = path_tool;

   if (segment)
      path_set_flags_helper (path, curve, segment, tmp);
   else if (curve)
      path_traverse_curve (path, curve, path_set_flags_helper, NULL, tmp);
   else if (path)
      path_traverse_path (path, NULL, path_set_flags_helper, NULL, tmp);

   g_free (tmp);
}


/**************************************************************
 * Set of functions to draw the segments to the window
 */

/* This is a CurveTraverseFunc */
static void
path_tool_draw_helper (Path *path, PathCurve *curve, PathSegment * segment, gpointer tool_ptr)
{
   Tool     * tool = (Tool *) tool_ptr;
   GDisplay * gdisp;
   PathTool * path_tool;
   DrawCore * core;
   gint x1, y1, x2, y2;
   gboolean draw = TRUE;
   
   if (!tool) {
#ifdef PATH_TOOL_DEBUG
      fprintf (stderr, "Fatal Error: path_tool_draw_segment called without valid tool *\n");
#endif PATH_TOOL_DEBUG
      return;
   }
   
   gdisp = tool->gdisp_ptr;

   path_tool = (PathTool *) tool->private;
   core = path_tool->core;
   
   if (path_tool->draw & PATH_TOOL_REDRAW_ACTIVE)
      draw = (segment->flags & SEGMENT_ACTIVE || (segment->next && segment->next->flags & SEGMENT_ACTIVE));

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
#ifdef PATH_TOOL_DEBUG
   else if (!segment)
      fprintf(stderr, "path_tool_draw_segment: no segment to draw\n");
#endif PATH_TOOL_DEBUG
}

static void
path_tool_draw (Tool *tool)
{
   GDisplay * gdisp;
   Path * cur_path;
   PathTool * path_tool;
   PathCurve * cur_curve;
  
#ifdef PATH_TOOL_DEBUG
   fprintf (stderr, "path_tool_draw\n");
#endif PATH_TOOL_DEBUG
   
   gdisp = tool->gdisp_ptr;
   path_tool = tool->private;
   cur_path = path_tool->cur_path;
   
   path_traverse_path (cur_path, NULL, path_tool_draw_helper, NULL, tool);
   
}
