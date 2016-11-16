/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimp-utils.h"
#include "core/gimpdrawable.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"
#include "core/gimppaintinfo.h"
#include "core/gimpprojection.h"
#include "core/gimptoolinfo.h"

#include "paint/gimppaintcore.h"
#include "paint/gimppaintoptions.h"

#include "widgets/gimpdevices.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-selection.h"

#include "gimpcoloroptions.h"
#include "gimppainttool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static void   gimp_paint_tool_constructed    (GObject               *object);
static void   gimp_paint_tool_finalize       (GObject               *object);

static void   gimp_paint_tool_control        (GimpTool              *tool,
                                              GimpToolAction         action,
                                              GimpDisplay           *display);
static void   gimp_paint_tool_button_press   (GimpTool              *tool,
                                              const GimpCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              GimpButtonPressType    press_type,
                                              GimpDisplay           *display);
static void   gimp_paint_tool_button_release (GimpTool              *tool,
                                              const GimpCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              GimpButtonReleaseType  release_type,
                                              GimpDisplay           *display);
static void   gimp_paint_tool_motion         (GimpTool              *tool,
                                              const GimpCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              GimpDisplay           *display);
static void   gimp_paint_tool_modifier_key   (GimpTool              *tool,
                                              GdkModifierType        key,
                                              gboolean               press,
                                              GdkModifierType        state,
                                              GimpDisplay           *display);
static void   gimp_paint_tool_cursor_update  (GimpTool              *tool,
                                              const GimpCoords      *coords,
                                              GdkModifierType        state,
                                              GimpDisplay           *display);
static void   gimp_paint_tool_oper_update    (GimpTool              *tool,
                                              const GimpCoords      *coords,
                                              GdkModifierType        state,
                                              gboolean               proximity,
                                              GimpDisplay           *display);

static void   gimp_paint_tool_draw           (GimpDrawTool          *draw_tool);

static void   gimp_paint_tool_hard_notify    (GimpPaintOptions      *options,
                                              const GParamSpec      *pspec,
                                              GimpTool              *tool);


G_DEFINE_TYPE (GimpPaintTool, gimp_paint_tool, GIMP_TYPE_COLOR_TOOL)

#define parent_class gimp_paint_tool_parent_class


static void
gimp_paint_tool_class_init (GimpPaintToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  object_class->constructed  = gimp_paint_tool_constructed;
  object_class->finalize     = gimp_paint_tool_finalize;

  tool_class->control        = gimp_paint_tool_control;
  tool_class->button_press   = gimp_paint_tool_button_press;
  tool_class->button_release = gimp_paint_tool_button_release;
  tool_class->motion         = gimp_paint_tool_motion;
  tool_class->modifier_key   = gimp_paint_tool_modifier_key;
  tool_class->cursor_update  = gimp_paint_tool_cursor_update;
  tool_class->oper_update    = gimp_paint_tool_oper_update;

  draw_tool_class->draw      = gimp_paint_tool_draw;
}

static void
gimp_paint_tool_init (GimpPaintTool *paint_tool)
{
  GimpTool *tool = GIMP_TOOL (paint_tool);

  gimp_tool_control_set_motion_mode    (tool->control, GIMP_MOTION_MODE_EXACT);
  gimp_tool_control_set_scroll_lock    (tool->control, TRUE);
  gimp_tool_control_set_action_value_1 (tool->control,
                                        "context/context-opacity-set");

  paint_tool->pick_colors = FALSE;
  paint_tool->draw_line   = FALSE;

  paint_tool->status      = _("Click to paint");
  paint_tool->status_line = _("Click to draw the line");
  paint_tool->status_ctrl = _("%s to pick a color");

  paint_tool->core        = NULL;
}

