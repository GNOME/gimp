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
#include "libgimpbase/gimpbase.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-pick-color.h"
#include "core/gimptoolinfo.h"

#include "paint/gimppaintcore.h"

#include "widgets/gimpdevices.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpstatusbar.h"

#include "gimpcolorpickertool.h"
#include "gimppainttool.h"
#include "tool_manager.h"

#include "app_procs.h"
#include "gimprc.h"
#include "undo.h"

#include "libgimp/gimpintl.h"


#define TARGET_SIZE    15
#define STATUSBAR_SIZE 128


static void   gimp_paint_tool_class_init     (GimpPaintToolClass  *klass);
static void   gimp_paint_tool_init           (GimpPaintTool       *paint_tool);

static void   gimp_paint_tool_finalize       (GObject             *object);

static void   gimp_paint_tool_control        (GimpTool	          *tool,
					      GimpToolAction       action,
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

  tool->motion_mode       = GIMP_MOTION_MODE_EXACT;

  paint_tool->pick_colors = FALSE;
  paint_tool->pick_state  = FALSE;
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
gimp_paint_tool_control (GimpTool       *tool,
			 GimpToolAction  action,
			 GimpDisplay    *gdisp)
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
      gimp_paint_core_paint (paint_tool->core,
                             drawable,
                             (GimpPaintOptions *) tool->tool_info->tool_options,
                             FINISH_PAINT);
      gimp_paint_core_cleanup (paint_tool->core);

#if 0
      /*  evil hack i'm thinking about...  --mitch  */
      {
        /*  HALT means the current display is going to go away (TM),
         *  so try to find another display of the same image to make
         *  straight line drawing continue to work...
         */

        GSList *list;

        for (list = display_list; list; list = g_slist_next (list))
          {
            GimpDisplay *tmp_disp;

            tmp_disp = (GimpDisplay *) list->data;

            if (tmp_disp != gdisp && tmp_disp->gimage == gdisp->gimage)
              {
                tool->gdisp = tmp_disp;
                break;
              }
          }
      }
#endif
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
  GimpDrawTool     *draw_tool;
  GimpPaintTool    *paint_tool;
  GimpPaintCore    *core;
  GimpPaintOptions *paint_options;
  GimpBrush        *current_brush;
  GimpDrawable     *drawable;
  GimpCoords        curr_coords;
  gboolean          draw_line = FALSE;

  draw_tool  = GIMP_DRAW_TOOL (tool);
  paint_tool = GIMP_PAINT_TOOL (tool);

  core = paint_tool->core;

  paint_options = (GimpPaintOptions *) tool->tool_info->tool_options;

  drawable = gimp_image_active_drawable (gdisp->gimage);

  curr_coords = *coords;

  {
    gint off_x, off_y;

    gimp_drawable_offsets (drawable, &off_x, &off_y);

    curr_coords.x -= off_x;
    curr_coords.y -= off_y;
  }

  if (draw_tool->gdisp)
    gimp_draw_tool_stop (draw_tool);

  if (tool->gdisp          &&
      tool->gdisp != gdisp &&
      tool->gdisp->gimage == gdisp->gimage)
    {
      /*  if this is a different display, but the same image, HACK around
       *  in tool internals AFTER stopping the current draw_tool, so
       *  straight line drawing works across different views of the
       *  same image.
       */

      tool->gdisp = gdisp;
    }

  if (gimp_devices_get_current (gdisp->gimage->gimp) ==
      gdk_device_get_core_pointer ())
    {
      core->use_pressure = FALSE;
    }
  else
    {
      core->use_pressure = TRUE;
    }

  if (! gimp_paint_core_start (core, drawable, &curr_coords))
    return;

  if ((gdisp != tool->gdisp) || ! (state & GDK_SHIFT_MASK))
    {
      /*  if this is a new image, reinit the core vals  */

      core->start_coords = core->cur_coords;
      core->last_coords  = core->cur_coords;
    }
  else if (state & GDK_SHIFT_MASK)
    {
      /*  If shift is down and this is not the first paint
       *  stroke, then draw a line from the last coords to the pointer
       */

      draw_line = TRUE;

      core->start_coords = core->last_coords;

      if (state & GDK_CONTROL_MASK)
	{
          /* Restrict to multiples of 15 degrees if ctrl is pressed */

          gimp_paint_core_constrain (core);
	}
    }

  tool->state = ACTIVE;
  tool->gdisp = gdisp;

  /*  pause the current selection  */
  gimp_image_selection_control (gdisp->gimage, GIMP_SELECTION_PAUSE);

  /*  Let the specific painting function initialize itself  */
  gimp_paint_core_paint (core, drawable, paint_options, INIT_PAINT);

  if (paint_tool->pick_colors    &&
      ! (state & GDK_SHIFT_MASK) &&
      (state & (GDK_CONTROL_MASK | GDK_MOD1_MASK)))
    {
      gimp_paint_tool_sample_color (drawable,
                                    coords->x,
                                    coords->y,
                                    state);

      paint_tool->pick_state = TRUE;

      gimp_draw_tool_start (draw_tool, gdisp);

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
      gimp_paint_core_paint (core, drawable, paint_options, MOTION_PAINT);
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
  GimpPaintTool    *paint_tool;
  GimpPaintCore    *core;
  GimpPaintOptions *paint_options;
  GimpDrawable     *drawable;

  paint_tool = GIMP_PAINT_TOOL (tool);

  core = paint_tool->core;

  paint_options = (GimpPaintOptions *) tool->tool_info->tool_options;

  drawable = gimp_image_active_drawable (gdisp->gimage);

  if (paint_tool->pick_state)
    {
      gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

      paint_tool->pick_state = FALSE;
    }

  /*  Let the specific painting function finish up  */
  gimp_paint_core_paint (core, drawable, paint_options, FINISH_PAINT);

  /*  resume the current selection  */
  gimp_image_selection_control (gdisp->gimage, GIMP_SELECTION_RESUME);

  /*  Set tool state to inactive -- no longer painting */
  tool->state = INACTIVE;

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
  GimpPaintTool    *paint_tool;
  GimpPaintCore    *core;
  GimpPaintOptions *paint_options;
  GimpDrawable     *drawable;

  paint_tool = GIMP_PAINT_TOOL (tool);

  core = paint_tool->core;

  paint_options = (GimpPaintOptions *) tool->tool_info->tool_options;

  drawable = gimp_image_active_drawable (gdisp->gimage);

  if (paint_tool->pick_state)
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (paint_tool));
    }

  core->cur_coords = *coords;

  {
    gint off_x, off_y;
    
    gimp_drawable_offsets (drawable, &off_x, &off_y);

    core->cur_coords.x -= off_x;
    core->cur_coords.y -= off_y;
  }

  if (paint_tool->pick_state)
    {
      gimp_paint_tool_sample_color (drawable,
				    coords->x,
                                    coords->y,
				    state);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (paint_tool));

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

  paint_tool = GIMP_PAINT_TOOL (tool);
  draw_tool  = GIMP_DRAW_TOOL (tool);

  core = paint_tool->core;

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  /*  undraw the old line (if any)  */
  if (draw_tool->gdisp)
    gimp_draw_tool_stop (draw_tool);

  gimp_statusbar_pop (GIMP_STATUSBAR (shell->statusbar),
                      g_type_name (G_TYPE_FROM_INSTANCE (tool)));

  if (tool->gdisp          &&
      tool->gdisp != gdisp &&
      tool->gdisp->gimage == gdisp->gimage)
    {
      /*  if this is a different display, but the same image, HACK around
       *  in tool internals AFTER stopping the current draw_tool, so
       *  straight line drawing works across different views of the
       *  same image.
       */

      tool->gdisp = gdisp;
    }

  if ((layer = gimp_image_get_active_layer (gdisp->gimage)))
    {
      if (gdisp == tool->gdisp && (state & GDK_SHIFT_MASK))
	{
          /*  If shift is down and this is not the first paint stroke,
           *  draw a line
           */

	  gdouble dx, dy, dist;
          gchar   status_str[STATUSBAR_SIZE];

          core->cur_coords = *coords;

          {
            gint off_x, off_y;

            gimp_drawable_offsets (GIMP_DRAWABLE (layer), &off_x, &off_y);

            core->cur_coords.x -= off_x;
            core->cur_coords.y -= off_y;
          }

	  if (state & GDK_CONTROL_MASK)
	    {
              /*  Restrict to multiples of 15 degrees if ctrl is pressed  */

              gimp_paint_core_constrain (core);
	    }

	  dx = core->cur_coords.x - core->last_coords.x;
	  dy = core->cur_coords.y - core->last_coords.y;

	  /*  show distance in statusbar  */
	  if (gdisp->dot_for_dot)
	    {
	      dist = sqrt (SQR (dx) + SQR (dy));

	      g_snprintf (status_str, sizeof (status_str), "%.1f %s",
                          dist, _("pixels"));
	    }
	  else
	    {
	      gchar format_str[64];

              g_snprintf (format_str, sizeof (format_str), "%%.%df %s",
                          gimp_unit_get_digits (gdisp->gimage->unit),
                          gimp_unit_get_symbol (gdisp->gimage->unit));

	      dist = (gimp_unit_get_factor (gdisp->gimage->unit) *
                      sqrt (SQR (dx / gdisp->gimage->xresolution) +
                            SQR (dy / gdisp->gimage->yresolution)));

	      g_snprintf (status_str, sizeof (status_str), format_str,
                          dist);
	    }

	  gimp_statusbar_push (GIMP_STATUSBAR (shell->statusbar),
                               g_type_name (G_TYPE_FROM_INSTANCE (tool)),
                               status_str);

          gimp_draw_tool_start (draw_tool, gdisp);
	}
      else if (paint_tool->pick_colors &&
	       ! (state & GDK_SHIFT_MASK) &&
	       (state & (GDK_CONTROL_MASK | GDK_MOD1_MASK)))
        {
          /* If Ctrl or Mod1 is pressed, pick colors */

          gimp_tool_set_cursor (tool, gdisp,
                                GIMP_COLOR_PICKER_CURSOR,
                                GIMP_COLOR_PICKER_TOOL_CURSOR,
                                GIMP_CURSOR_MODIFIER_NONE);
          return;
	}
    }

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}

