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
#include "appenv.h"
#include "draw_core.h"
#include "edit_selection.h"
#include "errors.h"
#include "floating_sel.h"
#include "gimage_mask.h"
#include "gdisplay.h"
#include "gdisplay_ops.h"
#include "move.h"
#include "undo.h"

#include "libgimp/gimpintl.h"

/*  the move structures  */

typedef struct _MoveTool MoveTool;
struct _MoveTool
{
  Layer *layer;
  Guide *guide;
  GDisplay *disp;
};


/*  the move tool options  */
static ToolOptions *move_options = NULL;

/*  local variables  */
static GdkGC *move_gc = NULL;


/*  move tool action functions  */

static void   move_tool_button_press      (Tool *, GdkEventButton *, gpointer);
static void   move_tool_button_release    (Tool *, GdkEventButton *, gpointer);
static void   move_tool_motion            (Tool *, GdkEventMotion *, gpointer);
static void   move_tool_cursor_update     (Tool *, GdkEventMotion *, gpointer);
static void   move_tool_control		  (Tool *, int, gpointer);
static void   move_create_gc              (GDisplay *);


/*  move action functions  */

static void
move_tool_button_press (Tool           *tool,
			GdkEventButton *bevent,
			gpointer        gdisp_ptr)
{
  GDisplay * gdisp;
  MoveTool * move;
  Layer * layer;
  Guide * guide;
  int x, y;

  gdisp = (GDisplay *) gdisp_ptr;
  move = (MoveTool *) tool->private;

  tool->gdisp_ptr = gdisp_ptr;
  move->layer = NULL;
  move->guide = NULL;
  move->disp = NULL;

  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y, &x, &y, FALSE, FALSE);
 
  if (bevent->state & GDK_MOD1_MASK)
    {
      init_edit_selection (tool, gdisp_ptr, bevent, MaskTranslate);
      tool->state = ACTIVE;
    }
  else if (bevent->state & GDK_SHIFT_MASK)
    {
      init_edit_selection (tool, gdisp_ptr, bevent, LayerTranslate);
      tool->state = ACTIVE;
    }
  else
    {
      if (gdisp->draw_guides && (guide = gdisplay_find_guide (gdisp, bevent->x, bevent->y)))
	{
	  undo_push_guide (gdisp->gimage, guide);

	  move->guide = NULL;

	  gdisplays_expose_guide (gdisp->gimage, guide);
	  gimage_remove_guide (gdisp->gimage, guide);
	  gdisplay_flush (gdisp);
	  gimage_add_guide (gdisp->gimage, guide);

	  move->guide = guide;
	  move->disp = gdisp;

	  tool->scroll_lock = TRUE;
	  tool->state = ACTIVE;

	  move_tool_motion (tool, NULL, gdisp);
	}
      else if ((layer = gimage_pick_correlate_layer (gdisp->gimage, x, y)))
	{
	  /*  If there is a floating selection, and this aint it, use the move tool  */
	  if (gimage_floating_sel (gdisp->gimage) && !layer_is_floating_sel (layer))
	    move->layer = gimage_floating_sel (gdisp->gimage);
	  /*  Otherwise, init the edit selection  */
	  else
	    {
	      gimage_set_active_layer (gdisp->gimage, layer);
	      init_edit_selection (tool, gdisp_ptr, bevent, LayerTranslate);
	    }
	  tool->state = ACTIVE;
	}
    }

  /* if we've got an active tool grab the pointer */
  if (tool->state == ACTIVE)      
    gdk_pointer_grab (gdisp->canvas->window, FALSE,
		      GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
		      NULL, NULL, bevent->time);
    
}

static void
move_draw_guide (GDisplay *gdisp, Guide *guide)
{
  int x1, y1;
  int x2, y2;
  int w, h;
  int x, y;

  if (!move_gc)
    move_create_gc (gdisp);

  if (guide->position == -1)
    return;
  
  gdisplay_transform_coords (gdisp, gdisp->gimage->width, 
			     gdisp->gimage->height, &x2, &y2, FALSE); 

  gdk_window_get_size (gdisp->canvas->window, &w, &h);

  switch (guide->orientation) {
  case HORIZONTAL_GUIDE:
    gdisplay_transform_coords (gdisp, 0, guide->position, &x1, &y, FALSE);
    if (x1 < 0) x1 = 0;
    if (x2 > w) x2 = w;

    gdk_draw_line (gdisp->canvas->window, move_gc, x1, y, x2, y); 
    break;
  case VERTICAL_GUIDE:
    gdisplay_transform_coords (gdisp, guide->position, 0, &x, &y1, FALSE);
    if (y1 < 0) y1 = 0;
    if (y2 > h) y2 = h;

    gdk_draw_line (gdisp->canvas->window, move_gc, x, y1, x, y2);
    break;
  default:
    g_warning ("mdg / BAD FALLTHROUGH");
  }
}

