/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2017 Sébastien Fourey & David Tchumperlé
 * Copyright (C) 2018 Jehan
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

#define GEGL_ITERATOR2_API
#include <gegl.h>

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gimplineart.h"

static int DeltaX[4] = {+1, -1, 0, 0};
static int DeltaY[4] = {0, 0, +1, -1};

static const GimpVector2 Direction2Normal[4] =
{
    {  1.0f,  0.0f },
    { -1.0f,  0.0f },
    {  0.0f,  1.0f },
    {  0.0f, -1.0f }
};

typedef enum _Direction
{
  XPlusDirection  = 0,
  XMinusDirection = 1,
  YPlusDirection  = 2,
  YMinusDirection = 3
} Direction;

typedef GimpVector2 Pixel;

typedef struct _SplineCandidate
{
  Pixel p1;
  Pixel p2;
  float quality;
} SplineCandidate;

typedef struct _Edgel
{
  gint      x, y;
  Direction direction;

  gfloat    x_normal;
  gfloat    y_normal;
  gfloat    curvature;
  guint     next, previous;
} Edgel;

static void         gimp_lineart_denoise                    (GeglBuffer             *buffer,
                                                             int                     size);
static void         gimp_lineart_compute_normals_curvatures (GeglBuffer             *mask,
                                                             gfloat                 *normals,
                                                             gfloat                 *curvatures,
                                                             gfloat                 *smoothed_curvatures,
                                                             int                     normal_estimate_mask_size);
static gfloat *     gimp_lineart_get_smooth_curvatures      (GArray                 *edgelset);
static GArray     * gimp_lineart_curvature_extremums        (gfloat                 *curvatures,
                                                             gfloat                 *smoothed_curvatures,
                                                             gint                    curvatures_width,
                                                             gint                    curvatures_height);
static gint         gimp_spline_candidate_cmp               (const SplineCandidate  *a,
                                                             const SplineCandidate  *b,
                                                             gpointer                user_data);
static GList      * gimp_lineart_find_spline_candidates     (GArray                 *max_positions,
                                                             gfloat                 *normals,
                                                             gint                    width,
                                                             gint                    distance_threshold,
                                                             gfloat                  max_angle_deg);

static GArray     * gimp_lineart_discrete_spline            (Pixel                   p0,
                                                             GimpVector2             n0,
                                                             Pixel                   p1,
                                                             GimpVector2             n1);

static gint         gimp_number_of_transitions               (GArray                 *pixels,
                                                              GeglBuffer             *buffer,
                                                              gboolean                border_value);
static gboolean     gimp_lineart_curve_creates_region        (GeglBuffer             *mask,
                                                              GArray                 *pixels,
                                                              int                     lower_size_limit,
                                                              int                     upper_size_limit);
static GArray     * gimp_lineart_line_segment_until_hit      (const GeglBuffer       *buffer,
                                                              Pixel                   start,
                                                              GimpVector2             direction,
                                                              int                     size);
static gfloat     * gimp_lineart_estimate_strokes_radii      (GeglBuffer             *mask);

/* Some callback-type functions. */

static guint        visited_hash_fun                         (Pixel                  *key);
static gboolean     visited_equal_fun                        (Pixel                  *e1,
                                                              Pixel                  *e2);

static inline gboolean    border_in_direction (GeglBuffer *mask,
                                               Pixel       p,
                                               int         direction);
static inline GimpVector2 pair2normal         (Pixel       p,
                                               gfloat     *normals,
                                               gint        width);

/* Edgel */

static Edgel    * gimp_edgel_new                  (int                x,
                                                   int                y,
                                                   Direction          direction);
static void       gimp_edgel_init                 (Edgel             *edgel);
static void       gimp_edgel_clear                (Edgel            **edgel);
static int        gimp_edgel_cmp                  (const Edgel       *e1,
                                                   const Edgel       *e2);
static guint      edgel2index_hash_fun            (Edgel              *key);
static gboolean   edgel2index_equal_fun           (Edgel              *e1,
                                                   Edgel              *e2);

static glong      gimp_edgel_track_mark           (GeglBuffer         *mask,
                                                   Edgel               edgel,
                                                   long                size_limit);
static glong      gimp_edgel_region_area          (const GeglBuffer   *mask,
                                                   Edgel               starting_edgel);

/* Edgel set */

static GArray   * gimp_edgelset_new               (GeglBuffer          *buffer);
static void       gimp_edgelset_add               (GArray             *set,
                                                   int                 x,
                                                   int                 y,
                                                   Direction           direction,
                                                   GHashTable         *edgel2index);
static void       gimp_edgelset_init_normals      (GArray             *set);
static void       gimp_edgelset_smooth_normals    (GArray             *set,
                                                   int                 mask_size);
static void       gimp_edgelset_compute_curvature (GArray             *set);

static void       gimp_edgelset_build_graph       (GArray            *set,
                                                   GeglBuffer        *buffer,
                                                   GHashTable        *edgel2index);
static void       gimp_edgelset_next8             (const GeglBuffer  *buffer,
                                                   Edgel             *it,
                                                   Edgel             *n);

/* Public functions */

/**
 * gimp_lineart_close:
 * @buffer: the input #GeglBuffer.
 * @select_transparent: whether we binarize the alpha channel or the
 *                      luminosity.
 * @stroke_threshold: [0-1] threshold value for detecting stroke pixels
 *                    (higher values will detect more stroke pixels).
 * @minimal_lineart_area: the minimum size in number pixels for area to
 *                        be considered as line art.
 * @normal_estimate_mask_size:
 * @end_point_rate: threshold to estimate if a curvature is an end-point
 *                  in [0-1] range value.
 * @spline_max_length: the maximum length for creating splines between
 *                     end points.
 * @spline_max_angle: the maximum angle between end point normals for
 *                    creating splines between them.
 * @end_point_connectivity:
 * @spline_roundness:
 * @allow_self_intersections: whether to allow created splines and
 *                            segments to intersect.
 * @created_regions_significant_area:
 * @created_regions_minimum_area:
 * @small_segments_from_spline_sources:
 * @segments_max_length: the maximum length for creating segments
 *                       between end points. Unlike splines, segments
 *                       are straight lines.
 * @closed_distmap: a distance map of the closed line art pixels.
 * @lineart_radii: a map of estimated radii of line art border pixels.
 *
 * Creates a binarized version of the strokes of @buffer, detected either
 * with luminosity (light means background) or alpha values depending on
 * @select_transparent. This binary version of the strokes will have closed
 * regions allowing adequate selection of "nearly closed regions".
 * This algorithm is meant for digital painting (and in particular on the
 * sketch-only step), and therefore will likely produce unexpected results on
 * other types of input.
 *
 * The algorithm is the first step from the research paper "A Fast and
 * Efficient Semi-guided Algorithm for Flat Coloring Line-arts", by Sébastian
 * Fourey, David Tschumperlé, David Revoy.
 *
 * Returns: a new #GeglBuffer of format "Y u8" representing the
 *          binarized @line_art. If @lineart_radii and @lineart_distmap
 *          are not #NULL, newly allocated float buffer are returned,
 *          which can be used for overflowing created masks later.
 */
