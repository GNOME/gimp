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
#include "core/gimpimage-mask.h"

#include "gimpeditselectiontool.h"
#include "gimpfuzzyselecttool.h"
#include "gimptool.h"
#include "selection_options.h"
#include "tool_options.h"
#include "tool_manager.h"

#include "gdisplay.h"
#include "gimprc.h"

#include "libgimp/gimpintl.h"


static void   gimp_fuzzy_select_tool_class_init (GimpFuzzySelectToolClass *klass);
static void   gimp_fuzzy_select_tool_init       (GimpFuzzySelectTool      *fuzzy_select);
static void   gimp_fuzzy_select_tool_destroy        (GtkObject      *object);

static void   gimp_fuzzy_select_tool_button_press   (GimpTool       *tool,
                                                     GdkEventButton *bevent,
                                                     GDisplay       *gdisp);
static void   gimp_fuzzy_select_tool_button_release (GimpTool       *tool,
                                                     GdkEventButton *bevent,
                                                     GDisplay       *gdisp);
static void   gimp_fuzzy_select_tool_motion         (GimpTool       *tool,
                                                     GdkEventMotion *mevent,
                                                     GDisplay       *gdisp);

static void   gimp_fuzzy_select_tool_draw           (GimpDrawTool   *draw_tool);

static GdkSegment * fuzzy_select_calculate     (GimpFuzzySelectTool *fuzzy_sel,
                                                GDisplay            *gdisp,
                                                gint                *nsegs);


static GimpSelectionToolClass *parent_class = NULL;

/*  the fuzzy selection tool options  */
static SelectionOptions  *fuzzy_options = NULL;

/*  XSegments which make up the fuzzy selection boundary  */
static GdkSegment *segs     = NULL;
static gint        num_segs = 0;

GimpChannel * fuzzy_mask = NULL;


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

GtkType
gimp_fuzzy_select_tool_get_type (void)
{
  static GtkType fuzzy_select_type = 0;

  if (! fuzzy_select_type)
    {
      GtkTypeInfo fuzzy_select_info =
      {
        "GimpFuzzySelectTool",
        sizeof (GimpFuzzySelectTool),
        sizeof (GimpFuzzySelectToolClass),
        (GtkClassInitFunc) gimp_fuzzy_select_tool_class_init,
        (GtkObjectInitFunc) gimp_fuzzy_select_tool_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL
      };

      fuzzy_select_type = gtk_type_unique (GIMP_TYPE_SELECTION_TOOL,
                                           &fuzzy_select_info);
    }

  return fuzzy_select_type;
}

/*******************************/
/*  Fuzzy selection apparatus  */

static gint
is_pixel_sufficiently_different (guchar   *col1, 
				 guchar   *col2,
				 gboolean  antialias, 
				 gint      threshold, 
				 gint      bytes,
				 gboolean  has_alpha)
{
  gint diff;
  gint max;
  gint b;
  gint alpha;

  max = 0;
  alpha = (has_alpha) ? bytes - 1 : bytes;

  /*  if there is an alpha channel, never select transparent regions  */
  if (has_alpha && col2[alpha] == 0)
    return 0;

  for (b = 0; b < bytes; b++)
    {
      diff = col1[b] - col2[b];
      diff = abs (diff);
      if (diff > max)
	max = diff;
    }

  if (antialias)
    {
      float aa;

      aa = 1.5 - ((float) max / threshold);
      if (aa <= 0)
	return 0;
      else if (aa < 0.5)
	return (unsigned char) (aa * 512);
      else
	return 255;
    }
  else
    {
      if (max > threshold)
	return 0;
      else
	return 255;
    }
}

static void
ref_tiles (TileManager  *src, 
	   TileManager  *mask, 
	   Tile        **s_tile, 
	   Tile        **m_tile,
	   gint          x, 
	   gint          y, 
	   guchar      **s, 
	   guchar      **m)
{
  if (*s_tile != NULL)
    tile_release (*s_tile, FALSE);
  if (*m_tile != NULL)
    tile_release (*m_tile, TRUE);

  *s_tile = tile_manager_get_tile (src, x, y, TRUE, FALSE);
  *m_tile = tile_manager_get_tile (mask, x, y, TRUE, TRUE);

  *s = tile_data_pointer (*s_tile, x % TILE_WIDTH, y % TILE_HEIGHT);
  *m = tile_data_pointer (*m_tile, x % TILE_WIDTH, y % TILE_HEIGHT);
}

