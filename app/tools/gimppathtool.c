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

#include <stdlib.h>

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "gdisplay.h"
#include "path_curves.h"
#include "gimpdrawtool.h"
#include "gimppathtool.h"
#include "tool_manager.h"
#include "tool_options.h"

#include "libgimp/gimpintl.h"

#include "path_tool.h"


/*  local function prototypes  */
static void   gimp_path_tool_class_init      (GimpPathToolClass *klass);
static void   gimp_path_tool_init            (GimpPathTool      *tool);

static void   gimp_path_tool_finalize        (GObject        *object);

static void   gimp_path_tool_control         (GimpTool       *tool,
                                              ToolAction      action,
                                              GDisplay       *gdisp);
static void   gimp_path_tool_button_press    (GimpTool       *tool,
                                              GdkEventButton *bevent,
                                              GDisplay       *gdisp);
static gint   gimp_path_tool_button_press_canvas (GimpPathTool *tool,
						  GdkEventButton *bevent,
						  GDisplay *gdisp);
static gint   gimp_path_tool_button_press_anchor (GimpPathTool *tool,
						  GdkEventButton *bevent,
						  GDisplay *gdisp);
static gint   gimp_path_tool_button_press_handle (GimpPathTool *tool,
						  GdkEventButton *bevent,
						  GDisplay *gdisp);
static gint   gimp_path_tool_button_press_curve  (GimpPathTool *tool,
						  GdkEventButton *bevent,
						  GDisplay *gdisp);

static void   gimp_path_tool_button_release  (GimpTool       *tool,
                                              GdkEventButton *bevent,
                                              GDisplay       *gdisp);

static void   gimp_path_tool_motion          (GimpTool       *tool,
                                              GdkEventMotion *mevent,
                                              GDisplay       *gdisp);
static void   gimp_path_tool_motion_anchor   (GimpPathTool *path_tool,
					      GdkEventMotion *mevent,
					      GDisplay *gdisp);
static void   gimp_path_tool_motion_handle   (GimpPathTool *path_tool,
					      GdkEventMotion *mevent,
					      GDisplay *gdisp);
static void   gimp_path_tool_motion_curve    (GimpPathTool *path_tool,
					      GdkEventMotion *mevent,
					      GDisplay *gdisp);

static void   gimp_path_tool_cursor_update   (GimpTool       *tool,
                                              GdkEventMotion *mevent,
                                              GDisplay       *gdisp);

static void   gimp_path_tool_draw            (GimpDrawTool   *draw_tool);




static GimpDrawToolClass *parent_class = NULL;

/*  the move tool options  */
static GimpToolOptions *path_options = NULL;


void
gimp_path_tool_register (Gimp *gimp)
{
  tool_manager_register_tool (gimp,
			      GIMP_TYPE_PATH_TOOL,
                              FALSE,
			      "gimp:path_tool",
			      _("Path Tool"),
			      _("Path tool prototype"),
			      N_("/Tools/Path"), NULL,
			      NULL, "tools/path.html",
			      GIMP_STOCK_TOOL_PATH);
}

GType
gimp_path_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpPathToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_path_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpPathTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_path_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_DRAW_TOOL,
					  "GimpPathTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_path_tool_class_init (GimpPathToolClass *klass)
{
  GObjectClass      *object_class;
  GimpToolClass     *tool_class;
  GimpDrawToolClass *draw_tool_class;

  object_class    = G_OBJECT_CLASS (klass);
  tool_class      = GIMP_TOOL_CLASS (klass);
  draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize     = gimp_path_tool_finalize;

  tool_class->control        = gimp_path_tool_control;
  tool_class->button_press   = gimp_path_tool_button_press;
  tool_class->button_release = gimp_path_tool_button_release;
  tool_class->motion         = gimp_path_tool_motion;
  tool_class->cursor_update  = gimp_path_tool_cursor_update;

  draw_tool_class->draw      = gimp_path_tool_draw;
}

