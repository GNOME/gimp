/* GIMP - The GNU Image Manipulation Program
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

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimppaintinfo.h"
#include "core/gimpprojection.h"
#include "core/gimptoolinfo.h"
#include "core/gimpunit.h"

#include "paint/gimppaintcore.h"
#include "paint/gimppaintoptions.h"

#include "widgets/gimpdevices.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "gimpcoloroptions.h"
#include "gimppainttool.h"
#include "gimptoolcontrol.h"
#include "tools-utils.h"

#include "gimp-intl.h"


#define HANDLE_SIZE    15
#define STATUSBAR_SIZE 200


static GObject * gimp_paint_tool_constructor (GType                  type,
                                              guint                  n_params,
                                              GObjectConstructParam *params);
static void   gimp_paint_tool_finalize       (GObject               *object);

static void   gimp_paint_tool_control        (GimpTool              *tool,
                                              GimpToolAction         action,
                                              GimpDisplay           *display);
static void   gimp_paint_tool_button_press   (GimpTool              *tool,
                                              GimpCoords            *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              GimpDisplay           *display);
static void   gimp_paint_tool_button_release (GimpTool              *tool,
                                              GimpCoords            *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              GimpButtonReleaseType  release_type,
                                              GimpDisplay           *display);
static void   gimp_paint_tool_motion         (GimpTool              *tool,
                                              GimpCoords            *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              GimpDisplay           *display);
static void   gimp_paint_tool_modifier_key   (GimpTool              *tool,
                                              GdkModifierType        key,
                                              gboolean               press,
                                              GdkModifierType        state,
                                              GimpDisplay           *display);
static void   gimp_paint_tool_oper_update    (GimpTool              *tool,
                                              GimpCoords            *coords,
                                              GdkModifierType        state,
                                              gboolean               proximity,
                                              GimpDisplay           *display);

static void   gimp_paint_tool_draw           (GimpDrawTool          *draw_tool);


G_DEFINE_TYPE (GimpPaintTool, gimp_paint_tool, GIMP_TYPE_COLOR_TOOL)

#define parent_class gimp_paint_tool_parent_class


static void
gimp_paint_tool_class_init (GimpPaintToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  object_class->constructor  = gimp_paint_tool_constructor;
  object_class->finalize     = gimp_paint_tool_finalize;

  tool_class->control        = gimp_paint_tool_control;
  tool_class->button_press   = gimp_paint_tool_button_press;
  tool_class->button_release = gimp_paint_tool_button_release;
  tool_class->motion         = gimp_paint_tool_motion;
  tool_class->modifier_key   = gimp_paint_tool_modifier_key;
  tool_class->oper_update    = gimp_paint_tool_oper_update;

  draw_tool_class->draw      = gimp_paint_tool_draw;
}

static void
gimp_paint_tool_init (GimpPaintTool *paint_tool)
{
  GimpTool *tool = GIMP_TOOL (paint_tool);

  gimp_tool_control_set_motion_mode (tool->control, GIMP_MOTION_MODE_EXACT);
  gimp_tool_control_set_scroll_lock (tool->control, TRUE);
  gimp_tool_control_set_action_value_1 (tool->control,
                                        "context/context-opacity-set");

  paint_tool->pick_colors = FALSE;
  paint_tool->draw_line   = FALSE;

  paint_tool->status      = _("Click to paint");
  paint_tool->status_line = _("Click to draw the line");
  paint_tool->status_ctrl = _("%s to pick a color");

  paint_tool->core        = NULL;
}

static GObject *
gimp_paint_tool_constructor (GType                  type,
                             guint                  n_params,
                             GObjectConstructParam *params)
{
  GObject       *object;
  GimpTool      *tool;
  GimpPaintInfo *paint_info;
  GimpPaintTool *paint_tool;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  tool       = GIMP_TOOL (object);
  paint_tool = GIMP_PAINT_TOOL (object);

  g_assert (GIMP_IS_TOOL_INFO (tool->tool_info));
  g_assert (GIMP_IS_PAINT_INFO (tool->tool_info->paint_info));

  paint_info = tool->tool_info->paint_info;

  g_assert (g_type_is_a (paint_info->paint_type, GIMP_TYPE_PAINT_CORE));

  paint_tool->core = g_object_new (paint_info->paint_type,
                                   "undo-desc", paint_info->blurb,
                                   NULL);

  return object;
}

static void
gimp_paint_tool_finalize (GObject *object)
{
  GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (object);

  if (paint_tool->core)
    {
      g_object_unref (paint_tool->core);
      paint_tool->core = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * gimp_paint_tool_enable_color_picker:
 * @tool: a #GimpPaintTool
 * @mode: the #GimpColorPickMode to set
 *
 * This is a convenience function used from the init method of paint
 * tools that want the color picking functionality. The @mode that is
 * set here is used to decide what cursor modifier to draw and if the
 * picked color goes to the foreground or background color.
 **/
