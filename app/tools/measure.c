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
#include <math.h>
#include "appenv.h"
#include "draw_core.h"
#include "measure.h"

#include "libgimp/gimpintl.h"


/*  target size  */
#define  TARGET_HEIGHT  15
#define  TARGET_WIDTH   15

/*  handle size (actually half of TARGET_[WIDTH|HEIGHT])  */
#define  HANDLE_SIZE    7

#define  ARC_RADIUS     40

#define  STATUSBAR_SIZE 128

/*  possible measure functions  */
#define CREATING        0
#define MOVING_START    1
#define MOVING_END      2
#define FINISHED        4

/*  the measure structure  */
typedef struct _MeasureTool MeasureTool;
struct _MeasureTool
{
  DrawCore *core;        /*  Core select object   */

  int       startx;      /*  starting x coord     */
  int       starty;      /*  starting y coord     */
  int       endx;        /*  ending x coord       */
  int       endy;        /*  ending y coord       */
  double    angle;       /*  angle                */

  int       function;    /*  moving or resizing   */
  guint     context_id;  /*  for the statusbar    */
};

/*  the measure tool options  */
static ToolOptions *measure_options = NULL;

/*  local function prototypes  */
static void   measure_tool_button_press   (Tool *, GdkEventButton *, gpointer);
static void   measure_tool_button_release (Tool *, GdkEventButton *, gpointer);
static void   measure_tool_motion         (Tool *, GdkEventMotion *, gpointer);
static void   measure_tool_cursor_update  (Tool *, GdkEventMotion *, gpointer);
static void   measure_tool_control	  (Tool *, ToolAction,       gpointer);



static void
measure_tool_button_press (Tool           *tool,
			   GdkEventButton *bevent,
			   gpointer        gdisp_ptr)
{
  GDisplay * gdisp;
  MeasureTool * measure_tool;
  int x1, x2, y1, y2;

  gdisp = (GDisplay *) gdisp_ptr;
  measure_tool = (MeasureTool *) tool->private;

  /*  if we are changing displays, pop the statusbar on the old one  */ 
  if (tool->state == ACTIVE && gdisp_ptr != tool->gdisp_ptr)
    {
      GDisplay *old_gdisp = tool->gdisp_ptr;
      gtk_statusbar_pop (GTK_STATUSBAR (old_gdisp->statusbar), measure_tool->context_id);
    }

  if (tool->state == INACTIVE || gdisp_ptr != tool->gdisp_ptr)
    {
      measure_tool->function = CREATING;
    }  
  else
    {
      /*  if the cursor is in one of the handles, the new function will be moving  */
      gdisplay_transform_coords (gdisp, measure_tool->startx, measure_tool->starty, &x1, &y1, TRUE);
      gdisplay_transform_coords (gdisp, measure_tool->endx, measure_tool->endy, &x2, &y2, TRUE);      

      if (bevent->x == BOUNDS (bevent->x, x1 - HANDLE_SIZE, x1 + HANDLE_SIZE) &&
	  bevent->y == BOUNDS (bevent->y, y1 - HANDLE_SIZE, y1 + HANDLE_SIZE))
	measure_tool->function = MOVING_START;
      else if (bevent->x == BOUNDS (bevent->x, x2 - HANDLE_SIZE, x2 + HANDLE_SIZE) &&
	       bevent->y == BOUNDS (bevent->y, y2 - HANDLE_SIZE, y2 + HANDLE_SIZE))
	measure_tool->function = MOVING_END;
      /*  otherwise, the new function will be creating  */
      else
	measure_tool->function = CREATING;
    }

  if (measure_tool->function == CREATING)
    {
      if (tool->state == ACTIVE)
	draw_core_stop (measure_tool->core, tool);
      
      gdisplay_untransform_coords (gdisp,  bevent->x, bevent->y, 
				   &measure_tool->startx, &measure_tool->starty, TRUE, TRUE);
      measure_tool->endx = measure_tool->startx;
      measure_tool->endy = measure_tool->starty;

      /*  set the gdisplay  */
      tool->gdisp_ptr = gdisp_ptr;

      /* initialize the statusbar display */
      measure_tool->context_id = 
	gtk_statusbar_get_context_id (GTK_STATUSBAR (gdisp->statusbar), "measure");
      gtk_statusbar_push (GTK_STATUSBAR (gdisp->statusbar), 
			  measure_tool->context_id, (""));

      /*  start drawing the measure tool  */
      draw_core_start (measure_tool->core, gdisp->canvas->window, tool);
    }

  gdk_pointer_grab (gdisp->canvas->window, FALSE,
		    GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, bevent->time);
  
  tool->state = ACTIVE;
}

