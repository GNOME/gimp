/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <stdlib.h>
#include <string.h>

#include <gegl.h>

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gimpboundary.h"


/* GimpBoundSeg array growth parameter */
#define MAX_SEGS_INC  2048


typedef struct _GimpBoundary GimpBoundary;

struct _GimpBoundary
{
  /*  The array of segments  */
  GimpBoundSeg *segs;
  gint          num_segs;
  gint          max_segs;

  /*  The array of vertical segments  */
  gint         *vert_segs;

  /*  The empty segment arrays */
  gint         *empty_segs_n;
  gint         *empty_segs_c;
  gint         *empty_segs_l;
  gint          max_empty_segs;
};


/*  local function prototypes  */

static GimpBoundary * gimp_boundary_new        (const GeglRectangle *region);
static GimpBoundSeg * gimp_boundary_free       (GimpBoundary        *boundary,
                                                gboolean             free_segs);

static void           gimp_boundary_add_seg    (GimpBoundary        *bounrady,
                                                gint                 x1,
                                                gint                 y1,
                                                gint                 x2,
                                                gint                 y2,
                                                gboolean             open);

static void           find_empty_segs          (const GeglRectangle *region,
                                                const gfloat        *line_data,
                                                gint                 scanline,
                                                gint                 empty_segs[],
                                                gint                 max_empty,
                                                gint                *num_empty,
                                                GimpBoundaryType     type,
                                                gint                 x1,
                                                gint                 y1,
                                                gint                 x2,
                                                gint                 y2,
                                                gfloat               threshold);
static void           process_horiz_seg        (GimpBoundary        *boundary,
                                                gint                 x1,
                                                gint                 y1,
                                                gint                 x2,
                                                gint                 y2,
                                                gboolean             open);
static void           make_horiz_segs          (GimpBoundary        *boundary,
                                                gint                 start,
                                                gint                 end,
                                                gint                 scanline,
                                                gint                 empty[],
                                                gint                 num_empty,
                                                gint                 top);
static GimpBoundary * generate_boundary        (GeglBuffer          *buffer,
                                                const GeglRectangle *region,
                                                const Babl          *format,
                                                GimpBoundaryType     type,
                                                gint                 x1,
                                                gint                 y1,
                                                gint                 x2,
                                                gint                 y2,
                                                gfloat               threshold);

static gint       cmp_segptr_xy1_addr     (const GimpBoundSeg **seg_ptr_a,
                                           const GimpBoundSeg **seg_ptr_b);
static gint       cmp_segptr_xy2_addr     (const GimpBoundSeg **seg_ptr_a,
                                           const GimpBoundSeg **seg_ptr_b);

static gint       cmp_segptr_xy1          (const GimpBoundSeg **seg_ptr_a,
                                           const GimpBoundSeg **seg_ptr_b);
static gint       cmp_segptr_xy2          (const GimpBoundSeg **seg_ptr_a,
                                           const GimpBoundSeg **seg_ptr_b);

static const GimpBoundSeg * find_segment  (const GimpBoundSeg **segs_by_xy1,
                                           const GimpBoundSeg **segs_by_xy2,
                                           gint                 num_segs,
                                           gint                 x,
                                           gint                 y);

static const GimpBoundSeg * find_segment_with_func (const GimpBoundSeg **segs,
                                                    gint                 num_segs,
                                                    const GimpBoundSeg  *search_seg,
                                                    GCompareFunc         cmp_func);

static void       simplify_subdivide  (const GimpBoundSeg  *segs,
                                       gint                 start_idx,
                                       gint                 end_idx,
                                       GArray             **ret_points);


/*  public functions  */

