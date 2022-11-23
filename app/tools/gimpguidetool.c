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
#include "core/ligmaguide.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-guides.h"
#include "core/ligmaimage-undo.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmadisplayshell-selection.h"
#include "display/ligmadisplayshell-transform.h"

#include "ligmaguidetool.h"
#include "ligmatoolcontrol.h"
#include "tool_manager.h"

#include "ligma-intl.h"


#define SWAP_ORIENT(orient) ((orient) == LIGMA_ORIENTATION_HORIZONTAL ? \
                             LIGMA_ORIENTATION_VERTICAL : \
                             LIGMA_ORIENTATION_HORIZONTAL)


/*  local function prototypes  */

static void   ligma_guide_tool_finalize       (GObject               *object);

static void   ligma_guide_tool_button_release (LigmaTool              *tool,
                                              const LigmaCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              LigmaButtonReleaseType  release_type,
                                              LigmaDisplay           *display);
static void   ligma_guide_tool_motion         (LigmaTool              *tool,
                                              const LigmaCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              LigmaDisplay           *display);

static void   ligma_guide_tool_draw           (LigmaDrawTool          *draw_tool);

static void   ligma_guide_tool_start          (LigmaTool              *parent_tool,
                                              LigmaDisplay           *display,
                                              GList                 *guides,
                                              LigmaOrientationType    orientation);

static void   ligma_guide_tool_push_status    (LigmaGuideTool         *guide_tool,
                                              LigmaDisplay           *display,
                                              gboolean               remove_guides);


G_DEFINE_TYPE (LigmaGuideTool, ligma_guide_tool, LIGMA_TYPE_DRAW_TOOL)

#define parent_class ligma_guide_tool_parent_class


static void
ligma_guide_tool_class_init (LigmaGuideToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  LigmaToolClass     *tool_class      = LIGMA_TOOL_CLASS (klass);
  LigmaDrawToolClass *draw_tool_class = LIGMA_DRAW_TOOL_CLASS (klass);

  object_class->finalize     = ligma_guide_tool_finalize;

  tool_class->button_release = ligma_guide_tool_button_release;
  tool_class->motion         = ligma_guide_tool_motion;

  draw_tool_class->draw      = ligma_guide_tool_draw;
}

static void
ligma_guide_tool_init (LigmaGuideTool *guide_tool)
{
  LigmaTool *tool = LIGMA_TOOL (guide_tool);

  ligma_tool_control_set_snap_to            (tool->control, FALSE);
  ligma_tool_control_set_handle_empty_image (tool->control, TRUE);
  ligma_tool_control_set_tool_cursor        (tool->control,
                                            LIGMA_TOOL_CURSOR_MOVE);
  ligma_tool_control_set_scroll_lock        (tool->control, TRUE);
  ligma_tool_control_set_precision          (tool->control,
                                            LIGMA_CURSOR_PRECISION_PIXEL_BORDER);

  guide_tool->guides   = NULL;
  guide_tool->n_guides = 0;
}

