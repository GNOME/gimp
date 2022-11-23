/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaregionselecttool.c
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

#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "core/ligma-utils.h"
#include "core/ligmaboundary.h"
#include "core/ligmachannel.h"
#include "core/ligmachannel-select.h"
#include "core/ligmaimage.h"
#include "core/ligmalayer-floating-selection.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmadisplayshell-cursor.h"

#include "ligmaregionselectoptions.h"
#include "ligmaregionselecttool.h"
#include "ligmatoolcontrol.h"

#include "ligma-intl.h"


static void   ligma_region_select_tool_finalize       (GObject               *object);

static void   ligma_region_select_tool_button_press   (LigmaTool              *tool,
                                                      const LigmaCoords      *coords,
                                                      guint32                time,
                                                      GdkModifierType        state,
                                                      LigmaButtonPressType    press_type,
                                                      LigmaDisplay           *display);
static void   ligma_region_select_tool_button_release (LigmaTool              *tool,
                                                      const LigmaCoords      *coords,
                                                      guint32                time,
                                                      GdkModifierType        state,
                                                      LigmaButtonReleaseType  release_type,
                                                      LigmaDisplay           *display);
static void   ligma_region_select_tool_motion         (LigmaTool              *tool,
                                                      const LigmaCoords      *coords,
                                                      guint32                time,
                                                      GdkModifierType        state,
                                                      LigmaDisplay           *display);
static void   ligma_region_select_tool_cursor_update  (LigmaTool              *tool,
                                                      const LigmaCoords      *coords,
                                                      GdkModifierType        state,
                                                      LigmaDisplay           *display);

static void   ligma_region_select_tool_draw           (LigmaDrawTool          *draw_tool);

static void   ligma_region_select_tool_get_mask       (LigmaRegionSelectTool  *region_sel,
                                                      LigmaDisplay           *display);


G_DEFINE_TYPE (LigmaRegionSelectTool, ligma_region_select_tool,
               LIGMA_TYPE_SELECTION_TOOL)

#define parent_class ligma_region_select_tool_parent_class


static void
ligma_region_select_tool_class_init (LigmaRegionSelectToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  LigmaToolClass     *tool_class      = LIGMA_TOOL_CLASS (klass);
  LigmaDrawToolClass *draw_tool_class = LIGMA_DRAW_TOOL_CLASS (klass);

  object_class->finalize     = ligma_region_select_tool_finalize;

  tool_class->button_press   = ligma_region_select_tool_button_press;
  tool_class->button_release = ligma_region_select_tool_button_release;
  tool_class->motion         = ligma_region_select_tool_motion;
  tool_class->cursor_update  = ligma_region_select_tool_cursor_update;

  draw_tool_class->draw      = ligma_region_select_tool_draw;
}

static void
ligma_region_select_tool_init (LigmaRegionSelectTool *region_select)
{
  LigmaTool *tool = LIGMA_TOOL (region_select);

  ligma_tool_control_set_scroll_lock (tool->control, TRUE);

  region_select->x               = 0;
  region_select->y               = 0;
  region_select->saved_threshold = 0.0;

  region_select->region_mask     = NULL;
  region_select->segs            = NULL;
  region_select->n_segs          = 0;
}

