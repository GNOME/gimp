/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpchunkiterator.c
 * Copyright (C) 2019 Ell
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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gimpchunkiterator.h"


/* the maximal chunk size */
#define MAX_CHUNK_WIDTH          4096
#define MAX_CHUNK_HEIGHT         4096

/* the default iteration interval */
#define DEFAULT_INTERVAL         (1.0 / 15.0) /* seconds */

/* the minimal area to process per iteration */
#define MIN_AREA_PER_ITERATION   4096

/* the maximal ratio between the actual processed area and the target area,
 * above which the current chunk height is readjusted, even in the middle of a
 * row, to better match the target area
 */
#define MAX_AREA_RATIO           2.0

/* the width of the target-area sliding window */
#define TARGET_AREA_HISTORY_SIZE 3


struct _GimpChunkIterator
{
  cairo_region_t *region;
  cairo_region_t *priority_region;

  GeglRectangle   tile_rect;
  GeglRectangle   priority_rect;

  gdouble         interval;

  cairo_region_t *current_region;
  GeglRectangle   current_rect;

  gint            current_x;
  gint            current_y;
  gint            current_height;

  gint64          iteration_time;

  gint64          last_time;
  gint            last_area;

  gdouble         target_area;
  gdouble         target_area_min;
  gdouble         target_area_history[TARGET_AREA_HISTORY_SIZE];
  gint            target_area_history_i;
  gint            target_area_history_n;
};


/*  local function prototypes  */

static void       gimp_chunk_iterator_set_current_rect   (GimpChunkIterator   *iter,
                                                          const GeglRectangle *rect);
static void       gimp_chunk_iterator_merge_current_rect (GimpChunkIterator   *iter);

static void       gimp_chunk_iterator_merge              (GimpChunkIterator   *iter);

static gboolean   gimp_chunk_iterator_prepare            (GimpChunkIterator   *iter);

static void       gimp_chunk_iterator_set_target_area    (GimpChunkIterator   *iter,
                                                          gdouble              target_area);
static gdouble    gimp_chunk_iterator_get_target_area    (GimpChunkIterator   *iter);
static void       gimp_chunk_iterator_reset_target_area  (GimpChunkIterator   *iter);

static void       gimp_chunk_iterator_calc_rect          (GimpChunkIterator   *iter,
                                                          GeglRectangle       *rect,
                                                          gboolean             readjust_height);


/*  private functions  */

static void
gimp_chunk_iterator_set_current_rect (GimpChunkIterator   *iter,
                                      const GeglRectangle *rect)
{
  cairo_region_subtract_rectangle (iter->current_region,
                                   (const cairo_rectangle_int_t *) rect);

  iter->current_rect   = *rect;

  iter->current_x      = rect->x;
  iter->current_y      = rect->y;
  iter->current_height = 0;
}

static void
gimp_chunk_iterator_merge_current_rect (GimpChunkIterator *iter)
{
  GeglRectangle rect;

  if (gegl_rectangle_is_empty (&iter->current_rect))
    return;

  /* merge the remainder of the current row */
  rect.x      = iter->current_x;
  rect.y      = iter->current_y;
  rect.width  = iter->current_rect.x + iter->current_rect.width -
                iter->current_x;
  rect.height = iter->current_height;

  cairo_region_union_rectangle (iter->current_region,
                                (const cairo_rectangle_int_t *) &rect);

  /* merge the remainder of the current rect */
  rect.x      = iter->current_rect.x;
  rect.y      = iter->current_y + iter->current_height;
  rect.width  = iter->current_rect.width;
  rect.height = iter->current_rect.y + iter->current_rect.height - rect.y;

  cairo_region_union_rectangle (iter->current_region,
                                (const cairo_rectangle_int_t *) &rect);

  /* reset the current rect and coordinates */
  iter->current_rect.x      = 0;
  iter->current_rect.y      = 0;
  iter->current_rect.width  = 0;
  iter->current_rect.height = 0;

  iter->current_x           = 0;
  iter->current_y           = 0;
  iter->current_height      = 0;
}

static void
gimp_chunk_iterator_merge (GimpChunkIterator *iter)
{
  /* merge the current rect back to the current region */
  gimp_chunk_iterator_merge_current_rect (iter);

  /* merge the priority region back to the global region */
  if (iter->priority_region)
    {
      cairo_region_union (iter->region, iter->priority_region);

      g_clear_pointer (&iter->priority_region, cairo_region_destroy);

      iter->current_region = iter->region;
    }
}

