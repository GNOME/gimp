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
#include "libgimpwidgets/gimpwidgets.h"

#include "core/core-types.h"
#include "display/display-types.h"
#include "libgimptool/gimptooltypes.h"

#include "core/gimpimage.h"

#include "display/gimpdisplay.h"

#include "gimppathtool.h"
#include "path_tool.h"

#include "gimprc.h"
#include "path_curves.h"

#include "libgimp/gimpintl.h"


/*  local function prototypes  */

static void   gimp_path_tool_class_init          (GimpPathToolClass  *klass);
static void   gimp_path_tool_init                (GimpPathTool       *tool);

static void   gimp_path_tool_finalize            (GObject         *object);

static void   gimp_path_tool_control             (GimpTool        *tool,
                                                  GimpToolAction   action,
                                                  GimpDisplay     *gdisp);

static void   gimp_path_tool_button_press        (GimpTool        *tool,
                                                  GimpCoords      *coords,
                                                  guint32          time,
                                                  GdkModifierType  state,
                                                  GimpDisplay     *gdisp);
static gint   gimp_path_tool_button_press_canvas (GimpPathTool    *tool,
						  guint32          time,
						  GimpDisplay     *gdisp);
static gint   gimp_path_tool_button_press_anchor (GimpPathTool    *tool,
						  guint32          time,
						  GimpDisplay     *gdisp);
static gint   gimp_path_tool_button_press_handle (GimpPathTool    *tool,
						  guint32          time,
						  GimpDisplay     *gdisp);
static gint   gimp_path_tool_button_press_curve  (GimpPathTool    *tool,
						  guint32          time,
						  GimpDisplay     *gdisp);

static void   gimp_path_tool_button_release      (GimpTool        *tool,
                                                  GimpCoords      *coords,
                                                  guint32          time,
                                                  GdkModifierType  state,
                                                  GimpDisplay     *gdisp);

static void   gimp_path_tool_motion              (GimpTool        *tool,
                                                  GimpCoords      *coords,
                                                  guint32          time,
                                                  GdkModifierType  state,
                                                  GimpDisplay     *gdisp);
static void   gimp_path_tool_motion_anchor       (GimpPathTool    *path_tool,
                                                  GimpCoords      *coords,
                                                  GdkModifierType  state,
                                                  GimpDisplay     *gdisp);
static void   gimp_path_tool_motion_handle       (GimpPathTool    *path_tool,
                                                  GimpCoords      *coords,
                                                  GdkModifierType  state,
                                                  GimpDisplay     *gdisp);
static void   gimp_path_tool_motion_curve        (GimpPathTool    *path_tool,
                                                  GimpCoords      *coords,
                                                  GdkModifierType  state,
                                                  GimpDisplay     *gdisp);

static void   gimp_path_tool_cursor_update       (GimpTool        *tool,
                                                  GimpCoords      *coords,
                                                  GdkModifierType  state,
                                                  GimpDisplay     *gdisp);

static void   gimp_path_tool_draw                (GimpDrawTool    *draw_tool);



static GimpDrawToolClass *parent_class = NULL;


