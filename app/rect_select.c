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
#include "gdk/gdkkeysyms.h"
#include "appenv.h"
#include "gdisplay.h"
#include "gimage_mask.h"
#include "edit_selection.h"
#include "floating_sel.h"
#include "rect_select.h"
#include "rect_selectP.h"
#include "selection_options.h"
#include "cursorutil.h"

#include "config.h"
#include "libgimp/gimpunitmenu.h"
#include "libgimp/gimpintl.h"

#define STATUSBAR_SIZE 128


/*  the rectangular selection tool options  */
static SelectionOptions *rect_options = NULL;

/*  in gimp, ellipses are rectangular, too ;)  */
extern SelectionOptions *ellipse_options;
extern void ellipse_select (GImage *, int, int, int, int, int, int, int, double);

static void selection_tool_update_op_state (RectSelect *rect_sel,
					    gint        x,
					    gint        y,
					    gint        state, 
					    GDisplay   *gdisp);

/*************************************/
/*  Rectangular selection apparatus  */

void
rect_select (GimpImage *gimage,
	     int        x,
	     int        y,
	     int        w,
	     int        h,
	     int        op,
	     int        feather,
	     double     feather_radius)
{
  Channel * new_mask;

  /*  if applicable, replace the current selection  */
  if (op == SELECTION_REPLACE)
    gimage_mask_clear (gimage);
  else
    gimage_mask_undo (gimage);

  /*  if feathering for rect, make a new mask with the
   *  rectangle and feather that with the old mask
   */
  if (feather)
    {
      new_mask = channel_new_mask (gimage, gimage->width, gimage->height);
      channel_combine_rect (new_mask, ADD, x, y, w, h);
      channel_feather (new_mask, gimage_get_mask (gimage),
		       feather_radius,
		       feather_radius,
		       op, 0, 0);
      channel_delete (new_mask);
    }
  else if (op == SELECTION_INTERSECT)
    {
      new_mask = channel_new_mask (gimage, gimage->width, gimage->height);
      channel_combine_rect (new_mask, ADD, x, y, w, h);
      channel_combine_mask (gimage_get_mask (gimage), new_mask, op, 0, 0);
      channel_delete (new_mask);
    }
  else
    channel_combine_rect (gimage_get_mask (gimage), op, x, y, w, h);
}

