/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationflood.c
 * Copyright (C) 2016 Ell
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


/* Implementation of the Flood algorithm.
 * See https://wiki.gimp.org/wiki/Algorithms:Flood for details.
 */


#include "config.h"

#include <string.h> /* For `memcpy()`. */

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"

#include "operations-types.h"

#include "gimpoperationflood.h"


/* Maximal gap, in pixels, between consecutive dirty ranges, below (and
 * including) which they are coalesced, at the beginning of the distribution
 * step.
 */
#define GIMP_OPERATION_FLOOD_COALESCE_MAX_GAP 32


typedef struct _GimpOperationFloodSegment    GimpOperationFloodSegment;
typedef struct _GimpOperationFloodDirtyRange GimpOperationFloodDirtyRange;
typedef struct _GimpOperationFloodContext    GimpOperationFloodContext;


/* A segment. */
struct _GimpOperationFloodSegment
{
  /* A boolean flag indicating whether the image- and ROI-virtual coordinate
   * systems should be transposed when processing this segment.  TRUE iff the
   * segment is vertical.
   */
  guint  transpose      : 1;

  /* The y-coordinate of the segment, in the ROI-virtual coordinate system. */
  guint  y              : 8 * sizeof (guint) - 3;
  /* The difference between the y-coordinates of the source segment and this
   * segment, in the ROI-virtual coordinate system.  Either -1 or +1 for
   * ordinary segments, and 0 for seed segments, as a special case.
   *
   * Note the use of `signed` as the type specifier.  The C standard doesn't
   * specify the signedness of bit-fields whose type specifier is `int`, or a
   * typedef-name defined as `int`, such as `gint`.
   */
  signed source_y_delta : 2;

  /* The x-coordinates of the first and last pixels of the segment, in the ROI-
   * virtual coordinate system.  Note that this is a closed range:
   * [x[0], x[1]].
   */
  gint   x[2];
};
/* Make sure the maximal image dimension fits in
 * `GimpOperationFloodSegment::y`.
 */
G_STATIC_ASSERT (GIMP_MAX_IMAGE_SIZE <= (1 << (8 * sizeof (guint) - 3)));

/* A dirty range of the current segment. */
struct _GimpOperationFloodDirtyRange
{
  /* A boolean flag indicating whether the range was extended, or its existing
   * pixels were modified, during the horizontal propagation step.
   */
  gboolean modified;

  /* The x-coordinates of the first and last pixels of the range, in the ROI-
   * virtual coordinate system.  Note that this is a closed range:
   * [x[0], x[1]].
   */
  gint     x[2];
};

/* Common parameters for the various parts of the algorithm. */
struct _GimpOperationFloodContext
{
  /* Input image. */
  GeglBuffer                *input;
  /* Input image format. */
  const Babl                *input_format;
  /* Output image. */
  GeglBuffer                *output;
  /* Output image format. */
  const Babl                *output_format;

  /* Region of interset. */
  GeglRectangle  roi;

  /* Current segment. */
  GimpOperationFloodSegment  segment;

  /* The following arrays hold the ground- and water-level of the current- and
   * source-segments.  The vertical- and horizontal-propagation steps don't
   * generally access the input and output GEGL buffers directly, but rather
   * read from, and write to, these arrays, for efficiency.  These arrays are
   * read-from, and written-to, the corresponding GEGL buffers before and after
   * these steps.
   */

  /* Ground level of the current segment, indexed by x-coordinate in the ROI-
   * virtual coordinate system.  Only valid inside the range
   * `[segment.x[0], segment.x[1]]`.
   */
  gfloat                    *ground;
  /* Water level of the current segment, indexed by x-coordinate in the ROI-
   * virtual coordinate system.  Initially only valid inside the range
   * `[segment.x[0], segment.x[1]]`, but may be written-to outside this range
   * during horizontal propagation, if the dirty ranges are extended past the
   * bounds of the segment.
   */
  gfloat                    *water;
  /* Water level of the source segment, indexed by x-coordinate in the ROI-
   * virtual coordinate system.  Only valid inside the range
   * `[segment.x[0], segment.x[1]]`.
   */
  gfloat                    *source_water;

  /* A common buffer for the water level of the current- and source-segments.
   * `water` and `source_water` are pointers into this buffer.  This buffer is
   * used as an optimization, in order to read the water level of both segments
   * from the output GEGL buffer in a single call, and is otherwise not used
   * directly (`water` and `source_water` are used to access the water level
   * instead.)
   */
  gfloat                    *water_buffer;
};


static void          gimp_operation_flood_prepare                      (GeglOperation                     *operation);
static GeglRectangle gimp_operation_flood_get_required_for_output      (GeglOperation                     *self,
                                                                        const gchar                       *input_pad,
                                                                        const GeglRectangle               *roi);
