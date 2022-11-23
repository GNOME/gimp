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

#include "tools-types.h"

#include "core/ligma.h"
#include "core/ligmasamplepoint.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-sample-points.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmadisplayshell-selection.h"
#include "display/ligmadisplayshell-transform.h"

#include "ligmasamplepointtool.h"
#include "ligmatoolcontrol.h"
#include "tool_manager.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void   ligma_sample_point_tool_button_release (LigmaTool              *tool,
                                                     const LigmaCoords      *coords,
                                                     guint32                time,
                                                     GdkModifierType        state,
                                                     LigmaButtonReleaseType  release_type,
                                                     LigmaDisplay           *display);
static void   ligma_sample_point_tool_motion         (LigmaTool              *tool,
                                                     const LigmaCoords      *coords,
                                                     guint32                time,
                                                     GdkModifierType        state,
                                                     LigmaDisplay           *display);

static void   ligma_sample_point_tool_draw           (LigmaDrawTool          *draw_tool);

static void   ligma_sample_point_tool_start          (LigmaTool              *parent_tool,
                                                     LigmaDisplay           *display,
                                                     LigmaSamplePoint       *sample_point);


G_DEFINE_TYPE (LigmaSamplePointTool, ligma_sample_point_tool, LIGMA_TYPE_DRAW_TOOL)

#define parent_class ligma_sample_point_tool_parent_class


static void
ligma_sample_point_tool_class_init (LigmaSamplePointToolClass *klass)
{
  LigmaToolClass     *tool_class      = LIGMA_TOOL_CLASS (klass);
  LigmaDrawToolClass *draw_tool_class = LIGMA_DRAW_TOOL_CLASS (klass);

  tool_class->button_release = ligma_sample_point_tool_button_release;
  tool_class->motion         = ligma_sample_point_tool_motion;

  draw_tool_class->draw      = ligma_sample_point_tool_draw;
}

static void
ligma_sample_point_tool_init (LigmaSamplePointTool *sp_tool)
{
  LigmaTool *tool = LIGMA_TOOL (sp_tool);

  ligma_tool_control_set_snap_to            (tool->control, FALSE);
  ligma_tool_control_set_handle_empty_image (tool->control, TRUE);
  ligma_tool_control_set_tool_cursor        (tool->control,
                                            LIGMA_TOOL_CURSOR_MOVE);
  ligma_tool_control_set_scroll_lock        (tool->control, TRUE);
  ligma_tool_control_set_precision          (tool->control,
                                            LIGMA_CURSOR_PRECISION_PIXEL_CENTER);

  sp_tool->sample_point   = NULL;
  sp_tool->sample_point_x = LIGMA_SAMPLE_POINT_POSITION_UNDEFINED;
  sp_tool->sample_point_y = LIGMA_SAMPLE_POINT_POSITION_UNDEFINED;
}