void
gimp_path_tool_register (GimpToolRegisterCallback  callback,
                         Gimp                     *gimp)
{
  (* callback) (GIMP_TYPE_PATH_TOOL,
                NULL,
                FALSE,
                "gimp-path-tool",
                _("Path Tool"),
                _("Path tool prototype"),
                N_("/Tools/Path"), NULL,
                NULL, "tools/path.html",
                GIMP_STOCK_TOOL_PATH,
                gimp);
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
  path_tool->cur_path        = g_new0 (NPath, 1);
  path_tool->scanlines       = NULL;


  /* Initial Path */
  path_tool->cur_path->curves    = NULL;
  path_tool->cur_path->cur_curve = NULL;
  path_tool->cur_path->name      = g_string_new ("Path 0");
  path_tool->cur_path->state     = 0;
  /* path_tool->cur_path->path_tool = path_tool; */

  tool->control = gimp_tool_control_new  (FALSE,                      /* scroll_lock */
                                          TRUE,                       /* auto_snap_to */
                                          TRUE,                       /* preserve */
                                          FALSE,                      /* handle_empty_image */
                                          FALSE,                      /* perfectmouse */
                                          GIMP_MOUSE_CURSOR,          /* cursor */
                                          GIMP_TOOL_CURSOR_NONE,      /* tool_cursor */
                                          GIMP_CURSOR_MODIFIER_NONE,  /* cursor_modifier */
                                          GIMP_MOUSE_CURSOR,          /* toggle_cursor */
                                          GIMP_TOOL_CURSOR_NONE,      /* toggle_tool_cursor */
                                          GIMP_CURSOR_MODIFIER_NONE   /* toggle_cursor_modifier */);
}

static void
gimp_path_tool_finalize (GObject *object)
{
  GimpPathTool *path_tool;

  path_tool = GIMP_PATH_TOOL (object);

#ifdef PATH_TOOL_DEBUG
  g_printerr ("gimp_path_tool_free start\n");
#endif

  if (path_tool->cur_path)
    {
      path_free_path (path_tool->cur_path);
      path_tool->cur_path = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_path_tool_control (GimpTool       *tool,
                        GimpToolAction  action,
                        GimpDisplay    *gdisp)
{
  GimpPathTool *path_tool;

#ifdef PATH_TOOL_DEBUG
  g_printerr ("path_tool_control\n");
#endif

  path_tool = GIMP_PATH_TOOL (tool);

  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      break;

    case HALT:
      gimp_tool_control_halt(tool->control);    /* sets paused_count to 0 -- is this ok? */
      break;

    default:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}

static void
gimp_path_tool_button_press (GimpTool        *tool,
                             GimpCoords      *coords,
                             guint32          time,
                             GdkModifierType  state,
                             GimpDisplay     *gdisp)
{
  GimpPathTool *path_tool;
  gint          grab_pointer = 0;
  gint          halfwidth, halfheight;

  path_tool = GIMP_PATH_TOOL (tool);

#ifdef PATH_TOOL_DEBUG
  g_printerr ("path_tool_button_press\n");
#endif

  path_tool->click_x        = coords->x;
  path_tool->click_y        = coords->y;
  path_tool->click_modifier = state;

  /* get halfwidth in image coord */
  halfwidth  = UNSCALEX (gdisp, PATH_TOOL_HALFWIDTH);
  halfheight = UNSCALEY (gdisp, PATH_TOOL_HALFWIDTH);

  path_tool->click_halfwidth  = halfwidth;
  path_tool->click_halfheight = halfheight;
  
  tool->gdisp = gdisp;
  gimp_tool_control_activate(tool->control);

  if (! path_tool->cur_path->curves)
    gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), gdisp);

  /* determine point, where clicked,
   * switch accordingly.
   */
 
  path_tool->click_type =
    path_tool_cursor_position (path_tool->cur_path,
                               coords->x,
                               coords->y,
                               halfwidth,
                               halfheight,
                               &path_tool->click_path,
                               &path_tool->click_curve,
                               &path_tool->click_segment,
                               &path_tool->click_position,
                               &path_tool->click_handle_id);
 
  switch (path_tool->click_type)
    {
    case ON_CANVAS:
      grab_pointer = gimp_path_tool_button_press_canvas (path_tool, time, gdisp);
      break;

    case ON_ANCHOR:
      grab_pointer = gimp_path_tool_button_press_anchor (path_tool, time, gdisp);
      break;

    case ON_HANDLE:
      grab_pointer = gimp_path_tool_button_press_handle (path_tool, time, gdisp);
      break;

    case ON_CURVE:
      grab_pointer = gimp_path_tool_button_press_curve (path_tool, time, gdisp);
      break;

    default:
      g_message("Huh? Whats happening here? (button_press_*)");
    }
}

