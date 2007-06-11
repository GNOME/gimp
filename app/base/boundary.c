/* GIMP - The GNU Image Manipulation Program
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


typedef struct _Boundary Boundary;

struct _Boundary
{
  /*  The array of segments  */
  BoundSeg *segs;
  gint      num_segs;
  gint      max_segs;

  /*  The array of vertical segments  */
  gint     *vert_segs;

  /*  The empty segment arrays */
  gint     *empty_segs_n;
  gint     *empty_segs_c;
  gint     *empty_segs_l;
  gint      max_empty_segs;
};


/*  local function prototypes  */

static Boundary * boundary_new       (PixelRegion     *PR);
static BoundSeg * boundary_free      (Boundary        *boundary,
                                      gboolean         free_segs);

static void       boundary_add_seg   (Boundary        *bounrady,
                                      gint             x1,
                                      gint             y1,
                                      gint             x2,
                                      gint             y2,
                                      gboolean         open);

static void       find_empty_segs    (PixelRegion     *maskPR,
                                      gint             scanline,
                                      gint             empty_segs[],
                                      gint             max_empty,
                                      gint            *num_empty,
                                      BoundaryType     type,
                                      gint             x1,
                                      gint             y1,
                                      gint             x2,
                                      gint             y2,
                                      guchar           threshold);
static void       process_horiz_seg  (Boundary        *boundary,
                                      gint             x1,
                                      gint             y1,
                                      gint             x2,
                                      gint             y2,
                                      gboolean         open);
static void       make_horiz_segs    (Boundary        *boundary,
                                      gint             start,
                                      gint             end,
                                      gint             scanline,
                                      gint             empty[],
                                      gint             num_empty,
                                      gint             top);
static Boundary * generate_boundary  (PixelRegion     *PR,
                                      BoundaryType     type,
                                      gint             x1,
                                      gint             y1,
                                      gint             x2,
                                      gint             y2,
                                      guchar           threshold);

static gint       cmp_xy              (gint ax,
                                       gint ay,
                                       gint bx,
                                       gint by);

static gint       cmp_segptr_xy1_addr (const BoundSeg **seg_ptr_a,
                                       const BoundSeg **seg_ptr_b);
static gint       cmp_segptr_xy2_addr (const BoundSeg **seg_ptr_a,
                                       const BoundSeg **seg_ptr_b);

static gint       cmp_segptr_xy1      (const BoundSeg **seg_ptr_a,
                                       const BoundSeg **seg_ptr_b);
static gint       cmp_segptr_xy2      (const BoundSeg **seg_ptr_a,
                                       const BoundSeg **seg_ptr_b);

static const BoundSeg * find_segment  (const BoundSeg **segs_by_xy1,
                                       const BoundSeg **segs_by_xy2,
                                       gint             num_segs,
                                       gint             x,
                                       gint             y);

static const BoundSeg * find_segment_with_func (const BoundSeg **segs,
                                                gint             num_segs,
                                                const BoundSeg  *search_seg,
                                                GCompareFunc     cmp_func);

static void       simplify_subdivide (const BoundSeg  *segs,
                                      gint             start_idx,
                                      gint             end_idx,
                                      GArray         **ret_points);


/*  public functions  */

/**
 * boundary_find:
 * @maskPR:    any PixelRegion
 * @type:      type of bounds
 * @x1:        left side of bounds
 * @y1:        top side of bounds
 * @x2:        right side of bounds
 * @y2:        botton side of bounds
 * @threshold: pixel value of boundary line
 * @num_segs:  number of returned #BoundSeg's
 *
 * This function returns an array of #BoundSeg's which describe all
 * outlines along pixel value @threahold, optionally within specified
 * bounds instead of the whole region.
 *
 * The @maskPR paramater can be any PixelRegion.  If the region has
 * more than 1 bytes/pixel, the last byte of each pixel is used to
 * determine the boundary outline.
 *
 * Return value: the boundary array.
 **/
