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

#include <stdlib.h>

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"
#include "gui/gui-types.h"

#include "core/gimpimage.h"

#include "gui/info-dialog.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell.h"

#include "gimpdrawtool.h"
#include "gimpmeasuretool.h"
#include "tool_manager.h"
#include "tool_options.h"

#include "gimprc.h"
#include "undo.h"

#include "libgimp/gimpintl.h"


/*  definitions  */
#define  TARGET         8
#define  ARC_RADIUS     30
#define  ARC_RADIUS_2   1100
#define  STATUSBAR_SIZE 128

/*  maximum information buffer size  */
#define  MAX_INFO_BUF   16

/*  the measure tool options  */
typedef struct _MeasureOptions MeasureOptions;

struct _MeasureOptions
{
  GimpToolOptions  tool_options;

  gboolean     use_info_window;
  gboolean     use_info_window_d;
  GtkWidget   *use_info_window_w;
};


/*  local function prototypes  */
static void   gimp_measure_tool_class_init      (GimpMeasureToolClass *klass);
static void   gimp_measure_tool_init            (GimpMeasureTool      *tool);

static void   gimp_measure_tool_control         (GimpTool        *tool,
                                                 ToolAction       action,
                                                 GimpDisplay     *gdisp);
static void   gimp_measure_tool_button_press    (GimpTool        *tool,
                                                 GimpCoords      *coords,
                                                 guint32          time,
                                                 GdkModifierType  state,
                                                 GimpDisplay     *gdisp);
static void   gimp_measure_tool_button_release  (GimpTool        *tool,
                                                 GimpCoords      *coords,
                                                 guint32          time,
                                                 GdkModifierType  state,
                                                 GimpDisplay     *gdisp);
static void   gimp_measure_tool_motion          (GimpTool        *tool,
                                                 GimpCoords      *coords,
                                                 guint32          time,
                                                 GdkModifierType  state,
                                                 GimpDisplay     *gdisp);
static void   gimp_measure_tool_cursor_update   (GimpTool        *tool,
                                                 GimpCoords      *coords,
                                                 GdkModifierType  state,
                                                 GimpDisplay     *gdisp);

static void   gimp_measure_tool_draw            (GimpDrawTool    *draw_tool);



static gdouble measure_get_angle                      (gint         dx,
                                                       gint         dy,
                                                       gdouble      xres,
                                                       gdouble      yres);
static MeasureOptions * measure_tool_options_new      (void);
static void   measure_tool_options_reset              (GimpToolOptions *tool_options);
static void   measure_tool_info_window_close_callback (GtkWidget   *widget,
						       gpointer     data);
static void   measure_tool_info_update                (void);


static MeasureOptions *measure_tool_options = NULL;

/*  the measure tool info window  */
static InfoDialog *measure_tool_info = NULL;
static gchar       distance_buf[MAX_INFO_BUF];
static gchar       angle_buf[MAX_INFO_BUF];

static GimpDrawToolClass *parent_class = NULL;


void
gimp_measure_tool_register (Gimp *gimp)
{
  tool_manager_register_tool (gimp,
			      GIMP_TYPE_MEASURE_TOOL,
                              FALSE,
			      "gimp:measure_tool",
			      _("Measure Tool"),
			      _("Measure angles and lengths"),
			      N_("/Tools/Measure"), NULL,
			      NULL, "tools/measure.html",
			      GIMP_STOCK_TOOL_MEASURE);
}

GType
gimp_measure_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpMeasureToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_measure_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpMeasureTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_measure_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_DRAW_TOOL,
					  "GimpMeasureTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_measure_tool_class_init (GimpMeasureToolClass *klass)
{
  GimpToolClass     *tool_class;
  GimpDrawToolClass *draw_tool_class;

  tool_class      = GIMP_TOOL_CLASS (klass);
  draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  tool_class->control        = gimp_measure_tool_control;
  tool_class->button_press   = gimp_measure_tool_button_press;
  tool_class->button_release = gimp_measure_tool_button_release;
  tool_class->motion         = gimp_measure_tool_motion;
  tool_class->cursor_update  = gimp_measure_tool_cursor_update;

  draw_tool_class->draw      = gimp_measure_tool_draw;
}