static int
find_contiguous_segment (guchar      *col, 
			 PixelRegion *src,
			 PixelRegion *mask, 
			 gint         width, 
			 gint         bytes,
			 gboolean     has_alpha, 
			 gboolean     antialias, 
			 gint         threshold,
			 gint         initial, 
			 gint        *start, 
			 gint        *end)
{
  guchar *s;
  guchar *m;
  guchar  diff;
  Tile   *s_tile = NULL;
  Tile   *m_tile = NULL;

  ref_tiles (src->tiles, mask->tiles, &s_tile, &m_tile, src->x, src->y, &s, &m);

  /* check the starting pixel */
  if (! (diff = is_pixel_sufficiently_different (col, s, antialias,
						 threshold, bytes, has_alpha)))
    {
      tile_release (s_tile, FALSE);
      tile_release (m_tile, TRUE);
      return FALSE;
    }

  *m-- = diff;
  s -= bytes;
  *start = initial - 1;

  while (*start >= 0 && diff)
    {
      if (! ((*start + 1) % TILE_WIDTH))
	ref_tiles (src->tiles, mask->tiles, &s_tile, &m_tile, *start, src->y, &s, &m);

      diff = is_pixel_sufficiently_different (col, s, antialias,
					      threshold, bytes, has_alpha);
      if ((*m-- = diff))
	{
	  s -= bytes;
	  (*start)--;
	}
    }

  diff = 1;
  *end = initial + 1;
  if (*end % TILE_WIDTH && *end < width)
    ref_tiles (src->tiles, mask->tiles, &s_tile, &m_tile, *end, src->y, &s, &m);

  while (*end < width && diff)
    {
      if (! (*end % TILE_WIDTH))
	ref_tiles (src->tiles, mask->tiles, &s_tile, &m_tile, *end, src->y, &s, &m);

      diff = is_pixel_sufficiently_different (col, s, antialias,
					      threshold, bytes, has_alpha);
      if ((*m++ = diff))
	{
	  s += bytes;
	  (*end)++;
	}
    }

  tile_release (s_tile, FALSE);
  tile_release (m_tile, TRUE);
  return TRUE;
}

static void
find_contiguous_region_helper (PixelRegion *mask, 
			       PixelRegion *src,
			       gboolean     has_alpha, 
			       gboolean     antialias, 
			       gint         threshold, 
			       gboolean     indexed,
			       gint         x, 
			       gint         y, 
			       guchar      *col)
{
  gint start, end, i;
  gint val;
  gint bytes;

  Tile *tile;

  if (threshold == 0) threshold = 1;
  if (x < 0 || x >= src->w) return;
  if (y < 0 || y >= src->h) return;

  tile = tile_manager_get_tile (mask->tiles, x, y, TRUE, FALSE);
  val = *(guchar *)(tile_data_pointer (tile, 
				       x % TILE_WIDTH, y % TILE_HEIGHT));
  tile_release (tile, FALSE);
  if (val != 0)
    return;

  src->x = x;
  src->y = y;

  bytes = src->bytes;
  if(indexed)
    {
      bytes = has_alpha ? 4 : 3;
    }

  if (! find_contiguous_segment (col, src, mask, src->w,
				 src->bytes, has_alpha,
				 antialias, threshold, x, &start, &end))
    return;

  for (i = start + 1; i < end; i++)
    {
      find_contiguous_region_helper (mask, src, has_alpha, antialias, 
				     threshold, indexed, i, y - 1, col);
      find_contiguous_region_helper (mask, src, has_alpha, antialias, 
				     threshold, indexed, i, y + 1, col);
    }
}

GimpChannel *
find_contiguous_region (GimpImage    *gimage, 
			GimpDrawable *drawable, 
			gboolean      antialias,
			gint          threshold, 
			gint          x, 
			gint          y, 
			gboolean      sample_merged)
{
  PixelRegion  srcPR, maskPR;
  GimpChannel *mask;
  guchar      *start;
  gboolean     has_alpha;
  gboolean     indexed;
  gint         type;
  gint         bytes;
  Tile        *tile;

  if (sample_merged)
    {
      pixel_region_init (&srcPR, gimp_image_composite (gimage), 0, 0,
			 gimage->width, gimage->height, FALSE);
      type = gimp_image_composite_type (gimage);
      has_alpha = (type == RGBA_GIMAGE ||
		   type == GRAYA_GIMAGE ||
		   type == INDEXEDA_GIMAGE);
    }
  else
    {
      pixel_region_init (&srcPR, gimp_drawable_data (drawable),
			 0, 0,
			 gimp_drawable_width (drawable),
			 gimp_drawable_height (drawable),
			 FALSE);
      has_alpha = gimp_drawable_has_alpha (drawable);
    }
  indexed = gimp_drawable_is_indexed (drawable);
  bytes   = gimp_drawable_bytes (drawable);
  
  if (indexed)
    {
      bytes = has_alpha ? 4 : 3;
    }
  mask = gimp_channel_new_mask (gimage, srcPR.w, srcPR.h);
  pixel_region_init (&maskPR, gimp_drawable_data (GIMP_DRAWABLE(mask)),
		     0, 0, 
		     gimp_drawable_width (GIMP_DRAWABLE(mask)), 
		     gimp_drawable_height (GIMP_DRAWABLE(mask)), 
		     TRUE);

  tile = tile_manager_get_tile (srcPR.tiles, x, y, TRUE, FALSE);
  if (tile)
    {
      start = tile_data_pointer (tile, x%TILE_WIDTH, y%TILE_HEIGHT);

      find_contiguous_region_helper (&maskPR, &srcPR, has_alpha, antialias, 
				     threshold, bytes, x, y, start);

      tile_release (tile, FALSE);
    }

  return mask;
}

