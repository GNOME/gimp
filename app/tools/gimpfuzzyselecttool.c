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

#include "display/display-types.h"
#include "libgimptool/gimptooltypes.h"

#include "base/boundary.h"
#include "base/pixel-region.h"

#include "core/gimpchannel.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-contiguous-region.h"
#include "core/gimpimage-mask.h"
#include "core/gimpimage-mask-select.h"
#include "core/gimplayer-floating-sel.h"
#include "core/gimptoolinfo.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell.h"

#include "gimpeditselectiontool.h"
#include "gimpfuzzyselecttool.h"
#include "selection_options.h"

#include "gimprc.h"

#include "libgimp/gimpintl.h"


static void   gimp_fuzzy_select_tool_class_init (GimpFuzzySelectToolClass *klass);
static void   gimp_fuzzy_select_tool_init       (GimpFuzzySelectTool      *fuzzy_select);

static void   gimp_fuzzy_select_tool_finalize       (GObject         *object);

static void   gimp_fuzzy_select_tool_button_press   (GimpTool        *tool,
                                                     GimpCoords      *coords,
                                                     guint32          time,
                                                     GdkModifierType  state,
                                                     GimpDisplay     *gdisp);
static void   gimp_fuzzy_select_tool_button_release (GimpTool        *tool,
                                                     GimpCoords      *coords,
                                                     guint32          time,
                                                     GdkModifierType  state,
                                                     GimpDisplay     *gdisp);
static void   gimp_fuzzy_select_tool_motion         (GimpTool        *tool,
                                                     GimpCoords      *coords,
                                                     guint32          time,
                                                     GdkModifierType  state,
                                                     GimpDisplay     *gdisp);

static void   gimp_fuzzy_select_tool_draw           (GimpDrawTool    *draw_tool);

static GdkSegment * fuzzy_select_calculate     (GimpFuzzySelectTool *fuzzy_sel,
                                                GimpDisplay         *gdisp,
                                                gint                *nsegs);


/*  XSegments which make up the fuzzy selection boundary  */
static GdkSegment *segs     = NULL;
static gint        num_segs = 0;

static GimpSelectionToolClass *parent_class = NULL;


/*  public functions  */

void
gimp_fuzzy_select_tool_register (GimpToolRegisterCallback  callback,
                                 Gimp                     *gimp)
{
  (* callback) (GIMP_TYPE_FUZZY_SELECT_TOOL,
                selection_options_new,
                FALSE,
                "gimp-fuzzy-select-tool",
                _("Fuzzy Select"),
                _("Select contiguous regions"),
                _("/Tools/Selection Tools/Fuzzy Select"), "Z",
                NULL, "tools/fuzzy_select.html",
                GIMP_STOCK_TOOL_FUZZY_SELECT,
                gimp);
}

GType
gimp_fuzzy_select_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpFuzzySelectToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_fuzzy_select_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpFuzzySelectTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_fuzzy_select_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_SELECTION_TOOL,
					  "GimpFuzzySelectTool",
                                          &tool_info, 0);
    }

  return tool_type;
}


/*  private functions  */

static void
gimp_fuzzy_select_tool_class_init (GimpFuzzySelectToolClass *klass)
{
  GObjectClass      *object_class;
  GimpToolClass     *tool_class;
  GimpDrawToolClass *draw_tool_class;

  object_class    = G_OBJECT_CLASS (klass);
  tool_class      = GIMP_TOOL_CLASS (klass);
  draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize     = gimp_fuzzy_select_tool_finalize;

  tool_class->button_press   = gimp_fuzzy_select_tool_button_press;
  tool_class->button_release = gimp_fuzzy_select_tool_button_release;
  tool_class->motion         = gimp_fuzzy_select_tool_motion;

  draw_tool_class->draw      = gimp_fuzzy_select_tool_draw;
}

static void
gimp_fuzzy_select_tool_init (GimpFuzzySelectTool *fuzzy_select)
{
  GimpTool          *tool;
  GimpSelectionTool *select_tool;

  tool        = GIMP_TOOL (fuzzy_select);
  select_tool = GIMP_SELECTION_TOOL (fuzzy_select);

  tool->control = gimp_tool_control_new  (TRUE,                       /* scroll_lock */
                                          TRUE,                       /* auto_snap_to */
                                          TRUE,                       /* preserve */
                                          FALSE,                      /* handle_empty_image */
                                          FALSE,                      /* perfectmouse */
                                          GIMP_MOUSE_CURSOR,          /* cursor */
                                          GIMP_FUZZY_SELECT_TOOL_CURSOR,      /* tool_cursor */
                                          GIMP_CURSOR_MODIFIER_NONE,  /* cursor_modifier */
                                          GIMP_MOUSE_CURSOR,          /* toggle_cursor */
                                          GIMP_TOOL_CURSOR_NONE,      /* toggle_tool_cursor */
                                          GIMP_CURSOR_MODIFIER_NONE   /* toggle_cursor_modifier */);

  /* FIXME!! tool->motion_mode = GIMP_MOTION_MODE_COMPRESS; */

  fuzzy_select->fuzzy_mask      = NULL;
  fuzzy_select->x               = 0;
  fuzzy_select->y               = 0;
  fuzzy_select->first_x         = 0;
  fuzzy_select->first_y         = 0;
  fuzzy_select->first_threshold = 0.0;
}

