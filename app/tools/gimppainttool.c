/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"

#include "tools-types.h"

#include "config/ligmadisplayconfig.h"
#include "config/ligmaguiconfig.h"

#include "core/ligma.h"
#include "core/ligma-utils.h"
#include "core/ligmadrawable.h"
#include "core/ligmaerror.h"
#include "core/ligmaimage.h"
#include "core/ligmalayer.h"
#include "core/ligmapaintinfo.h"
#include "core/ligmaprojection.h"
#include "core/ligmatoolinfo.h"

#include "paint/ligmapaintcore.h"
#include "paint/ligmapaintoptions.h"

#include "widgets/ligmadevices.h"
#include "widgets/ligmawidgets-utils.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmadisplayshell-selection.h"
#include "display/ligmadisplayshell-utils.h"

#include "ligmacoloroptions.h"
#include "ligmapaintoptions-gui.h"
#include "ligmapainttool.h"
#include "ligmapainttool-paint.h"
#include "ligmatoolcontrol.h"
#include "ligmatools-utils.h"

#include "ligma-intl.h"


static void   ligma_paint_tool_constructed    (GObject               *object);
static void   ligma_paint_tool_finalize       (GObject               *object);

static void   ligma_paint_tool_control        (LigmaTool              *tool,
                                              LigmaToolAction         action,
                                              LigmaDisplay           *display);
static void   ligma_paint_tool_button_press   (LigmaTool              *tool,
                                              const LigmaCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              LigmaButtonPressType    press_type,
                                              LigmaDisplay           *display);
static void   ligma_paint_tool_button_release (LigmaTool              *tool,
                                              const LigmaCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              LigmaButtonReleaseType  release_type,
                                              LigmaDisplay           *display);
static void   ligma_paint_tool_motion         (LigmaTool              *tool,
                                              const LigmaCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              LigmaDisplay           *display);
static void   ligma_paint_tool_modifier_key   (LigmaTool              *tool,
                                              GdkModifierType        key,
                                              gboolean               press,
                                              GdkModifierType        state,
                                              LigmaDisplay           *display);
static void   ligma_paint_tool_cursor_update  (LigmaTool              *tool,
                                              const LigmaCoords      *coords,
                                              GdkModifierType        state,
                                              LigmaDisplay           *display);
static void   ligma_paint_tool_oper_update    (LigmaTool              *tool,
                                              const LigmaCoords      *coords,
                                              GdkModifierType        state,
                                              gboolean               proximity,
                                              LigmaDisplay           *display);

static void   ligma_paint_tool_draw           (LigmaDrawTool          *draw_tool);

static void
          ligma_paint_tool_real_paint_prepare (LigmaPaintTool         *paint_tool,
                                              LigmaDisplay           *display);

static LigmaCanvasItem *
              ligma_paint_tool_get_outline    (LigmaPaintTool         *paint_tool,
                                              LigmaDisplay           *display,
                                              gdouble                x,
                                              gdouble                y);

static gboolean  ligma_paint_tool_check_alpha (LigmaPaintTool         *paint_tool,
                                              LigmaDrawable          *drawable,
                                              LigmaDisplay           *display,
                                              GError               **error);

static void   ligma_paint_tool_hard_notify    (LigmaPaintOptions      *options,
                                              const GParamSpec      *pspec,
                                              LigmaPaintTool         *paint_tool);
static void   ligma_paint_tool_cursor_notify  (LigmaDisplayConfig     *config,
                                              GParamSpec            *pspec,
                                              LigmaPaintTool         *paint_tool);


G_DEFINE_TYPE (LigmaPaintTool, ligma_paint_tool, LIGMA_TYPE_COLOR_TOOL)

#define parent_class ligma_paint_tool_parent_class


static void
ligma_paint_tool_class_init (LigmaPaintToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  LigmaToolClass     *tool_class      = LIGMA_TOOL_CLASS (klass);
  LigmaDrawToolClass *draw_tool_class = LIGMA_DRAW_TOOL_CLASS (klass);

  object_class->constructed  = ligma_paint_tool_constructed;
  object_class->finalize     = ligma_paint_tool_finalize;

  tool_class->control        = ligma_paint_tool_control;
  tool_class->button_press   = ligma_paint_tool_button_press;
  tool_class->button_release = ligma_paint_tool_button_release;
  tool_class->motion         = ligma_paint_tool_motion;
  tool_class->modifier_key   = ligma_paint_tool_modifier_key;
  tool_class->cursor_update  = ligma_paint_tool_cursor_update;
  tool_class->oper_update    = ligma_paint_tool_oper_update;

  draw_tool_class->draw      = ligma_paint_tool_draw;

  klass->paint_prepare       = ligma_paint_tool_real_paint_prepare;
}

