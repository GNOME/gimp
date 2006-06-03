/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <stdlib.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "libgimpmath/gimpmath.h"

#include "tools-types.h"

#include "base/boundary.h"
#include "base/pixel-region.h"

#include "core/gimpchannel.h"
#include "core/gimpchannel-select.h"
#include "core/gimpimage.h"
#include "core/gimpimage-contiguous-region.h"
#include "core/gimplayer-floating-sel.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpcanvas.h"
#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-cursor.h"
#include "display/gimpdisplayshell-transform.h"

#include "gimpfuzzyselecttool.h"
#include "gimpselectionoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static void   gimp_fuzzy_select_tool_finalize       (GObject         *object);

static void   gimp_fuzzy_select_tool_button_press   (GimpTool        *tool,
                                                     GimpCoords      *coords,
                                                     guint32          time,
                                                     GdkModifierType  state,
                                                     GimpDisplay     *display);
static void   gimp_fuzzy_select_tool_button_release (GimpTool        *tool,
                                                     GimpCoords      *coords,
                                                     guint32          time,
                                                     GdkModifierType  state,
                                                     GimpDisplay     *display);
static void   gimp_fuzzy_select_tool_motion         (GimpTool        *tool,
                                                     GimpCoords      *coords,
                                                     guint32          time,
                                                     GdkModifierType  state,
                                                     GimpDisplay     *display);
static void   gimp_fuzzy_select_tool_cursor_update  (GimpTool        *tool,
                                                     GimpCoords      *coords,
                                                     GdkModifierType  state,
                                                     GimpDisplay     *display);

static void   gimp_fuzzy_select_tool_draw           (GimpDrawTool    *draw_tool);

static GdkSegment *
               gimp_fuzzy_select_tool_calculate (GimpFuzzySelectTool *fuzzy_sel,
                                                 GimpDisplay         *display,
                                                 gint                *num_segs);


G_DEFINE_TYPE (GimpFuzzySelectTool, gimp_fuzzy_select_tool,
               GIMP_TYPE_SELECTION_TOOL)

#define parent_class gimp_fuzzy_select_tool_parent_class


void
gimp_fuzzy_select_tool_register (GimpToolRegisterCallback  callback,
                                 gpointer                  data)
{
  (* callback) (GIMP_TYPE_FUZZY_SELECT_TOOL,
                GIMP_TYPE_SELECTION_OPTIONS,
                gimp_selection_options_gui,
                0,
                "gimp-fuzzy-select-tool",
                _("Fuzzy Select"),
                _("Select contiguous regions"),
                N_("Fu_zzy Select"), "U",
                NULL, GIMP_HELP_TOOL_FUZZY_SELECT,
                GIMP_STOCK_TOOL_FUZZY_SELECT,
                data);
}

static void
gimp_fuzzy_select_tool_class_init (GimpFuzzySelectToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  object_class->finalize     = gimp_fuzzy_select_tool_finalize;

  tool_class->button_press   = gimp_fuzzy_select_tool_button_press;
  tool_class->button_release = gimp_fuzzy_select_tool_button_release;
  tool_class->motion         = gimp_fuzzy_select_tool_motion;
  tool_class->cursor_update  = gimp_fuzzy_select_tool_cursor_update;

  draw_tool_class->draw      = gimp_fuzzy_select_tool_draw;
}

static void
gimp_fuzzy_select_tool_init (GimpFuzzySelectTool *fuzzy_select)
{
  GimpTool *tool = GIMP_TOOL (fuzzy_select);

  gimp_tool_control_set_scroll_lock (tool->control, TRUE);
  gimp_tool_control_set_motion_mode (tool->control, GIMP_MOTION_MODE_COMPRESS);
  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_FUZZY_SELECT);

  fuzzy_select->x               = 0;
  fuzzy_select->y               = 0;
  fuzzy_select->first_x         = 0;
  fuzzy_select->first_y         = 0;
  fuzzy_select->first_threshold = 0.0;

  fuzzy_select->fuzzy_mask      = NULL;
  fuzzy_select->segs            = NULL;
  fuzzy_select->num_segs        = 0;
}