static gboolean
gimp_chunk_iterator_prepare (GimpChunkIterator *iter)
{
  if (iter->current_x == iter->current_rect.x + iter->current_rect.width)
    {
      iter->current_x       = iter->current_rect.x;
      iter->current_y      += iter->current_height;
      iter->current_height  = 0;

      if (iter->current_y == iter->current_rect.y + iter->current_rect.height)
        {
          GeglRectangle rect;

          if (! iter->priority_region &&
              ! gegl_rectangle_is_empty (&iter->priority_rect))
            {
              iter->priority_region = cairo_region_copy (iter->region);

              cairo_region_intersect_rectangle (
                iter->priority_region,
                (const cairo_rectangle_int_t *) &iter->priority_rect);

              cairo_region_subtract_rectangle (
                iter->region,
                (const cairo_rectangle_int_t *) &iter->priority_rect);
            }

          if (! iter->priority_region ||
              cairo_region_is_empty (iter->priority_region))
            {
              iter->current_region = iter->region;
            }
          else
            {
              iter->current_region = iter->priority_region;
            }

          if (cairo_region_is_empty (iter->current_region))
            {
              iter->current_rect.x      = 0;
              iter->current_rect.y      = 0;
              iter->current_rect.width  = 0;
              iter->current_rect.height = 0;

              iter->current_x           = 0;
              iter->current_y           = 0;
              iter->current_height      = 0;

              return FALSE;
            }

          cairo_region_get_rectangle (iter->current_region, 0,
                                      (cairo_rectangle_int_t *) &rect);

          gimp_chunk_iterator_set_current_rect (iter, &rect);
        }
    }

  return TRUE;
}

static gint
compare_double (const gdouble *x,
                const gdouble *y)
{
  return (*x > *y) - (*x < *y);
}

static void
gimp_chunk_iterator_set_target_area (GimpChunkIterator *iter,
                                     gdouble            target_area)
{
  gdouble target_area_history[TARGET_AREA_HISTORY_SIZE];

  iter->target_area_min = MIN (iter->target_area_min, target_area);

  iter->target_area_history[iter->target_area_history_i++] = target_area;

  iter->target_area_history_n  = MAX (iter->target_area_history_n,
                                      iter->target_area_history_i);
  iter->target_area_history_i %= TARGET_AREA_HISTORY_SIZE;

  memcpy (target_area_history, iter->target_area_history,
          iter->target_area_history_n * sizeof (gdouble));

  qsort (target_area_history, iter->target_area_history_n, sizeof (gdouble),
         (gpointer) compare_double);

  iter->target_area = target_area_history[iter->target_area_history_n / 2];
}

static gdouble
gimp_chunk_iterator_get_target_area (GimpChunkIterator *iter)
{
  if (iter->target_area)
    return iter->target_area;
  else
    return iter->tile_rect.width * iter->tile_rect.height;
}

static void
gimp_chunk_iterator_reset_target_area (GimpChunkIterator *iter)
{
  if (iter->target_area_history_n)
    {
      iter->target_area           = iter->target_area_min;
      iter->target_area_min       = MAX_CHUNK_WIDTH * MAX_CHUNK_HEIGHT;
      iter->target_area_history_i = 0;
      iter->target_area_history_n = 0;
    }
}

static void
gimp_chunk_iterator_calc_rect (GimpChunkIterator *iter,
                               GeglRectangle     *rect,
                               gboolean           readjust_height)
{
  gdouble target_area;
  gdouble aspect_ratio;
  gint    offset_x;
  gint    offset_y;

  if (readjust_height)
    gimp_chunk_iterator_reset_target_area (iter);

  target_area = gimp_chunk_iterator_get_target_area (iter);

  aspect_ratio = (gdouble) iter->tile_rect.height /
                 (gdouble) iter->tile_rect.width;

  rect->x = iter->current_x;
  rect->y = iter->current_y;

  offset_x = rect->x - iter->tile_rect.x;
  offset_y = rect->y - iter->tile_rect.y;

  if (readjust_height)
    {
      rect->height = RINT ((offset_y + sqrt (target_area * aspect_ratio)) /
                           iter->tile_rect.height)                        *
                     iter->tile_rect.height                               -
                     offset_y;

      if (rect->height <= 0)
        rect->height += iter->tile_rect.height;

      rect->height = MIN (rect->height,
                          iter->current_rect.y + iter->current_rect.height -
                          rect->y);
      rect->height = MIN (rect->height, MAX_CHUNK_HEIGHT);
    }
  else
    {
      rect->height = iter->current_height;
    }

  rect->width = RINT ((offset_x + (gdouble) target_area   /
                                  (gdouble) rect->height) /
                      iter->tile_rect.width)              *
                iter->tile_rect.width                     -
                offset_x;

  if (rect->width <= 0)
    rect->width += iter->tile_rect.width;

  rect->width = MIN (rect->width,
                     iter->current_rect.x + iter->current_rect.width -
                     rect->x);
  rect->width = MIN (rect->width, MAX_CHUNK_WIDTH);
}


/*  public functions  */