BoundSeg *
boundary_find (PixelRegion  *maskPR,
               BoundaryType  type,
               int           x1,
               int           y1,
               int           x2,
               int           y2,
               guchar        threshold,
               int          *num_segs)
{
  Boundary *boundary;

  g_return_val_if_fail (maskPR != NULL, NULL);
  g_return_val_if_fail (num_segs != NULL, NULL);

  boundary = generate_boundary (maskPR, type, x1, y1, x2, y2, threshold);

  *num_segs = boundary->num_segs;

  return boundary_free (boundary, FALSE);
}

/**
 * boundary_sort:
 * @segs:       unsorted input segs.
 * @num_segs:   number of input segs
 * @num_groups: number of groups in the sorted segs
 *
 * This function takes an array of #BoundSeg's as returned by
 * boundary_find() and sorts it by contiguous groups. The returned
 * array contains markers consisting of -1 coordinates and is
 * @num_groups elements longer than @segs.
 *
 * Return value: the sorted segs
 **/
BoundSeg *
boundary_sort (const BoundSeg *segs,
               gint            num_segs,
               gint           *num_groups)
{
  Boundary *boundary;
  const BoundSeg **segs_ptrs_by_xy1, **segs_ptrs_by_xy2;
  gint      index;
  gint      x, y;
  gint      startx, starty;

  g_return_val_if_fail ((segs == NULL && num_segs == 0) ||
                        (segs != NULL && num_segs >  0), NULL);
  g_return_val_if_fail (num_groups != NULL, NULL);

  *num_groups = 0;

  if (num_segs == 0)
    return NULL;

  /* prepare arrays with BoundSeg pointers sorted by xy1 and xy2 accordingly */
  segs_ptrs_by_xy1 = g_new (const BoundSeg *, num_segs);
  segs_ptrs_by_xy2 = g_new (const BoundSeg *, num_segs);

  for (index = 0; index < num_segs; index++)
    {
      segs_ptrs_by_xy1[index] = segs + index;
      segs_ptrs_by_xy2[index] = segs + index;
    }

  qsort (segs_ptrs_by_xy1, num_segs, sizeof (BoundSeg *),
         (GCompareFunc) cmp_segptr_xy1_addr);
  qsort (segs_ptrs_by_xy2, num_segs, sizeof (BoundSeg *),
         (GCompareFunc) cmp_segptr_xy2_addr);

  for (index = 0; index < num_segs; index++)
    ((BoundSeg *) segs)[index].visited = FALSE;

  boundary = boundary_new (NULL);

  for (index = 0; index < num_segs; index++)
    {
      const BoundSeg *cur_seg;

      if (segs[index].visited)
        continue;

      boundary_add_seg (boundary,
                        segs[index].x1, segs[index].y1,
                        segs[index].x2, segs[index].y2,
                        segs[index].open);

      ((BoundSeg *) segs)[index].visited = TRUE;

      startx = segs[index].x1;
      starty = segs[index].y1;
      x = segs[index].x2;
      y = segs[index].y2;

      while ((cur_seg = find_segment (segs_ptrs_by_xy1, segs_ptrs_by_xy2,
                                      num_segs, x, y)) != NULL)
        {
          /*  make sure ordering is correct  */
          if (x == cur_seg->x1 && y == cur_seg->y1)
            {
              boundary_add_seg (boundary,
                                cur_seg->x1, cur_seg->y1,
                                cur_seg->x2, cur_seg->y2,
                                cur_seg->open);
              x = cur_seg->x2;
              y = cur_seg->y2;
            }
          else
            {
              boundary_add_seg (boundary,
                                cur_seg->x2, cur_seg->y2,
                                cur_seg->x1, cur_seg->y1,
                                cur_seg->open);
              x = cur_seg->x1;
              y = cur_seg->y1;
            }

          ((BoundSeg *) cur_seg)->visited = TRUE;
        }

      if (G_UNLIKELY (x != startx || y != starty))
        g_warning ("sort_boundary(): Unconnected boundary group!");

      /*  Mark the end of a group  */
      *num_groups = *num_groups + 1;
      boundary_add_seg (boundary, -1, -1, -1, -1, 0);
  }

  g_free (segs_ptrs_by_xy1);
  g_free (segs_ptrs_by_xy2);

  return boundary_free (boundary, FALSE);
}

