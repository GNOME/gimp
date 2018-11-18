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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gegl/gimp-babl.h"

#include "gimp-utils.h" /* GIMP_TIMER */
#include "gimplineart.h"
#include "gimppickable.h"
#include "gimppickable-contiguous-region.h"


/*  local function prototypes  */

static const Babl * choose_format         (GeglBuffer          *buffer,
                                           GimpSelectCriterion  select_criterion,
                                           gint                *n_components,
                                           gboolean            *has_alpha);
static gfloat   pixel_difference          (const gfloat        *col1,
                                           const gfloat        *col2,
                                           gboolean             antialias,
                                           gfloat               threshold,
                                           gint                 n_components,
                                           gboolean             has_alpha,
                                           gboolean             select_transparent,
                                           GimpSelectCriterion  select_criterion);
static void     push_segment              (GQueue              *segment_queue,
                                           gint                 y,
                                           gint                 old_y,
                                           gint                 start,
                                           gint                 end,
                                           gint                 new_y,
                                           gint                 new_start,
                                           gint                 new_end);
static void     pop_segment               (GQueue              *segment_queue,
                                           gint                *y,
                                           gint                *old_y,
                                           gint                *start,
                                           gint                *end);
static gboolean find_contiguous_segment   (const gfloat        *col,
                                           GeglBuffer          *src_buffer,
                                           GeglSampler         *src_sampler,
                                           GeglBuffer          *mask_buffer,
                                           const Babl          *src_format,
                                           const Babl          *mask_format,
                                           gint                 n_components,
                                           gboolean             has_alpha,
                                           gint                 width,
                                           gboolean             select_transparent,
                                           GimpSelectCriterion  select_criterion,
                                           gboolean             antialias,
                                           gfloat               threshold,
                                           gint                 initial_x,
                                           gint                 initial_y,
                                           gint                *start,
                                           gint                *end,
                                           gfloat              *row);
static void     find_contiguous_region    (GeglBuffer          *src_buffer,
                                           GeglBuffer          *mask_buffer,
                                           const Babl          *format,
                                           gint                 n_components,
                                           gboolean             has_alpha,
                                           gboolean             select_transparent,
                                           GimpSelectCriterion  select_criterion,
                                           gboolean             antialias,
                                           gfloat               threshold,
                                           gboolean             diagonal_neighbors,
                                           gint                 x,
                                           gint                 y,
                                           const gfloat        *col);


/*  public functions  */

GeglBuffer *
gimp_pickable_contiguous_region_prepare_line_art (GimpPickable *pickable,
                                                  gboolean      select_transparent,
                                                  gfloat        stroke_threshold)
{
  GeglBuffer *lineart;
  gboolean    has_alpha;

  g_return_val_if_fail (GIMP_IS_PICKABLE (pickable), NULL);

  gimp_pickable_flush (pickable);

  lineart = gimp_pickable_get_buffer (pickable);
  has_alpha = babl_format_has_alpha (gegl_buffer_get_format (lineart));

  if (! has_alpha)
    {
      if (select_transparent)
        {
          /*  don't select transparent regions if there are no fully
           *  transparent pixels.
           */
          GeglBufferIterator *gi;

          select_transparent = FALSE;
          gi = gegl_buffer_iterator_new (lineart, NULL, 0, babl_format ("A u8"),
                                         GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 3);
          while (gegl_buffer_iterator_next (gi))
            {
              guint8 *p = (guint8*) gi->items[0].data;
              gint    k;

              for (k = 0; k < gi->length; k++)
                {
                  if (! *p)
                    {
                      select_transparent = TRUE;
                      break;
                    }
                  p++;
                }
              if (select_transparent)
                break;
            }
          if (select_transparent)
            gegl_buffer_iterator_stop (gi);
        }
    }
  else
    {
      select_transparent = FALSE;
    }

  /* For smart selection, we generate a binarized image with close
   * regions, then run a composite selection with no threshold on
   * this intermediate buffer.
   */
  GIMP_TIMER_START();

  lineart = gimp_lineart_close (lineart,
                                select_transparent,
                                stroke_threshold,
                                /*minimal_lineart_area,*/
                                5,
                                /*normal_estimate_mask_size,*/
                                5,
                                /*end_point_rate,*/
                                0.85,
                                /*spline_max_length,*/
                                60,
                                /*spline_max_angle,*/
                                90.0,
                                /*end_point_connectivity,*/
                                2,
                                /*spline_roundness,*/
                                1.0,
                                /*allow_self_intersections,*/
                                TRUE,
                                /*created_regions_significant_area,*/
                                4,
                                /*created_regions_minimum_area,*/
                                100,
                                /*small_segments_from_spline_sources,*/
                                TRUE,
                                /*segments_max_length*/
                                20);

  GIMP_TIMER_END("close line-art");

  return lineart;
}