static void
move_tool_button_release (Tool           *tool,
			  GdkEventButton *bevent,
			  gpointer        gdisp_ptr)
{
  MoveTool * move;
  GDisplay * gdisp;
  int remove_guide;
  int x1, y1;
  int x2, y2;

  gdisp = (GDisplay *) gdisp_ptr;
  move = (MoveTool *) tool->private;

  gdk_flush ();

  tool->state = INACTIVE;
  gdk_pointer_ungrab (bevent->time);

  if (move->guide)
    {
      tool->scroll_lock = FALSE;

      remove_guide = FALSE;
      gdisplay_untransform_coords (gdisp, 0, 0, &x1, &y1, FALSE, FALSE);
      gdisplay_untransform_coords (gdisp, gdisp->disp_width, gdisp->disp_height, &x2, &y2, FALSE, FALSE);

      if (x1 < 0) x1 = 0;
      if (y1 < 0) y1 = 0;
      if (x2 > gdisp->gimage->width) x2 = gdisp->gimage->width;
      if (y2 > gdisp->gimage->height) y2 = gdisp->gimage->height;

      switch (move->guide->orientation)
	{
	case HORIZONTAL_GUIDE:
	  if ((move->guide->position < y1) || (move->guide->position > y2))
	    remove_guide = TRUE;
	  break;
	case VERTICAL_GUIDE:
	  if ((move->guide->position < x1) || (move->guide->position > x2))
	    remove_guide = TRUE;
	  break;
	}

      gdisplays_expose_guide (gdisp->gimage, move->guide);

      if (remove_guide)
	{
	  move_draw_guide (gdisp, move->guide);
	  move->guide->position = -1;
	  move->guide = NULL;
	  move->disp = NULL;
	}
      else
	{
	  move_tool_motion (tool, NULL, gdisp_ptr);
	}

      selection_resume (gdisp->select);
      gdisplays_flush ();

      if (move->guide)
	gdisplay_draw_guide (gdisp, move->guide, TRUE);
    }
  else
    {
      /*  First take care of the case where the user "cancels" the action  */
      if (! (bevent->state & GDK_BUTTON3_MASK))
	{
	  if (move->layer)
	    {
	      floating_sel_anchor (move->layer);
	      gdisplays_flush ();
	    }
	}
    }
}

static void
move_tool_motion (Tool           *tool,
		  GdkEventMotion *mevent,
		  gpointer        gdisp_ptr)

{
  GDisplay *gdisp;
  MoveTool *private;
  int x, y;

  gdisp = gdisp_ptr;
  private = tool->private;

  if (private->guide)
    {
      move_draw_guide (gdisp, private->guide);

      if(mevent && mevent->window != gdisp->canvas->window)
	{
	  private->guide->position = -1;
	  return;
	}
      
      if (mevent)
	{
	  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, 
				       &x, &y, TRUE, FALSE);
	  
	  if (private->guide->orientation == HORIZONTAL_GUIDE)
	    private->guide->position = y;
	  else
	    private->guide->position = x;
	  
	  move_draw_guide (gdisp, private->guide);
	}
    }
}

