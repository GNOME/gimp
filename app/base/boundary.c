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

#include <string.h>

#include <glib-object.h>

#include "libgimpmath/gimpmath.h"

#include "base-types.h"

#include "boundary.h"
#include "pixel-region.h"
#include "tile.h"
#include "tile-manager.h"


/* BoundSeg array growth parameter */
#define MAX_SEGS_INC  2048


/*  The array of vertical segments  */
static gint      *vert_segs      = NULL;

/*  The array of segments  */
static BoundSeg  *tmp_segs       = NULL;
static gint       num_segs       = 0;
static gint       max_segs       = 0;

/* static empty segment arrays */
static gint      *empty_segs_n   = NULL;
static gint      *empty_segs_c   = NULL;
static gint      *empty_segs_l   = NULL;
static gint       max_empty_segs = 0;


/*  local function prototypes  */
static void find_empty_segs     (PixelRegion  *maskPR,
				 gint          scanline,
				 gint          empty_segs[],
				 gint          max_empty,
				 gint         *num_empty,
				 BoundaryType  type,
				 gint          x1,
				 gint          y1,
				 gint          x2,
				 gint          y2,
                                 guchar        threshold);
static void make_seg            (gint          x1,
				 gint          y1,
				 gint          x2,
				 gint          y2,
				 gboolean      open);
static void allocate_vert_segs  (PixelRegion  *PR);
static void allocate_empty_segs (PixelRegion  *PR);
static void process_horiz_seg   (gint          x1,
				 gint          y1,
				 gint          x2,
				 gint          y2,
				 gboolean      open);
static void make_horiz_segs     (gint          start,
				 gint          end,
				 gint          scanline,
				 gint          empty[],
				 gint          num_empty,
				 gint          top);
static void generate_boundary   (PixelRegion  *PR,
				 BoundaryType  type,
				 gint          x1,
				 gint          y1,
				 gint          x2,
				 gint          y2,
                                 guchar        threshold);
static void simplify_subdivide  (const         BoundSeg *segs,
                                 gint          start_idx,
                                 gint          end_idx,
                                 GArray      **ret_points);

/*  Function definitions  */

static void
find_empty_segs (PixelRegion  *maskPR,
		 gint          scanline,
		 gint          empty_segs[],
		 gint          max_empty,
		 gint         *num_empty,
		 BoundaryType  type,
		 gint          x1,
		 gint          y1,
		 gint          x2,
		 gint          y2,
                 guchar        threshold)
{
  guchar *data;
  gint    x;
  gint    start, end;
  gint    val, last;
  gint    tilex;
  Tile   *tile = NULL;
  gint    endx;
  gint    l_num_empty;
  gint    dstep = 0;

  data  = NULL;
  start = 0;
  end   = 0;
  endx  = 0;

  *num_empty = 0;

  if (scanline < maskPR->y || scanline >= (maskPR->y + maskPR->h))
    {
      empty_segs[(*num_empty)++] = 0;
      empty_segs[(*num_empty)++] = G_MAXINT;
      return;
    }

  if (type == WithinBounds)
    {
      if (scanline < y1 || scanline >= y2)
	{
	  empty_segs[(*num_empty)++] = 0;
	  empty_segs[(*num_empty)++] = G_MAXINT;
	  return;
	}

      start = x1;
      end   = x2;
    }
  else if (type == IgnoreBounds)
    {
      start = maskPR->x;
      end   = maskPR->x + maskPR->w;
      if (scanline < y1 || scanline >= y2)
	x2 = -1;
    }

  tilex = -1;
  empty_segs[(*num_empty)++] = 0;
  last = -1;

  l_num_empty = *num_empty;

  if (! maskPR->tiles)
    {
      data  = maskPR->data + scanline * maskPR->rowstride;
      dstep = maskPR->bytes;

      endx = end;
    }

  for (x = start; x < end;)
    {
      /*  Check to see if we must advance to next tile  */
      if (maskPR->tiles)
        {
          if ((x / TILE_WIDTH) != tilex)
            {
              if (tile)
                tile_release (tile, FALSE);
              tile = tile_manager_get_tile (maskPR->tiles,
					    x, scanline, TRUE, FALSE);
              data =
		(guchar *) tile_data_pointer (tile,
					      x % TILE_WIDTH,
					      scanline % TILE_HEIGHT) +
		(tile_bpp(tile) - 1);

              tilex = x / TILE_WIDTH;
              dstep = tile_bpp (tile);
            }

          endx = x + (TILE_WIDTH - (x%TILE_WIDTH));
          endx = MIN (end, endx);
        }

      if (type == IgnoreBounds && (endx > x1 || x < x2))
	{
	  for (; x < endx; x++)
	    {
	      if (*data > threshold)
		if (x >= x1 && x < x2)
		  val = -1;
		else
		  val = 1;
	      else
		val = -1;

	      data += dstep;

	      if (last != val)
		empty_segs[l_num_empty++] = x;

	      last = val;
	    }
	}
      else
	{
	  for (; x < endx; x++)
	    {
	      if (*data > threshold)
		val = 1;
	      else
		val = -1;

	      data += dstep;

	      if (last != val)
		empty_segs[l_num_empty++] = x;

	      last = val;
	    }
	}
    }
  *num_empty = l_num_empty;

  if (last > 0)
    empty_segs[(*num_empty)++] = x;

  empty_segs[(*num_empty)++] = G_MAXINT;

  if (tile)
    tile_release (tile, FALSE);
}