static void
ligma_sample_point_tool_button_release (LigmaTool              *tool,
                                       const LigmaCoords      *coords,
                                       guint32                time,
                                       GdkModifierType        state,
                                       LigmaButtonReleaseType  release_type,
                                       LigmaDisplay           *display)
{
  LigmaSamplePointTool *sp_tool = LIGMA_SAMPLE_POINT_TOOL (tool);
  LigmaDisplayShell    *shell   = ligma_display_get_shell (display);
  LigmaImage           *image   = ligma_display_get_image (display);

  ligma_tool_pop_status (tool, display);

  ligma_tool_control_halt (tool->control);

  ligma_draw_tool_stop (LIGMA_DRAW_TOOL (tool));

  if (release_type != LIGMA_BUTTON_RELEASE_CANCEL)
    {
      gint width  = ligma_image_get_width  (image);
      gint height = ligma_image_get_height (image);

      if (sp_tool->sample_point_x == LIGMA_SAMPLE_POINT_POSITION_UNDEFINED ||
          sp_tool->sample_point_x <  0                                    ||
          sp_tool->sample_point_x >= width                                ||
          sp_tool->sample_point_y == LIGMA_SAMPLE_POINT_POSITION_UNDEFINED ||
          sp_tool->sample_point_y <  0                                    ||
          sp_tool->sample_point_y >= height)
        {
          if (sp_tool->sample_point)
            {
              ligma_image_remove_sample_point (image,
                                              sp_tool->sample_point, TRUE);
              sp_tool->sample_point = NULL;
            }
        }
      else
        {
          if (sp_tool->sample_point)
            {
              ligma_image_move_sample_point (image,
                                            sp_tool->sample_point,
                                            sp_tool->sample_point_x,
                                            sp_tool->sample_point_y,
                                            TRUE);
            }
          else
            {
              sp_tool->sample_point =
                ligma_image_add_sample_point_at_pos (image,
                                                    sp_tool->sample_point_x,
                                                    sp_tool->sample_point_y,
                                                    TRUE);
            }
        }

      ligma_image_flush (image);
    }

  ligma_display_shell_selection_resume (shell);

  sp_tool->sample_point_x = LIGMA_SAMPLE_POINT_POSITION_UNDEFINED;
  sp_tool->sample_point_y = LIGMA_SAMPLE_POINT_POSITION_UNDEFINED;

  tool_manager_pop_tool (display->ligma);
  g_object_unref (sp_tool);

  {
    LigmaTool *active_tool = tool_manager_get_active (display->ligma);

    if (LIGMA_IS_DRAW_TOOL (active_tool))
      ligma_draw_tool_pause (LIGMA_DRAW_TOOL (active_tool));

    tool_manager_oper_update_active (display->ligma, coords, state,
                                     TRUE, display);
    tool_manager_cursor_update_active (display->ligma, coords, state,
                                       display);

    if (LIGMA_IS_DRAW_TOOL (active_tool))
      ligma_draw_tool_resume (LIGMA_DRAW_TOOL (active_tool));
  }
}

static void
ligma_sample_point_tool_motion (LigmaTool         *tool,
                               const LigmaCoords *coords,
                               guint32           time,
                               GdkModifierType   state,
                               LigmaDisplay      *display)

{
  LigmaSamplePointTool *sp_tool      = LIGMA_SAMPLE_POINT_TOOL (tool);
  LigmaDisplayShell    *shell        = ligma_display_get_shell (display);
  gboolean             delete_point = FALSE;
  gint                 tx, ty;

  ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

  ligma_display_shell_transform_xy (shell,
                                   coords->x, coords->y,
                                   &tx, &ty);

  if (tx < 0 || tx >= shell->disp_width ||
      ty < 0 || ty >= shell->disp_height)
    {
      sp_tool->sample_point_x = LIGMA_SAMPLE_POINT_POSITION_UNDEFINED;
      sp_tool->sample_point_y = LIGMA_SAMPLE_POINT_POSITION_UNDEFINED;

      delete_point = TRUE;
    }
  else
    {
      LigmaImage *image  = ligma_display_get_image (display);
      gint       width  = ligma_image_get_width  (image);
      gint       height = ligma_image_get_height (image);

      sp_tool->sample_point_x = floor (coords->x);
      sp_tool->sample_point_y = floor (coords->y);

      if (sp_tool->sample_point_x <  0      ||
          sp_tool->sample_point_x >= height ||
          sp_tool->sample_point_y <  0      ||
          sp_tool->sample_point_y >= width)
        {
          delete_point = TRUE;
        }
    }

  ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));

  ligma_tool_pop_status (tool, display);

  if (delete_point)
    {
      ligma_tool_push_status (tool, display,
                             sp_tool->sample_point ?
                             _("Remove Sample Point") :
                             _("Cancel Sample Point"));
    }
  else if (sp_tool->sample_point)
    {
      ligma_tool_push_status_coords (tool, display,
                                    ligma_tool_control_get_precision (tool->control),
                                    _("Move Sample Point: "),
                                    sp_tool->sample_point_x -
                                    sp_tool->sample_point_old_x,
                                    ", ",
                                    sp_tool->sample_point_y -
                                    sp_tool->sample_point_old_y,
                                    NULL);
    }
  else
    {
      ligma_tool_push_status_coords (tool, display,
                                    ligma_tool_control_get_precision (tool->control),
                                    _("Add Sample Point: "),
                                    sp_tool->sample_point_x,
                                    ", ",
                                    sp_tool->sample_point_y,
                                    NULL);
    }
}