/**
 * gimp_boundary_find:
 * @buffer:    a #GeglBuffer
 * @format:    a #Babl float format representing the component to analyze
 * @type:      type of bounds
 * @x1:        left side of bounds
 * @y1:        top side of bounds
 * @x2:        right side of bounds
 * @y2:        bottom side of bounds
 * @threshold: pixel value of boundary line
 * @num_segs:  number of returned #GimpBoundSeg's
 *
 * This function returns an array of #GimpBoundSeg's which describe all
 * outlines along pixel value @threahold, optionally within specified
 * bounds instead of the whole region.
 *
 * The @maskPR parameter can be any PixelRegion.  If the region has
 * more than 1 bytes/pixel, the last byte of each pixel is used to
 * determine the boundary outline.
 *
 * Return value: the boundary array.
 **/
GimpBoundSeg *
gimp_boundary_find (GeglBuffer          *buffer,
                    const GeglRectangle *region,
                    const Babl          *format,
                    GimpBoundaryType     type,
                    int                  x1,
                    int                  y1,
                    int                  x2,
                    int                  y2,
                    gfloat               threshold,
                    int                 *num_segs)
{
  GimpBoundary  *boundary;
  GeglRectangle  rect = { 0, };

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);
  g_return_val_if_fail (num_segs != NULL, NULL);
  g_return_val_if_fail (format != NULL, NULL);
  g_return_val_if_fail (babl_format_get_bytes_per_pixel (format) ==
                        sizeof (gfloat), NULL);

  if (region)
    {
      rect = *region;
    }
  else
    {
      rect.width  = gegl_buffer_get_width  (buffer);
      rect.height = gegl_buffer_get_height (buffer);
    }

  boundary = generate_boundary (buffer, &rect, format, type,
                                x1, y1, x2, y2, threshold);

  *num_segs = boundary->num_segs;

  return gimp_boundary_free (boundary, FALSE);
}

/**
 * gimp_boundary_sort:
 * @segs:       unsorted input segs.
 * @num_segs:   number of input segs
 * @num_groups: number of groups in the sorted segs
 *
 * This function takes an array of #GimpBoundSeg's as returned by
 * gimp_boundary_find() and sorts it by contiguous groups. The returned
 * array contains markers consisting of -1 coordinates and is
 * @num_groups elements longer than @segs.
 *
 * Return value: the sorted segs
 **/
GimpBoundSeg *
gimp_boundary_sort (const GimpBoundSeg *segs,
                    gint                num_segs,
                    gint               *num_groups)
{
  GimpBoundary        *boundary;
  const GimpBoundSeg **segs_ptrs_by_xy1;
  const GimpBoundSeg **segs_ptrs_by_xy2;
  gint                 index;
  gint                 x, y;
  gint                 startx, starty;

  g_return_val_if_fail ((segs == NULL && num_segs == 0) ||
                        (segs != NULL && num_segs >  0), NULL);
  g_return_val_if_fail (num_groups != NULL, NULL);

  *num_groups = 0;

  if (num_segs == 0)
    return NULL;

  /* prepare arrays with GimpBoundSeg pointers sorted by xy1 and xy2
   * accordingly
   */
  segs_ptrs_by_xy1 = g_new (const GimpBoundSeg *, num_segs);
  segs_ptrs_by_xy2 = g_new (const GimpBoundSeg *, num_segs);

  for (index = 0; index < num_segs; index++)
    {
      segs_ptrs_by_xy1[index] = segs + index;
      segs_ptrs_by_xy2[index] = segs + index;
    }

  qsort (segs_ptrs_by_xy1, num_segs, sizeof (GimpBoundSeg *),
         (GCompareFunc) cmp_segptr_xy1_addr);
  qsort (segs_ptrs_by_xy2, num_segs, sizeof (GimpBoundSeg *),
         (GCompareFunc) cmp_segptr_xy2_addr);

  for (index = 0; index < num_segs; index++)
    ((GimpBoundSeg *) segs)[index].visited = FALSE;

  boundary = gimp_boundary_new (NULL);

  for (index = 0; index < num_segs; index++)
    {
      const GimpBoundSeg *cur_seg;

      if (segs[index].visited)
        continue;

      gimp_boundary_add_seg (boundary,
                             segs[index].x1, segs[index].y1,
                             segs[index].x2, segs[index].y2,
                             segs[index].open);

      ((GimpBoundSeg *) segs)[index].visited = TRUE;

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
              gimp_boundary_add_seg (boundary,
                                     cur_seg->x1, cur_seg->y1,
                                     cur_seg->x2, cur_seg->y2,
                                     cur_seg->open);
              x = cur_seg->x2;
              y = cur_seg->y2;
            }
          else
            {
              gimp_boundary_add_seg (boundary,
                                     cur_seg->x2, cur_seg->y2,
                                     cur_seg->x1, cur_seg->y1,
                                     cur_seg->open);
              x = cur_seg->x1;
              y = cur_seg->y1;
            }

          ((GimpBoundSeg *) cur_seg)->visited = TRUE;
        }

      if (G_UNLIKELY (x != startx || y != starty))
        g_warning ("sort_boundary(): Unconnected boundary group!");

      /*  Mark the end of a group  */
      *num_groups = *num_groups + 1;
      gimp_boundary_add_seg (boundary, -1, -1, -1, -1, 0);
  }

  g_free (segs_ptrs_by_xy1);
  g_free (segs_ptrs_by_xy2);

  return gimp_boundary_free (boundary, FALSE);
}

