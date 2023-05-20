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
#include "core/gimpguide.h"
#include "core/gimpimage.h"
#include "core/gimpimage-guides.h"
#include "core/gimpimage-undo.h"

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

static void   gimp_guide_tool_finalize       (GObject               *object);

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
                                              GList                 *guides,
                                              GimpOrientationType    orientation);

static void   gimp_guide_tool_push_status    (GimpGuideTool         *guide_tool,
                                              GimpDisplay           *display,
                                              gboolean               remove_guides);


G_DEFINE_TYPE (GimpGuideTool, gimp_guide_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_guide_tool_parent_class


static void
gimp_guide_tool_class_init (GimpGuideToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  object_class->finalize     = gimp_guide_tool_finalize;

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

  guide_tool->guides   = NULL;
  guide_tool->n_guides = 0;
}

static void
gimp_guide_tool_finalize (GObject *object)
{
  GimpGuideTool *guide_tool = GIMP_GUIDE_TOOL (object);
  gint           i;

  for (i = 0; i < guide_tool->n_guides; i++)
    g_clear_object (&guide_tool->guides[i].guide);

  g_free (guide_tool->guides);

  G_OBJECT_CLASS (parent_class)->finalize (object);
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
  gint              i;

  gimp_tool_pop_status (tool, display);

  gimp_tool_control_halt (tool->control);

  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
    {
      for (i = 0; i < guide_tool->n_guides; i++)
        {
          GimpGuideToolGuide *guide = &guide_tool->guides[i];

          /* custom guides are moved live */
          if (guide->custom)
            {
              gimp_image_move_guide (image, guide->guide, guide->old_position,
                                     TRUE);
            }
        }
    }
  else
    {
      gint     n_non_custom_guides = 0;
      gboolean remove_guides       = FALSE;

      for (i = 0; i < guide_tool->n_guides; i++)
        {
          GimpGuideToolGuide *guide = &guide_tool->guides[i];

          n_non_custom_guides += ! guide->custom;

          if (guide->position == GIMP_GUIDE_POSITION_UNDEFINED)
            {
              remove_guides = TRUE;
            }
        }

      if (n_non_custom_guides > 1)
        {
          gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_GUIDE,
                                       remove_guides ?
                                         C_("undo-type", "Remove Guides") :
                                         C_("undo-type", "Move Guides"));
        }

      for (i = 0; i < guide_tool->n_guides; i++)
        {
          GimpGuideToolGuide *guide = &guide_tool->guides[i];

          if (remove_guides)
            {
              /* removing a guide can cause other guides to be removed as well
               * (in particular, in case of symmetry guides).  these guides
               * will be kept alive, since we hold a reference on them, but we
               * need to make sure that they're still part of the image.
               */
              if (g_list_find (gimp_image_get_guides (image), guide->guide))
                gimp_image_remove_guide (image, guide->guide, TRUE);
            }
          else
            {
              if (guide->guide)
                {
                  /* custom guides are moved live */
                  if (! guide->custom)
                    {
                      gimp_image_move_guide (image, guide->guide,
                                             guide->position, TRUE);
                    }
                }
              else
                {
                  switch (guide->orientation)
                    {
                    case GIMP_ORIENTATION_HORIZONTAL:
                      gimp_image_add_hguide (image,
                                             guide->position,
                                             TRUE);
                      break;

                    case GIMP_ORIENTATION_VERTICAL:
                      gimp_image_add_vguide (image,
                                             guide->position,
                                             TRUE);
                      break;

                    default:
                      gimp_assert_not_reached ();
                    }
                }
            }
        }

      if (n_non_custom_guides > 1)
        gimp_image_undo_group_end (image);

      gimp_image_flush (image);
    }

  gimp_display_shell_selection_resume (shell);

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
  GimpGuideTool    *guide_tool    = GIMP_GUIDE_TOOL (tool);
  GimpDisplayShell *shell         = gimp_display_get_shell (display);
  GimpImage        *image         = gimp_display_get_image (display);
  gboolean          remove_guides = FALSE;
  gint              tx, ty;
  gint              i;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  gimp_display_shell_transform_xy (shell,
                                   coords->x, coords->y,
                                   &tx, &ty);

  for (i = 0; i < guide_tool->n_guides; i++)
    {
      GimpGuideToolGuide *guide = &guide_tool->guides[i];
      gint                max_position;
      gint                position;

      if (guide->orientation == GIMP_ORIENTATION_HORIZONTAL)
        max_position = gimp_image_get_height (image);
      else
        max_position = gimp_image_get_width (image);

      if (guide->orientation == GIMP_ORIENTATION_HORIZONTAL)
        guide->position = RINT (coords->y);
      else
        guide->position = RINT (coords->x);

      position = CLAMP (guide->position, 0, max_position);

      if (tx < 0 || tx >= shell->disp_width ||
          ty < 0 || ty >= shell->disp_height)
        {
          guide->position = GIMP_GUIDE_POSITION_UNDEFINED;

          remove_guides = TRUE;
        }
      else
        {
          if (guide->position < 0 || guide->position > max_position)
            remove_guides = TRUE;
        }

      /* custom guides are moved live */
      if (guide->custom)
        gimp_image_move_guide (image, guide->guide, position, TRUE);
    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

  gimp_tool_pop_status (tool, display);

  gimp_guide_tool_push_status (guide_tool, display, remove_guides);
}

static void
gimp_guide_tool_draw (GimpDrawTool *draw_tool)
{
  GimpGuideTool *guide_tool = GIMP_GUIDE_TOOL (draw_tool);
  gint           i;

  for (i = 0; i < guide_tool->n_guides; i++)
    {
      GimpGuideToolGuide *guide = &guide_tool->guides[i];

      if (guide->position != GIMP_GUIDE_POSITION_UNDEFINED)
        {
          /* custom guides are moved live */
          if (! guide->custom)
            {
              gimp_draw_tool_add_guide (draw_tool,
                                        guide->orientation,
                                        guide->position,
                                        GIMP_GUIDE_STYLE_NONE);
            }
        }
    }
}

static void
gimp_guide_tool_start (GimpTool            *parent_tool,
                       GimpDisplay         *display,
                       GList               *guides,
                       GimpOrientationType  orientation)
{
  GimpGuideTool *guide_tool;
  GimpTool      *tool;

  guide_tool = g_object_new (GIMP_TYPE_GUIDE_TOOL,
                             "tool-info", parent_tool->tool_info,
                             NULL);

  tool = GIMP_TOOL (guide_tool);

  gimp_display_shell_selection_pause (gimp_display_get_shell (display));

  if (guides)
    {
      gint i;

      guide_tool->n_guides = g_list_length (guides);
      guide_tool->guides   = g_new (GimpGuideToolGuide, guide_tool->n_guides);

      for (i = 0; i < guide_tool->n_guides; i++)
        {
          GimpGuide *guide = guides->data;

          guide_tool->guides[i].guide        = g_object_ref (guide);
          guide_tool->guides[i].old_position = gimp_guide_get_position (guide);
          guide_tool->guides[i].position     = gimp_guide_get_position (guide);
          guide_tool->guides[i].orientation  = gimp_guide_get_orientation (guide);
          guide_tool->guides[i].custom       = gimp_guide_is_custom (guide);

          guides = g_list_next (guides);
        }
    }
  else
    {
      guide_tool->n_guides = 1;
      guide_tool->guides   = g_new (GimpGuideToolGuide, 1);

      guide_tool->guides[0].guide        = NULL;
      guide_tool->guides[0].old_position = 0;
      guide_tool->guides[0].position     = GIMP_GUIDE_POSITION_UNDEFINED;
      guide_tool->guides[0].orientation  = orientation;
      guide_tool->guides[0].custom       = FALSE;
    }

  gimp_tool_set_cursor (tool, display,
                        GIMP_CURSOR_MOUSE,
                        GIMP_TOOL_CURSOR_HAND,
                        GIMP_CURSOR_MODIFIER_MOVE);

  tool_manager_push_tool (display->gimp, tool);

  tool->display = display;
  gimp_tool_control_activate (tool->control);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (guide_tool), display);

  gimp_guide_tool_push_status (guide_tool, display, FALSE);
}