static GeglRectangle gimp_operation_flood_get_cached_region            (GeglOperation                     *self,
                                                                        const GeglRectangle               *roi);

static void          gimp_operation_flood_process_push                 (GQueue                            *queue,
                                                                        gboolean                           transpose,
                                                                        gint                               y,
                                                                        gint                               source_y_delta,
                                                                        gint                               x0,
                                                                        gint                               x1);
static void          gimp_operation_flood_process_seed                 (GQueue                            *queue,
                                                                        const GeglRectangle               *roi);
static void          gimp_operation_flood_process_transform_rect       (const GimpOperationFloodContext    *ctx,
                                                                        GeglRectangle                      *dest,
                                                                        const GeglRectangle                *src);
static void          gimp_operation_flood_process_fetch                (GimpOperationFloodContext          *ctx);
static gint          gimp_operation_flood_process_propagate_vertical   (GimpOperationFloodContext          *ctx,
                                                                        GimpOperationFloodDirtyRange       *dirty_ranges);
static void          gimp_operation_flood_process_propagate_horizontal (GimpOperationFloodContext          *ctx,
                                                                        gint                                dir,
                                                                        GimpOperationFloodDirtyRange       *dirty_ranges,
                                                                        gint                                range_count);
static gint         gimp_operation_flood_process_coalesce              (const GimpOperationFloodContext    *ctx,
                                                                        GimpOperationFloodDirtyRange       *dirty_ranges,
                                                                        gint                                range_count,
                                                                        gint                                gap);
static void          gimp_operation_flood_process_commit               (const GimpOperationFloodContext    *ctx,
                                                                        const GimpOperationFloodDirtyRange *dirty_ranges,
                                                                        gint                                range_count);
static void          gimp_operation_flood_process_distribute           (const GimpOperationFloodContext    *ctx,
                                                                        GQueue                             *queue,
                                                                        const GimpOperationFloodDirtyRange *dirty_ranges,
                                                                        gint                                range_count);
static gboolean      gimp_operation_flood_process                      (GeglOperation                      *operation,
                                                                        GeglBuffer                         *input,
                                                                        GeglBuffer                         *output,
                                                                        const GeglRectangle                *roi,
                                                                        gint                                level);


G_DEFINE_TYPE (GimpOperationFlood, gimp_operation_flood,
               GEGL_TYPE_OPERATION_FILTER)

#define parent_class gimp_operation_flood_parent_class


/* GEGL graph for the test case. */
static const gchar* reference_xml = "<?xml version='1.0' encoding='UTF-8'?>"
"<gegl>"
"<node operation='gimp:flood'> </node>"
"<node operation='gegl:load'>"
"  <params>"
"    <param name='path'>flood-input.png</param>"
"  </params>"
"</node>"
"</gegl>";


static void
gimp_operation_flood_class_init (GimpOperationFloodClass *klass)
{
  GeglOperationClass       *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationFilterClass *filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  /* The input and output buffers must be different, since we generally need to
   * be able to access the input-image values after having written to the
   * output buffer.
   */
  operation_class->want_in_place = FALSE;
  /* We don't want `GeglOperationFilter` to split the image across multiple
   * threads, since this operation depends on, and affects, the image as a
   * whole.
   */
  operation_class->threaded      = FALSE;
  /* Note that both of these options are the default; we set them here for
   * explicitness.
   */

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:flood",
                                 "categories",  "gimp",
                                 "description", "GIMP Flood operation",
                                 "reference", "https://wiki.gimp.org/wiki/Algorithms:Flood",
                                 "reference-image", "flood-output.png",
                                 "reference-composition", reference_xml,
                                 NULL);

  operation_class->prepare                 = gimp_operation_flood_prepare;
  operation_class->get_required_for_output = gimp_operation_flood_get_required_for_output;
  operation_class->get_cached_region       = gimp_operation_flood_get_cached_region;

  filter_class->process                    = gimp_operation_flood_process;
}

static void
gimp_operation_flood_init (GimpOperationFlood *self)
{
}

static void
gimp_operation_flood_prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  gegl_operation_set_format (operation, "input",  babl_format_with_space ("Y float", space));
  gegl_operation_set_format (operation, "output", babl_format_with_space ("Y float", space));
}

static GeglRectangle
gimp_operation_flood_get_required_for_output (GeglOperation       *self,
                                              const gchar         *input_pad,
                                              const GeglRectangle *roi)
{
  return *gegl_operation_source_get_bounding_box (self, "input");
}

static GeglRectangle
gimp_operation_flood_get_cached_region (GeglOperation       *self,
                                        const GeglRectangle *roi)
{
  return *gegl_operation_source_get_bounding_box (self, "input");
}


