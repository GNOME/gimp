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
#include <string.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"

#include "tools-types.h"

#include "base/brush-scale.h"
#include "base/pixel-region.h"
#include "base/temp-buf.h"
#include "base/tile-manager.h"
#include "base/tile.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimp.h"
#include "core/gimpbrushpipe.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpgradient.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimpmarshal.h"
#include "core/gimptoolinfo.h"

#include "paint/gimppaintcore.h"

#include "widgets/gimpdevices.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpstatusbar.h"

#include "gimppainttool.h"

#include "app_procs.h"
#include "gimprc.h"
#include "undo.h"

#include "libgimp/gimpintl.h"


#define TARGET_SIZE    15
#define STATUSBAR_SIZE 128


/*  local function prototypes  */

static void   gimp_paint_tool_class_init     (GimpPaintToolClass  *klass);
static void   gimp_paint_tool_init           (GimpPaintTool       *paint_tool);

static void   gimp_paint_tool_finalize       (GObject             *object);

static void   gimp_paint_tool_control        (GimpTool	           *tool,
					      ToolAction           action,
					      GimpDisplay         *gdisp);
static void   gimp_paint_tool_button_press   (GimpTool            *tool,
                                              GimpCoords          *coords,
                                              guint32              time,
					      GdkModifierType      state,
					      GimpDisplay         *gdisp);
static void   gimp_paint_tool_button_release (GimpTool            *tool,
                                              GimpCoords          *coords,
                                              guint32              time,
					      GdkModifierType      state,
					      GimpDisplay         *gdisp);
static void   gimp_paint_tool_motion         (GimpTool            *tool,
                                              GimpCoords          *coords,
                                              guint32              time,
					      GdkModifierType      state,
					      GimpDisplay         *gdisp);
static void   gimp_paint_tool_cursor_update  (GimpTool            *tool,
                                              GimpCoords          *coords,
					      GdkModifierType      state,
					      GimpDisplay         *gdisp);

static void   gimp_paint_tool_draw           (GimpDrawTool        *draw_tool);

static void   gimp_paint_tool_sample_color   (GimpDrawable        *drawable,
                                              gint                 x,
                                              gint                 y,
                                              gint                 state);


static GimpDrawToolClass *parent_class = NULL;


GType
gimp_paint_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpPaintToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_paint_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpPaintTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_paint_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_DRAW_TOOL,
					  "GimpPaintTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_paint_tool_class_init (GimpPaintToolClass *klass)
{
  GObjectClass      *object_class;
  GimpToolClass     *tool_class;
  GimpDrawToolClass *draw_tool_class;

  object_class    = G_OBJECT_CLASS (klass);
  tool_class      = GIMP_TOOL_CLASS (klass);
  draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize     = gimp_paint_tool_finalize;

  tool_class->control        = gimp_paint_tool_control;
  tool_class->button_press   = gimp_paint_tool_button_press;
  tool_class->button_release = gimp_paint_tool_button_release;
  tool_class->motion         = gimp_paint_tool_motion;
  tool_class->cursor_update  = gimp_paint_tool_cursor_update;

  draw_tool_class->draw      = gimp_paint_tool_draw;
}

static void
gimp_paint_tool_init (GimpPaintTool *paint_tool)
{
  GimpTool *tool;

  tool = GIMP_TOOL (paint_tool);

  tool->perfectmouse = TRUE;

  paint_tool->state          = 0;
  paint_tool->pick_colors    = FALSE;
  paint_tool->pick_state     = FALSE;
}

