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

#include "libgimpmath/gimpmath.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpsamplepoint.h"
#include "core/gimpimage.h"
#include "core/gimpimage-sample-points.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-selection.h"
#include "display/gimpdisplayshell-transform.h"

#include "gimpsamplepointtool.h"
#include "gimptoolcontrol.h"
#include "tool_manager.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_sample_point_tool_button_release (GimpTool              *tool,
                                                     const GimpCoords      *coords,
                                                     guint32                time,
                                                     GdkModifierType        state,
                                                     GimpButtonReleaseType  release_type,
                                                     GimpDisplay           *display);
static void   gimp_sample_point_tool_motion         (GimpTool              *tool,
                                                     const GimpCoords      *coords,
                                                     guint32                time,
                                                     GdkModifierType        state,
                                                     GimpDisplay           *display);

static void   gimp_sample_point_tool_draw           (GimpDrawTool          *draw_tool);

static void   gimp_sample_point_tool_start          (GimpTool              *parent_tool,
                                                     GimpDisplay           *display,
                                                     GimpSamplePoint       *sample_point);


G_DEFINE_TYPE (GimpSamplePointTool, gimp_sample_point_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_sample_point_tool_parent_class


static void
gimp_sample_point_tool_class_init (GimpSamplePointToolClass *klass)
{
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  tool_class->button_release = gimp_sample_point_tool_button_release;
  tool_class->motion         = gimp_sample_point_tool_motion;

  draw_tool_class->draw      = gimp_sample_point_tool_draw;
}

static void
gimp_sample_point_tool_init (GimpSamplePointTool *sp_tool)
{
  GimpTool *tool = GIMP_TOOL (sp_tool);

  gimp_tool_control_set_snap_to            (tool->control, FALSE);
  gimp_tool_control_set_handle_empty_image (tool->control, TRUE);
  gimp_tool_control_set_tool_cursor        (tool->control,
                                            GIMP_TOOL_CURSOR_MOVE);
  gimp_tool_control_set_scroll_lock        (tool->control, TRUE);
  gimp_tool_control_set_precision          (tool->control,
                                            GIMP_CURSOR_PRECISION_PIXEL_CENTER);

  sp_tool->sample_point   = NULL;
  sp_tool->sample_point_x = GIMP_SAMPLE_POINT_POSITION_UNDEFINED;
  sp_tool->sample_point_y = GIMP_SAMPLE_POINT_POSITION_UNDEFINED;
}

static void
gimp_sample_point_tool_button_release (GimpTool              *tool,
                                       const GimpCoords      *coords,
                                       guint32                time,
                                       GdkModifierType        state,
                                       GimpButtonReleaseType  release_type,
                                       GimpDisplay           *display)
{
  GimpSamplePointTool *sp_tool = GIMP_SAMPLE_POINT_TOOL (tool);
  GimpDisplayShell    *shell   = gimp_display_get_shell (display);
  GimpImage           *image   = gimp_display_get_image (display);

  gimp_tool_pop_status (tool, display);

  gimp_tool_control_halt (tool->control);

  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  if (release_type != GIMP_BUTTON_RELEASE_CANCEL)
    {
      gint width  = gimp_image_get_width  (image);
      gint height = gimp_image_get_height (image);

      if (sp_tool->sample_point_x == GIMP_SAMPLE_POINT_POSITION_UNDEFINED ||
          sp_tool->sample_point_x <  0                                    ||
          sp_tool->sample_point_x >= width                                ||
          sp_tool->sample_point_y == GIMP_SAMPLE_POINT_POSITION_UNDEFINED ||
          sp_tool->sample_point_y <  0                                    ||
          sp_tool->sample_point_y >= height)
        {
          if (sp_tool->sample_point)
            {
              gimp_image_remove_sample_point (image,
                                              sp_tool->sample_point, TRUE);
              sp_tool->sample_point = NULL;
            }
        }
      else
        {
          if (sp_tool->sample_point)
            {
              gimp_image_move_sample_point (image,
                                            sp_tool->sample_point,
                                            sp_tool->sample_point_x,
                                            sp_tool->sample_point_y,
                                            TRUE);
            }
          else
            {
              sp_tool->sample_point =
                gimp_image_add_sample_point_at_pos (image,
                                                    sp_tool->sample_point_x,
                                                    sp_tool->sample_point_y,
                                                    TRUE);
            }
        }

      gimp_image_flush (image);
    }

  gimp_display_shell_selection_resume (shell);

  sp_tool->sample_point_x = GIMP_SAMPLE_POINT_POSITION_UNDEFINED;
  sp_tool->sample_point_y = GIMP_SAMPLE_POINT_POSITION_UNDEFINED;

  tool_manager_pop_tool (display->gimp);
  g_object_unref (sp_tool);

  {
    GimpTool *active_tool = tool_manager_get_active (display->gimp);

    if (GIMP_IS_DRAW_TOOL (active_tool))
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (active_tool));

    tool_manager_oper_update_active (display->gimp, coords, state,
                                     TRUE, display);
    tool_manager_cursor_update_active (display->gimp, coords, state,
                                       display);

    if (GIMP_IS_DRAW_TOOL (active_tool))
      gimp_draw_tool_resume (GIMP_DRAW_TOOL (active_tool));
  }
}