void
rect_select_button_press (Tool           *tool,
			  GdkEventButton *bevent,
			  gpointer        gdisp_ptr)
{
  GDisplay   *gdisp;
  RectSelect *rect_sel;
  gchar       select_mode[STATUSBAR_SIZE];
  gint        x, y;
  GimpUnit    unit = GIMP_UNIT_PIXEL;
  gdouble     unit_factor;

  gdisp = (GDisplay *) gdisp_ptr;
  rect_sel = (RectSelect *) tool->private;

  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y, &x, &y, TRUE, 0);

  rect_sel->x = x;
  rect_sel->y = y;
  switch (tool->type)
    {
    case RECT_SELECT:
      rect_sel->fixed_size = rect_options->fixed_size;
      rect_sel->fixed_width = rect_options->fixed_width;
      rect_sel->fixed_height = rect_options->fixed_height;
      unit = rect_options->fixed_unit;
      break;
    case ELLIPSE_SELECT:
      rect_sel->fixed_size = ellipse_options->fixed_size;
      rect_sel->fixed_width = ellipse_options->fixed_width;
      rect_sel->fixed_height = ellipse_options->fixed_height;
      unit = ellipse_options->fixed_unit;
      break;
    default:
      break;
    }

  switch (unit)
    {
    case GIMP_UNIT_PIXEL:
      break;
    case GIMP_UNIT_PERCENT:
      rect_sel->fixed_width =
	gdisp->gimage->width * rect_sel->fixed_width / 100;
      rect_sel->fixed_height =
	gdisp->gimage->height * rect_sel->fixed_height / 100;
      break;
    default:
      unit_factor = gimp_unit_get_factor (unit);
      rect_sel->fixed_width =
	 rect_sel->fixed_width * gdisp->gimage->xresolution / unit_factor;
      rect_sel->fixed_height =
	rect_sel->fixed_height * gdisp->gimage->yresolution / unit_factor;
      break;
    }

  rect_sel->fixed_width = MAX (1, rect_sel->fixed_width);
  rect_sel->fixed_height = MAX (1, rect_sel->fixed_height);

  rect_sel->w = 0;
  rect_sel->h = 0;

  rect_sel->center = FALSE;

  gdk_pointer_grab (gdisp->canvas->window, FALSE,
		    GDK_POINTER_MOTION_HINT_MASK |
		    GDK_BUTTON1_MOTION_MASK |
		    GDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, bevent->time);

  tool->state = ACTIVE;
  tool->gdisp_ptr = gdisp_ptr;

  switch (rect_sel->op)
    {
    case SELECTION_MOVE_MASK:
      init_edit_selection (tool, gdisp_ptr, bevent, MaskTranslate);
      return;
    case SELECTION_MOVE:
      init_edit_selection (tool, gdisp_ptr, bevent, MaskToLayerTranslate);
      return;
    default:
      break;
    }

  /* initialize the statusbar display */
  rect_sel->context_id =
    gtk_statusbar_get_context_id (GTK_STATUSBAR (gdisp->statusbar), "selection");
  switch (rect_sel->op)
    {
    case SELECTION_ADD:
      g_snprintf (select_mode, STATUSBAR_SIZE, _("Selection: ADD"));
      break;
    case SELECTION_SUB:
      g_snprintf (select_mode, STATUSBAR_SIZE, _("Selection: SUBTRACT"));
      break;
    case SELECTION_INTERSECT:
      g_snprintf (select_mode, STATUSBAR_SIZE, _("Selection: INTERSECT"));
      break;
    case SELECTION_REPLACE:
      g_snprintf (select_mode, STATUSBAR_SIZE, _("Selection: REPLACE"));
      break;
    default:
      break;
    }
  gtk_statusbar_push (GTK_STATUSBAR (gdisp->statusbar),
		      rect_sel->context_id, select_mode);

  draw_core_start (rect_sel->core, gdisp->canvas->window, tool);
}

void
rect_select_button_release (Tool           *tool,
			    GdkEventButton *bevent,
			    gpointer        gdisp_ptr)
{
  RectSelect * rect_sel;
  GDisplay * gdisp;
  int x1, y1, x2, y2, w, h;

  gdisp = (GDisplay *) gdisp_ptr;
  rect_sel = (RectSelect *) tool->private;

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  gtk_statusbar_pop (GTK_STATUSBAR (gdisp->statusbar), rect_sel->context_id);

  draw_core_stop (rect_sel->core, tool);
  tool->state = INACTIVE;

  /*  First take care of the case where the user "cancels" the action  */
  if (! (bevent->state & GDK_BUTTON3_MASK))
    {
      x1 = (rect_sel->w < 0) ? rect_sel->x + rect_sel->w : rect_sel->x;
      y1 = (rect_sel->h < 0) ? rect_sel->y + rect_sel->h : rect_sel->y;
      w = (rect_sel->w < 0) ? -rect_sel->w : rect_sel->w;
      h = (rect_sel->h < 0) ? -rect_sel->h : rect_sel->h;

     if ((!w || !h) && !rect_sel->fixed_size)
	{
	  /*  If there is a floating selection, anchor it  */
	  if (gimage_floating_sel (gdisp->gimage))
	    floating_sel_anchor (gimage_floating_sel (gdisp->gimage));
	  /*  Otherwise, clear the selection mask  */
	  else
	    gimage_mask_clear (gdisp->gimage);
	  
	  gdisplays_flush ();
	  return;
	}
      
      x2 = x1 + w;
      y2 = y1 + h;

      switch (tool->type)
	{
	case RECT_SELECT:
	  rect_select (gdisp->gimage,
		       x1, y1, (x2 - x1), (y2 - y1),
		       rect_sel->op,
		       rect_options->feather,
		       rect_options->feather_radius);
	  break;
	  
	case ELLIPSE_SELECT:
	  ellipse_select (gdisp->gimage,
			  x1, y1, (x2 - x1), (y2 - y1),
			  rect_sel->op,
			  ellipse_options->antialias,
			  ellipse_options->feather,
			  ellipse_options->feather_radius);
	  break;
	default:
	  break;
	}
	  
      /*  show selection on all views  */
      gdisplays_flush ();
   }
}