static void
gimp_measure_tool_init (GimpMeasureTool *measure_tool)
{
  GimpTool *tool;

  tool = GIMP_TOOL (measure_tool);
 
  /*  The tool options  */
  if (! measure_tool_options)
    {
      measure_tool_options = measure_tool_options_new ();

      tool_manager_register_tool_options (GIMP_TYPE_MEASURE_TOOL,
					  (GimpToolOptions *) measure_tool_options);
    }

  tool->tool_cursor = GIMP_MEASURE_TOOL_CURSOR;

  tool->preserve    = TRUE;  /*  Preserve on drawable change  */
}

static void
gimp_measure_tool_control (GimpTool    *tool,
                           ToolAction   action,
                           GimpDisplay *gdisp)
{
  GimpMeasureTool  *measure_tool;
  GimpDisplayShell *shell;

  measure_tool = GIMP_MEASURE_TOOL (tool);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      break;

    case HALT:
      gtk_statusbar_pop (GTK_STATUSBAR (shell->statusbar),
			 measure_tool->context_id);
      tool->state = INACTIVE;
      break;

    default:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}

static void
gimp_measure_tool_button_press (GimpTool        *tool,
                                GimpCoords      *coords,
                                guint32          time,
                                GdkModifierType  state,
                                GimpDisplay     *gdisp)
{
  GimpMeasureTool  *measure_tool;
  GimpDisplayShell *shell;
  gint              i;

  measure_tool = GIMP_MEASURE_TOOL (tool);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  /*  if we are changing displays, pop the statusbar of the old one  */ 
  if (tool->state == ACTIVE && gdisp != tool->gdisp)
    {
      GimpDisplay *old_gdisp = tool->gdisp;

      gtk_statusbar_pop (GTK_STATUSBAR (GIMP_DISPLAY_SHELL (old_gdisp->shell)->statusbar),
			 measure_tool->context_id);

      gtk_statusbar_push (GTK_STATUSBAR (shell->statusbar), 
			  measure_tool->context_id, (""));
    } 
  
  measure_tool->function = CREATING;

  if (tool->state == ACTIVE && gdisp == tool->gdisp)
    {
      gint target_w, target_h;

      target_w = UNSCALEX (gdisp, TARGET);
      target_h = UNSCALEY (gdisp, TARGET);

      /*  if the cursor is in one of the handles,
       *  the new function will be moving or adding a new point or guide
       */
      for (i = 0; i < measure_tool->num_points; i++)
	{
	  if (coords->x == CLAMP (coords->x,
                                  measure_tool->x[i] - target_w,
                                  measure_tool->x[i] + target_w) &&
	      coords->y == CLAMP (coords->y,
                                  measure_tool->y[i] - target_h,
                                  measure_tool->y[i] + target_h))
	    {
	      if (state & (GDK_CONTROL_MASK | GDK_MOD1_MASK))
		{
                  gboolean   create_hguide;
                  gboolean   create_vguide;
		  GimpGuide *guide;

                  create_hguide = ((state & GDK_CONTROL_MASK) && 
                                   (measure_tool->y[i] ==
                                    CLAMP (measure_tool->y[i],
                                           0,
                                           gdisp->gimage->height)));

                  create_vguide = ((state & GDK_MOD1_MASK) &&
                                   (measure_tool->x[i] ==
                                    CLAMP (measure_tool->x[i],
                                           0,
                                           gdisp->gimage->width)));

		  if (create_hguide && create_vguide)
		    undo_push_group_start (gdisp->gimage, GUIDE_UNDO);

		  if (create_hguide)
		    {
		      guide = gimp_image_add_hguide (gdisp->gimage);
		      undo_push_guide (gdisp->gimage, guide);
		      guide->position = measure_tool->y[i];
		      gdisplays_expose_guide (gdisp->gimage, guide);
		    }

		  if (create_vguide)
		    {
		      guide = gimp_image_add_vguide (gdisp->gimage);
		      undo_push_guide (gdisp->gimage, guide);
		      guide->position = measure_tool->x[i];
		      gdisplays_expose_guide (gdisp->gimage, guide);
		    }

		  if (create_hguide && create_vguide)
		    undo_push_group_end (gdisp->gimage);

		  if (create_hguide || create_vguide)
                    gdisplays_flush ();

		  measure_tool->function = GUIDING;
		  break;
		}

	      measure_tool->function = (state & GDK_SHIFT_MASK) ? ADDING : MOVING;
	      measure_tool->point = i;
	      break;
	    }
	}

      /*  adding to the middle point makes no sense  */
      if (i == 0 &&
          measure_tool->function == ADDING &&
          measure_tool->num_points == 3)
        {
          measure_tool->function = MOVING;
        }

      /*  if the function is still CREATING, we are outside the handles  */
      if (measure_tool->function == CREATING) 
	{
	  if (measure_tool->num_points > 1 && (state & GDK_MOD1_MASK))
	    {
	      measure_tool->function = MOVING_ALL;

              measure_tool->last_x = coords->x;
              measure_tool->last_y = coords->y;
	    }
	}
    }
  
  if (measure_tool->function == CREATING)
    {
      if (tool->state == ACTIVE)
	{
	  /* reset everything */
	  gimp_draw_tool_stop (GIMP_DRAW_TOOL (measure_tool));

	  gtk_statusbar_pop (GTK_STATUSBAR (shell->statusbar),
			     measure_tool->context_id);
	  gtk_statusbar_push (GTK_STATUSBAR (shell->statusbar),
			      measure_tool->context_id, "");

	  distance_buf[0] = '\0';
	  angle_buf[0] = '\0';
	  if (measure_tool_info)
	    measure_tool_info_update ();
	}
      else
	{
	  /* initialize the statusbar display */
	  measure_tool->context_id =
	    gtk_statusbar_get_context_id (GTK_STATUSBAR (shell->statusbar),
					  "measure");
	}

      /*  set the first point and go into ADDING mode  */
      measure_tool->x[0]       = coords->x;
      measure_tool->y[0]       = coords->y;
      measure_tool->point      = 0;
      measure_tool->num_points = 1;
      measure_tool->function   = ADDING;

      /*  set the gdisplay  */
      tool->gdisp = gdisp;

      /*  start drawing the measure tool  */
      gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), shell->canvas->window);
    }

  /*  create the info window if necessary  */
  if (! measure_tool_info &&
      (measure_tool_options->use_info_window ||
       ! GTK_WIDGET_VISIBLE (shell->statusarea)))
    {
      measure_tool_info = info_dialog_new (_("Measure Tool"),
					   tool_manager_help_func, NULL);
      info_dialog_add_label (measure_tool_info, _("Distance:"), distance_buf);
      info_dialog_add_label (measure_tool_info, _("Angle:"), angle_buf);

      gimp_dialog_create_action_area (GIMP_DIALOG (measure_tool_info->shell),

				      GTK_STOCK_CLOSE,
				      measure_tool_info_window_close_callback,
				      measure_tool_info, NULL, NULL, TRUE, FALSE,

				      NULL);
    }

  gdk_pointer_grab (shell->canvas->window, FALSE,
		    GDK_POINTER_MOTION_HINT_MASK |
		    GDK_BUTTON1_MOTION_MASK |
		    GDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, time);

  tool->state = ACTIVE;

  /*  set the pointer to the crosshair,
   *  so one actually sees the cursor position
   */
  gimp_display_shell_install_tool_cursor (shell,
                                          GIMP_CROSSHAIR_SMALL_CURSOR,
                                          GIMP_MEASURE_TOOL_CURSOR,
                                          GIMP_CURSOR_MODIFIER_NONE);
}