static void
gimp_paint_tool_draw (GimpDrawTool *draw_tool)
{
  GimpPaintTool *paint_tool;
  GimpPaintCore *core;

  paint_tool = GIMP_PAINT_TOOL (draw_tool);

  core = paint_tool->core;

  if (paint_tool->pick_state)
    {
      GimpToolInfo               *info;
      GimpColorPickerToolOptions *options;

      info = tool_manager_get_info_by_type (GIMP_TOOL (draw_tool)->tool_info->gimp,
                                            GIMP_TYPE_COLOR_PICKER_TOOL);

      options = (GimpColorPickerToolOptions *) info->tool_options;

      if (options->sample_average)
        {
          gimp_draw_tool_draw_rectangle (draw_tool,
                                         FALSE,
                                         (core->cur_coords.x -
                                          options->average_radius),
                                         (core->cur_coords.y -
                                          options->average_radius),
                                         2 * options->average_radius + 1,
                                         2 * options->average_radius + 1,
                                         TRUE);
        }
    }
  else
    {
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
  GimpToolInfo               *picker_info;
  GimpColorPickerToolOptions *picker_options;
  GimpImage                  *gimage;
  GimpRGB                     color;

  gimage = gimp_item_get_image (GIMP_ITEM (drawable));

  picker_info = tool_manager_get_info_by_type (gimage->gimp,
                                               GIMP_TYPE_COLOR_PICKER_TOOL);

  picker_options = (GimpColorPickerToolOptions *) picker_info->tool_options;

  if (gimp_image_pick_color (gimage,
                             drawable,
                             picker_options->sample_merged,
                             x, y,
                             picker_options->sample_average,
                             picker_options->average_radius,
                             &color,
                             NULL,
                             NULL))
    {
      if ((state & GDK_CONTROL_MASK))
        gimp_context_set_foreground (gimp_get_user_context (gimage->gimp),
                                     &color);
      else
        gimp_context_set_background (gimp_get_user_context (gimage->gimp),
                                     &color);
    }
}
