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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "tools-types.h"

#include "config/gimpdisplayconfig.h"
#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimp-utils.h"
#include "core/gimpdrawable.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
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
#include "display/gimpdisplayshell-utils.h"

#include "gimpcoloroptions.h"
#include "gimppaintoptions-gui.h"
#include "gimppainttool.h"
#include "gimppainttool-paint.h"
#include "gimptoolcontrol.h"
#include "gimptools-utils.h"

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

static void
          gimp_paint_tool_real_paint_prepare (GimpPaintTool         *paint_tool,
                                              GimpDisplay           *display);

static GimpCanvasItem *
              gimp_paint_tool_get_outline    (GimpPaintTool         *paint_tool,
                                              GimpDisplay           *display,
                                              gdouble                x,
                                              gdouble                y);

static gboolean  gimp_paint_tool_check_alpha (GimpPaintTool         *paint_tool,
                                              GimpDrawable          *drawable,
                                              GimpDisplay           *display,
                                              GError               **error);

static void   gimp_paint_tool_hard_notify    (GimpPaintOptions      *options,
                                              const GParamSpec      *pspec,
                                              GimpPaintTool         *paint_tool);
static void   gimp_paint_tool_cursor_notify  (GimpDisplayConfig     *config,
                                              GParamSpec            *pspec,
                                              GimpPaintTool         *paint_tool);


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

  klass->paint_prepare       = gimp_paint_tool_real_paint_prepare;
}

static void
gimp_paint_tool_init (GimpPaintTool *paint_tool)
{
  GimpTool *tool = GIMP_TOOL (paint_tool);

  gimp_tool_control_set_motion_mode    (tool->control, GIMP_MOTION_MODE_EXACT);
  gimp_tool_control_set_scroll_lock    (tool->control, TRUE);
  gimp_tool_control_set_action_opacity (tool->control,
                                        "context/context-opacity-set");

  paint_tool->active        = TRUE;
  paint_tool->pick_colors   = FALSE;
  paint_tool->draw_line     = FALSE;

  paint_tool->show_cursor   = TRUE;
  paint_tool->draw_brush    = TRUE;
  paint_tool->snap_brush    = FALSE;
  paint_tool->draw_fallback = FALSE;
  paint_tool->fallback_size = 0.0;
  paint_tool->draw_circle   = FALSE;
  paint_tool->circle_size   = 0.0;

  paint_tool->status      = _("Click to paint");
  paint_tool->status_line = _("Click to draw the line");
  paint_tool->status_ctrl = _("%s to pick a color");

  paint_tool->core        = NULL;
}

