/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Measure tool
 * Copyright (C) 1999 Sven Neumann <sven@gimp.org>
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

#include "actionarea.h"
#include "appenv.h"
#include "draw_core.h"
#include "info_dialog.h"
#include "measure.h"
#include "tool_options_ui.h"

#include "libgimp/gimpintl.h"
#include "libgimp/gimpmath.h"

/*  definitions  */
#define  TARGET         8
#define  ARC_RADIUS     30
#define  ARC_RADIUS_2   1100
#define  STATUSBAR_SIZE 128

/*  possible measure functions  */
typedef enum 
{
  CREATING,
  ADDING,
  MOVING,
  GUIDING,
  FINISHED
} MeasureFunction;

/*  the measure structure  */
typedef struct _MeasureTool MeasureTool;
struct _MeasureTool
{
  DrawCore        *core;        /*  draw core                  */
  MeasureFunction  function;    /*  what are we doing?         */
  int              point;       /*  what are we manipulating?  */
  int              num_points;  /*  how many points?           */
  int              x[3];        /*  three x coordinates        */
  int              y[3];        /*  three y coordinates        */
  double           angle1;      /*  first angle                */
  double           angle2;      /*  second angle               */
  guint            context_id;  /*  for the statusbar          */
};

/*  the measure tool options  */
typedef struct _MeasureOptions MeasureOptions;
struct _MeasureOptions
{
  ToolOptions  tool_options;

  gint         use_info_window;
  gint         use_info_window_d;
  GtkWidget   *use_info_window_w;
};


/*  maximum information buffer size  */
#define MAX_INFO_BUF 16

static MeasureOptions *measure_tool_options = NULL;

/*  the measure tool info window  */
static InfoDialog  *measure_tool_info = NULL;
static gchar        distance_buf [MAX_INFO_BUF];
static gchar        angle_buf    [MAX_INFO_BUF];


/*  local function prototypes  */
static void   measure_tool_button_press   (Tool *, GdkEventButton *, gpointer);
static void   measure_tool_button_release (Tool *, GdkEventButton *, gpointer);
static void   measure_tool_motion         (Tool *, GdkEventMotion *, gpointer);
static void   measure_tool_cursor_update  (Tool *, GdkEventMotion *, gpointer);
static void   measure_tool_control	  (Tool *, ToolAction,       gpointer);

static void   measure_tool_info_window_close_callback (GtkWidget *, gpointer);
static void   measure_tool_info_update                ();


static void
measure_tool_options_reset (void)
{
  MeasureOptions *options = measure_tool_options;

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->use_info_window_w),
				options->use_info_window_d);
}

static MeasureOptions *
measure_tool_options_new (void)
{
  MeasureOptions *options;
  GtkWidget *vbox;

  /*  the new measure tool options structure  */
  options = g_new (MeasureOptions, 1);
  tool_options_init ((ToolOptions *) options,
		     _("Measure Options"),
		     measure_tool_options_reset);
  options->use_info_window = options->use_info_window_d  = FALSE;

    /*  the main vbox  */
  vbox = options->tool_options.main_vbox;

  /*  the use_info_window toggle button  */
  options->use_info_window_w =
    gtk_check_button_new_with_label (_("Use Info Window"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->use_info_window_w),
				options->use_info_window_d);
  gtk_box_pack_start (GTK_BOX (vbox), options->use_info_window_w, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (options->use_info_window_w), "toggled",
		      (GtkSignalFunc) tool_options_toggle_update,
		      &options->use_info_window);
  gtk_widget_show (options->use_info_window_w);

  return options;
}

static double
measure_get_angle (int    dx,
		   int    dy,
		   double xres,
		   double yres)
{
  double angle;

  if (dx)
    angle = 180.0 / G_PI * atan (((double)(dy) / yres) / ((double)(dx) / xres));
  else if (dy)
    angle = dy > 0 ? 270.0 : 90.0;
  else
    angle = 180.0;
  
  if (dx > 0)
    {
      if (dy > 0)
	angle = 360.0 - angle;
      else
	angle = -angle;
    }
  else
    angle = 180.0 - angle;

  return (angle);
}