/* Pushes a single segment into the queue. */
static void
gimp_operation_flood_process_push (GQueue   *queue,
                                   gboolean  transpose,
                                   gint      y,
                                   gint      source_y_delta,
                                   gint      x0,
                                   gint      x1)
{
  GimpOperationFloodSegment *segment;

  segment                 = g_slice_new (GimpOperationFloodSegment);

  segment->transpose      = transpose;
  segment->y              = y;
  segment->source_y_delta = source_y_delta;
  segment->x[0]           = x0;
  segment->x[1]           = x1;

  g_queue_push_tail (queue, segment);
}

/* Pushes the seed segments into the queue.  Recall that the seed segments are
 * indicated by having their `source_y_delta` field equal 0.
 *
 * `roi` is given in the image-physical coordinate system.
 */
static void
gimp_operation_flood_process_seed (GQueue              *queue,
                                   const GeglRectangle *roi)
{
  if (roi->width == 0 || roi->height == 0)
    return;

  /* Top edge. */
  gimp_operation_flood_process_push (queue,
                                     /* transpose      = */ FALSE,
                                     /* y              = */ 0,
                                     /* source_y_delta = */ 0,
                                     /* x0             = */ 0,
                                     /* x1             = */ roi->width - 1);

  if (roi->height == 1)
    return;

  /* Bottom edge. */
  gimp_operation_flood_process_push (queue,
                                     /* transpose      = */ FALSE,
                                     /* y              = */ roi->height - 1,
                                     /* source_y_delta = */ 0,
                                     /* x0             = */ 0,
                                     /* x1             = */ roi->width - 1);

  if (roi->height == 2)
    return;

  /* Left edge. */
  gimp_operation_flood_process_push (queue,
                                     /* transpose      = */ TRUE,
                                     /* y              = */ 0,
                                     /* source_y_delta = */ 0,
                                     /* x0             = */ 1,
                                     /* x1             = */ roi->height - 2);

  if (roi->width == 1)
    return;

  /* Right edge. */
  gimp_operation_flood_process_push (queue,
                                     /* transpose      = */ TRUE,
                                     /* y              = */ roi->width - 1,
                                     /* source_y_delta = */ 0,
                                     /* x0             = */ 1,
                                     /* x1             = */ roi->height - 2);
}

/* Transforms a `GeglRectangle` between the image-physical and image-virtual
 * coordinate systems, in either direction, based on the attributes of the
 * current segment (namely, its `transpose` flag.)
 *
 * Takes the input rectangle through `src`, and stores the result in `dest`.
 * Both parameters may refer to the same object.
 */
static void
gimp_operation_flood_process_transform_rect (const GimpOperationFloodContext *ctx,
                                             GeglRectangle                   *dest,
                                             const GeglRectangle             *src)
{
  if (! ctx->segment.transpose)
    *dest = *src;
  else
    {
      gint temp;

      temp         = src->x;
      dest->x      = src->y;
      dest->y      = temp;

      temp         = src->width;
      dest->width  = src->height;
      dest->height = temp;
    }
}

/* Reads the ground- and water-level for the current- and source-segments from
 * the GEGL buffers into the corresponding arrays.  Sets up the `water` and
 * `source_water` pointers of `ctx` to point to the right location in
 * `water_buffer`.
 */