static void
gimp_paint_tool_constructed (GObject *object)
{
  GimpTool         *tool       = GIMP_TOOL (object);
  GimpPaintTool    *paint_tool = GIMP_PAINT_TOOL (object);
  GimpPaintOptions *options    = GIMP_PAINT_TOOL_GET_OPTIONS (tool);
  GimpPaintInfo    *paint_info;

  if (G_OBJECT_CLASS (parent_class)->constructed)
    G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (GIMP_IS_TOOL_INFO (tool->tool_info));
  g_assert (GIMP_IS_PAINT_INFO (tool->tool_info->paint_info));

  paint_info = tool->tool_info->paint_info;

  g_assert (g_type_is_a (paint_info->paint_type, GIMP_TYPE_PAINT_CORE));

  paint_tool->core = g_object_new (paint_info->paint_type,
                                   "undo-desc", paint_info->blurb,
                                   NULL);

  g_signal_connect_object (options, "notify::hard",
                           G_CALLBACK (gimp_paint_tool_hard_notify),
                           tool, 0);

  gimp_paint_tool_hard_notify (options, NULL, tool);
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

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_paint_core_cleanup (paint_tool->core);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_paint_tool_button_press (GimpTool            *tool,
                              const GimpCoords    *coords,
                              guint32              time,
                              GdkModifierType      state,
                              GimpButtonPressType  press_type,
                              GimpDisplay         *display)
{
  GimpDrawTool     *draw_tool     = GIMP_DRAW_TOOL (tool);
  GimpPaintTool    *paint_tool    = GIMP_PAINT_TOOL (tool);
  GimpPaintOptions *paint_options = GIMP_PAINT_TOOL_GET_OPTIONS (tool);
  GimpPaintCore    *core          = paint_tool->core;
  GimpDisplayShell *shell         = gimp_display_get_shell (display);
  GimpImage        *image         = gimp_display_get_image (display);
  GimpDrawable     *drawable      = gimp_image_get_active_drawable (image);
  GimpCoords        curr_coords;
  gint              off_x, off_y;
  GError           *error = NULL;

  if (gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (tool)))
    {
      GIMP_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                    press_type, display);
      return;
    }

  if (gimp_viewable_get_children (GIMP_VIEWABLE (drawable)))
    {
      gimp_tool_message_literal (tool, display,
                                 _("Cannot paint on layer groups."));
      return;
    }

  if (gimp_item_is_content_locked (GIMP_ITEM (drawable)))
    {
      gimp_tool_message_literal (tool, display,
                                 _("The active layer's pixels are locked."));
      return;
    }

  curr_coords = *coords;

  gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

  curr_coords.x -= off_x;
  curr_coords.y -= off_y;

  if (gimp_draw_tool_is_active (draw_tool))
    gimp_draw_tool_stop (draw_tool);

  if (tool->display            &&
      tool->display != display &&
      gimp_display_get_image (tool->display) == image)
    {
      /*  if this is a different display, but the same image, HACK around
       *  in tool internals AFTER stopping the current draw_tool, so
       *  straight line drawing works across different views of the
       *  same image.
       */

      tool->display = display;
    }

  if (! gimp_paint_core_start (core, drawable, paint_options, &curr_coords,
                               &error))
    {
      gimp_tool_message_literal (tool, display, error->message);
      g_clear_error (&error);
      return;
    }

  if ((display != tool->display) || ! paint_tool->draw_line)
    {
      /* if this is a new display, resest the "last stroke's endpoint"
       * because there is none
       */
      if (display != tool->display)
        core->start_coords = core->cur_coords;

      core->last_coords = core->cur_coords;

      core->distance    = 0.0;
      core->pixel_dist  = 0.0;
    }
  else if (paint_tool->draw_line)
    {
      gboolean constrain = (state & gimp_get_constrain_behavior_mask ()) != 0;

      /*  If shift is down and this is not the first paint
       *  stroke, then draw a line from the last coords to the pointer
       */
      gimp_paint_core_round_line (core, paint_options, constrain);
    }

  /*  chain up to activate the tool  */
  GIMP_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                press_type, display);

  /*  pause the current selection  */
  gimp_display_shell_selection_pause (shell);

  /*  Let the specific painting function initialize itself  */
  gimp_paint_core_paint (core, drawable, paint_options,
                         GIMP_PAINT_STATE_INIT, time);

  /*  Paint to the image  */
  if (paint_tool->draw_line)
    {
      gimp_paint_core_interpolate (core, drawable, paint_options,
                                   &core->cur_coords, time);
    }
  else
    {
      gimp_paint_core_paint (core, drawable, paint_options,
                             GIMP_PAINT_STATE_MOTION, time);
    }

  gimp_projection_flush_now (gimp_image_get_projection (image));
  gimp_display_flush_now (display);

  gimp_draw_tool_start (draw_tool, display);
}