static void
ligma_sample_point_tool_draw (LigmaDrawTool *draw_tool)
{
  LigmaSamplePointTool *sp_tool = LIGMA_SAMPLE_POINT_TOOL (draw_tool);

  if (sp_tool->sample_point_x != LIGMA_SAMPLE_POINT_POSITION_UNDEFINED &&
      sp_tool->sample_point_y != LIGMA_SAMPLE_POINT_POSITION_UNDEFINED)
    {
      ligma_draw_tool_add_crosshair (draw_tool,
                                    sp_tool->sample_point_x,
                                    sp_tool->sample_point_y);
    }
}

static void
ligma_sample_point_tool_start (LigmaTool        *parent_tool,
                              LigmaDisplay     *display,
                              LigmaSamplePoint *sample_point)
{
  LigmaSamplePointTool *sp_tool;
  LigmaTool            *tool;

  sp_tool = g_object_new (LIGMA_TYPE_SAMPLE_POINT_TOOL,
                          "tool-info", parent_tool->tool_info,
                          NULL);

  tool = LIGMA_TOOL (sp_tool);

  ligma_display_shell_selection_pause (ligma_display_get_shell (display));

  if (sample_point)
    {
      sp_tool->sample_point = sample_point;

      ligma_sample_point_get_position (sample_point,
                                      &sp_tool->sample_point_old_x,
                                      &sp_tool->sample_point_old_y);

      sp_tool->sample_point_x = sp_tool->sample_point_old_x;
      sp_tool->sample_point_y = sp_tool->sample_point_old_y;
    }
  else
    {
      sp_tool->sample_point       = NULL;
      sp_tool->sample_point_old_x = 0;
      sp_tool->sample_point_old_y = 0;
      sp_tool->sample_point_x     = LIGMA_SAMPLE_POINT_POSITION_UNDEFINED;
      sp_tool->sample_point_y     = LIGMA_SAMPLE_POINT_POSITION_UNDEFINED;
    }

  ligma_tool_set_cursor (tool, display,
                        LIGMA_CURSOR_MOUSE,
                        LIGMA_TOOL_CURSOR_COLOR_PICKER,
                        LIGMA_CURSOR_MODIFIER_MOVE);

  tool_manager_push_tool (display->ligma, tool);

  tool->display = display;
  ligma_tool_control_activate (tool->control);

  ligma_draw_tool_start (LIGMA_DRAW_TOOL (sp_tool), display);

  if (sp_tool->sample_point)
    {
      ligma_tool_push_status_coords (tool, display,
                                    ligma_tool_control_get_precision (tool->control),
                                    _("Move Sample Point: "),
                                    sp_tool->sample_point_x -
                                    sp_tool->sample_point_old_x,
                                    ", ",
                                    sp_tool->sample_point_y -
                                    sp_tool->sample_point_old_y,
                                    NULL);
    }
  else
    {
      ligma_tool_push_status_coords (tool, display,
                                    ligma_tool_control_get_precision (tool->control),
                                    _("Add Sample Point: "),
                                    sp_tool->sample_point_x,
                                    ", ",
                                    sp_tool->sample_point_y,
                                    NULL);
    }
}


/*  public functions  */

void
ligma_sample_point_tool_start_new (LigmaTool    *parent_tool,
                                  LigmaDisplay *display)
{
  g_return_if_fail (LIGMA_IS_TOOL (parent_tool));
  g_return_if_fail (LIGMA_IS_DISPLAY (display));

  ligma_sample_point_tool_start (parent_tool, display, NULL);
}

void
ligma_sample_point_tool_start_edit (LigmaTool        *parent_tool,
                                   LigmaDisplay     *display,
                                   LigmaSamplePoint *sample_point)
{
  g_return_if_fail (LIGMA_IS_TOOL (parent_tool));
  g_return_if_fail (LIGMA_IS_DISPLAY (display));
  g_return_if_fail (LIGMA_IS_SAMPLE_POINT (sample_point));

  ligma_sample_point_tool_start (parent_tool, display, sample_point);
}