/**
 * boundary_simplify:
 * @sorted_segs: sorted input segs
 * @num_groups:  number of groups in the sorted segs
 * @num_segs:    number of returned segs.
 *
 * This function takes an array of #BoundSeg's which has been sorted
 * with boundary_sort() and reduces the number of segments while
 * preserving the general shape as close as possible.
 *
 * Return value: the simplified segs.
 **/
BoundSeg *
boundary_simplify (BoundSeg *sorted_segs,
                   gint      num_groups,
                   gint     *num_segs)
{
  GArray *new_bounds;
  gint    i, seg;

  g_return_val_if_fail ((sorted_segs == NULL && num_groups == 0) ||
                        (sorted_segs != NULL && num_groups >  0), NULL);
  g_return_val_if_fail (num_segs != NULL, NULL);

  new_bounds = g_array_new (FALSE, FALSE, sizeof (BoundSeg));

  seg = 0;

  for (i = 0; i < num_groups; i++)
    {
      gint start    = seg;
      gint n_points = 0;

      while (sorted_segs[seg].x1 != -1 ||
             sorted_segs[seg].x2 != -1 ||
             sorted_segs[seg].y1 != -1 ||
             sorted_segs[seg].y2 != -1)
        {
          n_points++;
          seg++;
        }

      if (n_points > 0)
        {
          GArray   *tmp_points;
          BoundSeg  tmp_seg;
          gint      j;

          tmp_points = g_array_new (FALSE, FALSE, sizeof (gint));

          /* temporarily use the delimiter to close the polygon */
          tmp_seg = sorted_segs[seg];
          sorted_segs[seg] = sorted_segs[start];
          simplify_subdivide (sorted_segs, start, start + n_points,
                              &tmp_points);
          sorted_segs[seg] = tmp_seg;

          for (j = 0; j < tmp_points->len; j++)
            g_array_append_val (new_bounds,
                                sorted_segs[g_array_index (tmp_points,
                                                           gint, j)]);

          g_array_append_val (new_bounds, sorted_segs[seg]);

          g_array_free (tmp_points, TRUE);
        }

      seg++;
    }

  *num_segs = new_bounds->len;

  return (BoundSeg *) g_array_free (new_bounds, FALSE);
}


/*  private functions  */

static Boundary *
boundary_new (PixelRegion *PR)
{
  Boundary *boundary = g_slice_new0 (Boundary);

  if (PR)
    {
      gint i;

      /*  array for determining the vertical line segments
       *  which must be drawn
       */
      boundary->vert_segs = g_new (gint, PR->w + PR->x + 1);

      for (i = 0; i <= (PR->w + PR->x); i++)
        boundary->vert_segs[i] = -1;

      /*  find the maximum possible number of empty segments
       *  given the current mask
       */
      boundary->max_empty_segs = PR->w + 3;

      boundary->empty_segs_n = g_new (gint, boundary->max_empty_segs);
      boundary->empty_segs_c = g_new (gint, boundary->max_empty_segs);
      boundary->empty_segs_l = g_new (gint, boundary->max_empty_segs);
    }

  return boundary;
}

static BoundSeg *
boundary_free (Boundary *boundary,
               gboolean  free_segs)
{
  BoundSeg *segs = NULL;

  if (free_segs)
    g_free (boundary->segs);
  else
    segs = boundary->segs;

  g_free (boundary->vert_segs);
  g_free (boundary->empty_segs_n);
  g_free (boundary->empty_segs_c);
  g_free (boundary->empty_segs_l);

  g_slice_free (Boundary, boundary);

  return segs;
}