static void
gimp_paint_tool_constructed (GObject *object)
{
  GimpTool          *tool       = GIMP_TOOL (object);
  GimpPaintTool     *paint_tool = GIMP_PAINT_TOOL (object);
  GimpPaintOptions  *options    = GIMP_PAINT_TOOL_GET_OPTIONS (tool);
  GimpDisplayConfig *display_config;
  GimpPaintInfo     *paint_info;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_TOOL_INFO (tool->tool_info));
  gimp_assert (GIMP_IS_PAINT_INFO (tool->tool_info->paint_info));

  display_config = GIMP_DISPLAY_CONFIG (tool->tool_info->gimp->config);

  paint_info = tool->tool_info->paint_info;

  gimp_assert (g_type_is_a (paint_info->paint_type, GIMP_TYPE_PAINT_CORE));

  paint_tool->core = g_object_new (paint_info->paint_type,
                                   "undo-desc", paint_info->blurb,
                                   NULL);

  g_signal_connect_object (options, "notify::hard",
                           G_CALLBACK (gimp_paint_tool_hard_notify),
                           paint_tool, 0);

  gimp_paint_tool_hard_notify (options, NULL, paint_tool);

  paint_tool->show_cursor = display_config->show_paint_tool_cursor;
  paint_tool->draw_brush  = display_config->show_brush_outline;
  paint_tool->snap_brush  = display_config->snap_brush_outline;

  g_signal_connect_object (display_config, "notify::show-paint-tool-cursor",
                           G_CALLBACK (gimp_paint_tool_cursor_notify),
                           paint_tool, 0);
  g_signal_connect_object (display_config, "notify::show-brush-outline",
                           G_CALLBACK (gimp_paint_tool_cursor_notify),
                           paint_tool, 0);
  g_signal_connect_object (display_config, "notify::snap-brush-outline",
                           G_CALLBACK (gimp_paint_tool_cursor_notify),
                           paint_tool, 0);
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

    case GIMP_TOOL_ACTION_COMMIT:
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
  GimpDrawTool     *draw_tool  = GIMP_DRAW_TOOL (tool);
  GimpPaintTool    *paint_tool = GIMP_PAINT_TOOL (tool);
  GimpPaintOptions *options    = GIMP_PAINT_TOOL_GET_OPTIONS (tool);
  GimpGuiConfig    *config     = GIMP_GUI_CONFIG (display->gimp->config);
  GimpDisplayShell *shell      = gimp_display_get_shell (display);
  GimpImage        *image      = gimp_display_get_image (display);
  GimpDrawable     *drawable   = gimp_image_get_active_drawable (image);
  gboolean          constrain;
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
      gimp_tools_blink_lock_box (display->gimp, GIMP_ITEM (drawable));
      return;
    }

  if (! gimp_paint_tool_check_alpha (paint_tool, drawable, display, &error))
    {
      GtkWidget *options_gui;
      GtkWidget *mode_box;

      gimp_tool_message_literal (tool, display, error->message);

      options_gui = gimp_tools_get_tool_options_gui (
                      GIMP_TOOL_OPTIONS (options));
      mode_box    = gimp_paint_options_gui_get_paint_mode_box (options_gui);

      if (gtk_widget_is_sensitive (mode_box))
        gimp_widget_blink (mode_box);

      g_clear_error (&error);

      return;
    }

  if (! gimp_item_is_visible (GIMP_ITEM (drawable)) &&
      ! config->edit_non_visible)
    {
      gimp_tool_message_literal (tool, display,
                                 _("The active layer is not visible."));
      return;
    }

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

  constrain = (state & gimp_get_constrain_behavior_mask ()) != 0;

  if (! gimp_paint_tool_paint_start (paint_tool,
                                     display, coords, time, constrain,
                                     &error))
    {
      gimp_tool_message_literal (tool, display, error->message);
      g_clear_error (&error);
      return;
    }

  tool->display  = display;
  tool->drawable = drawable;

  /*  pause the current selection  */
  gimp_display_shell_selection_pause (shell);

  gimp_draw_tool_start (draw_tool, display);

  gimp_tool_control_activate (tool->control);
}

