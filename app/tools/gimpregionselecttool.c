/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpregionselecttool.c
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

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp-utils.h"
#include "core/gimpboundary.h"
#include "core/gimpchannel.h"
#include "core/gimpchannel-select.h"
#include "core/gimpimage.h"
#include "core/gimplayer-floating-selection.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-cursor.h"

#include "gimpregionselectoptions.h"
#include "gimpregionselecttool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static void   gimp_region_select_tool_finalize       (GObject               *object);

static void   gimp_region_select_tool_button_press   (GimpTool              *tool,
                                                      const GimpCoords      *coords,
                                                      guint32                time,
                                                      GdkModifierType        state,
                                                      GimpButtonPressType    press_type,
                                                      GimpDisplay           *display);
static void   gimp_region_select_tool_button_release (GimpTool              *tool,
                                                      const GimpCoords      *coords,
                                                      guint32                time,
                                                      GdkModifierType        state,
                                                      GimpButtonReleaseType  release_type,
                                                      GimpDisplay           *display);
static void   gimp_region_select_tool_motion         (GimpTool              *tool,
                                                      const GimpCoords      *coords,
                                                      guint32                time,
                                                      GdkModifierType        state,
                                                      GimpDisplay           *display);
static void   gimp_region_select_tool_cursor_update  (GimpTool              *tool,
                                                      const GimpCoords      *coords,
                                                      GdkModifierType        state,
                                                      GimpDisplay           *display);

static void   gimp_region_select_tool_draw           (GimpDrawTool          *draw_tool);

static void   gimp_region_select_tool_get_mask       (GimpRegionSelectTool  *region_sel,
                                                      GimpDisplay           *display);


G_DEFINE_TYPE (GimpRegionSelectTool, gimp_region_select_tool,
               GIMP_TYPE_SELECTION_TOOL)

#define parent_class gimp_region_select_tool_parent_class


static void
gimp_region_select_tool_class_init (GimpRegionSelectToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  object_class->finalize     = gimp_region_select_tool_finalize;

  tool_class->button_press   = gimp_region_select_tool_button_press;
  tool_class->button_release = gimp_region_select_tool_button_release;
  tool_class->motion         = gimp_region_select_tool_motion;
  tool_class->cursor_update  = gimp_region_select_tool_cursor_update;

  draw_tool_class->draw      = gimp_region_select_tool_draw;
}

static void
gimp_region_select_tool_init (GimpRegionSelectTool *region_select)
{
  GimpTool *tool = GIMP_TOOL (region_select);

  gimp_tool_control_set_scroll_lock (tool->control, TRUE);

  region_select->x               = 0;
  region_select->y               = 0;
  region_select->saved_threshold = 0.0;

  region_select->region_mask     = NULL;
  region_select->segs            = NULL;
  region_select->n_segs          = 0;
}

static void
gimp_region_select_tool_finalize (GObject *object)
{
  GimpRegionSelectTool *region_sel = GIMP_REGION_SELECT_TOOL (object);

  g_clear_object (&region_sel->region_mask);

  g_clear_pointer (&region_sel->segs, g_free);
  region_sel->n_segs = 0;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_region_select_tool_button_press (GimpTool            *tool,
                                      const GimpCoords    *coords,
                                      guint32              time,
                                      GdkModifierType      state,
                                      GimpButtonPressType  press_type,
                                      GimpDisplay         *display)
{
  GimpRegionSelectTool    *region_sel = GIMP_REGION_SELECT_TOOL (tool);
  GimpRegionSelectOptions *options    = GIMP_REGION_SELECT_TOOL_GET_OPTIONS (tool);

  region_sel->x               = coords->x;
  region_sel->y               = coords->y;
  region_sel->saved_threshold = options->threshold;

  if (gimp_selection_tool_start_edit (GIMP_SELECTION_TOOL (region_sel),
                                      display, coords))
    {
      return;
    }

  gimp_tool_control_activate (tool->control);
  tool->display = display;

  gimp_tool_push_status (tool, display,
                         _("Move the mouse to change threshold"));

  gimp_region_select_tool_get_mask (region_sel, display);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);
}