static void
ligma_region_select_tool_finalize (GObject *object)
{
  LigmaRegionSelectTool *region_sel = LIGMA_REGION_SELECT_TOOL (object);

  g_clear_object (&region_sel->region_mask);

  g_clear_pointer (&region_sel->segs, g_free);
  region_sel->n_segs = 0;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_region_select_tool_button_press (LigmaTool            *tool,
                                      const LigmaCoords    *coords,
                                      guint32              time,
                                      GdkModifierType      state,
                                      LigmaButtonPressType  press_type,
                                      LigmaDisplay         *display)
{
  LigmaRegionSelectTool    *region_sel = LIGMA_REGION_SELECT_TOOL (tool);
  LigmaRegionSelectOptions *options    = LIGMA_REGION_SELECT_TOOL_GET_OPTIONS (tool);

  region_sel->x               = coords->x;
  region_sel->y               = coords->y;
  region_sel->saved_threshold = options->threshold;

  if (ligma_selection_tool_start_edit (LIGMA_SELECTION_TOOL (region_sel),
                                      display, coords))
    {
      return;
    }

  ligma_tool_control_activate (tool->control);
  tool->display = display;

  ligma_tool_push_status (tool, display,
                         _("Move the mouse to change threshold"));

  ligma_region_select_tool_get_mask (region_sel, display);

  ligma_draw_tool_start (LIGMA_DRAW_TOOL (tool), display);
}

static void
ligma_region_select_tool_button_release (LigmaTool              *tool,
                                        const LigmaCoords      *coords,
                                        guint32                time,
                                        GdkModifierType        state,
                                        LigmaButtonReleaseType  release_type,
                                        LigmaDisplay           *display)
{
  LigmaRegionSelectTool    *region_sel  = LIGMA_REGION_SELECT_TOOL (tool);
  LigmaSelectionOptions    *sel_options = LIGMA_SELECTION_TOOL_GET_OPTIONS (tool);
  LigmaRegionSelectOptions *options     = LIGMA_REGION_SELECT_TOOL_GET_OPTIONS (tool);
  LigmaImage               *image       = ligma_display_get_image (display);

  ligma_tool_pop_status (tool, display);

  ligma_draw_tool_stop (LIGMA_DRAW_TOOL (tool));

  ligma_tool_control_halt (tool->control);

  if (options->draw_mask)
    ligma_display_shell_set_mask (ligma_display_get_shell (display),
                                 NULL, 0, 0, NULL, FALSE);

  if (release_type != LIGMA_BUTTON_RELEASE_CANCEL)
    {
      if (LIGMA_SELECTION_TOOL (tool)->function == SELECTION_ANCHOR)
        {
          if (ligma_image_get_floating_selection (image))
            {
              /*  If there is a floating selection, anchor it  */
              floating_sel_anchor (ligma_image_get_floating_selection (image));
            }
          else
            {
              /*  Otherwise, clear the selection mask  */
              ligma_channel_clear (ligma_image_get_mask (image), NULL, TRUE);
            }

          ligma_image_flush (image);
        }
      else if (region_sel->region_mask)
        {
          gint off_x = 0;
          gint off_y = 0;

          if (! options->sample_merged)
            {
              GList *drawables = ligma_image_get_selected_drawables (image);

              if (g_list_length (drawables) == 1)
                ligma_item_get_offset (drawables->data, &off_x, &off_y);

              g_list_free (drawables);
            }

          ligma_channel_select_buffer (ligma_image_get_mask (image),
                                      LIGMA_REGION_SELECT_TOOL_GET_CLASS (tool)->undo_desc,
                                      region_sel->region_mask,
                                      off_x,
                                      off_y,
                                      sel_options->operation,
                                      sel_options->feather,
                                      sel_options->feather_radius,
                                      sel_options->feather_radius);


          ligma_image_flush (image);
        }
    }

  g_clear_object (&region_sel->region_mask);

  g_clear_pointer (&region_sel->segs, g_free);
  region_sel->n_segs = 0;

  /*  Restore the original threshold  */
  g_object_set (options,
                "threshold", region_sel->saved_threshold,
                NULL);
}

static void
ligma_region_select_tool_motion (LigmaTool         *tool,
                                const LigmaCoords *coords,
                                guint32           time,
                                GdkModifierType   state,
                                LigmaDisplay      *display)
{
  LigmaRegionSelectTool    *region_sel = LIGMA_REGION_SELECT_TOOL (tool);
  LigmaRegionSelectOptions *options    = LIGMA_REGION_SELECT_TOOL_GET_OPTIONS (tool);
  gint                     diff_x, diff_y;
  gdouble                  diff;

  static guint32 last_time = 0;

  /* don't let the events come in too fast, ignore below a delay of 100 ms */
  if (time - last_time < 100)
    return;

  last_time = time;

  diff_x = coords->x - region_sel->x;
  diff_y = coords->y - region_sel->y;

  diff = ((ABS (diff_x) > ABS (diff_y)) ? diff_x : diff_y) / 2.0;

  g_object_set (options,
                "threshold", CLAMP (region_sel->saved_threshold + diff, 0, 255),
                NULL);

  ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

  ligma_region_select_tool_get_mask (region_sel, display);

  ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
}

static void
ligma_region_select_tool_cursor_update (LigmaTool         *tool,
                                       const LigmaCoords *coords,
                                       GdkModifierType   state,
                                       LigmaDisplay      *display)
{
  LigmaRegionSelectOptions *options  = LIGMA_REGION_SELECT_TOOL_GET_OPTIONS (tool);
  LigmaCursorModifier       modifier = LIGMA_CURSOR_MODIFIER_NONE;
  LigmaImage               *image    = ligma_display_get_image (display);

  if (! ligma_image_coords_in_active_pickable (image, coords,
                                              FALSE, options->sample_merged,
                                              FALSE))
    modifier = LIGMA_CURSOR_MODIFIER_BAD;

  ligma_tool_control_set_cursor_modifier (tool->control, modifier);

  LIGMA_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
ligma_region_select_tool_draw (LigmaDrawTool *draw_tool)
{
  LigmaRegionSelectTool    *region_sel = LIGMA_REGION_SELECT_TOOL (draw_tool);
  LigmaRegionSelectOptions *options    = LIGMA_REGION_SELECT_TOOL_GET_OPTIONS (draw_tool);

  if (! options->draw_mask && region_sel->region_mask)
    {
      if (! region_sel->segs)
        {
          /*  calculate and allocate a new segment array which represents
           *  the boundary of the contiguous region
           */
          region_sel->segs = ligma_boundary_find (region_sel->region_mask, NULL,
                                                 babl_format ("Y float"),
                                                 LIGMA_BOUNDARY_WITHIN_BOUNDS,
                                                 0, 0,
                                                 gegl_buffer_get_width  (region_sel->region_mask),
                                                 gegl_buffer_get_height (region_sel->region_mask),
                                                 LIGMA_BOUNDARY_HALF_WAY,
                                                 &region_sel->n_segs);

        }

      if (region_sel->segs)
        {
          gint off_x = 0;
          gint off_y = 0;

          if (! options->sample_merged)
            {
              LigmaImage *image     = ligma_display_get_image (draw_tool->display);
              GList     *drawables = ligma_image_get_selected_drawables (image);

              if (g_list_length (drawables) == 1)
                ligma_item_get_offset (drawables->data, &off_x, &off_y);

              g_list_free (drawables);
            }

          ligma_draw_tool_add_boundary (draw_tool,
                                       region_sel->segs,
                                       region_sel->n_segs,
                                       NULL,
                                       off_x, off_y);
        }
    }
}

static void
ligma_region_select_tool_get_mask (LigmaRegionSelectTool *region_sel,
                                  LigmaDisplay          *display)
{
  LigmaRegionSelectOptions *options = LIGMA_REGION_SELECT_TOOL_GET_OPTIONS (region_sel);
  LigmaDisplayShell        *shell   = ligma_display_get_shell (display);

  ligma_display_shell_set_override_cursor (shell, (LigmaCursorType) GDK_WATCH);

  g_clear_pointer (&region_sel->segs, g_free);
  region_sel->n_segs = 0;

  if (region_sel->region_mask)
    g_object_unref (region_sel->region_mask);

  region_sel->region_mask =
    LIGMA_REGION_SELECT_TOOL_GET_CLASS (region_sel)->get_mask (region_sel,
                                                              display);

  if (options->draw_mask)
    {
      if (region_sel->region_mask)
        {
          LigmaRGB color = { 1.0, 0.0, 1.0, 1.0 };
          gint    off_x = 0;
          gint    off_y = 0;

          if (! options->sample_merged)
            {
              LigmaImage *image     = ligma_display_get_image (display);
              GList     *drawables = ligma_image_get_selected_drawables (image);

              if (g_list_length (drawables) == 1)
                ligma_item_get_offset (drawables->data, &off_x, &off_y);

              g_list_free (drawables);
            }

          ligma_display_shell_set_mask (shell, region_sel->region_mask,
                                       off_x, off_y, &color, FALSE);
        }
      else
        {
          ligma_display_shell_set_mask (shell, NULL, 0, 0, NULL, FALSE);
        }
    }

  ligma_display_shell_unset_override_cursor (shell);
}
