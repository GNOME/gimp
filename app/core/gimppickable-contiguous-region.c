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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
static gboolean find_contiguous_segment   (const gfloat        *col,
                                           GeglBuffer          *src_buffer,
                                           GeglBuffer          *mask_buffer,
                                           const Babl          *src_format,
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
                                           gint                 x,
                                           gint                 y,
                                           const gfloat        *col);


/*  public functions  */

GeglBuffer *
gimp_pickable_contiguous_region_by_seed (GimpPickable        *pickable,
                                         gboolean             antialias,
                                         gfloat               threshold,
                                         gboolean             select_transparent,
                                         GimpSelectCriterion  select_criterion,
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

  g_return_val_if_fail (GIMP_IS_PICKABLE (pickable), NULL);

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

  extent = *gegl_buffer_get_extent (src_buffer);

  mask_buffer = gegl_buffer_new (&extent, babl_format ("Y float"));

  if (x >= extent.x && x < (extent.x + extent.width) &&
      y >= extent.y && y < (extent.y + extent.height))
    {
      GIMP_TIMER_START();

      find_contiguous_region (src_buffer, mask_buffer,
                              format, n_components, has_alpha,
                              select_transparent, select_criterion,
                              antialias, threshold,
                              x, y, start_col);

      GIMP_TIMER_END("foo");
    }

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
          /*  don't select transparancy if "color" isn't fully transparent
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
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, mask_buffer,
                            NULL, 0, babl_format ("Y float"),
                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      const gfloat *src   = iter->data[0];
      gfloat       *dest  = iter->data[1];
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
      format = babl_format ("R'G'B'A float");
      break;

    case GIMP_SELECT_CRITERION_H:
    case GIMP_SELECT_CRITERION_S:
    case GIMP_SELECT_CRITERION_V:
      format = babl_format ("HSVA float");
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

        case GIMP_SELECT_CRITERION_H:
          {
            /* wrap around candidates for the actual distance */
            gfloat dist1 = fabs (col1[0] - col2[0]);
            gfloat dist2 = fabs (col1[0] - 1.0 - col2[0]);
            gfloat dist3 = fabs (col1[0] - col2[0] + 1.0);

            max = MIN (dist1, dist2);
            if (max > dist3)
              max = dist3;
          }
          break;

        case GIMP_SELECT_CRITERION_S:
          max = fabs (col1[1] - col2[1]);
          break;

        case GIMP_SELECT_CRITERION_V:
          max = fabs (col1[2] - col2[2]);
          break;
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

/* #define FETCH_ROW 1 */

static gboolean
find_contiguous_segment (const gfloat        *col,
                         GeglBuffer          *src_buffer,
                         GeglBuffer          *mask_buffer,
                         const Babl          *format,
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
                   format,
                   row, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
  s = row + initial_x * n_components;
#else
  s = g_alloca (n_components * sizeof (gfloat));

  gegl_buffer_sample (src_buffer, initial_x, initial_y, NULL, s, format,
                      GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
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

  while (*start >= 0 && diff)
    {
#ifndef FETCH_ROW
      gegl_buffer_sample (src_buffer, *start, initial_y, NULL, s, format,
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
#endif

      diff = pixel_difference (col, s, antialias, threshold,
                               n_components, has_alpha, select_transparent,
                               select_criterion);

      mask_row[*start] = diff;

      if (diff)
        {
          (*start)--;
#ifdef FETCH_ROW
          s -= n_components;
#endif
        }
    }

  diff = 1;
  *end = initial_x + 1;
#ifdef FETCH_ROW
  s = row + *end * n_components;
#endif

  while (*end < width && diff)
    {
#ifndef FETCH_ROW
      gegl_buffer_sample (src_buffer, *end, initial_y, NULL, s, format,
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
#endif

      diff = pixel_difference (col, s, antialias, threshold,
                               n_components, has_alpha, select_transparent,
                               select_criterion);

      mask_row[*end] = diff;

      if (diff)
        {
          (*end)++;
#ifdef FETCH_ROW
          s += n_components;
#endif
        }
    }

  gegl_buffer_set (mask_buffer, GEGL_RECTANGLE (*start, initial_y,
                                                *end - *start, 1),
                   0, babl_format ("Y float"), &mask_row[*start],
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
                        gint                 x,
                        gint                 y,
                        const gfloat        *col)
{
  gint    start, end;
  gint    new_start, new_end;
  GQueue *coord_stack;
  gfloat *row = NULL;

#ifdef FETCH_ROW
  row = g_new (gfloat, gegl_buffer_get_width (src_buffer) * n_components);
#endif

  coord_stack = g_queue_new ();

  /* To avoid excessive memory allocation (y, start, end) tuples are
   * stored in interleaved format:
   *
   * [y1] [start1] [end1] [y2] [start2] [end2]
   */
  g_queue_push_tail (coord_stack, GINT_TO_POINTER (y));
  g_queue_push_tail (coord_stack, GINT_TO_POINTER (x - 1));
  g_queue_push_tail (coord_stack, GINT_TO_POINTER (x + 1));

  do
    {
      y     = GPOINTER_TO_INT (g_queue_pop_head (coord_stack));
      start = GPOINTER_TO_INT (g_queue_pop_head (coord_stack));
      end   = GPOINTER_TO_INT (g_queue_pop_head (coord_stack));

      for (x = start + 1; x < end; x++)
        {
          gfloat val;

          gegl_buffer_sample (mask_buffer, x, y, NULL, &val,
                              babl_format ("Y float"),
                              GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
          if (val != 0.0)
            continue;

          if (! find_contiguous_segment (col, src_buffer, mask_buffer,
                                         format,
                                         n_components,
                                         has_alpha,
                                         gegl_buffer_get_width (src_buffer),
                                         select_transparent, select_criterion,
                                         antialias, threshold, x, y,
                                         &new_start, &new_end,
                                         row))
            continue;

          if (y + 1 < gegl_buffer_get_height (src_buffer))
            {
              g_queue_push_tail (coord_stack, GINT_TO_POINTER (y + 1));
              g_queue_push_tail (coord_stack, GINT_TO_POINTER (new_start));
              g_queue_push_tail (coord_stack, GINT_TO_POINTER (new_end));
            }

          if (y - 1 >= 0)
            {
              g_queue_push_tail (coord_stack, GINT_TO_POINTER (y - 1));
              g_queue_push_tail (coord_stack, GINT_TO_POINTER (new_start));
              g_queue_push_tail (coord_stack, GINT_TO_POINTER (new_end));
            }
        }
    }
  while (! g_queue_is_empty (coord_stack));

  g_queue_free (coord_stack);

#ifdef FETCH_ROW
  g_free (row);
#endif
}