void
rect_select_motion (Tool           *tool,
		    GdkEventMotion *mevent,
		    gpointer        gdisp_ptr)
{
  RectSelect * rect_sel;
  GDisplay * gdisp;
  gchar size[STATUSBAR_SIZE];
  int ox, oy;
  int x, y;
  int w, h, s;
  int tw, th;
  double ratio;

  gdisp = (GDisplay *) gdisp_ptr;
  rect_sel = (RectSelect *) tool->private;

  /*  needed for immediate cursor update on modifier event  */
  rect_sel->current_x = mevent->x;
  rect_sel->current_y = mevent->y;

  if (tool->state != ACTIVE)
    return;

  draw_core_pause (rect_sel->core, tool);

  /* Calculate starting point */

  if (rect_sel->center)
    {
      ox = rect_sel->x + rect_sel->w / 2;
      oy = rect_sel->y + rect_sel->h / 2;
    }
  else
    {
      ox = rect_sel->x;
      oy = rect_sel->y;
    }

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &x, &y, TRUE, 0);
  if (rect_sel->fixed_size)
    {
      if (mevent->state & GDK_SHIFT_MASK)
	{
	  ratio = (double)(rect_sel->fixed_height /
			   (double)rect_sel->fixed_width);
	  tw = x - ox;
	  th = y - oy;
	  /* 
	   * This is probably an inefficient way to do it, but it gives
	   * nicer, more predictable results than the original agorithm
	   */
 
	  if ((abs(th) < (ratio * abs(tw))) && (abs(tw) > (abs(th) / ratio)))
	    {
	      w = tw;
	      h = (int)(tw * ratio);
	      /* h should have the sign of th */
	      if ((th < 0 && h > 0) || (th > 0 && h < 0))
		h = -h;
	    } 
	  else 
	    {
	      h = th;
	      w = (int)(th / ratio);
	      /* w should have the sign of tw */
	      if ((tw < 0 && w > 0) || (tw > 0 && w < 0))
		w = -w;
	    }
	}
      else
	{
	  w = (x - ox > 0 ? rect_sel->fixed_width  : -rect_sel->fixed_width);
	  h = (y - oy > 0 ? rect_sel->fixed_height : -rect_sel->fixed_height);
	}
    }
  else
    {
      w = (x - ox);
      h = (y - oy);
    }

  /*  If the shift key is down, then make the rectangle square (or ellipse circular) */
  if ((mevent->state & GDK_SHIFT_MASK) && !rect_sel->fixed_size)
    {
      s = MAX (abs (w), abs (h));
	  
      if (w < 0)
		w = -s;
      else
		w = s;

      if (h < 0)
		h = -s;
      else
		h = s;
    }

  /*  If the control key is down, create the selection from the center out */
  if (mevent->state & GDK_CONTROL_MASK)
    {
      if (rect_sel->fixed_size) 
	{
          if (mevent->state & GDK_SHIFT_MASK) 
            {
              rect_sel->x = ox - w;
              rect_sel->y = oy - h;
              rect_sel->w = w * 2;
              rect_sel->h = h * 2;
            }
          else
            {
              rect_sel->x = ox - w / 2;
              rect_sel->y = oy - h / 2;
              rect_sel->w = w;
              rect_sel->h = h;
            }
	} 
      else 
	{
	  w = abs(w);
	  h = abs(h);
	  
	  rect_sel->x = ox - w;
	  rect_sel->y = oy - h;
	  rect_sel->w = 2 * w + 1;
	  rect_sel->h = 2 * h + 1;
	}
      rect_sel->center = TRUE;
    }
  else
    {
      rect_sel->x = ox;
      rect_sel->y = oy;
      rect_sel->w = w;
      rect_sel->h = h;

      rect_sel->center = FALSE;
    }

  gtk_statusbar_pop (GTK_STATUSBAR (gdisp->statusbar), rect_sel->context_id);
  if (gdisp->dot_for_dot)
    {
      g_snprintf (size, STATUSBAR_SIZE, gdisp->cursor_format_str,
		  _("Selection: "), abs(rect_sel->w), " x ", abs(rect_sel->h));
    }
  else /* show real world units */
    {
      gdouble unit_factor = gimp_unit_get_factor (gdisp->gimage->unit);

      g_snprintf (size, STATUSBAR_SIZE, gdisp->cursor_format_str,
		  _("Selection: "),
		  (gdouble) abs(rect_sel->w) * unit_factor /
		  gdisp->gimage->xresolution,
		  " x ",
		  (gdouble) abs(rect_sel->h) * unit_factor /
		  gdisp->gimage->yresolution);
    }
  gtk_statusbar_push (GTK_STATUSBAR (gdisp->statusbar), rect_sel->context_id,
		      size);

  draw_core_resume (rect_sel->core, tool);
}

