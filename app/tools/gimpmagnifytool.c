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

#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "config/ligmaguiconfig.h"

#include "core/ligmaimage.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmawidgets-utils.h"

#include "display/ligmacanvasrectangle.h"
#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmadisplayshell-scale.h"

#include "ligmamagnifyoptions.h"
#include "ligmamagnifytool.h"
#include "ligmatoolcontrol.h"

#include "ligma-intl.h"


static void   ligma_magnify_tool_button_press   (LigmaTool              *tool,
                                                const LigmaCoords      *coords,
                                                guint32                time,
                                                GdkModifierType        state,
                                                LigmaButtonPressType    press_type,
                                                LigmaDisplay           *display);
static void   ligma_magnify_tool_button_release (LigmaTool              *tool,
                                                const LigmaCoords      *coords,
                                                guint32                time,
                                                GdkModifierType        state,
                                                LigmaButtonReleaseType  release_type,
                                                LigmaDisplay           *display);
static void   ligma_magnify_tool_motion         (LigmaTool              *tool,
                                                const LigmaCoords      *coords,
                                                guint32                time,
                                                GdkModifierType        state,
                                                LigmaDisplay           *display);
static void   ligma_magnify_tool_modifier_key   (LigmaTool              *tool,
                                                GdkModifierType        key,
                                                gboolean               press,
                                                GdkModifierType        state,
                                                LigmaDisplay           *display);
static void   ligma_magnify_tool_cursor_update  (LigmaTool              *tool,
                                                const LigmaCoords      *coords,
                                                GdkModifierType        state,
                                                LigmaDisplay           *display);

static void   ligma_magnify_tool_draw           (LigmaDrawTool          *draw_tool);

static void   ligma_magnify_tool_update_items   (LigmaMagnifyTool       *magnify);


G_DEFINE_TYPE (LigmaMagnifyTool, ligma_magnify_tool, LIGMA_TYPE_DRAW_TOOL)

#define parent_class ligma_magnify_tool_parent_class


void
ligma_magnify_tool_register (LigmaToolRegisterCallback  callback,
                            gpointer                  data)
{
  (* callback) (LIGMA_TYPE_MAGNIFY_TOOL,
                LIGMA_TYPE_MAGNIFY_OPTIONS,
                ligma_magnify_options_gui,
                0,
                "ligma-zoom-tool",
                _("Zoom"),
                _("Zoom Tool: Adjust the zoom level"),
                N_("_Zoom"), "Z",
                NULL, LIGMA_HELP_TOOL_ZOOM,
                LIGMA_ICON_TOOL_ZOOM,
                data);
}

static void
ligma_magnify_tool_class_init (LigmaMagnifyToolClass *klass)
{
  LigmaToolClass     *tool_class      = LIGMA_TOOL_CLASS (klass);
  LigmaDrawToolClass *draw_tool_class = LIGMA_DRAW_TOOL_CLASS (klass);

  tool_class->button_press   = ligma_magnify_tool_button_press;
  tool_class->button_release = ligma_magnify_tool_button_release;
  tool_class->motion         = ligma_magnify_tool_motion;
  tool_class->modifier_key   = ligma_magnify_tool_modifier_key;
  tool_class->cursor_update  = ligma_magnify_tool_cursor_update;

  draw_tool_class->draw      = ligma_magnify_tool_draw;
}

static void
ligma_magnify_tool_init (LigmaMagnifyTool *magnify_tool)
{
  LigmaTool *tool = LIGMA_TOOL (magnify_tool);

  ligma_tool_control_set_scroll_lock            (tool->control, TRUE);
  ligma_tool_control_set_handle_empty_image     (tool->control, TRUE);
  ligma_tool_control_set_wants_click            (tool->control, TRUE);
  ligma_tool_control_set_snap_to                (tool->control, FALSE);

  ligma_tool_control_set_tool_cursor            (tool->control,
                                                LIGMA_TOOL_CURSOR_ZOOM);
  ligma_tool_control_set_cursor_modifier        (tool->control,
                                                LIGMA_CURSOR_MODIFIER_PLUS);
  ligma_tool_control_set_toggle_cursor_modifier (tool->control,
                                                LIGMA_CURSOR_MODIFIER_MINUS);

  magnify_tool->x = 0;
  magnify_tool->y = 0;
  magnify_tool->w = 0;
  magnify_tool->h = 0;
}

static void
ligma_magnify_tool_button_press (LigmaTool            *tool,
                                const LigmaCoords    *coords,
                                guint32              time,
                                GdkModifierType      state,
                                LigmaButtonPressType  press_type,
                                LigmaDisplay         *display)
{
  LigmaMagnifyTool *magnify = LIGMA_MAGNIFY_TOOL (tool);

  magnify->x = coords->x;
  magnify->y = coords->y;
  magnify->w = 0;
  magnify->h = 0;

  ligma_tool_control_activate (tool->control);
  tool->display = display;

  ligma_draw_tool_start (LIGMA_DRAW_TOOL (tool), display);
}

