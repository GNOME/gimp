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

#include "libgimpmath/gimpmath.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpguide.h"
#include "core/gimpimage.h"
#include "core/gimpimage-guides.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-selection.h"
#include "display/gimpdisplayshell-transform.h"

#include "gimpguidetool.h"
#include "gimptoolcontrol.h"
#include "tool_manager.h"

#include "gimp-intl.h"


#define SWAP_ORIENT(orient) ((orient) == GIMP_ORIENTATION_HORIZONTAL ? \
                             GIMP_ORIENTATION_VERTICAL : \
                             GIMP_ORIENTATION_HORIZONTAL)


/*  local function prototypes  */

static void   gimp_guide_tool_button_release (GimpTool              *tool,
                                              const GimpCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              GimpButtonReleaseType  release_type,
                                              GimpDisplay           *display);
static void   gimp_guide_tool_motion         (GimpTool              *tool,
                                              const GimpCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              GimpDisplay           *display);

static void   gimp_guide_tool_draw           (GimpDrawTool          *draw_tool);

static void   gimp_guide_tool_start          (GimpTool              *parent_tool,
                                              GimpDisplay           *display,
                                              GimpGuide             *guide,
                                              GimpOrientationType    orientation);


G_DEFINE_TYPE (GimpGuideTool, gimp_guide_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_guide_tool_parent_class


static void
gimp_guide_tool_class_init (GimpGuideToolClass *klass)
{
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  tool_class->button_release = gimp_guide_tool_button_release;
  tool_class->motion         = gimp_guide_tool_motion;

  draw_tool_class->draw      = gimp_guide_tool_draw;
}

static void
gimp_guide_tool_init (GimpGuideTool *guide_tool)
{
  GimpTool *tool = GIMP_TOOL (guide_tool);

  gimp_tool_control_set_snap_to            (tool->control, FALSE);
  gimp_tool_control_set_handle_empty_image (tool->control, TRUE);
  gimp_tool_control_set_tool_cursor        (tool->control,
                                            GIMP_TOOL_CURSOR_MOVE);
  gimp_tool_control_set_scroll_lock        (tool->control, TRUE);
  gimp_tool_control_set_precision          (tool->control,
                                            GIMP_CURSOR_PRECISION_PIXEL_BORDER);

  guide_tool->guide             = NULL;
  guide_tool->guide_position    = GIMP_GUIDE_POSITION_UNDEFINED;
  guide_tool->guide_orientation = GIMP_ORIENTATION_UNKNOWN;
}

static void
gimp_guide_tool_button_release (GimpTool              *tool,
                                const GimpCoords      *coords,
                                guint32                time,
                                GdkModifierType        state,
                                GimpButtonReleaseType  release_type,
                                GimpDisplay           *display)
{
  GimpGuideTool    *guide_tool = GIMP_GUIDE_TOOL (tool);
  GimpDisplayShell *shell      = gimp_display_get_shell (display);
  GimpImage        *image      = gimp_display_get_image (display);

  gimp_tool_pop_status (tool, display);

  gimp_tool_control_halt (tool->control);

  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
    {
      if (guide_tool->guide_custom)
        gimp_image_move_guide (image, guide_tool->guide,
                               guide_tool->guide_old_position, TRUE);
    }
  else
    {
      gint max_position;

      if (guide_tool->guide_orientation == GIMP_ORIENTATION_HORIZONTAL)
        max_position = gimp_image_get_height (image);
      else
        max_position = gimp_image_get_width (image);

      if (guide_tool->guide_position == GIMP_GUIDE_POSITION_UNDEFINED ||
          guide_tool->guide_position <  0                             ||
          guide_tool->guide_position >= max_position)
        {
          if (guide_tool->guide)
            {
              gimp_image_remove_guide (image, guide_tool->guide, TRUE);
              guide_tool->guide = NULL;
            }
        }
      else
        {
          if (guide_tool->guide)
            {
              /* custom guides are moved live */
              if (! guide_tool->guide_custom)
                gimp_image_move_guide (image, guide_tool->guide,
                                       guide_tool->guide_position, TRUE);
            }
          else
            {
              switch (guide_tool->guide_orientation)
                {
                case GIMP_ORIENTATION_HORIZONTAL:
                  guide_tool->guide =
                    gimp_image_add_hguide (image,
                                           guide_tool->guide_position,
                                           TRUE);
                  break;

                case GIMP_ORIENTATION_VERTICAL:
                  guide_tool->guide =
                    gimp_image_add_vguide (image,
                                           guide_tool->guide_position,
                                           TRUE);
                  break;

                default:
                  g_assert_not_reached ();
                }
            }
        }

      gimp_image_flush (image);
    }

  gimp_display_shell_selection_resume (shell);

  guide_tool->guide_position    = GIMP_GUIDE_POSITION_UNDEFINED;
  guide_tool->guide_orientation = GIMP_ORIENTATION_UNKNOWN;

  tool_manager_pop_tool (display->gimp);
  g_object_unref (guide_tool);

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
gimp_guide_tool_motion (GimpTool         *tool,
                        const GimpCoords *coords,
                        guint32           time,
                        GdkModifierType   state,
                        GimpDisplay      *display)

{
  GimpGuideTool    *guide_tool   = GIMP_GUIDE_TOOL (tool);
  GimpDisplayShell *shell        = gimp_display_get_shell (display);
  GimpImage        *image        = gimp_display_get_image (display);
  gboolean          delete_guide = FALSE;
  gint              max_position;
  gint              tx, ty;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  gimp_display_shell_transform_xy (shell,
                                   coords->x, coords->y,
                                   &tx, &ty);

  if (guide_tool->guide_orientation == GIMP_ORIENTATION_HORIZONTAL)
    max_position = gimp_image_get_height (image);
  else
    max_position = gimp_image_get_width (image);

  if (tx < 0 || tx >= shell->disp_width ||
      ty < 0 || ty >= shell->disp_height)
    {
      guide_tool->guide_position = GIMP_GUIDE_POSITION_UNDEFINED;

      delete_guide = TRUE;
    }
  else
    {
      if (guide_tool->guide_orientation == GIMP_ORIENTATION_HORIZONTAL)
        guide_tool->guide_position = RINT (coords->y);
      else
        guide_tool->guide_position = RINT (coords->x);

      if (guide_tool->guide_position <  0 ||
          guide_tool->guide_position >= max_position)
        {
          delete_guide = TRUE;
        }
    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

  gimp_tool_pop_status (tool, display);

  /* custom guides are moved live */
  if (guide_tool->guide_custom &&
      guide_tool->guide_position != GIMP_GUIDE_POSITION_UNDEFINED)
    {
      gimp_image_move_guide (image, guide_tool->guide,
                             CLAMP (guide_tool->guide_position,
                                    0, max_position), TRUE);
    }

  if (delete_guide)
    {
      gimp_tool_push_status (tool, display,
                             guide_tool->guide ?
                             _("Remove Guide") : _("Cancel Guide"));
    }
  else
    {
      gimp_tool_push_status_length (tool, display,
                                    guide_tool->guide ?
                                    _("Move Guide: ") : _("Add Guide: "),
                                    SWAP_ORIENT (guide_tool->guide_orientation),
                                    guide_tool->guide_position,
                                    NULL);
    }
}

static void
gimp_guide_tool_draw (GimpDrawTool *draw_tool)
{
  GimpGuideTool *guide_tool = GIMP_GUIDE_TOOL (draw_tool);

  if (guide_tool->guide_position != GIMP_GUIDE_POSITION_UNDEFINED)
    {
      if (! guide_tool->guide_custom)
        gimp_draw_tool_add_guide (draw_tool,
                                  guide_tool->guide_orientation,
                                  guide_tool->guide_position,
                                  GIMP_GUIDE_STYLE_NONE);
    }
}

static void
gimp_guide_tool_start (GimpTool            *parent_tool,
                       GimpDisplay         *display,
                       GimpGuide           *guide,
                       GimpOrientationType  orientation)
{
  GimpGuideTool *guide_tool;
  GimpTool      *tool;

  guide_tool = g_object_new (GIMP_TYPE_GUIDE_TOOL,
                             "tool-info", parent_tool->tool_info,
                             NULL);

  tool = GIMP_TOOL (guide_tool);

  gimp_display_shell_selection_pause (gimp_display_get_shell (display));

  if (guide)
    {
      guide_tool->guide              = guide;
      guide_tool->guide_position     = gimp_guide_get_position (guide);
      guide_tool->guide_orientation  = gimp_guide_get_orientation (guide);
      guide_tool->guide_custom       = gimp_guide_is_custom (guide);
      guide_tool->guide_old_position = gimp_guide_get_position (guide);
    }
  else
    {
      guide_tool->guide              = NULL;
      guide_tool->guide_position     = GIMP_GUIDE_POSITION_UNDEFINED;
      guide_tool->guide_orientation  = orientation;
      guide_tool->guide_custom       = FALSE;
      guide_tool->guide_old_position = GIMP_GUIDE_POSITION_UNDEFINED;
    }

  gimp_tool_set_cursor (tool, display,
                        GIMP_CURSOR_MOUSE,
                        GIMP_TOOL_CURSOR_HAND,
                        GIMP_CURSOR_MODIFIER_MOVE);

  tool_manager_push_tool (display->gimp, tool);

  tool->display = display;
  gimp_tool_control_activate (tool->control);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (guide_tool), display);

  gimp_tool_push_status_length (tool, display,
                                _("Add Guide: "),
                                SWAP_ORIENT (guide_tool->guide_orientation),
                                guide_tool->guide_position,
                                NULL);
}


/*  public functions  */

void
gimp_guide_tool_start_new (GimpTool            *parent_tool,
                           GimpDisplay         *display,
                           GimpOrientationType  orientation)
{
  g_return_if_fail (GIMP_IS_TOOL (parent_tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));
  g_return_if_fail (orientation != GIMP_ORIENTATION_UNKNOWN);

  gimp_guide_tool_start (parent_tool, display,
                         NULL, orientation);
}

void
gimp_guide_tool_start_edit (GimpTool    *parent_tool,
                            GimpDisplay *display,
                            GimpGuide   *guide)
{
  g_return_if_fail (GIMP_IS_TOOL (parent_tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));
  g_return_if_fail (GIMP_IS_GUIDE (guide));

  gimp_guide_tool_start (parent_tool, display,
                         guide, GIMP_ORIENTATION_UNKNOWN);
}