static void
gimp_measure_tool_button_release (GimpTool        *tool,
                                  GimpCoords      *coords,
                                  guint32          time,
                                  GdkModifierType  state,
                                  GimpDisplay     *gdisp)
{
  GimpMeasureTool *measure_tool;

  measure_tool = GIMP_MEASURE_TOOL (tool);

  measure_tool->function = FINISHED;

  gdk_pointer_ungrab (time);
  gdk_flush ();
}

static void
gimp_measure_tool_motion (GimpTool        *tool,
                          GimpCoords      *coords,
                          guint32          time,
                          GdkModifierType  state,
                          GimpDisplay     *gdisp)
{
  GimpMeasureTool  *measure_tool;
  GimpDisplayShell *shell;
  gint              ax, ay;
  gint              bx, by;
  gint              dx, dy;
  gint              i;
  gint              tmp;
  gdouble           angle;
  gdouble           distance;
  gchar             status_str[STATUSBAR_SIZE];

  measure_tool = GIMP_MEASURE_TOOL (tool);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL(measure_tool));

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

      measure_tool->x[i] = ROUND (coords->x);
      measure_tool->y[i] = ROUND (coords->y);

      /*  restrict to horizontal/vertical movements, if modifiers are pressed */
      if (state & GDK_MOD1_MASK)
	{
	  if (state & GDK_CONTROL_MASK)
	    {
	      dx = measure_tool->x[i] - measure_tool->x[0];
	      dy = measure_tool->y[i] - measure_tool->y[0];

	      measure_tool->x[i] = measure_tool->x[0] +
		(dx > 0 ? MAX (abs (dx), abs (dy)) : - MAX (abs (dx), abs (dy)));
	      measure_tool->y[i] = measure_tool->y[0] +
		(dy > 0  ? MAX (abs (dx), abs (dy)) : - MAX (abs (dx), abs (dy)));
	    }
	  else
            {
              measure_tool->x[i] = measure_tool->x[0];
            }
	}
      else if (state & GDK_CONTROL_MASK)
        {
          measure_tool->y[i] = measure_tool->y[0];
        }
      break;

    case MOVING_ALL:
      dx = ROUND (coords->x) - measure_tool->last_x;
      dy = ROUND (coords->y) - measure_tool->last_y;

      for (i = 0; i < measure_tool->num_points; i++)
	{
	  measure_tool->x[i] += dx;
	  measure_tool->y[i] += dy;
	}

      measure_tool->last_x = ROUND (coords->x);
      measure_tool->last_y = ROUND (coords->y);
      break;

    default:
      break;
    }

  if (measure_tool->function == MOVING)
    {
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
	  bx = 0;
	  by = 0;
	}

      if (gdisp->dot_for_dot)
	{
	  distance = sqrt (SQR (ax - bx) + SQR (ay - by));
	
	  if (measure_tool->num_points != 3)
	    bx = ax > 0 ? 1 : -1;
	
	  measure_tool->angle1 = measure_get_angle (ax, ay, 1.0, 1.0);
	  measure_tool->angle2 = measure_get_angle (bx, by, 1.0, 1.0);
	  angle = fabs (measure_tool->angle1 - measure_tool->angle2);
	  if (angle > 180.0)
	    angle = fabs (360.0 - angle);
	
	  g_snprintf (status_str, sizeof (status_str), "%.1f %s, %.2f %s",
		      distance, _("pixels"), angle, _("degrees"));
	
	  if (measure_tool_options)
	    {
	      g_snprintf (distance_buf, sizeof (distance_buf), "%.1f %s",
                          distance, _("pixels"));
	      g_snprintf (angle_buf, sizeof (angle_buf), "%.2f %s",
                          angle, _("degrees"));
	    }
	}
      else /* show real world units */
	{
	  gchar *format_str =
	    g_strdup_printf ("%%.%df %s, %%.2f %s",
			     gimp_unit_get_digits (gdisp->gimage->unit),
			     gimp_unit_get_symbol (gdisp->gimage->unit),
			     _("degrees"));

	  distance =  gimp_unit_get_factor (gdisp->gimage->unit) *
	    sqrt (SQR ((gdouble)(ax - bx) / gdisp->gimage->xresolution) +
		  SQR ((gdouble)(ay - by) / gdisp->gimage->yresolution));

	  if (measure_tool->num_points != 3)
	    bx = ax > 0 ? 1 : -1;

	  measure_tool->angle1 = measure_get_angle (ax, ay,
						    gdisp->gimage->xresolution,
						    gdisp->gimage->yresolution);
	  measure_tool->angle2 = measure_get_angle (bx, by,
						    gdisp->gimage->xresolution,
						    gdisp->gimage->yresolution);
	  angle = fabs (measure_tool->angle1 - measure_tool->angle2);
	  if (angle > 180.0)
	    angle = fabs (360.0 - angle);

	  g_snprintf (status_str, sizeof (status_str), format_str,
                      distance, angle);
	  g_free (format_str);

	  if (measure_tool_options)
	    {
	      gchar *format_str =
		g_strdup_printf ("%%.%df %s",
				 gimp_unit_get_digits (gdisp->gimage->unit),
				 gimp_unit_get_symbol (gdisp->gimage->unit));
	      g_snprintf (distance_buf, sizeof (distance_buf), format_str,
                          distance);
	      g_snprintf (angle_buf, sizeof (angle_buf), "%.2f %s",
                          angle, _("degrees"));
	      g_free (format_str);
	    }
	}

      /*  show info in statusbar  */
      gtk_statusbar_pop (GTK_STATUSBAR (shell->statusbar),
			 measure_tool->context_id);
      gtk_statusbar_push (GTK_STATUSBAR (shell->statusbar),
			  measure_tool->context_id,
			  status_str);

      /*  and in the info window  */
      if (measure_tool_info)
	measure_tool_info_update ();

    }  /*  measure_tool->function == MOVING  */

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (measure_tool));
}