void
fuzzy_select (GimpImage    *gimage, 
	      GimpDrawable *drawable, 
	      gint          op, 
	      gboolean      feather,
	      gdouble       feather_radius)
{
  gint off_x, off_y;

  /*  if applicable, replace the current selection  */
  if (op == CHANNEL_OP_REPLACE)
    gimage_mask_clear (gimage);
  else
    gimage_mask_undo (gimage);

  if (drawable)     /* NULL if sample_merged is active */
    gimp_drawable_offsets (drawable, &off_x, &off_y);
  else
    off_x = off_y = 0;
  
  if (feather)
    gimp_channel_feather (fuzzy_mask, gimp_image_get_mask (gimage),
			  feather_radius,
			  feather_radius,
			  op, off_x, off_y);
  else
    gimp_channel_combine_mask (gimp_image_get_mask (gimage),
			       fuzzy_mask, op, off_x, off_y);

  g_object_unref (G_OBJECT (fuzzy_mask));
  fuzzy_mask = NULL;
}

/*  fuzzy select action functions  */

static void
gimp_fuzzy_select_tool_class_init (GimpFuzzySelectToolClass *klass)
{
  GtkObjectClass    *object_class;
  GimpToolClass     *tool_class;
  GimpDrawToolClass *draw_tool_class;

  object_class    = (GtkObjectClass *) klass;
  tool_class      = (GimpToolClass *) klass;
  draw_tool_class = (GimpDrawToolClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_SELECTION_TOOL);

  object_class->destroy      = gimp_fuzzy_select_tool_destroy;

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

  fuzzy_select->x               = 0;
  fuzzy_select->y               = 0;
  fuzzy_select->first_x         = 0;
  fuzzy_select->first_y         = 0;
  fuzzy_select->first_threshold = 0.0;
}

static void
gimp_fuzzy_select_tool_destroy (GtkObject *object)
{
  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_fuzzy_select_tool_button_press (GimpTool       *tool, 
                                     GdkEventButton *bevent,
                                     GDisplay       *gdisp)
{
  GimpFuzzySelectTool *fuzzy_sel;

  fuzzy_sel = GIMP_FUZZY_SELECT_TOOL (tool);

  gdk_pointer_grab (gdisp->canvas->window, FALSE,
		    GDK_POINTER_MOTION_HINT_MASK |
		    GDK_BUTTON1_MOTION_MASK |
		    GDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, bevent->time);

  tool->state = ACTIVE;
  tool->gdisp = gdisp;

  fuzzy_sel->x               = bevent->x;
  fuzzy_sel->y               = bevent->y;
  fuzzy_sel->first_x         = fuzzy_sel->x;
  fuzzy_sel->first_y         = fuzzy_sel->y;
  fuzzy_sel->first_threshold = fuzzy_options->threshold;

  switch (GIMP_SELECTION_TOOL (tool)->op)
    {
    case SELECTION_MOVE_MASK:
      init_edit_selection (tool, gdisp, bevent, EDIT_MASK_TRANSLATE);
      return;
    case SELECTION_MOVE:
      init_edit_selection (tool, gdisp, bevent, EDIT_MASK_TO_LAYER_TRANSLATE);
      return;
    default:
      break;
    }

  /*  calculate the region boundary  */
  segs = fuzzy_select_calculate (fuzzy_sel, gdisp, &num_segs);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool),
                        gdisp->canvas->window);
}

