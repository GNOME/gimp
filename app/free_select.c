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
#include "draw_core.h"
#include "edit_selection.h"
#include "errors.h"
#include "free_select.h"
#include "gimage_mask.h"
#include "gdisplay.h"
#include "rect_select.h"
#include "selection_options.h"
#include "scan_convert.h"

#include "libgimp/gimpmath.h"

#define DEFAULT_MAX_INC  1024
#define SUPERSAMPLE      3
#define SUPERSAMPLE2     9

/*  the free selection structures  */

typedef struct _FreeSelect FreeSelect;
struct _FreeSelect
{
  DrawCore  *core;      /*  Core select object                      */

  SelectOps  op;        /*  selection operation (ADD, SUB, etc)     */

  gint      current_x;  /*  these values are updated on every motion event  */
  gint      current_y;  /*  (enables immediate cursor updating on modifier
			 *   key events).  */

  gint      num_pts;    /*  Number of points in the polygon         */
};



/*  the free selection tool options  */
static SelectionOptions * free_options = NULL;

/*  The global array of XPoints for drawing the polygon...  */
static GdkPoint *global_pts = NULL;
static gint      max_segs = 0;


/*  functions  */

static gint
add_point (gint num_pts,
	   gint x,
	   gint y)
{
  if (num_pts >= max_segs)
    {
      max_segs += DEFAULT_MAX_INC;

      global_pts = (GdkPoint *) g_realloc ((void *) global_pts, sizeof (GdkPoint) * max_segs);

      if (!global_pts)
	gimp_fatal_error ("add_point(): Unable to reallocate points array in free_select.");
    }

  global_pts[num_pts].x = x;
  global_pts[num_pts].y = y;

  return 1;
}


static Channel *
scan_convert (GimpImage        *gimage,
	      gint              num_pts,
	      ScanConvertPoint *pts,
	      gint              width,
	      gint              height,
	      gboolean          antialias)
{
  Channel       *mask;
  ScanConverter *sc;

  sc = scan_converter_new (width, height, antialias ? SUPERSAMPLE : 1);
  scan_converter_add_points (sc, num_pts, pts);

  mask = scan_converter_to_channel (sc, gimage);
  scan_converter_free (sc);

  return mask;
}


/*************************************/
/*  Polygonal selection apparatus    */

void
free_select (GImage           *gimage,
	     gint              num_pts,
	     ScanConvertPoint *pts,
	     SelectOps         op,
	     gboolean          antialias,
	     gboolean          feather,
	     gdouble           feather_radius)
{
  Channel *mask;

  /*  if applicable, replace the current selection  */
  /*  or insure that a floating selection is anchored down...  */
  if (op == SELECTION_REPLACE)
    gimage_mask_clear (gimage);
  else
    gimage_mask_undo (gimage);

  mask = scan_convert (gimage, num_pts, pts,
		       gimage->width, gimage->height, antialias);

  if (mask)
    {
      if (feather)
	channel_feather (mask, gimage_get_mask (gimage),
			 feather_radius,
			 feather_radius,
			 op, 0, 0);
      else
	channel_combine_mask (gimage_get_mask (gimage),
			      mask, op, 0, 0);
      channel_delete (mask);
    }
}

void
free_select_button_press (Tool           *tool,
			  GdkEventButton *bevent,
			  gpointer        gdisp_ptr)
{
  GDisplay   *gdisp;
  FreeSelect *free_sel;

  gdisp = (GDisplay *) gdisp_ptr;
  free_sel = (FreeSelect *) tool->private;

  gdk_pointer_grab (gdisp->canvas->window, FALSE,
		    GDK_POINTER_MOTION_HINT_MASK |
		    GDK_BUTTON1_MOTION_MASK |
		    GDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, bevent->time);

  tool->state = ACTIVE;
  tool->gdisp_ptr = gdisp_ptr;

  switch (free_sel->op)
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

  add_point (0, bevent->x, bevent->y);
  free_sel->num_pts = 1;

  draw_core_start (free_sel->core,
		   gdisp->canvas->window,
		   tool);
}