/**
 * gimp_boundary_simplify:
 * @sorted_segs: sorted input segs
 * @num_groups:  number of groups in the sorted segs
 * @num_segs:    number of returned segs.
 *
 * This function takes an array of #GimpBoundSeg's which has been sorted
 * with gimp_boundary_sort() and reduces the number of segments while
 * preserving the general shape as close as possible.
 *
 * Return value: the simplified segs.
 **/
GimpBoundSeg *
gimp_boundary_simplify (GimpBoundSeg *sorted_segs,
                        gint          num_groups,
                        gint         *num_segs)
{
  GArray *new_bounds;
  gint    i, seg;

  g_return_val_if_fail ((sorted_segs == NULL && num_groups == 0) ||
                        (sorted_segs != NULL && num_groups >  0), NULL);
  g_return_val_if_fail (num_segs != NULL, NULL);

  new_bounds = g_array_new (FALSE, FALSE, sizeof (GimpBoundSeg));

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
          GimpBoundSeg  tmp_seg;
          gint      j;

          tmp_points = g_array_new (FALSE, FALSE, sizeof (gint));

          /* temporarily use the delimiter to close the polygon */
          tmp_seg = sorted_segs[seg];
          sorted_segs[seg] = sorted_segs[start];
          simplify_subdivide (sorted_segs,
                              start, start + n_points, &tmp_points);
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

  return (GimpBoundSeg *) g_array_free (new_bounds, FALSE);
}

void
gimp_boundary_offset (GimpBoundSeg *segs,
                      gint          num_segs,
                      gint          off_x,
                      gint          off_y)
{
  gint i;

  g_return_if_fail ((segs == NULL && num_segs == 0) ||
                    (segs != NULL && num_segs >  0));

  for (i = 0; i < num_segs; i++)
    {
      /* don't offset sorting sentinels */
      if (!(segs[i].x1 == -1 &&
            segs[i].y1 == -1 &&
            segs[i].x2 == -1 &&
            segs[i].y2 == -1))
        {
          segs[i].x1 += off_x;
          segs[i].y1 += off_y;
          segs[i].x2 += off_x;
          segs[i].y2 += off_y;
        }
    }
}


/*  private functions  */

static GimpBoundary *
gimp_boundary_new (const GeglRectangle *region)
{
  GimpBoundary *boundary = g_slice_new0 (GimpBoundary);

  if (region)
    {
      gint i;

      /*  array for determining the vertical line segments
       *  which must be drawn
       */
      boundary->vert_segs = g_new (gint, region->width + region->x + 1);

      for (i = 0; i <= (region->width + region->x); i++)
        boundary->vert_segs[i] = -1;

      /*  find the maximum possible number of empty segments
       *  given the current mask
       */
      boundary->max_empty_segs = region->width + 3;

      boundary->empty_segs_n = g_new (gint, boundary->max_empty_segs);
      boundary->empty_segs_c = g_new (gint, boundary->max_empty_segs);
      boundary->empty_segs_l = g_new (gint, boundary->max_empty_segs);
    }

  return boundary;
}