static void
gimp_fuzzy_select_tool_button_release (GimpTool       *tool, 
                                       GdkEventButton *bevent,
                                       GDisplay       *gdisp)
{
  GimpFuzzySelectTool *fuzzy_sel;
  GimpDrawable        *drawable;

  fuzzy_sel = GIMP_FUZZY_SELECT_TOOL (tool);

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));
  tool->state = INACTIVE;

  /*  First take care of the case where the user "cancels" the action  */
  if (! (bevent->state & GDK_BUTTON3_MASK))
    {
      drawable = (fuzzy_options->sample_merged ?
		  NULL : gimp_image_active_drawable (gdisp->gimage));

      fuzzy_select (gdisp->gimage,
                    drawable,
                    GIMP_SELECTION_TOOL (tool)->op,
		    fuzzy_options->feather,
		    fuzzy_options->feather_radius);

      gdisplays_flush ();
    }

  /*  If the segment array is allocated, free it  */
  if (segs)
    g_free (segs);
  segs = NULL;
}

static void
gimp_fuzzy_select_tool_motion (GimpTool       *tool, 
                               GdkEventMotion *mevent, 
                               GDisplay       *gdisp)
{
  GimpFuzzySelectTool *fuzzy_sel;
  GimpSelectionTool   *sel_tool;
  GdkSegment          *new_segs;
  gint                 num_new_segs;
  gint                 diff_x, diff_y;
  gdouble              diff;

  static guint last_time = 0;

  fuzzy_sel = GIMP_FUZZY_SELECT_TOOL (tool);
  sel_tool  = GIMP_SELECTION_TOOL (tool);

  /*  needed for immediate cursor update on modifier event  */
  sel_tool->current_x = mevent->x;
  sel_tool->current_y = mevent->y;

  if (tool->state != ACTIVE)
    return;

  /* don't let the events come in too fast, ignore below a delay of 100 ms */
  if (ABS (mevent->time - last_time) < 100)
    return;

  last_time = mevent->time;

  diff_x = mevent->x - fuzzy_sel->first_x;
  diff_y = mevent->y - fuzzy_sel->first_y;

  diff = ((ABS (diff_x) > ABS (diff_y)) ? diff_x : diff_y) / 2.0;

  gtk_adjustment_set_value (GTK_ADJUSTMENT (fuzzy_options->threshold_w), 
			    fuzzy_sel->first_threshold + diff);

  /*  calculate the new fuzzy boundary  */
  new_segs = fuzzy_select_calculate (fuzzy_sel, gdisp, &num_new_segs);

  /*  stop the current boundary  */
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  /*  make sure the XSegment array is freed before we assign the new one  */
  if (segs)
    g_free (segs);
  segs = new_segs;
  num_segs = num_new_segs;

  /*  start the new boundary  */
  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}


static GdkSegment *
fuzzy_select_calculate (GimpFuzzySelectTool *fuzzy_sel,
			GDisplay            *gdisp,
			gint                *nsegs)
{
  PixelRegion   maskPR;
  GimpChannel  *new;
  GdkSegment   *segs;
  BoundSeg     *bsegs;
  GimpDrawable *drawable;
  gint          i;
  gint          x, y;
  gboolean      use_offsets;

  drawable  = gimp_image_active_drawable (gdisp->gimage);

  gimp_set_busy (gdisp->gimage->gimp);

  use_offsets = fuzzy_options->sample_merged ? FALSE : TRUE;

  gdisplay_untransform_coords (gdisp, fuzzy_sel->x,
			       fuzzy_sel->y, &x, &y, FALSE, use_offsets);

  new = find_contiguous_region (gdisp->gimage, drawable, 
				fuzzy_options->antialias,
				fuzzy_options->threshold, x, y, 
				fuzzy_options->sample_merged);

  if (fuzzy_mask)
    g_object_unref (G_OBJECT (fuzzy_mask));

  fuzzy_mask = new;

  /*  calculate and allocate a new XSegment array which represents the boundary
   *  of the color-contiguous region
   */
  pixel_region_init (&maskPR, gimp_drawable_data (GIMP_DRAWABLE (fuzzy_mask)),
		     0, 0, 
		     gimp_drawable_width (GIMP_DRAWABLE (fuzzy_mask)), 
		     gimp_drawable_height (GIMP_DRAWABLE (fuzzy_mask)), 
		     FALSE);
  bsegs = find_mask_boundary (&maskPR, nsegs, WithinBounds,
			      0, 0,
			      gimp_drawable_width (GIMP_DRAWABLE (fuzzy_mask)),
			      gimp_drawable_height (GIMP_DRAWABLE (fuzzy_mask)));

  segs = g_new (GdkSegment, *nsegs);

  for (i = 0; i < *nsegs; i++)
    {
      gdisplay_transform_coords (gdisp, bsegs[i].x1, bsegs[i].y1, &x, &y, use_offsets);
      segs[i].x1 = x;  segs[i].y1 = y;
      gdisplay_transform_coords (gdisp, bsegs[i].x2, bsegs[i].y2, &x, &y, use_offsets);
      segs[i].x2 = x;  segs[i].y2 = y;
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