static void
measure_tool_button_release (Tool           *tool,
			     GdkEventButton *bevent,
			     gpointer        gdisp_ptr)
{
  GDisplay * gdisp;
  MeasureTool * measure_tool;

  gdisp = (GDisplay *) gdisp_ptr;
  measure_tool = (MeasureTool *) tool->private;
  
  measure_tool->function = FINISHED;

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();
}

static void
measure_tool_motion (Tool           *tool,
		     GdkEventMotion *mevent,
		     gpointer        gdisp_ptr)
{

  GDisplay * gdisp;
  MeasureTool * measure_tool;
  int x, y;
  gchar distance_str[STATUSBAR_SIZE];
  gdouble distance;

  gdisp = (GDisplay *) gdisp_ptr;
  measure_tool = (MeasureTool *) tool->private;

  /*  undraw the current tool  */
  draw_core_pause (measure_tool->core, tool);

  /*  get the coordinates  */
  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &x, &y, FALSE, TRUE);

  switch (measure_tool->function)
    {
    case CREATING:
    case MOVING_END:
      measure_tool->endx = x;
      measure_tool->endy = y;
      /*  restrict to horizontal/vertical movements, if modifiers are pressed */
      if (mevent->state & GDK_MOD1_MASK)
	{
	  if (mevent->state & GDK_CONTROL_MASK)
	    {
	      int dx, dy, d;
	      
	      dx = measure_tool->endx - measure_tool->startx;
	      dy = measure_tool->endy - measure_tool->starty;
	      d  = (abs(dx) + abs(dy)) >> 1;
	      
	      measure_tool->endx = measure_tool->startx + ((dx < 0) ? -d : d);
	      measure_tool->endy = measure_tool->starty + ((dy < 0) ? -d : d);
	    }
	  else
	    measure_tool->endx = measure_tool->startx;
	}
      else if (mevent->state & GDK_CONTROL_MASK)
	measure_tool->endy = measure_tool->starty;
      break;

    case MOVING_START:
      measure_tool->startx = x;
      measure_tool->starty = y;
       /*  restrict to horizontal/vertical movements, if modifiers are pressed */
      if (mevent->state & GDK_MOD1_MASK)
	{
	  if (mevent->state & GDK_CONTROL_MASK)
	    {
	      int dx, dy, d;
	      
	      dx = measure_tool->endx - measure_tool->startx;
	      dy = measure_tool->endy - measure_tool->starty;
	      d  = (abs(dx) + abs(dy)) >> 1;
	      
	      measure_tool->startx = measure_tool->endx - ((dx < 0) ? -d : d);
	      measure_tool->starty = measure_tool->endy - ((dy < 0) ? -d : d);
	    }
	  else
	    measure_tool->startx = measure_tool->endx;
	}
      else if (mevent->state & GDK_CONTROL_MASK)
	measure_tool->starty = measure_tool->endy;
      break;

    default:
      break;
    }

  /*  show info in statusbar  */
  gtk_statusbar_pop (GTK_STATUSBAR (gdisp->statusbar), measure_tool->context_id);

  if (gdisp->dot_for_dot)
    {
      distance = sqrt (SQR (measure_tool->endx - measure_tool->startx) + 
		       SQR (measure_tool->endy - measure_tool->starty));
      measure_tool->angle = 180.0 / G_PI * 
	atan ((double)(measure_tool->endy - measure_tool->starty) / 
	      (double)(measure_tool->endx - measure_tool->startx)); 
      g_snprintf (distance_str, sizeof (distance_str), "%s %.1f %s, %s %.2f %s",
		  _("Distance:"), distance, _("pixels"), _("Angle:"), fabs(measure_tool->angle), _("degrees"));
    }
  else /* show real world units */
    {
      gchar *format_str = g_strdup_printf ("%s %%.%df %s, %s %%.2f %s",
					   _("Distance:"),
					   gimp_unit_get_digits (gdisp->gimage->unit),
					   gimp_unit_get_symbol (gdisp->gimage->unit),
					   _("Angle:"), _("degrees"));

      distance = 
	sqrt (SQR ((measure_tool->endx - measure_tool->startx) / gdisp->gimage->xresolution) +
	      SQR ((measure_tool->endy - measure_tool->starty) / gdisp->gimage->yresolution));
      measure_tool->angle = 180.0 / G_PI * 
	atan (((double)(measure_tool->endy - measure_tool->starty) / gdisp->gimage->xresolution) /
	      ((double)(measure_tool->endx - measure_tool->startx) / gdisp->gimage->yresolution)); 
      
      g_snprintf (distance_str, sizeof (distance_str), format_str,
		  distance * gimp_unit_get_factor (gdisp->gimage->unit), fabs (measure_tool->angle));
      g_free (format_str);
    }

  gtk_statusbar_push (GTK_STATUSBAR (gdisp->statusbar), measure_tool->context_id,
		      distance_str);

  /*  redraw the current tool  */
  draw_core_resume (measure_tool->core, tool);
}