static void
ligma_guide_tool_finalize (GObject *object)
{
  LigmaGuideTool *guide_tool = LIGMA_GUIDE_TOOL (object);
  gint           i;

  for (i = 0; i < guide_tool->n_guides; i++)
    g_clear_object (&guide_tool->guides[i].guide);

  g_free (guide_tool->guides);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_guide_tool_button_release (LigmaTool              *tool,
                                const LigmaCoords      *coords,
                                guint32                time,
                                GdkModifierType        state,
                                LigmaButtonReleaseType  release_type,
                                LigmaDisplay           *display)
{
  LigmaGuideTool    *guide_tool = LIGMA_GUIDE_TOOL (tool);
  LigmaDisplayShell *shell      = ligma_display_get_shell (display);
  LigmaImage        *image      = ligma_display_get_image (display);
  gint              i;

  ligma_tool_pop_status (tool, display);

  ligma_tool_control_halt (tool->control);

  ligma_draw_tool_stop (LIGMA_DRAW_TOOL (tool));

  if (release_type == LIGMA_BUTTON_RELEASE_CANCEL)
    {
      for (i = 0; i < guide_tool->n_guides; i++)
        {
          LigmaGuideToolGuide *guide = &guide_tool->guides[i];

          /* custom guides are moved live */
          if (guide->custom)
            {
              ligma_image_move_guide (image, guide->guide, guide->old_position,
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
          LigmaGuideToolGuide *guide = &guide_tool->guides[i];

          n_non_custom_guides += ! guide->custom;

          if (guide->position == LIGMA_GUIDE_POSITION_UNDEFINED)
            {
              remove_guides = TRUE;
            }
        }

      if (n_non_custom_guides > 1)
        {
          ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_GUIDE,
                                       remove_guides ?
                                         C_("undo-type", "Remove Guides") :
                                         C_("undo-type", "Move Guides"));
        }

      for (i = 0; i < guide_tool->n_guides; i++)
        {
          LigmaGuideToolGuide *guide = &guide_tool->guides[i];

          if (remove_guides)
            {
              /* removing a guide can cause other guides to be removed as well
               * (in particular, in case of symmetry guides).  these guides
               * will be kept alive, since we hold a reference on them, but we
               * need to make sure that they're still part of the image.
               */
              if (g_list_find (ligma_image_get_guides (image), guide->guide))
                ligma_image_remove_guide (image, guide->guide, TRUE);
            }
          else
            {
              if (guide->guide)
                {
                  /* custom guides are moved live */
                  if (! guide->custom)
                    {
                      ligma_image_move_guide (image, guide->guide,
                                             guide->position, TRUE);
                    }
                }
              else
                {
                  switch (guide->orientation)
                    {
                    case LIGMA_ORIENTATION_HORIZONTAL:
                      ligma_image_add_hguide (image,
                                             guide->position,
                                             TRUE);
                      break;

                    case LIGMA_ORIENTATION_VERTICAL:
                      ligma_image_add_vguide (image,
                                             guide->position,
                                             TRUE);
                      break;

                    default:
                      ligma_assert_not_reached ();
                    }
                }
            }
        }

      if (n_non_custom_guides > 1)
        ligma_image_undo_group_end (image);

      ligma_image_flush (image);
    }

  ligma_display_shell_selection_resume (shell);

  tool_manager_pop_tool (display->ligma);
  g_object_unref (guide_tool);

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
ligma_guide_tool_motion (LigmaTool         *tool,
                        const LigmaCoords *coords,
                        guint32           time,
                        GdkModifierType   state,
                        LigmaDisplay      *display)

{
  LigmaGuideTool    *guide_tool    = LIGMA_GUIDE_TOOL (tool);
  LigmaDisplayShell *shell         = ligma_display_get_shell (display);
  LigmaImage        *image         = ligma_display_get_image (display);
  gboolean          remove_guides = FALSE;
  gint              tx, ty;
  gint              i;

  ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

  ligma_display_shell_transform_xy (shell,
                                   coords->x, coords->y,
                                   &tx, &ty);

  for (i = 0; i < guide_tool->n_guides; i++)
    {
      LigmaGuideToolGuide *guide = &guide_tool->guides[i];
      gint                max_position;
      gint                position;

      if (guide->orientation == LIGMA_ORIENTATION_HORIZONTAL)
        max_position = ligma_image_get_height (image);
      else
        max_position = ligma_image_get_width (image);

      if (guide->orientation == LIGMA_ORIENTATION_HORIZONTAL)
        guide->position = RINT (coords->y);
      else
        guide->position = RINT (coords->x);

      position = CLAMP (guide->position, 0, max_position);

      if (tx < 0 || tx >= shell->disp_width ||
          ty < 0 || ty >= shell->disp_height)
        {
          guide->position = LIGMA_GUIDE_POSITION_UNDEFINED;

          remove_guides = TRUE;
        }
      else
        {
          if (guide->position < 0 || guide->position > max_position)
            remove_guides = TRUE;
        }

      /* custom guides are moved live */
      if (guide->custom)
        ligma_image_move_guide (image, guide->guide, position, TRUE);
    }

  ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));

  ligma_tool_pop_status (tool, display);

  ligma_guide_tool_push_status (guide_tool, display, remove_guides);
}

static void
ligma_guide_tool_draw (LigmaDrawTool *draw_tool)
{
  LigmaGuideTool *guide_tool = LIGMA_GUIDE_TOOL (draw_tool);
  gint           i;

  for (i = 0; i < guide_tool->n_guides; i++)
    {
      LigmaGuideToolGuide *guide = &guide_tool->guides[i];

      if (guide->position != LIGMA_GUIDE_POSITION_UNDEFINED)
        {
          /* custom guides are moved live */
          if (! guide->custom)
            {
              ligma_draw_tool_add_guide (draw_tool,
                                        guide->orientation,
                                        guide->position,
                                        LIGMA_GUIDE_STYLE_NONE);
            }
        }
    }
}

static void
ligma_guide_tool_start (LigmaTool            *parent_tool,
                       LigmaDisplay         *display,
                       GList               *guides,
                       LigmaOrientationType  orientation)
{
  LigmaGuideTool *guide_tool;
  LigmaTool      *tool;

  guide_tool = g_object_new (LIGMA_TYPE_GUIDE_TOOL,
                             "tool-info", parent_tool->tool_info,
                             NULL);

  tool = LIGMA_TOOL (guide_tool);

  ligma_display_shell_selection_pause (ligma_display_get_shell (display));

  if (guides)
    {
      gint i;

      guide_tool->n_guides = g_list_length (guides);
      guide_tool->guides   = g_new (LigmaGuideToolGuide, guide_tool->n_guides);

      for (i = 0; i < guide_tool->n_guides; i++)
        {
          LigmaGuide *guide = guides->data;

          guide_tool->guides[i].guide        = g_object_ref (guide);
          guide_tool->guides[i].old_position = ligma_guide_get_position (guide);
          guide_tool->guides[i].position     = ligma_guide_get_position (guide);
          guide_tool->guides[i].orientation  = ligma_guide_get_orientation (guide);
          guide_tool->guides[i].custom       = ligma_guide_is_custom (guide);

          guides = g_list_next (guides);
        }
    }
  else
    {
      guide_tool->n_guides = 1;
      guide_tool->guides   = g_new (LigmaGuideToolGuide, 1);

      guide_tool->guides[0].guide        = NULL;
      guide_tool->guides[0].old_position = 0;
      guide_tool->guides[0].position     = LIGMA_GUIDE_POSITION_UNDEFINED;
      guide_tool->guides[0].orientation  = orientation;
      guide_tool->guides[0].custom       = FALSE;
    }

  ligma_tool_set_cursor (tool, display,
                        LIGMA_CURSOR_MOUSE,
                        LIGMA_TOOL_CURSOR_HAND,
                        LIGMA_CURSOR_MODIFIER_MOVE);

  tool_manager_push_tool (display->ligma, tool);

  tool->display = display;
  ligma_tool_control_activate (tool->control);

  ligma_draw_tool_start (LIGMA_DRAW_TOOL (guide_tool), display);

  ligma_guide_tool_push_status (guide_tool, display, FALSE);
}

static void
ligma_guide_tool_push_status (LigmaGuideTool *guide_tool,
                             LigmaDisplay   *display,
                             gboolean       remove_guides)
{
  LigmaTool *tool = LIGMA_TOOL (guide_tool);

  if (remove_guides)
    {
      ligma_tool_push_status (tool, display,
                             guide_tool->n_guides > 1    ? _("Remove Guides") :
                             guide_tool->guides[0].guide ? _("Remove Guide")  :
                                                           _("Cancel Guide"));
    }
  else
    {
      LigmaGuideToolGuide *guides[2];
      gint                n_guides = 0;
      gint                i;

      for (i = 0; i < guide_tool->n_guides; i++)
        {
          LigmaGuideToolGuide *guide = &guide_tool->guides[i];

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
          guides[0]->orientation == LIGMA_ORIENTATION_HORIZONTAL)
        {
          LigmaGuideToolGuide *temp;

          temp      = guides[0];
          guides[0] = guides[1];
          guides[1] = temp;
        }

       if (n_guides == 1)
        {
          ligma_tool_push_status_length (tool, display,
                                        _("Move Guide: "),
                                        SWAP_ORIENT (guides[0]->orientation),
                                        guides[0]->position -
                                        guides[0]->old_position,
                                        NULL);
        }
      else if (n_guides == 2)
        {
          ligma_tool_push_status_coords (tool, display,
                                        LIGMA_CURSOR_PRECISION_PIXEL_BORDER,
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
          ligma_tool_push_status_length (tool, display,
                                        _("Add Guide: "),
                                        SWAP_ORIENT (guide_tool->guides[0].orientation),
                                        guide_tool->guides[0].position,
                                        NULL);
        }
    }
}


/*  public functions  */

void
ligma_guide_tool_start_new (LigmaTool            *parent_tool,
                           LigmaDisplay         *display,
                           LigmaOrientationType  orientation)
{
  g_return_if_fail (LIGMA_IS_TOOL (parent_tool));
  g_return_if_fail (LIGMA_IS_DISPLAY (display));
  g_return_if_fail (orientation != LIGMA_ORIENTATION_UNKNOWN);

  ligma_guide_tool_start (parent_tool, display,
                         NULL, orientation);
}

void
ligma_guide_tool_start_edit (LigmaTool    *parent_tool,
                            LigmaDisplay *display,
                            LigmaGuide   *guide)
{
  GList *guides = NULL;

  g_return_if_fail (LIGMA_IS_TOOL (parent_tool));
  g_return_if_fail (LIGMA_IS_DISPLAY (display));
  g_return_if_fail (LIGMA_IS_GUIDE (guide));

  guides = g_list_append (guides, guide);

  ligma_guide_tool_start (parent_tool, display,
                         guides, LIGMA_ORIENTATION_UNKNOWN);

  g_list_free (guides);
}

void
ligma_guide_tool_start_edit_many (LigmaTool    *parent_tool,
                                 LigmaDisplay *display,
                                 GList       *guides)
{
  g_return_if_fail (LIGMA_IS_TOOL (parent_tool));
  g_return_if_fail (LIGMA_IS_DISPLAY (display));
  g_return_if_fail (guides != NULL);

  ligma_guide_tool_start (parent_tool, display,
                         guides, LIGMA_ORIENTATION_UNKNOWN);
}
