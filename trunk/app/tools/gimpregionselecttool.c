/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpregionselecttool.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/boundary.h"
#include "base/pixel-region.h"

#include "core/gimpchannel.h"
#include "core/gimpchannel-select.h"
#include "core/gimpimage.h"
#include "core/gimplayer-floating-sel.h"

#include "display/gimpcanvas.h"
#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-cursor.h"
#include "display/gimpdisplayshell-transform.h"

#include "gimpregionselecttool.h"
#include "gimpselectionoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static void   gimp_region_select_tool_finalize       (GObject               *object);

static void   gimp_region_select_tool_button_press   (GimpTool              *tool,
                                                      GimpCoords            *coords,
                                                      guint32                time,
                                                      GdkModifierType        state,
                                                      GimpDisplay           *display);
static void   gimp_region_select_tool_button_release (GimpTool              *tool,
                                                      GimpCoords            *coords,
                                                      guint32                time,
                                                      GdkModifierType        state,
                                                      GimpButtonReleaseType  release_type,
                                                      GimpDisplay           *display);
static void   gimp_region_select_tool_motion         (GimpTool              *tool,
                                                      GimpCoords            *coords,
                                                      guint32                time,
                                                      GdkModifierType        state,
                                                      GimpDisplay           *display);
static void   gimp_region_select_tool_cursor_update  (GimpTool              *tool,
                                                      GimpCoords            *coords,
                                                      GdkModifierType        state,
                                                      GimpDisplay           *display);

static void   gimp_region_select_tool_draw           (GimpDrawTool          *draw_tool);

static GdkSegment *gimp_region_select_tool_calculate (GimpRegionSelectTool  *region_sel,
                                                      GimpDisplay           *display,
                                                      gint                  *num_segs);


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
  gimp_tool_control_set_motion_mode (tool->control, GIMP_MOTION_MODE_COMPRESS);

  region_select->x               = 0;
  region_select->y               = 0;
  region_select->saved_threshold = 0.0;

  region_select->region_mask     = NULL;
  region_select->segs            = NULL;
  region_select->num_segs        = 0;
}

static void
gimp_region_select_tool_finalize (GObject *object)
{
  GimpRegionSelectTool *region_sel = GIMP_REGION_SELECT_TOOL (object);

  if (region_sel->region_mask)
    {
      g_object_unref (region_sel->region_mask);
      region_sel->region_mask = NULL;
    }

  if (region_sel->segs)
    {
      g_free (region_sel->segs);
      region_sel->segs     = NULL;
      region_sel->num_segs = 0;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_region_select_tool_button_press (GimpTool        *tool,
                                      GimpCoords      *coords,
                                      guint32          time,
                                      GdkModifierType  state,
                                      GimpDisplay     *display)
{
  GimpRegionSelectTool *region_sel = GIMP_REGION_SELECT_TOOL (tool);
  GimpSelectionOptions *options    = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);

  region_sel->x               = coords->x;
  region_sel->y               = coords->y;
  region_sel->saved_threshold = options->threshold;

  gimp_tool_control_activate (tool->control);
  tool->display = display;

  if (gimp_selection_tool_start_edit (GIMP_SELECTION_TOOL (region_sel), coords))
    return;

  gimp_tool_push_status (tool, display,
                         _("Move the mouse to change threshold"));

  /*  calculate the region boundary  */
  region_sel->segs = gimp_region_select_tool_calculate (region_sel, display,
                                                        &region_sel->num_segs);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);
}

static void
gimp_region_select_tool_button_release (GimpTool              *tool,
                                        GimpCoords            *coords,
                                        guint32                time,
                                        GdkModifierType        state,
                                        GimpButtonReleaseType  release_type,
                                        GimpDisplay           *display)
{
  GimpRegionSelectTool *region_sel = GIMP_REGION_SELECT_TOOL (tool);
  GimpSelectionOptions *options    = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);

  gimp_tool_pop_status (tool, display);

  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  gimp_tool_control_halt (tool->control);

  if (release_type != GIMP_BUTTON_RELEASE_CANCEL)
    {
      gint off_x, off_y;

      if (GIMP_SELECTION_TOOL (tool)->function == SELECTION_ANCHOR)
        {
          if (gimp_image_floating_sel (display->image))
            {
              /*  If there is a floating selection, anchor it  */
              floating_sel_anchor (gimp_image_floating_sel (display->image));
            }
          else
            {
              /*  Otherwise, clear the selection mask  */
              gimp_channel_clear (gimp_image_get_mask (display->image), NULL,
                                  TRUE);
            }

          gimp_image_flush (display->image);
        }
      else if (region_sel->region_mask)
        {
          if (options->sample_merged)
            {
              off_x = 0;
              off_y = 0;
            }
          else
            {
              GimpDrawable *drawable;

              drawable = gimp_image_get_active_drawable (display->image);

              gimp_item_offsets (GIMP_ITEM (drawable), &off_x, &off_y);
            }

          gimp_channel_select_channel (gimp_image_get_mask (display->image),
                                       GIMP_REGION_SELECT_TOOL_GET_CLASS (tool)->undo_desc,
                                       region_sel->region_mask,
                                       off_x,
                                       off_y,
                                       options->operation,
                                       options->feather,
                                       options->feather_radius,
                                       options->feather_radius);


          gimp_image_flush (display->image);
        }
    }

  if (region_sel->region_mask)
    {
      g_object_unref (region_sel->region_mask);
      region_sel->region_mask = NULL;
    }

  if (region_sel->segs)
    {
      g_free (region_sel->segs);
      region_sel->segs     = NULL;
      region_sel->num_segs = 0;
    }

  /*  Restore the original threshold  */
  g_object_set (options,
                "threshold", region_sel->saved_threshold,
                NULL);
}