static void
measure_tool_cursor_update (Tool           *tool,
			    GdkEventMotion *mevent,
			    gpointer        gdisp_ptr)
{
  GdkCursorType  ctype = GDK_TCROSS;
  MeasureTool   *measure_tool;
  GDisplay      *gdisp;
  int            x1, x2, y1, y2;

  gdisp = (GDisplay *) gdisp_ptr;
  measure_tool = (MeasureTool *) tool->private;

  gdisplay_transform_coords (gdisp, measure_tool->startx, measure_tool->starty, &x1, &y1, TRUE);
  gdisplay_transform_coords (gdisp, measure_tool->endx, measure_tool->endy, &x2, &y2, TRUE);      

  if (tool->state == ACTIVE && tool->gdisp_ptr == gdisp_ptr)
    {
      if (mevent->x == BOUNDS (mevent->x, x1 - HANDLE_SIZE, x1 + HANDLE_SIZE) &&
	  mevent->y == BOUNDS (mevent->y, y1 - HANDLE_SIZE, y1 + HANDLE_SIZE))
	ctype = GDK_FLEUR;
      else if (mevent->x == BOUNDS (mevent->x, x2 - HANDLE_SIZE, x2 + HANDLE_SIZE) &&
	  mevent->y == BOUNDS (mevent->y, y2 - HANDLE_SIZE, y2 + HANDLE_SIZE))
	ctype = GDK_FLEUR;
    }

  gdisplay_install_tool_cursor (gdisp, ctype);
}