static void
gimp_measure_tool_cursor_update (GimpTool        *tool,
                                 GimpCoords      *coords,
                                 GdkModifierType  state,
                                 GimpDisplay     *gdisp)
{
  GimpMeasureTool   *measure_tool;
  GimpDisplayShell  *shell;
  gboolean           in_handle = FALSE;
  GdkCursorType      ctype     = GIMP_CROSSHAIR_SMALL_CURSOR;
  GimpCursorModifier cmodifier = GIMP_CURSOR_MODIFIER_NONE;
  gint               i;

  measure_tool = GIMP_MEASURE_TOOL (tool);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  if (tool->state == ACTIVE && tool->gdisp == gdisp)
    {
      gint target_w, target_h;

      target_w = UNSCALEX (gdisp, TARGET);
      target_h = UNSCALEY (gdisp, TARGET);

      for (i = 0; i < measure_tool->num_points; i++)
	{
	  if (coords->x == CLAMP (coords->x,
                                  measure_tool->x[i] - target_w,
                                  measure_tool->x[i] + target_w) &&
	      coords->y == CLAMP (coords->y,
                                  measure_tool->y[i] - target_h,
                                  measure_tool->y[i] + target_h))
	    {
	      in_handle = TRUE;

	      if (state & GDK_CONTROL_MASK)
		{
		  if (state & GDK_MOD1_MASK)
		    ctype = GDK_BOTTOM_RIGHT_CORNER;
		  else
		    ctype = GDK_BOTTOM_SIDE;
		  break;
		}

	      if (state & GDK_MOD1_MASK)
		{
		  ctype = GDK_RIGHT_SIDE;
		  break;
		}

	      if (state & GDK_SHIFT_MASK)
		cmodifier = GIMP_CURSOR_MODIFIER_PLUS;
	      else
		cmodifier = GIMP_CURSOR_MODIFIER_MOVE;

	      if (i == 0 && measure_tool->num_points == 3 &&
		  cmodifier == GIMP_CURSOR_MODIFIER_PLUS)
		cmodifier = GIMP_CURSOR_MODIFIER_MOVE;
	      break;
	    }
	}

      if (! in_handle && measure_tool->num_points > 1 && (state & GDK_MOD1_MASK))
	cmodifier = GIMP_CURSOR_MODIFIER_MOVE;
    }

  gimp_display_shell_install_tool_cursor (shell,
                                          ctype,
                                          GIMP_MEASURE_TOOL_CURSOR,
                                          cmodifier);
}

