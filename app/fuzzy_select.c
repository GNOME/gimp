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
#include <stdlib.h>
#include <math.h>
#include "appenv.h"
#include "boundary.h"
#include "draw_core.h"
#include "drawable.h"
#include "edit_selection.h"
#include "fuzzy_select.h"
#include "gimage_mask.h"
#include "gimprc.h"
#include "gdisplay.h"
#include "rect_select.h"

#include "tile.h"			/* ick. */

#define NO  0
#define YES 1

typedef struct _fuzzy_select FuzzySelect;

struct _fuzzy_select
{
  DrawCore *     core;         /*  Core select object                      */

  int            x, y;         /*  Point from which to execute seed fill  */
  int            last_x;       /*                                         */
  int            last_y;       /*  variables to keep track of sensitivity */
  int            threshold;    /*  threshold value for soft seed fill     */

  int            op;           /*  selection operation (ADD, SUB, etc)     */
};


/*  fuzzy select action functions  */

static void   fuzzy_select_button_press   (Tool *, GdkEventButton *, gpointer);
static void   fuzzy_select_button_release (Tool *, GdkEventButton *, gpointer);
static void   fuzzy_select_motion         (Tool *, GdkEventMotion *, gpointer);
static void   fuzzy_select_draw           (Tool *);
static void   fuzzy_select_control        (Tool *, int, gpointer);

/*  fuzzy select action functions  */
static GdkSegment *   fuzzy_select_calculate (Tool *, void *, int *);


/*  XSegments which make up the fuzzy selection boundary  */

static GdkSegment *segs = NULL;
static int         num_segs = 0;
static Channel *   fuzzy_mask = NULL;
static SelectionOptions *fuzzy_options = NULL;

static void fuzzy_select (GImage *, GimpDrawable *, int, int, double);
static Argument *fuzzy_select_invoker (Argument *);

/*************************************/
/*  Fuzzy selection apparatus  */