void
free_select_button_release (Tool           *tool,
			    GdkEventButton *bevent,
			    gpointer        gdisp_ptr)
{
  FreeSelect       *free_sel;
  ScanConvertPoint *pts;
  GDisplay         *gdisp;
  gint              i;

  gdisp = (GDisplay *) gdisp_ptr;
  free_sel = (FreeSelect *) tool->private;

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  draw_core_stop (free_sel->core, tool);

  tool->state = INACTIVE;

  /*  First take care of the case where the user "cancels" the action  */
  if (! (bevent->state & GDK_BUTTON3_MASK))
    {
      pts = g_new (ScanConvertPoint, free_sel->num_pts);

      for (i = 0; i < free_sel->num_pts; i++)
	{
	  gdisplay_untransform_coords_f (gdisp, global_pts[i].x, global_pts[i].y,
					 &pts[i].x, &pts[i].y, FALSE);
	}

      free_select (gdisp->gimage, free_sel->num_pts, pts, free_sel->op,
		   free_options->antialias, free_options->feather,
		   free_options->feather_radius);

      g_free (pts);

      gdisplays_flush ();
    }
}

void
free_select_motion (Tool           *tool,
		    GdkEventMotion *mevent,
		    gpointer        gdisp_ptr)
{
  FreeSelect *free_sel;
  GDisplay   *gdisp;

  gdisp = (GDisplay *) gdisp_ptr;
  free_sel = (FreeSelect *) tool->private;

  /*  needed for immediate cursor update on modifier event  */
  free_sel->current_x = mevent->x;
  free_sel->current_y = mevent->y;

  if (tool->state != ACTIVE)
    return;

  if (add_point (free_sel->num_pts, mevent->x, mevent->y))
    {
      gdk_draw_line (free_sel->core->win, free_sel->core->gc,
		     global_pts[free_sel->num_pts - 1].x,
		     global_pts[free_sel->num_pts - 1].y,
		     global_pts[free_sel->num_pts].x,
		     global_pts[free_sel->num_pts].y);

      free_sel->num_pts ++;
    }
}

static void
free_select_control (Tool       *tool,
		     ToolAction  action,
		     gpointer    gdisp_ptr)
{
  FreeSelect *free_sel;

  free_sel = (FreeSelect *) tool->private;

  switch (action)
    {
    case PAUSE:
      draw_core_pause (free_sel->core, tool);
      break;

    case RESUME:
      draw_core_resume (free_sel->core, tool);
      break;

    case HALT:
      draw_core_stop (free_sel->core, tool);
      break;

    default:
      break;
    }
}

void
free_select_draw (Tool *tool)
{
  FreeSelect *free_sel;
  gint        i;

  free_sel = (FreeSelect *) tool->private;

  for (i = 1; i < free_sel->num_pts; i++)
    gdk_draw_line (free_sel->core->win, free_sel->core->gc,
		   global_pts[i - 1].x, global_pts[i - 1].y,
		   global_pts[i].x, global_pts[i].y);
}

static void
free_select_options_reset (void)
{
  selection_options_reset (free_options);
}

Tool *
tools_new_free_select (void)
{
  Tool       *tool;
  FreeSelect *private;

  /*  The tool options  */
  if (!free_options)
    {
      free_options =
	selection_options_new (FREE_SELECT, free_select_options_reset);
      tools_register (FREE_SELECT, (ToolOptions *) free_options);
    }

  tool = tools_new_tool (FREE_SELECT);
  private = g_new (FreeSelect, 1);

  private->core    = draw_core_new (free_select_draw);
  private->num_pts = 0;
  private->op      = SELECTION_REPLACE;

  tool->scroll_lock = TRUE;   /*  Do not allow scrolling  */

  tool->private = (void *) private;

  tool->button_press_func   = free_select_button_press;
  tool->button_release_func = free_select_button_release;
  tool->motion_func         = free_select_motion;
  tool->modifier_key_func   = rect_select_modifier_update;
  tool->cursor_update_func  = rect_select_cursor_update;
  tool->oper_update_func    = rect_select_oper_update;
  tool->control_func        = free_select_control;

  return tool;
}

void
tools_free_free_select (Tool *tool)
{
  FreeSelect *free_sel;

  free_sel = (FreeSelect *) tool->private;

  draw_core_free (free_sel->core);
  g_free (free_sel);
}