static void
gimp_fuzzy_select_tool_finalize (GObject *object)
{
  GimpFuzzySelectTool *fuzzy_sel;

  fuzzy_sel = GIMP_FUZZY_SELECT_TOOL (object);

  if (fuzzy_sel->fuzzy_mask)
    {
      g_object_unref (G_OBJECT (fuzzy_sel->fuzzy_mask));
      fuzzy_sel->fuzzy_mask = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_fuzzy_select_tool_button_press (GimpTool        *tool, 
                                     GimpCoords      *coords,
                                     guint32          time,
                                     GdkModifierType  state,
                                     GimpDisplay     *gdisp)
{
  GimpFuzzySelectTool *fuzzy_sel;
  SelectionOptions    *sel_options;

  fuzzy_sel = GIMP_FUZZY_SELECT_TOOL (tool);

  sel_options = (SelectionOptions *) tool->tool_info->tool_options;

  fuzzy_sel->x               = coords->x;
  fuzzy_sel->y               = coords->y;
  fuzzy_sel->first_x         = fuzzy_sel->x;
  fuzzy_sel->first_y         = fuzzy_sel->y;
  fuzzy_sel->first_threshold = sel_options->threshold;

  gimp_tool_control_activate(tool->control);
  tool->gdisp = gdisp;

  switch (GIMP_SELECTION_TOOL (tool)->op)
    {
    case SELECTION_MOVE_MASK:
      init_edit_selection (tool, gdisp, coords, EDIT_MASK_TRANSLATE);
      return;
    case SELECTION_MOVE:
      init_edit_selection (tool, gdisp, coords, EDIT_MASK_TO_LAYER_TRANSLATE);
      return;
    default:
      break;
    }

  /*  calculate the region boundary  */
  segs = fuzzy_select_calculate (fuzzy_sel, gdisp, &num_segs);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), gdisp);
}

static void
gimp_fuzzy_select_tool_button_release (GimpTool        *tool, 
                                       GimpCoords      *coords,
                                       guint32          time,
                                       GdkModifierType  state,
                                       GimpDisplay     *gdisp)
{
  GimpFuzzySelectTool *fuzzy_sel;
  SelectionOptions    *sel_options;

  fuzzy_sel = GIMP_FUZZY_SELECT_TOOL (tool);

  sel_options = (SelectionOptions *) tool->tool_info->tool_options;

  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  gimp_tool_control_halt(tool->control);    /* sets paused_count to 0 -- is this ok? */

  /*  First take care of the case where the user "cancels" the action  */
  if (! (state & GDK_BUTTON3_MASK))
    {
      gint off_x, off_y;

      if (GIMP_SELECTION_TOOL (tool)->op == SELECTION_ANCHOR)
	{
	  /*  If there is a floating selection, anchor it  */
	  if (gimp_image_floating_sel (gdisp->gimage))
	    floating_sel_anchor (gimp_image_floating_sel (gdisp->gimage));
	  /*  Otherwise, clear the selection mask  */
	  else
	    gimp_image_mask_clear (gdisp->gimage);

	  gdisplays_flush ();
	  return;
	}

      if (sel_options->sample_merged)
        {
          off_x = 0;
          off_y = 0;
        }
      else
        {
          GimpDrawable *drawable;

          drawable = gimp_image_active_drawable (gdisp->gimage);

          gimp_drawable_offsets (drawable, &off_x, &off_y);
        }

      gimp_image_mask_select_channel (gdisp->gimage,
                                      fuzzy_sel->fuzzy_mask,
                                      off_x,
                                      off_y,
                                      GIMP_SELECTION_TOOL (tool)->op,
                                      sel_options->feather,
                                      sel_options->feather_radius,
                                      sel_options->feather_radius);

      g_object_unref (G_OBJECT (fuzzy_sel->fuzzy_mask));
      fuzzy_sel->fuzzy_mask = NULL;

      gdisplays_flush ();
    }

  /*  If the segment array is allocated, free it  */
  if (segs)
    {
      g_free (segs);
      segs = NULL;
    }
}

