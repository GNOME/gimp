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

#include <stdlib.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimplayer.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-ops.h"

#include "floating_sel.h"
#include "undo.h"

#include "gimpeditselectiontool.h"
#include "gimpmovetool.h"
#include "tool_manager.h"
#include "tool_options.h"

#include "libgimp/gimpintl.h"


static void   gimp_move_tool_class_init (GimpMoveToolClass *klass);
static void   gimp_move_tool_init       (GimpMoveTool      *move_tool);

static void   move_tool_button_press    (GimpTool          *tool,
					 GdkEventButton    *bevent,
					 GDisplay          *gdisp);
static void   move_tool_button_release  (GimpTool          *tool,
					 GdkEventButton    *bevent,
					 GDisplay          *gdisp);
static void   move_tool_motion          (GimpTool          *tool,
					 GdkEventMotion    *mevent,
					 GDisplay          *gdisp);
static void   move_tool_cursor_update   (GimpTool          *tool,
					 GdkEventMotion    *mevent,
					 GDisplay          *gdisp);
static void   move_tool_control	        (GimpTool          *tool,
					 ToolAction         tool_action,
					 GDisplay          *gdisp);

static void   move_create_gc            (GDisplay          *gdisp);


/*  the move tool options  */
static GimpToolOptions *move_options = NULL;

/*  local variables  */
static GdkGC *move_gc = NULL;

static GimpToolClass *parent_class = NULL;


void
gimp_move_tool_register (Gimp *gimp)
{
  tool_manager_register_tool (gimp,
			      GIMP_TYPE_MOVE_TOOL,
                              FALSE,
			      "gimp:move_tool",
			      _("Move Tool"),
			      _("Move layers & selections"),
			      N_("/Tools/Transform Tools/Move"), "M",
			      NULL, "tools/move.html",
			      GIMP_STOCK_TOOL_MOVE);
}

GType
gimp_move_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpMoveToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_move_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpMoveTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_move_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_TOOL,
					  "GimpMoveTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_move_tool_class_init (GimpMoveToolClass *klass)
{
  GimpToolClass *tool_class;

  tool_class = GIMP_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  tool_class->control        = move_tool_control;
  tool_class->button_press   = move_tool_button_press;
  tool_class->button_release = move_tool_button_release;
  tool_class->motion         = move_tool_motion;
  tool_class->cursor_update  = move_tool_cursor_update;
  tool_class->arrow_key      = gimp_edit_selection_tool_arrow_key;
}

static void
gimp_move_tool_init (GimpMoveTool *move_tool)
{
  GimpTool *tool;

  tool = GIMP_TOOL (move_tool);

  /*  The tool options  */
  if (! move_options)
    {
      move_options = tool_options_new ();

      tool_manager_register_tool_options (GIMP_TYPE_MOVE_TOOL,
					  (GimpToolOptions *) move_options);
    }

  move_tool->layer = NULL;
  move_tool->guide = NULL;
  move_tool->disp  = NULL;

  tool->auto_snap_to = FALSE;  /*  Don't snap to guides  */
}

static void
move_tool_control (GimpTool   *tool,
		   ToolAction  action,
		   GDisplay   *gdisp)
{
  GimpMoveTool *move_tool;

  move_tool = GIMP_MOVE_TOOL (tool);

  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      if (move_tool->guide)
	gdisplay_draw_guide (gdisp, move_tool->guide, TRUE);
      break;

    case HALT:
      if (move_tool->guide)
        gdisplay_draw_guide (gdisp, move_tool->guide, FALSE);
      break;

    default:
      break;
    }

  if (GIMP_TOOL_CLASS (parent_class)->control)
    GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}

static void
move_tool_button_press (GimpTool       *tool,
			GdkEventButton *bevent,
			GDisplay       *gdisp)
{
  GimpMoveTool *move;
  GimpLayer    *layer;
  GimpGuide    *guide;
  gint          x, y;

  move = GIMP_MOVE_TOOL (tool);

  tool->gdisp = gdisp;
  move->layer = NULL;
  move->guide = NULL;
  move->disp  = NULL;

  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y, &x, &y,
			       FALSE, FALSE);

  if (bevent->state & GDK_MOD1_MASK &&
      !gimage_mask_is_empty (gdisp->gimage))
    {
      init_edit_selection (tool, gdisp, bevent, EDIT_MASK_TRANSLATE);
      tool->state = ACTIVE;
    }
  else if (bevent->state & GDK_SHIFT_MASK)
    {
      init_edit_selection (tool, gdisp, bevent, EDIT_LAYER_TRANSLATE);
      tool->state = ACTIVE;
    }
  else
    {
      if (gdisp->draw_guides &&
	  (guide = gdisplay_find_guide (gdisp, bevent->x, bevent->y)))
	{
	  undo_push_guide (gdisp->gimage, guide);

	  gdisplays_expose_guide (gdisp->gimage, guide);
	  gimp_image_remove_guide (gdisp->gimage, guide);
	  gdisplay_flush (gdisp);
	  gimp_image_add_guide (gdisp->gimage, guide);

	  move->guide = guide;
	  move->disp  = gdisp;

	  tool->scroll_lock = TRUE;
	  tool->state       = ACTIVE;

	  move_tool_motion (tool, NULL, gdisp);
	}
      else if ((layer = gimp_image_pick_correlate_layer (gdisp->gimage, x, y)))
	{
	  /*  If there is a floating selection, and this aint it,
	   *  use the move tool
	   */
	  if (gimp_image_floating_sel (gdisp->gimage) &&
	      ! gimp_layer_is_floating_sel (layer))
	    {
	      move->layer = gimp_image_floating_sel (gdisp->gimage);
	    }
	  /*  Otherwise, init the edit selection  */
	  else
	    {
	      gimp_image_set_active_layer (gdisp->gimage, layer);
	      init_edit_selection (tool, gdisp, bevent, EDIT_LAYER_TRANSLATE);
	    }
	  tool->state = ACTIVE;
	}
    }

  /* if we've got an active tool grab the pointer */
  if (tool->state == ACTIVE)      
    {
      gdk_pointer_grab (gdisp->canvas->window, FALSE,
			GDK_POINTER_MOTION_HINT_MASK |
			GDK_BUTTON1_MOTION_MASK |
			GDK_BUTTON_RELEASE_MASK,
			NULL, NULL, bevent->time);
    }
}