static void
gimp_guide_tool_push_status (GimpGuideTool *guide_tool,
                             GimpDisplay   *display,
                             gboolean       remove_guides)
{
  GimpTool *tool = GIMP_TOOL (guide_tool);

  if (remove_guides)
    {
      gimp_tool_push_status (tool, display,
                             guide_tool->n_guides > 1    ? _("Remove Guides") :
                             guide_tool->guides[0].guide ? _("Remove Guide")  :
                                                           _("Cancel Guide"));
    }
  else
    {
      GimpGuideToolGuide *guides[2] = { 0, };
      gint                n_guides = 0;
      gint                i;

      for (i = 0; i < guide_tool->n_guides; i++)
        {
          GimpGuideToolGuide *guide = &guide_tool->guides[i];

          if (guide_tool->guides[i].guide)
            {
              if (n_guides == 0 || guide->orientation != guides[0]->orientation)
                {
                  guides[n_guides++] = guide;

                  if (n_guides == 2)
                    break;
                }
            }
        }

      if (n_guides == 2 &&
          guides[0]->orientation == GIMP_ORIENTATION_HORIZONTAL)
        {
          GimpGuideToolGuide *temp;

          temp      = guides[0];
          guides[0] = guides[1];
          guides[1] = temp;
        }

       if (n_guides == 1)
        {
          gimp_tool_push_status_length (tool, display,
                                        _("Move Guide: "),
                                        SWAP_ORIENT (guides[0]->orientation),
                                        guides[0]->position -
                                        guides[0]->old_position,
                                        NULL);
        }
      else if (n_guides == 2)
        {
          gimp_tool_push_status_coords (tool, display,
                                        GIMP_CURSOR_PRECISION_PIXEL_BORDER,
                                        _("Move Guides: "),
                                        guides[0]->position -
                                        guides[0]->old_position,
                                        ", ",
                                        guides[1]->position -
                                        guides[1]->old_position,
                                        NULL);
        }
      else
        {
          gimp_tool_push_status_length (tool, display,
                                        _("Add Guide: "),
                                        SWAP_ORIENT (guide_tool->guides[0].orientation),
                                        guide_tool->guides[0].position,
                                        NULL);
        }
    }
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
  GList *guides = NULL;

  g_return_if_fail (GIMP_IS_TOOL (parent_tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));
  g_return_if_fail (GIMP_IS_GUIDE (guide));

  guides = g_list_append (guides, guide);

  gimp_guide_tool_start (parent_tool, display,
                         guides, GIMP_ORIENTATION_UNKNOWN);

  g_list_free (guides);
}

void
gimp_guide_tool_start_edit_many (GimpTool    *parent_tool,
                                 GimpDisplay *display,
                                 GList       *guides)
{
  g_return_if_fail (GIMP_IS_TOOL (parent_tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));
  g_return_if_fail (guides != NULL);

  gimp_guide_tool_start (parent_tool, display,
                         guides, GIMP_ORIENTATION_UNKNOWN);
}