static void
gimp_region_select_tool_button_release (GimpTool              *tool,
                                        const GimpCoords      *coords,
                                        guint32                time,
                                        GdkModifierType        state,
                                        GimpButtonReleaseType  release_type,
                                        GimpDisplay           *display)
{
  GimpRegionSelectTool    *region_sel  = GIMP_REGION_SELECT_TOOL (tool);
  GimpSelectionOptions    *sel_options = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);
  GimpRegionSelectOptions *options     = GIMP_REGION_SELECT_TOOL_GET_OPTIONS (tool);
  GimpImage               *image       = gimp_display_get_image (display);

  gimp_tool_pop_status (tool, display);

  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  gimp_tool_control_halt (tool->control);

  if (options->draw_mask)
    gimp_display_shell_set_mask (gimp_display_get_shell (display),
                                 NULL, 0, 0, NULL, FALSE);

  if (release_type != GIMP_BUTTON_RELEASE_CANCEL)
    {
      if (GIMP_SELECTION_TOOL (tool)->function == SELECTION_ANCHOR)
        {
          if (gimp_image_get_floating_selection (image))
            {
              /*  If there is a floating selection, anchor it  */
              floating_sel_anchor (gimp_image_get_floating_selection (image));
            }
          else
            {
              /*  Otherwise, clear the selection mask  */
              gimp_channel_clear (gimp_image_get_mask (image), NULL, TRUE);
            }

          gimp_image_flush (image);
        }
      else if (region_sel->region_mask)
        {
          gint off_x = 0;
          gint off_y = 0;

          if (! options->sample_merged)
            {
              GimpDrawable *drawable = gimp_image_get_active_drawable (image);

              gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);
            }

          gimp_channel_select_buffer (gimp_image_get_mask (image),
                                      GIMP_REGION_SELECT_TOOL_GET_CLASS (tool)->undo_desc,
                                      region_sel->region_mask,
                                      off_x,
                                      off_y,
                                      sel_options->operation,
                                      sel_options->feather,
                                      sel_options->feather_radius,
                                      sel_options->feather_radius);


          gimp_image_flush (image);
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
gimp_region_select_tool_motion (GimpTool         *tool,
                                const GimpCoords *coords,
                                guint32           time,
                                GdkModifierType   state,
                                GimpDisplay      *display)
{
  GimpRegionSelectTool    *region_sel = GIMP_REGION_SELECT_TOOL (tool);
  GimpRegionSelectOptions *options    = GIMP_REGION_SELECT_TOOL_GET_OPTIONS (tool);
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

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  gimp_region_select_tool_get_mask (region_sel, display);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_region_select_tool_cursor_update (GimpTool         *tool,
                                       const GimpCoords *coords,
                                       GdkModifierType   state,
                                       GimpDisplay      *display)
{
  GimpRegionSelectOptions *options  = GIMP_REGION_SELECT_TOOL_GET_OPTIONS (tool);
  GimpCursorModifier       modifier = GIMP_CURSOR_MODIFIER_NONE;
  GimpImage               *image    = gimp_display_get_image (display);

  if (! gimp_image_coords_in_active_pickable (image, coords,
                                              FALSE, options->sample_merged,
                                              FALSE))
    modifier = GIMP_CURSOR_MODIFIER_BAD;

  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_region_select_tool_draw (GimpDrawTool *draw_tool)
{
  GimpRegionSelectTool    *region_sel = GIMP_REGION_SELECT_TOOL (draw_tool);
  GimpRegionSelectOptions *options    = GIMP_REGION_SELECT_TOOL_GET_OPTIONS (draw_tool);

  if (! options->draw_mask && region_sel->region_mask)
    {
      if (! region_sel->segs)
        {
          /*  calculate and allocate a new segment array which represents
           *  the boundary of the contiguous region
           */
          region_sel->segs = gimp_boundary_find (region_sel->region_mask, NULL,
                                                 babl_format ("Y float"),
                                                 GIMP_BOUNDARY_WITHIN_BOUNDS,
                                                 0, 0,
                                                 gegl_buffer_get_width  (region_sel->region_mask),
                                                 gegl_buffer_get_height (region_sel->region_mask),
                                                 GIMP_BOUNDARY_HALF_WAY,
                                                 &region_sel->n_segs);

        }

      if (region_sel->segs)
        {
          gint off_x = 0;
          gint off_y = 0;

          if (! options->sample_merged)
            {
              GimpImage    *image    = gimp_display_get_image (draw_tool->display);
              GimpDrawable *drawable = gimp_image_get_active_drawable (image);

              gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);
            }

          gimp_draw_tool_add_boundary (draw_tool,
                                       region_sel->segs,
                                       region_sel->n_segs,
                                       NULL,
                                       off_x, off_y);
        }
    }
}

static void
gimp_region_select_tool_get_mask (GimpRegionSelectTool *region_sel,
                                  GimpDisplay          *display)
{
  GimpRegionSelectOptions *options = GIMP_REGION_SELECT_TOOL_GET_OPTIONS (region_sel);
  GimpDisplayShell        *shell   = gimp_display_get_shell (display);

  gimp_display_shell_set_override_cursor (shell, (GimpCursorType) GDK_WATCH);

  g_clear_pointer (&region_sel->segs, g_free);
  region_sel->n_segs = 0;

  if (region_sel->region_mask)
    g_object_unref (region_sel->region_mask);

  region_sel->region_mask =
    GIMP_REGION_SELECT_TOOL_GET_CLASS (region_sel)->get_mask (region_sel,
                                                              display);

  if (options->draw_mask)
    {
      if (region_sel->region_mask)
        {
          GimpRGB color = { 1.0, 0.0, 1.0, 1.0 };
          gint    off_x = 0;
          gint    off_y = 0;

          if (! options->sample_merged)
            {
              GimpImage    *image    = gimp_display_get_image (display);
              GimpDrawable *drawable = gimp_image_get_active_drawable (image);

              gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);
            }

          gimp_display_shell_set_mask (shell, region_sel->region_mask,
                                       off_x, off_y, &color, FALSE);
        }
      else
        {
          gimp_display_shell_set_mask (shell, NULL, 0, 0, NULL, FALSE);
        }
    }

  gimp_display_shell_unset_override_cursor (shell);
}