static void
gimp_region_select_tool_motion (GimpTool        *tool,
                                GimpCoords      *coords,
                                guint32          time,
                                GdkModifierType  state,
                                GimpDisplay     *display)
{
  GimpRegionSelectTool *region_sel = GIMP_REGION_SELECT_TOOL (tool);
  GimpSelectionOptions *options    = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);
  GdkSegment           *new_segs;
  gint                  num_new_segs;
  gint                  diff_x, diff_y;
  gdouble               diff;

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

  /*  calculate the new region boundary  */
  new_segs = gimp_region_select_tool_calculate (region_sel, display,
                                                &num_new_segs);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  /*  make sure the XSegment array is freed before we assign the new one  */
  if (region_sel->segs)
    g_free (region_sel->segs);

  region_sel->segs     = new_segs;
  region_sel->num_segs = num_new_segs;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_region_select_tool_cursor_update (GimpTool        *tool,
                                       GimpCoords      *coords,
                                       GdkModifierType  state,
                                       GimpDisplay     *display)
{
  GimpSelectionOptions *options  = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);
  GimpCursorModifier    modifier = GIMP_CURSOR_MODIFIER_NONE;

  if (! gimp_image_coords_in_active_pickable (display->image, coords,
                                              options->sample_merged, FALSE))
    modifier = GIMP_CURSOR_MODIFIER_BAD;

  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_region_select_tool_draw (GimpDrawTool *draw_tool)
{
  GimpRegionSelectTool *region_sel = GIMP_REGION_SELECT_TOOL (draw_tool);

  if (region_sel->segs)
    {
      GimpDisplayShell *shell;

      shell = GIMP_DISPLAY_SHELL (GIMP_TOOL (draw_tool)->display->shell);

      gimp_canvas_draw_segments (GIMP_CANVAS (shell->canvas),
                                 GIMP_CANVAS_STYLE_XOR,
                                 region_sel->segs, region_sel->num_segs);
    }
}

static GdkSegment *
gimp_region_select_tool_calculate (GimpRegionSelectTool *region_sel,
                                   GimpDisplay          *display,
                                   gint                 *num_segs)
{
  GimpTool             *tool    = GIMP_TOOL (region_sel);
  GimpSelectionOptions *options = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);
  GimpDisplayShell     *shell   = GIMP_DISPLAY_SHELL (display->shell);
  GimpDrawable         *drawable;
  GdkSegment           *segs;
  BoundSeg             *bsegs;
  PixelRegion           maskPR;

  drawable = gimp_image_get_active_drawable (display->image);

  gimp_display_shell_set_override_cursor (shell, GDK_WATCH);

  if (region_sel->region_mask)
    g_object_unref (region_sel->region_mask);

  region_sel->region_mask =
    GIMP_REGION_SELECT_TOOL_GET_CLASS (region_sel)->get_mask (region_sel,
                                                              display);

  if (! region_sel->region_mask)
    {
      gimp_display_shell_unset_override_cursor (shell);

      *num_segs = 0;
      return NULL;
    }

  /*  calculate and allocate a new segment array which represents the
   *  boundary of the contiguous region
   */
  pixel_region_init (&maskPR,
                     gimp_drawable_get_tiles (GIMP_DRAWABLE (region_sel->region_mask)),
                     0, 0,
                     gimp_item_width  (GIMP_ITEM (region_sel->region_mask)),
                     gimp_item_height (GIMP_ITEM (region_sel->region_mask)),
                     FALSE);

  bsegs = boundary_find (&maskPR, BOUNDARY_WITHIN_BOUNDS,
                         0, 0,
                         gimp_item_width  (GIMP_ITEM (region_sel->region_mask)),
                         gimp_item_height (GIMP_ITEM (region_sel->region_mask)),
                         BOUNDARY_HALF_WAY,
                         num_segs);

  segs = g_new (GdkSegment, *num_segs);

  gimp_display_shell_transform_segments (shell, bsegs, segs, *num_segs,
                                         ! options->sample_merged);
  g_free (bsegs);

  gimp_display_shell_unset_override_cursor (shell);

  return segs;
}