static void
move_draw_guide (GDisplay  *gdisp, 
		 GimpGuide *guide)
{
  gint x1, y1;
  gint x2, y2;
  gint w, h;
  gint x, y;

  if (!move_gc)
    move_create_gc (gdisp);

  if (guide->position == -1)
    return;
  
  gdisplay_transform_coords (gdisp, gdisp->gimage->width, 
			     gdisp->gimage->height, &x2, &y2, FALSE); 

  w = gdisp->canvas->allocation.width;
  h = gdisp->canvas->allocation.height;

  switch (guide->orientation)
    {
    case ORIENTATION_HORIZONTAL:
      gdisplay_transform_coords (gdisp, 0, guide->position, &x1, &y, FALSE);
      if (x1 < 0) x1 = 0;
      if (x2 > w) x2 = w;

      gdk_draw_line (gdisp->canvas->window, move_gc, x1, y, x2, y); 
      break;

    case ORIENTATION_VERTICAL:
      gdisplay_transform_coords (gdisp, guide->position, 0, &x, &y1, FALSE);
      if (y1 < 0) y1 = 0;
      if (y2 > h) y2 = h;

      gdk_draw_line (gdisp->canvas->window, move_gc, x, y1, x, y2);
      break;

    default:
      g_warning ("mdg / BAD FALLTHROUGH");
      break;
    }
}