static void
gimp_paint_tool_button_release (GimpTool              *tool,
                                const GimpCoords      *coords,
                                guint32                time,
                                GdkModifierType        state,
                                GimpButtonReleaseType  release_type,
                                GimpDisplay           *display)
{
  GimpPaintTool    *paint_tool = GIMP_PAINT_TOOL (tool);
  GimpDisplayShell *shell      = gimp_display_get_shell (display);
  GimpImage        *image      = gimp_display_get_image (display);
  gboolean          cancel;

  if (gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (tool)))
    {
      GIMP_TOOL_CLASS (parent_class)->button_release (tool, coords, time,
                                                      state, release_type,
                                                      display);
      return;
    }

  cancel = (release_type == GIMP_BUTTON_RELEASE_CANCEL);

  gimp_paint_tool_paint_end (paint_tool, time, cancel);

  gimp_tool_control_halt (tool->control);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  /*  resume the current selection  */
  gimp_display_shell_selection_resume (shell);

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
  GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (tool);

  GIMP_TOOL_CLASS (parent_class)->motion (tool, coords, time, state, display);

  if (gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (tool)))
    return;

  if (! paint_tool->snap_brush)
    gimp_draw_tool_pause  (GIMP_DRAW_TOOL (tool));

  gimp_paint_tool_paint_motion (paint_tool, coords, time);

  if (! paint_tool->snap_brush)
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

                  switch (GIMP_COLOR_TOOL (tool)->pick_target)
                    {
                    case GIMP_COLOR_PICK_TARGET_FOREGROUND:
                      gimp_tool_push_status (tool, display,
                                             _("Click in any image to pick the "
                                               "foreground color"));
                      break;

                    case GIMP_COLOR_PICK_TARGET_BACKGROUND:
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
  GimpPaintTool      *paint_tool = GIMP_PAINT_TOOL (tool);
  GimpGuiConfig      *config     = GIMP_GUI_CONFIG (display->gimp->config);
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

      if (gimp_viewable_get_children (GIMP_VIEWABLE (drawable))               ||
          gimp_item_is_content_locked (GIMP_ITEM (drawable))                  ||
          ! gimp_paint_tool_check_alpha (paint_tool, drawable, display, NULL) ||
          ! (gimp_item_is_visible (GIMP_ITEM (drawable)) ||
             config->edit_non_visible))
        {
          modifier        = GIMP_CURSOR_MODIFIER_BAD;
          toggle_modifier = GIMP_CURSOR_MODIFIER_BAD;
        }

      if (! paint_tool->show_cursor &&
          modifier != GIMP_CURSOR_MODIFIER_BAD)
        {
          gimp_tool_set_cursor (tool, display,
                                GIMP_CURSOR_NONE,
                                GIMP_TOOL_CURSOR_NONE,
                                GIMP_CURSOR_MODIFIER_NONE);
          return;
        }

      gimp_tool_control_set_cursor_modifier        (tool->control,
                                                    modifier);
      gimp_tool_control_set_toggle_cursor_modifier (tool->control,
                                                    toggle_modifier);
    }

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                 display);

  /*  reset old stuff here so we are not interfering with the modifiers
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
      gchar    *status;
      gboolean  constrain_mask = gimp_get_constrain_behavior_mask ();
      gint      off_x, off_y;

      core->cur_coords = *coords;

      gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

      core->cur_coords.x -= off_x;
      core->cur_coords.y -= off_y;

      if (display == tool->display && (state & GIMP_PAINT_TOOL_LINE_MASK))
        {
          /*  If shift is down and this is not the first paint stroke,
           *  draw a line.
           */
          gchar   *status_help;
          gdouble  offset_angle;
          gdouble  xres, yres;

          gimp_display_shell_get_constrained_line_params (shell,
                                                          &offset_angle,
                                                          &xres, &yres);

          gimp_paint_core_round_line (core, paint_options,
                                      (state & constrain_mask) != 0,
                                      offset_angle, xres, yres);

          status_help = gimp_suggest_modifiers (paint_tool->status_line,
                                                constrain_mask & ~state,
                                                NULL,
                                                _("%s for constrained angles"),
                                                NULL);

          status = gimp_display_shell_get_line_status (shell, status_help,
                                                       ". ",
                                                       core->last_coords.x,
                                                       core->last_coords.y,
                                                       core->cur_coords.x,
                                                       core->cur_coords.y);
          g_free (status_help);
          paint_tool->draw_line = TRUE;
        }
      else
        {
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
            modifiers |= GIMP_PAINT_TOOL_LINE_MASK;

          status = gimp_suggest_modifiers (paint_tool->status,
                                           modifiers & ~state,
                                           _("%s for a straight line"),
                                           paint_tool->status_ctrl,
                                           NULL);
          paint_tool->draw_line = FALSE;
        }
      gimp_tool_push_status (tool, display, "%s", status);
      g_free (status);

      paint_tool->cursor_x = core->cur_coords.x;
      paint_tool->cursor_y = core->cur_coords.y;

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
  GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (draw_tool);


  if (paint_tool->active &&
      ! gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (draw_tool)))
    {
      GimpPaintCore  *core       = paint_tool->core;
      GimpImage      *image      = gimp_display_get_image (draw_tool->display);
      GimpDrawable   *drawable   = gimp_image_get_active_drawable (image);
      GimpCanvasItem *outline    = NULL;
      gboolean        line_drawn = FALSE;
      gdouble         cur_x, cur_y;
      gint            off_x, off_y;

      gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

      if (gimp_paint_tool_paint_is_active (paint_tool) &&
          paint_tool->snap_brush)
        {
          cur_x = paint_tool->paint_x + off_x;
          cur_y = paint_tool->paint_y + off_y;
        }
      else
        {
          cur_x = paint_tool->cursor_x + off_x;
          cur_y = paint_tool->cursor_y + off_y;

          if (paint_tool->draw_line &&
              ! gimp_tool_control_is_active (GIMP_TOOL (draw_tool)->control))
            {
              GimpCanvasGroup *group;
              gdouble          last_x, last_y;

              last_x = core->last_coords.x + off_x;
              last_y = core->last_coords.y + off_y;

              group = gimp_draw_tool_add_stroke_group (draw_tool);
              gimp_draw_tool_push_group (draw_tool, group);

              gimp_draw_tool_add_handle (draw_tool,
                                         GIMP_HANDLE_CIRCLE,
                                         last_x, last_y,
                                         GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                         GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                         GIMP_HANDLE_ANCHOR_CENTER);

              gimp_draw_tool_add_line (draw_tool,
                                       last_x, last_y,
                                       cur_x, cur_y);

              gimp_draw_tool_add_handle (draw_tool,
                                         GIMP_HANDLE_CIRCLE,
                                         cur_x, cur_y,
                                         GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                         GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                         GIMP_HANDLE_ANCHOR_CENTER);

              gimp_draw_tool_pop_group (draw_tool);

              line_drawn = TRUE;
            }
        }

      gimp_paint_tool_set_draw_fallback (paint_tool, FALSE, 0.0);

      if (paint_tool->draw_brush)
        outline = gimp_paint_tool_get_outline (paint_tool,
                                               draw_tool->display,
                                               cur_x, cur_y);

      if (outline)
        {
          gimp_draw_tool_add_item (draw_tool, outline);
          g_object_unref (outline);
        }
      else if (paint_tool->draw_fallback)
        {
          /* Lets make a sensible fallback cursor
           *
           * Sensible cursor is
           *  * crossed to indicate draw point
           *  * reactive to options alterations
           *  * not a full circle that would be in the way
           */
          gint size = (gint) paint_tool->fallback_size;

#define TICKMARK_ANGLE 48
#define ROTATION_ANGLE G_PI / 4

          /* marks for indicating full size */
          gimp_draw_tool_add_arc (draw_tool,
                                  FALSE,
                                  cur_x - (size / 2.0),
                                  cur_y - (size / 2.0),
                                  size, size,
                                  ROTATION_ANGLE - (2.0 * G_PI) / (TICKMARK_ANGLE * 2),
                                  (2.0 * G_PI) / TICKMARK_ANGLE);

          gimp_draw_tool_add_arc (draw_tool,
                                  FALSE,
                                  cur_x - (size / 2.0),
                                  cur_y - (size / 2.0),
                                  size, size,
                                  ROTATION_ANGLE + G_PI / 2 - (2.0 * G_PI) / (TICKMARK_ANGLE * 2),
                                  (2.0 * G_PI) / TICKMARK_ANGLE);

          gimp_draw_tool_add_arc (draw_tool,
                                  FALSE,
                                  cur_x - (size / 2.0),
                                  cur_y - (size / 2.0),
                                  size, size,
                                  ROTATION_ANGLE + G_PI - (2.0 * G_PI) / (TICKMARK_ANGLE * 2),
                                  (2.0 * G_PI) / TICKMARK_ANGLE);

          gimp_draw_tool_add_arc (draw_tool,
                                  FALSE,
                                  cur_x - (size / 2.0),
                                  cur_y - (size / 2.0),
                                  size, size,
                                  ROTATION_ANGLE + 3 * G_PI / 2 - (2.0 * G_PI) / (TICKMARK_ANGLE * 2),
                                  (2.0 * G_PI) / TICKMARK_ANGLE);
        }
      else if (paint_tool->draw_circle)
        {
          gint size = (gint) paint_tool->circle_size;

          /* draw an indicatory circle */
          gimp_draw_tool_add_arc (draw_tool,
                                  FALSE,
                                  cur_x - (size / 2.0),
                                  cur_y - (size / 2.0),
                                  size, size,
                                  0.0, (2.0 * G_PI));
        }

      if (! outline                 &&
          ! line_drawn              &&
          ! paint_tool->show_cursor &&
          ! paint_tool->draw_circle)
        {
          /*  don't leave the user without any indication and draw
           *  a fallback crosshair
           */
          gimp_draw_tool_add_handle (draw_tool,
                                     GIMP_HANDLE_CROSSHAIR,
                                     cur_x, cur_y,
                                     GIMP_TOOL_HANDLE_SIZE_CROSSHAIR,
                                     GIMP_TOOL_HANDLE_SIZE_CROSSHAIR,
                                     GIMP_HANDLE_ANCHOR_CENTER);
        }
    }

  GIMP_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
}