static GimpBoundSeg *
gimp_boundary_free (GimpBoundary *boundary,
                    gboolean      free_segs)
{
  GimpBoundSeg *segs = NULL;

  if (free_segs)
    g_free (boundary->segs);
  else
    segs = boundary->segs;

  g_free (boundary->vert_segs);
  g_free (boundary->empty_segs_n);
  g_free (boundary->empty_segs_c);
  g_free (boundary->empty_segs_l);

  g_slice_free (GimpBoundary, boundary);

  return segs;
}

static void
gimp_boundary_add_seg (GimpBoundary *boundary,
                       gint          x1,
                       gint          y1,
                       gint          x2,
                       gint          y2,
                       gboolean      open)
{
  if (boundary->num_segs >= boundary->max_segs)
    {
      boundary->max_segs += MAX_SEGS_INC;

      boundary->segs = g_renew (GimpBoundSeg, boundary->segs, boundary->max_segs);
    }

  boundary->segs[boundary->num_segs].x1   = x1;
  boundary->segs[boundary->num_segs].y1   = y1;
  boundary->segs[boundary->num_segs].x2   = x2;
  boundary->segs[boundary->num_segs].y2   = y2;
  boundary->segs[boundary->num_segs].open = open;

  boundary->num_segs ++;
}

static void
find_empty_segs (const GeglRectangle *region,
                 const gfloat        *line_data,
                 gint                 scanline,
                 gint                 empty_segs[],
                 gint                 max_empty,
                 gint                *num_empty,
                 GimpBoundaryType     type,
                 gint                 x1,
                 gint                 y1,
                 gint                 x2,
                 gint                 y2,
                 gfloat               threshold)
{
  gint start = 0;
  gint end   = 0;
  gint endx  = 0;
  gint last  = -1;
  gint l_num_empty;
  gint x;

  *num_empty = 0;

  if (scanline < region->y || scanline >= (region->y + region->height))
    {
      empty_segs[(*num_empty)++] = 0;
      empty_segs[(*num_empty)++] = G_MAXINT;
      return;
    }

  if (type == GIMP_BOUNDARY_WITHIN_BOUNDS)
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
  else if (type == GIMP_BOUNDARY_IGNORE_BOUNDS)
    {
      start = region->x;
      end   = region->x + region->width;
      if (scanline < y1 || scanline >= y2)
        x2 = -1;
    }

  empty_segs[(*num_empty)++] = 0;

  l_num_empty = *num_empty;

  endx = end;

  line_data += start;

  for (x = start; x < end;)
    {
      if (type == GIMP_BOUNDARY_IGNORE_BOUNDS && (endx > x1 || x < x2))
        {
          for (; x < endx; x++)
            {
              gint val;

              if (*line_data > threshold)
                {
                  if (x >= x1 && x < x2)
                    val = -1;
                  else
                    val = 1;
                }
              else
                {
                  val = -1;
                }

              line_data++;

              if (last != val)
                empty_segs[l_num_empty++] = x;

              last = val;
            }
        }
      else
        {
          for (; x < endx; x++)
            {
              gint val;

              if (*line_data > threshold)
                val = 1;
              else
                val = -1;

              line_data++;

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
}

static void
process_horiz_seg (GimpBoundary *boundary,
                   gint          x1,
                   gint          y1,
                   gint          x2,
                   gint          y2,
                   gboolean      open)
{
  /*  This procedure accounts for any vertical segments that must be
      drawn to close in the horizontal segments.                     */

  if (boundary->vert_segs[x1] >= 0)
    {
      gimp_boundary_add_seg (boundary, x1, boundary->vert_segs[x1], x1, y1, !open);
      boundary->vert_segs[x1] = -1;
    }
  else
    boundary->vert_segs[x1] = y1;

  if (boundary->vert_segs[x2] >= 0)
    {
      gimp_boundary_add_seg (boundary, x2, boundary->vert_segs[x2], x2, y2, open);
      boundary->vert_segs[x2] = -1;
    }
  else
    boundary->vert_segs[x2] = y2;

  gimp_boundary_add_seg (boundary, x1, y1, x2, y2, open);
}

static void
make_horiz_segs (GimpBoundary *boundary,
                 gint          start,
                 gint          end,
                 gint          scanline,
                 gint          empty[],
                 gint          num_empty,
                 gint          top)
{
  gint empty_index;
  gint e_s, e_e;    /* empty segment start and end values */

  for (empty_index = 0; empty_index < num_empty; empty_index += 2)
    {
      e_s = *empty++;
      e_e = *empty++;

      if (e_s <= start && e_e >= end)
        {
          process_horiz_seg (boundary,
                             start, scanline, end, scanline, top);
        }
      else if ((e_s > start && e_s < end) ||
               (e_e < end && e_e > start))
        {
          process_horiz_seg (boundary,
                             MAX (e_s, start), scanline,
                             MIN (e_e, end), scanline, top);
        }
    }
}

static GimpBoundary *
generate_boundary (GeglBuffer          *buffer,
                   const GeglRectangle *region,
                   const Babl          *format,
                   GimpBoundaryType     type,
                   gint                 x1,
                   gint                 y1,
                   gint                 x2,
                   gint                 y2,
                   gfloat               threshold)
{
  GimpBoundary  *boundary;
  GeglRectangle  line_rect = { 0, };
  gfloat        *line_data;
  gint           scanline;
  gint           i;
  gint           start, end;
  gint          *tmp_segs;

  gint          num_empty_n = 0;
  gint          num_empty_c = 0;
  gint          num_empty_l = 0;

  boundary = gimp_boundary_new (region);

  line_rect.width  = gegl_buffer_get_width (buffer);
  line_rect.height = 1;

  line_data = g_alloca (sizeof (gfloat) * line_rect.width);

  start = 0;
  end   = 0;

  if (type == GIMP_BOUNDARY_WITHIN_BOUNDS)
    {
      start = y1;
      end   = y2;
    }
  else if (type == GIMP_BOUNDARY_IGNORE_BOUNDS)
    {
      start = region->y;
      end   = region->y + region->height;
    }

  /*  Find the empty segments for the previous and current scanlines  */
  find_empty_segs (region, NULL,
                   start - 1, boundary->empty_segs_l,
                   boundary->max_empty_segs, &num_empty_l,
                   type, x1, y1, x2, y2,
                   threshold);

  line_rect.y = start;
  gegl_buffer_get (buffer, &line_rect, 1.0, format,
                   line_data, GEGL_AUTO_ROWSTRIDE,
                   GEGL_ABYSS_NONE);

  find_empty_segs (region, line_data,
                   start, boundary->empty_segs_c,
                   boundary->max_empty_segs, &num_empty_c,
                   type, x1, y1, x2, y2,
                   threshold);

  for (scanline = start; scanline < end; scanline++)
    {
      /*  find the empty segment list for the next scanline  */
      line_rect.y = scanline + 1;
      if (scanline + 1 == end)
        line_data = NULL;
      else
        gegl_buffer_get (buffer, &line_rect, 1.0, format,
                         line_data, GEGL_AUTO_ROWSTRIDE,
                         GEGL_ABYSS_NONE);

      find_empty_segs (region, line_data,
                       scanline + 1, boundary->empty_segs_n,
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

static inline gint
cmp_xy (const gint ax,
        const gint ay,
        const gint bx,
        const gint by)
{
  if (ay < by)
    {
      return -1;
    }
  else if (ay > by)
    {
      return 1;
    }
  else if (ax < bx)
    {
      return -1;
    }
  else if (ax > bx)
    {
      return 1;
    }
  else
    {
      return 0;
    }
}


/*
 * Compares (x1, y1) pairs in specified segments, using their addresses if
 * (x1, y1) pairs are equal.
 */
static gint
cmp_segptr_xy1_addr (const GimpBoundSeg **seg_ptr_a,
                     const GimpBoundSeg **seg_ptr_b)
{
  const GimpBoundSeg *seg_a = *seg_ptr_a;
  const GimpBoundSeg *seg_b = *seg_ptr_b;

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
cmp_segptr_xy2_addr (const GimpBoundSeg **seg_ptr_a,
                     const GimpBoundSeg **seg_ptr_b)
{
  const GimpBoundSeg *seg_a = *seg_ptr_a;
  const GimpBoundSeg *seg_b = *seg_ptr_b;

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
cmp_segptr_xy1 (const GimpBoundSeg **seg_ptr_a,
                const GimpBoundSeg **seg_ptr_b)
{
  const GimpBoundSeg *seg_a = *seg_ptr_a, *seg_b = *seg_ptr_b;

  return cmp_xy (seg_a->x1, seg_a->y1, seg_b->x1, seg_b->y1);
}

/*
 * Compares (x2, y2) pairs in specified segments.
 */
static gint
cmp_segptr_xy2 (const GimpBoundSeg **seg_ptr_a,
                const GimpBoundSeg **seg_ptr_b)
{
  const GimpBoundSeg *seg_a = *seg_ptr_a;
  const GimpBoundSeg *seg_b = *seg_ptr_b;

  return cmp_xy (seg_a->x2, seg_a->y2, seg_b->x2, seg_b->y2);
}


static const GimpBoundSeg *
find_segment (const GimpBoundSeg **segs_by_xy1,
              const GimpBoundSeg **segs_by_xy2,
              gint                 num_segs,
              gint                 x,
              gint                 y)
{
  const GimpBoundSeg *segptr_xy1;
  const GimpBoundSeg *segptr_xy2;
  GimpBoundSeg        search_seg;

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


static const GimpBoundSeg *
find_segment_with_func (const GimpBoundSeg **segs,
                        gint                 num_segs,
                        const GimpBoundSeg  *search_seg,
                        GCompareFunc         cmp_func)
{
  const GimpBoundSeg **seg;
  const GimpBoundSeg *found_seg = NULL;

  seg = bsearch (&search_seg, segs, num_segs, sizeof (GimpBoundSeg *), cmp_func);

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
simplify_subdivide (const GimpBoundSeg *segs,
                    gint                start_idx,
                    gint                end_idx,
                    GArray            **ret_points)
{
  gint maxdist_idx;
  gint maxdist;
  gint threshold;
  gint i, dx, dy;

  if (end_idx - start_idx < 2)
    {
      *ret_points = g_array_append_val (*ret_points, start_idx);
      return;
    }

  maxdist = 0;

  if (segs[start_idx].x1 == segs[end_idx].x1 &&
      segs[start_idx].y1 == segs[end_idx].y1)
    {
      /* start and endpoint are at the same coordinates */
      for (i = start_idx + 1; i < end_idx; i++)
        {
          /* compare the sqared distances */
          gint dist = (SQR (segs[i].x1 - segs[start_idx].x1) +
                       SQR (segs[i].y1 - segs[start_idx].y1));

          if (dist > maxdist)
            {
              maxdist = dist;
            }
        }

      threshold = 1;
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
          gint dist = abs (dx * (segs[start_idx].y1 - segs[i].y1) -
                           dy * (segs[start_idx].x1 - segs[i].x1));

          if (dist > maxdist)
            {
              maxdist = dist;
            }
        }

      /* threshold is chosen to catch 45 degree stairs */
      threshold = SQR (dx) + SQR (dy);
    }

  if (maxdist <= threshold)
    {
      *ret_points = g_array_append_val (*ret_points, start_idx);
      return;
    }

  /* Simons hack */
  maxdist_idx = (start_idx + end_idx) / 2;

  simplify_subdivide (segs, start_idx, maxdist_idx, ret_points);
  simplify_subdivide (segs, maxdist_idx, end_idx, ret_points);
}