void
gimp_paint_tool_enable_color_picker (GimpPaintTool     *tool,
                                     GimpColorPickMode  mode)
{
  g_return_if_fail (GIMP_IS_PAINT_TOOL (tool));

  tool->pick_colors = TRUE;

  GIMP_COLOR_TOOL (tool)->pick_mode = mode;
}

static void
gimp_paint_tool_control (GimpTool       *tool,
                         GimpToolAction  action,
                         GimpDisplay    *display)
{
  GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (tool);
  GimpDrawable  *drawable   = gimp_image_get_active_drawable (display->image);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_paint_core_paint (paint_tool->core,
                             drawable,
                             GIMP_PAINT_TOOL_GET_OPTIONS (tool),
                             GIMP_PAINT_STATE_FINISH, 0);
      gimp_paint_core_cleanup (paint_tool->core);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

/**
 * gimp_paint_tool_round_line:
 * @core:          the #GimpPaintCore
 * @center_pixels: push coordinates to pixel centers?
 * @state:         the modifier state
 *
 * Adjusts core->last_coords and core_cur_coords in preparation to
 * drawing a straight line. If @center_pixels is TRUE the endpoints
 * get pushed to the center of the pixels. This avoids artefacts
 * for e.g. the hard mode. The rounding of the slope to 15 degree
 * steps if ctrl is pressed happens, as does rounding the start and
 * end coordinates (which may be fractional in high zoom modes) to
 * the center of pixels.
 **/
static void
gimp_paint_tool_round_line (GimpPaintCore   *core,
                            gboolean         center_pixels,
                            GdkModifierType  state)
{
  if (center_pixels)
    {
      core->last_coords.x = floor (core->last_coords.x) + 0.5;
      core->last_coords.y = floor (core->last_coords.y) + 0.5;
      core->cur_coords.x  = floor (core->cur_coords.x ) + 0.5;
      core->cur_coords.y  = floor (core->cur_coords.y ) + 0.5;
    }

  /* Restrict to multiples of 15 degrees if ctrl is pressed */
  if (state & GDK_CONTROL_MASK)
    gimp_tool_motion_constrain (core->last_coords.x, core->last_coords.y,
                                &core->cur_coords.x, &core->cur_coords.y);
}

static void
gimp_paint_tool_button_press (GimpTool        *tool,
                              GimpCoords      *coords,
                              guint32          time,
                              GdkModifierType  state,
                              GimpDisplay     *display)
{
  GimpDrawTool     *draw_tool     = GIMP_DRAW_TOOL (tool);
  GimpPaintTool    *paint_tool    = GIMP_PAINT_TOOL (tool);
  GimpPaintOptions *paint_options = GIMP_PAINT_TOOL_GET_OPTIONS (tool);
  GimpPaintCore    *core          = paint_tool->core;
  GimpDrawable     *drawable;
  GdkDisplay       *gdk_display;
  GimpCoords        curr_coords;
  gint              off_x, off_y;
  GError           *error = NULL;

  if (gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (tool)))
    {
      GIMP_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                    display);
      return;
    }

  drawable = gimp_image_get_active_drawable (display->image);

  curr_coords = *coords;

  gimp_item_offsets (GIMP_ITEM (drawable), &off_x, &off_y);

  curr_coords.x -= off_x;
  curr_coords.y -= off_y;

  if (gimp_draw_tool_is_active (draw_tool))
    gimp_draw_tool_stop (draw_tool);

  if (tool->display          &&
      tool->display != display &&
      tool->display->image == display->image)
    {
      /*  if this is a different display, but the same image, HACK around
       *  in tool internals AFTER stopping the current draw_tool, so
       *  straight line drawing works across different views of the
       *  same image.
       */

      tool->display = display;
    }

  gdk_display = gtk_widget_get_display (display->shell);

  core->use_pressure = (gimp_devices_get_current (display->image->gimp) !=
                        gdk_display_get_core_pointer (gdk_display));

  if (! gimp_paint_core_start (core, drawable, paint_options, &curr_coords,
                               &error))
    {
      gimp_tool_message (tool, display, error->message);
      g_clear_error (&error);
      return;
    }

  if ((display != tool->display) || ! paint_tool->draw_line)
    {
      /*  if this is a new image, reinit the core vals  */

      core->start_coords = core->cur_coords;
      core->last_coords  = core->cur_coords;

      core->distance     = 0.0;
      core->pixel_dist   = 0.0;
    }
  else if (paint_tool->draw_line)
    {
      /*  If shift is down and this is not the first paint
       *  stroke, then draw a line from the last coords to the pointer
       */
      gboolean hard;

      core->start_coords = core->last_coords;

      hard = (gimp_paint_options_get_brush_mode (paint_options) ==
              GIMP_BRUSH_HARD);
      gimp_paint_tool_round_line (core, hard, state);
    }

  /*  chain up to activate the tool  */
  GIMP_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                display);

  /*  pause the current selection  */
  gimp_image_selection_control (display->image, GIMP_SELECTION_PAUSE);

  /*  Let the specific painting function initialize itself  */
  gimp_paint_core_paint (core, drawable, paint_options,
                         GIMP_PAINT_STATE_INIT, time);

  /*  Paint to the image  */
  if (paint_tool->draw_line)
    {
      gimp_paint_core_interpolate (core, drawable, paint_options, time);
    }
  else
    {
      gimp_paint_core_paint (core, drawable, paint_options,
                             GIMP_PAINT_STATE_MOTION, time);
    }

  gimp_projection_flush_now (display->image->projection);
  gimp_display_flush_now (display);

  gimp_draw_tool_start (draw_tool, display);
}