GeglBuffer *
gimp_pickable_contiguous_region_by_seed (GimpPickable        *pickable,
                                         GeglBuffer          *line_art,
                                         gboolean             antialias,
                                         gfloat               threshold,
                                         gboolean             select_transparent,
                                         GimpSelectCriterion  select_criterion,
                                         gboolean             diagonal_neighbors,
                                         gfloat               stroke_threshold,
                                         gint                 x,
                                         gint                 y)
{
  GeglBuffer    *src_buffer;
  GeglBuffer    *mask_buffer;
  const Babl    *format;
  GeglRectangle  extent;
  gint           n_components;
  gboolean       has_alpha;
  gfloat         start_col[MAX_CHANNELS];
  gfloat         flag           = 2.0;
  gboolean       smart_line_art = FALSE;
  gboolean       free_line_art  = FALSE;

  g_return_val_if_fail (GIMP_IS_PICKABLE (pickable), NULL);

  if (select_criterion == GIMP_SELECT_CRITERION_LINE_ART)
    {
      if (line_art == NULL)
        {
          /* It is much better experience to pre-compute the line art,
           * but it may not be always possible (for instance when
           * selecting/filling through a PDB call).
           */
          line_art      = gimp_pickable_contiguous_region_prepare_line_art (pickable, select_transparent,
                                                                            stroke_threshold);
          free_line_art = TRUE;
        }

      src_buffer = line_art;

      smart_line_art     = TRUE;
      antialias          = FALSE;
      threshold          = 0.0;
      select_transparent = FALSE;
      select_criterion   = GIMP_SELECT_CRITERION_COMPOSITE;
      diagonal_neighbors = FALSE;

      format = choose_format (src_buffer, select_criterion,
                              &n_components, &has_alpha);
      gegl_buffer_sample (src_buffer, x, y, NULL, start_col, format,
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
    }
  else
    {
      gimp_pickable_flush (pickable);
      src_buffer = gimp_pickable_get_buffer (pickable);

      format = choose_format (src_buffer, select_criterion,
                              &n_components, &has_alpha);
      gegl_buffer_sample (src_buffer, x, y, NULL, start_col, format,
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

      if (has_alpha)
        {
          if (select_transparent)
            {
              /*  don't select transparent regions if the start pixel isn't
               *  fully transparent
               */
              if (start_col[n_components - 1] > 0)
                select_transparent = FALSE;
            }
        }
      else
        {
          select_transparent = FALSE;
        }
    }

  extent = *gegl_buffer_get_extent (src_buffer);

  mask_buffer = gegl_buffer_new (&extent, babl_format ("Y float"));

  if (smart_line_art && start_col[0])
    {
      /* As a special exception, if you fill over a line art pixel, only
       * fill the pixel and exit
       */
      start_col[0] = 1.0;
      gegl_buffer_set (mask_buffer, GEGL_RECTANGLE (x, y, 1, 1),
                       0, babl_format ("Y float"), start_col,
                       GEGL_AUTO_ROWSTRIDE);
      smart_line_art = FALSE;
    }
  else if (x >= extent.x && x < (extent.x + extent.width) &&
      y >= extent.y && y < (extent.y + extent.height))
    {
      GIMP_TIMER_START();

      find_contiguous_region (src_buffer, mask_buffer,
                              format, n_components, has_alpha,
                              select_transparent, select_criterion,
                              antialias, threshold, diagonal_neighbors,
                              x, y, start_col);

      GIMP_TIMER_END("foo");
    }

  if (smart_line_art)
    {
      /* The last step of the line art algorithm is to make sure that
       * selections does not leave "holes" between its borders and the
       * line arts, while not stepping over as well.
       * To achieve this, I label differently the selection and the rest
       * and leave the stroke pixels unlabelled, then I let these
       * unknown pixels be labelled by flooding through watershed.
       */
      GeglBufferIterator *gi;
      GeglBuffer         *priomap;
      GeglNode           *graph;
      GeglNode           *input;
      GeglNode           *aux;
      GeglNode           *op;

      GIMP_TIMER_START();

      /* Flag the mask on the line art and create a priority map. */
      priomap = gegl_buffer_new (gegl_buffer_get_extent (mask_buffer),
                                 babl_format ("Y u8"));

      gi = gegl_buffer_iterator_new (src_buffer, NULL, 0, NULL,
                                     GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 3);
      gegl_buffer_iterator_add (gi, mask_buffer, NULL, 0,
                                babl_format ("Y float"),
                                GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE);
      gegl_buffer_iterator_add (gi, priomap, NULL, 0,
                                babl_format ("Y u8"),
                                GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

      while (gegl_buffer_iterator_next (gi))
        {
          guchar *lineart = (guchar*) gi->items[0].data;
          gfloat *mask    = (gfloat*) gi->items[1].data;
          guint8 *prio    = (guint8*) gi->items[2].data;
          gint    k;

          for (k = 0; k < gi->length; k++)
            {
              if (*lineart)
                {
                  *prio = 1;
                  if (! *mask)
                    *mask = flag;
                }
              else
                {
                  *prio = 0;
                }
              lineart++;
              mask++;
              prio++;
            }
        }

      /* Watershed the line art. */
      graph = gegl_node_new ();
      input = gegl_node_new_child (graph,
                                   "operation", "gegl:buffer-source",
                                   "buffer", mask_buffer,
                                   NULL);
      aux = gegl_node_new_child (graph,
                                 "operation", "gegl:buffer-source",
                                 "buffer", priomap,
                                 NULL);
      op  = gegl_node_new_child (graph,
                                 "operation", "gegl:watershed-transform",
                                 "flag-component", 0,
                                 "flag", &flag,
                                 NULL);
      gegl_node_connect_to (input, "output",
                            op, "input");
      gegl_node_connect_to (aux, "output",
                            op, "aux");
      gegl_node_blit_buffer (op, mask_buffer, NULL, 0, GEGL_ABYSS_NONE);
      g_object_unref (graph);
      g_object_unref (priomap);

      GIMP_TIMER_END("watershed line art");
    }
  if (free_line_art)
    g_object_unref (src_buffer);

  return mask_buffer;
}

GeglBuffer *
gimp_pickable_contiguous_region_by_color (GimpPickable        *pickable,
                                          gboolean             antialias,
                                          gfloat               threshold,
                                          gboolean             select_transparent,
                                          GimpSelectCriterion  select_criterion,
                                          const GimpRGB       *color)
{
  /*  Scan over the pickable's active layer, finding pixels within the
   *  specified threshold from the given R, G, & B values.  If
   *  antialiasing is on, use the same antialiasing scheme as in
   *  fuzzy_select.  Modify the pickable's mask to reflect the
   *  additional selection
   */
  GeglBufferIterator *iter;
  GeglBuffer         *src_buffer;
  GeglBuffer         *mask_buffer;
  const Babl         *format;
  gint                n_components;
  gboolean            has_alpha;
  gfloat              start_col[MAX_CHANNELS];

  g_return_val_if_fail (GIMP_IS_PICKABLE (pickable), NULL);
  g_return_val_if_fail (color != NULL, NULL);

  gimp_pickable_flush (pickable);

  src_buffer = gimp_pickable_get_buffer (pickable);

  format = choose_format (src_buffer, select_criterion,
                          &n_components, &has_alpha);

  gimp_rgba_get_pixel (color, format, start_col);

  if (has_alpha)
    {
      if (select_transparent)
        {
          /*  don't select transparency if "color" isn't fully transparent
           */
          if (start_col[n_components - 1] > 0.0)
            select_transparent = FALSE;
        }
    }
  else
    {
      select_transparent = FALSE;
    }

  mask_buffer = gegl_buffer_new (gegl_buffer_get_extent (src_buffer),
                                 babl_format ("Y float"));

  iter = gegl_buffer_iterator_new (src_buffer,
                                   NULL, 0, format,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 2);

  gegl_buffer_iterator_add (iter, mask_buffer,
                            NULL, 0, babl_format ("Y float"),
                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      const gfloat *src   = iter->items[0].data;
      gfloat       *dest  = iter->items[1].data;
      gint          count = iter->length;

      while (count--)
        {
          /*  Find how closely the colors match  */
          *dest = pixel_difference (start_col, src,
                                    antialias,
                                    threshold,
                                    n_components,
                                    has_alpha,
                                    select_transparent,
                                    select_criterion);

          src  += n_components;
          dest += 1;
        }
    }

  return mask_buffer;
}


/*  private functions  */

static const Babl *
choose_format (GeglBuffer          *buffer,
               GimpSelectCriterion  select_criterion,
               gint                *n_components,
               gboolean            *has_alpha)
{
  const Babl *format = gegl_buffer_get_format (buffer);

  *has_alpha = babl_format_has_alpha (format);

  switch (select_criterion)
    {
    case GIMP_SELECT_CRITERION_COMPOSITE:
      if (babl_format_is_palette (format))
        format = babl_format ("R'G'B'A float");
      else
        format = gimp_babl_format (gimp_babl_format_get_base_type (format),
                                   GIMP_PRECISION_FLOAT_GAMMA,
                                   *has_alpha);
      break;

    case GIMP_SELECT_CRITERION_R:
    case GIMP_SELECT_CRITERION_G:
    case GIMP_SELECT_CRITERION_B:
    case GIMP_SELECT_CRITERION_A:
      format = babl_format ("R'G'B'A float");
      break;

    case GIMP_SELECT_CRITERION_H:
    case GIMP_SELECT_CRITERION_S:
    case GIMP_SELECT_CRITERION_V:
      format = babl_format ("HSVA float");
      break;

    case GIMP_SELECT_CRITERION_LCH_L:
      format = babl_format ("CIE L alpha float");
      break;

    case GIMP_SELECT_CRITERION_LCH_C:
    case GIMP_SELECT_CRITERION_LCH_H:
      format = babl_format ("CIE LCH(ab) alpha float");
      break;

    case GIMP_SELECT_CRITERION_LINE_ART:
      format = babl_format ("Y'A float");
      break;

    default:
      g_return_val_if_reached (NULL);
      break;
    }

  *n_components = babl_format_get_n_components (format);

  return format;
}

static gfloat
pixel_difference (const gfloat        *col1,
                  const gfloat        *col2,
                  gboolean             antialias,
                  gfloat               threshold,
                  gint                 n_components,
                  gboolean             has_alpha,
                  gboolean             select_transparent,
                  GimpSelectCriterion  select_criterion)
{
  gfloat max = 0.0;

  /*  if there is an alpha channel, never select transparent regions  */
  if (! select_transparent && has_alpha && col2[n_components - 1] == 0.0)
    return 0.0;

  if (select_transparent && has_alpha)
    {
      max = fabs (col1[n_components - 1] - col2[n_components - 1]);
    }
  else
    {
      gfloat diff;
      gint   b;

      if (has_alpha)
        n_components--;

      switch (select_criterion)
        {
        case GIMP_SELECT_CRITERION_COMPOSITE:
          for (b = 0; b < n_components; b++)
            {
              diff = fabs (col1[b] - col2[b]);
              if (diff > max)
                max = diff;
            }
          break;

        case GIMP_SELECT_CRITERION_R:
          max = fabs (col1[0] - col2[0]);
          break;

        case GIMP_SELECT_CRITERION_G:
          max = fabs (col1[1] - col2[1]);
          break;

        case GIMP_SELECT_CRITERION_B:
          max = fabs (col1[2] - col2[2]);
          break;

        case GIMP_SELECT_CRITERION_A:
          max = fabs (col1[3] - col2[3]);
          break;

        case GIMP_SELECT_CRITERION_H:
          max = fabs (col1[0] - col2[0]);
          max = MIN (max, 1.0 - max);
          break;

        case GIMP_SELECT_CRITERION_S:
          max = fabs (col1[1] - col2[1]);
          break;

        case GIMP_SELECT_CRITERION_V:
          max = fabs (col1[2] - col2[2]);
          break;

        case GIMP_SELECT_CRITERION_LCH_L:
          max = fabs (col1[0] - col2[0]) / 100.0;
          break;

        case GIMP_SELECT_CRITERION_LCH_C:
          max = fabs (col1[1] - col2[1]) / 100.0;
          break;

        case GIMP_SELECT_CRITERION_LCH_H:
          max = fabs (col1[2] - col2[2]) / 360.0;
          max = MIN (max, 1.0 - max);
          break;

        case GIMP_SELECT_CRITERION_LINE_ART:
          /* Smart selection is handled before. */
          g_return_val_if_reached (0.0);
        }
    }

  if (antialias && threshold > 0.0)
    {
      gfloat aa = 1.5 - (max / threshold);

      if (aa <= 0.0)
        return 0.0;
      else if (aa < 0.5)
        return aa * 2.0;
      else
        return 1.0;
    }
  else
    {
      if (max > threshold)
        return 0.0;
      else
        return 1.0;
    }
}

static void
push_segment (GQueue *segment_queue,
              gint    y,
              gint    old_y,
              gint    start,
              gint    end,
              gint    new_y,
              gint    new_start,
              gint    new_end)
{
  /* To avoid excessive memory allocation (y, old_y, start, end) tuples are
   * stored in interleaved format:
   *
   * [y1] [old_y1] [start1] [end1] [y2] [old_y2] [start2] [end2]
   */

  if (new_y != old_y)
    {
      /* If the new segment's y-coordinate is different than the old (source)
       * segment's y-coordinate, push the entire segment.
       */
      g_queue_push_tail (segment_queue, GINT_TO_POINTER (new_y));
      g_queue_push_tail (segment_queue, GINT_TO_POINTER (y));
      g_queue_push_tail (segment_queue, GINT_TO_POINTER (new_start));
      g_queue_push_tail (segment_queue, GINT_TO_POINTER (new_end));
    }
  else
    {
      /* Otherwise, only push the set-difference between the new segment and
       * the source segment (since we've already scanned the source segment.)
       * Note that the `+ 1` and `- 1` terms of the end/start coordinates below
       * are only necessary when `diagonal_neighbors` is on (and otherwise make
       * the segments slightly larger than necessary), but, meh...
       */
      if (new_start < start)
        {
          g_queue_push_tail (segment_queue, GINT_TO_POINTER (new_y));
          g_queue_push_tail (segment_queue, GINT_TO_POINTER (y));
          g_queue_push_tail (segment_queue, GINT_TO_POINTER (new_start));
          g_queue_push_tail (segment_queue, GINT_TO_POINTER (start + 1));
        }

      if (new_end > end)
        {
          g_queue_push_tail (segment_queue, GINT_TO_POINTER (new_y));
          g_queue_push_tail (segment_queue, GINT_TO_POINTER (y));
          g_queue_push_tail (segment_queue, GINT_TO_POINTER (end - 1));
          g_queue_push_tail (segment_queue, GINT_TO_POINTER (new_end));
        }
    }
}

static void
pop_segment (GQueue *segment_queue,
             gint   *y,
             gint   *old_y,
             gint   *start,
             gint   *end)
{
  *y     = GPOINTER_TO_INT (g_queue_pop_head (segment_queue));
  *old_y = GPOINTER_TO_INT (g_queue_pop_head (segment_queue));
  *start = GPOINTER_TO_INT (g_queue_pop_head (segment_queue));
  *end   = GPOINTER_TO_INT (g_queue_pop_head (segment_queue));
}

/* #define FETCH_ROW 1 */

static gboolean
find_contiguous_segment (const gfloat        *col,
                         GeglBuffer          *src_buffer,
                         GeglSampler         *src_sampler,
                         GeglBuffer          *mask_buffer,
                         const Babl          *src_format,
                         const Babl          *mask_format,
                         gint                 n_components,
                         gboolean             has_alpha,
                         gint                 width,
                         gboolean             select_transparent,
                         GimpSelectCriterion  select_criterion,
                         gboolean             antialias,
                         gfloat               threshold,
                         gint                 initial_x,
                         gint                 initial_y,
                         gint                *start,
                         gint                *end,
                         gfloat              *row)
{
  gfloat *s;
  gfloat  mask_row[width];
  gfloat  diff;

#ifdef FETCH_ROW
  gegl_buffer_get (src_buffer, GEGL_RECTANGLE (0, initial_y, width, 1), 1.0,
                   src_format,
                   row, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
  s = row + initial_x * n_components;
#else
  s = g_alloca (n_components * sizeof (gfloat));

  gegl_sampler_get (src_sampler,
                    initial_x, initial_y, NULL, s, GEGL_ABYSS_NONE);
#endif

  diff = pixel_difference (col, s, antialias, threshold,
                           n_components, has_alpha, select_transparent,
                           select_criterion);

  /* check the starting pixel */
  if (! diff)
    return FALSE;

  mask_row[initial_x] = diff;

  *start = initial_x - 1;
#ifdef FETCH_ROW
  s = row + *start * n_components;
#endif

  while (*start >= 0)
    {
#ifndef FETCH_ROW
      gegl_sampler_get (src_sampler,
                        *start, initial_y, NULL, s, GEGL_ABYSS_NONE);
#endif

      diff = pixel_difference (col, s, antialias, threshold,
                               n_components, has_alpha, select_transparent,
                               select_criterion);
      if (diff == 0.0)
        break;

      mask_row[*start] = diff;

      (*start)--;
#ifdef FETCH_ROW
      s -= n_components;
#endif
    }

  *end = initial_x + 1;
#ifdef FETCH_ROW
  s = row + *end * n_components;
#endif

  while (*end < width)
    {
#ifndef FETCH_ROW
      gegl_sampler_get (src_sampler,
                        *end, initial_y, NULL, s, GEGL_ABYSS_NONE);
#endif

      diff = pixel_difference (col, s, antialias, threshold,
                               n_components, has_alpha, select_transparent,
                               select_criterion);
      if (diff == 0.0)
        break;

      mask_row[*end] = diff;

      (*end)++;
#ifdef FETCH_ROW
      s += n_components;
#endif
    }

  gegl_buffer_set (mask_buffer, GEGL_RECTANGLE (*start + 1, initial_y,
                                                *end - *start - 1, 1),
                   0, mask_format, &mask_row[*start + 1],
                   GEGL_AUTO_ROWSTRIDE);

  return TRUE;
}

static void
find_contiguous_region (GeglBuffer          *src_buffer,
                        GeglBuffer          *mask_buffer,
                        const Babl          *format,
                        gint                 n_components,
                        gboolean             has_alpha,
                        gboolean             select_transparent,
                        GimpSelectCriterion  select_criterion,
                        gboolean             antialias,
                        gfloat               threshold,
                        gboolean             diagonal_neighbors,
                        gint                 x,
                        gint                 y,
                        const gfloat        *col)
{
  const Babl  *mask_format = babl_format ("Y float");
  GeglSampler *src_sampler;
  gint         old_y;
  gint         start, end;
  gint         new_start, new_end;
  GQueue      *segment_queue;
  gfloat      *row = NULL;

#ifdef FETCH_ROW
  row = g_new (gfloat, gegl_buffer_get_width (src_buffer) * n_components);
#endif

  src_sampler = gegl_buffer_sampler_new (src_buffer,
                                          format, GEGL_SAMPLER_NEAREST);

  segment_queue = g_queue_new ();

  push_segment (segment_queue,
                y, /* dummy values: */ -1, 0, 0,
                y, x - 1, x + 1);

  do
    {
      pop_segment (segment_queue,
                   &y, &old_y, &start, &end);

      for (x = start + 1; x < end; x++)
        {
          gfloat val;

          gegl_buffer_get (mask_buffer, GEGL_RECTANGLE (x, y, 1, 1), 1.0,
                           mask_format, &val, GEGL_AUTO_ROWSTRIDE,
                           GEGL_ABYSS_NONE);

          if (val != 0.0)
            {
              /* If the current pixel is selected, then we've already visited
               * the next pixel.  (Note that we assume that the maximal image
               * width is sufficiently low that `x` won't overflow.)
               */
              x++;
              continue;
            }

          if (! find_contiguous_segment (col,
                                         src_buffer, src_sampler, mask_buffer,
                                         format, mask_format,
                                         n_components,
                                         has_alpha,
                                         gegl_buffer_get_width (src_buffer),
                                         select_transparent, select_criterion,
                                         antialias, threshold, x, y,
                                         &new_start, &new_end,
                                         row))
            continue;

          /* We can skip directly to `new_end + 1` on the next iteration, since
           * we've just selected all pixels in the range `[x, new_end)`, and
           * the pixel at `new_end` is above threshold.  (Note that we assume
           * that the maximal image width is sufficiently low that `x` won't
           * overflow.)
           */
          x = new_end;

          if (diagonal_neighbors)
            {
              if (new_start >= 0)
                new_start--;

              if (new_end < gegl_buffer_get_width (src_buffer))
                new_end++;
            }

          if (y + 1 < gegl_buffer_get_height (src_buffer))
            {
              push_segment (segment_queue,
                            y, old_y, start, end,
                            y + 1, new_start, new_end);
            }

          if (y - 1 >= 0)
            {
              push_segment (segment_queue,
                            y, old_y, start, end,
                            y - 1, new_start, new_end);
            }

        }
    }
  while (! g_queue_is_empty (segment_queue));

  g_queue_free (segment_queue);

  g_object_unref (src_sampler);

#ifdef FETCH_ROW
  g_free (row);
#endif
}