static void
make_seg (gint     x1,
	  gint     y1,
	  gint     x2,
	  gint     y2,
	  gboolean open)
{
  if (num_segs >= max_segs)
    {
      max_segs += MAX_SEGS_INC;

      tmp_segs = g_renew (BoundSeg, tmp_segs, max_segs);
    }

  tmp_segs[num_segs].x1 = x1;
  tmp_segs[num_segs].y1 = y1;
  tmp_segs[num_segs].x2 = x2;
  tmp_segs[num_segs].y2 = y2;
  tmp_segs[num_segs].open = open;
  num_segs ++;
}


static void
allocate_vert_segs (PixelRegion *PR)
{
  gint i;

  /*  allocate and initialize the vert_segs array  */
  vert_segs = g_renew (gint, vert_segs, PR->w + PR->x + 1);

  for (i = 0; i <= (PR->w + PR->x); i++)
    vert_segs[i] = -1;
}


static void
allocate_empty_segs (PixelRegion *PR)
{
  gint need_num_segs;

  /*  find the maximum possible number of empty segments given the current mask  */
  need_num_segs = PR->w + 3;

  if (need_num_segs > max_empty_segs)
    {
      max_empty_segs = need_num_segs;

      empty_segs_n = g_renew (gint, empty_segs_n, max_empty_segs);
      empty_segs_c = g_renew (gint, empty_segs_c, max_empty_segs);
      empty_segs_l = g_renew (gint, empty_segs_l, max_empty_segs);
    }
}


static void
process_horiz_seg (gint     x1,
		   gint     y1,
		   gint     x2,
		   gint     y2,
		   gboolean open)
{
  /*  This procedure accounts for any vertical segments that must be
      drawn to close in the horizontal segments.                     */

  if (vert_segs[x1] >= 0)
    {
      make_seg (x1, vert_segs[x1], x1, y1, !open);
      vert_segs[x1] = -1;
    }
  else
    vert_segs[x1] = y1;

  if (vert_segs[x2] >= 0)
    {
      make_seg (x2, vert_segs[x2], x2, y2, open);
      vert_segs[x2] = -1;
    }
  else
    vert_segs[x2] = y2;

  make_seg (x1, y1, x2, y2, open);
}


