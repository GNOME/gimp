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
#include <string.h>
#include "appenv.h"
#include "errors.h"
#include "boundary.h"

/* half intensity for mask */
#define HALF_WAY 127

/* BoundSeg array growth parameter */
#define MAX_SEGS_INC  2048

/*  The array of vertical segments  */
static int *      vert_segs = NULL;

/*  The array of segments  */
static BoundSeg * tmp_segs = NULL;
static int        num_segs = 0;
static int        max_segs = 0;

/* static empty segment arrays */
static int *      empty_segs_n = NULL;
static int        num_empty_n = 0;
static int *      empty_segs_c = NULL;
static int        num_empty_c = 0;
static int *      empty_segs_l = NULL;
static int        num_empty_l = 0;
static int        max_empty_segs = 0;

/* global state variables--improve parameter efficiency */
static PixelRegion * cur_PR;


/*  local function prototypes  */
static void find_empty_segs (PixelRegion *, int, int *, int, int *,
			     BoundaryType, int, int, int, int);
static void make_seg (int, int, int, int, int);
static void allocate_vert_segs (void);
static void allocate_empty_segs (void);
static void process_horiz_seg (int, int, int, int, int);
static void make_horiz_segs (int, int, int, int *, int, int);
static void generate_boundary (BoundaryType, int, int, int, int);

/*  Function definitions  */

static void
find_empty_segs (PixelRegion  *maskPR,
		 int           scanline,
		 int           empty_segs[],
		 int           max_empty,
		 int          *num_empty,
		 BoundaryType  type,
		 int           x1,
		 int           y1,
		 int           x2,
		 int           y2)
{
  unsigned char *data;
  int x;
  int start, end;
  int val, last;
  int tilex;
  Tile *tile = NULL;

  data  = NULL;
  start = 0;
  end   = 0;

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
      end = x2;
    }
  else if (type == IgnoreBounds)
    {
      start = maskPR->x;
      end = maskPR->x + maskPR->w;
      if (scanline < y1 || scanline >= y2)
	x2 = -1;
    }

  tilex = -1;
  empty_segs[(*num_empty)++] = 0;
  last = -1;

  for (x = start; x < end; x++)
    {
      /*  Check to see if we must advance to next tile  */
      if ((x / TILE_WIDTH) != tilex)
	{
	  if (tile)
	    tile_unref (tile, FALSE);
	  tile = tile_manager_get_tile (maskPR->tiles, x, scanline, 0);
	  tile_ref2 (tile, FALSE);
	  data = tile->data + tile->bpp *
	    ((scanline % TILE_HEIGHT) * tile->ewidth + (x % TILE_WIDTH)) + (tile->bpp - 1);
	  tilex = x / TILE_WIDTH;
	}

      empty_segs[*num_empty] = x;
      val = (*data > HALF_WAY) ? 1 : -1;

      /*  The IgnoreBounds case  */
      if (val == 1 && type == IgnoreBounds)
	if (x >= x1 && x < x2)
	  val = -1;

      if (last * val < 0)
	(*num_empty)++;
      last = val;

      data += tile->bpp;
    }

  if (last > 0)
    empty_segs[(*num_empty)++] = x;

  empty_segs[(*num_empty)++] = G_MAXINT;

  if (tile)
    tile_unref (tile, FALSE);
}


static void
make_seg (int x1,
	  int y1,
	  int x2,
	  int y2,
	  int open)
{
  if (num_segs >= max_segs)
    {
      max_segs += MAX_SEGS_INC;

      tmp_segs = (BoundSeg *) g_realloc ((void *) tmp_segs,
					sizeof (BoundSeg) * max_segs);

      if (!tmp_segs)
	fatal_error ("Unable to reallocate segments array for mask boundary.");
    }

  tmp_segs[num_segs].x1 = x1;
  tmp_segs[num_segs].y1 = y1;
  tmp_segs[num_segs].x2 = x2;
  tmp_segs[num_segs].y2 = y2;
  tmp_segs[num_segs].open = open;
  num_segs ++;
}


static void
allocate_vert_segs (void)
{
  int i;

  /*  allocate and initialize the vert_segs array  */
  vert_segs = (int *) g_realloc ((void *) vert_segs, (cur_PR->w + cur_PR->x + 1) * sizeof (int));

  for (i = 0; i <= (cur_PR->w + cur_PR->x); i++)
    vert_segs[i] = -1;
}