static void
gimp_fuzzy_select_tool_finalize (GObject *object)
{
  GimpFuzzySelectTool *fuzzy_sel = GIMP_FUZZY_SELECT_TOOL (object);

  if (fuzzy_sel->fuzzy_mask)
    {
      g_object_unref (fuzzy_sel->fuzzy_mask);
      fuzzy_sel->fuzzy_mask = NULL;
    }

  if (fuzzy_sel->segs)
    {
      g_free (fuzzy_sel->segs);
      fuzzy_sel->segs     = NULL;
      fuzzy_sel->num_segs = 0;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_fuzzy_select_tool_button_press (GimpTool        *tool,
                                     GimpCoords      *coords,
                                     guint32          time,
                                     GdkModifierType  state,
                                     GimpDisplay     *display)
{
  GimpFuzzySelectTool  *fuzzy_sel = GIMP_FUZZY_SELECT_TOOL (tool);
  GimpSelectionOptions *options;

  options = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  fuzzy_sel->x               = coords->x;
  fuzzy_sel->y               = coords->y;
  fuzzy_sel->first_x         = fuzzy_sel->x;
  fuzzy_sel->first_y         = fuzzy_sel->y;
  fuzzy_sel->first_threshold = options->threshold;

  gimp_tool_control_activate (tool->control);
  tool->display = display;

  if (gimp_selection_tool_start_edit (GIMP_SELECTION_TOOL (fuzzy_sel), coords))
    return;

  gimp_tool_push_status (tool, display, _("Move the mouse to change threshold."));

  /*  calculate the region boundary  */
  fuzzy_sel->segs = gimp_fuzzy_select_tool_calculate (fuzzy_sel, display,
                                                      &fuzzy_sel->num_segs);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);
}

static void
gimp_fuzzy_select_tool_button_release (GimpTool        *tool,
                                       GimpCoords      *coords,
                                       guint32          time,
                                       GdkModifierType  state,
                                       GimpDisplay     *display)
{
  GimpFuzzySelectTool  *fuzzy_sel = GIMP_FUZZY_SELECT_TOOL (tool);
  GimpSelectionOptions *options;

  options = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  gimp_tool_pop_status (tool, display);

  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  gimp_tool_control_halt (tool->control);

  /*  First take care of the case where the user "cancels" the action  */
  if (! (state & GDK_BUTTON3_MASK))
    {
      gint off_x, off_y;

      if (GIMP_SELECTION_TOOL (tool)->op == SELECTION_ANCHOR)
        {
          /*  If there is a floating selection, anchor it  */
          if (gimp_image_floating_sel (display->image))
            floating_sel_anchor (gimp_image_floating_sel (display->image));
          /*  Otherwise, clear the selection mask  */
          else
            gimp_channel_clear (gimp_image_get_mask (display->image), NULL,
                                TRUE);

          gimp_image_flush (display->image);
          return;
        }

      if (options->sample_merged)
        {
          off_x = 0;
          off_y = 0;
        }
      else
        {
          GimpDrawable *drawable = gimp_image_active_drawable (display->image);

          gimp_item_offsets (GIMP_ITEM (drawable), &off_x, &off_y);
        }

      gimp_channel_select_channel (gimp_image_get_mask (display->image),
                                   tool->tool_info->blurb,
                                   fuzzy_sel->fuzzy_mask,
                                   off_x,
                                   off_y,
                                   GIMP_SELECTION_TOOL (tool)->op,
                                   options->feather,
                                   options->feather_radius,
                                   options->feather_radius);

      g_object_unref (fuzzy_sel->fuzzy_mask);
      fuzzy_sel->fuzzy_mask = NULL;

      gimp_image_flush (display->image);
    }

  /*  If the segment array is allocated, free it  */
  if (fuzzy_sel->segs)
    {
      g_free (fuzzy_sel->segs);
      fuzzy_sel->segs     = NULL;
      fuzzy_sel->num_segs = 0;
    }

  /*  Restore the original threshold  */
  g_object_set (options,
                "threshold", fuzzy_sel->first_threshold,
                NULL);
}

static void
gimp_fuzzy_select_tool_motion (GimpTool        *tool,
                               GimpCoords      *coords,
                               guint32          time,
                               GdkModifierType  state,
                               GimpDisplay     *display)
{
  GimpFuzzySelectTool  *fuzzy_sel = GIMP_FUZZY_SELECT_TOOL (tool);
  GimpSelectionOptions *options;
  GdkSegment           *new_segs;
  gint                  num_new_segs;
  gint                  diff_x, diff_y;
  gdouble               diff;

  static guint32 last_time = 0;

  options = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  /* don't let the events come in too fast, ignore below a delay of 100 ms */
  if (time - last_time < 100)
    return;

  last_time = time;

  diff_x = coords->x - fuzzy_sel->first_x;
  diff_y = coords->y - fuzzy_sel->first_y;

  diff = ((ABS (diff_x) > ABS (diff_y)) ? diff_x : diff_y) / 2.0;

  g_object_set (options,
                "threshold", CLAMP (fuzzy_sel->first_threshold + diff, 0, 255),
                NULL);

  /*  calculate the new fuzzy boundary  */
  new_segs = gimp_fuzzy_select_tool_calculate (fuzzy_sel, display, &num_new_segs);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  /*  make sure the XSegment array is freed before we assign the new one  */
  if (fuzzy_sel->segs)
    g_free (fuzzy_sel->segs);

  fuzzy_sel->segs     = new_segs;
  fuzzy_sel->num_segs = num_new_segs;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_fuzzy_select_tool_cursor_update (GimpTool        *tool,
                                      GimpCoords      *coords,
                                      GdkModifierType  state,
                                      GimpDisplay     *display)
{
  GimpSelectionOptions *options;
  GimpCursorModifier    modifier = GIMP_CURSOR_MODIFIER_NONE;

  options = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  if (! gimp_image_coords_in_active_pickable (display->image, coords,
                                              options->sample_merged, TRUE))
    modifier = GIMP_CURSOR_MODIFIER_BAD;

  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_fuzzy_select_tool_draw (GimpDrawTool *draw_tool)
{
  GimpFuzzySelectTool *fuzzy_sel = GIMP_FUZZY_SELECT_TOOL (draw_tool);

  if (fuzzy_sel->segs)
    {
      GimpDisplayShell *shell;

      shell = GIMP_DISPLAY_SHELL (GIMP_TOOL (draw_tool)->display->shell);

      gimp_canvas_draw_segments (GIMP_CANVAS (shell->canvas),
                                 GIMP_CANVAS_STYLE_XOR,
                                 fuzzy_sel->segs, fuzzy_sel->num_segs);
    }
}

static GdkSegment *
gimp_fuzzy_select_tool_calculate (GimpFuzzySelectTool *fuzzy_sel,
                                  GimpDisplay         *display,
                                  gint                *num_segs)
{
  GimpTool             *tool = GIMP_TOOL (fuzzy_sel);
  GimpSelectionOptions *options;
  GimpDisplayShell     *shell;
  PixelRegion           maskPR;
  GimpChannel          *new;
  GdkSegment           *segs;
  BoundSeg             *bsegs;
  GimpDrawable         *drawable;
  gint                  i;
  gint                  x, y;

  options = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  shell = GIMP_DISPLAY_SHELL (display->shell);

  drawable = gimp_image_active_drawable (display->image);

  gimp_display_shell_set_override_cursor (shell, GDK_WATCH);

  x = fuzzy_sel->x;
  y = fuzzy_sel->y;

  if (! options->sample_merged)
    {
      gint off_x, off_y;

      gimp_item_offsets (GIMP_ITEM (drawable), &off_x, &off_y);

      x -= off_x;
      y -= off_y;
    }

  new = gimp_image_contiguous_region_by_seed (display->image, drawable,
                                              options->sample_merged,
                                              options->antialias,
                                              options->threshold,
                                              options->select_transparent,
                                              x, y);

  if (fuzzy_sel->fuzzy_mask)
    g_object_unref (fuzzy_sel->fuzzy_mask);

  fuzzy_sel->fuzzy_mask = new;

  /*  calculate and allocate a new XSegment array which represents the boundary
   *  of the color-contiguous region
   */
  pixel_region_init (&maskPR,
                     gimp_drawable_get_tiles (GIMP_DRAWABLE (fuzzy_sel->fuzzy_mask)),
                     0, 0,
                     gimp_item_width  (GIMP_ITEM (fuzzy_sel->fuzzy_mask)),
                     gimp_item_height (GIMP_ITEM (fuzzy_sel->fuzzy_mask)),
                     FALSE);

  bsegs = boundary_find (&maskPR, BOUNDARY_WITHIN_BOUNDS,
                         0, 0,
                         gimp_item_width  (GIMP_ITEM (fuzzy_sel->fuzzy_mask)),
                         gimp_item_height (GIMP_ITEM (fuzzy_sel->fuzzy_mask)),
                         BOUNDARY_HALF_WAY,
                         num_segs);

  segs = g_new (GdkSegment, *num_segs);

  for (i = 0; i < *num_segs; i++)
    {
      gimp_display_shell_transform_xy (shell,
                                       bsegs[i].x1, bsegs[i].y1,
                                       &x, &y,
                                       ! options->sample_merged);
      segs[i].x1 = x;
      segs[i].y1 = y;

      gimp_display_shell_transform_xy (shell,
                                       bsegs[i].x2, bsegs[i].y2,
                                       &x, &y,
                                       ! options->sample_merged);
      segs[i].x2 = x;
      segs[i].y2 = y;
    }

  g_free (bsegs);

  gimp_display_shell_unset_override_cursor (shell);

  return segs;
}