static void
gimp_path_tool_init (GimpPathTool *path_tool)
{
  GimpTool *tool;

  tool = GIMP_TOOL (path_tool);
 
  /*  The tool options  */
  if (! path_options)
    {
      path_options = tool_options_new ();

      tool_manager_register_tool_options (GIMP_TYPE_PATH_TOOL,
                                          (GimpToolOptions *) path_options);
    }


  tool->preserve = TRUE;  /*  Preserve on drawable change  */

  path_tool->click_type       = ON_CANVAS;
  path_tool->click_x         = 0;
  path_tool->click_y         = 0;
  path_tool->click_halfwidth = 0;
  path_tool->click_modifier  = 0;
  path_tool->click_path      = NULL;
  path_tool->click_curve     = NULL;
  path_tool->click_segment   = NULL;
  path_tool->click_position  = -1;

  path_tool->active_count    = 0;
  path_tool->single_active_segment = NULL;

  path_tool->state           = 0;
  path_tool->draw            = PATH_TOOL_REDRAW_ALL;
  path_tool->cur_path        = g_new0(NPath, 1);
  path_tool->scanlines       = NULL;


  /* Initial Path */
  path_tool->cur_path->curves    = NULL;
  path_tool->cur_path->cur_curve = NULL;
  path_tool->cur_path->name      = g_string_new("Path 0");
  path_tool->cur_path->state     = 0;
  /* path_tool->cur_path->path_tool = path_tool; */

}