static void
gimp_paint_tool_button_release (GimpTool              *tool,
                                const GimpCoords      *coords,
                                guint32                time,
                                GdkModifierType        state,
                                GimpButtonReleaseType  release_type,
                                GimpDisplay           *display)
{
  GimpPaintTool    *paint_tool    = GIMP_PAINT_TOOL (tool);
  GimpPaintOptions *paint_options = GIMP_PAINT_TOOL_GET_OPTIONS (tool);
  GimpPaintCore    *core          = paint_tool->core;
  GimpDisplayShell *shell         = gimp_display_get_shell (display);
  GimpImage        *image         = gimp_display_get_image (display);
  GimpDrawable     *drawable      = gimp_image_get_active_drawable (image);

  if (gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (tool)))
    {
      GIMP_TOOL_CLASS (parent_class)->button_release (tool, coords, time,
                                                      state, release_type,
                                                      display);
      return;
    }

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  /*  Let the specific painting function finish up  */
  gimp_paint_core_paint (core, drawable, paint_options,
                         GIMP_PAINT_STATE_FINISH, time);

  /*  resume the current selection  */
  gimp_display_shell_selection_resume (shell);

  /*  chain up to halt the tool */
  GIMP_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                  release_type, display);

  if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
    gimp_paint_core_cancel (core, drawable);
  else
    gimp_paint_core_finish (core, drawable, TRUE);

  gimp_image_flush (image);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_paint_tool_motion (GimpTool         *tool,
                        const GimpCoords *coords,
                        guint32           time,
                        GdkModifierType   state,
                        GimpDisplay      *display)
{
  GimpPaintTool    *paint_tool    = GIMP_PAINT_TOOL (tool);
  GimpPaintOptions *paint_options = GIMP_PAINT_TOOL_GET_OPTIONS (tool);
  GimpPaintCore    *core          = paint_tool->core;
  GimpImage        *image         = gimp_display_get_image (display);
  GimpDrawable     *drawable      = gimp_image_get_active_drawable (image);
  GimpCoords        curr_coords;
  gint              off_x, off_y;

  GIMP_TOOL_CLASS (parent_class)->motion (tool, coords, time, state, display);

  if (gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (tool)))
    return;

  curr_coords = *coords;

  gimp_paint_core_smooth_coords (core, paint_options, &curr_coords);

  gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

  curr_coords.x -= off_x;
  curr_coords.y -= off_y;

  /*  don't paint while the Shift key is pressed for line drawing  */
  if (paint_tool->draw_line)
    {
      gimp_paint_core_set_current_coords (core, &curr_coords);
      return;
    }

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  gimp_paint_core_interpolate (core, drawable, paint_options,
                               &curr_coords, time);

  gimp_projection_flush_now (gimp_image_get_projection (image));
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

  if (paint_tool->pick_colors && ! paint_tool->draw_line)
    {
      if ((state & gimp_get_all_modifiers_mask ()) ==
          gimp_get_constrain_behavior_mask ())
        {
          if (! gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (tool)))
            {
              GimpToolInfo *info = gimp_get_tool_info (display->gimp,
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
gimp_paint_tool_cursor_update (GimpTool         *tool,
                               const GimpCoords *coords,
                               GdkModifierType   state,
                               GimpDisplay      *display)
{
  GimpCursorModifier  modifier;
  GimpCursorModifier  toggle_modifier;
  GimpCursorModifier  old_modifier;
  GimpCursorModifier  old_toggle_modifier;

  modifier        = tool->control->cursor_modifier;
  toggle_modifier = tool->control->toggle_cursor_modifier;

  old_modifier        = modifier;
  old_toggle_modifier = toggle_modifier;

  if (! gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (tool)))
    {
      GimpImage    *image    = gimp_display_get_image (display);
      GimpDrawable *drawable = gimp_image_get_active_drawable (image);

      if (gimp_viewable_get_children (GIMP_VIEWABLE (drawable)) ||
          gimp_item_is_content_locked (GIMP_ITEM (drawable)))
        {
          modifier        = GIMP_CURSOR_MODIFIER_BAD;
          toggle_modifier = GIMP_CURSOR_MODIFIER_BAD;
        }

      gimp_tool_control_set_cursor_modifier        (tool->control,
                                                    modifier);
      gimp_tool_control_set_toggle_cursor_modifier (tool->control,
                                                    toggle_modifier);
    }

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                 display);

  /*  reset old stuff here so we are not interferring with the modifiers
   *  set by our subclasses
   */
  gimp_tool_control_set_cursor_modifier        (tool->control,
                                                old_modifier);
  gimp_tool_control_set_toggle_cursor_modifier (tool->control,
                                                old_toggle_modifier);
}

static void
gimp_paint_tool_oper_update (GimpTool         *tool,
                             const GimpCoords *coords,
                             GdkModifierType   state,
                             gboolean          proximity,
                             GimpDisplay      *display)
{
  GimpPaintTool    *paint_tool    = GIMP_PAINT_TOOL (tool);
  GimpDrawTool     *draw_tool     = GIMP_DRAW_TOOL (tool);
  GimpPaintOptions *paint_options = GIMP_PAINT_TOOL_GET_OPTIONS (tool);
  GimpPaintCore    *core          = paint_tool->core;
  GimpDisplayShell *shell         = gimp_display_get_shell (display);
  GimpImage        *image         = gimp_display_get_image (display);
  GimpDrawable     *drawable      = gimp_image_get_active_drawable (image);

  if (gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (tool)))
    {
      GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state,
                                                   proximity, display);
      return;
    }

  gimp_draw_tool_pause (draw_tool);

  if (gimp_draw_tool_is_active (draw_tool) &&
      draw_tool->display != display)
    gimp_draw_tool_stop (draw_tool);

  gimp_tool_pop_status (tool, display);

  if (tool->display            &&
      tool->display != display &&
      gimp_display_get_image (tool->display) == image)
    {
      /*  if this is a different display, but the same image, HACK around
       *  in tool internals AFTER stopping the current draw_tool, so
       *  straight line drawing works across different views of the
       *  same image.
       */

      tool->display = display;
    }

  if (drawable && proximity)
    {
      gboolean constrain_mask = gimp_get_constrain_behavior_mask ();

      if (display == tool->display && (state & GDK_SHIFT_MASK))
        {
          /*  If shift is down and this is not the first paint stroke,
           *  draw a line.
           */

          gchar   *status_help;
          gdouble  dx, dy, dist;
          gint     off_x, off_y;

          core->cur_coords = *coords;

          gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

          core->cur_coords.x -= off_x;
          core->cur_coords.y -= off_y;

          gimp_paint_core_round_line (core, paint_options,
                                      (state & constrain_mask) != 0);

          dx = core->cur_coords.x - core->last_coords.x;
          dy = core->cur_coords.y - core->last_coords.y;

          status_help = gimp_suggest_modifiers (paint_tool->status_line,
                                                constrain_mask & ~state,
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
              gdouble xres;
              gdouble yres;
              gchar   format_str[64];

              gimp_image_get_resolution (image, &xres, &yres);

              g_snprintf (format_str, sizeof (format_str), "%%.%df %s.  %%s",
                          gimp_unit_get_digits (shell->unit),
                          gimp_unit_get_symbol (shell->unit));

              dist = (gimp_unit_get_factor (shell->unit) *
                      sqrt (SQR (dx / xres) +
                            SQR (dy / yres)));

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
            modifiers |= constrain_mask;

          /* suggest drawing lines only after the first point is set
           */
          if (display == tool->display)
            modifiers |= GDK_SHIFT_MASK;

          status = gimp_suggest_modifiers (paint_tool->status,
                                           modifiers & ~state,
                                           _("%s for a straight line"),
                                           paint_tool->status_ctrl,
                                           NULL);
          gimp_tool_push_status (tool, display, "%s", status);
          g_free (status);

          paint_tool->draw_line = FALSE;
        }

      if (! gimp_draw_tool_is_active (draw_tool))
        gimp_draw_tool_start (draw_tool, display);
    }
  else if (gimp_draw_tool_is_active (draw_tool))
    {
      gimp_draw_tool_stop (draw_tool);
    }

  GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);

  gimp_draw_tool_resume (draw_tool);
}