static void
ligma_paint_tool_init (LigmaPaintTool *paint_tool)
{
  LigmaTool *tool = LIGMA_TOOL (paint_tool);

  ligma_tool_control_set_motion_mode    (tool->control, LIGMA_MOTION_MODE_EXACT);
  ligma_tool_control_set_scroll_lock    (tool->control, TRUE);
  ligma_tool_control_set_action_opacity (tool->control,
                                        "context/context-opacity-set");

  paint_tool->active          = TRUE;
  paint_tool->pick_colors     = FALSE;
  paint_tool->can_multi_paint = FALSE;
  paint_tool->draw_line       = FALSE;

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
ligma_paint_tool_constructed (GObject *object)
{
  LigmaTool          *tool       = LIGMA_TOOL (object);
  LigmaPaintTool     *paint_tool = LIGMA_PAINT_TOOL (object);
  LigmaPaintOptions  *options    = LIGMA_PAINT_TOOL_GET_OPTIONS (tool);
  LigmaDisplayConfig *display_config;
  LigmaPaintInfo     *paint_info;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_TOOL_INFO (tool->tool_info));
  ligma_assert (LIGMA_IS_PAINT_INFO (tool->tool_info->paint_info));

  display_config = LIGMA_DISPLAY_CONFIG (tool->tool_info->ligma->config);

  paint_info = tool->tool_info->paint_info;

  ligma_assert (g_type_is_a (paint_info->paint_type, LIGMA_TYPE_PAINT_CORE));

  paint_tool->core = g_object_new (paint_info->paint_type,
                                   "undo-desc", paint_info->blurb,
                                   NULL);

  g_signal_connect_object (options, "notify::hard",
                           G_CALLBACK (ligma_paint_tool_hard_notify),
                           paint_tool, 0);

  ligma_paint_tool_hard_notify (options, NULL, paint_tool);

  paint_tool->show_cursor = display_config->show_paint_tool_cursor;
  paint_tool->draw_brush  = display_config->show_brush_outline;
  paint_tool->snap_brush  = display_config->snap_brush_outline;

  g_signal_connect_object (display_config, "notify::show-paint-tool-cursor",
                           G_CALLBACK (ligma_paint_tool_cursor_notify),
                           paint_tool, 0);
  g_signal_connect_object (display_config, "notify::show-brush-outline",
                           G_CALLBACK (ligma_paint_tool_cursor_notify),
                           paint_tool, 0);
  g_signal_connect_object (display_config, "notify::snap-brush-outline",
                           G_CALLBACK (ligma_paint_tool_cursor_notify),
                           paint_tool, 0);
}