void
rect_select_draw (Tool *tool)
{
  GDisplay * gdisp;
  RectSelect * rect_sel;
  int x1, y1, x2, y2;

  gdisp = (GDisplay *) tool->gdisp_ptr;
  rect_sel = (RectSelect *) tool->private;

  x1 = MIN (rect_sel->x, rect_sel->x + rect_sel->w);
  y1 = MIN (rect_sel->y, rect_sel->y + rect_sel->h);
  x2 = MAX (rect_sel->x, rect_sel->x + rect_sel->w);
  y2 = MAX (rect_sel->y, rect_sel->y + rect_sel->h);

  gdisplay_transform_coords (gdisp, x1, y1, &x1, &y1, 0);
  gdisplay_transform_coords (gdisp, x2, y2, &x2, &y2, 0);

  gdk_draw_rectangle (rect_sel->core->win,
		      rect_sel->core->gc, 0,
		      x1, y1, (x2 - x1), (y2 - y1));
}

static void
selection_tool_update_op_state (RectSelect *rect_sel,
				gint        x,
				gint        y,
				gint        state,  
				GDisplay   *gdisp)
{
  if (active_tool->state == ACTIVE)
    return;

  if (state & GDK_MOD1_MASK &&
      !(layer_is_floating_sel (gimage_get_active_layer (gdisp->gimage))))
    rect_sel->op = SELECTION_MOVE_MASK;  /* move just the selection mask */
  else if (layer_is_floating_sel (gimage_get_active_layer (gdisp->gimage)) || 
	   (!(state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)) &&
	    gdisplay_mask_value (gdisp, x, y)))
    rect_sel->op = SELECTION_MOVE;      /* move the selection */
  else if ((state & GDK_SHIFT_MASK) &&
	   !(state & GDK_CONTROL_MASK))
    rect_sel->op = SELECTION_ADD;       /* add to the selection */
  else if ((state & GDK_CONTROL_MASK) &&
	   !(state & GDK_SHIFT_MASK))
    rect_sel->op = SELECTION_SUB;      /* subtract from the selection */
  else if ((state & GDK_CONTROL_MASK) &&
	   (state & GDK_SHIFT_MASK))
    rect_sel->op = SELECTION_INTERSECT;/* intersect with selection */
  else
    rect_sel->op = SELECTION_REPLACE;  /* replace the selection */
}

void
rect_select_oper_update  (Tool           *tool,
			  GdkEventMotion *mevent,
			  gpointer        gdisp_ptr)
{
  RectSelect *rect_sel;

  rect_sel = (RectSelect *) tool->private;
  selection_tool_update_op_state (rect_sel, mevent->x, mevent->y,
				  mevent->state, gdisp_ptr);
}