static void
gimp_paint_tool_draw (GimpDrawTool *draw_tool)
{
  if (! gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (draw_tool)))
    {
      GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (draw_tool);

      if (paint_tool->draw_line &&
          ! gimp_tool_control_is_active (GIMP_TOOL (draw_tool)->control))
        {
          GimpPaintCore *core       = paint_tool->core;
          GimpImage     *image      = gimp_display_get_image (draw_tool->display);
          GimpDrawable  *drawable   = gimp_image_get_active_drawable (image);
          gint           off_x, off_y;

          gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

          /*  Draw the line between the start and end coords  */
          gimp_draw_tool_add_line (draw_tool,
                                   core->last_coords.x + off_x,
                                   core->last_coords.y + off_y,
                                   core->cur_coords.x + off_x,
                                   core->cur_coords.y + off_y);

          /*  Draw start target  */
          gimp_draw_tool_add_handle (draw_tool,
                                     GIMP_HANDLE_CROSS,
                                     core->last_coords.x + off_x,
                                     core->last_coords.y + off_y,
                                     GIMP_TOOL_HANDLE_SIZE_CROSS,
                                     GIMP_TOOL_HANDLE_SIZE_CROSS,
                                     GIMP_HANDLE_ANCHOR_CENTER);

          /*  Draw end target  */
          gimp_draw_tool_add_handle (draw_tool,
                                     GIMP_HANDLE_CROSS,
                                     core->cur_coords.x + off_x,
                                     core->cur_coords.y + off_y,
                                     GIMP_TOOL_HANDLE_SIZE_CROSS,
                                     GIMP_TOOL_HANDLE_SIZE_CROSS,
                                     GIMP_HANDLE_ANCHOR_CENTER);
        }
    }

  GIMP_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
}

static void
gimp_paint_tool_hard_notify (GimpPaintOptions *options,
                             const GParamSpec *pspec,
                             GimpTool         *tool)
{
  gimp_tool_control_set_precision (tool->control,
                                   options->hard ?
                                   GIMP_CURSOR_PRECISION_PIXEL_CENTER :
                                   GIMP_CURSOR_PRECISION_SUBPIXEL);
}