static void
move_tool_cursor_update (Tool           *tool,
			 GdkEventMotion *mevent,
			 gpointer        gdisp_ptr)
{
  MoveTool *move;
  GDisplay *gdisp;
  Guide *guide;
  Layer *layer;
  int x, y;

  move = tool->private;
  gdisp = (GDisplay *) gdisp_ptr;
  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &x, &y, FALSE, FALSE);

  if (mevent->state & GDK_MOD1_MASK)
    gdisplay_install_tool_cursor (gdisp, GDK_DIAMOND_CROSS);
  else if (mevent->state & GDK_SHIFT_MASK)
    gdisplay_install_tool_cursor (gdisp, GDK_FLEUR);
  else
    {
      if (gdisp->draw_guides && (guide = gdisplay_find_guide (gdisp, mevent->x, mevent->y)))
	{
	  tool->gdisp_ptr = gdisp_ptr;
	  gdisplay_install_tool_cursor (gdisp, GDK_HAND2);

	  if (tool->state != ACTIVE)
	    {
	      if (move->guide)
		{
		  gdisp = move->disp;
		  if (gdisp)
		    gdisplay_draw_guide (gdisp, move->guide, FALSE);
		}

	      gdisp = gdisp_ptr;
	      gdisplay_draw_guide (gdisp, guide, TRUE);
	      move->guide = guide;
	      move->disp = gdisp;
	    }
	}
      else if ((layer = gimage_pick_correlate_layer (gdisp->gimage, x, y)))
	{
	  /*  if there is a floating selection, and this aint it...  */
	  if (gimage_floating_sel (gdisp->gimage) && !layer_is_floating_sel (layer))
	    gdisplay_install_tool_cursor (gdisp, GDK_SB_DOWN_ARROW);
	  else if (layer == gdisp->gimage->active_layer)
	    gdisplay_install_tool_cursor (gdisp, GDK_FLEUR);
	  else
	    gdisplay_install_tool_cursor (gdisp, GDK_HAND2);
	}
      else
	gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW);
    }
}


static void
move_tool_control (Tool     *tool,
		   int       action,
		   gpointer  gdisp_ptr)
{
  MoveTool *move;

  move = tool->private;

  switch (action)
    {
    case PAUSE :
      break;
    case RESUME :
      if (move->guide)
	gdisplay_draw_guide (gdisp_ptr, move->guide, TRUE);
      break;
    case HALT :
      break;
    }
}

static void
move_create_gc (GDisplay *gdisp)
{
  if (!move_gc)
    {
      GdkGCValues values;

      values.foreground.pixel = gdisplay_white_pixel (gdisp);
      values.function = GDK_INVERT;
      move_gc = gdk_gc_new_with_values (gdisp->canvas->window, &values, GDK_GC_FUNCTION);
    }
}


void
move_tool_start_hguide (Tool *tool,
			void *data)
{
  MoveTool *private;
  GDisplay *gdisp;

  gdisp = data;

  selection_pause (gdisp->select);

  tool->gdisp_ptr = gdisp;
  tool->scroll_lock = TRUE;

  private = tool->private;
  private->guide = gimage_add_hguide (gdisp->gimage);

  tool->state = ACTIVE;

  undo_push_guide (gdisp->gimage, private->guide);
}

void
move_tool_start_vguide (Tool *tool,
			void *data)
{
  MoveTool *private;
  GDisplay *gdisp;

  gdisp = data;

  selection_pause (gdisp->select);

  tool->gdisp_ptr = gdisp;
  tool->scroll_lock = TRUE;

  private = tool->private;
  private->guide = gimage_add_vguide (gdisp->gimage);

  tool->state = ACTIVE;

  undo_push_guide (gdisp->gimage, private->guide);
}

Tool *
tools_new_move_tool ()
{
  Tool * tool;
  MoveTool * private;

  /*  The tool options  */
  if (! move_options)
    {
      move_options = tool_options_new (_("Move Tool Options"));
      tools_register (MOVE, (ToolOptions *) move_options);
    }

  tool = (Tool *) g_malloc (sizeof (Tool));
  private = (MoveTool *) g_malloc (sizeof (MoveTool));

  tool->type = MOVE;
  tool->state = INACTIVE;
  tool->scroll_lock = 0;   /*  Allow scrolling  */
  tool->auto_snap_to = FALSE;
  tool->private = (void *) private;

  tool->button_press_func = move_tool_button_press;
  tool->button_release_func = move_tool_button_release;
  tool->motion_func = move_tool_motion;
  tool->arrow_keys_func = edit_sel_arrow_keys_func;
  tool->toggle_key_func = standard_toggle_key_func;
  tool->cursor_update_func = move_tool_cursor_update;
  tool->control_func = move_tool_control;
  tool->preserve = TRUE;

  private->layer = NULL;
  private->guide = NULL;
  private->disp = NULL;

  return tool;
}


void
tools_free_move_tool (Tool *tool)
{
  MoveTool * move;

  move = (MoveTool *) tool->private;

  if (tool->gdisp_ptr)
    {
      if (move->guide)
	gdisplay_draw_guide (tool->gdisp_ptr, move->guide, FALSE);
    }

  g_free (move);
}