static void
allocate_empty_segs (void)
{
  int need_num_segs;

  /*  find the maximum possible number of empty segments given the current mask  */
  need_num_segs = cur_PR->w + 2;

  if (need_num_segs > max_empty_segs)
    {
      max_empty_segs = need_num_segs;

      empty_segs_n = (int *) g_realloc (empty_segs_n, sizeof (int) * max_empty_segs);
      empty_segs_c = (int *) g_realloc (empty_segs_c, sizeof (int) * max_empty_segs);
      empty_segs_l = (int *) g_realloc (empty_segs_l, sizeof (int) * max_empty_segs);

      if (!empty_segs_n || !empty_segs_l || !empty_segs_c)
	fatal_error ("Unable to reallocate empty segments array for mask boundary.");
    }
}


static void
process_horiz_seg (int x1,
		   int y1,
		   int x2,
		   int y2,
		   int open)
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
make_horiz_segs (int start,
		 int end,
		 int scanline,
		 int empty[],
		 int num_empty,
		 int top)
{
  int empty_index;
  int e_s, e_e;    /* empty segment start and end values */

  for (empty_index = 0; empty_index < num_empty; empty_index += 2)
    {
      e_s = *empty++;
      e_e = *empty++;
      if (e_s <= start && e_e >= end)
	process_horiz_seg (start, scanline, end, scanline, top);
      else if ((e_s > start && e_s < end) ||
	       (e_e < end && e_e > start))
	process_horiz_seg (MAXIMUM (e_s, start), scanline,
			   MINIMUM (e_e, end), scanline, top);
    }
}


static void
generate_boundary (BoundaryType type,
		   int          x1,
		   int          y1,
		   int          x2,
		   int          y2)
{
  int scanline;
  int i;
  int start, end;
  int * tmp_segs;

  start = 0;
  end   = 0;

  /*  array for determining the vertical line segments which must be drawn  */
  allocate_vert_segs ();

  /*  make sure there is enough space for the empty segment array  */
  allocate_empty_segs ();

  num_segs = 0;

  if (type == WithinBounds)
    {
      start = y1;
      end = y2;
    }
  else if (type == IgnoreBounds)
    {
      start = cur_PR->y;
      end = cur_PR->y + cur_PR->h;
    }

  /*  Find the empty segments for the previous and current scanlines  */
  find_empty_segs (cur_PR, start - 1, empty_segs_l,
		   max_empty_segs, &num_empty_l,
		   type, x1, y1, x2, y2);
  find_empty_segs (cur_PR, start, empty_segs_c,
		   max_empty_segs, &num_empty_c,
		   type, x1, y1, x2, y2);

  for (scanline = start; scanline < end; scanline++)
    {
      /*  find the empty segment list for the next scanline  */
      find_empty_segs (cur_PR, scanline + 1, empty_segs_n,
		       max_empty_segs, &num_empty_n,
		       type, x1, y1, x2, y2);

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
		    int           y2)
{
  BoundSeg * new_segs = NULL;

  /*  The mask paramater can be any PixelRegion.  If the region
   *  has more than 1 bytes/pixel, the last byte of each pixel is
   *  used to determine the boundary outline.
   */
  cur_PR = maskPR;

  /*  Calculate the boundary  */
  generate_boundary (type, x1, y1, x2, y2);

  /*  Set the number of X segments  */
  *num_elems = num_segs;

  /*  Make a copy of the boundary  */
  if (num_segs)
    {
      new_segs = (BoundSeg *) g_malloc (sizeof (BoundSeg) * num_segs);
      memcpy (new_segs, tmp_segs, (sizeof (BoundSeg) * num_segs));
    }

  /*  Return the new boundary  */
  return new_segs;
}


/************************/
/*  Sorting a Boundary  */

static int find_segment (BoundSeg *, int, int, int);

static int
find_segment (BoundSeg *segs,
	      int       ns,
	      int       x,
	      int       y)
{
  int index;

  for (index = 0; index < ns; index++)
    if (((segs[index].x1 == x && segs[index].y1 == y) || (segs[index].x2 == x && segs[index].y2 == y)) &&
	segs[index].visited == FALSE)
      return index;

  return -1;
}


BoundSeg *
sort_boundary (BoundSeg *segs,
	       int       ns,
	       int      *num_groups)
{
  int i;
  int index;
  int x, y;
  int startx, starty;
  int empty = (num_segs == 0);
  BoundSeg *new_segs;

  index = 0;
  new_segs = NULL;

  for (i = 0; i < ns; i++)
    segs[i].visited = FALSE;

  num_segs = 0;
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
	  segs[index].visited = TRUE;

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

	      segs[index].visited = TRUE;
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
      new_segs = (BoundSeg *) g_malloc (sizeof (BoundSeg) * num_segs);
      memcpy (new_segs, tmp_segs, (sizeof (BoundSeg) * num_segs));
    }

  /*  Return the new boundary  */
  return new_segs;
}