static void
measure_tool_button_press (Tool           *tool,
			   GdkEventButton *bevent,
			   gpointer        gdisp_ptr)
{
  GDisplay * gdisp;
  MeasureTool * measure_tool;
  int x[3];
  int y[3];
  int i;

  static ActionAreaItem action_items[] =
  {
    { N_("Close"), measure_tool_info_window_close_callback, NULL, NULL },
  };

  gdisp = (GDisplay *) gdisp_ptr;
  measure_tool = (MeasureTool *) tool->private;

  /*  if we are changing displays, pop the statusbar of the old one  */ 
  if (tool->state == ACTIVE && gdisp_ptr != tool->gdisp_ptr)
    {
      GDisplay *old_gdisp = tool->gdisp_ptr;
      gtk_statusbar_pop (GTK_STATUSBAR (old_gdisp->statusbar), measure_tool->context_id);
      gtk_statusbar_push (GTK_STATUSBAR (gdisp->statusbar), 
			  measure_tool->context_id, (""));
    } 
  
  measure_tool->function = CREATING;

  if (tool->state == ACTIVE && gdisp_ptr == tool->gdisp_ptr)
    {
      /*  if the cursor is in one of the handles, 
	  the new function will be moving or adding a new point or guide */
      for (i=0; i < measure_tool->num_points; i++)
	{
	  gdisplay_transform_coords (gdisp, measure_tool->x[i], measure_tool->y[i], 
				     &x[i], &y[i], FALSE);      
	  if (bevent->x == BOUNDS (bevent->x, x[i] - TARGET, x[i] + TARGET) &&
	      bevent->y == BOUNDS (bevent->y, y[i] - TARGET, y[i] + TARGET))
	    {
	      if (bevent->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK))
		{
		  Guide *guide;

		  if (bevent->state & GDK_CONTROL_MASK && 
		      (measure_tool->y[i] == BOUNDS (measure_tool->y[i], 0, gdisp->gimage->height)))
		    {
		      guide = gimp_image_add_hguide (gdisp->gimage);
		      guide->position = measure_tool->y[i];
		      gdisplays_expose_guide (gdisp->gimage, guide);
		    }  
		  if (bevent->state & GDK_MOD1_MASK &&
		      (measure_tool->x[i] == BOUNDS (measure_tool->x[i], 0, gdisp->gimage->width)))
		    {
		      guide = gimp_image_add_vguide (gdisp->gimage);
		      guide->position = measure_tool->x[i];
		      gdisplays_expose_guide (gdisp->gimage, guide);
		    }
		  gdisplays_flush ();
		  measure_tool->function = GUIDING;
		  break;
		}
	      measure_tool->function = (bevent->state & GDK_SHIFT_MASK) ? ADDING : MOVING;
	      measure_tool->point = i;
	      break;
	    }
	}
      /*  adding to the middle point makes no sense  */
      if (i == 0 && measure_tool->function == ADDING && measure_tool->num_points == 3)
	measure_tool->function = MOVING;
    }
  
  if (measure_tool->function == CREATING)
    {
      if (tool->state == ACTIVE)
	draw_core_stop (measure_tool->core, tool);
      else
	/* initialize the statusbar display */
	measure_tool->context_id = 
	  gtk_statusbar_get_context_id (GTK_STATUSBAR (gdisp->statusbar), "measure");

      /*  set the first point and go into ADDING mode  */
      gdisplay_untransform_coords (gdisp,  bevent->x, bevent->y, 
				   &measure_tool->x[0], &measure_tool->y[0], TRUE, FALSE);
      measure_tool->point = 0;
      measure_tool->num_points = 1;
      measure_tool->function = ADDING;

      /*  set the gdisplay  */
      tool->gdisp_ptr = gdisp_ptr;

      /*  start drawing the measure tool  */
      draw_core_start (measure_tool->core, gdisp->canvas->window, tool);
    }

  /*  create the info window if necessary  */
   if (!measure_tool_info &&
       (measure_tool_options->use_info_window || !GTK_WIDGET_VISIBLE (gdisp->statusarea)))
     {
       measure_tool_info = info_dialog_new (_("Measure Tool"));
       info_dialog_add_label (measure_tool_info, _("Distance:"), distance_buf);
       info_dialog_add_label (measure_tool_info, _("Angle:"), angle_buf);
       action_items[0].user_data = measure_tool_info;
       build_action_area (GTK_DIALOG (measure_tool_info->shell), action_items, 1, 0);
      }

  gdk_pointer_grab (gdisp->canvas->window, FALSE,
		    GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, bevent->time);
  tool->state = ACTIVE;

  /*  set the pointer to the crosshair, so one actually sees the cursor position  */
  gdisplay_install_tool_cursor (gdisp, GDK_TCROSS);
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
  int ax, ay;
  int bx, by;
  int dx, dy;
  int i;
  int tmp;
  double angle;
  double distance;
  gchar status_str[STATUSBAR_SIZE];

  gdisp = (GDisplay *) gdisp_ptr;
  measure_tool = (MeasureTool *) tool->private;

  /*  undraw the current tool  */
  draw_core_pause (measure_tool->core, tool);

  /*  get the coordinates  */
  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &x, &y, TRUE, FALSE);

  /*  
   *  A few comments here, because this routine looks quite weird at first ...
   *
   *  The goal is to keep point 0, called the start point, to be always the one 
   *  in the middle or, if there are only two points, the one that is fixed.
   *  The angle is then always measured at this point.
   */

  switch (measure_tool->function)
    {
    case ADDING:
      switch (measure_tool->point)
	{
	case 0:  /*  we are adding to the start point  */  
	  break;
	case 1:  /*  we are adding to the end point, make it the new start point  */
	  tmp = measure_tool->x[0];
	  measure_tool->x[0] = measure_tool->x[1];
	  measure_tool->x[1] = tmp;
	  tmp = measure_tool->y[0];
	  measure_tool->y[0] = measure_tool->y[1];
	  measure_tool->y[1] = tmp;
	  break;
	case 2:  /*  we are adding to the third point, make it the new start point  */
	  measure_tool->x[1] = measure_tool->x[0];
	  measure_tool->y[1] = measure_tool->y[0];
	  measure_tool->x[0] = measure_tool->x[2];
	  measure_tool->y[0] = measure_tool->y[2];
	  break;
	default:
	  break;
	}
      measure_tool->num_points = MIN (measure_tool->num_points + 1, 3);
      measure_tool->point = measure_tool->num_points - 1;
      measure_tool->function = MOVING;
      /*  no, don't break here!  */
 
    case MOVING: 
      /*  if we are moving the start point and only have two, make it the end point  */
      if (measure_tool->num_points == 2 && measure_tool->point == 0)
	{
	  tmp = measure_tool->x[0];
	  measure_tool->x[0] = measure_tool->x[1];
	  measure_tool->x[1] = tmp;
	  tmp = measure_tool->y[0];
	  measure_tool->y[0] = measure_tool->y[1];
	  measure_tool->y[1] = tmp;
	  measure_tool->point = 1;
	}
      i = measure_tool->point;

      measure_tool->x[i] = x;
      measure_tool->y[i] = y;

      /*  restrict to horizontal/vertical movements, if modifiers are pressed */
      if (mevent->state & GDK_MOD1_MASK)
	{
	  if (mevent->state & GDK_CONTROL_MASK)
	    {
	      int d;
	      
	      dx = measure_tool->x[i] - measure_tool->x[0];
	      dy = measure_tool->y[i] - measure_tool->y[0];
	      d  = (abs(dx) + abs(dy)) >> 1;
      
	      measure_tool->x[i] = measure_tool->x[0] + ((dx < 0) ? -d : d);
	      measure_tool->y[i] = measure_tool->y[0] + ((dy < 0) ? -d : d);
	    }
	  else
	    measure_tool->x[i] = measure_tool->x[0];
	}
      else if (mevent->state & GDK_CONTROL_MASK)
	measure_tool->y[i] = measure_tool->y[0];
      break;
    default:
      break;
    }

  /*  calculate distance and angle  */
  ax = measure_tool->x[1] - measure_tool->x[0];
  ay = measure_tool->y[1] - measure_tool->y[0];

  if (measure_tool->num_points == 3)
    {
      bx = measure_tool->x[2] - measure_tool->x[0];
      by = measure_tool->y[2] - measure_tool->y[0];
    }
  else
    {
      bx = ax < 0 ? -1 : 1;
      by = 0;
    }

  if (gdisp->dot_for_dot)
    {
      distance = sqrt (SQR (ax - bx) + SQR (ay - by));
      measure_tool->angle1 = measure_get_angle (ax, ay, 1.0, 1.0);
      measure_tool->angle2 = measure_get_angle (bx, by, 1.0, 1.0);
      angle = fabs (measure_tool->angle1 - measure_tool->angle2);
      if (angle > 180.0)
	angle = fabs (360.0 - angle);

      g_snprintf (status_str, STATUSBAR_SIZE, "%.1f %s, %.2f %s",
		  distance, _("pixels"), angle, _("degrees"));

      if (measure_tool_options)
	{
	  g_snprintf (distance_buf, MAX_INFO_BUF, "%.1f %s", distance, _("pixels"));
	  g_snprintf (angle_buf, MAX_INFO_BUF, "%.2f %s", angle, _("degrees"));
	}
    }
  else /* show real world units */
    {
      gchar *format_str = g_strdup_printf ("%%.%df %s, %%.2f %s",
					   gimp_unit_get_digits (gdisp->gimage->unit),
					   gimp_unit_get_symbol (gdisp->gimage->unit),
					   _("degrees"));
      
      distance =  gimp_unit_get_factor (gdisp->gimage->unit) * 
	sqrt (SQR ((double)(ax - bx) / gdisp->gimage->xresolution) +
	      SQR ((double)(ay - by) / gdisp->gimage->yresolution));

      measure_tool->angle1 = measure_get_angle (ax, ay, 
						gdisp->gimage->xresolution, 
						gdisp->gimage->yresolution); 
      measure_tool->angle2 = measure_get_angle (bx, by,
						gdisp->gimage->xresolution, 
						gdisp->gimage->yresolution);
      angle = fabs (measure_tool->angle1 - measure_tool->angle2);     
      if (angle > 180.0)
	angle = fabs (360.0 - angle);
 
      g_snprintf (status_str, STATUSBAR_SIZE, format_str, distance , angle);
      g_free (format_str);

      if (measure_tool_options)
	{
	  gchar *format_str = g_strdup_printf ("%%.%df %s",
					       gimp_unit_get_digits (gdisp->gimage->unit),
					       gimp_unit_get_symbol (gdisp->gimage->unit));
	  g_snprintf (distance_buf, MAX_INFO_BUF, format_str, distance);
	  g_snprintf (angle_buf, MAX_INFO_BUF, "%.2f %s", angle, _("degrees"));
	  g_free (format_str);
	}
    }
  
  /*  show info in statusbar  */
  gtk_statusbar_pop (GTK_STATUSBAR (gdisp->statusbar), measure_tool->context_id);
  gtk_statusbar_push (GTK_STATUSBAR (gdisp->statusbar), measure_tool->context_id,
		      status_str);

  /*  and in the info window  */
  if (measure_tool_info)
    measure_tool_info_update ();

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
  int            x[3];
  int            y[3];
  int            i;

  gdisp = (GDisplay *) gdisp_ptr;
  measure_tool = (MeasureTool *) tool->private;

  if (tool->state == ACTIVE && tool->gdisp_ptr == gdisp_ptr)
    {
      for (i=0; i<measure_tool->num_points; i++)
	{
	  gdisplay_transform_coords (gdisp, measure_tool->x[i], measure_tool->y[i], 
				     &x[i], &y[i], FALSE);      
	  
	  if (mevent->x == BOUNDS (mevent->x, x[i] - TARGET, x[i] + TARGET) &&
	      mevent->y == BOUNDS (mevent->y, y[i] - TARGET, y[i] + TARGET))
	    {
	      if (mevent->state & GDK_CONTROL_MASK)
		{
		  if (mevent->state & GDK_MOD1_MASK)
		    ctype = GDK_BOTTOM_RIGHT_CORNER;
		  else
		    ctype = GDK_BOTTOM_SIDE;
		  break;
		}
	      if (mevent->state & GDK_MOD1_MASK)
		{
		  ctype = GDK_RIGHT_SIDE;
		  break;
		}
	      ctype = (mevent->state & GDK_SHIFT_MASK) ? GDK_EXCHANGE : GDK_FLEUR;
	      if (i == 0 && measure_tool->num_points == 3 && ctype == GDK_EXCHANGE)
		ctype = GDK_FLEUR;
	      break;
	    }
	}
    }
  gdisplay_install_tool_cursor (gdisp, ctype);
}