static void
gimp_operation_flood_process_fetch (GimpOperationFloodContext *ctx)
{
  /* Image-virtual and image-physical rectangles, respectively. */
  GeglRectangle iv_rect, ip_rect;

  /* Set the horizontal extent of the rectangle to span the entire segment. */
  iv_rect.x     = ctx->roi.x + ctx->segment.x[0];
  iv_rect.width = ctx->segment.x[1] - ctx->segment.x[0] + 1;

  /* For reading the water level, we treat ordinary (non-seed) and seed
   * segments differently.
   */
  if (ctx->segment.source_y_delta != 0)
    {
      /* Ordinary segment. */

      /* We set the vertical extent of the rectangle to span both the current-
       * and the source-segments, and set the `water` and `source_water`
       * pointers to point to two consecutive rows of the `water_buffer` array
       * (the y-coordinate of the rectangle, and which row is above which,
       * depends on whether the source segment is above, or below, the current
       * one.)
       */
      if (ctx->segment.source_y_delta < 0)
        {
          iv_rect.y         = ctx->roi.y + ctx->segment.y - 1;
          ctx->water        = ctx->water_buffer + ctx->roi.width;
          ctx->source_water = ctx->water_buffer;
        }
      else
        {
          iv_rect.y         = ctx->roi.y + ctx->segment.y;
          ctx->water        = ctx->water_buffer;
          ctx->source_water = ctx->water_buffer + ctx->roi.width;
        }
      iv_rect.height        = 2;

      /* Transform `iv_rect` to the image-physical coordinate system, and store
       * the result in `ip_rect`.
       */
      gimp_operation_flood_process_transform_rect (ctx, &ip_rect, &iv_rect);

      /* Read the water level from the output GEGL buffer into `water_buffer`.
       *
       * Notice the stride:  If the current segment is horizontal, then we're
       * reading a pair of rows directly into the correct locations inside
       * `water_buffer` (i.e., `water` and `source_water`).  On the other hand,
       * if the current segment is vertical, then we're reading a pair of
       * *columns*; we set the stride to 2-pixels so that the current- and
       * source-water levels are interleaved in `water_buffer`, and reorder
       * them below.
       */
      gegl_buffer_get (ctx->output, &ip_rect, 1.0, ctx->output_format,
                       ctx->water_buffer + ctx->segment.x[0],
                       sizeof (gfloat) *
                       (ctx->segment.transpose ? 2 : ctx->roi.width),
                       GEGL_ABYSS_NONE);

      /* As mentioned above, if the current segment is vertical, then the
       * water levels of the current- and source-segments are interleaved in
       * `water_buffer`.  We deinterleave the water levels into `water` and
       * `source_water`, using the yet-to-be-written-to `ground` array as a
       * temporary buffer, as necessary.
       */
      if (ctx->segment.transpose)
        {
          const gfloat *src;
          gfloat       *dest1, *dest2, *temp;
          gint          size, temp_size;
          gint          i;

          src       = ctx->water_buffer + ctx->segment.x[0];

          dest1     = ctx->water_buffer + ctx->segment.x[0];
          dest2     = ctx->water_buffer + ctx->roi.width + ctx->segment.x[0];
          temp      = ctx->ground;

          size      = ctx->segment.x[1] - ctx->segment.x[0] + 1;
          temp_size = MAX (0, 2 * size - ctx->roi.width);

          for (i = 0; i < temp_size; i++)
            {
              dest1[i] = src[2 * i];
              temp[i]  = src[2 * i + 1];
            }
          for (; i < size; i++)
            {
              dest1[i] = src[2 * i];
              dest2[i] = src[2 * i + 1];
            }

          memcpy (dest2, temp, sizeof (gfloat) * temp_size);
        }
    }
  else
    {
      /* Seed segment. */

      gint x;

      /* Set the `water` and `source_water` pointers to point to consecutive
       * rows of the `water_buffer` array.
       */
      ctx->water        = ctx->water_buffer;
      ctx->source_water = ctx->water_buffer + ctx->roi.width;

      /* Set the vertical extent of the rectangle to span a the current
       * segment's row.
       */
      iv_rect.y      = ctx->roi.y + ctx->segment.y;
      iv_rect.height = 1;

      /* Transform `iv_rect` to the image-physical coordinate system, and store
       * the result in `ip_rect`.
       */
      gimp_operation_flood_process_transform_rect (ctx, &ip_rect, &iv_rect);

      /* Read the water level of the current segment from the output GEGL
       * buffer into `water`.
       */
      gegl_buffer_get (ctx->output, &ip_rect, 1.0, ctx->output_format,
                       ctx->water + ctx->segment.x[0],
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      /* Initialize `source_water` to 0, as this is a seed segment. */
      for (x = ctx->segment.x[0]; x <= ctx->segment.x[1]; x++)
        ctx->source_water[x] = 0.0;
    }

  /* Set the vertical extent of the rectangle to span a the current segment's
   * row.
   */
  iv_rect.y      = ctx->roi.y + ctx->segment.y;
  iv_rect.height = 1;

  /* Transform `iv_rect` to the image-physical coordinate system, and store the
   * result in `ip_rect`.
   */
  gimp_operation_flood_process_transform_rect (ctx, &ip_rect, &iv_rect);

  /* Read the ground level of the current segment from the input GEGL buffer
   * into `ground`.
   */
  gegl_buffer_get (ctx->input, &ip_rect, 1.0, ctx->input_format,
                   ctx->ground + ctx->segment.x[0],
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
}

/* Performs the vertical propagation step of the algorithm.  Writes the dirty
 * ranges to the `dirty_ranges` parameter, and returns the number of dirty
 * ranges as the function's result.
 */
static gint
gimp_operation_flood_process_propagate_vertical (GimpOperationFloodContext    *ctx,
                                                 GimpOperationFloodDirtyRange *dirty_ranges)
{
  GimpOperationFloodDirtyRange *range = dirty_ranges;
  gint                          x;

  for (x = ctx->segment.x[0]; x <= ctx->segment.x[1]; x++)
    {
      /* Scan the segment until we find a pixel whose water level needs to be
       * updated.
       */
      if (ctx->source_water[x] < ctx->water[x] &&
          ctx->ground[x]       < ctx->water[x])
        {
          /* Compute and update the water level. */
          gfloat level = MAX (ctx->source_water[x], ctx->ground[x]);

          ctx->water[x] = level;

          /* Start a new dirty range at the current pixel. */
          range->x[0]     = x;
          range->modified = FALSE;

          for (x++; x <= ctx->segment.x[1]; x++)
            {
              /* Keep scanning the segment while the water level of consecutive
               * pixels needs to be updated.
               */
              if (ctx->source_water[x] < ctx->water[x] &&
                  ctx->ground[x]       < ctx->water[x])
                {
                  /* Compute and update the water level. */
                  gfloat other_level = MAX (ctx->source_water[x],
                                            ctx->ground[x]);

                  ctx->water[x] = other_level;

                  /* If the water level of the current pixel, `other_level`,
                   * equals the water level of the current dirty range,
                   * `level`, we keep scanning, making the current pixel part
                   * of the current range.  On the other hand, if the current
                   * pixel's water level is different than the that of the
                   * current range, we finalize the range, and start a new one
                   * at the current pixel.
                   */
                  if (other_level != level)
                    {
                      range->x[1]     = x - 1;
                      range++;

                      range->x[0]     = x;
                      range->modified = FALSE;
                      level           = other_level;
                    }
                }
              else
                break;
            }

          /* Finalize the current dirty range. */
          range->x[1] = x - 1;
          range++;

          /* Make sure we don't over-increment `x` on the continuation of the
           * loop.
           */
          if (x > ctx->segment.x[1])
            break;
        }
    }

  /* Return the number of dirty ranges. */
  return range - dirty_ranges;
}

/* Performs a single pass of the horizontal propagation step of the algorithm.
 * `dir` controls the direction of the pass: either +1 for a left-to-right
 * pass, or -1 for a right-to-left pass.  The dirty ranges are passed through
 * the `dirty_ranges` array (and their number in `range_count`), and are
 * modified in-place.
 */
static void
gimp_operation_flood_process_propagate_horizontal (GimpOperationFloodContext    *ctx,
                                                   gint                          dir,
                                                   GimpOperationFloodDirtyRange *dirty_ranges,
                                                   gint                          range_count)
{
  /* The index of the terminal (i.e., "`dir`-most") component of the `x[]`
   * array of `GimpOperationFloodSegment` and `GimpOperationFloodDirtyRange`,
   * based on the scan direction.  Equals 1 (i.e., the right component) when
   * `dir` is +1 (i.e., left-to-right), and equals 0 (i.e., the left component)
   * when `dir` is -1 (i.e., right-to-left).
   */
  gint          x_component;
  /* One-past the final x-coordinate of the ROI, in the ROI-virtual coordinate
   * system, based on the scan direction.  That is, the x-coordinate of the
   * pixel to the right of the rightmost pixel, for a left-to-right scan, and
   * of the pixel to the left of the leftmost pixel, for a right-to-left scan.
   */
  gint          roi_lim;
  /* One-past the final x-coordinate of the segment, in the ROI-virtual
   * coordinate system, based on the scan direction, in a similar fashion to
   * `roi_lim`.
   */
  gint          segment_lim;
  /* The indices of the first, and one-past-the-last dirty ranges, based on the
   * direction of the scan.  Recall that when scanning right-to-left, we
   * iterate over the ranges in reverse.
   */
  gint          first_range, last_range;
  /* Index of the current dirty range. */
  gint          range_index;
  /* Image-virtual and image-physical rectangles, respectively. */
  GeglRectangle iv_rect, ip_rect;

  /* Initialize the above variables based on the scan direction. */
  if (dir > 0)
    {
      /* Left-to-right. */
      x_component = 1;
      roi_lim     = ctx->roi.width;
      first_range = 0;
      last_range  = range_count;
    }
  else
    {
      /* Right-to-left. */
      x_component = 0;
      roi_lim     = -1;
      first_range = range_count - 1;
      last_range  = -1;
    }
  segment_lim     = ctx->segment.x[x_component] + dir;

  /* We loop over the dirty ranges, in the direction of the scan.  For each
   * range, we iterate over the pixels, in the scan direction, starting at the
   * outer edge of the range, and update the water level, considering only the
   * water level of the previous and current pixels, until we arrive at a pixel
   * whose water level remains the same, at which point we move to the next
   * range, as described in the algorithm overview.
   */
  for (range_index  = first_range;
       range_index != last_range;
       range_index += dir)
    {
      /* Current dirty range. */
      GimpOperationFloodDirtyRange *range;
      /* Current pixel, in the ROI-virtual coordinate system. */
      gint                          x;
      /* We use `level` to compute the water level of the current pixel.  At
       * the beginning of each iteration, it holds the water level of the
       * previous pixel.
       */
      gfloat                        level;
      /* The `inside` flag indicates whether `x` is inside the current segment.
       * Recall that we may iterate past the bounds of the current segment, in
       * which case we need to read the ground- and water-levels from the GEGL
       * buffers directly, instead of the corresponding arrays.
       */
      gboolean                      inside;
      /* Loop limit. */
      gint                          lim;

      range  = &dirty_ranges[range_index];
      /* Last x-coordinate of the range, in the direction of the scan. */
      x      = range->x[x_component];
      /* We start iterating on the pixel after `x`; initialize `level` to the
       * water level of the previous pixel.
       */
      level  = ctx->water[x];
      /* The ranges produced by the vertical propagation step are all within
       * the bounds of the segment; the horizontal propagation step may only
       * extend them in the direction of the scan.  Therefore, on both passes
       * of the horizontal propagation step, the last pixel of each range, in
       * the direction of the scan, is initially inside the segment.
       */
      inside = TRUE;
      /* If this isn't the last range, break the loop at the beginning of the
       * next range.  Otherwise, break the loop at the edge of the ROI.
       */
      if (range_index + dir != last_range)
        lim  = (range + dir)->x[1 - x_component];
      else
        lim  = roi_lim;

      /* Loop over the pixels between the edge of the current range, and the
       * beginning of the next range (or the edge of the ROI).
       */
      for (x += dir; x != lim; x += dir)
        {
          gfloat ground_level, water_level;

          /* Recall that `segment_lim` is one-past the last pixel of the
           * segment.  If we hit it, we've gone outside the segment bounds.
           */
          if (x == segment_lim)
            {
              inside         = FALSE;
              /* Initialize the rectangle to sample pixels directly from the
               * GEGL buffers.
               */
              iv_rect.y      = ctx->roi.y + ctx->segment.y;
              iv_rect.width  = 1;
              iv_rect.height = 1;
            }

          /* If we're inside the segment, read the ground- and water-levels
           * from the corresponding arrays; otherwise, read them from the GEGL
           * buffers directly.  Note that, on each pass, we may only write to
           * pixels outside the segment *in direction of the scan* (in which
           * case, the new values are written to the `water` array, but not
           * directly to the output GEGL buffer), hence, when reading from the
           * GEGL buffers, there's no danger of reading stale values, that were
           * changed on the previous pass.
           */
          if (inside)
            {
              ground_level = ctx->ground[x];
              water_level  = ctx->water[x];
            }
          else
            {
              iv_rect.x = ctx->roi.x + x;

              /* Transform `iv_rect` to the image-physical coordinate system,
               * and store the result in `ip_rect`.
               */
              gimp_operation_flood_process_transform_rect (ctx,
                                                           &ip_rect, &iv_rect);

              /* Read the current pixel's ground level. */
              gegl_buffer_get (ctx->input, &ip_rect, 1.0, ctx->input_format,
                               &ground_level,
                               GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
              /* Read the current pixel's water level. */
              gegl_buffer_get (ctx->output, &ip_rect, 1.0, ctx->output_format,
                               &water_level,
                               GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
            }

          /* The new water level is the maximum of the current ground level,
           * and the minimum of the current and previous water levels.  Recall
           * that `level` holds the previous water level, and that the current
           * water level is never less than the ground level.
           */
          if (level < ground_level)
            level             = ground_level;
          if (level < water_level)
            {
              /* The water level changed.  Update the current pixel, and set
               * the `modified` flag of the current range, since it will be
               * extended to include the current pixel.
               */
              ctx->water[x]   = level;
              range->modified = TRUE;
            }
          else
            /* The water level stayed the same.  Break the loop. */
            break;
        }

      /* Extend the current dirty range to include the last modified pixel, if
       * any.
       */
      range->x[x_component] = x - dir;

      /* If we stopped the loop before hitting the edge of the next range, or
       * if we're at the last range, continue to the next range (or quit).
       */
      if (x != lim || range_index + dir == last_range)
        continue;

      /* If we hit the edge of the next range, we keep propagating the changes
       * *inside* the next range, until we hit its other edge, or until the
       * water level stays the same.
       */
      range += dir;
      lim    = range->x[x_component] + dir;

      for (; x != lim; x += dir)
        {
          /* Note that we're necessarily inside the segment right now, since
           * the only range that could have been extended past the edge of the
           * segment by the previous pass, is the first range of the current
           * pass, while the range we're currently inside is at least the
           * second.
           */
          if (level < ctx->ground[x])
            level             = ctx->ground[x];
          if (level < ctx->water[x])
            {
              ctx->water[x]   = level;
              /* Set the `modified` flag of the range, since the water level of
               * its existing pixels changed.
               */
              range->modified = TRUE;
            }
          else
            break;
        }
    }
}

/* Coalesces consecutive dirty ranges that are separated by a gap less-than or
 * equal-to `max_gap`, in-place, and returns the new number of ranges.
 */
static gint
gimp_operation_flood_process_coalesce (const GimpOperationFloodContext *ctx,
                                       GimpOperationFloodDirtyRange    *dirty_ranges,
                                       gint                             range_count,
                                       gint                             max_gap)
{
  /* First and last ranges to coalesce, respectively. */
  const GimpOperationFloodDirtyRange *first_range, *last_range;
  /* Destination range. */
  GimpOperationFloodDirtyRange       *range = dirty_ranges;

  for (first_range  = dirty_ranges;
       first_range != dirty_ranges + range_count;
       first_range++)
    {
      /* The `modified` flag of the coalesced range -- the logical-OR of the
       * `modified` flags of the individual ranges.
       */
      gboolean modified = first_range->modified;

      /* Find all consecutive ranges with a small-enough gap. */
      for (last_range      = first_range;
           last_range + 1 != dirty_ranges + range_count;
           last_range++)
        {
          if ((last_range + 1)->x[0] - last_range->x[1] > max_gap)
            break;

          modified |= (last_range + 1)->modified;
        }

      /* Write the coalesced range, or copy the current range, to the
       * destination range.
       */
      if (first_range != last_range || first_range != range)
        {
          range->x[0]     = first_range->x[0];
          range->x[1]     = last_range->x[1];
          range->modified = modified;
        }

      first_range = last_range;
      range++;
    }

  /* Return the new range count. */
  return range - dirty_ranges;
}

/* Writes the updated water level of the dirty ranges back to the output GEGL
 * buffer.
 */
static void
gimp_operation_flood_process_commit (const GimpOperationFloodContext    *ctx,
                                     const GimpOperationFloodDirtyRange *dirty_ranges,
                                     gint                                range_count)
{
  const GimpOperationFloodDirtyRange *range;
  /* Image-virtual and image-physical rectangles, respectively. */
  GeglRectangle                       iv_rect, ip_rect;

  /* Set the vertical extent of the rectangle to span a the current segment's
   * row.
   */
  iv_rect.y      = ctx->roi.y + ctx->segment.y;
  iv_rect.height = 1;

  for (range = dirty_ranges; range != dirty_ranges + range_count; range++)
    {
      /* Set the horizontal extent of the rectangle to span the dirty range. */
      iv_rect.x     = ctx->roi.x + range->x[0];
      iv_rect.width = range->x[1] - range->x[0] + 1;

      /* Transform `iv_rect` to the image-physical coordinate system, and store
       * the result in `ip_rect`.
       */
      gimp_operation_flood_process_transform_rect (ctx, &ip_rect, &iv_rect);

      /* Write the updated water level to the output GEGL buffer. */
      gegl_buffer_set (ctx->output, &ip_rect, 0, ctx->output_format,
                       ctx->water + range->x[0],
                       GEGL_AUTO_ROWSTRIDE);
    }
}

/* Pushes the new segments, corresponding to the dirty ranges of the current
 * segment, into the queue.
 */
static void
gimp_operation_flood_process_distribute (const GimpOperationFloodContext    *ctx,
                                         GQueue                             *queue,
                                         const GimpOperationFloodDirtyRange *dirty_ranges,
                                         gint                                range_count)
{
  const GimpOperationFloodDirtyRange *range;
  static const gint                   y_deltas[] = {-1, +1};
  gint                                i;

  /* For each neighboring row... */
  for (i = 0; i < G_N_ELEMENTS (y_deltas); i++)
    {
      /* The difference between the negihboring row's y-coordinate and the
       * current row's y-corindate, in the ROI-virtual coordinate system.
       */
      gint y_delta = y_deltas[i];
      /* The negihboring row's y-coordinate in the ROI-virtual coordinate
       * system.
       */
      gint y = ctx->segment.y + y_delta;

      /* If the neighboring row is outside the ROI, skip it. */
      if (y < 0 || y >= ctx->roi.height)
        continue;

      /* For each dirty range... */
      for (range = dirty_ranges; range != dirty_ranges + range_count; range++)
        {
          /* If the range was modified during horizontal propagation, or if the
           * neighboring row is not the source segment's row... (note that the
           * latter is always true for seed segments.)
           */
          if (range->modified || y_delta != ctx->segment.source_y_delta)
            {
              /* Push a new segment into the queue, spanning the same pixels as
               * the dirty range on the neighboring row, using the current row
               * as its source segment.
               */
              gimp_operation_flood_process_push (queue,
                                                 ctx->segment.transpose,
                                                 y,
                                                 -y_delta,
                                                 range->x[0],
                                                 range->x[1]);
            }
        }
    }
}

/* Main algorithm. */
static gboolean
gimp_operation_flood_process (GeglOperation       *operation,
                              GeglBuffer          *input,
                              GeglBuffer          *output,
                              const GeglRectangle *roi,
                              gint                 level)
{
  const Babl                   *input_format  = gegl_operation_get_format (operation, "input");
  const Babl                   *output_format = gegl_operation_get_format (operation, "output");
  GeglColor                    *color;
  gint                          max_size;
  GimpOperationFloodContext     ctx;
  GimpOperationFloodDirtyRange *dirty_ranges;
  GQueue                       *queue;

  /* Make sure the input- and output-buffers are different. */
  g_return_val_if_fail (input != output, FALSE);

  /* Make sure the ROI is small enough for the `GimpOperationFloodSegment::y`
   * field.
   */
  g_return_val_if_fail (roi->width  <= GIMP_MAX_IMAGE_SIZE &&
                        roi->height <= GIMP_MAX_IMAGE_SIZE, FALSE);

  ctx.input         = input;
  ctx.input_format  = input_format;
  ctx.output        = output;
  ctx.output_format = output_format;

  /* All buffers need to have enough capacity to process a full row, or a full
   * column, since, when processing vertical segments, we treat the image as
   * transposed.
   */
  max_size          = MAX (roi->width, roi->height);
  ctx.ground        = g_new (gfloat, max_size);
  /* The `water_buffer` array needs to be able to hold two rows (or columns). */
  ctx.water_buffer  = g_new (gfloat, 2 * max_size);
  dirty_ranges      = g_new (GimpOperationFloodDirtyRange, max_size);

  /* Initialize the water level to 1 everywhere. */
  color = gegl_color_new ("#fff");
  gegl_buffer_set_color (output, roi, color);
  g_object_unref (color);

  /* Create the queue and push the seed segments. */
  queue  = g_queue_new ();
  gimp_operation_flood_process_seed (queue, roi);

  /* While there are segments to process in the queue... */
  while (! g_queue_is_empty (queue))
    {
      GimpOperationFloodSegment *segment;
      gint                       range_count;

      /* Pop a segment off the top of the queue, copy it to `ctx.segment`, and
       * free its memory.
       */
      segment     = (GimpOperationFloodSegment *) g_queue_pop_head (queue);
      ctx.segment = *segment;
      g_slice_free (GimpOperationFloodSegment, segment);

      /* Transform the ROI from the image-physical coordinate system to the
       * image-virtual coordinate system, and store the result in `ctx.roi`.
       */
      gimp_operation_flood_process_transform_rect (&ctx, &ctx.roi, roi);

      /* Read the ground- and water-levels of the current- and source-segments
       * from the corresponding GEGL buffers to the corresponding arrays.
       */
      gimp_operation_flood_process_fetch (&ctx);

      /* Perform the vertical propagation step. */
      range_count = gimp_operation_flood_process_propagate_vertical (&ctx,
                                                                     dirty_ranges);
      /* If no dirty ranges were produced during vertical propagation, then the
       * water level of the current segment didn't change, and we can short-
       * circuit early.
       */
      if (range_count == 0)
        continue;

      /* Perform both passes of the horizontal propagation step. */
      gimp_operation_flood_process_propagate_horizontal (&ctx,
                                                         /* Left-to-right */ +1,
                                                         dirty_ranges,
                                                         range_count);
      gimp_operation_flood_process_propagate_horizontal (&ctx,
                                                         /* Right-to-left */ -1,
                                                         dirty_ranges,
                                                         range_count);

      /* Coalesce consecutive dirty ranges separated by a gap less-than or
       * equal-to `GIMP_OPERATION_FLOOD_COALESCE_MAX_GAP`.
       */
      range_count = gimp_operation_flood_process_coalesce (&ctx,
                                                           dirty_ranges,
                                                           range_count,
                                                           GIMP_OPERATION_FLOOD_COALESCE_MAX_GAP);

      /* Write the updated water level back to the output GEGL buffer. */
      gimp_operation_flood_process_commit (&ctx, dirty_ranges, range_count);

      /* Push the new segments into the queue. */
      gimp_operation_flood_process_distribute (&ctx, queue,
                                               dirty_ranges, range_count);
    }

  g_queue_free (queue);

  g_free (dirty_ranges);
  g_free (ctx.water_buffer);
  g_free (ctx.ground);

  return TRUE;
}