static void
gimp_paint_tool_finalize (GObject *object)
{
  GimpPaintTool *paint_tool;

  paint_tool = GIMP_PAINT_TOOL (object);

  if (paint_tool->core)
    {
      g_object_unref (G_OBJECT (paint_tool->core));
      paint_tool->core = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_paint_tool_control (GimpTool    *tool,
			 ToolAction   action,
			 GimpDisplay *gdisp)
{
  GimpPaintTool *paint_tool;
  GimpDrawable  *drawable;

  paint_tool = GIMP_PAINT_TOOL (tool);
  drawable   = gimp_image_active_drawable (gdisp->gimage);

  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      break;

    case HALT:
      gimp_paint_core_paint (paint_tool->core, drawable,
                             (PaintOptions *) tool->tool_info->tool_options,
                             FINISH_PAINT);
      gimp_paint_core_cleanup (paint_tool->core);
      break;

    default:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}

static void
gimp_paint_tool_button_press (GimpTool        *tool,
                              GimpCoords      *coords,
                              guint32          time,
			      GdkModifierType  state,
			      GimpDisplay     *gdisp)
{
  GimpDrawTool  *draw_tool;
  GimpPaintTool *paint_tool;
  GimpPaintCore *core;
  PaintOptions  *paint_options;
  GimpBrush     *current_brush;
  gboolean       draw_line;
  GimpDrawable  *drawable;
  GimpCoords     curr_coords;
  gint           off_x, off_y;

  draw_tool  = GIMP_DRAW_TOOL (tool);
  paint_tool = GIMP_PAINT_TOOL (tool);

  core = paint_tool->core;

  paint_options = (PaintOptions *) tool->tool_info->tool_options;

  drawable = gimp_image_active_drawable (gdisp->gimage);

  curr_coords = *coords;

  gimp_drawable_offsets (drawable, &off_x, &off_y);

  curr_coords.x -= off_x;
  curr_coords.y -= off_y;

  if (draw_tool->gdisp)
    gimp_draw_tool_stop (draw_tool);

  if (! gimp_paint_core_start (core, drawable, &curr_coords))
    return;

  draw_line = FALSE;

  paint_tool->state = state;

  /*  if this is a new image, reinit the core vals  */
  if ((gdisp != tool->gdisp) || ! (state & GDK_SHIFT_MASK))
    {
      core->start_coords = core->cur_coords;
      core->last_coords  = core->cur_coords;
    }

  /*  If shift is down and this is not the first paint
   *  stroke, then draw a line from the last coords to the pointer
   */
  else if (state & GDK_SHIFT_MASK)
    {
      draw_line = TRUE;

      core->start_coords = core->last_coords;

      /* Restrict to multiples of 15 degrees if ctrl is pressed */
      if (state & GDK_CONTROL_MASK)
	{
	  static const gint tangens2[6] = {  34, 106, 196, 334, 618, 1944    };
	  static const gint cosinus[7]  = { 256, 247, 222, 181, 128,   66, 0 };

	  gint dx, dy, i, radius, frac;

	  dx = core->cur_coords.x - core->last_coords.x;
	  dy = core->cur_coords.y - core->last_coords.y;

	  if (dy)
	    {
	      radius = sqrt (SQR (dx) + SQR (dy));
	      frac   = abs ((dx << 8) / dy);

	      for (i = 0; i < 6; i++)
		{
		  if (frac < tangens2[i])
		    break;
		}

	      dx = (dx > 0 ?
                       (cosinus[6-i] * radius) >> 8 :
                    - ((cosinus[6-i] * radius) >> 8));

	      dy = (dy > 0 ?
                       (cosinus[i] * radius)   >> 8 :
                    - ((cosinus[i] * radius)   >> 8));
	    }

	  core->cur_coords.x = core->last_coords.x + dx;
	  core->cur_coords.y = core->last_coords.y + dy;
	}
    }

  tool->state        = ACTIVE;
  tool->gdisp        = gdisp;
  tool->paused_count = 0;

  /*  pause the current selection  */
  gimp_image_selection_control (gdisp->gimage, GIMP_SELECTION_PAUSE);

  /*  Let the specific painting function initialize itself  */
  gimp_paint_core_paint (core, drawable, paint_options, INIT_PAINT);

  if (paint_tool->pick_colors    &&
      ! (state & GDK_SHIFT_MASK) &&
      (state & (GDK_CONTROL_MASK | GDK_MOD1_MASK)))
    {
      gimp_paint_tool_sample_color (drawable,
                                    curr_coords.x,
                                    curr_coords.y,
                                    state);
      paint_tool->pick_state = TRUE;
      return;
    }
  else
    {
      paint_tool->pick_state = FALSE;
    }

  /*  store the current brush pointer  */
  current_brush = core->brush;

  if (core->flags & CORE_TRACES_ON_WINDOW)
    gimp_paint_core_paint (core, drawable, paint_options, PRETRACE_PAINT);

  /*  Paint to the image  */
  if (draw_line)
    {
      gimp_paint_core_interpolate (core, drawable, paint_options);

      core->last_coords = core->cur_coords;
    }
  else
    {
      /* If we current point == last point, check if the brush
       * wants to be painted in that case. (Direction dependent
       * pixmap brush pipes don't, as they don't know which
       * pixmap to select.)
       */
      if (core->last_coords.x != core->cur_coords.x ||
	  core->last_coords.y != core->cur_coords.y ||
	  gimp_brush_want_null_motion (core->brush,
                                       &core->last_coords,
                                       &core->cur_coords))
	{
	  if (core->flags & CORE_CAN_HANDLE_CHANGING_BRUSH)
	    {
	      core->brush = gimp_brush_select_brush (core->brush,
                                                     &core->last_coords,
                                                     &core->cur_coords);
	    }

	  gimp_paint_core_paint (core, drawable, paint_options, MOTION_PAINT);
	}
    }

  gimp_display_flush_now (gdisp);

  if (core->flags & CORE_TRACES_ON_WINDOW)
    gimp_paint_core_paint (core, drawable, paint_options, POSTTRACE_PAINT);

  /*  restore the current brush pointer  */
  core->brush = current_brush;
}

static void
gimp_paint_tool_button_release (GimpTool        *tool,
                                GimpCoords      *coords,
                                guint32          time,
				GdkModifierType  state,
				GimpDisplay     *gdisp)
{
  GimpPaintTool *paint_tool;
  GimpPaintCore *core;
  PaintOptions  *paint_options;
  GimpDrawable  *drawable;

  paint_tool = GIMP_PAINT_TOOL (tool);

  core = paint_tool->core;

  paint_options = (PaintOptions *) tool->tool_info->tool_options;

  drawable = gimp_image_active_drawable (gdisp->gimage);

  /*  resume the current selection  */
  gimp_image_selection_control (gdisp->gimage, GIMP_SELECTION_RESUME);

  /*  Let the specific painting function finish up  */
  gimp_paint_core_paint (core, drawable, paint_options, FINISH_PAINT);

  /*  Set tool state to inactive -- no longer painting */
  tool->state = INACTIVE;

  paint_tool->pick_state = FALSE;

  gimp_paint_core_finish (core, drawable);

  gdisplays_flush ();
}

static void
gimp_paint_tool_motion (GimpTool        *tool,
                        GimpCoords      *coords,
                        guint32          time,
			GdkModifierType  state,
			GimpDisplay     *gdisp)
{
  GimpPaintTool *paint_tool;
  GimpPaintCore *core;
  PaintOptions  *paint_options;
  GimpDrawable  *drawable;

  paint_tool = GIMP_PAINT_TOOL (tool);

  core = paint_tool->core;

  paint_options = (PaintOptions *) tool->tool_info->tool_options;

  drawable = gimp_image_active_drawable (gdisp->gimage);

  paint_tool->state = state;

  {
    gint off_x, off_y;
    
    gimp_drawable_offsets (drawable, &off_x, &off_y);

    core->cur_coords = *coords;

    core->cur_coords.x -= off_x;
    core->cur_coords.y -= off_y;
  }

  if (paint_tool->pick_state)
    {
      gimp_paint_tool_sample_color (drawable,
				    core->cur_coords.x,
                                    core->cur_coords.y,
				    state);
      return;
    }

  if (core->flags & CORE_TRACES_ON_WINDOW)
    gimp_paint_core_paint (core, drawable, paint_options, PRETRACE_PAINT);

  gimp_paint_core_interpolate (core, drawable, paint_options);

  gimp_display_flush_now (gdisp);

  if (core->flags & CORE_TRACES_ON_WINDOW)
    gimp_paint_core_paint (core, drawable, paint_options, POSTTRACE_PAINT);

  core->last_coords = core->cur_coords;
}

static void
gimp_paint_tool_cursor_update (GimpTool        *tool,
                               GimpCoords      *coords,
			       GdkModifierType  state,
			       GimpDisplay     *gdisp)
{
  GimpPaintTool    *paint_tool;
  GimpDrawTool     *draw_tool;
  GimpPaintCore    *core;
  GimpDisplayShell *shell;
  GimpLayer        *layer;
  gchar             status_str[STATUSBAR_SIZE];
  gboolean          pick_colors = FALSE;

  paint_tool = GIMP_PAINT_TOOL (tool);
  draw_tool  = GIMP_DRAW_TOOL (tool);

  core = paint_tool->core;

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  /*  undraw the old line (if any)  */
  if (draw_tool->gdisp)
    gimp_draw_tool_stop (draw_tool);

  gimp_statusbar_pop (GIMP_STATUSBAR (shell->statusbar),
                      g_type_name (G_TYPE_FROM_INSTANCE (tool)));

  if ((layer = gimp_image_get_active_layer (gdisp->gimage)))
    {
      gint off_x, off_y;

      gimp_drawable_offsets (GIMP_DRAWABLE (layer), &off_x, &off_y);

      /* If shift is down and this is not the first paint stroke, draw a line */
      if (gdisp == tool->gdisp && (state & GDK_SHIFT_MASK))
	{
	  gdouble dx, dy, d;

	  /*  Get the current coordinates */
          core->cur_coords.x = coords->x - off_x;
          core->cur_coords.y = coords->y - off_y;

	  dx = core->cur_coords.x - core->last_coords.x;
	  dy = core->cur_coords.y - core->last_coords.y;

	  /* Restrict to multiples of 15 degrees if ctrl is pressed */
	  if (state & GDK_CONTROL_MASK)
	    {
	      static const gint tangens2[6] = {  34, 106, 196, 334, 618, 1944 };
	      static const gint cosinus[7]  = { 256, 247, 222, 181, 128,   66, 0 };

	      gint idx = dx;
	      gint idy = dy;
	      gint i, radius, frac;

	      if (idy)
		{
		  radius = sqrt (SQR (idx) + SQR (idy));
		  frac   = abs ((idx << 8) / idy);

		  for (i = 0; i < 6; i++)
		    {
		      if (frac < tangens2[i])
			break;
		    }

		  dx = (idx > 0 ?
                          (cosinus[6-i]  * radius) >> 8 :
                        - ((cosinus[6-i] * radius) >> 8));

		  dy = (idy > 0 ?
                          (cosinus[i]  * radius) >> 8 :
                        - ((cosinus[i] * radius) >> 8));
		}

	      core->cur_coords.x = core->last_coords.x + dx;
	      core->cur_coords.y = core->last_coords.y + dy;
	    }

	  /*  show distance in statusbar  */
	  if (gdisp->dot_for_dot)
	    {
	      d = sqrt (SQR (dx) + SQR (dy));
	      g_snprintf (status_str, sizeof (status_str), "%.1f %s",
                          d, _("pixels"));
	    }
	  else
	    {
	      gchar format_str[64];

              g_snprintf (format_str, sizeof (format_str), "%%.%df %s",
                          gimp_unit_get_digits (gdisp->gimage->unit),
                          gimp_unit_get_symbol (gdisp->gimage->unit));

	      d = (gimp_unit_get_factor (gdisp->gimage->unit) *
		   sqrt (SQR (dx / gdisp->gimage->xresolution) +
			 SQR (dy / gdisp->gimage->yresolution)));

	      g_snprintf (status_str, sizeof (status_str), format_str, d);
	    }

	  gimp_statusbar_push (GIMP_STATUSBAR (shell->statusbar),
                               g_type_name (G_TYPE_FROM_INSTANCE (tool)),
                               status_str);

          gimp_draw_tool_start (draw_tool, gdisp);
	}
      /* If Ctrl or Mod1 is pressed, pick colors */
      else if (paint_tool->pick_colors &&
	       ! (state & GDK_SHIFT_MASK) &&
	       (state & (GDK_CONTROL_MASK | GDK_MOD1_MASK)))
        {
	  pick_colors = TRUE;
	}
    }

  if (pick_colors)
    {
      gimp_tool_set_cursor (tool, gdisp,
                            GIMP_COLOR_PICKER_CURSOR,
                            GIMP_COLOR_PICKER_TOOL_CURSOR,
                            GIMP_CURSOR_MODIFIER_NONE);
    }
  else
    {
      GIMP_TOOL_CLASS (parent_class)->cursor_update (tool,
                                                     coords,
                                                     state,
                                                     gdisp);
    }
}

static void
gimp_paint_tool_draw (GimpDrawTool *draw_tool)
{
  if (draw_tool->gdisp)
    {
      GimpPaintTool *paint_tool;
      GimpPaintCore *core;

      paint_tool = GIMP_PAINT_TOOL (draw_tool);

      core = paint_tool->core;

      /*  Draw start target  */
      gimp_draw_tool_draw_handle (draw_tool,
                                  GIMP_HANDLE_CROSS,
                                  floor (core->last_coords.x) + 0.5,
                                  floor (core->last_coords.y) + 0.5,
                                  TARGET_SIZE,
                                  TARGET_SIZE,
                                  GTK_ANCHOR_CENTER,
                                  TRUE);

      /*  Draw end target  */
      gimp_draw_tool_draw_handle (draw_tool,
                                  GIMP_HANDLE_CROSS,
                                  floor (core->cur_coords.x) + 0.5,
                                  floor (core->cur_coords.y) + 0.5,
                                  TARGET_SIZE,
                                  TARGET_SIZE,
                                  GTK_ANCHOR_CENTER,
                                  TRUE);

      /*  Draw the line between the start and end coords  */
      gimp_draw_tool_draw_line (draw_tool,
                                floor (core->last_coords.x) + 0.5,
                                floor (core->last_coords.y) + 0.5,
                                floor (core->cur_coords.x) + 0.5,
                                floor (core->cur_coords.y) + 0.5,
                                TRUE);
    }
}

static void
gimp_paint_tool_sample_color (GimpDrawable *drawable,
			      gint          x,
			      gint          y,
			      gint          state)
{
  GimpRGB  color;
  guchar  *col;

  if( x >= 0 && x < gimp_drawable_width (drawable) &&
      y >= 0 && y < gimp_drawable_height (drawable))
    {
      if ((col = gimp_drawable_get_color_at (drawable, x, y)))
	{
	  Gimp *gimp;

	  gimp = gimp_drawable_gimage (drawable)->gimp;

	  gimp_rgba_set_uchar (&color,
			       col[RED_PIX],
			       col[GREEN_PIX],
			       col[BLUE_PIX],
			       255);

	  if ((state & GDK_CONTROL_MASK))
	    gimp_context_set_foreground (gimp_get_user_context (gimp), &color);
	  else
	    gimp_context_set_background (gimp_get_user_context (gimp), &color);

	  g_free (col);
	}
    }
}