static void
gimp_paint_tool_button_release (GimpTool              *tool,
                                GimpCoords            *coords,
                                guint32                time,
                                GdkModifierType        state,
                                GimpButtonReleaseType  release_type,
                                GimpDisplay           *display)
{
  GimpPaintTool    *paint_tool    = GIMP_PAINT_TOOL (tool);
  GimpPaintOptions *paint_options = GIMP_PAINT_TOOL_GET_OPTIONS (tool);
  GimpPaintCore    *core          = paint_tool->core;
  GimpDrawable     *drawable;

  if (gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (tool)))
    {
      GIMP_TOOL_CLASS (parent_class)->button_release (tool, coords, time,
                                                      state, release_type,
                                                      display);
      return;
    }

  drawable = gimp_image_get_active_drawable (display->image);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  /*  Let the specific painting function finish up  */
  gimp_paint_core_paint (core, drawable, paint_options,
                         GIMP_PAINT_STATE_FINISH, time);

  /*  resume the current selection  */
  gimp_image_selection_control (display->image, GIMP_SELECTION_RESUME);

  /*  chain up to halt the tool */
  GIMP_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                  release_type, display);

  if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
    gimp_paint_core_cancel (core, drawable);
  else
    gimp_paint_core_finish (core, drawable);

  gimp_image_flush (display->image);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_paint_tool_motion (GimpTool        *tool,
                        GimpCoords      *coords,
                        guint32          time,
                        GdkModifierType  state,
                        GimpDisplay     *display)
{
  GimpPaintTool    *paint_tool    = GIMP_PAINT_TOOL (tool);
  GimpPaintOptions *paint_options = GIMP_PAINT_TOOL_GET_OPTIONS (tool);
  GimpPaintCore    *core          = paint_tool->core;
  GimpDrawable     *drawable;
  gint              off_x, off_y;

  if (gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (tool)))
    {
      GIMP_TOOL_CLASS (parent_class)->motion (tool, coords, time, state,
                                              display);
      return;
    }

  drawable = gimp_image_get_active_drawable (display->image);

  core->cur_coords = *coords;

  gimp_item_offsets (GIMP_ITEM (drawable), &off_x, &off_y);

  core->cur_coords.x -= off_x;
  core->cur_coords.y -= off_y;

  GIMP_TOOL_CLASS (parent_class)->motion (tool, coords, time, state,
                                          display);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  gimp_paint_core_interpolate (core, drawable, paint_options, time);

  gimp_projection_flush_now (display->image->projection);
  gimp_display_flush_now (display);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_paint_tool_modifier_key (GimpTool        *tool,
                              GdkModifierType  key,
                              gboolean         press,
                              GdkModifierType  state,
                              GimpDisplay     *display)
{
  GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (tool);
  GimpDrawTool  *draw_tool  = GIMP_DRAW_TOOL (tool);

  if (key != GDK_CONTROL_MASK)
    return;

  if (paint_tool->pick_colors && ! paint_tool->draw_line)
    {
      if (press)
        {
          GimpToolInfo *info = gimp_get_tool_info (display->image->gimp,
                                                   "gimp-color-picker-tool");

          if (GIMP_IS_TOOL_INFO (info))
            {
              if (gimp_draw_tool_is_active (draw_tool))
                gimp_draw_tool_stop (draw_tool);

              gimp_color_tool_enable (GIMP_COLOR_TOOL (tool),
                                      GIMP_COLOR_OPTIONS (info->tool_options));

              switch (GIMP_COLOR_TOOL (tool)->pick_mode)
                {
                case GIMP_COLOR_PICK_MODE_FOREGROUND:
                  gimp_tool_push_status (tool, display,
                                         _("Click in any image to pick the "
                                           "foreground color"));
                  break;

                case GIMP_COLOR_PICK_MODE_BACKGROUND:
                  gimp_tool_push_status (tool, display,
                                         _("Click in any image to pick the "
                                           "background color"));
                  break;

                default:
                  break;
                }
            }
        }
      else
        {
          if (gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (tool)))
            {
              gimp_tool_pop_status (tool, display);
              gimp_color_tool_disable (GIMP_COLOR_TOOL (tool));
            }
        }
    }
}