static void
gimp_paint_tool_real_paint_prepare (GimpPaintTool *paint_tool,
                                    GimpDisplay   *display)
{
  GimpDisplayShell *shell = gimp_display_get_shell (display);

  gimp_paint_core_set_show_all (paint_tool->core, shell->show_all);
}

static GimpCanvasItem *
gimp_paint_tool_get_outline (GimpPaintTool *paint_tool,
                             GimpDisplay   *display,
                             gdouble        x,
                             gdouble        y)
{
  if (GIMP_PAINT_TOOL_GET_CLASS (paint_tool)->get_outline)
    return GIMP_PAINT_TOOL_GET_CLASS (paint_tool)->get_outline (paint_tool,
                                                                display, x, y);

  return NULL;
}

static gboolean
gimp_paint_tool_check_alpha (GimpPaintTool  *paint_tool,
                             GimpDrawable   *drawable,
                             GimpDisplay    *display,
                             GError        **error)
{
  GimpPaintToolClass *klass = GIMP_PAINT_TOOL_GET_CLASS (paint_tool);

  if (klass->is_alpha_only && klass->is_alpha_only (paint_tool, drawable))
    {
      if (! gimp_drawable_has_alpha (drawable))
        {
          g_set_error_literal (
            error, GIMP_ERROR, GIMP_FAILED,
            _("The active layer does not have an alpha channel."));

          return FALSE;
        }

        if (GIMP_IS_LAYER (drawable) &&
            gimp_layer_get_lock_alpha (GIMP_LAYER (drawable)))
        {
          g_set_error_literal (
            error, GIMP_ERROR, GIMP_FAILED,
            _("The active layer's alpha channel is locked."));

          if (error)
            gimp_tools_blink_lock_box (display->gimp, GIMP_ITEM (drawable));

          return FALSE;
        }
    }

  return TRUE;
}