static void
boundary_add_seg (Boundary *boundary,
                  gint      x1,
                  gint      y1,
                  gint      x2,
                  gint      y2,
                  gboolean  open)
{
  if (boundary->num_segs >= boundary->max_segs)
    {
      boundary->max_segs += MAX_SEGS_INC;

      boundary->segs = g_renew (BoundSeg, boundary->segs, boundary->max_segs);
    }

  boundary->segs[boundary->num_segs].x1 = x1;
  boundary->segs[boundary->num_segs].y1 = y1;
  boundary->segs[boundary->num_segs].x2 = x2;
  boundary->segs[boundary->num_segs].y2 = y2;
  boundary->segs[boundary->num_segs].open = open;
  boundary->num_segs ++;
}

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
  const guchar *data  = NULL;
  Tile         *tile  = NULL;
  gint          start = 0;
  gint          end   = 0;
  gint          endx  = 0;
  gint          dstep = 0;
  gint          val, last;
  gint          x, tilex;
  gint          l_num_empty;

  *num_empty = 0;

  if (scanline < maskPR->y || scanline >= (maskPR->y + maskPR->h))
    {
      empty_segs[(*num_empty)++] = 0;
      empty_segs[(*num_empty)++] = G_MAXINT;
      return;
    }

  if (type == BOUNDARY_WITHIN_BOUNDS)
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
  else if (type == BOUNDARY_IGNORE_BOUNDS)
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
                (const guchar *) tile_data_pointer (tile,
                                                    x % TILE_WIDTH,
                                                    scanline % TILE_HEIGHT) +
                (tile_bpp(tile) - 1);

              tilex = x / TILE_WIDTH;
              dstep = tile_bpp (tile);
            }

          endx = x + (TILE_WIDTH - (x%TILE_WIDTH));
          endx = MIN (end, endx);
        }

      if (type == BOUNDARY_IGNORE_BOUNDS && (endx > x1 || x < x2))
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
process_horiz_seg (Boundary *boundary,
                   gint      x1,
                   gint      y1,
                   gint      x2,
                   gint      y2,
                   gboolean  open)
{
  /*  This procedure accounts for any vertical segments that must be
      drawn to close in the horizontal segments.                     */

  if (boundary->vert_segs[x1] >= 0)
    {
      boundary_add_seg (boundary, x1, boundary->vert_segs[x1], x1, y1, !open);
      boundary->vert_segs[x1] = -1;
    }
  else
    boundary->vert_segs[x1] = y1;

  if (boundary->vert_segs[x2] >= 0)
    {
      boundary_add_seg (boundary, x2, boundary->vert_segs[x2], x2, y2, open);
      boundary->vert_segs[x2] = -1;
    }
  else
    boundary->vert_segs[x2] = y2;

  boundary_add_seg (boundary, x1, y1, x2, y2, open);
}

static void
make_horiz_segs (Boundary *boundary,
                 gint      start,
                 gint      end,
                 gint      scanline,
                 gint      empty[],
                 gint      num_empty,
                 gint      top)
{
  gint empty_index;
  gint e_s, e_e;    /* empty segment start and end values */

  for (empty_index = 0; empty_index < num_empty; empty_index += 2)
    {
      e_s = *empty++;
      e_e = *empty++;

      if (e_s <= start && e_e >= end)
        process_horiz_seg (boundary,
                           start, scanline, end, scanline, top);
      else if ((e_s > start && e_s < end) ||
               (e_e < end && e_e > start))
        process_horiz_seg (boundary,
                           MAX (e_s, start), scanline,
                           MIN (e_e, end), scanline, top);
    }
}