static void
measure_tool_draw (Tool *tool)
{
  GDisplay * gdisp;
  MeasureTool * measure_tool;
  int x[3];
  int y[3];
  int i;
  int angle1, angle2;
  int draw_arc = 0;

  gdisp = (GDisplay *) tool->gdisp_ptr;
  measure_tool = (MeasureTool *) tool->private;

  for (i = 0; i < measure_tool->num_points; i++)
    {
      gdisplay_transform_coords (gdisp, measure_tool->x[i], measure_tool->y[i],
				 &x[i], &y[i], FALSE);
      if (i == 0 && measure_tool->num_points == 3)
	{
	  gdk_draw_arc (measure_tool->core->win, measure_tool->core->gc, FALSE,
			x[i] - (TARGET >> 1), y[i] - (TARGET >> 1),
			TARGET, TARGET, 0, 23040);
	}
      else
	{
	  gdk_draw_line (measure_tool->core->win, measure_tool->core->gc,
			 x[i] - TARGET, y[i], 
			 x[i] + TARGET, y[i]);
	  gdk_draw_line (measure_tool->core->win, measure_tool->core->gc,
			 x[i], y[i] - TARGET, 
			 x[i], y[i] + TARGET);
	}
      if (i > 0)
	{
	  gdk_draw_line (measure_tool->core->win, measure_tool->core->gc,
			 x[0], y[0],
			 x[i], y[i]);
	  /*  only draw the arc if the lines are long enough  */
	  if ((SQR (x[i] - x[0]) + SQR (y[i] - y[0])) > ARC_RADIUS_2)
	    draw_arc++;
	}
    }
  
  if (measure_tool->num_points > 1 && draw_arc == measure_tool->num_points - 1)
    {
      angle1 = measure_tool->angle2 * 64.0;
      angle2 = (measure_tool->angle1 - measure_tool->angle2) * 64.0;
  
      if (angle2 > 11520)
	  angle2 -= 23040;
      if (angle2 < -11520)
	  angle2 += 23040;
     
      if (angle2 != 0)
	{
	  gdk_draw_arc (measure_tool->core->win, measure_tool->core->gc, FALSE,
			x[0] - ARC_RADIUS, y[0] - ARC_RADIUS,
			2 * ARC_RADIUS, 2 * ARC_RADIUS, 
			angle1, angle2); 
	  
	  if (measure_tool->num_points == 2)
	    gdk_draw_line (measure_tool->core->win, measure_tool->core->gc,
			   x[0], y[0],
			   x[1] - x[0] < 0 ? x[0] - ARC_RADIUS - 4 : x[0] + ARC_RADIUS + 4, 
			   y[0]);
	}
    }
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

static void
measure_tool_info_update (void)
{
  info_dialog_update (measure_tool_info);
  info_dialog_popup  (measure_tool_info);
}

static void
measure_tool_info_window_close_callback (GtkWidget *widget,
					 gpointer   client_data)
{
  info_dialog_free (measure_tool_info);
  measure_tool_info = NULL;
}

Tool *
tools_new_measure_tool (void)
{
  Tool * tool;
  MeasureTool * private;

  /*  The tool options  */
  if (! measure_tool_options)
    {
      measure_tool_options = measure_tool_options_new ();
      tools_register (MEASURE, (ToolOptions *) measure_tool_options);
    }

  tool = tools_new_tool (MEASURE);
  private = g_new (MeasureTool, 1);

  private->core       = draw_core_new (measure_tool_draw);
  private->num_points = 0;
  private->function   = CREATING;

  tool->private = (void *) private;
 
  tool->preserve = TRUE;  /*  Preserve tool across drawable changes  */

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

  if (measure_tool_info)
    {
      info_dialog_free (measure_tool_info);
      measure_tool_info = NULL;
    }
  g_free (measure_tool);
}