static void
ligma_magnify_tool_button_release (LigmaTool              *tool,
                                  const LigmaCoords      *coords,
                                  guint32                time,
                                  GdkModifierType        state,
                                  LigmaButtonReleaseType  release_type,
                                  LigmaDisplay           *display)
{
  LigmaMagnifyTool    *magnify = LIGMA_MAGNIFY_TOOL (tool);
  LigmaMagnifyOptions *options = LIGMA_MAGNIFY_TOOL_GET_OPTIONS (tool);
  LigmaDisplayShell   *shell   = ligma_display_get_shell (tool->display);

  ligma_draw_tool_stop (LIGMA_DRAW_TOOL (tool));

  ligma_tool_control_halt (tool->control);

  switch (release_type)
    {
    case LIGMA_BUTTON_RELEASE_CLICK:
    case LIGMA_BUTTON_RELEASE_NO_MOTION:
      ligma_display_shell_scale (shell,
                                options->zoom_type,
                                0.0,
                                LIGMA_ZOOM_FOCUS_POINTER);
      break;

    case LIGMA_BUTTON_RELEASE_NORMAL:
      {
        gdouble  x, y;
        gdouble  width, height;
        gboolean resize_window;

        x      = (magnify->w < 0) ?  magnify->x + magnify->w : magnify->x;
        y      = (magnify->h < 0) ?  magnify->y + magnify->h : magnify->y;
        width  = (magnify->w < 0) ? -magnify->w : magnify->w;
        height = (magnify->h < 0) ? -magnify->h : magnify->h;

        /* Resize windows only in multi-window mode */
        resize_window = (options->auto_resize &&
                         ! LIGMA_GUI_CONFIG (display->config)->single_window_mode);

        ligma_display_shell_scale_to_rectangle (shell,
                                               options->zoom_type,
                                               x, y, width, height,
                                               resize_window);
      }
      break;

    default:
      break;
    }
}

static void
ligma_magnify_tool_motion (LigmaTool         *tool,
                          const LigmaCoords *coords,
                          guint32           time,
                          GdkModifierType   state,
                          LigmaDisplay      *display)
{
  LigmaMagnifyTool *magnify = LIGMA_MAGNIFY_TOOL (tool);

  magnify->w = coords->x - magnify->x;
  magnify->h = coords->y - magnify->y;

  ligma_magnify_tool_update_items (magnify);
}

static void
ligma_magnify_tool_modifier_key (LigmaTool        *tool,
                                GdkModifierType  key,
                                gboolean         press,
                                GdkModifierType  state,
                                LigmaDisplay     *display)
{
  LigmaMagnifyOptions *options = LIGMA_MAGNIFY_TOOL_GET_OPTIONS (tool);

  if (key == ligma_get_toggle_behavior_mask ())
    {
      switch (options->zoom_type)
        {
        case LIGMA_ZOOM_IN:
          g_object_set (options, "zoom-type", LIGMA_ZOOM_OUT, NULL);
          break;

        case LIGMA_ZOOM_OUT:
          g_object_set (options, "zoom-type", LIGMA_ZOOM_IN, NULL);
          break;

        default:
          break;
        }
    }
}

static void
ligma_magnify_tool_cursor_update (LigmaTool         *tool,
                                 const LigmaCoords *coords,
                                 GdkModifierType   state,
                                 LigmaDisplay      *display)
{
  LigmaMagnifyOptions *options = LIGMA_MAGNIFY_TOOL_GET_OPTIONS (tool);

  ligma_tool_control_set_toggled (tool->control,
                                 options->zoom_type == LIGMA_ZOOM_OUT);

  LIGMA_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
ligma_magnify_tool_draw (LigmaDrawTool *draw_tool)
{
  LigmaMagnifyTool *magnify = LIGMA_MAGNIFY_TOOL (draw_tool);

  magnify->rectangle =
    ligma_draw_tool_add_rectangle (draw_tool, FALSE,
                                  magnify->x,
                                  magnify->y,
                                  magnify->w,
                                  magnify->h);
}

static void
ligma_magnify_tool_update_items (LigmaMagnifyTool *magnify)
{
  if (ligma_draw_tool_is_active (LIGMA_DRAW_TOOL (magnify)))
    {
      ligma_canvas_rectangle_set (magnify->rectangle,
                                 magnify->x,
                                 magnify->y,
                                 magnify->w,
                                 magnify->h);
    }
}