static Boundary *
generate_boundary (PixelRegion  *PR,
                   BoundaryType  type,
                   gint          x1,
                   gint          y1,
                   gint          x2,
                   gint          y2,
                   guchar        threshold)
{
  Boundary *boundary;
  gint      scanline;
  gint      i;
  gint      start, end;
  gint     *tmp_segs;

  gint      num_empty_n = 0;
  gint      num_empty_c = 0;
  gint      num_empty_l = 0;

  boundary = boundary_new (PR);

  start = 0;
  end   = 0;

  if (type == BOUNDARY_WITHIN_BOUNDS)
    {
      start = y1;
      end   = y2;
    }
  else if (type == BOUNDARY_IGNORE_BOUNDS)
    {
      start = PR->y;
      end   = PR->y + PR->h;
    }

  /*  Find the empty segments for the previous and current scanlines  */
  find_empty_segs (PR, start - 1, boundary->empty_segs_l,
                   boundary->max_empty_segs, &num_empty_l,
                   type, x1, y1, x2, y2,
                   threshold);
  find_empty_segs (PR, start, boundary->empty_segs_c,
                   boundary->max_empty_segs, &num_empty_c,
                   type, x1, y1, x2, y2,
                   threshold);

  for (scanline = start; scanline < end; scanline++)
    {
      /*  find the empty segment list for the next scanline  */
      find_empty_segs (PR, scanline + 1, boundary->empty_segs_n,
                       boundary->max_empty_segs, &num_empty_n,
                       type, x1, y1, x2, y2,
                       threshold);

      /*  process the segments on the current scanline  */
      for (i = 1; i < num_empty_c - 1; i += 2)
        {
          make_horiz_segs (boundary,
                           boundary->empty_segs_c [i],
                           boundary->empty_segs_c [i+1],
                           scanline,
                           boundary->empty_segs_l, num_empty_l, 1);
          make_horiz_segs (boundary,
                           boundary->empty_segs_c [i],
                           boundary->empty_segs_c [i+1],
                           scanline + 1,
                           boundary->empty_segs_n, num_empty_n, 0);
        }

      /*  get the next scanline of empty segments, swap others  */
      tmp_segs               = boundary->empty_segs_l;
      boundary->empty_segs_l = boundary->empty_segs_c;
      num_empty_l            = num_empty_c;
      boundary->empty_segs_c = boundary->empty_segs_n;
      num_empty_c            = num_empty_n;
      boundary->empty_segs_n = tmp_segs;
    }

  return boundary;
}

/*  sorting utility functions  */

static gint
cmp_xy(gint ax, gint ay, gint bx, gint by)
{
  if (ax < bx) {
    return -1;
  } else if (ax > bx) {
    return 1;
  } else if (ay < by) {
    return -1;
  } else if (ay > by) {
    return 1;
  } else {
    return 0;
  }
}


/*
 * Compares (x1, y1) pairs in specified segments, using their addresses if
 * (x1, y1) pairs are equal.
 */
static gint
cmp_segptr_xy1_addr (const BoundSeg **seg_ptr_a,
                     const BoundSeg **seg_ptr_b)
{
  const BoundSeg *seg_a = *seg_ptr_a;
  const BoundSeg *seg_b = *seg_ptr_b;

  gint result = cmp_xy (seg_a->x1, seg_a->y1, seg_b->x1, seg_b->y1);

  if (result == 0)
    {
      if (seg_a < seg_b)
        result = -1;
      else if (seg_a > seg_b)
        result = 1;
    }

  return result;
}

/*
 * Compares (x2, y2) pairs in specified segments, using their addresses if
 * (x2, y2) pairs are equal.
 */
static gint
cmp_segptr_xy2_addr (const BoundSeg **seg_ptr_a,
                     const BoundSeg **seg_ptr_b)
{
  const BoundSeg *seg_a = *seg_ptr_a;
  const BoundSeg *seg_b = *seg_ptr_b;

  gint result = cmp_xy (seg_a->x2, seg_a->y2, seg_b->x2, seg_b->y2);

  if (result == 0)
    {
      if (seg_a < seg_b)
        result = -1;
      else if (seg_a > seg_b)
        result = 1;
    }

  return result;
}