static void
gimp_sample_point_tool_motion (GimpTool         *tool,
                               const GimpCoords *coords,
                               guint32           time,
                               GdkModifierType   state,
                               GimpDisplay      *display)

{
  GimpSamplePointTool *sp_tool      = GIMP_SAMPLE_POINT_TOOL (tool);
  GimpDisplayShell    *shell        = gimp_display_get_shell (display);
  gboolean             delete_point = FALSE;
  gint                 tx, ty;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  gimp_display_shell_transform_xy (shell,
                                   coords->x, coords->y,
                                   &tx, &ty);

  if (tx < 0 || tx >= shell->disp_width ||
      ty < 0 || ty >= shell->disp_height)
    {
      sp_tool->sample_point_x = GIMP_SAMPLE_POINT_POSITION_UNDEFINED;
      sp_tool->sample_point_y = GIMP_SAMPLE_POINT_POSITION_UNDEFINED;

      delete_point = TRUE;
    }
  else
    {
      GimpImage *image  = gimp_display_get_image (display);
      gint       width  = gimp_image_get_width  (image);
      gint       height = gimp_image_get_height (image);

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

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

  gimp_tool_pop_status (tool, display);

  if (delete_point)
    {
      gimp_tool_push_status (tool, display,
                             sp_tool->sample_point ?
                             _("Remove Sample Point") :
                             _("Cancel Sample Point"));
    }
  else if (sp_tool->sample_point)
    {
      gimp_tool_push_status_coords (tool, display,
                                    gimp_tool_control_get_precision (tool->control),
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
      gimp_tool_push_status_coords (tool, display,
                                    gimp_tool_control_get_precision (tool->control),
                                    _("Add Sample Point: "),
                                    sp_tool->sample_point_x,
                                    ", ",
                                    sp_tool->sample_point_y,
                                    NULL);
    }
}

static void
gimp_sample_point_tool_draw (GimpDrawTool *draw_tool)
{
  GimpSamplePointTool *sp_tool = GIMP_SAMPLE_POINT_TOOL (draw_tool);

  if (sp_tool->sample_point_x != GIMP_SAMPLE_POINT_POSITION_UNDEFINED &&
      sp_tool->sample_point_y != GIMP_SAMPLE_POINT_POSITION_UNDEFINED)
    {
      gimp_draw_tool_add_crosshair (draw_tool,
                                    sp_tool->sample_point_x,
                                    sp_tool->sample_point_y);
    }
}

static void
gimp_sample_point_tool_start (GimpTool        *parent_tool,
                              GimpDisplay     *display,
                              GimpSamplePoint *sample_point)
{
  GimpSamplePointTool *sp_tool;
  GimpTool            *tool;

  sp_tool = g_object_new (GIMP_TYPE_SAMPLE_POINT_TOOL,
                          "tool-info", parent_tool->tool_info,
                          NULL);

  tool = GIMP_TOOL (sp_tool);

  gimp_display_shell_selection_pause (gimp_display_get_shell (display));

  if (sample_point)
    {
      sp_tool->sample_point = sample_point;

      gimp_sample_point_get_position (sample_point,
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
      sp_tool->sample_point_x     = GIMP_SAMPLE_POINT_POSITION_UNDEFINED;
      sp_tool->sample_point_y     = GIMP_SAMPLE_POINT_POSITION_UNDEFINED;
    }

  gimp_tool_set_cursor (tool, display,
                        GIMP_CURSOR_MOUSE,
                        GIMP_TOOL_CURSOR_COLOR_PICKER,
                        GIMP_CURSOR_MODIFIER_MOVE);

  tool_manager_push_tool (display->gimp, tool);

  tool->display = display;
  gimp_tool_control_activate (tool->control);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (sp_tool), display);

  if (sp_tool->sample_point)
    {
      gimp_tool_push_status_coords (tool, display,
                                    gimp_tool_control_get_precision (tool->control),
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
      gimp_tool_push_status_coords (tool, display,
                                    gimp_tool_control_get_precision (tool->control),
                                    _("Add Sample Point: "),
                                    sp_tool->sample_point_x,
                                    ", ",
                                    sp_tool->sample_point_y,
                                    NULL);
    }
}


/*  public functions  */

void
gimp_sample_point_tool_start_new (GimpTool    *parent_tool,
                                  GimpDisplay *display)
{
  g_return_if_fail (GIMP_IS_TOOL (parent_tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));

  gimp_sample_point_tool_start (parent_tool, display, NULL);
}

void
gimp_sample_point_tool_start_edit (GimpTool        *parent_tool,
                                   GimpDisplay     *display,
                                   GimpSamplePoint *sample_point)
{
  g_return_if_fail (GIMP_IS_TOOL (parent_tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));
  g_return_if_fail (GIMP_IS_SAMPLE_POINT (sample_point));

  gimp_sample_point_tool_start (parent_tool, display, sample_point);
}