static void
make_horiz_segs (gint start,
		 gint end,
		 gint scanline,
		 gint empty[],
		 gint num_empty,
		 gint top)
{
  gint empty_index;
  gint e_s, e_e;    /* empty segment start and end values */

  for (empty_index = 0; empty_index < num_empty; empty_index += 2)
    {
      e_s = *empty++;
      e_e = *empty++;
      if (e_s <= start && e_e >= end)
	process_horiz_seg (start, scanline, end, scanline, top);
      else if ((e_s > start && e_s < end) ||
	       (e_e < end && e_e > start))
	process_horiz_seg (MAX (e_s, start), scanline,
			   MIN (e_e, end), scanline, top);
    }
}


static void
generate_boundary (PixelRegion  *PR,
		   BoundaryType  type,
		   gint          x1,
		   gint          y1,
		   gint          x2,
		   gint          y2,
                   guchar        threshold)
{
  gint  scanline;
  gint  i;
  gint  start, end;
  gint *tmp_segs;

  gint  num_empty_n = 0;
  gint  num_empty_c = 0;
  gint  num_empty_l = 0;

  start = 0;
  end   = 0;

  /*  array for determining the vertical line segments which must be drawn  */
  allocate_vert_segs (PR);

  /*  make sure there is enough space for the empty segment array  */
  allocate_empty_segs (PR);

  num_segs = 0;

  if (type == WithinBounds)
    {
      start = y1;
      end = y2;
    }
  else if (type == IgnoreBounds)
    {
      start = PR->y;
      end   = PR->y + PR->h;
    }

  /*  Find the empty segments for the previous and current scanlines  */
  find_empty_segs (PR, start - 1, empty_segs_l,
		   max_empty_segs, &num_empty_l,
		   type, x1, y1, x2, y2,
                   threshold);
  find_empty_segs (PR, start, empty_segs_c,
		   max_empty_segs, &num_empty_c,
		   type, x1, y1, x2, y2,
                   threshold);

  for (scanline = start; scanline < end; scanline++)
    {
      /*  find the empty segment list for the next scanline  */
      find_empty_segs (PR, scanline + 1, empty_segs_n,
		       max_empty_segs, &num_empty_n,
		       type, x1, y1, x2, y2,
                       threshold);

      /*  process the segments on the current scanline  */
      for (i = 1; i < num_empty_c - 1; i += 2)
	{
	  make_horiz_segs (empty_segs_c [i], empty_segs_c [i+1],
			   scanline, empty_segs_l, num_empty_l, 1);
	  make_horiz_segs (empty_segs_c [i], empty_segs_c [i+1],
			   scanline+1, empty_segs_n, num_empty_n, 0);
	}

      /*  get the next scanline of empty segments, swap others  */
      tmp_segs = empty_segs_l;
      empty_segs_l = empty_segs_c;
      num_empty_l = num_empty_c;
      empty_segs_c = empty_segs_n;
      num_empty_c = num_empty_n;
      empty_segs_n = tmp_segs;
    }
}


BoundSeg *
find_mask_boundary (PixelRegion  *maskPR,
		    int          *num_elems,
		    BoundaryType  type,
		    int           x1,
		    int           y1,
		    int           x2,
		    int           y2,
                    guchar        threshold)
{
  BoundSeg *new_segs = NULL;

  /*  The mask paramater can be any PixelRegion.  If the region
   *  has more than 1 bytes/pixel, the last byte of each pixel is
   *  used to determine the boundary outline.
   */

  /*  Calculate the boundary  */
  generate_boundary (maskPR, type, x1, y1, x2, y2, threshold);

  /*  Set the number of X segments  */
  *num_elems = num_segs;

  /*  Make a copy of the boundary  */
  if (num_segs)
    {
      new_segs = g_new (BoundSeg, num_segs);
      memcpy (new_segs, tmp_segs, (sizeof (BoundSeg) * num_segs));
    }

  /*  Return the new boundary  */
  return new_segs;
}