static gint
gimp_path_tool_button_press_anchor (GimpPathTool *path_tool,
                                    guint32       time,
                                    GimpDisplay  *gdisp)
{
   static guint32 last_click_time=0;
   gboolean doubleclick=FALSE;
   
   NPath * cur_path = path_tool->cur_path;
   PathSegment *p_sas;
   gint grab_pointer;
   
#ifdef PATH_TOOL_DEBUG
   g_printerr ("path_tool_button_press_anchor:\n");
#endif
   
   grab_pointer = 1;

   if (!cur_path) {
#ifdef PATH_TOOL_DEBUG
      g_printerr ("Fatal error: No current Path\n");
#endif
      return 0;
   }
   
   /* 
    * We have to determine, if this was a doubleclick for ourself, because
    * disp_callback.c ignores the GDK_[23]BUTTON_EVENT's and adding them to
    * the switch statement confuses some tools.
    */
   if (time - last_click_time < 250) {
      doubleclick=TRUE;
#ifdef PATH_TOOL_DEBUG
      g_printerr ("Doppelclick!\n");
#endif
   } else
      doubleclick=FALSE;
   last_click_time = time;


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
   g_printerr ("p_sas: %p\n", p_sas);
#endif

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
                                    guint32       time,
				    GimpDisplay  *gdisp)
{
   static guint32 last_click_time=0;
   gboolean doubleclick=FALSE;
   
   NPath * cur_path = path_tool->cur_path;
   gint grab_pointer;
   
#ifdef PATH_TOOL_DEBUG
   g_printerr ("path_tool_button_press_handle:\n");
#endif
   
   grab_pointer = 1;

   if (!cur_path) {
#ifdef PATH_TOOL_DEBUG
      g_printerr ("Fatal error: No current Path\n");
#endif
      return 0;
   }
   
   /*   gint         click_halfwidth;

    * We have to determine, if this was a doubleclick for ourself, because
    * disp_callback.c ignores the GDK_[23]BUTTON_EVENT's and adding them to
    * the switch statement confuses some tools.
    */
   if (time - last_click_time < 250) {
      doubleclick=TRUE;
#ifdef PATH_TOOL_DEBUG
      g_printerr ("Doppelclick!\n");
#endif
   } else
      doubleclick=FALSE;
   last_click_time = time;

   return grab_pointer;
}