static void
measure_tool_draw (Tool *tool)
{
  GDisplay * gdisp;
  MeasureTool * measure_tool;
  int anglex, angley;
  int x1, x2, y1, y2;

  gdisp = (GDisplay *) tool->gdisp_ptr;
  measure_tool = (MeasureTool *) tool->private;

  gdisplay_transform_coords (gdisp, measure_tool->startx, measure_tool->starty,
			     &x1, &y1, TRUE);
  gdisplay_transform_coords (gdisp, measure_tool->endx, measure_tool->endy,
			     &x2, &y2, TRUE);

  /*  Draw start target  */
  gdk_draw_line (measure_tool->core->win, measure_tool->core->gc,
		 x1 - (TARGET_WIDTH >> 1), y1, 
		 x1 + (TARGET_WIDTH >> 1), y1);
  gdk_draw_line (measure_tool->core->win, measure_tool->core->gc,
		 x1, y1 - (TARGET_HEIGHT >> 1), 
		 x1, y1 + (TARGET_HEIGHT >> 1));

  /*  Draw end target  */
  gdk_draw_line (measure_tool->core->win, measure_tool->core->gc,
		 x2 - (TARGET_WIDTH >> 1), y2, 
		 x2 + (TARGET_WIDTH >> 1), y2);
  gdk_draw_line (measure_tool->core->win, measure_tool->core->gc,
		 x2, y2 - (TARGET_HEIGHT >> 1), 
		 x2, y2 + (TARGET_HEIGHT >> 1));

  /*  Draw an arc to indicate where the angle is measured  */
  if (measure_tool->angle != 0.0)
    {
      if (x2 >= x1)
	{
	  gdk_draw_arc (measure_tool->core->win, measure_tool->core->gc, FALSE,
			x1 - ARC_RADIUS, y1 - ARC_RADIUS,
			2 * ARC_RADIUS, 2 * ARC_RADIUS, 
			0, measure_tool->angle * -64.0);
	  gdk_draw_line (measure_tool->core->win, measure_tool->core->gc,
			 x1, y1,
			 x1 + ARC_RADIUS + 4, y1);
	  anglex = x1 + (double)(ARC_RADIUS + 4) * cos (measure_tool->angle / 180.0 * G_PI);
	  angley = y1 + (double)(ARC_RADIUS + 4) * sin (measure_tool->angle / 180.0 * G_PI);
	}
      else
	{
	  gdk_draw_arc (measure_tool->core->win, measure_tool->core->gc, FALSE,
			x1 - ARC_RADIUS, y1 - ARC_RADIUS,
			2 * ARC_RADIUS, 2 * ARC_RADIUS, 
			11520, measure_tool->angle * -64.0);
	  gdk_draw_line (measure_tool->core->win, measure_tool->core->gc,
			 x1, y1,
			 x1 - ARC_RADIUS - 4, y1);
	  anglex = x1 - (double)(ARC_RADIUS + 4) * cos (measure_tool->angle / 180.0 * G_PI);
	  angley = y1 - (double)(ARC_RADIUS + 4) * sin (measure_tool->angle / 180.0 * G_PI);
	}
    }
  else
    {
      anglex = x1;
      angley = y1;
    }

  /*  Draw the line between the start and end coords  */
  if ((SQR (x1 - x2) + SQR (y1 - y2)) > (SQR (x1 - anglex) + SQR (y1 - angley)))
     gdk_draw_line (measure_tool->core->win, measure_tool->core->gc,
		    x1, y1,
		    x2, y2);
  else
    gdk_draw_line (measure_tool->core->win, measure_tool->core->gc,
		   x1, y1,
		   anglex, angley);
}


static void
measure_tool_control (Tool       *tool,
		      ToolAction  action,
		      gpointer    gdisp_ptr)
{
  GDisplay * gdisp;
  MeasureTool * measure_tool;

  gdisp = (GDisplay *) tool->gdisp_ptr;
  measure_tool = (MeasureTool *) tool->private;

  switch (action)
    {
    case PAUSE:
      draw_core_pause (measure_tool->core, tool);
      break;

    case RESUME:
      draw_core_resume (measure_tool->core, tool);
      break;

    case HALT:
      gtk_statusbar_pop (GTK_STATUSBAR (gdisp->statusbar), measure_tool->context_id);
      draw_core_stop (measure_tool->core, tool);
      tool->state = INACTIVE;
      break;

    default:
      break;
    }
}

Tool *
tools_new_measure_tool (void)
{
  Tool * tool;
  MeasureTool * private;

  /*  The tool options  */
  if (! measure_options)
    {
      measure_options = tool_options_new (_("Measure Tool Options"));
      tools_register (MEASURE, (ToolOptions *) measure_options);
    }

  tool = tools_new_tool (MEASURE);
  private = g_new (MeasureTool, 1);

  private->core = draw_core_new (measure_tool_draw);
  private->startx = private->starty = 0;
  private->angle  = 0.0;
   private->function = CREATING;

  tool->private = (void *) private;
 
  tool->button_press_func   = measure_tool_button_press;
  tool->button_release_func = measure_tool_button_release;
  tool->motion_func         = measure_tool_motion;
  tool->cursor_update_func  = measure_tool_cursor_update;
  tool->control_func        = measure_tool_control;

  return tool;
}

void
tools_free_measure_tool (Tool *tool)
{
  GDisplay * gdisp;
  MeasureTool * measure_tool;

  measure_tool = (MeasureTool *) tool->private;
  gdisp = (GDisplay *) tool->gdisp_ptr;

  if (tool->state == ACTIVE)
    {
      gtk_statusbar_pop (GTK_STATUSBAR (gdisp->statusbar), measure_tool->context_id);
      draw_core_stop (measure_tool->core, tool);
    }

  draw_core_free (measure_tool->core);

  g_free (measure_tool);
}