GeglBuffer *
gimp_lineart_close (GeglBuffer  *buffer,
                    gboolean     select_transparent,
                    gfloat       stroke_threshold,
                    gint         minimal_lineart_area,
                    gint         normal_estimate_mask_size,
                    gfloat       end_point_rate,
                    gint         spline_max_length,
                    gfloat       spline_max_angle,
                    gint         end_point_connectivity,
                    gfloat       spline_roundness,
                    gboolean     allow_self_intersections,
                    gint         created_regions_significant_area,
                    gint         created_regions_minimum_area,
                    gboolean     small_segments_from_spline_sources,
                    gint         segments_max_length,
                    gfloat     **closed_distmap,
                    gfloat     **lineart_radii)
{
  const Babl         *gray_format;
  gfloat             *normals;
  gfloat             *curvatures;
  gfloat             *smoothed_curvatures;
  gfloat             *radii;
  GeglBufferIterator *gi;
  GeglBuffer         *closed;
  GeglBuffer         *strokes;
  GHashTable         *visited;
  GArray             *keypoints;
  Pixel              *point;
  GList              *candidates;
  SplineCandidate    *candidate;
  guchar              max_value = 0;
  gfloat              threshold;
  gfloat              clamped_threshold;
  gint                width  = gegl_buffer_get_width (buffer);
  gint                height = gegl_buffer_get_height (buffer);
  gint                i;

  normals             = g_new0 (gfloat, width * height * 2);
  curvatures          = g_new0 (gfloat, width * height);
  smoothed_curvatures = g_new0 (gfloat, width * height);

  if (select_transparent)
    /* Keep alpha channel as gray levels */
    gray_format = babl_format ("A u8");
  else
    /* Keep luminance */
    gray_format = babl_format ("Y' u8");

  /* Transform the line art from any format to gray. */
  strokes = gegl_buffer_new (gegl_buffer_get_extent (buffer),
                             gray_format);
  gegl_buffer_copy (buffer, NULL, GEGL_ABYSS_NONE, strokes, NULL);
  gegl_buffer_set_format (strokes, babl_format ("Y' u8"));

  if (! select_transparent)
    {
      /* Compute the biggest value */
      gi = gegl_buffer_iterator_new (strokes, NULL, 0, NULL,
                                     GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 1);
      while (gegl_buffer_iterator_next (gi))
        {
          guchar *data = (guchar*) gi->items[0].data;
          gint    k;

          for (k = 0; k < gi->length; k++)
            {
              if (*data > max_value)
                max_value = *data;
              data++;
            }
        }
    }

  /* Make the image binary: 1 is stroke, 0 background */
  gi = gegl_buffer_iterator_new (strokes, NULL, 0, NULL,
                                 GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE, 1);
  while (gegl_buffer_iterator_next (gi))
    {
      guchar *data = (guchar*) gi->items[0].data;
      gint    k;

      for (k = 0; k < gi->length; k++)
        {
          if (! select_transparent)
            /* Negate the value. */
            *data = max_value - *data;
          /* Apply a threshold. */
          if (*data > (guchar) (255.0f * (1.0f - stroke_threshold)))
            *data = 1;
          else
            *data = 0;
          data++;
        }
    }

  /* Denoise (remove small connected components) */
  gimp_lineart_denoise (strokes, minimal_lineart_area);

  /* Estimate normals & curvature */
  gimp_lineart_compute_normals_curvatures (strokes, normals, curvatures,
                                           smoothed_curvatures,
                                           normal_estimate_mask_size);

  radii = gimp_lineart_estimate_strokes_radii (strokes);
  threshold = 1.0f - end_point_rate;
  clamped_threshold = MAX (0.25f, threshold);
  for (i = 0; i < width; i++)
    {
      gint j;
      for (j = 0; j < height; j++)
        {
          if (smoothed_curvatures[i + j * width] >= (threshold / MAX (1.0f, radii[i + j * width])) ||
              curvatures[i + j * width] >= clamped_threshold)
            curvatures[i + j * width] = 1.0;
          else
            curvatures[i + j * width] = 0.0;
        }
    }
  if (lineart_radii)
    *lineart_radii = radii;
  else
    g_free (radii);

  keypoints = gimp_lineart_curvature_extremums (curvatures, smoothed_curvatures,
                                                width, height);
  candidates = gimp_lineart_find_spline_candidates (keypoints, normals, width,
                                                    spline_max_length,
                                                    spline_max_angle);
  closed = gegl_buffer_dup (strokes);

  /* Draw splines */
  visited = g_hash_table_new_full ((GHashFunc) visited_hash_fun,
                                   (GEqualFunc) visited_equal_fun,
                                   (GDestroyNotify) g_free, NULL);
  while (candidates)
    {
      Pixel    *p1 = g_new (Pixel, 1);
      Pixel    *p2 = g_new (Pixel, 1);
      gboolean  inserted = FALSE;

      candidate = (SplineCandidate *) candidates->data;
      p1->x = candidate->p1.x;
      p1->y = candidate->p1.y;
      p2->x = candidate->p2.x;
      p2->y = candidate->p2.y;

      g_free (candidate);
      candidates = g_list_delete_link (candidates, candidates);

      if ((! g_hash_table_contains (visited, p1) ||
           GPOINTER_TO_INT (g_hash_table_lookup (visited, p1)) < end_point_connectivity) &&
          (! g_hash_table_contains (visited, p2) ||
           GPOINTER_TO_INT (g_hash_table_lookup (visited, p2)) < end_point_connectivity))
        {
          GArray      *discrete_curve;
          GimpVector2  vect1 = pair2normal (*p1, normals, width);
          GimpVector2  vect2 = pair2normal (*p2, normals, width);
          gfloat       distance = gimp_vector2_length_val (gimp_vector2_sub_val (*p1, *p2));
          gint         transitions;

          gimp_vector2_mul (&vect1, distance);
          gimp_vector2_mul (&vect1, spline_roundness);
          gimp_vector2_mul (&vect2, distance);
          gimp_vector2_mul (&vect2, spline_roundness);

          discrete_curve = gimp_lineart_discrete_spline (*p1, vect1, *p2, vect2);

          transitions = allow_self_intersections ?
                          gimp_number_of_transitions (discrete_curve, strokes, FALSE) :
                          gimp_number_of_transitions (discrete_curve, closed, FALSE);

          if (transitions == 2 &&
              ! gimp_lineart_curve_creates_region (closed, discrete_curve,
                                                   created_regions_significant_area,
                                                   created_regions_minimum_area - 1))
            {
              for (i = 0; i < discrete_curve->len; i++)
                {
                  Pixel p = g_array_index (discrete_curve, Pixel, i);

                  if (p.x >= 0 && p.x < gegl_buffer_get_width (closed) &&
                      p.y >= 0 && p.y < gegl_buffer_get_height (closed))
                    {
                      guchar val = 1;

                      gegl_buffer_set (closed, GEGL_RECTANGLE ((gint) p.x, (gint) p.y, 1, 1), 0,
                                       NULL, &val, GEGL_AUTO_ROWSTRIDE);
                    }
                }
              g_hash_table_replace (visited, p1,
                                    GINT_TO_POINTER (GPOINTER_TO_INT (g_hash_table_lookup (visited, p1)) + 1));
              g_hash_table_replace (visited, p2,
                                    GINT_TO_POINTER (GPOINTER_TO_INT (g_hash_table_lookup (visited, p2)) + 1));
              inserted = TRUE;
            }
          g_array_free (discrete_curve, TRUE);
        }
      if (! inserted)
        {
          g_free (p1);
          g_free (p2);
        }
    }

  /* Draw straight line segments */
  point = (Pixel *) keypoints->data;
  for (i = 0; i < keypoints->len; i++)
    {
      Pixel    *p = g_new (Pixel, 1);
      gboolean  inserted = FALSE;

      *p = *point;

      if (! g_hash_table_contains (visited, p) ||
          (small_segments_from_spline_sources &&
           GPOINTER_TO_INT (g_hash_table_lookup (visited, p)) < end_point_connectivity))
        {
          GArray *segment = gimp_lineart_line_segment_until_hit (closed, *point,
                                                                 pair2normal (*point, normals, width),
                                                                 segments_max_length);

          if (segment->len &&
              ! gimp_lineart_curve_creates_region (closed, segment,
                                                   created_regions_significant_area,
                                                   created_regions_minimum_area - 1))
            {
              gint j;

              for (j = 0; j < segment->len; j++)
                {
                  Pixel  p2 = g_array_index (segment, Pixel, j);
                  guchar val = 1;

                  gegl_buffer_set (closed, GEGL_RECTANGLE ((gint) p2.x, (gint) p2.y, 1, 1), 0,
                                   NULL, &val, GEGL_AUTO_ROWSTRIDE);
                }
              g_hash_table_replace (visited, p,
                                    GINT_TO_POINTER (GPOINTER_TO_INT (g_hash_table_lookup (visited, p)) + 1));
              inserted = TRUE;
            }
          g_array_free (segment, TRUE);
        }
      if (! inserted)
        g_free (p);
      point++;
    }

  if (closed_distmap)
    {
      GeglNode   *graph;
      GeglNode   *input;
      GeglNode   *op;

      /* Flooding needs a distance map for closed line art. */
      *closed_distmap = g_new (gfloat, width * height);

      graph = gegl_node_new ();
      input = gegl_node_new_child (graph,
                                   "operation", "gegl:buffer-source",
                                   "buffer", closed,
                                   NULL);
      op  = gegl_node_new_child (graph,
                                 "operation", "gegl:distance-transform",
                                 "metric",    GEGL_DISTANCE_METRIC_EUCLIDEAN,
                                 "normalize", FALSE,
                                 NULL);
      gegl_node_connect_to (input, "output",
                            op, "input");
      gegl_node_blit (op, 1.0, gegl_buffer_get_extent (closed),
                      NULL, *closed_distmap,
                      GEGL_AUTO_ROWSTRIDE, GEGL_BLIT_DEFAULT);
      g_object_unref (graph);
    }

  g_hash_table_destroy (visited);
  g_array_free (keypoints, TRUE);
  g_object_unref (strokes);
  g_free (normals);
  g_free (curvatures);
  g_free (smoothed_curvatures);
  g_list_free_full (candidates, g_free);

  return closed;
}