static void
gimp_paint_tool_oper_update (GimpTool        *tool,
                             GimpCoords      *coords,
                             GdkModifierType  state,
                             gboolean         proximity,
                             GimpDisplay     *display)
{
  GimpPaintTool    *paint_tool    = GIMP_PAINT_TOOL (tool);
  GimpDrawTool     *draw_tool     = GIMP_DRAW_TOOL (tool);
  GimpPaintOptions *paint_options = GIMP_PAINT_TOOL_GET_OPTIONS (tool);
  GimpPaintCore    *core          = paint_tool->core;
  GimpDisplayShell *shell         = GIMP_DISPLAY_SHELL (display->shell);
  GimpDrawable     *drawable;

  if (gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (tool)))
    {
      GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state,
                                                   proximity, display);
      return;
    }

  if (gimp_draw_tool_is_active (draw_tool))
    gimp_draw_tool_stop (draw_tool);

  gimp_tool_pop_status (tool, display);

  if (tool->display          &&
      tool->display != display &&
      tool->display->image == display->image)
    {
      /*  if this is a different display, but the same image, HACK around
       *  in tool internals AFTER stopping the current draw_tool, so
       *  straight line drawing works across different views of the
       *  same image.
       */

      tool->display = display;
    }

  drawable = gimp_image_get_active_drawable (display->image);

  if (drawable && proximity)
    {
      if (display == tool->display && (state & GDK_SHIFT_MASK))
        {
          /*  If shift is down and this is not the first paint stroke,
           *  draw a line.
           */

          gchar    *status_help;
          gdouble   dx, dy, dist;
          gint      off_x, off_y;
          gboolean  hard;

          core->cur_coords = *coords;

          gimp_item_offsets (GIMP_ITEM (drawable), &off_x, &off_y);

          core->cur_coords.x -= off_x;
          core->cur_coords.y -= off_y;

          hard = (gimp_paint_options_get_brush_mode (paint_options) ==
                  GIMP_BRUSH_HARD);
          gimp_paint_tool_round_line (core, hard, state);

          dx = core->cur_coords.x - core->last_coords.x;
          dy = core->cur_coords.y - core->last_coords.y;

          status_help = gimp_suggest_modifiers (paint_tool->status_line,
                                                GDK_CONTROL_MASK & ~state,
                                                NULL,
                                                _("%s for constrained angles"),
                                                NULL);

          /*  show distance in statusbar  */
          if (shell->unit == GIMP_UNIT_PIXEL)
            {
              dist = sqrt (SQR (dx) + SQR (dy));

              gimp_tool_push_status (tool, display, "%.1f %s.  %s",
                                     dist, _("pixels"), status_help);
            }
          else
            {
              GimpImage *image = display->image;
              gchar      format_str[64];

              g_snprintf (format_str, sizeof (format_str), "%%.%df %s.  %%s",
                          _gimp_unit_get_digits (image->gimp, shell->unit),
                          _gimp_unit_get_symbol (image->gimp, shell->unit));

              dist = (_gimp_unit_get_factor (image->gimp, shell->unit) *
                      sqrt (SQR (dx / image->xresolution) +
                            SQR (dy / image->yresolution)));

              gimp_tool_push_status (tool, display, format_str,
                                     dist, status_help);
            }

          g_free (status_help);

          paint_tool->draw_line = TRUE;
        }
      else
        {
          gchar           *status;
          GdkModifierType  modifiers = 0;

          /* HACK: A paint tool may set status_ctrl to NULL to indicate that
           * it ignores the Ctrl modifier (temporarily or permanently), so
           * it should not be suggested.  This is different from how
           * gimp_suggest_modifiers() would interpret this parameter.
           */
          if (paint_tool->status_ctrl != NULL)
            modifiers |= GDK_CONTROL_MASK;

          /* suggest drawing lines only after the first point is set
           */
          if (display == tool->display)
            modifiers |= GDK_SHIFT_MASK;

          status = gimp_suggest_modifiers (paint_tool->status,
                                           modifiers & ~state,
                                           _("%s for a straight line"),
                                           paint_tool->status_ctrl,
                                           NULL);
          gimp_tool_push_status (tool, display, status);
          g_free (status);

          paint_tool->draw_line = FALSE;
        }

      gimp_draw_tool_start (draw_tool, display);
    }

  GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);
}

