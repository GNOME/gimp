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

extern "C"
{

#include "core-types.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimp-gegl-utils.h"

#include "gimp-parallel.h"
#include "gimp-utils.h" /* GIMP_TIMER */
#include "gimpasync.h"
#include "gimplineart.h"
#include "gimppickable.h"
#include "gimppickable-contiguous-region.h"


#define EPSILON 1e-6

#define PIXELS_PER_THREAD \
  (/* each thread costs as much as */ 64.0 * 64.0 /* pixels */)


typedef struct
{
  gint   x;
  gint   y;
  gint   level;
} BorderPixel;


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
                                           const GeglRectangle *src_extent,
                                           GeglBuffer          *mask_buffer,
                                           const Babl          *src_format,
                                           const Babl          *mask_format,
                                           gint                 n_components,
                                           gboolean             has_alpha,
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

static void            line_art_queue_pixel (GQueue              *queue,
                                             gint                 x,
                                             gint                 y,
                                             gint                 level);


/*  public functions  */

GeglBuffer *
gimp_pickable_contiguous_region_by_seed (GimpPickable        *pickable,
                                         gboolean             antialias,
                                         gfloat               threshold,
                                         gboolean             select_transparent,
                                         GimpSelectCriterion  select_criterion,
                                         gboolean             diagonal_neighbors,
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
                              antialias, threshold, diagonal_neighbors,
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
                                          GeglColor           *color)
{
  /*  Scan over the pickable's active layer, finding pixels within the
   *  specified threshold from the given R, G, & B values.  If
   *  antialiasing is on, use the same antialiasing scheme as in
   *  fuzzy_select.  Modify the pickable's mask to reflect the
   *  additional selection
   */
  GeglBuffer *src_buffer;
  GeglBuffer *mask_buffer;
  const Babl *format;
  gint        n_components;
  gboolean    has_alpha;
  gfloat      start_col[MAX_CHANNELS];

  g_return_val_if_fail (GIMP_IS_PICKABLE (pickable), NULL);
  g_return_val_if_fail (GEGL_IS_COLOR (color), NULL);

  /* increase the threshold by EPSILON, to allow for conversion errors,
   * especially when threshold == 0 (see issue #1554.)  we need to do this
   * here, but not in the other functions, since the input color gets converted
   * to the format in which we perform the comparison through a different path
   * than the pickable's pixels, which can introduce error.
   */
  threshold += EPSILON;

  gimp_pickable_flush (pickable);

  src_buffer = gimp_pickable_get_buffer (pickable);

  format = choose_format (src_buffer, select_criterion,
                          &n_components, &has_alpha);

  gegl_color_get_pixel (color, format, start_col);

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

  gegl_parallel_distribute_area (
    gegl_buffer_get_extent (src_buffer), PIXELS_PER_THREAD,
    [=] (const GeglRectangle *area)
    {
      GeglBufferIterator *iter;

      iter = gegl_buffer_iterator_new (src_buffer,
                                       area, 0, format,
                                       GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 2);

      gegl_buffer_iterator_add (iter, mask_buffer,
                                area, 0, babl_format ("Y float"),
                                GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

      while (gegl_buffer_iterator_next (iter))
        {
          const gfloat *src   = (const gfloat *) iter->items[0].data;
          gfloat       *dest  = (      gfloat *) iter->items[1].data;
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
    });

  return mask_buffer;
}

GeglBuffer *
gimp_pickable_contiguous_region_by_line_art (GimpPickable  *pickable,
                                             GimpLineArt   *line_art,
                                             GeglBuffer    *fill_buffer,
                                             GeglColor     *fill_color,
                                             gfloat         fill_threshold,
                                             gint           fill_offset_x,
                                             gint           fill_offset_y,
                                             gint           x,
                                             gint           y)
{
  GeglBuffer    *src_buffer;
  GeglBuffer    *mask_buffer;
  const Babl    *format  = babl_format ("Y float");
  gfloat        *distmap = NULL;
  GeglRectangle  extent;
  gboolean       free_line_art   = FALSE;
  gboolean       free_src_buffer = FALSE;
  gboolean       filled          = FALSE;
  guchar         start_col;

  g_return_val_if_fail (GIMP_IS_PICKABLE (pickable) || GIMP_IS_LINE_ART (line_art), NULL);

  if (! line_art)
    {
      /* It is much better experience to pre-compute the line art,
       * but it may not be always possible (for instance when
       * selecting/filling through a PDB call).
       */
      line_art = gimp_line_art_new ();
      gimp_line_art_set_input (line_art, pickable);
      free_line_art = TRUE;
    }

  src_buffer = gimp_line_art_get (line_art, &distmap);
  g_return_val_if_fail (src_buffer && distmap, NULL);

  if (fill_buffer != NULL)
    {
      GeglBufferIterator  *gi;
      const Babl          *fill_format;
      const GeglRectangle *fill_extent;
      gfloat               fill_col[MAX_CHANNELS];
      gboolean             has_alpha;
      gint                 n_components;

      fill_extent = gegl_buffer_get_extent (fill_buffer);
      fill_format = choose_format (fill_buffer,
                                   GIMP_SELECT_CRITERION_COMPOSITE,
                                   &n_components, &has_alpha);
      fill_format = babl_format_with_space (babl_format_get_encoding (fill_format),
                                            gegl_buffer_get_format (fill_buffer));
      gegl_color_get_pixel (fill_color, fill_format, fill_col);

      src_buffer = gimp_gegl_buffer_dup (src_buffer);
      gi = gegl_buffer_iterator_new (src_buffer, NULL, 0, NULL,
                                     GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE, 2);
      gegl_buffer_iterator_add (gi, fill_buffer,
                                GEGL_RECTANGLE (-fill_offset_x, -fill_offset_y,
                                                fill_extent->width + fill_offset_x,
                                                fill_extent->height + fill_offset_y),
                                0, fill_format, GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
      while (gegl_buffer_iterator_next (gi))
        {
          guchar       *data = (guchar*) gi->items[0].data;
          const gfloat *fill = (const gfloat *) gi->items[1].data;
          gint          k;

          for (k = 0; k < gi->length; k++)
            {
              /* Only consider if the line art source hasn't filled yet,
               * and also if this the fill target has full opacity.
               */
              if (! *data &&
                  ( ! has_alpha ||
                   fill[n_components - 1] == 1.0))
                {
                  gfloat diff;

                  diff = pixel_difference (fill, fill_col,
                                           FALSE,
                                           fill_threshold,
                                           n_components, has_alpha, FALSE,
                                           GIMP_SELECT_CRITERION_COMPOSITE);

                  /* Make the additional fill pixel a closure pixel. */
                  if (diff == 1.0)
                    *data = 2;
                }

              data++;
              fill += n_components;
            }
        }

      free_src_buffer = TRUE;
    }

  gegl_buffer_sample (src_buffer, x, y, NULL, &start_col, NULL,
                      GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

  extent = *gegl_buffer_get_extent (src_buffer);

  mask_buffer = gegl_buffer_new (&extent, format);

  if (start_col)
    {
      if (start_col == 1)
        {
          /* As a special exception, if you fill over a line art pixel, only
           * fill the pixel and exit
           */
          gfloat col = 1.0;

          gegl_buffer_set (mask_buffer, GEGL_RECTANGLE (x, y, 1, 1),
                           0, format, &col, GEGL_AUTO_ROWSTRIDE);
        }
      else /* start_col == 2 */
        {
          /* If you fill over a closure pixel, let's fill on all sides
           * of the start point. Otherwise we get a very weird result
           * with only a single pixel filled in the middle of an empty
           * region (since closure pixels are invisible by nature).
           */
          gfloat col = 0.0;

          if (x - 1 >= extent.x && x - 1 < extent.x + extent.width &&
              y - 1 >= extent.y && y - 1 < (extent.y + extent.height))
            find_contiguous_region (src_buffer, mask_buffer,
                                    format, 1, FALSE,
                                    FALSE, GIMP_SELECT_CRITERION_COMPOSITE,
                                    FALSE, 0.0, FALSE,
                                    x - 1, y - 1, &col);

          if (x - 1 >= extent.x && x - 1 < extent.x + extent.width &&
              y >= extent.y && y < (extent.y + extent.height))
            find_contiguous_region (src_buffer, mask_buffer,
                                    format, 1, FALSE,
                                    FALSE, GIMP_SELECT_CRITERION_COMPOSITE,
                                    FALSE, 0.0, FALSE,
                                    x - 1, y, &col);

          if (x - 1 >= extent.x && x - 1 < extent.x + extent.width &&
              y + 1 >= extent.y && y + 1 < (extent.y + extent.height))
            find_contiguous_region (src_buffer, mask_buffer,
                                    format, 1, FALSE,
                                    FALSE, GIMP_SELECT_CRITERION_COMPOSITE,
                                    FALSE, 0.0, FALSE,
                                    x - 1, y + 1, &col);

          if (x >= extent.x && x < extent.x + extent.width &&
              y - 1 >= extent.y && y - 1 < (extent.y + extent.height))
            find_contiguous_region (src_buffer, mask_buffer,
                                    format, 1, FALSE,
                                    FALSE, GIMP_SELECT_CRITERION_COMPOSITE,
                                    FALSE, 0.0, FALSE,
                                    x, y - 1, &col);

          if (x >= extent.x && x < extent.x + extent.width &&
              y + 1 >= extent.y && y + 1 < (extent.y + extent.height))
            find_contiguous_region (src_buffer, mask_buffer,
                                    format, 1, FALSE,
                                    FALSE, GIMP_SELECT_CRITERION_COMPOSITE,
                                    FALSE, 0.0, FALSE,
                                    x, y + 1, &col);

          if (x + 1 >= extent.x && x + 1 < extent.x + extent.width &&
              y - 1 >= extent.y && y - 1 < (extent.y + extent.height))
            find_contiguous_region (src_buffer, mask_buffer,
                                    format, 1, FALSE,
                                    FALSE, GIMP_SELECT_CRITERION_COMPOSITE,
                                    FALSE, 0.0, FALSE,
                                    x + 1, y - 1, &col);

          if (x + 1 >= extent.x && x + 1 < extent.x + extent.width &&
              y >= extent.y && y < (extent.y + extent.height))
            find_contiguous_region (src_buffer, mask_buffer,
                                    format, 1, FALSE,
                                    FALSE, GIMP_SELECT_CRITERION_COMPOSITE,
                                    FALSE, 0.0, FALSE,
                                    x + 1, y, &col);

          if (x + 1 >= extent.x && x + 1 < extent.x + extent.width &&
              y + 1 >= extent.y && y + 1 < (extent.y + extent.height))
            find_contiguous_region (src_buffer, mask_buffer,
                                    format, 1, FALSE,
                                    FALSE, GIMP_SELECT_CRITERION_COMPOSITE,
                                    FALSE, 0.0, FALSE,
                                    x + 1, y + 1, &col);

          filled = TRUE;
        }
    }
  else if (x >= extent.x && x < (extent.x + extent.width) &&
           y >= extent.y && y < (extent.y + extent.height))
    {
      gfloat col = 0.0;

      find_contiguous_region (src_buffer, mask_buffer,
                              format, 1, FALSE,
                              FALSE, GIMP_SELECT_CRITERION_COMPOSITE,
                              FALSE, 0.0, FALSE,
                              x, y, &col);
      filled = TRUE;
    }

  if (filled)
    {
      GQueue *queue  = g_queue_new ();
      gfloat *mask;
      gint    width  = gegl_buffer_get_width (src_buffer);
      gint    height = gegl_buffer_get_height (src_buffer);
      gint    line_art_max_grow;
      gint    nx, ny;

      GIMP_TIMER_START();
      /* The last step of the line art algorithm is to make sure that
       * selections does not leave "holes" between its borders and the
       * line arts, while not stepping over as well.
       * I used to run the "gegl:watershed-transform" operation to flood
       * the stroke pixels, but for such simple need, this simple code
       * is so much faster while producing better results.
       */
      mask = g_new (gfloat, width * height);
      gegl_buffer_get (mask_buffer, NULL, 1.0, NULL,
                       mask, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
          {
            if (distmap[x + y * width] == 1.0)
              {
                if (x > 0)
                  {
                    nx = x - 1;
                    if (y > 0)
                      {
                        ny = y - 1;
                        if (mask[nx + ny * width] != 0.0)
                          {
                            line_art_queue_pixel (queue, x, y, 1);
                            continue;
                          }
                      }
                    ny = y;
                    if (mask[nx + ny * width] != 0.0)
                      {
                        line_art_queue_pixel (queue, x, y, 1);
                        continue;
                      }
                    if (y < height - 1)
                      {
                        ny = y + 1;
                        if (mask[nx + ny * width] != 0.0)
                          {
                            line_art_queue_pixel (queue, x, y, 1);
                            continue;
                          }
                      }
                  }
                if (x < width - 1)
                  {
                    nx = x + 1;
                    if (y > 0)
                      {
                        ny = y - 1;
                        if (mask[nx + ny * width] != 0.0)
                          {
                            line_art_queue_pixel (queue, x, y, 1);
                            continue;
                          }
                      }
                    ny = y;
                    if (mask[nx + ny * width] != 0.0)
                      {
                        line_art_queue_pixel (queue, x, y, 1);
                        continue;
                      }
                    if (y < height - 1)
                      {
                        ny = y + 1;
                        if (mask[nx + ny * width] != 0.0)
                          {
                            line_art_queue_pixel (queue, x, y, 1);
                            continue;
                          }
                      }
                  }
                nx = x;
                if (y > 0)
                  {
                    ny = y - 1;
                    if (mask[nx + ny * width] != 0.0)
                      {
                        line_art_queue_pixel (queue, x, y, 1);
                        continue;
                      }
                  }
                if (y < height - 1)
                  {
                    ny = y + 1;
                    if (mask[nx + ny * width] != 0.0)
                      {
                        line_art_queue_pixel (queue, x, y, 1);
                        continue;
                      }
                  }
              }
          }

      g_object_get (line_art,
                    "max-grow", &line_art_max_grow,
                    NULL);
      while (! g_queue_is_empty (queue))
        {
          BorderPixel *c = (BorderPixel *) g_queue_pop_head (queue);

          if (mask[c->x + c->y * width] != 1.0)
            {
              mask[c->x + c->y * width] = 1.0;
              if (c->level >= line_art_max_grow)
                {
                  /* Do not overflood under line arts. */
                  g_free (c);
                  continue;
                }
              if (c->x > 0)
                {
                  nx = c->x - 1;
                  if (c->y > 0)
                    {
                      ny = c->y - 1;
                      if (mask[nx + ny * width] == 0.0 &&
                          distmap[nx + ny * width] > distmap[c->x + c->y * width])
                        line_art_queue_pixel (queue, nx, ny, c->level + 1);
                    }
                  ny = c->y;
                  if (mask[nx + ny * width] == 0.0 &&
                      distmap[nx + ny * width] > distmap[c->x + c->y * width])
                    line_art_queue_pixel (queue, nx, ny, c->level + 1);
                  if (c->y < height - 1)
                    {
                      ny = c->y + 1;
                      if (mask[nx + ny * width] == 0.0 &&
                          distmap[nx + ny * width] > distmap[c->x + c->y * width])
                        line_art_queue_pixel (queue, nx, ny, c->level + 1);
                    }
                }
              if (c->x < width - 1)
                {
                  nx = c->x + 1;
                  if (c->y > 0)
                    {
                      ny = c->y - 1;
                      if (mask[nx + ny * width] == 0.0 &&
                          distmap[nx + ny * width] > distmap[c->x + c->y * width])
                        line_art_queue_pixel (queue, nx, ny, c->level + 1);
                    }
                  ny = c->y;
                  if (mask[nx + ny * width] == 0.0 &&
                      distmap[nx + ny * width] > distmap[c->x + c->y * width])
                    line_art_queue_pixel (queue, nx, ny, c->level + 1);
                  if (c->y < height - 1)
                    {
                      ny = c->y + 1;
                      if (mask[nx + ny * width] == 0.0 &&
                          distmap[nx + ny * width] > distmap[c->x + c->y * width])
                        line_art_queue_pixel (queue, nx, ny, c->level + 1);
                    }
                }
              nx = c->x;
              if (c->y > 0)
                {
                  ny = c->y - 1;
                  if (mask[nx + ny * width] == 0.0 &&
                      distmap[nx + ny * width] > distmap[c->x + c->y * width])
                    line_art_queue_pixel (queue, nx, ny, c->level + 1);
                }
              if (c->y < height - 1)
                {
                  ny = c->y + 1;
                  if (mask[nx + ny * width] == 0.0 &&
                      distmap[nx + ny * width] > distmap[c->x + c->y * width])
                    line_art_queue_pixel (queue, nx, ny, c->level + 1);
                }
            }
          g_free (c);
        }
      g_queue_free (queue);
      gegl_buffer_set (mask_buffer, gegl_buffer_get_extent (mask_buffer),
                       0, NULL, mask, GEGL_AUTO_ROWSTRIDE);
      g_free (mask);

      GIMP_TIMER_END("watershed line art");
    }
  if (free_line_art)
    g_clear_object (&line_art);
  if (free_src_buffer)
    g_object_unref (src_buffer);

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
                                   GIMP_PRECISION_FLOAT_NON_LINEAR,
                                   *has_alpha,
                                   NULL);
      break;

    case GIMP_SELECT_CRITERION_RGB_RED:
    case GIMP_SELECT_CRITERION_RGB_GREEN:
    case GIMP_SELECT_CRITERION_RGB_BLUE:
    case GIMP_SELECT_CRITERION_ALPHA:
      format = babl_format ("R'G'B'A float");
      break;

    case GIMP_SELECT_CRITERION_HSV_HUE:
    case GIMP_SELECT_CRITERION_HSV_SATURATION:
    case GIMP_SELECT_CRITERION_HSV_VALUE:
      format = babl_format ("HSVA float");
      break;

    case GIMP_SELECT_CRITERION_LCH_LIGHTNESS:
      format = babl_format ("CIE L alpha float");
      break;

    case GIMP_SELECT_CRITERION_LCH_CHROMA:
    case GIMP_SELECT_CRITERION_LCH_HUE:
      format = babl_format ("CIE LCH(ab) alpha float");
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

        case GIMP_SELECT_CRITERION_RGB_RED:
          max = fabs (col1[0] - col2[0]);
          break;

        case GIMP_SELECT_CRITERION_RGB_GREEN:
          max = fabs (col1[1] - col2[1]);
          break;

        case GIMP_SELECT_CRITERION_RGB_BLUE:
          max = fabs (col1[2] - col2[2]);
          break;

        case GIMP_SELECT_CRITERION_ALPHA:
          max = fabs (col1[3] - col2[3]);
          break;

        case GIMP_SELECT_CRITERION_HSV_HUE:
          if (col1[1] > EPSILON)
            {
              if (col2[1] > EPSILON)
                {
                  max = fabs (col1[0] - col2[0]);
                  max = MIN (max, 1.0 - max);
                }
              else
                {
                  /* "infinite" difference.  anything >> 1 will do. */
                  max = 10.0;
                }
            }
          else
            {
              if (col2[1] > EPSILON)
                {
                  /* "infinite" difference.  anything >> 1 will do. */
                  max = 10.0;
                }
              else
                {
                  max = 0.0;
                }
            }
          break;

        case GIMP_SELECT_CRITERION_HSV_SATURATION:
          max = fabs (col1[1] - col2[1]);
          break;

        case GIMP_SELECT_CRITERION_HSV_VALUE:
          max = fabs (col1[2] - col2[2]);
          break;

        case GIMP_SELECT_CRITERION_LCH_LIGHTNESS:
          max = fabs (col1[0] - col2[0]) / 100.0;
          break;

        case GIMP_SELECT_CRITERION_LCH_CHROMA:
          max = fabs (col1[1] - col2[1]) / 100.0;
          break;

        case GIMP_SELECT_CRITERION_LCH_HUE:
          if (col1[1] > 100.0 * EPSILON)
            {
              if (col2[1] > 100.0 * EPSILON)
                {
                  max = fabs (col1[2] - col2[2]) / 360.0;
                  max = MIN (max, 1.0 - max);
                }
              else
                {
                  /* "infinite" difference.  anything >> 1 will do. */
                  max = 10.0;
                }
            }
          else
            {
              if (col2[1] > 100.0 * EPSILON)
                {
                  /* "infinite" difference.  anything >> 1 will do. */
                  max = 10.0;
                }
              else
                {
                  max = 0.0;
                }
            }
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
                         const GeglRectangle *src_extent,
                         GeglBuffer          *mask_buffer,
                         const Babl          *src_format,
                         const Babl          *mask_format,
                         gint                 n_components,
                         gboolean             has_alpha,
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
  gfloat  mask_row_buf[src_extent->width];
  gfloat *mask_row = mask_row_buf - src_extent->x;
  gfloat  diff;

#ifdef FETCH_ROW
  gegl_buffer_get (src_buffer, GEGL_RECTANGLE (0, initial_y, width, 1), 1.0,
                   src_format,
                   row, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
  s = row + initial_x * n_components;
#else
  s = (gfloat *) g_alloca (n_components * sizeof (gfloat));

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

  while (*start >= src_extent->x)
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

  while (*end < src_extent->x + src_extent->width)
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
  const Babl          *mask_format = babl_format ("Y float");
  GeglSampler         *src_sampler;
  const GeglRectangle *src_extent;
  gint                 old_y;
  gint                 start, end;
  gint                 new_start, new_end;
  GQueue              *segment_queue;
  gfloat              *row = NULL;

  src_extent = gegl_buffer_get_extent (src_buffer);

#ifdef FETCH_ROW
  row = g_new (gfloat, src_extent->width * n_components);
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
                                         src_buffer, src_sampler, src_extent,
                                         mask_buffer,
                                         format, mask_format,
                                         n_components,
                                         has_alpha,
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
              if (new_start >= src_extent->x)
                new_start--;

              if (new_end < src_extent->x + src_extent->width)
                new_end++;
            }

          if (y + 1 < src_extent->y + src_extent->height)
            {
              push_segment (segment_queue,
                            y, old_y, start, end,
                            y + 1, new_start, new_end);
            }

          if (y - 1 >= src_extent->y)
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

static void
line_art_queue_pixel (GQueue *queue,
                      gint    x,
                      gint    y,
                      gint    level)
{
  BorderPixel *p = g_new (BorderPixel, 1);

  p->x      = x;
  p->y      = y;
  p->level  = level;

  g_queue_push_head (queue, p);
}

} /* extern "C" */