static void
gimp_path_tool_finalize (GObject *object)
{
  GimpPathTool *path_tool;

  path_tool = GIMP_PATH_TOOL (object);

#ifdef PATH_TOOL_DEBUG
  fprintf (stderr, "gimp_path_tool_free start\n");
#endif PATH_TOOL_DEBUG

  if (path_tool->cur_path)
    {
      path_free_path (path_tool->cur_path);
      path_tool->cur_path = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_path_tool_control (GimpTool   *tool,
                        ToolAction  action,
                        GDisplay   *gdisp)
{
  GimpPathTool *path_tool;

#ifdef PATH_TOOL_DEBUG
  fprintf (stderr, "path_tool_control\n");
#endif PATH_TOOL_DEBUG

  path_tool = GIMP_PATH_TOOL (tool);

  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      break;

    case HALT:
      tool->state = INACTIVE;
      break;

    default:
      break;
    }

  if (GIMP_TOOL_CLASS (parent_class)->control)
    GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}

static void
gimp_path_tool_button_press (GimpTool       *tool,
                             GdkEventButton *bevent,
                             GDisplay       *gdisp)
{
  GimpPathTool *path_tool = GIMP_PATH_TOOL (tool);
  gint grab_pointer=0;
  gdouble x, y;
  gint halfwidth, dummy;

#ifdef PATH_TOOL_DEBUG
  fprintf (stderr, "path_tool_button_press\n");
#endif PATH_TOOL_DEBUG

  /* Transform window-coordinates to canvas-coordinates */
  gdisplay_untransform_coords_f (gdisp, bevent->x, bevent->y, &x, &y, TRUE);
#ifdef PATH_TOOL_DEBUG
  fprintf(stderr, "Clickcoordinates %.2f, %.2f\n",x,y);
#endif PATH_TOOL_DEBUG
  path_tool->click_x = x;
  path_tool->click_y = y;
  path_tool->click_modifier = bevent->state;
  /* get halfwidth in image coord */
  gdisplay_untransform_coords (gdisp, bevent->x + PATH_TOOL_HALFWIDTH, 0, &halfwidth, &dummy, TRUE, 0);
  halfwidth -= x;
  path_tool->click_halfwidth = halfwidth;
  
  tool->gdisp = gdisp;
  tool->state = ACTIVE;

  if (!path_tool->cur_path->curves)
    gimp_draw_tool_start (GIMP_DRAW_TOOL(path_tool), gdisp->canvas->window);

  /* determine point, where clicked,
   * switch accordingly.
   */
 
  path_tool->click_type =
	       path_tool_cursor_position (path_tool->cur_path, x, y, halfwidth,
					  &(path_tool->click_path),
					  &(path_tool->click_curve),
					  &(path_tool->click_segment),
					  &(path_tool->click_position),
					  &(path_tool->click_handle_id));
 
  switch (path_tool->click_type)
  {
  case ON_CANVAS:
     grab_pointer = gimp_path_tool_button_press_canvas(path_tool, bevent, gdisp);
     break;

  case ON_ANCHOR:
     grab_pointer = gimp_path_tool_button_press_anchor(path_tool, bevent, gdisp);
     break;

  case ON_HANDLE:
     grab_pointer = gimp_path_tool_button_press_handle(path_tool, bevent, gdisp);
     break;

  case ON_CURVE:
     grab_pointer = gimp_path_tool_button_press_curve(path_tool, bevent, gdisp);
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


}

static gint
gimp_path_tool_button_press_anchor (GimpPathTool *path_tool,
                               GdkEventButton *bevent,
                               GDisplay *gdisp)
{
   static guint32 last_click_time=0;
   gboolean doubleclick=FALSE;
   
   NPath * cur_path = path_tool->cur_path;
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


   gimp_draw_tool_pause (GIMP_DRAW_TOOL(path_tool));
  
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
      /* Maybe CTRL-ALT Click should remove the whole curve? Or the active points? */
   }
   else if (!(path_tool->click_segment->flags & SEGMENT_ACTIVE))
   {
      path_set_flags (path_tool, cur_path, NULL, NULL, 0, SEGMENT_ACTIVE);
      path_set_flags (path_tool, cur_path, path_tool->click_curve, path_tool->click_segment, SEGMENT_ACTIVE, 0);
   }
   

   gimp_draw_tool_resume (GIMP_DRAW_TOOL(path_tool));

   return grab_pointer;
}


static gint
gimp_path_tool_button_press_handle (GimpPathTool *path_tool,
				    GdkEventButton *bevent,
				    GDisplay *gdisp)
{
   static guint32 last_click_time=0;
   gboolean doubleclick=FALSE;
   
   NPath * cur_path = path_tool->cur_path;
   gint grab_pointer;
   
#ifdef PATH_TOOL_DEBUG
   fprintf(stderr, "path_tool_button_press_handle:\n");
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

   return grab_pointer;
}

static gint
gimp_path_tool_button_press_canvas (GimpPathTool *path_tool,
				    GdkEventButton *bevent,
				    GDisplay *gdisp)
{
   
   NPath * cur_path = path_tool->cur_path;
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
   
   gimp_draw_tool_pause (GIMP_DRAW_TOOL(path_tool));
   
   if (path_tool->active_count == 1 && path_tool->single_active_segment != NULL
       && (path_tool->single_active_segment->prev == NULL || path_tool->single_active_segment->next == NULL)) {
      cur_segment = path_tool->single_active_segment;
      cur_curve = cur_segment->parent;

      path_set_flags (path_tool, cur_path, NULL, NULL, 0, SEGMENT_ACTIVE);

      if (cur_segment->next == NULL)
         cur_curve->cur_segment = path_append_segment(cur_path, cur_curve, SEGMENT_BEZIER, path_tool->click_x, path_tool->click_y);
      else
         cur_curve->cur_segment = path_prepend_segment(cur_path, cur_curve, SEGMENT_BEZIER, path_tool->click_x, path_tool->click_y);
      if (cur_curve->cur_segment) {
         path_set_flags (path_tool, cur_path, cur_curve, cur_curve->cur_segment, SEGMENT_ACTIVE, 0);
      }
   } else {
      if (path_tool->active_count == 0) {
         path_set_flags (path_tool, cur_path, NULL, NULL, 0, SEGMENT_ACTIVE);
         cur_path->cur_curve = path_add_curve(cur_path, path_tool->click_x, path_tool->click_y);
         path_set_flags (path_tool, cur_path, cur_path->cur_curve, cur_path->cur_curve->segments, SEGMENT_ACTIVE, 0);
      } else {
         path_set_flags (path_tool, cur_path, NULL, NULL, 0, SEGMENT_ACTIVE);
      }
   }
   
   gimp_draw_tool_resume (GIMP_DRAW_TOOL(path_tool));

   return 0;
}

static gint
gimp_path_tool_button_press_curve (GimpPathTool *path_tool,
				   GdkEventButton *bevent,
				   GDisplay *gdisp)
{
   NPath * cur_path = path_tool->cur_path;
   PathSegment * cur_segment;
   gint grab_pointer;
   
#ifdef PATH_TOOL_DEBUG
   fprintf(stderr, "path_tool_button_press_curve:\n");
#endif PATH_TOOL_DEBUG
   
   grab_pointer = 1;
   
   if (!cur_path) {
#ifdef PATH_TOOL_DEBUG
      fprintf (stderr, "Fatal error: No current NPath\n");
#endif PATH_TOOL_DEBUG
      return 0;
   }
   
   gimp_draw_tool_pause (GIMP_DRAW_TOOL(path_tool));
   
   if (path_tool->click_modifier & GDK_SHIFT_MASK) {
      cur_segment = path_curve_insert_anchor (path_tool->click_segment, path_tool->click_position);
      path_set_flags (path_tool, cur_path, NULL, NULL, 0, SEGMENT_ACTIVE);
      path_set_flags (path_tool, cur_path, path_tool->click_curve, cur_segment, SEGMENT_ACTIVE, 0);
      path_tool->click_type = ON_ANCHOR;
      path_tool->click_segment = cur_segment;

   } else {
      path_set_flags (path_tool, cur_path, NULL, NULL, 0, SEGMENT_ACTIVE);
      path_set_flags (path_tool, cur_path, path_tool->click_curve, path_tool->click_segment, SEGMENT_ACTIVE, 0);
      path_set_flags (path_tool, cur_path, path_tool->click_curve, path_tool->click_segment->next, SEGMENT_ACTIVE, 0);
   }
   gimp_draw_tool_resume (GIMP_DRAW_TOOL(path_tool));

   return 0;
}


static void
gimp_path_tool_button_release (GimpTool       *tool,
			       GdkEventButton *bevent,
			       GDisplay       *gdisp)
{
  GimpPathTool *path_tool = GIMP_PATH_TOOL (tool);

#ifdef PATH_TOOL_DEBUG
   fprintf (stderr, "path_tool_button_release\n");
#endif PATH_TOOL_DEBUG
 
  path_tool->state &= ~PATH_TOOL_DRAG;

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();
}


static void
gimp_path_tool_motion (GimpTool       *tool,
                       GdkEventMotion *mevent,
                       GDisplay       *gdisp)
{
  GimpPathTool *path_tool;

  path_tool = GIMP_PATH_TOOL (tool);

  if (gtk_events_pending()) return;
  
  switch (path_tool->click_type) {
  case ON_ANCHOR:
     gimp_path_tool_motion_anchor (path_tool, mevent, gdisp);
     break;
  case ON_HANDLE:
     gimp_path_tool_motion_handle (path_tool, mevent, gdisp);
     break;
  case ON_CURVE:
     gimp_path_tool_motion_curve (path_tool, mevent, gdisp);
     break;
  default:
     return;
  }
}



static void
gimp_path_tool_motion_anchor (GimpPathTool  *path_tool,
			      GdkEventMotion *mevent,
			      GDisplay       *gdisp)
{
   gdouble dx, dy, d;
   gdouble x,y;
   static gdouble dxsum = 0;
   static gdouble dysum = 0;
   
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

   gdisplay_untransform_coords_f (gdisp, mevent->x, mevent->y, &x, &y, TRUE);
   
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
   
   gimp_draw_tool_pause (GIMP_DRAW_TOOL(path_tool));

   path_offset_active (path_tool->cur_path, dx, dy);

   dxsum += dx;
   dysum += dy;

   gimp_draw_tool_resume (GIMP_DRAW_TOOL (path_tool));
   
   path_tool->draw &= ~PATH_TOOL_REDRAW_ACTIVE;

}

static void
gimp_path_tool_motion_handle (GimpPathTool   *path_tool,
			      GdkEventMotion *mevent,
			      GDisplay       *gdisp)
{
   gdouble dx, dy;
   gdouble x,y;
   static gdouble dxsum = 0;
   static gdouble dysum = 0;
   
   /*
    * Dont do anything, if the user clicked with pressed CONTROL-Key,
    * because he moved the handle to the anchor an anchor. 
    * XXX: Not yet! :-)
    */
   if (path_tool->click_modifier & GDK_CONTROL_MASK)
      return;
   
   if (!(path_tool->state & PATH_TOOL_DRAG))
   {
      path_tool->state |= PATH_TOOL_DRAG;
      dxsum = 0;
      dysum = 0;
   }

   gdisplay_untransform_coords_f (gdisp, mevent->x, mevent->y, &x, &y, TRUE);
   
   dx = x - path_tool->click_x - dxsum;
   dy = y - path_tool->click_y - dysum;
   
   path_tool->draw |= PATH_TOOL_REDRAW_ACTIVE;
   
   gimp_draw_tool_pause (GIMP_DRAW_TOOL(path_tool));

   path_curve_drag_handle (path_tool->click_segment, dx, dy, path_tool->click_handle_id);

   dxsum += dx;
   dysum += dy;

   gimp_draw_tool_resume (GIMP_DRAW_TOOL(path_tool));
   
   path_tool->draw &= ~PATH_TOOL_REDRAW_ACTIVE;

}
   

static void
gimp_path_tool_motion_curve (GimpPathTool   *path_tool,
			     GdkEventMotion *mevent,
			     GDisplay       *gdisp)
{
   gdouble dx, dy;
   gdouble x,y;
   static gdouble dxsum = 0;
   static gdouble dysum = 0;
   
   if (!(path_tool->state & PATH_TOOL_DRAG))
   {
      path_tool->state |= PATH_TOOL_DRAG;
      dxsum = 0;
      dysum = 0;
   }

   gdisplay_untransform_coords_f (gdisp, mevent->x, mevent->y, &x, &y, TRUE);
   
   dx = x - path_tool->click_x - dxsum;
   dy = y - path_tool->click_y - dysum;
   
   path_tool->draw |= PATH_TOOL_REDRAW_ACTIVE;
   
   gimp_draw_tool_pause (GIMP_DRAW_TOOL (path_tool));

   path_curve_drag_segment (path_tool->click_segment, path_tool->click_position, dx, dy);

   dxsum += dx;
   dysum += dy;

   gimp_draw_tool_resume (GIMP_DRAW_TOOL (path_tool));
   
   path_tool->draw &= ~PATH_TOOL_REDRAW_ACTIVE;

}
   



static void
gimp_path_tool_cursor_update (GimpTool       *tool,
			      GdkEventMotion *mevent,
			      GDisplay       *gdisp)
{
#if 0
  gint     x, y, halfwidth, dummy, cursor_location;
  
#idef PATH_TOOL_DEBUG
  /* fprintf (stderr, "path_tool_cursor_update\n");
   */
#edif PATH_TOOL_DEBUG

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &x, &y, TRUE, 0);
  /* get halfwidth in image coord */
  gdisplay_untransform_coords (gdisp, mevent->x + PATH_TOOL_HALFWIDTH, 0, &halfwidth, &dummy, TRUE, 0);
  halfwidth -= x;

  cursor_location = path_tool_cursor_position (tool, x, y, halfwidth, NULL, NULL, NULL, NULL, NULL);

  switch (cursor_location) {
  case ON_CANVAS:
     gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE1AP_CURSOR);
     break;
  case ON_ANCHOR:
     gdisplay_install_tool_cursor (gdisp, GDK_FLEUR);
     break;
  case ON_HANDLE:
     gdisplay_install_tool_cursor (gdisp, GDK_CROSSHAIR);
     break;
  case ON_CURVE:
     gdisplay_install_tool_cursor (gdisp, GDK_CROSSHAIR);
     break;
  default:
     gdisplay_install_tool_cursor (gdisp, GDK_QUESTION_ARROW);
     break;
  }