static void
gimp_paint_tool_draw (GimpDrawTool *draw_tool)
{
  if (! gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (draw_tool)))
    {
      GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (draw_tool);
      GimpPaintCore *core       = paint_tool->core;

      if (paint_tool->draw_line &&
          ! gimp_tool_control_is_active (GIMP_TOOL (draw_tool)->control))
        {
          /*  Draw start target  */
          gimp_draw_tool_draw_handle (draw_tool,
                                      GIMP_HANDLE_CROSS,
                                      core->last_coords.x,
                                      core->last_coords.y,
                                      HANDLE_SIZE,
                                      HANDLE_SIZE,
                                      GTK_ANCHOR_CENTER,
                                      TRUE);

          /*  Draw end target  */
          gimp_draw_tool_draw_handle (draw_tool,
                                      GIMP_HANDLE_CROSS,
                                      core->cur_coords.x,
                                      core->cur_coords.y,
                                      HANDLE_SIZE,
                                      HANDLE_SIZE,
                                      GTK_ANCHOR_CENTER,
                                      TRUE);

          /*  Draw the line between the start and end coords  */
          gimp_draw_tool_draw_line (draw_tool,
                                    core->last_coords.x,
                                    core->last_coords.y,
                                    core->cur_coords.x,
                                    core->cur_coords.y,
                                    TRUE);
        }
    }

  GIMP_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
}