static int
is_pixel_sufficiently_different (unsigned char *col1, unsigned char *col2,
				 int antialias, int threshold, int bytes,
				 int has_alpha)
{
  int diff;
  int max;
  int b;
  int alpha;

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
ref_tiles (TileManager *src, TileManager *mask, Tile **s_tile, Tile **m_tile,
	   int x, int y, unsigned char **s, unsigned char **m)
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
find_contiguous_segment (unsigned char *col, PixelRegion *src,
			 PixelRegion *mask, int width, int bytes,
			 int has_alpha, int antialias, int threshold,
			 int initial, int *start, int *end)
{
  unsigned char *s, *m;
  unsigned char diff;
  Tile *s_tile = NULL;
  Tile *m_tile = NULL;

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
find_contiguous_region_helper (PixelRegion *mask, PixelRegion *src,
			       int has_alpha, int antialias, int threshold, int indexed,
			       int x, int y, unsigned char *col)
{
  int start, end, i;
  int val;
  int bytes;

  Tile *tile;

  if (threshold == 0) threshold = 1;
  if (x < 0 || x >= src->w) return;
  if (y < 0 || y >= src->h) return;

  tile = tile_manager_get_tile (mask->tiles, x, y, TRUE, FALSE);
  val = *(unsigned char *)(tile_data_pointer (tile, 
					      x%TILE_WIDTH, y%TILE_HEIGHT));
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
    

  if ( ! find_contiguous_segment (col, src, mask, src->w,
				  src->bytes, has_alpha,
				  antialias, threshold, x, &start, &end))
    return;

  for (i = start + 1; i < end; i++)
    {
      find_contiguous_region_helper (mask, src, has_alpha, antialias, threshold, indexed, i, y - 1, col);
      find_contiguous_region_helper (mask, src, has_alpha, antialias, threshold, indexed, i, y + 1, col);
    }
}

Channel *
find_contiguous_region (GImage *gimage, GimpDrawable *drawable, int antialias,
			int threshold, int x, int y, int sample_merged)
{
  PixelRegion srcPR, maskPR;
  Channel *mask;
  unsigned char *start;
  int has_alpha;
  int indexed;
  int type;
  int bytes;
  Tile *tile;

  if (sample_merged)
    {
      pixel_region_init (&srcPR, gimage_composite (gimage), 0, 0,
			 gimage->width, gimage->height, FALSE);
      type = gimage_composite_type (gimage);
      has_alpha = (type == RGBA_GIMAGE ||
		   type == GRAYA_GIMAGE ||
		   type == INDEXEDA_GIMAGE);
    }
  else
    {
      pixel_region_init (&srcPR, drawable_data (drawable), 0, 0,
			 drawable_width (drawable), drawable_height (drawable), FALSE);
      has_alpha = drawable_has_alpha (drawable);
    }
  indexed = drawable_indexed (drawable);
  bytes = drawable_bytes (drawable);
  
  if(indexed)
    {
      bytes = has_alpha ? 4 : 3;
    }
  mask = channel_new_mask (gimage, srcPR.w, srcPR.h);
  pixel_region_init (&maskPR, drawable_data (GIMP_DRAWABLE(mask)), 0, 0, drawable_width (GIMP_DRAWABLE(mask)), drawable_height (GIMP_DRAWABLE(mask)), TRUE);

  tile = tile_manager_get_tile (srcPR.tiles, x, y, TRUE, FALSE);
  if (tile)
    {
      start = tile_data_pointer (tile, x%TILE_WIDTH, y%TILE_HEIGHT);

      find_contiguous_region_helper (&maskPR, &srcPR, has_alpha, antialias, threshold, bytes, x, y, start);

      tile_release (tile, FALSE);
    }

  return mask;
}

static void
fuzzy_select (GImage *gimage, GimpDrawable *drawable, int op, int feather,
	      double feather_radius)
{
  int off_x, off_y;

  /*  if applicable, replace the current selection  */
  if (op == REPLACE)
    gimage_mask_clear (gimage);
  else
    gimage_mask_undo (gimage);

  drawable_offsets (drawable, &off_x, &off_y);
  if (feather)
    channel_feather (fuzzy_mask, gimage_get_mask (gimage),
		     feather_radius, op, off_x, off_y);
  else
    channel_combine_mask (gimage_get_mask (gimage),
			  fuzzy_mask, op, off_x, off_y);

  /*  free the fuzzy region struct  */
  channel_delete (fuzzy_mask);
  fuzzy_mask = NULL;
}

/*  fuzzy select action functions  */

static void
fuzzy_select_button_press (Tool *tool, GdkEventButton *bevent,
			   gpointer gdisp_ptr)
{
  GDisplay *gdisp;
  FuzzySelect *fuzzy_sel;

  gdisp = (GDisplay *) gdisp_ptr;
  fuzzy_sel = (FuzzySelect *) tool->private;

  fuzzy_sel->x = bevent->x;
  fuzzy_sel->y = bevent->y;
  fuzzy_sel->last_x = fuzzy_sel->x;
  fuzzy_sel->last_y = fuzzy_sel->y;
  fuzzy_sel->threshold = default_threshold;

  gdk_pointer_grab (gdisp->canvas->window, FALSE,
		    GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, bevent->time);

  tool->state = ACTIVE;
  tool->gdisp_ptr = gdisp_ptr;

  if (bevent->state & GDK_MOD1_MASK)
    {
      init_edit_selection (tool, gdisp_ptr, bevent, MaskTranslate);
      return;
    }
  else if ((bevent->state & GDK_SHIFT_MASK) && !(bevent->state & GDK_CONTROL_MASK))
    fuzzy_sel->op = ADD;
  else if ((bevent->state & GDK_CONTROL_MASK) && !(bevent->state & GDK_SHIFT_MASK))
    fuzzy_sel->op = SUB;
  else if ((bevent->state & GDK_CONTROL_MASK) && (bevent->state & GDK_SHIFT_MASK))
    fuzzy_sel->op = INTERSECT;
  else
    {
      if (! (layer_is_floating_sel (gimage_get_active_layer (gdisp->gimage))) &&
	  gdisplay_mask_value (gdisp, bevent->x, bevent->y) > HALF_WAY)
	{
	  init_edit_selection (tool, gdisp_ptr, bevent, MaskToLayerTranslate);
	  return;
	}
      fuzzy_sel->op = REPLACE;
    }

  /*  calculate the region boundary  */
  segs = fuzzy_select_calculate (tool, gdisp_ptr, &num_segs);

  draw_core_start (fuzzy_sel->core,
		   gdisp->canvas->window,
		   tool);
}

static void
fuzzy_select_button_release (Tool *tool, GdkEventButton *bevent,
			     gpointer gdisp_ptr)
{
  FuzzySelect * fuzzy_sel;
  GDisplay * gdisp;
  GimpDrawable *drawable;

  gdisp = (GDisplay *) gdisp_ptr;
  fuzzy_sel = (FuzzySelect *) tool->private;

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  draw_core_stop (fuzzy_sel->core, tool);
  tool->state = INACTIVE;

  /*  First take care of the case where the user "cancels" the action  */
  if (! (bevent->state & GDK_BUTTON3_MASK))
    {
      drawable = (fuzzy_options->sample_merged) ? NULL : gimage_active_drawable (gdisp->gimage);
      fuzzy_select (gdisp->gimage, drawable, fuzzy_sel->op,
		    fuzzy_options->feather, fuzzy_options->feather_radius);

      gdisplays_flush ();

      /*  adapt the threshold based on the final value of this use  */
      default_threshold = fuzzy_sel->threshold;
    }

  /*  If the segment array is allocated, free it  */
  if (segs)
    g_free (segs);
  segs = NULL;
}

static void
fuzzy_select_motion (Tool *tool, GdkEventMotion *mevent, gpointer gdisp_ptr)
{
  FuzzySelect * fuzzy_sel;
  GdkSegment * new_segs;
  int num_new_segs;
  int diff, diff_x, diff_y;

  if (tool->state != ACTIVE)
    return;

  fuzzy_sel = (FuzzySelect *) tool->private;

  diff_x = mevent->x - fuzzy_sel->last_x;
  diff_y = mevent->y - fuzzy_sel->last_y;

  diff = ((ABS (diff_x) > ABS (diff_y)) ? diff_x : diff_y) / 2;

  fuzzy_sel->last_x = mevent->x;
  fuzzy_sel->last_y = mevent->y;

  fuzzy_sel->threshold += diff;
  fuzzy_sel->threshold = BOUNDS (fuzzy_sel->threshold, 0, 255);

  /*  calculate the new fuzzy boundary  */
  new_segs = fuzzy_select_calculate (tool, gdisp_ptr, &num_new_segs);

  /*  stop the current boundary  */
  draw_core_pause (fuzzy_sel->core, tool);

  /*  make sure the XSegment array is freed before we assign the new one  */
  if (segs)
    g_free (segs);
  segs = new_segs;
  num_segs = num_new_segs;

  /*  start the new boundary  */
  draw_core_resume (fuzzy_sel->core, tool);
}

static GdkSegment *
fuzzy_select_calculate (Tool *tool, void *gdisp_ptr, int *nsegs)
{
  PixelRegion maskPR;
  FuzzySelect *fuzzy_sel;
  GDisplay *gdisp;
  Channel *new;
  GdkSegment *segs;
  BoundSeg *bsegs;
  int i, x, y;
  GimpDrawable *drawable;
  int use_offsets;

  fuzzy_sel = (FuzzySelect *) tool->private;
  gdisp = (GDisplay *) gdisp_ptr;
  drawable = gimage_active_drawable (gdisp->gimage);

  use_offsets = (fuzzy_options->sample_merged) ? FALSE : TRUE;

  gdisplay_untransform_coords (gdisp, fuzzy_sel->x,
			       fuzzy_sel->y, &x, &y, FALSE, use_offsets);

  new = find_contiguous_region (gdisp->gimage, drawable, fuzzy_options->antialias,
				fuzzy_sel->threshold, x, y, fuzzy_options->sample_merged);

  if (fuzzy_mask)
    channel_delete (fuzzy_mask);
  fuzzy_mask = channel_ref (new);

  /*  calculate and allocate a new XSegment array which represents the boundary
   *  of the color-contiguous region
   */
  pixel_region_init (&maskPR, drawable_data (GIMP_DRAWABLE(fuzzy_mask)), 0, 0, drawable_width (GIMP_DRAWABLE(fuzzy_mask)), drawable_height (GIMP_DRAWABLE(fuzzy_mask)), FALSE);
  bsegs = find_mask_boundary (&maskPR, nsegs, WithinBounds,
			      0, 0,
			      drawable_width (GIMP_DRAWABLE(fuzzy_mask)),
			      drawable_height (GIMP_DRAWABLE(fuzzy_mask)));

  segs = (GdkSegment *) g_malloc (sizeof (GdkSegment) * *nsegs);

  for (i = 0; i < *nsegs; i++)
    {
      gdisplay_transform_coords (gdisp, bsegs[i].x1, bsegs[i].y1, &x, &y, use_offsets);
      segs[i].x1 = x;  segs[i].y1 = y;
      gdisplay_transform_coords (gdisp, bsegs[i].x2, bsegs[i].y2, &x, &y, use_offsets);
      segs[i].x2 = x;  segs[i].y2 = y;
    }

  /*  free boundary segments  */
  g_free (bsegs);

  return segs;
}

static void
fuzzy_select_draw (Tool *tool)
{
  FuzzySelect * fuzzy_sel;

  fuzzy_sel = (FuzzySelect *) tool->private;

  if (segs)
    gdk_draw_segments (fuzzy_sel->core->win, fuzzy_sel->core->gc, segs, num_segs);
}

static void
fuzzy_select_control (Tool *tool, int action, gpointer gdisp_ptr)
{
  FuzzySelect * fuzzy_sel;

  fuzzy_sel = (FuzzySelect *) tool->private;

  switch (action)
    {
    case PAUSE :
      draw_core_pause (fuzzy_sel->core, tool);
      break;
    case RESUME :
      draw_core_resume (fuzzy_sel->core, tool);
      break;
    case HALT :
      draw_core_stop (fuzzy_sel->core, tool);
      break;
    }
}

Tool *
tools_new_fuzzy_select (void)
{
  Tool * tool;
  FuzzySelect * private;

  /*  The tool options  */
  if (!fuzzy_options)
    fuzzy_options = create_selection_options (FUZZY_SELECT);

  tool = (Tool *) g_malloc (sizeof (Tool));
  private = (FuzzySelect *) g_malloc (sizeof (FuzzySelect));

  private->core = draw_core_new (fuzzy_select_draw);

  tool->type = FUZZY_SELECT;
  tool->state = INACTIVE;
  tool->scroll_lock = 1;  /*  Disallow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->private = (void *) private;
  tool->button_press_func = fuzzy_select_button_press;
  tool->button_release_func = fuzzy_select_button_release;
  tool->motion_func = fuzzy_select_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = rect_select_cursor_update;
  tool->control_func = fuzzy_select_control;

  return tool;
}

void
tools_free_fuzzy_select (Tool *tool)
{
  FuzzySelect * fuzzy_sel;

  fuzzy_sel = (FuzzySelect *) tool->private;
  draw_core_free (fuzzy_sel->core);
  g_free (fuzzy_sel);
}

/*  The fuzzy_select procedure definition  */
ProcArg fuzzy_select_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_FLOAT,
    "x",
    "x coordinate of initial seed fill point: (image coordinates)"
  },
  { PDB_FLOAT,
    "y",
    "y coordinate of initial seed fill point: (image coordinates)"
  },
  { PDB_INT32,
    "threshold",
    "threshold in intensity levels: 0 <= threshold <= 255"
  },
  { PDB_INT32,
    "operation",
    "the selection operation: { ADD (0), SUB (1), REPLACE (2), INTERSECT (3) }"
  },
  { PDB_INT32,
    "antialias",
    "antialiasing On/Off"
  },
  { PDB_INT32,
    "feather",
    "feather option for selections"
  },
  { PDB_FLOAT,
    "feather_radius",
    "radius for feather operation"
  },
  { PDB_INT32,
    "sample_merged",
    "use the composite image, not the drawable"
  }
};

ProcRecord fuzzy_select_proc =
{
  "gimp_fuzzy_select",
  "Create a fuzzy selection starting at the specified coordinates on the specified drawable",
  "This tool creates a fuzzy selection over the specified image.  A fuzzy selection is determined by a seed fill under the constraints of the specified threshold.  Essentially, the color at the specified coordinates (in the drawable) is measured and the selection expands outwards from that point to any adjacent pixels which are not significantly different (as determined by the threshold value).  This process continues until no more expansion is possible.  The antialiasing parameter allows the final selection mask to contain intermediate values based on close misses to the threshold bar at pixels along the seed fill boundary.  Feathering can be enabled optionally and is controlled with the \"feather_radius\" paramter.  If the sample_merged parameter is non-zero, the data of the composite image will be used instead of that for the specified drawable.  This is equivalent to sampling for colors after merging all visible layers.  In the case of a merged sampling, the supplied drawable is ignored.  If the sample is merged, the specified coordinates are relative to the image origin; otherwise, they are relative to the drawable's origin.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  10,
  fuzzy_select_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { fuzzy_select_invoker } },
};