static void
move_tool_button_release (GimpTool       *tool,
			  GdkEventButton *bevent,
			  GDisplay       *gdisp)
{
  GimpMoveTool *move;
  gboolean      delete_guide;
  gint          x1, y1;
  gint          x2, y2;

  move = GIMP_MOVE_TOOL (tool);

  gdk_flush ();

  tool->state = INACTIVE;
  gdk_pointer_ungrab (bevent->time);

  if (move->guide)
    {
      tool->scroll_lock = FALSE;

      delete_guide = FALSE;
      gdisplay_untransform_coords (gdisp, 0, 0, &x1, &y1, FALSE, FALSE);
      gdisplay_untransform_coords (gdisp, gdisp->disp_width, gdisp->disp_height,
				   &x2, &y2, FALSE, FALSE);

      if (x1 < 0) x1 = 0;
      if (y1 < 0) y1 = 0;
      if (x2 > gdisp->gimage->width)  x2 = gdisp->gimage->width;
      if (y2 > gdisp->gimage->height) y2 = gdisp->gimage->height;

      switch (move->guide->orientation)
	{
	case ORIENTATION_HORIZONTAL:
	  if ((move->guide->position < y1) || (move->guide->position > y2))
	    delete_guide = TRUE;
	  break;

	case ORIENTATION_VERTICAL:
	  if ((move->guide->position < x1) || (move->guide->position > x2))
	    delete_guide = TRUE;
	  break;

	default:
	  break;
	}

      gdisplays_expose_guide (gdisp->gimage, move->guide);

      if (delete_guide)
	{
	  move_draw_guide (gdisp, move->guide);
	  gimp_image_delete_guide (gdisp->gimage, move->guide);
	  move->guide = NULL;
	  move->disp = NULL;
	}
      else
	{
	  move_tool_motion (tool, NULL, gdisp);
	}

      gdisplay_selection_visibility (gdisp, GIMP_SELECTION_RESUME);
      gdisplays_flush ();

      if (move->guide)
	gdisplay_draw_guide (gdisp, move->guide, TRUE);
    }
  else
    {
      /*  Take care of the case where the user "cancels" the action  */
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
move_tool_motion (GimpTool       *tool,
		  GdkEventMotion *mevent,
		  GDisplay       *gdisp)

{
  GimpMoveTool *move;
  gint          x, y;

  move = GIMP_MOVE_TOOL (tool);

  if (move->guide)
    {
      move_draw_guide (gdisp, move->guide);

      if (mevent && mevent->window != gdisp->canvas->window)
	{
	  move->guide->position = -1;
	  return;
	}
      
      if (mevent)
	{
	  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, 
				       &x, &y, TRUE, FALSE);

	  if (move->guide->orientation == ORIENTATION_HORIZONTAL)
	    move->guide->position = y;
	  else
	    move->guide->position = x;
	  
	  move_draw_guide (gdisp, move->guide);
	}
    }
}

static void
move_tool_cursor_update (GimpTool       *tool,
			 GdkEventMotion *mevent,
			 GDisplay       *gdisp)
{
  GimpMoveTool *move;
  GimpGuide    *guide;
  GimpLayer    *layer;
  gint          x, y;

  move = GIMP_MOVE_TOOL (tool);

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &x, &y,
			       FALSE, FALSE);

  if (mevent->state & GDK_MOD1_MASK &&
      ! gimage_mask_is_empty (gdisp->gimage))
    {
      gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_CURSOR,
				    GIMP_RECT_SELECT_TOOL_CURSOR,
				    GIMP_CURSOR_MODIFIER_MOVE);
    }
  else if (mevent->state & GDK_SHIFT_MASK)
    {
      gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_CURSOR,
				    GIMP_MOVE_TOOL_CURSOR,
				    GIMP_CURSOR_MODIFIER_NONE);
    }
  else
    {
      if (gdisp->draw_guides &&
	  (guide = gdisplay_find_guide (gdisp, mevent->x, mevent->y)))
	{
	  tool->gdisp = gdisp;
	  gdisplay_install_tool_cursor (gdisp, GDK_HAND2,
					GIMP_TOOL_CURSOR_NONE,
					GIMP_CURSOR_MODIFIER_HAND);

	  if (tool->state != ACTIVE)
	    {
	      if (move->guide)
		{
		  gdisp = gdisplays_check_valid (move->disp, move->disp->gimage);
		  if (gdisp)
		    gdisplay_draw_guide (gdisp, move->guide, FALSE);
		}

	      gdisp = gdisp;
	      gdisplay_draw_guide (gdisp, guide, TRUE);
	      move->guide = guide;
	      move->disp  = gdisp;
	    }
	}
      else if ((layer = gimp_image_pick_correlate_layer (gdisp->gimage, x, y)))
	{
	  /*  if there is a floating selection, and this aint it...  */
	  if (gimp_image_floating_sel (gdisp->gimage) &&
	      ! gimp_layer_is_floating_sel (layer))
	    {
	      gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_CURSOR,
					    GIMP_RECT_SELECT_TOOL_CURSOR,
					    GIMP_CURSOR_MODIFIER_ANCHOR);
	    }
	  else if (layer == gimp_image_get_active_layer (gdisp->gimage))
	    {
	      gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_CURSOR,
					    GIMP_MOVE_TOOL_CURSOR,
					    GIMP_CURSOR_MODIFIER_NONE);
	    }
	  else
	    {
	      gdisplay_install_tool_cursor (gdisp, GDK_HAND2,
					    GIMP_TOOL_CURSOR_NONE,
					    GIMP_CURSOR_MODIFIER_HAND);
	    }
	}
      else
	{
	  gdisplay_install_tool_cursor (gdisp, GIMP_BAD_CURSOR,
					GIMP_MOVE_TOOL_CURSOR,
					GIMP_CURSOR_MODIFIER_NONE);
	}
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
      move_gc = gdk_gc_new_with_values (gdisp->canvas->window, &values,
					GDK_GC_FUNCTION);
    }
}

void
gimp_move_tool_start_hguide (GimpTool *tool,
			     GDisplay *gdisp)
{
  GimpMoveTool *move;

  move = GIMP_MOVE_TOOL (tool);

  gdisplay_selection_visibility (gdisp, GIMP_SELECTION_PAUSE);

  tool->gdisp       = gdisp;
  tool->scroll_lock = TRUE;

  if (move->guide && move->disp && move->disp->gimage)
    gdisplay_draw_guide (move->disp, move->guide, FALSE);

  move->guide = gimp_image_add_hguide (gdisp->gimage);
  move->disp  = gdisp;

  tool->state = ACTIVE;

  undo_push_guide (gdisp->gimage, move->guide);
}

void
gimp_move_tool_start_vguide (GimpTool *tool,
			     GDisplay *gdisp)
{
  GimpMoveTool *move;

  move = GIMP_MOVE_TOOL (tool);

  gdisplay_selection_visibility (gdisp, GIMP_SELECTION_PAUSE);

  tool->gdisp       = gdisp;
  tool->scroll_lock = TRUE;

  if (move->guide && move->disp && move->disp->gimage)
    gdisplay_draw_guide (move->disp, move->guide, FALSE);

  move->guide = gimp_image_add_vguide (gdisp->gimage);
  move->disp  = gdisp;

  tool->state = ACTIVE;

  undo_push_guide (gdisp->gimage, move->guide);
}