void
rect_select_modifier_update (Tool        *tool,
			     GdkEventKey *kevent,
			     gpointer     gdisp_ptr)
{
  RectSelect *rect_sel;
  gint state;

  state = kevent->state;

  switch (kevent->keyval)
    {
    case GDK_Alt_L: case GDK_Alt_R:
      if (state & GDK_MOD1_MASK)
	state &= ~GDK_MOD1_MASK;
      else
	state |= GDK_MOD1_MASK;
      break;

    case GDK_Shift_L: case GDK_Shift_R:
      if (state & GDK_SHIFT_MASK)
	state &= ~GDK_SHIFT_MASK;
      else
	state |= GDK_SHIFT_MASK;
      break;

    case GDK_Control_L: case GDK_Control_R:
      if (state & GDK_CONTROL_MASK)
	state &= ~GDK_CONTROL_MASK;
      else
	state |= GDK_CONTROL_MASK;
      break;
    }

  rect_sel = (RectSelect *) tool->private;
  selection_tool_update_op_state (rect_sel,
				  rect_sel->current_x, rect_sel->current_y,
				  state, gdisp_ptr);
}

void
rect_select_cursor_update (Tool           *tool,
			   GdkEventMotion *mevent,
			   gpointer        gdisp_ptr)
{
  int active;
  RectSelect *rect_sel;
  GDisplay *gdisp;
  gdisp = (GDisplay *)gdisp_ptr;
  active = (active_tool->state == ACTIVE);
  rect_sel = (RectSelect*)tool->private;

  switch (rect_sel->op)
    {
    case SELECTION_ADD:
      gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_ADD_CURSOR);
      break;
    case SELECTION_SUB:
      gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_SUBTRACT_CURSOR);
      break;
    case SELECTION_INTERSECT: 
      gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_INTERSECT_CURSOR);
      break;
    case SELECTION_REPLACE:
      gdisplay_install_tool_cursor (gdisp, GDK_TCROSS);
      break;
    case SELECTION_MOVE_MASK:
      gdisplay_install_tool_cursor (gdisp, GDK_DIAMOND_CROSS);
      break;
    case SELECTION_MOVE: 
      gdisplay_install_tool_cursor (gdisp, GDK_FLEUR);
    }
}

void
rect_select_control (Tool       *tool,
		     ToolAction  action,
		     gpointer    gdisp_ptr)
{
  RectSelect * rect_sel;

  rect_sel = (RectSelect *) tool->private;

  switch (action)
    {
    case PAUSE :
      draw_core_pause (rect_sel->core, tool);
      break;

    case RESUME :
      draw_core_resume (rect_sel->core, tool);
      break;

    case HALT :
      draw_core_stop (rect_sel->core, tool);
      break;

    default:
      break;
    }
}

static void
rect_select_options_reset ()
{
  selection_options_reset (rect_options);
}

Tool *
tools_new_rect_select ()
{
  Tool * tool;
  RectSelect * private;

  /*  The tool options  */
  if (! rect_options)
    {
      rect_options =
	selection_options_new (RECT_SELECT, rect_select_options_reset);
      tools_register (RECT_SELECT, (ToolOptions *) rect_options);
    }

  tool = tools_new_tool (RECT_SELECT);
  private = g_new (RectSelect, 1);

  private->core = draw_core_new (rect_select_draw);
  private->x = private->y = 0;
  private->w = private->h = 0;
  private->op = SELECTION_REPLACE;

  tool->private = (void *) private;

  tool->button_press_func   = rect_select_button_press;
  tool->button_release_func = rect_select_button_release;
  tool->motion_func         = rect_select_motion;
  tool->modifier_key_func   = rect_select_modifier_update;
  tool->cursor_update_func  = rect_select_cursor_update;
  tool->oper_update_func    = rect_select_oper_update;
  tool->control_func        = rect_select_control;

  return tool;
}

void
tools_free_rect_select (Tool *tool)
{
  RectSelect * rect_sel;

  rect_sel = (RectSelect *) tool->private;

  draw_core_free (rect_sel->core);
  g_free (rect_sel);
}