GimpChunkIterator *
gimp_chunk_iterator_new (cairo_region_t *region)
{
  GimpChunkIterator *iter;

  g_return_val_if_fail (region != NULL, NULL);

  iter = g_slice_new0 (GimpChunkIterator);

  iter->region         = region;
  iter->current_region = region;

  g_object_get (gegl_config (),
                "tile-width",  &iter->tile_rect.width,
                "tile-height", &iter->tile_rect.height,
                NULL);

  iter->interval = DEFAULT_INTERVAL;

  return iter;
}

void
gimp_chunk_iterator_set_tile_rect (GimpChunkIterator   *iter,
                                   const GeglRectangle *rect)
{
  g_return_if_fail (iter != NULL);
  g_return_if_fail (rect != NULL);
  g_return_if_fail (! gegl_rectangle_is_empty (rect));

  iter->tile_rect = *rect;
}

void
gimp_chunk_iterator_set_priority_rect (GimpChunkIterator   *iter,
                                       const GeglRectangle *rect)
{
  const GeglRectangle empty_rect = {};

  g_return_if_fail (iter != NULL);

  if (! rect)
    rect = &empty_rect;

  if (! gegl_rectangle_equal (rect, &iter->priority_rect))
    {
      iter->priority_rect = *rect;

      gimp_chunk_iterator_merge (iter);
    }
}

void
gimp_chunk_iterator_set_interval (GimpChunkIterator *iter,
                                  gdouble            interval)
{
  g_return_if_fail (iter != NULL);

  interval = MAX (interval, 0.0);

  if (interval != iter->interval)
    {
      if (iter->interval)
        {
          gdouble ratio = interval / iter->interval;
          gint    i;

          iter->target_area *= ratio;

          for (i = 0; i < TARGET_AREA_HISTORY_SIZE; i++)
            iter->target_area_history[i] *= ratio;
        }

      iter->interval = interval;
    }
}

gboolean
gimp_chunk_iterator_next (GimpChunkIterator *iter)
{
  g_return_val_if_fail (iter != NULL, FALSE);

  if (! gimp_chunk_iterator_prepare (iter))
    {
      gimp_chunk_iterator_stop (iter, TRUE);

      return FALSE;
    }

  iter->iteration_time = g_get_monotonic_time ();

  iter->last_time = iter->iteration_time;
  iter->last_area = 0;

  return TRUE;
}

gboolean
gimp_chunk_iterator_get_rect (GimpChunkIterator *iter,
                              GeglRectangle     *rect)
{
  gint64 time;

  g_return_val_if_fail (iter != NULL, FALSE);
  g_return_val_if_fail (rect != NULL, FALSE);

  if (! gimp_chunk_iterator_prepare (iter))
    return FALSE;

  time = g_get_monotonic_time ();

  if (iter->last_area >= MIN_AREA_PER_ITERATION)
    {
      gdouble interval;

      interval = (gdouble) (time - iter->last_time) / G_TIME_SPAN_SECOND;

      gimp_chunk_iterator_set_target_area (
        iter,
        iter->last_area * iter->interval / interval);

      interval = (gdouble) (time - iter->iteration_time) / G_TIME_SPAN_SECOND;

      if (interval > iter->interval)
        return FALSE;
    }

  if (iter->current_x == iter->current_rect.x)
    {
      gimp_chunk_iterator_calc_rect (iter, rect, TRUE);
    }
  else
    {
      gimp_chunk_iterator_calc_rect (iter, rect, FALSE);

      if (rect->width * rect->height >=
          MAX_AREA_RATIO * gimp_chunk_iterator_get_target_area (iter))
        {
          GeglRectangle old_rect = *rect;

          gimp_chunk_iterator_calc_rect (iter, rect, TRUE);

          if (rect->height >= old_rect.height)
            *rect = old_rect;
        }
    }

  if (rect->height != iter->current_height)
    {
      /* if the chunk height changed in the middle of a row, merge the
       * remaining area back into the current region, and reset the current
       * area to the remainder of the row, using the new chunk height
       */
      if (rect->x != iter->current_rect.x)
        {
          GeglRectangle rem;

          rem.x      = rect->x;
          rem.y      = rect->y;
          rem.width  = iter->current_rect.x + iter->current_rect.width -
                       rect->x;
          rem.height = rect->height;

          gimp_chunk_iterator_merge_current_rect (iter);

          gimp_chunk_iterator_set_current_rect (iter, &rem);
        }

      iter->current_height = rect->height;
    }

  iter->current_x += rect->width;

  iter->last_time = time;
  iter->last_area = rect->width * rect->height;

  return TRUE;
}

cairo_region_t *
gimp_chunk_iterator_stop (GimpChunkIterator *iter,
                          gboolean           free_region)
{
  cairo_region_t *result = NULL;

  g_return_val_if_fail (iter != NULL, NULL);

  if (free_region)
    {
      cairo_region_destroy (iter->region);
    }
  else
    {
      gimp_chunk_iterator_merge (iter);

      result = iter->region;
    }

  g_clear_pointer (&iter->priority_region, cairo_region_destroy);

  g_slice_free (GimpChunkIterator, iter);

  return result;
}