static void
ligma_paint_tool_finalize (GObject *object)
{
  LigmaPaintTool *paint_tool = LIGMA_PAINT_TOOL (object);

  if (paint_tool->core)
    {
      g_object_unref (paint_tool->core);
      paint_tool->core = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_paint_tool_control (LigmaTool       *tool,
                         LigmaToolAction  action,
                         LigmaDisplay    *display)
{
  LigmaPaintTool *paint_tool = LIGMA_PAINT_TOOL (tool);

  switch (action)
    {
    case LIGMA_TOOL_ACTION_PAUSE:
    case LIGMA_TOOL_ACTION_RESUME:
      break;

    case LIGMA_TOOL_ACTION_HALT:
      ligma_paint_core_cleanup (paint_tool->core);
      break;

    case LIGMA_TOOL_ACTION_COMMIT:
      break;
    }

  LIGMA_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
ligma_paint_tool_button_press (LigmaTool            *tool,
                              const LigmaCoords    *coords,
                              guint32              time,
                              GdkModifierType      state,
                              LigmaButtonPressType  press_type,
                              LigmaDisplay         *display)
{
  LigmaDrawTool     *draw_tool  = LIGMA_DRAW_TOOL (tool);
  LigmaPaintTool    *paint_tool = LIGMA_PAINT_TOOL (tool);
  LigmaPaintOptions *options    = LIGMA_PAINT_TOOL_GET_OPTIONS (tool);
  LigmaGuiConfig    *config     = LIGMA_GUI_CONFIG (display->ligma->config);
  LigmaDisplayShell *shell      = ligma_display_get_shell (display);
  LigmaImage        *image      = ligma_display_get_image (display);
  GList            *drawables;
  GList            *iter;
  gboolean          constrain;
  GError           *error = NULL;

  if (ligma_color_tool_is_enabled (LIGMA_COLOR_TOOL (tool)))
    {
      LIGMA_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                    press_type, display);
      return;
    }

  drawables  = ligma_image_get_selected_drawables (image);
  if (drawables == NULL)
    {
      ligma_tool_message_literal (tool, display,
                                 _("No selected drawables."));

      return;
    }
  else if (! paint_tool->can_multi_paint)
    {
      if (g_list_length (drawables) != 1)
        {
          ligma_tool_message_literal (tool, display,
                                     _("Cannot paint on multiple layers. Select only one layer."));

          g_list_free (drawables);
          return;
        }
    }

  for (iter = drawables; iter; iter = iter->next)
    {
      LigmaDrawable *drawable    = iter->data;
      LigmaItem     *locked_item = NULL;

      if (ligma_viewable_get_children (LIGMA_VIEWABLE (drawable)))
        {
          ligma_tool_message_literal (tool, display,
                                     _("Cannot paint on layer groups."));
          g_list_free (drawables);

          return;
        }

      if (ligma_item_is_content_locked (LIGMA_ITEM (drawable), &locked_item))
        {
          ligma_tool_message_literal (tool, display,
                                     _("The selected item's pixels are locked."));
          ligma_tools_blink_lock_box (display->ligma, locked_item);
          g_list_free (drawables);

          return;
        }

      if (! ligma_paint_tool_check_alpha (paint_tool, drawable, display, &error))
        {
          GtkWidget *options_gui;
          GtkWidget *mode_box;

          ligma_tool_message_literal (tool, display, error->message);

          options_gui = ligma_tools_get_tool_options_gui (LIGMA_TOOL_OPTIONS (options));
          mode_box    = ligma_paint_options_gui_get_paint_mode_box (options_gui);

          if (gtk_widget_is_sensitive (mode_box))
            {
              ligma_tools_show_tool_options (display->ligma);
              ligma_widget_blink (mode_box);
            }

          g_clear_error (&error);
          g_list_free (drawables);

          return;
        }

      if (! ligma_item_is_visible (LIGMA_ITEM (drawable)) &&
          ! config->edit_non_visible)
        {
          ligma_tool_message_literal (tool, display,
                                     _("A selected layer is not visible."));
          g_list_free (drawables);

          return;
        }
    }

  if (ligma_draw_tool_is_active (draw_tool))
    ligma_draw_tool_stop (draw_tool);

  if (tool->display            &&
      tool->display != display &&
      ligma_display_get_image (tool->display) == image)
    {
      /*  if this is a different display, but the same image, HACK around
       *  in tool internals AFTER stopping the current draw_tool, so
       *  straight line drawing works across different views of the
       *  same image.
       */

      tool->display = display;
    }

  constrain = (state & ligma_get_constrain_behavior_mask ()) != 0;

  if (! ligma_paint_tool_paint_start (paint_tool,
                                     display, coords, time, constrain,
                                     &error))
    {
      ligma_tool_message_literal (tool, display, error->message);
      g_clear_error (&error);
      return;
    }

  tool->display   = display;
  tool->drawables = drawables;

  /*  pause the current selection  */
  ligma_display_shell_selection_pause (shell);

  ligma_draw_tool_start (draw_tool, display);

  ligma_tool_control_activate (tool->control);
}

static void
ligma_paint_tool_button_release (LigmaTool              *tool,
                                const LigmaCoords      *coords,
                                guint32                time,
                                GdkModifierType        state,
                                LigmaButtonReleaseType  release_type,
                                LigmaDisplay           *display)
{
  LigmaPaintTool    *paint_tool = LIGMA_PAINT_TOOL (tool);
  LigmaDisplayShell *shell      = ligma_display_get_shell (display);
  LigmaImage        *image      = ligma_display_get_image (display);
  gboolean          cancel;

  if (ligma_color_tool_is_enabled (LIGMA_COLOR_TOOL (tool)))
    {
      LIGMA_TOOL_CLASS (parent_class)->button_release (tool, coords, time,
                                                      state, release_type,
                                                      display);
      return;
    }

  cancel = (release_type == LIGMA_BUTTON_RELEASE_CANCEL);

  ligma_paint_tool_paint_end (paint_tool, time, cancel);

  ligma_tool_control_halt (tool->control);

  ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

  /*  resume the current selection  */
  ligma_display_shell_selection_resume (shell);

  ligma_image_flush (image);

  ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
}

static void
ligma_paint_tool_motion (LigmaTool         *tool,
                        const LigmaCoords *coords,
                        guint32           time,
                        GdkModifierType   state,
                        LigmaDisplay      *display)
{
  LigmaPaintTool *paint_tool = LIGMA_PAINT_TOOL (tool);

  LIGMA_TOOL_CLASS (parent_class)->motion (tool, coords, time, state, display);

  if (ligma_color_tool_is_enabled (LIGMA_COLOR_TOOL (tool)))
    return;

  if (! paint_tool->snap_brush)
    ligma_draw_tool_pause  (LIGMA_DRAW_TOOL (tool));

  ligma_paint_tool_paint_motion (paint_tool, coords, time);

  if (! paint_tool->snap_brush)
    ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
}

static void
ligma_paint_tool_modifier_key (LigmaTool        *tool,
                              GdkModifierType  key,
                              gboolean         press,
                              GdkModifierType  state,
                              LigmaDisplay     *display)
{
  LigmaPaintTool *paint_tool = LIGMA_PAINT_TOOL (tool);
  LigmaDrawTool  *draw_tool  = LIGMA_DRAW_TOOL (tool);

  if (paint_tool->pick_colors && ! paint_tool->draw_line)
    {
      if ((state & ligma_get_all_modifiers_mask ()) ==
          ligma_get_constrain_behavior_mask ())
        {
          if (! ligma_color_tool_is_enabled (LIGMA_COLOR_TOOL (tool)))
            {
              LigmaToolInfo *info = ligma_get_tool_info (display->ligma,
                                                       "ligma-color-picker-tool");

              if (LIGMA_IS_TOOL_INFO (info))
                {
                  if (ligma_draw_tool_is_active (draw_tool))
                    ligma_draw_tool_stop (draw_tool);

                  ligma_color_tool_enable (LIGMA_COLOR_TOOL (tool),
                                          LIGMA_COLOR_OPTIONS (info->tool_options));

                  switch (LIGMA_COLOR_TOOL (tool)->pick_target)
                    {
                    case LIGMA_COLOR_PICK_TARGET_FOREGROUND:
                      ligma_tool_push_status (tool, display,
                                             _("Click in any image to pick the "
                                               "foreground color"));
                      break;

                    case LIGMA_COLOR_PICK_TARGET_BACKGROUND:
                      ligma_tool_push_status (tool, display,
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
          if (ligma_color_tool_is_enabled (LIGMA_COLOR_TOOL (tool)))
            {
              ligma_tool_pop_status (tool, display);
              ligma_color_tool_disable (LIGMA_COLOR_TOOL (tool));
            }
        }
    }
}

static void
ligma_paint_tool_cursor_update (LigmaTool         *tool,
                               const LigmaCoords *coords,
                               GdkModifierType   state,
                               LigmaDisplay      *display)
{
  LigmaPaintTool      *paint_tool = LIGMA_PAINT_TOOL (tool);
  LigmaGuiConfig      *config     = LIGMA_GUI_CONFIG (display->ligma->config);
  LigmaCursorModifier  modifier;
  LigmaCursorModifier  toggle_modifier;
  LigmaCursorModifier  old_modifier;
  LigmaCursorModifier  old_toggle_modifier;

  modifier        = tool->control->cursor_modifier;
  toggle_modifier = tool->control->toggle_cursor_modifier;

  old_modifier        = modifier;
  old_toggle_modifier = toggle_modifier;

  if (! ligma_color_tool_is_enabled (LIGMA_COLOR_TOOL (tool)))
    {
      LigmaImage    *image     = ligma_display_get_image (display);
      GList        *drawables = ligma_image_get_selected_drawables (image);
      GList        *iter;

      if (! drawables)
        return;

      for (iter = drawables; iter; iter = iter->next)
        {
          LigmaDrawable *drawable = iter->data;

          if (ligma_viewable_get_children (LIGMA_VIEWABLE (drawable))               ||
              ligma_item_is_content_locked (LIGMA_ITEM (drawable), NULL)            ||
              ! ligma_paint_tool_check_alpha (paint_tool, drawable, display, NULL) ||
              ! (ligma_item_is_visible (LIGMA_ITEM (drawable)) ||
                 config->edit_non_visible))
            {
              modifier        = LIGMA_CURSOR_MODIFIER_BAD;
              toggle_modifier = LIGMA_CURSOR_MODIFIER_BAD;
              break;
            }
        }

      g_list_free (drawables);

      if (! paint_tool->show_cursor &&
          modifier != LIGMA_CURSOR_MODIFIER_BAD)
        {
          if (paint_tool->draw_brush)
            ligma_tool_set_cursor (tool, display,
                                  LIGMA_CURSOR_NONE,
                                  LIGMA_TOOL_CURSOR_NONE,
                                  LIGMA_CURSOR_MODIFIER_NONE);
          else
            ligma_tool_set_cursor (tool, display,
                                  LIGMA_CURSOR_SINGLE_DOT,
                                  LIGMA_TOOL_CURSOR_NONE,
                                  LIGMA_CURSOR_MODIFIER_NONE);
          return;
        }

      ligma_tool_control_set_cursor_modifier        (tool->control,
                                                    modifier);
      ligma_tool_control_set_toggle_cursor_modifier (tool->control,
                                                    toggle_modifier);
    }

  LIGMA_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                 display);

  /*  reset old stuff here so we are not interfering with the modifiers
   *  set by our subclasses
   */
  ligma_tool_control_set_cursor_modifier        (tool->control,
                                                old_modifier);
  ligma_tool_control_set_toggle_cursor_modifier (tool->control,
                                                old_toggle_modifier);
}

static void
ligma_paint_tool_oper_update (LigmaTool         *tool,
                             const LigmaCoords *coords,
                             GdkModifierType   state,
                             gboolean          proximity,
                             LigmaDisplay      *display)
{
  LigmaPaintTool    *paint_tool    = LIGMA_PAINT_TOOL (tool);
  LigmaDrawTool     *draw_tool     = LIGMA_DRAW_TOOL (tool);
  LigmaPaintOptions *paint_options = LIGMA_PAINT_TOOL_GET_OPTIONS (tool);
  LigmaPaintCore    *core          = paint_tool->core;
  LigmaDisplayShell *shell         = ligma_display_get_shell (display);
  LigmaImage        *image         = ligma_display_get_image (display);
  GList            *drawables;
  gchar            *status        = NULL;

  if (ligma_color_tool_is_enabled (LIGMA_COLOR_TOOL (tool)))
    {
      LIGMA_TOOL_CLASS (parent_class)->oper_update (tool, coords, state,
                                                   proximity, display);
      return;
    }

  ligma_draw_tool_pause (draw_tool);

  if (ligma_draw_tool_is_active (draw_tool) &&
      draw_tool->display != display)
    ligma_draw_tool_stop (draw_tool);

  if (tool->display            &&
      tool->display != display &&
      ligma_display_get_image (tool->display) == image)
    {
      /*  if this is a different display, but the same image, HACK around
       *  in tool internals AFTER stopping the current draw_tool, so
       *  straight line drawing works across different views of the
       *  same image.
       */

      tool->display = display;
    }

  drawables = ligma_image_get_selected_drawables (image);

  if ((g_list_length (drawables) == 1 ||
       (g_list_length (drawables) > 1 && paint_tool->can_multi_paint)) &&
      proximity)
    {
      gboolean  constrain_mask = ligma_get_constrain_behavior_mask ();

      core->cur_coords = *coords;

      if (display == tool->display && (state & LIGMA_PAINT_TOOL_LINE_MASK))
        {
          /*  If shift is down and this is not the first paint stroke,
           *  draw a line.
           */
          gchar   *status_help;
          gdouble  offset_angle;
          gdouble  xres, yres;

          ligma_display_shell_get_constrained_line_params (shell,
                                                          &offset_angle,
                                                          &xres, &yres);

          ligma_paint_core_round_line (core, paint_options,
                                      (state & constrain_mask) != 0,
                                      offset_angle, xres, yres);

          status_help = ligma_suggest_modifiers (paint_tool->status_line,
                                                constrain_mask & ~state,
                                                NULL,
                                                _("%s for constrained angles"),
                                                NULL);

          status = ligma_display_shell_get_line_status (shell, status_help,
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
           * ligma_suggest_modifiers() would interpret this parameter.
           */
          if (paint_tool->status_ctrl != NULL)
            modifiers |= constrain_mask;

          /* suggest drawing lines only after the first point is set
           */
          if (display == tool->display)
            modifiers |= LIGMA_PAINT_TOOL_LINE_MASK;

          status = ligma_suggest_modifiers (paint_tool->status,
                                           modifiers & ~state,
                                           _("%s for a straight line"),
                                           paint_tool->status_ctrl,
                                           NULL);
          paint_tool->draw_line = FALSE;
        }

      paint_tool->cursor_x = core->cur_coords.x;
      paint_tool->cursor_y = core->cur_coords.y;

      if (! ligma_draw_tool_is_active (draw_tool))
        ligma_draw_tool_start (draw_tool, display);
    }
  else if (ligma_draw_tool_is_active (draw_tool))
    {
      ligma_draw_tool_stop (draw_tool);
    }

  if (status != NULL)
    {
      ligma_tool_push_status (tool, display, "%s", status);
      g_free (status);
    }
  else
    {
      ligma_tool_pop_status (tool, display);
    }

  LIGMA_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);

  ligma_draw_tool_resume (draw_tool);
  g_list_free (drawables);
}

static void
ligma_paint_tool_draw (LigmaDrawTool *draw_tool)
{
  LigmaPaintTool *paint_tool = LIGMA_PAINT_TOOL (draw_tool);

  if (paint_tool->active &&
      ! ligma_color_tool_is_enabled (LIGMA_COLOR_TOOL (draw_tool)))
    {
      LigmaPaintCore  *core       = paint_tool->core;
      LigmaCanvasItem *outline    = NULL;
      gboolean        line_drawn = FALSE;
      gdouble         cur_x, cur_y;

      if (ligma_paint_tool_paint_is_active (paint_tool) &&
          paint_tool->snap_brush)
        {
          cur_x = paint_tool->paint_x;
          cur_y = paint_tool->paint_y;
        }
      else
        {
          cur_x = paint_tool->cursor_x;
          cur_y = paint_tool->cursor_y;

          if (paint_tool->draw_line &&
              ! ligma_tool_control_is_active (LIGMA_TOOL (draw_tool)->control))
            {
              LigmaCanvasGroup *group;
              gdouble          last_x, last_y;

              last_x = core->last_coords.x;
              last_y = core->last_coords.y;

              group = ligma_draw_tool_add_stroke_group (draw_tool);
              ligma_draw_tool_push_group (draw_tool, group);

              ligma_draw_tool_add_handle (draw_tool,
                                         LIGMA_HANDLE_CIRCLE,
                                         last_x, last_y,
                                         LIGMA_TOOL_HANDLE_SIZE_CIRCLE,
                                         LIGMA_TOOL_HANDLE_SIZE_CIRCLE,
                                         LIGMA_HANDLE_ANCHOR_CENTER);

              ligma_draw_tool_add_line (draw_tool,
                                       last_x, last_y,
                                       cur_x, cur_y);

              ligma_draw_tool_add_handle (draw_tool,
                                         LIGMA_HANDLE_CIRCLE,
                                         cur_x, cur_y,
                                         LIGMA_TOOL_HANDLE_SIZE_CIRCLE,
                                         LIGMA_TOOL_HANDLE_SIZE_CIRCLE,
                                         LIGMA_HANDLE_ANCHOR_CENTER);

              ligma_draw_tool_pop_group (draw_tool);

              line_drawn = TRUE;
            }
        }

      ligma_paint_tool_set_draw_fallback (paint_tool, FALSE, 0.0);
      ligma_paint_tool_set_draw_circle (paint_tool, FALSE, 0.0);

      if (paint_tool->draw_brush)
        outline = ligma_paint_tool_get_outline (paint_tool,
                                               draw_tool->display,
                                               cur_x, cur_y);

      if (outline)
        {
          ligma_draw_tool_add_item (draw_tool, outline);
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
          ligma_draw_tool_add_arc (draw_tool,
                                  FALSE,
                                  cur_x - (size / 2.0),
                                  cur_y - (size / 2.0),
                                  size, size,
                                  ROTATION_ANGLE - (2.0 * G_PI) / (TICKMARK_ANGLE * 2),
                                  (2.0 * G_PI) / TICKMARK_ANGLE);

          ligma_draw_tool_add_arc (draw_tool,
                                  FALSE,
                                  cur_x - (size / 2.0),
                                  cur_y - (size / 2.0),
                                  size, size,
                                  ROTATION_ANGLE + G_PI / 2 - (2.0 * G_PI) / (TICKMARK_ANGLE * 2),
                                  (2.0 * G_PI) / TICKMARK_ANGLE);

          ligma_draw_tool_add_arc (draw_tool,
                                  FALSE,
                                  cur_x - (size / 2.0),
                                  cur_y - (size / 2.0),
                                  size, size,
                                  ROTATION_ANGLE + G_PI - (2.0 * G_PI) / (TICKMARK_ANGLE * 2),
                                  (2.0 * G_PI) / TICKMARK_ANGLE);

          ligma_draw_tool_add_arc (draw_tool,
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
          ligma_draw_tool_add_arc (draw_tool,
                                  FALSE,
                                  cur_x - (size / 2.0),
                                  cur_y - (size / 2.0),
                                  size, size,
                                  0.0, (2.0 * G_PI));
        }

      if (! outline                   &&
          ! paint_tool->draw_fallback &&
          ! line_drawn                &&
          ! paint_tool->show_cursor   &&
          ! paint_tool->draw_circle)
        {
          /* I am not sure this case can/should ever happen since now we
           * always set the LIGMA_CURSOR_SINGLE_DOT when neither pointer
           * nor outline options are checked. Yet let's imagine any
           * weird case where brush outline is wanted, without pointer
           * cursor, yet we fail to draw the outline while neither
           * circle nor fallbacks are requested (it depends on per-class
           * implementation of get_outline()).
           *
           * In such a case, we don't want to leave the user without any
           * indication so we draw a fallback crosshair.
           */
          if (paint_tool->draw_brush)
            ligma_draw_tool_add_handle (draw_tool,
                                       LIGMA_HANDLE_CIRCLE,
                                       cur_x, cur_y,
                                       LIGMA_TOOL_HANDLE_SIZE_CROSSHAIR,
                                       LIGMA_TOOL_HANDLE_SIZE_CROSSHAIR,
                                       LIGMA_HANDLE_ANCHOR_CENTER);
        }
    }

  LIGMA_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
}

static void
ligma_paint_tool_real_paint_prepare (LigmaPaintTool *paint_tool,
                                    LigmaDisplay   *display)
{
  LigmaDisplayShell *shell = ligma_display_get_shell (display);

  ligma_paint_core_set_show_all (paint_tool->core, shell->show_all);
}

static LigmaCanvasItem *
ligma_paint_tool_get_outline (LigmaPaintTool *paint_tool,
                             LigmaDisplay   *display,
                             gdouble        x,
                             gdouble        y)
{
  if (LIGMA_PAINT_TOOL_GET_CLASS (paint_tool)->get_outline)
    return LIGMA_PAINT_TOOL_GET_CLASS (paint_tool)->get_outline (paint_tool,
                                                                display, x, y);

  return NULL;
}

static gboolean
ligma_paint_tool_check_alpha (LigmaPaintTool  *paint_tool,
                             LigmaDrawable   *drawable,
                             LigmaDisplay    *display,
                             GError        **error)
{
  LigmaPaintToolClass *klass = LIGMA_PAINT_TOOL_GET_CLASS (paint_tool);

  if (klass->is_alpha_only && klass->is_alpha_only (paint_tool, drawable))
    {
      LigmaLayer *locked_layer = NULL;

      if (! ligma_drawable_has_alpha (drawable))
        {
          g_set_error_literal (
            error, LIGMA_ERROR, LIGMA_FAILED,
            _("The selected drawable does not have an alpha channel."));

          return FALSE;
        }

        if (LIGMA_IS_LAYER (drawable) &&
            ligma_layer_is_alpha_locked (LIGMA_LAYER (drawable),
                                        &locked_layer))
        {
          g_set_error_literal (
            error, LIGMA_ERROR, LIGMA_FAILED,
            _("The selected layer's alpha channel is locked."));

          if (error)
            ligma_tools_blink_lock_box (display->ligma, LIGMA_ITEM (locked_layer));

          return FALSE;
        }
    }

  return TRUE;
}

static void
ligma_paint_tool_hard_notify (LigmaPaintOptions *options,
                             const GParamSpec *pspec,
                             LigmaPaintTool    *paint_tool)
{
  if (paint_tool->active)
    {
      LigmaTool *tool = LIGMA_TOOL (paint_tool);

      ligma_tool_control_set_precision (tool->control,
                                       options->hard ?
                                       LIGMA_CURSOR_PRECISION_PIXEL_CENTER :
                                       LIGMA_CURSOR_PRECISION_SUBPIXEL);
    }
}

static void
ligma_paint_tool_cursor_notify (LigmaDisplayConfig *config,
                               GParamSpec        *pspec,
                               LigmaPaintTool     *paint_tool)
{
  ligma_draw_tool_pause (LIGMA_DRAW_TOOL (paint_tool));

  paint_tool->show_cursor = config->show_paint_tool_cursor;
  paint_tool->draw_brush  = config->show_brush_outline;
  paint_tool->snap_brush  = config->snap_brush_outline;

  ligma_draw_tool_resume (LIGMA_DRAW_TOOL (paint_tool));
}

void
ligma_paint_tool_set_active (LigmaPaintTool *tool,
                            gboolean       active)
{
  g_return_if_fail (LIGMA_IS_PAINT_TOOL (tool));

  if (active != tool->active)
    {
      LigmaPaintOptions *options = LIGMA_PAINT_TOOL_GET_OPTIONS (tool);

      ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

      tool->active = active;

      if (active)
        ligma_paint_tool_hard_notify (options, NULL, tool);

      ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
    }
}

/**
 * ligma_paint_tool_enable_color_picker:
 * @tool:   a #LigmaPaintTool
 * @target: the #LigmaColorPickTarget to set
 *
 * This is a convenience function used from the init method of paint
 * tools that want the color picking functionality. The @mode that is
 * set here is used to decide what cursor modifier to draw and if the
 * picked color goes to the foreground or background color.
 **/
void
ligma_paint_tool_enable_color_picker (LigmaPaintTool       *tool,
                                     LigmaColorPickTarget  target)
{
  g_return_if_fail (LIGMA_IS_PAINT_TOOL (tool));

  tool->pick_colors = TRUE;

  LIGMA_COLOR_TOOL (tool)->pick_target = target;
}

/**
 * ligma_paint_tool_enable_multi_paint:
 * @tool: a #LigmaPaintTool
 *
 * This is a convenience function used from the init method of paint
 * tools that want to allow painting with several drawables.
 **/
void
ligma_paint_tool_enable_multi_paint (LigmaPaintTool *tool)
{
  g_return_if_fail (LIGMA_IS_PAINT_TOOL (tool));

  tool->can_multi_paint = TRUE;
}

void
ligma_paint_tool_set_draw_fallback (LigmaPaintTool *tool,
                                   gboolean       draw_fallback,
                                   gint           fallback_size)
{
  g_return_if_fail (LIGMA_IS_PAINT_TOOL (tool));

  tool->draw_fallback = draw_fallback;
  tool->fallback_size = fallback_size;
}

void
ligma_paint_tool_set_draw_circle (LigmaPaintTool *tool,
                                 gboolean       draw_circle,
                                 gint           circle_size)
{
  g_return_if_fail (LIGMA_IS_PAINT_TOOL (tool));

  tool->draw_circle = draw_circle;
  tool->circle_size = circle_size;
}


/**
 * ligma_paint_tool_force_draw:
 * @tool:
 * @force:
 *
 * If @force is %TRUE, the brush, or a fallback, or circle will be
 * drawn, regardless of the Preferences settings. This can be used for
 * code such as when modifying brush size or shape on-canvas with a
 * visual feedback, temporarily bypassing the user setting.
 */
void
ligma_paint_tool_force_draw (LigmaPaintTool *tool,
                            gboolean       force)
{
  LigmaDisplayConfig *display_config;

  g_return_if_fail (LIGMA_IS_PAINT_TOOL (tool));

  display_config = LIGMA_DISPLAY_CONFIG (LIGMA_TOOL (tool)->tool_info->ligma->config);

  if (force)
    tool->draw_brush = TRUE;
  else
    tool->draw_brush  = display_config->show_brush_outline;
}