static void
gimp_paint_tool_hard_notify (GimpPaintOptions *options,
                             const GParamSpec *pspec,
                             GimpPaintTool    *paint_tool)
{
  if (paint_tool->active)
    {
      GimpTool *tool = GIMP_TOOL (paint_tool);

      gimp_tool_control_set_precision (tool->control,
                                       options->hard ?
                                       GIMP_CURSOR_PRECISION_PIXEL_CENTER :
                                       GIMP_CURSOR_PRECISION_SUBPIXEL);
    }
}

static void
gimp_paint_tool_cursor_notify (GimpDisplayConfig *config,
                               GParamSpec        *pspec,
                               GimpPaintTool     *paint_tool)
{
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (paint_tool));

  paint_tool->show_cursor = config->show_paint_tool_cursor;
  paint_tool->draw_brush  = config->show_brush_outline;
  paint_tool->snap_brush  = config->snap_brush_outline;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (paint_tool));
}

void
gimp_paint_tool_set_active (GimpPaintTool *tool,
                            gboolean       active)
{
  g_return_if_fail (GIMP_IS_PAINT_TOOL (tool));

  if (active != tool->active)
    {
      GimpPaintOptions *options = GIMP_PAINT_TOOL_GET_OPTIONS (tool);

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      tool->active = active;

      if (active)
        gimp_paint_tool_hard_notify (options, NULL, tool);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }
}

/**
 * gimp_paint_tool_enable_color_picker:
 * @tool:   a #GimpPaintTool
 * @target: the #GimpColorPickTarget to set
 *
 * This is a convenience function used from the init method of paint
 * tools that want the color picking functionality. The @mode that is
 * set here is used to decide what cursor modifier to draw and if the
 * picked color goes to the foreground or background color.
 **/
void
gimp_paint_tool_enable_color_picker (GimpPaintTool       *tool,
                                     GimpColorPickTarget  target)
{
  g_return_if_fail (GIMP_IS_PAINT_TOOL (tool));

  tool->pick_colors = TRUE;

  GIMP_COLOR_TOOL (tool)->pick_target = target;
}

void
gimp_paint_tool_set_draw_fallback (GimpPaintTool *tool,
                                   gboolean       draw_fallback,
                                   gint           fallback_size)
{
  g_return_if_fail (GIMP_IS_PAINT_TOOL (tool));

  tool->draw_fallback = draw_fallback;
  tool->fallback_size = fallback_size;
}

void
gimp_paint_tool_set_draw_circle (GimpPaintTool *tool,
                                 gboolean       draw_circle,
                                 gint           circle_size)
{
  g_return_if_fail (GIMP_IS_PAINT_TOOL (tool));

  tool->draw_circle = draw_circle;
  tool->circle_size = circle_size;
}