/*
 * Compares (x1, y1) pairs in specified segments.
 */
static gint
cmp_segptr_xy1 (const BoundSeg **seg_ptr_a, const BoundSeg **seg_ptr_b)
{
  const BoundSeg *seg_a = *seg_ptr_a, *seg_b = *seg_ptr_b;

  return cmp_xy(seg_a->x1, seg_a->y1, seg_b->x1, seg_b->y1);
}

/*
 * Compares (x2, y2) pairs in specified segments.
 */
static gint
cmp_segptr_xy2 (const BoundSeg **seg_ptr_a,
                const BoundSeg **seg_ptr_b)
{
  const BoundSeg *seg_a = *seg_ptr_a;
  const BoundSeg *seg_b = *seg_ptr_b;

  return cmp_xy (seg_a->x2, seg_a->y2, seg_b->x2, seg_b->y2);
}


static const BoundSeg *
find_segment (const BoundSeg **segs_by_xy1,
              const BoundSeg **segs_by_xy2,
              gint             num_segs,
              gint             x,
              gint             y)
{
  const BoundSeg *segptr_xy1;
  const BoundSeg *segptr_xy2;
  BoundSeg        search_seg;

  search_seg.x1 = search_seg.x2 = x;
  search_seg.y1 = search_seg.y2 = y;

  segptr_xy1 = find_segment_with_func (segs_by_xy1, num_segs, &search_seg,
                                       (GCompareFunc) cmp_segptr_xy1);
  segptr_xy2 = find_segment_with_func (segs_by_xy2, num_segs, &search_seg,
                                       (GCompareFunc) cmp_segptr_xy2);

  /* return segment with smaller address */
  if (segptr_xy1 != NULL && segptr_xy2 != NULL)
    return MIN(segptr_xy1, segptr_xy2);
  else if (segptr_xy1 != NULL)
    return segptr_xy1;
  else if (segptr_xy2 != NULL)
    return segptr_xy2;

  return NULL;
}


static const BoundSeg *
find_segment_with_func (const BoundSeg **segs,
                        gint             num_segs,
                        const BoundSeg  *search_seg,
                        GCompareFunc     cmp_func)
{
  const BoundSeg **seg;
  const BoundSeg *found_seg = NULL;

  seg = bsearch (&search_seg, segs, num_segs, sizeof (BoundSeg *), cmp_func);

  if (seg != NULL)
    {
      /* find first matching segment */
      while (seg > segs && cmp_func (seg - 1, &search_seg) == 0)
        seg--;

      /* find first non-visited segment */
      while (seg != segs + num_segs && cmp_func (seg, &search_seg) == 0)
        if (!(*seg)->visited)
          {
            found_seg = *seg;
            break;
          }
        else
          seg++;
    }

    return found_seg;
}


/*  simplifying utility functions  */

static void
simplify_subdivide (const BoundSeg *segs,
                    gint            start_idx,
                    gint            end_idx,
                    GArray        **ret_points)
{
  gint    maxdist_idx;
  gint    dist, maxdist;
  gint    i, dx, dy;
  gdouble realdist;

  if (end_idx - start_idx < 2)
    {
      *ret_points = g_array_append_val (*ret_points, start_idx);
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

  /* threshold is chosen to catch 45 degree stairs */
  if (realdist <= 1.0)
    {
      *ret_points = g_array_append_val (*ret_points, start_idx);
      return;
    }

  /* Simons hack */
  maxdist_idx = (start_idx + end_idx) / 2;

  simplify_subdivide (segs, start_idx, maxdist_idx, ret_points);
  simplify_subdivide (segs, maxdist_idx, end_idx, ret_points);
}