/* Private functions */

static void
gimp_lineart_denoise (GeglBuffer *buffer,
                      int         minimum_area)
{
  /* Keep connected regions with significant area. */
  GArray   *region;
  GQueue   *q       = g_queue_new ();
  gint      width   = gegl_buffer_get_width (buffer);
  gint      height  = gegl_buffer_get_height (buffer);
  gboolean *visited = g_new0 (gboolean, width * height);
  gint      x, y;

  region = g_array_sized_new (TRUE, TRUE, sizeof (Pixel *), minimum_area);

  for (y = 0; y < height; ++y)
    for (x = 0; x < width; ++x)
      {
        guchar has_stroke;

        gegl_buffer_sample (buffer, x, y, NULL, &has_stroke, NULL,
                            GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
        if (has_stroke && ! visited[x + y * width])
          {
            Pixel *p = g_new (Pixel, 1);
            gint   regionSize = 0;

            p->x = x;
            p->y = y;

            g_queue_push_tail (q, p);
            visited[x + y * width] = TRUE;

            while (! g_queue_is_empty (q))
              {
                Pixel *p = (Pixel *) g_queue_pop_head (q);
                gint   p2x;
                gint   p2y;

                p2x = p->x + 1;
                p2y = p->y;
                if (p2x >= 0 && p2x < width && p2y >= 0 && p2y < height)
                  {
                    gegl_buffer_sample (buffer, p2x, p2y, NULL, &has_stroke, NULL,
                                        GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
                    if (has_stroke && ! visited[p2x + p2y * width])
                      {
                        Pixel *p2 = g_new (Pixel, 1);

                        p2->x = p2x;
                        p2->y = p2y;
                        g_queue_push_tail (q, p2);
                        visited[p2x +p2y * width] = TRUE;
                      }
                  }
                p2x = p->x - 1;
                p2y = p->y;
                if (p2x >= 0 && p2x < width && p2y >= 0 && p2y < height)
                  {
                    gegl_buffer_sample (buffer, p2x, p2y, NULL, &has_stroke, NULL,
                                        GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
                    if (has_stroke && ! visited[p2x + p2y * width])
                      {
                        Pixel *p2 = g_new (Pixel, 1);

                        p2->x = p2x;
                        p2->y = p2y;
                        g_queue_push_tail (q, p2);
                        visited[p2x + p2y * width] = TRUE;
                      }
                  }
                p2x = p->x;
                p2y = p->y - 1;
                if (p2x >= 0 && p2x < width && p2y >= 0 && p2y < height)
                  {
                    gegl_buffer_sample (buffer, p2x, p2y, NULL, &has_stroke, NULL,
                                        GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
                    if (has_stroke && ! visited[p2x + p2y * width])
                      {
                        Pixel *p2 = g_new (Pixel, 1);

                        p2->x = p2x;
                        p2->y = p2y;
                        g_queue_push_tail (q, p2);
                        visited[p2x + p2y * width] = TRUE;
                      }
                  }
                p2x = p->x;
                p2y = p->y + 1;
                if (p2x >= 0 && p2x < width && p2y >= 0 && p2y < height)
                  {
                    gegl_buffer_sample (buffer, p2x, p2y, NULL, &has_stroke, NULL,
                                        GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
                    if (has_stroke && ! visited[p2x + p2y * width])
                      {
                        Pixel *p2 = g_new (Pixel, 1);

                        p2->x = p2x;
                        p2->y = p2y;
                        g_queue_push_tail (q, p2);
                        visited[p2x + p2y * width] = TRUE;
                      }
                  }
                p2x = p->x + 1;
                p2y = p->y + 1;
                if (p2x >= 0 && p2x < width && p2y >= 0 && p2y < height)
                  {
                    gegl_buffer_sample (buffer, p2x, p2y, NULL, &has_stroke, NULL,
                                        GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
                    if (has_stroke && ! visited[p2x + p2y * width])
                      {
                        Pixel *p2 = g_new (Pixel, 1);

                        p2->x = p2x;
                        p2->y = p2y;
                        g_queue_push_tail (q, p2);
                        visited[p2x + p2y * width] = TRUE;
                      }
                  }
                p2x = p->x - 1;
                p2y = p->y - 1;
                if (p2x >= 0 && p2x < width && p2y >= 0 && p2y < height)
                  {
                    gegl_buffer_sample (buffer, p2x, p2y, NULL, &has_stroke, NULL,
                                        GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
                    if (has_stroke && ! visited[p2x + p2y * width])
                      {
                        Pixel *p2 = g_new (Pixel, 1);

                        p2->x = p2x;
                        p2->y = p2y;
                        g_queue_push_tail (q, p2);
                        visited[p2x + p2y * width] = TRUE;
                      }
                  }
                p2x = p->x - 1;
                p2y = p->y + 1;
                if (p2x >= 0 && p2x < width && p2y >= 0 && p2y < height)
                  {
                    gegl_buffer_sample (buffer, p2x, p2y, NULL, &has_stroke, NULL,
                                        GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
                    if (has_stroke && ! visited[p2x + p2y * width])
                      {
                        Pixel *p2 = g_new (Pixel, 1);

                        p2->x = p2x;
                        p2->y = p2y;
                        g_queue_push_tail (q, p2);
                        visited[p2x + p2y * width] = TRUE;
                      }
                  }
                p2x = p->x + 1;
                p2y = p->y - 1;
                if (p2x >= 0 && p2x < width && p2y >= 0 && p2y < height)
                  {
                    gegl_buffer_sample (buffer, p2x, p2y, NULL, &has_stroke, NULL,
                                        GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
                    if (has_stroke && ! visited[p2x + p2y * width])
                      {
                        Pixel *p2 = g_new (Pixel, 1);

                        p2->x = p2x;
                        p2->y = p2y;
                        g_queue_push_tail (q, p2);
                        visited[p2x + p2y * width] = TRUE;
                      }
                  }

                ++regionSize;
                if (regionSize < minimum_area)
                  g_array_append_val (region, *p);
                g_free (p);
              }
            if (regionSize < minimum_area)
              {
                Pixel *pixel = (Pixel *) region->data;
                gint   i = 0;

                for (; i < region->len; i++)
                  {
                    guchar val = 0;
                    gegl_buffer_set (buffer, GEGL_RECTANGLE (pixel->x, pixel->y, 1, 1), 0,
                                     NULL, &val, GEGL_AUTO_ROWSTRIDE);
                    pixel++;
                  }
              }
            g_array_remove_range (region, 0, region->len);
          }
      }
  g_array_free (region, TRUE);
  g_queue_free_full (q, g_free);
  g_free (visited);
}

static void
gimp_lineart_compute_normals_curvatures (GeglBuffer *mask,
                                         gfloat     *normals,
                                         gfloat     *curvatures,
                                         gfloat     *smoothed_curvatures,
                                         int         normal_estimate_mask_size)
{
  gfloat  *edgels_curvatures;
  gfloat  *smoothed_curvature;
  GArray  *es    = gimp_edgelset_new (mask);
  Edgel  **e     = (Edgel **) es->data;
  gint     width = gegl_buffer_get_width (mask);

  gimp_edgelset_smooth_normals (es, normal_estimate_mask_size);
  gimp_edgelset_compute_curvature (es);

  while (*e)
    {
      const float curvature = ((*e)->curvature > 0.0f) ? (*e)->curvature : 0.0f;
      const float w         = MAX (1e-8f, curvature * curvature);

      normals[((*e)->x + (*e)->y * width) * 2] += w * (*e)->x_normal;
      normals[((*e)->x + (*e)->y * width) * 2 + 1] += w * (*e)->y_normal;
      curvatures[(*e)->x + (*e)->y * width] = MAX (curvature,
                                                   curvatures[(*e)->x + (*e)->y * width]);
      e++;
    }
  for (int y = 0; y < gegl_buffer_get_height (mask); ++y)
    for (int x = 0; x < gegl_buffer_get_width (mask); ++x)
      {
        const float _angle = atan2f (normals[(x + y * width) * 2 + 1],
                                     normals[(x + y * width) * 2]);
        normals[(x + y * width) * 2] = cosf (_angle);
        normals[(x + y * width) * 2 + 1] = sinf (_angle);
      }

  /* Smooth curvatures on edgels, then take maximum on each pixel. */
  edgels_curvatures = gimp_lineart_get_smooth_curvatures (es);
  smoothed_curvature = edgels_curvatures;

  e = (Edgel **) es->data;
  while (*e)
    {
      gfloat *pixel_curvature = &smoothed_curvatures[(*e)->x + (*e)->y * width];

      if (*pixel_curvature < *smoothed_curvature)
        *pixel_curvature = *smoothed_curvature;

      ++smoothed_curvature;
      e++;
    }
  g_free (edgels_curvatures);

  g_array_free (es, TRUE);
}

static gfloat *
gimp_lineart_get_smooth_curvatures (GArray *edgelset)
{
  Edgel  **e;
  gfloat  *smoothed_curvatures = g_new0 (gfloat, edgelset->len);
  gfloat   weights[9];
  gfloat   smoothed_curvature;
  gfloat   weights_sum;
  gint     idx = 0;

  weights[0] = 1.0f;
  for (int i = 1; i <= 8; ++i)
    weights[i] = expf (-(i * i) / 30.0f);

  e = (Edgel **) edgelset->data;
  while (*e)
    {
      Edgel *edgel_before = g_array_index (edgelset, Edgel*, (*e)->previous);
      Edgel *edgel_after  = g_array_index (edgelset, Edgel*, (*e)->next);
      int    n = 5;
      int    i = 1;

      smoothed_curvature = (*e)->curvature;
      weights_sum = weights[0];
      while (n-- && (edgel_after != edgel_before))
        {
          smoothed_curvature += weights[i] * edgel_before->curvature;
          smoothed_curvature += weights[i] * edgel_after->curvature;
          edgel_before = g_array_index (edgelset, Edgel*, edgel_before->previous);
          edgel_after  = g_array_index (edgelset, Edgel*, edgel_after->next);
          weights_sum += 2 * weights[i];
          i++;
        }
      smoothed_curvature /= weights_sum;
      smoothed_curvatures[idx++] = smoothed_curvature;

      e++;
    }

  return smoothed_curvatures;
}

/**
 * Keep one pixel per connected component of curvature extremums.
 */
static GArray *
gimp_lineart_curvature_extremums (gfloat *curvatures,
                                  gfloat *smoothed_curvatures,
                                  gint    width,
                                  gint    height)
{
  gboolean *visited = g_new0 (gboolean, width * height);
  GQueue   *q       = g_queue_new ();
  GArray   *max_positions;

  max_positions = g_array_new (FALSE, TRUE, sizeof (Pixel));

  for (int y = 0; y < height; ++y)
    for (int x = 0; x < width; ++x)
      {
        if ((curvatures[x + y * width] > 0.0) && ! visited[x + y * width])
          {
            Pixel  *p = g_new (Pixel, 1);
            Pixel   max_curvature_pixel = gimp_vector2_new (-1.0, -1.0);
            gfloat  max_curvature = 0.0f;

            p->x = x;
            p->y = y;
            g_queue_push_tail (q, p);
            visited[x + y * width] = TRUE;

            while (! g_queue_is_empty (q))
              {
                gfloat c;
                gint   p2x;
                gint   p2y;

                p = (Pixel *) g_queue_pop_head (q);
                c = smoothed_curvatures[(gint) p->x + (gint) p->y * width];

                curvatures[(gint) p->x + (gint) p->y * width] = 0.0f;

                p2x = (gint) p->x + 1;
                p2y = (gint) p->y;
                if (p2x >= 0 && p2x < width    &&
                    p2y >= 0 && p2y < height   &&
                    curvatures[p2x + p2y * width] > 0.0 &&
                    ! visited[p2x + p2y * width])
                  {
                    Pixel *p2 = g_new (Pixel, 1);

                    p2->x = p2x;
                    p2->y = p2y;
                    g_queue_push_tail (q, p2);
                    visited[p2x + p2y * width] = TRUE;
                  }

                p2x = p->x - 1;
                p2y = p->y;
                if (p2x >= 0 && p2x < width    &&
                    p2y >= 0 && p2y < height   &&
                    curvatures[p2x + p2y * width] > 0.0 &&
                    ! visited[p2x + p2y * width])
                  {
                    Pixel *p2 = g_new (Pixel, 1);

                    p2->x = p2x;
                    p2->y = p2y;
                    g_queue_push_tail (q, p2);
                    visited[p2x + p2y * width] = TRUE;
                  }

                p2x = p->x;
                p2y = p->y - 1;
                if (p2x >= 0 && p2x < width    &&
                    p2y >= 0 && p2y < height   &&
                    curvatures[p2x + p2y * width] > 0.0 &&
                    ! visited[p2x + p2y * width])
                  {
                    Pixel *p2 = g_new (Pixel, 1);

                    p2->x = p2x;
                    p2->y = p2y;
                    g_queue_push_tail (q, p2);
                    visited[p2x + p2y * width] = TRUE;
                  }

                p2x = p->x;
                p2y = p->y + 1;
                if (p2x >= 0 && p2x < width    &&
                    p2y >= 0 && p2y < height   &&
                    curvatures[p2x + p2y * width] > 0.0 &&
                    ! visited[p2x + p2y * width])
                  {
                    Pixel *p2 = g_new (Pixel, 1);

                    p2->x = p2x;
                    p2->y = p2y;
                    g_queue_push_tail (q, p2);
                    visited[p2x + p2y * width] = TRUE;
                  }

                p2x = p->x + 1;
                p2y = p->y + 1;
                if (p2x >= 0 && p2x < width    &&
                    p2y >= 0 && p2y < height   &&
                    curvatures[p2x + p2y * width] > 0.0 &&
                    ! visited[p2x + p2y * width])
                  {
                    Pixel *p2 = g_new (Pixel, 1);

                    p2->x = p2x;
                    p2->y = p2y;
                    g_queue_push_tail (q, p2);
                    visited[p2x + p2y * width] = TRUE;
                  }

                p2x = p->x - 1;
                p2y = p->y - 1;
                if (p2x >= 0 && p2x < width    &&
                    p2y >= 0 && p2y < height   &&
                    curvatures[p2x + p2y * width] > 0.0 &&
                    ! visited[p2x + p2y * width])
                  {
                    Pixel *p2 = g_new (Pixel, 1);

                    p2->x = p2x;
                    p2->y = p2y;
                    g_queue_push_tail (q, p2);
                    visited[p2x + p2y * width] = TRUE;
                  }

                p2x = p->x - 1;
                p2y = p->y + 1;
                if (p2x >= 0 && p2x < width    &&
                    p2y >= 0 && p2y < height   &&
                    curvatures[p2x + p2y * width] > 0.0 &&
                    ! visited[p2x + p2y * width])
                  {
                    Pixel *p2 = g_new (Pixel, 1);

                    p2->x = p2x;
                    p2->y = p2y;
                    g_queue_push_tail (q, p2);
                    visited[p2x + p2y * width] = TRUE;
                  }

                p2x = p->x + 1;
                p2y = p->y - 1;
                if (p2x >= 0 && p2x < width    &&
                    p2y >= 0 && p2y < height   &&
                    curvatures[p2x + p2y * width] > 0.0 &&
                    ! visited[p2x + p2y * width])
                  {
                    Pixel *p2 = g_new (Pixel, 1);

                    p2->x = p2x;
                    p2->y = p2y;
                    g_queue_push_tail (q, p2);
                    visited[p2x + p2y * width] = TRUE;
                  }

                if (c > max_curvature)
                  {
                    max_curvature_pixel = *p;
                    max_curvature = c;
                  }
                g_free (p);
              }
            curvatures[(gint) max_curvature_pixel.x + (gint) max_curvature_pixel.y * width] = max_curvature;
            g_array_append_val (max_positions, max_curvature_pixel);
          }
      }
  g_queue_free_full (q, g_free);
  g_free (visited);

  return max_positions;
}

static gint
gimp_spline_candidate_cmp (const SplineCandidate *a,
                           const SplineCandidate *b,
                           gpointer               user_data)
{
  /* This comparison actually returns the opposite of common comparison
   * functions on purpose, as we want the first element on the list to
   * be the "bigger".
   */
  if (a->quality < b->quality)
    return 1;
  else if (a->quality > b->quality)
    return -1;
  else
    return 0;
}

static GList *
gimp_lineart_find_spline_candidates (GArray *max_positions,
                                     gfloat *normals,
                                     gint    width,
                                     gint    distance_threshold,
                                     gfloat  max_angle_deg)
{
  GList       *candidates = NULL;
  const float  CosMin     = cosf (M_PI * (max_angle_deg / 180.0));
  gint         i;

  for (i = 0; i < max_positions->len; i++)
    {
      Pixel p1 = g_array_index (max_positions, Pixel, i);
      gint  j;

      for (j = i + 1; j < max_positions->len; j++)
        {
          Pixel       p2 = g_array_index (max_positions, Pixel, j);
          const float distance = gimp_vector2_length_val (gimp_vector2_sub_val (p1, p2));

          if (distance <= distance_threshold)
            {
              GimpVector2 normalP1;
              GimpVector2 normalP2;
              GimpVector2 p1f;
              GimpVector2 p2f;
              GimpVector2 p1p2;
              float       cosN;
              float       qualityA;
              float       qualityB;
              float       qualityC;
              float       quality;

              normalP1 = gimp_vector2_new (normals[((gint) p1.x + (gint) p1.y * width) * 2],
                                           normals[((gint) p1.x + (gint) p1.y * width) * 2 + 1]);
              normalP2 = gimp_vector2_new (normals[((gint) p2.x + (gint) p2.y * width) * 2],
                                           normals[((gint) p2.x + (gint) p2.y * width) * 2 + 1]);
              p1f = gimp_vector2_new (p1.x, p1.y);
              p2f = gimp_vector2_new (p2.x, p2.y);
              p1p2 = gimp_vector2_sub_val (p2f, p1f);

              cosN = gimp_vector2_inner_product_val (normalP1, (gimp_vector2_neg_val (normalP2)));
              qualityA = MAX (0.0f, 1 - distance / distance_threshold);
              qualityB = MAX (0.0f,
                              (float) (gimp_vector2_inner_product_val (normalP1, p1p2) - gimp_vector2_inner_product_val (normalP2, p1p2)) /
                              distance);
              qualityC = MAX (0.0f, cosN - CosMin);
              quality = qualityA * qualityB * qualityC;
              if (quality > 0)
                {
                  SplineCandidate *candidate = g_new (SplineCandidate, 1);

                  candidate->p1      = p1;
                  candidate->p2      = p2;
                  candidate->quality = quality;

                  candidates = g_list_insert_sorted_with_data (candidates, candidate,
                                                               (GCompareDataFunc) gimp_spline_candidate_cmp,
                                                               NULL);
                }
            }
        }
    }
  return candidates;
}

static GArray *
gimp_lineart_discrete_spline (Pixel       p0,
                              GimpVector2 n0,
                              Pixel       p1,
                              GimpVector2 n1)
{
  GArray       *points = g_array_new (FALSE, TRUE, sizeof (Pixel));
  const double  a0 = 2 * p0.x - 2 * p1.x + n0.x - n1.x;
  const double  b0 = -3 * p0.x + 3 * p1.x - 2 * n0.x + n1.x;
  const double  c0 = n0.x;
  const double  d0 = p0.x;
  const double  a1 = 2 * p0.y - 2 * p1.y + n0.y - n1.y;
  const double  b1 = -3 * p0.y + 3 * p1.y - 2 * n0.y + n1.y;
  const double  c1 = n0.y;
  const double  d1 = p0.y;

  double        t = 0.0;
  const double  dtMin = 1.0 / MAX (fabs (p0.x - p1.x), fabs (p0.y - p1.y));
  Pixel         point = gimp_vector2_new ((gint) round (d0), (gint) round (d1));

  g_array_append_val (points, point);

  while (t <= 1.0)
    {
      const double t2 = t * t;
      const double t3 = t * t2;
      double       dx;
      double       dy;
      Pixel        p = gimp_vector2_new ((gint) round (a0 * t3 + b0 * t2 + c0 * t + d0),
                                         (gint) round (a1 * t3 + b1 * t2 + c1 * t + d1));

      /* create gimp_vector2_neq () ? */
      if (g_array_index (points, Pixel, points->len - 1).x != p.x ||
          g_array_index (points, Pixel, points->len - 1).y != p.y)
        {
          g_array_append_val (points, p);
        }
      dx = fabs (3 * a0 * t * t + 2 * b0 * t + c0) + 1e-8;
      dy = fabs (3 * a1 * t * t + 2 * b1 * t + c1) + 1e-8;
      t += MIN (dtMin, 0.75 / MAX (dx, dy));
    }
  if (g_array_index (points, Pixel, points->len - 1).x != p1.x ||
      g_array_index (points, Pixel, points->len - 1).y != p1.y)
    {
      g_array_append_val (points, p1);
    }
  return points;
}

static gint
gimp_number_of_transitions (GArray     *pixels,
                            GeglBuffer *buffer,
                            gboolean    border_value)
{
  int result = 0;

  if (pixels->len > 0)
    {
      Pixel    it = g_array_index (pixels, Pixel, 0);
      guchar   value;
      gboolean previous;
      gint     i;

      gegl_buffer_sample (buffer, (gint) it.x, (gint) it.y, NULL, &value, NULL,
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
      previous = (gboolean) value;

      /* Starts at the second element. */
      for (i = 1; i < pixels->len; i++)
        {
          gboolean val;

          it = g_array_index (pixels, Pixel, i);
          if (it.x >= 0 && it.x < gegl_buffer_get_width (buffer) &&
              it.y >= 0 && it.y < gegl_buffer_get_height (buffer))
            {
              guchar value;

              gegl_buffer_sample (buffer, (gint) it.x, (gint) it.y, NULL, &value, NULL,
                                  GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
              val = (gboolean) value;
            }
          else
            {
              val = border_value;
            }
          result += (val != previous);
          previous = val;
        }
    }

  return result;
}

/**
 * Check whether a set of points will create a 4-connected background
 * region whose size (i.e. number of pixels) falls within a given interval.
 */
static gboolean
gimp_lineart_curve_creates_region (GeglBuffer *mask,
                                   GArray     *pixels,
                                   int         lower_size_limit,
                                   int         upper_size_limit)
{
  const glong max_edgel_count = 2 * (upper_size_limit + 1);
  Pixel      *p = (Pixel*) pixels->data;
  gint        i;

  /* Mark pixels */
  for (i = 0; i < pixels->len; i++)
    {
      if (p->x >= 0 && p->x < gegl_buffer_get_width (mask) &&
          p->y >= 0 && p->y < gegl_buffer_get_height (mask))
        {
          guchar val;

          gegl_buffer_sample (mask, (gint) p->x, (gint) p->y, NULL, &val,
                              NULL, GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
          val = val ? 3 : 2;
          gegl_buffer_set (mask, GEGL_RECTANGLE ((gint) p->x, (gint) p->y, 1, 1), 0,
                           NULL, &val, GEGL_AUTO_ROWSTRIDE);
        }
      p++;
    }

  for (i = 0; i < pixels->len; i++)
    {
      Pixel p = g_array_index (pixels, Pixel, i);

      for (int direction = 0; direction < 4; ++direction)
        {
          if (p.x >= 0 && p.x < gegl_buffer_get_width (mask) &&
              p.y >= 0 && p.y < gegl_buffer_get_height (mask) &&
              border_in_direction (mask, p, direction))
            {
              Edgel  e;
              guchar val;
              glong  count;
              glong  area;

              gegl_buffer_sample (mask, (gint) p.x, (gint) p.y, NULL, &val,
                                  NULL, GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
              if ((gboolean) (val & (4 << direction)))
                continue;

              gimp_edgel_init (&e);
              e.x = p.x;
              e.y = p.y;
              e.direction = direction;

              count = gimp_edgel_track_mark (mask, e, max_edgel_count);
              if ((count != -1) && (count <= max_edgel_count)              &&
                  ((area = -1 * gimp_edgel_region_area (mask, e)) >= lower_size_limit) &&
                  (area <= upper_size_limit))
                {
                  gint j;

                  /* Remove marks */
                  for (j = 0; j < pixels->len; j++)
                    {
                      Pixel p2 = g_array_index (pixels, Pixel, j);

                      if (p2.x >= 0 && p2.x < gegl_buffer_get_width (mask) &&
                          p2.y >= 0 && p2.y < gegl_buffer_get_height (mask))
                        {
                          guchar val;

                          gegl_buffer_sample (mask, (gint) p2.x, (gint) p2.y, NULL, &val,
                                              NULL, GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
                          val &= 1;
                          gegl_buffer_set (mask, GEGL_RECTANGLE ((gint) p2.x, (gint) p2.y, 1, 1), 0,
                                           NULL, &val, GEGL_AUTO_ROWSTRIDE);
                        }
                    }
                  return TRUE;
                }
            }
        }
    }

  /* Remove marks */
  for (i = 0; i < pixels->len; i++)
    {
      Pixel p = g_array_index (pixels, Pixel, i);

      if (p.x >= 0 && p.x < gegl_buffer_get_width (mask) &&
          p.y >= 0 && p.y < gegl_buffer_get_height (mask))
        {
          guchar val;

          gegl_buffer_sample (mask, (gint) p.x, (gint) p.y, NULL, &val,
                              NULL, GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
          val &= 1;
          gegl_buffer_set (mask, GEGL_RECTANGLE ((gint) p.x, (gint) p.y, 1, 1), 0,
                           NULL, &val, GEGL_AUTO_ROWSTRIDE);
        }
    }
  return FALSE;
}

static GArray *
gimp_lineart_line_segment_until_hit (const GeglBuffer *mask,
                                     Pixel             start,
                                     GimpVector2       direction,
                                     int               size)
{
  GeglBuffer  *buffer = (GeglBuffer *) mask;
  gboolean     out = FALSE;
  GArray      *points = g_array_new (FALSE, TRUE, sizeof (Pixel));
  int          tmax;
  GimpVector2  p0 = gimp_vector2_new (start.x, start.y);

  gimp_vector2_mul (&direction, (gdouble) size);
  direction.x = round (direction.x);
  direction.y = round (direction.y);

  tmax = MAX (abs ((int) direction.x), abs ((int) direction.y));

  for (int t = 0; t <= tmax; ++t)
    {
      GimpVector2 v = gimp_vector2_add_val (p0, gimp_vector2_mul_val (direction, (float)t / tmax));
      Pixel       p;

      p.x = (gint) round (v.x);
      p.y = (gint) round (v.y);
      if (p.x >= 0 && p.x < gegl_buffer_get_width (buffer) &&
          p.y >= 0 && p.y < gegl_buffer_get_height (buffer))
        {
          guchar val;
          gegl_buffer_sample (buffer, p.x, p.y, NULL, &val,
                              NULL, GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
          if (out && val)
            {
              return points;
            }
          out = ! val;
        }
      else if (out)
        {
          return points;
        }
      else
        {
          g_array_free (points, TRUE);
          return g_array_new (FALSE, TRUE, sizeof (Pixel));
        }
      g_array_append_val (points, p);
    }

  g_array_free (points, TRUE);
  return g_array_new (FALSE, TRUE, sizeof (Pixel));
}

static gfloat *
gimp_lineart_estimate_strokes_radii (GeglBuffer *mask)
{
  GeglBufferIterator *gi;
  gfloat             *dist;
  gfloat             *thickness;
  GeglBuffer         *distmap;
  GeglNode           *graph;
  GeglNode           *input;
  GeglNode           *op;
  GeglNode           *sink;
  gint                width  = gegl_buffer_get_width (mask);
  gint                height = gegl_buffer_get_height (mask);

  /* Compute a distance map for the line art. */
  graph = gegl_node_new ();
  input = gegl_node_new_child (graph,
                               "operation", "gegl:buffer-source",
                               "buffer", mask,
                               NULL);
  op  = gegl_node_new_child (graph,
                             "operation", "gegl:distance-transform",
                             "metric",    GEGL_DISTANCE_METRIC_EUCLIDEAN,
                             "normalize", FALSE,
                             NULL);
  sink  = gegl_node_new_child (graph,
                               "operation", "gegl:buffer-sink",
                               "buffer", &distmap,
                               NULL);
  gegl_node_connect_to (input, "output",
                        op, "input");
  gegl_node_connect_to (op, "output",
                        sink, "input");
  gegl_node_process (sink);
  g_object_unref (graph);

  dist = g_new (gfloat, width * height);
  gegl_buffer_get (distmap, NULL, 1.0, NULL, dist,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  thickness = g_new0 (gfloat, width * height);

  gi = gegl_buffer_iterator_new (mask, NULL, 0, NULL,
                                 GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 1);
  while (gegl_buffer_iterator_next (gi))
    {
      guint8 *m = (guint8*) gi->items[0].data;
      gint    startx = gi->items[0].roi.x;
      gint    starty = gi->items[0].roi.y;
      gint    endy   = starty + gi->items[0].roi.height;
      gint    endx   = startx + gi->items[0].roi.width;
      gint    x;
      gint    y;

      for (y = starty; y < endy; y++)
        for (x = startx; x < endx; x++)
          {
            if (*m && dist[x + y * width] == 1.0)
              {
                gint     dx = x;
                gint     dy = y;
                gfloat   d  = 1.0;
                gfloat   nd;
                gboolean neighbour_thicker = TRUE;

                while (neighbour_thicker)
                  {
                    gint px = dx - 1;
                    gint py = dy - 1;
                    gint nx = dx + 1;
                    gint ny = dy + 1;

                    neighbour_thicker = FALSE;
                    if (px >= 0)
                      {
                        if ((nd = dist[px + dy * width]) > d)
                          {
                            d = nd;
                            dx = px;
                            neighbour_thicker = TRUE;
                            continue;
                          }
                        if (py >= 0 && (nd = dist[px + py * width]) > d)
                          {
                            d = nd;
                            dx = px;
                            dy = py;
                            neighbour_thicker = TRUE;
                            continue;
                          }
                        if (ny < height && (nd = dist[px + ny * width]) > d)
                          {
                            d = nd;
                            dx = px;
                            dy = ny;
                            neighbour_thicker = TRUE;
                            continue;
                          }
                      }
                    if (nx < width)
                      {
                        if ((nd = dist[nx + dy * width]) > d)
                          {
                            d = nd;
                            dx = nx;
                            neighbour_thicker = TRUE;
                            continue;
                          }
                        if (py >= 0 && (nd = dist[nx + py * width]) > d)
                          {
                            d = nd;
                            dx = nx;
                            dy = py;
                            neighbour_thicker = TRUE;
                            continue;
                          }
                        if (ny < height && (nd = dist[nx + ny * width]) > d)
                          {
                            d = nd;
                            dx = nx;
                            dy = ny;
                            neighbour_thicker = TRUE;
                            continue;
                          }
                      }
                    if (py > 0 && (nd = dist[dx + py * width]) > d)
                      {
                        d = nd;
                        dy = py;
                        neighbour_thicker = TRUE;
                        continue;
                      }
                    if (ny < height && (nd = dist[dx + ny * width]) > d)
                      {
                        d = nd;
                        dy = ny;
                        neighbour_thicker = TRUE;
                        continue;
                      }
                  }
                thickness[(gint) x + (gint) y * width] = d;
              }
            m++;
          }
    }

  g_free (dist);
  g_object_unref (distmap);

  return thickness;
}

static guint
visited_hash_fun (Pixel *key)
{
  /* Cantor pairing function. */
  return (key->x + key->y) * (key->x + key->y + 1) / 2 + key->y;
}

static gboolean
visited_equal_fun (Pixel *e1,
                   Pixel *e2)
{
  return (e1->x == e2->x && e1->y == e2->y);
}

static inline gboolean
border_in_direction (GeglBuffer *mask,
                     Pixel       p,
                     int         direction)
{
  gint px = (gint) p.x + DeltaX[direction];
  gint py = (gint) p.y + DeltaY[direction];

  if (px >= 0 && px < gegl_buffer_get_width (mask) &&
      py >= 0 && py < gegl_buffer_get_height (mask))
    {
      guchar val;

      gegl_buffer_sample (mask, px, py, NULL, &val,
                          NULL, GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
      return ! ((gboolean) val);
    }
  return TRUE;
}

static inline GimpVector2
pair2normal (Pixel   p,
             gfloat *normals,
             gint    width)
{
  return gimp_vector2_new (normals[((gint) p.x + (gint) p.y * width) * 2],
                           normals[((gint) p.x + (gint) p.y * width) * 2 + 1]);
}
/* Edgel functions */

static Edgel *
gimp_edgel_new (int       x,
                int       y,
                Direction direction)
{
  Edgel *edgel = g_new (Edgel, 1);

  edgel->x         = x;
  edgel->y         = y;
  edgel->direction = direction;

  gimp_edgel_init (edgel);

  return edgel;
}

static void
gimp_edgel_init (Edgel *edgel)
{
  edgel->x_normal  = 0;
  edgel->y_normal  = 0;
  edgel->curvature = 0;
  edgel->next      = edgel->previous = G_MAXUINT;
}

static void
gimp_edgel_clear (Edgel **edgel)
{
  g_clear_pointer (edgel, g_free);
}

static int
gimp_edgel_cmp (const Edgel* e1,
                const Edgel* e2)
{
  gimp_assert (e1 && e2);

  if ((e1->x == e2->x) && (e1->y == e2->y) &&
      (e1->direction == e2->direction))
    return 0;
  else if ((e1->y < e2->y) || (e1->y == e2->y && e1->x < e2->x) ||
           (e1->y == e2->y && e1->x == e2->x && e1->direction < e2->direction))
    return -1;
  else
    return 1;
}

static guint
edgel2index_hash_fun (Edgel *key)
{
  /* Cantor pairing function.
   * Was not sure how to use the direction though. :-/
   */
  return (key->x + key->y) * (key->x + key->y + 1) / 2 + key->y * key->direction;
}

static gboolean
edgel2index_equal_fun (Edgel *e1,
                       Edgel *e2)
{
  return (e1->x == e2->x && e1->y == e2->y &&
          e1->direction == e2->direction);
}

/**
 * @mask;
 * @edgel:
 * @size_limit:
 *
 * Track a border, marking inner pixels with a bit corresponding to the
 * edgel traversed (4 << direction) for direction in {0,1,2,3}.
 * Stop tracking after @size_limit edgels have been visited.
 *
 * Returns: Number of visited edgels, or -1 if an already visited edgel
 *          has been encountered.
 */
static glong
gimp_edgel_track_mark (GeglBuffer *mask,
                       Edgel       edgel,
                       long        size_limit)
{
  Edgel start = edgel;
  long  count = 1;

  do
    {
      guchar val;

      gimp_edgelset_next8 (mask, &edgel, &edgel);
      gegl_buffer_sample (mask, edgel.x, edgel.y, NULL, &val,
                          NULL, GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
      if (val & 2)
        {
          /* Only mark pixels of the spline/segment */
          if (val & (4 << edgel.direction))
            return -1;

          /* Mark edgel in pixel (1 == In Mask, 2 == Spline/Segment) */
          val |= (4 << edgel.direction);
          gegl_buffer_set (mask, GEGL_RECTANGLE (edgel.x, edgel.y, 1, 1), 0,
                           NULL, &val, GEGL_AUTO_ROWSTRIDE);
        }
      if (gimp_edgel_cmp (&edgel, &start) != 0)
        ++count;
    }
  while (gimp_edgel_cmp (&edgel, &start) != 0 && count <= size_limit);

  return count;
}

static glong
gimp_edgel_region_area (const GeglBuffer *mask,
                        Edgel             start_edgel)
{
  Edgel edgel = start_edgel;
  long  area = 0;

  do
    {
      if (edgel.direction == XPlusDirection)
        area += edgel.x;
      else if (edgel.direction == XMinusDirection)
        area -= edgel.x - 1;

      gimp_edgelset_next8 (mask, &edgel, &edgel);
    }
  while (gimp_edgel_cmp (&edgel, &start_edgel) != 0);

  return area;
}

/* Edgel sets */

static GArray *
gimp_edgelset_new (GeglBuffer *buffer)
{
  GeglBufferIterator *gi;
  GArray             *set;
  GHashTable         *edgel2index;
  gint                width  = gegl_buffer_get_width (buffer);
  gint                height = gegl_buffer_get_height (buffer);

  set = g_array_new (TRUE, TRUE, sizeof (Edgel *));
  g_array_set_clear_func (set, (GDestroyNotify) gimp_edgel_clear);

  if (width <= 1 || height <= 1)
    return set;

  edgel2index = g_hash_table_new ((GHashFunc) edgel2index_hash_fun,
                                  (GEqualFunc) edgel2index_equal_fun);

  gi = gegl_buffer_iterator_new (buffer, GEGL_RECTANGLE (0, 0, width, height),
                                 0, NULL, GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 5);
  gegl_buffer_iterator_add (gi, buffer, GEGL_RECTANGLE (0, -1, width, height),
                            0, NULL, GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
  gegl_buffer_iterator_add (gi, buffer, GEGL_RECTANGLE (0, 1, width, height),
                            0, NULL, GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
  gegl_buffer_iterator_add (gi, buffer, GEGL_RECTANGLE (-1, 0, width, height),
                            0, NULL, GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
  gegl_buffer_iterator_add (gi, buffer, GEGL_RECTANGLE (1, 0, width, height),
                            0, NULL, GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
  while (gegl_buffer_iterator_next (gi))
    {
      guint8 *p      = (guint8*) gi->items[0].data;
      guint8 *prevy  = (guint8*) gi->items[1].data;
      guint8 *nexty  = (guint8*) gi->items[2].data;
      guint8 *prevx  = (guint8*) gi->items[3].data;
      guint8 *nextx  = (guint8*) gi->items[4].data;
      gint    startx = gi->items[0].roi.x;
      gint    starty = gi->items[0].roi.y;
      gint    endy   = starty + gi->items[0].roi.height;
      gint    endx   = startx + gi->items[0].roi.width;
      gint    x;
      gint    y;

      for (y = starty; y < endy; y++)
        for (x = startx; x < endx; x++)
          {
            if (*(p++))
              {
                if (y == 0 || ! *prevy)
                  gimp_edgelset_add (set, x, y, YMinusDirection, edgel2index);
                if (y == height - 1 || ! *nexty)
                  gimp_edgelset_add (set, x, y, YPlusDirection, edgel2index);
                if (x == 0 || ! *prevx)
                  gimp_edgelset_add (set, x, y, XMinusDirection, edgel2index);
                if (x == width - 1 || ! *nextx)
                  gimp_edgelset_add (set, x, y, XPlusDirection, edgel2index);
              }
            prevy++;
            nexty++;
            prevx++;
            nextx++;
          }
    }

  gimp_edgelset_build_graph (set, buffer, edgel2index);
  g_hash_table_destroy (edgel2index);
  gimp_edgelset_init_normals (set);

  return set;
}

static void
gimp_edgelset_add (GArray     *set,
                   int         x,
                   int         y,
                   Direction   direction,
                   GHashTable *edgel2index)
{
  Edgel         *edgel    = gimp_edgel_new (x, y, direction);
  unsigned long  position = set->len;

  g_array_append_val (set, edgel);
  g_hash_table_insert (edgel2index, edgel, GUINT_TO_POINTER (position));
}

static void
gimp_edgelset_init_normals (GArray *set)
{
  Edgel **e = (Edgel**) set->data;

  while (*e)
    {
      GimpVector2 n = Direction2Normal[(*e)->direction];

      (*e)->x_normal = n.x;
      (*e)->y_normal = n.y;
      e++;
    }
}

static void
gimp_edgelset_smooth_normals (GArray *set,
                              int     mask_size)
{
  const gfloat sigma = mask_size * 0.775;
  const gfloat den   = 2 * sigma * sigma;
  gfloat       weights[65];
  GimpVector2  smoothed_normal;
  gint         i;

  gimp_assert (mask_size <= 65);

  weights[0] = 1.0f;
  for (int i = 1; i <= mask_size; ++i)
    weights[i] = expf (-(i * i) / den);

  for (i = 0; i < set->len; i++)
    {
      Edgel *it           = g_array_index (set, Edgel*, i);
      Edgel *edgel_before = g_array_index (set, Edgel*, it->previous);
      Edgel *edgel_after  = g_array_index (set, Edgel*, it->next);
      int    n = mask_size;
      int    i = 1;

      smoothed_normal = Direction2Normal[it->direction];
      while (n-- && (edgel_after != edgel_before))
        {
          smoothed_normal = gimp_vector2_add_val (smoothed_normal,
                                                  gimp_vector2_mul_val (Direction2Normal[edgel_before->direction], weights[i]));
          smoothed_normal = gimp_vector2_add_val (smoothed_normal,
                                                  gimp_vector2_mul_val (Direction2Normal[edgel_after->direction], weights[i]));
          edgel_before = g_array_index (set, Edgel *, edgel_before->previous);
          edgel_after  = g_array_index (set, Edgel *, edgel_after->next);
          ++i;
        }
      gimp_vector2_normalize (&smoothed_normal);
      it->x_normal = smoothed_normal.x;
      it->y_normal = smoothed_normal.y;
    }
}

static void
gimp_edgelset_compute_curvature (GArray *set)
{
  gint i;

  for (i = 0; i < set->len; i++)
    {
      Edgel       *it        = g_array_index (set, Edgel*, i);
      Edgel       *previous  = g_array_index (set, Edgel *, it->previous);
      Edgel       *next      = g_array_index (set, Edgel *, it->next);
      GimpVector2  n_prev    = gimp_vector2_new (previous->x_normal, previous->y_normal);
      GimpVector2  n_next    = gimp_vector2_new (next->x_normal, next->y_normal);
      GimpVector2  diff      = gimp_vector2_mul_val (gimp_vector2_sub_val (n_next, n_prev),
                                                     0.5);
      const float  c         = gimp_vector2_length_val (diff);
      const float  crossp    = n_prev.x * n_next.y - n_prev.y * n_next.x;

      it->curvature = (crossp > 0.0f) ? c : -c;
      ++it;
    }
}

static void
gimp_edgelset_build_graph (GArray     *set,
                           GeglBuffer *buffer,
                           GHashTable *edgel2index)
{
  Edgel edgel;
  gint  i;

  for (i = 0; i < set->len; i++)
    {
      Edgel *neighbor;
      Edgel *it = g_array_index (set, Edgel *, i);
      guint  neighbor_pos;

      gimp_edgelset_next8 (buffer, it, &edgel);

      gimp_assert (g_hash_table_contains (edgel2index, &edgel));
      neighbor_pos = GPOINTER_TO_UINT (g_hash_table_lookup (edgel2index, &edgel));
      it->next = neighbor_pos;
      neighbor = g_array_index (set, Edgel *, neighbor_pos);
      neighbor->previous = i;
    }
}

static void
gimp_edgelset_next8 (const GeglBuffer *buffer,
                     Edgel            *it,
                     Edgel            *n)
{
  const int lx = gegl_buffer_get_width ((GeglBuffer *) buffer) - 1;
  const int ly = gegl_buffer_get_height ((GeglBuffer *) buffer) - 1;
  guchar    has_stroke;

  n->x         = it->x;
  n->y         = it->y;
  n->direction = it->direction;
  switch (n->direction)
    {
    case XPlusDirection:
      gegl_buffer_sample ((GeglBuffer *) buffer, n->x + 1, n->y + 1, NULL, &has_stroke, NULL,
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
      if ((n->x != lx) && (n->y != ly) && has_stroke)
        {
          ++(n->y);
          ++(n->x);
          n->direction = YMinusDirection;
        }
      else
        {
          gegl_buffer_sample ((GeglBuffer *) buffer, n->x, n->y + 1, NULL, &has_stroke, NULL,
                              GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
          if ((n->y != ly) && has_stroke)
            {
              ++(n->y);
            }
          else
            {
              n->direction = YPlusDirection;
            }
        }
      break;
    case YMinusDirection:
      gegl_buffer_sample ((GeglBuffer *) buffer, n->x + 1, n->y - 1, NULL, &has_stroke, NULL,
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
      if ((n->x != lx) && n->y && has_stroke)
        {
          ++(n->x);
          --(n->y);
          n->direction = XMinusDirection;
        }
      else
        {
          gegl_buffer_sample ((GeglBuffer *) buffer, n->x + 1, n->y, NULL, &has_stroke, NULL,
                              GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
          if ((n->x != lx) && has_stroke)
            {
              ++(n->x);
            }
          else
            {
              n->direction = XPlusDirection;
            }
        }
      break;
    case XMinusDirection:
      gegl_buffer_sample ((GeglBuffer *) buffer, n->x - 1, n->y - 1, NULL, &has_stroke, NULL,
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
      if (n->x && n->y && has_stroke)
        {
          --(n->x);
          --(n->y);
          n->direction = YPlusDirection;
        }
      else
        {
          gegl_buffer_sample ((GeglBuffer *) buffer, n->x, n->y - 1, NULL, &has_stroke, NULL,
                              GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
          if (n->y && has_stroke)
            {
              --(n->y);
            }
          else
            {
              n->direction = YMinusDirection;
            }
        }
      break;
    case YPlusDirection:
      gegl_buffer_sample ((GeglBuffer *) buffer, n->x - 1, n->y + 1, NULL, &has_stroke, NULL,
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
      if (n->x && (n->y != ly) && has_stroke)
        {
          --(n->x);
          ++(n->y);
          n->direction = XPlusDirection;
        }
      else
        {
          gegl_buffer_sample ((GeglBuffer *) buffer, n->x - 1, n->y, NULL, &has_stroke, NULL,
                              GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
          if (n->x && has_stroke)
            {
              --(n->x);
            }
          else
            {
              n->direction = XMinusDirection;
            }
        }
      break;
    default:
      gimp_assert (FALSE);
      break;
    }
}
