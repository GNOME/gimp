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
#include "base/tile-manager.h"
#include "base/tile.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-contiguous-region.h"
#include "core/gimpimage-mask.h"
#include "core/gimpimage-mask-select.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell.h"

#include "gimpeditselectiontool.h"
#include "gimpfuzzyselecttool.h"
#include "gimptool.h"
#include "selection_options.h"
#include "tool_options.h"
#include "tool_manager.h"

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


static GimpSelectionToolClass *parent_class = NULL;

/*  the fuzzy selection tool options  */
static SelectionOptions  *fuzzy_options = NULL;

/*  XSegments which make up the fuzzy selection boundary  */
static GdkSegment *segs     = NULL;
static gint        num_segs = 0;


/*  public functions  */

void
gimp_fuzzy_select_tool_register (Gimp *gimp)
{
  tool_manager_register_tool (gimp,
			      GIMP_TYPE_FUZZY_SELECT_TOOL,
			      FALSE,
                              "gimp:fuzzy_select_tool",
                              _("Fuzzy Select"),
                              _("Select contiguous regions"),
                              _("/Tools/Selection Tools/Fuzzy Select"), "Z",
                              NULL, "tools/fuzzy_select.html",
                              GIMP_STOCK_TOOL_FUZZY_SELECT);
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

  if (! fuzzy_options)
    {
      fuzzy_options = selection_options_new (GIMP_TYPE_FUZZY_SELECT_TOOL,
					     selection_options_reset);

      tool_manager_register_tool_options (GIMP_TYPE_FUZZY_SELECT_TOOL,
                                          (GimpToolOptions *) fuzzy_options);
    }

  tool->tool_cursor = GIMP_FUZZY_SELECT_TOOL_CURSOR;
  tool->scroll_lock = TRUE;   /*  Do not allow scrolling  */

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
}

static void
gimp_fuzzy_select_tool_button_press (GimpTool        *tool, 
                                     GimpCoords      *coords,
                                     guint32          time,
                                     GdkModifierType  state,
                                     GimpDisplay     *gdisp)
{
  GimpFuzzySelectTool *fuzzy_sel;
  GimpDisplayShell    *shell;

  fuzzy_sel = GIMP_FUZZY_SELECT_TOOL (tool);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  fuzzy_sel->x               = coords->x;
  fuzzy_sel->y               = coords->y;
  fuzzy_sel->first_x         = fuzzy_sel->x;
  fuzzy_sel->first_y         = fuzzy_sel->y;
  fuzzy_sel->first_threshold = fuzzy_options->threshold;

  tool->state = ACTIVE;
  tool->gdisp = gdisp;

  gdk_pointer_grab (shell->canvas->window, FALSE,
		    GDK_POINTER_MOTION_HINT_MASK |
		    GDK_BUTTON1_MOTION_MASK |
		    GDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, time);

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

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), shell->canvas->window);
}

static void
gimp_fuzzy_select_tool_button_release (GimpTool        *tool, 
                                       GimpCoords      *coords,
                                       guint32          time,
                                       GdkModifierType  state,
                                       GimpDisplay     *gdisp)
{
  GimpFuzzySelectTool *fuzzy_sel;
  GimpDrawable        *drawable;

  fuzzy_sel = GIMP_FUZZY_SELECT_TOOL (tool);

  gdk_pointer_ungrab (time);
  gdk_flush ();

  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));
  tool->state = INACTIVE;

  /*  First take care of the case where the user "cancels" the action  */
  if (! (state & GDK_BUTTON3_MASK))
    {
      drawable = gimp_image_active_drawable (gdisp->gimage);

      gimp_image_mask_select_channel (gdisp->gimage,
                                      drawable,
                                      fuzzy_options->sample_merged,
                                      fuzzy_sel->fuzzy_mask,
                                      GIMP_SELECTION_TOOL (tool)->op,
                                      fuzzy_options->feather,
                                      fuzzy_options->feather_radius,
                                      fuzzy_options->feather_radius);

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
  GdkSegment          *new_segs;
  gint                 num_new_segs;
  gint                 diff_x, diff_y;
  gdouble              diff;

  static guint32 last_time = 0;

  fuzzy_sel = GIMP_FUZZY_SELECT_TOOL (tool);
  sel_tool  = GIMP_SELECTION_TOOL (tool);

  if (tool->state != ACTIVE)
    return;

  /* don't let the events come in too fast, ignore below a delay of 100 ms */
  if (ABS (time - last_time) < 100)
    return;

  last_time = time;

  diff_x = coords->x - fuzzy_sel->first_x;
  diff_y = coords->y - fuzzy_sel->first_y;

  diff = ((ABS (diff_x) > ABS (diff_y)) ? diff_x : diff_y) / 2.0;

  gtk_adjustment_set_value (GTK_ADJUSTMENT (fuzzy_options->threshold_w), 
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
  PixelRegion   maskPR;
  GimpChannel  *new;
  GdkSegment   *segs;
  BoundSeg     *bsegs;
  GimpDrawable *drawable;
  gint          i;
  gint          x, y;

  drawable  = gimp_image_active_drawable (gdisp->gimage);

  gimp_set_busy (gdisp->gimage->gimp);

  x = fuzzy_sel->x;
  y = fuzzy_sel->y;

  if (! fuzzy_options->sample_merged)
    {
      gint off_x, off_y;

      gimp_drawable_offsets (drawable, &off_x, &off_y);

      x -= off_x;
      y -= off_y;
    }

  new = gimp_image_contiguous_region_by_seed (gdisp->gimage, drawable, 
                                              fuzzy_options->sample_merged,
                                              fuzzy_options->antialias,
                                              fuzzy_options->threshold,
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
                                 ! fuzzy_options->sample_merged);
      segs[i].x1 = x;
      segs[i].y1 = y;

      gdisplay_transform_coords (gdisp,
                                 bsegs[i].x2, bsegs[i].y2,
                                 &x, &y,
                                 ! fuzzy_options->sample_merged);
      segs[i].x2 = x;
      segs[i].y2 = y;
    }

  /*  free boundary segments  */
  g_free (bsegs);

  gimp_unset_busy (gdisp->gimage->gimp);

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