/************************/
/*  Sorting a Boundary  */

static gint
find_segment (const BoundSeg *segs,
	      gint            ns,
	      gint            x,
	      gint            y)
{
  gint index;

  for (index = 0; index < ns; index++)
    if (((segs[index].x1 == x && segs[index].y1 == y) ||
         (segs[index].x2 == x && segs[index].y2 == y)) &&
	segs[index].visited == FALSE)
      return index;

  return -1;
}


BoundSeg *
sort_boundary (const BoundSeg *segs,
	       gint            ns,
	       gint           *num_groups)
{
  gint      i;
  gint      index;
  gint      x, y;
  gint      startx, starty;
  gboolean  empty = (num_segs == 0);
  BoundSeg *new_segs;

  index    = 0;
  new_segs = NULL;

  for (i = 0; i < ns; i++)
    ((BoundSeg *) segs)[i].visited = FALSE;

  num_segs    = 0;
  *num_groups = 0;

  while (! empty)
    {
      empty = TRUE;

      /*  find the index of a non-visited segment to start a group  */
      for (i = 0; i < ns; i++)
	if (segs[i].visited == FALSE)
	  {
	    index = i;
	    empty = FALSE;
	    i = ns;
	  }

      if (! empty)
	{
	  make_seg (segs[index].x1, segs[index].y1,
		    segs[index].x2, segs[index].y2,
		    segs[index].open);
	  ((BoundSeg *) segs)[index].visited = TRUE;

	  startx = segs[index].x1;
	  starty = segs[index].y1;
	  x = segs[index].x2;
	  y = segs[index].y2;

	  while ((index = find_segment (segs, ns, x, y)) != -1)
	    {
	      /*  make sure ordering is correct  */
	      if (x == segs[index].x1 && y == segs[index].y1)
		{
		  make_seg (segs[index].x1, segs[index].y1,
			    segs[index].x2, segs[index].y2,
			    segs[index].open);
		  x = segs[index].x2;
		  y = segs[index].y2;
		}
	      else
		{
		  make_seg (segs[index].x2, segs[index].y2,
			    segs[index].x1, segs[index].y1,
			    segs[index].open);
		  x = segs[index].x1;
		  y = segs[index].y1;
		}

	      ((BoundSeg *) segs)[index].visited = TRUE;
	    }

	  if (x != startx || y != starty)
	    g_message ("sort_boundary(): Unconnected boundary group!");

	  /*  Mark the end of a group  */
	  *num_groups = *num_groups + 1;
	  make_seg (-1, -1, -1, -1, 0);
	}
    }

  /*  Make a copy of the boundary  */
  if (num_segs)
    {
      new_segs = g_new (BoundSeg, num_segs);
      memcpy (new_segs, tmp_segs, (sizeof (BoundSeg) * num_segs));
    }

  /*  Return the new boundary  */
  return new_segs;
}

/*********************************/
/* Reducing the number of points */
/* We expect the Boundary to be  */
/* sorted.                       */