static gint
gimp_path_tool_button_press_canvas (GimpPathTool *path_tool,
                                    guint32       time,
				    GimpDisplay  *gdisp)
{
   NPath * cur_path = path_tool->cur_path;
   PathCurve * cur_curve;
   PathSegment * cur_segment;
   gint grab_pointer;
   
#ifdef PATH_TOOL_DEBUG
   g_printerr ("path_tool_button_press_canvas:\n");
#endif
   
   grab_pointer = 1;
   
   if (!cur_path) {
#ifdef PATH_TOOL_DEBUG
      g_printerr ("Fatal error: No current Path\n");
#endif
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
                                   guint32       time,
				   GimpDisplay  *gdisp)
{
   NPath * cur_path = path_tool->cur_path;
   PathSegment * cur_segment;
   gint grab_pointer;
   
#ifdef PATH_TOOL_DEBUG
   g_printerr ("path_tool_button_press_curve:\n");
#endif
   
   grab_pointer = 1;
   
   if (!cur_path) {
#ifdef PATH_TOOL_DEBUG
      g_printerr ("Fatal error: No current NPath\n");
#endif
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
gimp_path_tool_button_release (GimpTool        *tool,
                               GimpCoords      *coords,
                               guint32          time,
			       GdkModifierType  state,
			       GimpDisplay     *gdisp)
{
  GimpPathTool *path_tool;

  path_tool = GIMP_PATH_TOOL (tool);

#ifdef PATH_TOOL_DEBUG
   g_printerr ("path_tool_button_release\n");
#endif
 
  path_tool->state &= ~PATH_TOOL_DRAG;
}


static void
gimp_path_tool_motion (GimpTool        *tool,
                       GimpCoords      *coords,
                       guint32          time,
                       GdkModifierType  state,
                       GimpDisplay     *gdisp)
{
  GimpPathTool *path_tool;

  path_tool = GIMP_PATH_TOOL (tool);

  switch (path_tool->click_type)
    {
    case ON_ANCHOR:
      gimp_path_tool_motion_anchor (path_tool, coords, state, gdisp);
      break;
    case ON_HANDLE:
      gimp_path_tool_motion_handle (path_tool, coords, state, gdisp);
      break;
    case ON_CURVE:
      gimp_path_tool_motion_curve (path_tool, coords, state, gdisp);
      break;
    default:
      return;
    }
}


static void
gimp_path_tool_motion_anchor (GimpPathTool    *path_tool,
                              GimpCoords      *coords,
			      GdkModifierType  state,
			      GimpDisplay     *gdisp)
{
  static gdouble dxsum = 0;
  static gdouble dysum = 0;

  gdouble dx, dy, d;

  /* Dont do anything, if the user clicked with pressed CONTROL-Key,
   * because he deleted an anchor.
   */
  if (path_tool->click_modifier & GDK_CONTROL_MASK)
    return;

  if (! (path_tool->state & PATH_TOOL_DRAG))
    {
      path_tool->state |= PATH_TOOL_DRAG;
      dxsum = 0;
      dysum = 0;
    }

  dx = coords->x - path_tool->click_x - dxsum;
  dy = coords->y - path_tool->click_y - dysum;

  /* restrict to horizontal/vertical lines, if modifiers are pressed
   * I'm not sure, if this is intuitive for the user. Esp. When moving
   * an endpoint of an curve I'd expect, that the *line* is
   * horiz/vertical - not the delta to the point, where the point was
   * originally...
   */
  if (state & GDK_MOD1_MASK)
    {
      if (state & GDK_CONTROL_MASK)
        {
          d  = (fabs (dx) + fabs (dy)) / 2;  
          d  = (fabs (coords->x - path_tool->click_x) +
                fabs (coords->y - path_tool->click_y)) / 2;
          dx = ((coords->x < path_tool->click_x) ? -d : d ) - dxsum;
          dy = ((coords->y < path_tool->click_y) ? -d : d ) - dysum;
        }
      else
        {
          dx = - dxsum;
        }
    }
  else if (state & GDK_CONTROL_MASK)
    {
      dy = - dysum;
    }

  path_tool->draw |= PATH_TOOL_REDRAW_ACTIVE;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (path_tool));

  path_offset_active (path_tool->cur_path, dx, dy);

  dxsum += dx;
  dysum += dy;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (path_tool));

  path_tool->draw &= ~PATH_TOOL_REDRAW_ACTIVE;
}

static void
gimp_path_tool_motion_handle (GimpPathTool    *path_tool,
                              GimpCoords      *coords,
			      GdkModifierType  state,
			      GimpDisplay     *gdisp)
{
  static gdouble dxsum = 0;
  static gdouble dysum = 0;

  gdouble dx, dy;
   
  /* Dont do anything, if the user clicked with pressed CONTROL-Key,
   * because he moved the handle to the anchor an anchor. 
   * XXX: Not yet! :-)
   */
  if (path_tool->click_modifier & GDK_CONTROL_MASK)
    return;

  if (! (path_tool->state & PATH_TOOL_DRAG))
    {
      path_tool->state |= PATH_TOOL_DRAG;
      dxsum = 0;
      dysum = 0;
    }

  dx = coords->x - path_tool->click_x - dxsum;
  dy = coords->y - path_tool->click_y - dysum;

  path_tool->draw |= PATH_TOOL_REDRAW_ACTIVE;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (path_tool));

  path_curve_drag_handle (path_tool->click_segment,
                          dx, dy,
                          path_tool->click_handle_id);

  dxsum += dx;
  dysum += dy;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (path_tool));

  path_tool->draw &= ~PATH_TOOL_REDRAW_ACTIVE;
}