static void
gimp_measure_tool_draw (GimpDrawTool *draw_tool)
{
  GimpMeasureTool *measure_tool;
  GimpTool        *tool;
  gint             x[3];
  gint             y[3];
  gint             i;
  gint             angle1, angle2;
  gint             draw_arc = 0;

  measure_tool = GIMP_MEASURE_TOOL (draw_tool);
  tool         = GIMP_TOOL (draw_tool);

  for (i = 0; i < measure_tool->num_points; i++)
    {
      gdisplay_transform_coords (tool->gdisp,
				 measure_tool->x[i], measure_tool->y[i],
				 &x[i], &y[i],
                                 FALSE);

      if (i == 0 && measure_tool->num_points == 3)
	{
	  gdk_draw_arc (draw_tool->win, draw_tool->gc, FALSE,
			x[i] - (TARGET >> 1), y[i] - (TARGET >> 1),
			TARGET, TARGET, 0, 23040);
	}
      else
	{
	  gdk_draw_line (draw_tool->win, draw_tool->gc,
			 x[i] - TARGET, y[i],
			 x[i] + TARGET, y[i]);
	  gdk_draw_line (draw_tool->win, draw_tool->gc,
			 x[i], y[i] - TARGET,
			 x[i], y[i] + TARGET);
	}
      if (i > 0)
	{
	  gdk_draw_line (draw_tool->win, draw_tool->gc,
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
	  gdk_draw_arc (draw_tool->win, draw_tool->gc, FALSE,
			x[0] - ARC_RADIUS, y[0] - ARC_RADIUS,
			2 * ARC_RADIUS, 2 * ARC_RADIUS,
			angle1, angle2);

	  if (measure_tool->num_points == 2)
	    gdk_draw_line (draw_tool->win, draw_tool->gc,
			   x[0], y[0],
			   x[1] - x[0] <= 0 ? x[0] - ARC_RADIUS - (TARGET >> 1) :
			                      x[0] + ARC_RADIUS + (TARGET >> 1),
			   y[0]);
	}
    }
}

static void
measure_tool_options_reset (GimpToolOptions *tool_options)
{
  MeasureOptions *options;

  options = (MeasureOptions *) tool_options;

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->use_info_window_w),
				options->use_info_window_d);
}