BoundSeg *
simplify_boundary (BoundSeg *stroke_segs,
                   gint      num_groups,
                   gint     *num_segs)
{
  GArray   *new_bounds;
  GArray   *points;
  BoundSeg *ret_bounds;
  BoundSeg  tmp_seg;
  gint      i, j, seg, start, n_points;

  g_return_val_if_fail (num_segs != NULL, NULL);

  new_bounds = g_array_new (FALSE, FALSE, sizeof (BoundSeg));

  seg = 0;

  for (i = 0; i < num_groups; i++)
    {
      start = seg;
      n_points = 0;

      while (stroke_segs[seg].x1 != -1 ||
             stroke_segs[seg].x2 != -1 ||
             stroke_segs[seg].y1 != -1 ||
             stroke_segs[seg].y2 != -1)
        {
          n_points++;
          seg++;
        }

      if (n_points > 0)
        {
          points = g_array_new (FALSE, FALSE, sizeof (gint));

          /* temporarily use the delimiter to close the polygon */
          tmp_seg = stroke_segs[seg];
          stroke_segs[seg] = stroke_segs[start];
          simplify_subdivide (stroke_segs, start, start + n_points,
                              &points);
          stroke_segs[seg] = tmp_seg;

          for (j = 0; j < points->len; j++)
            g_array_append_val (new_bounds,
                                stroke_segs [g_array_index (points, gint, j)]);

          g_array_append_val (new_bounds, stroke_segs[seg]);

          g_array_free (points, TRUE);
        }
      seg++;
    }

  if (new_bounds->len > 0)
    {
      ret_bounds = (BoundSeg *) new_bounds->data;
      *num_segs = new_bounds->len;
    }
  else
    {
      ret_bounds = NULL;
      *num_segs = 0;
    }

  g_array_free (new_bounds, FALSE);

  return ret_bounds;
}

static void
simplify_subdivide (const    BoundSeg *segs,
                    gint     start_idx,
                    gint     end_idx,
                    GArray **ret_points)
{
  gint    maxdist_idx;
  gint    dist, maxdist;
  gint    i, dx, dy;
  gdouble realdist;

  /* g_printerr ("subdiv %d - %d\n", start_idx, end_idx); */

  if (end_idx - start_idx < 2)
    {
      *ret_points = g_array_append_val (*ret_points, start_idx);
      /* g_printerr ("                               %d\n", start_idx); */
      return;
    }

  maxdist = 0;
  maxdist_idx = -1;

  if (segs[start_idx].x1 == segs[end_idx].x1 &&
      segs[start_idx].y1 == segs[end_idx].y1)
    {
      /* start and endpoint are at the same coordinates */
      for (i = start_idx + 1; i < end_idx; i++)
        {
          /* compare the sqared distances */
          dist = (SQR (segs[i].x1 - segs[start_idx].x1) +
                  SQR (segs[i].y1 - segs[start_idx].y1));
          if (dist > maxdist)
            {
              maxdist = dist;
              maxdist_idx = i;
            }
        }
      realdist = sqrt ((gdouble) maxdist);
    }
  else
    {
      dx = segs[end_idx].x1 - segs[start_idx].x1;
      dy = segs[end_idx].y1 - segs[start_idx].y1;
      
      /* g_printerr ("dx: %d, dy: %d\n", dx, dy); */

      for (i = start_idx + 1; i < end_idx; i++)
        {
          /* this is not really the euclidic distance, but is
           * proportional for this part of the line
           * (for the real distance we'd have to divide by
           * (SQR(dx)+SQR(dy)))
           */
          dist = (dx * (segs[start_idx].y1 - segs[i].y1) -
                  dy * (segs[start_idx].x1 - segs[i].x1));

          if (dist < 0)
            dist *= -1;


          if (dist > maxdist)
            {
              maxdist = dist;
              maxdist_idx = i;
            }
        }
      realdist = ((gdouble) maxdist) / sqrt ((gdouble) (SQR (dx) + SQR (dy)));
    }
  
  /* g_printerr ("Index %d, x: %d, y: %d, distance: %.4f\n", maxdist_idx,
              segs[maxdist_idx].x1, segs[maxdist_idx].y1, realdist); */
  /* threshold is chosen to catch 45 degree stairs */

  if (realdist <= 1.0)
    {
      *ret_points = g_array_append_val (*ret_points, start_idx);
      /* g_printerr ("                               %d\n", start_idx); */
      return;
    }

  /* Simons hack */
  maxdist_idx = (start_idx + end_idx) / 2;

  simplify_subdivide (segs, start_idx, maxdist_idx, ret_points);
  simplify_subdivide (segs, maxdist_idx, end_idx, ret_points);

}