static void
gimp_path_tool_motion_curve (GimpPathTool    *path_tool,
                             GimpCoords      *coords,
			     GdkModifierType  state,
			     GimpDisplay     *gdisp)
{
  static gdouble dxsum = 0;
  static gdouble dysum = 0;

  gdouble dx, dy;
   
  if (! (path_tool->state & PATH_TOOL_DRAG))
    {
      path_tool->state |= PATH_TOOL_DRAG;
      dxsum = 0;
      dysum = 0;
    }

  dx = coords->x - path_tool->click_x - dxsum;
  dy = coords->y - path_tool->click_y - dysum;

  path_tool->draw |= PATH_TOOL_REDRAW_ACTIVE;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (path_tool));

  path_curve_drag_segment (path_tool->click_segment,
                           path_tool->click_position,
                           dx, dy);

  dxsum += dx;
  dysum += dy;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (path_tool));

  path_tool->draw &= ~PATH_TOOL_REDRAW_ACTIVE;
}

static void
gimp_path_tool_cursor_update (GimpTool        *tool,
                              GimpCoords      *coords,
			      GdkModifierType  state,
			      GimpDisplay     *gdisp)
{
  GimpCursorModifier cmodifier = GIMP_CURSOR_MODIFIER_NONE;
  gint               cursor_location;

  cursor_location = path_tool_cursor_position (GIMP_PATH_TOOL (tool)->cur_path,
                                               coords->x,
                                               coords->y,
                                               PATH_TOOL_HALFWIDTH,
                                               PATH_TOOL_HALFWIDTH,
                                               NULL, NULL, NULL, NULL, NULL);

  /* FIXME: add GIMP_PATH_TOOL_CURSOR */

  switch (cursor_location)
    {
    case ON_CANVAS:
      cmodifier = GIMP_CURSOR_MODIFIER_PLUS;
      break;
    case ON_ANCHOR:
      cmodifier = GIMP_CURSOR_MODIFIER_MOVE;
      break;
    case ON_HANDLE:
      cmodifier = GIMP_CURSOR_MODIFIER_MOVE;
      break;
    case ON_CURVE:
      cmodifier = GIMP_CURSOR_MODIFIER_NONE;
      break;
    default:
      g_warning ("gimp_path_tool_cursor_update(): bad cursor_location");
      break;
    }

  gimp_tool_control_set_cursor_modifier(tool->control, cmodifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
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
  gboolean      draw = TRUE;

  path_tool = GIMP_PATH_TOOL (tool);
  draw_tool = GIMP_DRAW_TOOL (tool);

  if (path_tool->draw & PATH_TOOL_REDRAW_ACTIVE)
    draw = (segment->flags & SEGMENT_ACTIVE ||
	    (segment->next && segment->next->flags & SEGMENT_ACTIVE));

  if (segment->flags & SEGMENT_ACTIVE)
    {
      gimp_draw_tool_draw_handle (draw_tool,
                                  GIMP_HANDLE_CIRCLE,
                                  segment->x, segment->y,
                                  PATH_TOOL_WIDTH,
                                  PATH_TOOL_WIDTH,
                                  GTK_ANCHOR_CENTER,
                                  FALSE);
    }
  else
    {
      gimp_draw_tool_draw_handle (draw_tool,
                                  GIMP_HANDLE_FILLED_CIRCLE,
                                  segment->x, segment->y,
                                  PATH_TOOL_WIDTH,
                                  PATH_TOOL_WIDTH,
                                  GTK_ANCHOR_CENTER,
                                  FALSE);
    }

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