/* New Syntax */
  gdisplay_install_tool_cursor (gdisp,
				ctype,
				GIMP_MEASURE_TOOL_CURSOR,
				cmodifier);
#endif
}


/* This is a CurveTraverseFunc */
static void
gimp_path_tool_draw_helper (NPath *path,
    			    PathCurve *curve,
			    PathSegment *segment,
			    gpointer tool)
{
  GimpPathTool *path_tool;
  GimpDrawTool *draw_tool;
  gboolean draw = TRUE;

  path_tool = GIMP_PATH_TOOL (tool);
  draw_tool = GIMP_DRAW_TOOL (tool);
  
  if (path_tool->draw & PATH_TOOL_REDRAW_ACTIVE)
    draw = (segment->flags & SEGMENT_ACTIVE ||
	    (segment->next && segment->next->flags & SEGMENT_ACTIVE));

  if (segment->flags & SEGMENT_ACTIVE)
    gimp_draw_tool_draw_handle (draw_tool, segment->x, segment->y, PATH_TOOL_WIDTH, 2);
  else
    gimp_draw_tool_draw_handle (draw_tool, segment->x, segment->y, PATH_TOOL_WIDTH, 3);

  if (segment->next)
    path_curve_draw_segment (draw_tool, segment);
}
  

static void
gimp_path_tool_draw (GimpDrawTool *draw_tool)
{
  GimpPathTool *path_tool;

  path_tool = GIMP_PATH_TOOL (draw_tool);

  path_traverse_path (path_tool->click_path, NULL, gimp_path_tool_draw_helper, NULL, draw_tool);
}