static Argument *
fuzzy_select_invoker (Argument *args)
{
  int success = TRUE;
  GImage *gimage;
  GimpDrawable *drawable;
  int op;
  int threshold;
  int antialias;
  int feather;
  int sample_merged;
  double x, y;
  double feather_radius;
  int int_value;

  drawable    = NULL;
  op          = REPLACE;
  threshold   = 0;

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
	success = FALSE;
    }
  /*  the drawable  */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL || gimage != drawable_gimage (drawable))
	success = FALSE;
    }
  /*  x, y  */
  if (success)
    {
      x = args[2].value.pdb_float;
      y = args[3].value.pdb_float;
    }
  /*  threshold  */
  if (success)
    {
      int_value = args[4].value.pdb_int;
      if (int_value >= 0 && int_value <= 255)
	threshold = int_value;
      else
	success = FALSE;
    }
  /*  operation  */
  if (success)
    {
      int_value = args[5].value.pdb_int;
      switch (int_value)
	{
	case 0: op = ADD; break;
	case 1: op = SUB; break;
	case 2: op = REPLACE; break;
	case 3: op = INTERSECT; break;
	default: success = FALSE;
	}
    }
  /*  antialiasing?  */
  if (success)
    {
      int_value = args[6].value.pdb_int;
      antialias = (int_value) ? TRUE : FALSE;
    }
  /*  feathering  */
  if (success)
    {
      int_value = args[7].value.pdb_int;
      feather = (int_value) ? TRUE : FALSE;
    }
  /*  feather radius  */
  if (success)
    {
      feather_radius = args[8].value.pdb_float;
    }
  /*  sample merged  */
  if (success)
    {
      int_value = args[9].value.pdb_int;
      sample_merged = (int_value) ? TRUE : FALSE;
    }

  /*  call the fuzzy_select procedure  */
  if (success)
    {
      Channel *new;
      Channel *old_fuzzy_mask;

      new = find_contiguous_region (gimage, drawable, antialias, threshold, x, y, sample_merged);
      old_fuzzy_mask = fuzzy_mask;
      fuzzy_mask = new;

      drawable = (sample_merged) ? NULL : drawable;
      fuzzy_select (gimage, drawable, op, feather, feather_radius);

      fuzzy_mask = old_fuzzy_mask;
    }

  return procedural_db_return_args (&fuzzy_select_proc, success);
}