static MeasureOptions *
measure_tool_options_new (void)
{
  MeasureOptions *options;
  GtkWidget      *vbox;

  options = g_new0 (MeasureOptions, 1);
  tool_options_init ((GimpToolOptions *) options,
		     measure_tool_options_reset);

  options->use_info_window = options->use_info_window_d  = FALSE;

    /*  the main vbox  */
  vbox = options->tool_options.main_vbox;

  /*  the use_info_window toggle button  */
  options->use_info_window_w =
    gtk_check_button_new_with_label (_("Use Info Window"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->use_info_window_w),
				options->use_info_window_d);
  gtk_box_pack_start (GTK_BOX (vbox), options->use_info_window_w,
		      FALSE, FALSE, 0);
  gtk_widget_show (options->use_info_window_w);

  g_signal_connect (G_OBJECT (options->use_info_window_w), "toggled",
		    G_CALLBACK (gimp_toggle_button_update),
		    &options->use_info_window);

  return options;
}

static gdouble
measure_get_angle (gint    dx,
		   gint    dy,
		   gdouble xres,
		   gdouble yres)
{
  gdouble angle;

  if (dx)
    angle = gimp_rad_to_deg (atan (((gdouble) (dy) / yres) /
				   ((gdouble) (dx) / xres)));
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
    {
      angle = 180.0 - angle;
    }

  return angle;
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