static void
gimp_fuzzy_select_tool_motion (GimpTool        *tool, 
                               GimpCoords      *coords,
                               guint32          time,
                               GdkModifierType  state,
                               GimpDisplay     *gdisp)
{
  GimpFuzzySelectTool *fuzzy_sel;
  GimpSelectionTool   *sel_tool;
  SelectionOptions    *sel_options;
  GdkSegment          *new_segs;
  gint                 num_new_segs;
  gint                 diff_x, diff_y;
  gdouble              diff;

  static guint32 last_time = 0;

  fuzzy_sel = GIMP_FUZZY_SELECT_TOOL (tool);
  sel_tool  = GIMP_SELECTION_TOOL (tool);

  sel_options = (SelectionOptions *) tool->tool_info->tool_options;

  if (!gimp_tool_control_is_active(tool->control))
    return;

  /* don't let the events come in too fast, ignore below a delay of 100 ms */
  if (ABS (time - last_time) < 100)
    return;

  last_time = time;

  diff_x = coords->x - fuzzy_sel->first_x;
  diff_y = coords->y - fuzzy_sel->first_y;

  diff = ((ABS (diff_x) > ABS (diff_y)) ? diff_x : diff_y) / 2.0;

  gtk_adjustment_set_value (GTK_ADJUSTMENT (sel_options->threshold_w), 
			    fuzzy_sel->first_threshold + diff);

  /*  calculate the new fuzzy boundary  */
  new_segs = fuzzy_select_calculate (fuzzy_sel, gdisp, &num_new_segs);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  /*  make sure the XSegment array is freed before we assign the new one  */
  if (segs)
    g_free (segs);
  segs = new_segs;
  num_segs = num_new_segs;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}


static GdkSegment *
fuzzy_select_calculate (GimpFuzzySelectTool *fuzzy_sel,
			GimpDisplay         *gdisp,
			gint                *nsegs)
{
  GimpTool         *tool;
  SelectionOptions *sel_options;
  PixelRegion       maskPR;
  GimpChannel      *new;
  GdkSegment       *segs;
  BoundSeg         *bsegs;
  GimpDrawable     *drawable;
  gint              i;
  gint              x, y;

  tool = GIMP_TOOL (fuzzy_sel);

  sel_options = (SelectionOptions *) tool->tool_info->tool_options;

  drawable = gimp_image_active_drawable (gdisp->gimage);

  gimp_display_shell_set_override_cursor (GIMP_DISPLAY_SHELL (gdisp->shell),
                                          GDK_WATCH);

  x = fuzzy_sel->x;
  y = fuzzy_sel->y;

  if (! sel_options->sample_merged)
    {
      gint off_x, off_y;

      gimp_drawable_offsets (drawable, &off_x, &off_y);

      x -= off_x;
      y -= off_y;
    }

  new = gimp_image_contiguous_region_by_seed (gdisp->gimage, drawable, 
                                              sel_options->sample_merged,
                                              sel_options->antialias,
                                              sel_options->threshold,
                                              sel_options->select_transparent,
                                              x, y);

  if (fuzzy_sel->fuzzy_mask)
    g_object_unref (G_OBJECT (fuzzy_sel->fuzzy_mask));

  fuzzy_sel->fuzzy_mask = new;

  /*  calculate and allocate a new XSegment array which represents the boundary
   *  of the color-contiguous region
   */
  pixel_region_init (&maskPR,
                     gimp_drawable_data (GIMP_DRAWABLE (fuzzy_sel->fuzzy_mask)),
		     0, 0, 
		     gimp_drawable_width (GIMP_DRAWABLE (fuzzy_sel->fuzzy_mask)), 
		     gimp_drawable_height (GIMP_DRAWABLE (fuzzy_sel->fuzzy_mask)), 
		     FALSE);
  bsegs = find_mask_boundary (&maskPR, nsegs, WithinBounds,
			      0, 0,
			      gimp_drawable_width (GIMP_DRAWABLE (fuzzy_sel->fuzzy_mask)),
			      gimp_drawable_height (GIMP_DRAWABLE (fuzzy_sel->fuzzy_mask)));

  segs = g_new (GdkSegment, *nsegs);

  for (i = 0; i < *nsegs; i++)
    {
      gdisplay_transform_coords (gdisp,
                                 bsegs[i].x1, bsegs[i].y1,
                                 &x, &y,
                                 ! sel_options->sample_merged);
      segs[i].x1 = x;
      segs[i].y1 = y;

      gdisplay_transform_coords (gdisp,
                                 bsegs[i].x2, bsegs[i].y2,
                                 &x, &y,
                                 ! sel_options->sample_merged);
      segs[i].x2 = x;
      segs[i].y2 = y;
    }

  g_free (bsegs);

  gimp_display_shell_unset_override_cursor (GIMP_DISPLAY_SHELL (gdisp->shell));

  return segs;
}

static void
gimp_fuzzy_select_tool_draw (GimpDrawTool *draw_tool)
{
  GimpFuzzySelectTool *fuzzy_sel;

  fuzzy_sel = GIMP_FUZZY_SELECT_TOOL (draw_tool);

  if (segs)
    gdk_draw_segments (draw_tool->win,
                       draw_tool->gc,
                       segs, num_segs);
}
