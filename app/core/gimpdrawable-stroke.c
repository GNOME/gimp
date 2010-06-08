/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdrawable-stroke.c
 * Copyright (C) 2003 Simon Budig  <simon@gimp.org>
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

#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "base/boundary.h"
#include "base/pixel-region.h"
#include "base/temp-buf.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpcontext.h"
#include "gimpdrawable-stroke.h"
#include "gimpimage.h"
#include "gimppattern.h"
#include "gimpscanconvert.h"
#include "gimpstrokeoptions.h"

#include "vectors/gimpstroke.h"
#include "vectors/gimpvectors.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static GimpScanConvert * gimp_drawable_render_boundary     (GimpDrawable    *drawable,
                                                            const BoundSeg  *bound_segs,
                                                            gint             n_bound_segs,
                                                            gint             offset_x,
                                                            gint             offset_y);
static GimpScanConvert * gimp_drawable_render_vectors      (GimpDrawable    *drawable,
                                                            GimpVectors     *vectors,
                                                            gboolean         do_stroke);
static void              gimp_drawable_stroke_scan_convert (GimpDrawable    *drawable,
                                                            GimpFillOptions *options,
                                                            GimpScanConvert *scan_convert,
                                                            gboolean         do_stroke,
                                                            gboolean         push_undo);


/*  public functions  */

void
gimp_drawable_fill_boundary (GimpDrawable    *drawable,
                             GimpFillOptions *options,
                             const BoundSeg  *bound_segs,
                             gint             n_bound_segs,
                             gint             offset_x,
                             gint             offset_y,
                             gboolean         push_undo)
{
  GimpScanConvert *scan_convert;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (GIMP_IS_STROKE_OPTIONS (options));
  g_return_if_fail (bound_segs == NULL || n_bound_segs != 0);
  g_return_if_fail (options->style != GIMP_FILL_STYLE_PATTERN ||
                    gimp_context_get_pattern (GIMP_CONTEXT (options)) != NULL);

  scan_convert = gimp_drawable_render_boundary (drawable,
                                                bound_segs, n_bound_segs,
                                                offset_x, offset_y);

  if (scan_convert)
    {
      gimp_drawable_stroke_scan_convert (drawable, options,
                                         scan_convert, FALSE, push_undo);
      gimp_scan_convert_free (scan_convert);
    }
}

void
gimp_drawable_stroke_boundary (GimpDrawable      *drawable,
                               GimpStrokeOptions *options,
                               const BoundSeg    *bound_segs,
                               gint               n_bound_segs,
                               gint               offset_x,
                               gint               offset_y,
                               gboolean           push_undo)
{
  GimpScanConvert *scan_convert;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (GIMP_IS_STROKE_OPTIONS (options));
  g_return_if_fail (bound_segs == NULL || n_bound_segs != 0);
  g_return_if_fail (GIMP_FILL_OPTIONS (options)->style != GIMP_FILL_STYLE_PATTERN ||
                    gimp_context_get_pattern (GIMP_CONTEXT (options)) != NULL);

  scan_convert = gimp_drawable_render_boundary (drawable,
                                                bound_segs, n_bound_segs,
                                                offset_x, offset_y);

  if (scan_convert)
    {
      gimp_drawable_stroke_scan_convert (drawable, GIMP_FILL_OPTIONS (options),
                                         scan_convert, TRUE, push_undo);
      gimp_scan_convert_free (scan_convert);
    }
}

void
gimp_drawable_fill_vectors (GimpDrawable    *drawable,
                            GimpFillOptions *options,
                            GimpVectors     *vectors,
                            gboolean         push_undo)
{
  GimpScanConvert *scan_convert;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (GIMP_IS_FILL_OPTIONS (options));
  g_return_if_fail (GIMP_IS_VECTORS (vectors));
  g_return_if_fail (options->style != GIMP_FILL_STYLE_PATTERN ||
                    gimp_context_get_pattern (GIMP_CONTEXT (options)) != NULL);

  scan_convert = gimp_drawable_render_vectors (drawable, vectors, FALSE);

  if (scan_convert)
    {
      gimp_drawable_stroke_scan_convert (drawable, options,
                                         scan_convert, FALSE, push_undo);
      gimp_scan_convert_free (scan_convert);
    }
}

void
gimp_drawable_stroke_vectors (GimpDrawable      *drawable,
                              GimpStrokeOptions *options,
                              GimpVectors       *vectors,
                              gboolean           push_undo)
{
  GimpScanConvert *scan_convert;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (GIMP_IS_STROKE_OPTIONS (options));
  g_return_if_fail (GIMP_IS_VECTORS (vectors));
  g_return_if_fail (GIMP_FILL_OPTIONS (options)->style != GIMP_FILL_STYLE_PATTERN ||
                    gimp_context_get_pattern (GIMP_CONTEXT (options)) != NULL);

  scan_convert = gimp_drawable_render_vectors (drawable, vectors, TRUE);

  if (scan_convert)
    {
      gimp_drawable_stroke_scan_convert (drawable, GIMP_FILL_OPTIONS (options),
                                         scan_convert, TRUE, push_undo);
      gimp_scan_convert_free (scan_convert);
    }
}


/*  private functions  */

static GimpScanConvert *
gimp_drawable_render_boundary (GimpDrawable    *drawable,
                               const BoundSeg  *bound_segs,
                               gint             n_bound_segs,
                               gint             offset_x,
                               gint             offset_y)
{
  GimpScanConvert *scan_convert;
  BoundSeg        *stroke_segs;
  gint             n_stroke_segs;
  GimpVector2     *points;
  gint             n_points;
  gint             seg;
  gint             i;

  if (n_bound_segs == 0)
    return NULL;

  stroke_segs = boundary_sort (bound_segs, n_bound_segs, &n_stroke_segs);

  if (n_stroke_segs == 0)
    return NULL;

  scan_convert = gimp_scan_convert_new ();

  points = g_new0 (GimpVector2, n_bound_segs + 4);

  seg = 0;
  n_points = 0;

  points[n_points].x = (gdouble) (stroke_segs[0].x1 + offset_x);
  points[n_points].y = (gdouble) (stroke_segs[0].y1 + offset_y);

  n_points++;

  for (i = 0; i < n_stroke_segs; i++)
    {
      while (stroke_segs[seg].x1 != -1 ||
             stroke_segs[seg].x2 != -1 ||
             stroke_segs[seg].y1 != -1 ||
             stroke_segs[seg].y2 != -1)
        {
          points[n_points].x = (gdouble) (stroke_segs[seg].x1 + offset_x);
          points[n_points].y = (gdouble) (stroke_segs[seg].y1 + offset_y);

          n_points++;
          seg++;
        }

      /* Close the stroke points up */
      points[n_points] = points[0];

      n_points++;

      gimp_scan_convert_add_polyline (scan_convert, n_points, points, TRUE);

      n_points = 0;
      seg++;

      points[n_points].x = (gdouble) (stroke_segs[seg].x1 + offset_x);
      points[n_points].y = (gdouble) (stroke_segs[seg].y1 + offset_y);

      n_points++;
    }

  g_free (points);
  g_free (stroke_segs);

  return scan_convert;
}

static GimpScanConvert *
gimp_drawable_render_vectors (GimpDrawable *drawable,
                              GimpVectors  *vectors,
                              gboolean      do_stroke)
{
  GimpScanConvert *scan_convert;
  GimpStroke      *stroke;
  gint             num_coords = 0;

  scan_convert = gimp_scan_convert_new ();

  /* For each Stroke in the vector, interpolate it, and add it to the
   * ScanConvert
   */
  for (stroke = gimp_vectors_stroke_get_next (vectors, NULL);
       stroke;
       stroke = gimp_vectors_stroke_get_next (vectors, stroke))
    {
      GArray   *coords;
      gboolean  closed;

      /* Get the interpolated version of this stroke, and add it to our
       * scanconvert.
       */
      coords = gimp_stroke_interpolate (stroke, 0.2, &closed);

      if (coords && coords->len)
        {
          GimpVector2 *points = g_new0 (GimpVector2, coords->len);
          gint         i;

          for (i = 0; i < coords->len; i++)
            {
              points[i].x = g_array_index (coords, GimpCoords, i).x;
              points[i].y = g_array_index (coords, GimpCoords, i).y;
              num_coords++;
            }

          gimp_scan_convert_add_polyline (scan_convert, coords->len,
                                          points, closed || !do_stroke);

          g_free (points);
        }

      if (coords)
        g_array_free (coords, TRUE);
    }

  if (num_coords > 0)
    return scan_convert;

  gimp_scan_convert_free (scan_convert);

  return NULL;
}

static void
gimp_drawable_stroke_scan_convert (GimpDrawable    *drawable,
                                   GimpFillOptions *options,
                                   GimpScanConvert *scan_convert,
                                   gboolean         do_stroke,
                                   gboolean         push_undo)
{
  GimpContext *context = GIMP_CONTEXT (options);
  GimpImage   *image   = gimp_item_get_image (GIMP_ITEM (drawable));
  TileManager *base;
  TileManager *mask;
  gint         x, y, w, h;
  gint         bytes;
  gint         off_x;
  gint         off_y;
  guchar       bg[1] = { 0, };
  PixelRegion  maskPR;
  PixelRegion  basePR;

  /*  must call gimp_channel_is_empty() instead of relying on
   *  gimp_drawable_mask_intersect() because the selection pretends to
   *  be empty while it is being stroked, to prevent masking itself.
   */
  if (gimp_channel_is_empty (gimp_image_get_mask (image)))
    {
      x = 0;
      y = 0;
      w = gimp_item_get_width  (GIMP_ITEM (drawable));
      h = gimp_item_get_height (GIMP_ITEM (drawable));
    }
  else if (! gimp_drawable_mask_intersect (drawable, &x, &y, &w, &h))
    {
      return;
    }

  if (do_stroke)
    {
      GimpStrokeOptions *stroke_options = GIMP_STROKE_OPTIONS (options);
      gdouble            width          = stroke_options->width;

      if (stroke_options->unit != GIMP_UNIT_PIXEL)
        {
          gdouble xres;
          gdouble yres;

          gimp_image_get_resolution (image, &xres, &yres);

          gimp_scan_convert_set_pixel_ratio (scan_convert, yres / xres);

          width = gimp_units_to_pixels (stroke_options->width,
                                        stroke_options->unit, yres);
        }

      gimp_scan_convert_stroke (scan_convert, width,
                                stroke_options->join_style,
                                stroke_options->cap_style,
                                stroke_options->miter_limit,
                                stroke_options->dash_offset,
                                stroke_options->dash_info);
    }

  /* fill a 1-bpp Tilemanager with black, this will describe the shape
   * of the stroke.
   */
  mask = tile_manager_new (w, h, 1);
  pixel_region_init (&maskPR, mask, 0, 0, w, h, TRUE);
  color_region (&maskPR, bg);

  /* render the stroke into it */
  gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

  gimp_scan_convert_render (scan_convert, mask,
                            x + off_x, y + off_y,
                            GIMP_FILL_OPTIONS (options)->antialias);

  bytes = gimp_drawable_bytes_with_alpha (drawable);

  base = tile_manager_new (w, h, bytes);
  pixel_region_init (&basePR, base, 0, 0, w, h, TRUE);
  pixel_region_init (&maskPR, mask, 0, 0, w, h, FALSE);

  switch (options->style)
    {
    case GIMP_FILL_STYLE_SOLID:
      {
        guchar tmp_col[MAX_CHANNELS] = { 0, };
        guchar col[MAX_CHANNELS]     = { 0, };

        gimp_rgb_get_uchar (&context->foreground,
                            &tmp_col[RED],
                            &tmp_col[GREEN],
                            &tmp_col[BLUE]);

        gimp_image_transform_color (image, gimp_drawable_type (drawable), col,
                                    GIMP_RGB, tmp_col);
        col[bytes - 1] = OPAQUE_OPACITY;

        color_region_mask (&basePR, &maskPR, col);
      }
      break;

    case GIMP_FILL_STYLE_PATTERN:
      {
        GimpPattern *pattern;
        TempBuf     *pat_buf;
        gboolean     new_buf;

        pattern = gimp_context_get_pattern (context);
        pat_buf = gimp_image_transform_temp_buf (image,
                                                 gimp_drawable_type (drawable),
                                                 pattern->mask, &new_buf);

        pattern_region (&basePR, &maskPR, pat_buf, x, y);

        if (new_buf)
          temp_buf_free (pat_buf);
      }
      break;
    }

  /* Apply to drawable */
  pixel_region_init (&basePR, base, 0, 0, w, h, FALSE);
  gimp_drawable_apply_region (drawable, &basePR,
                              push_undo, C_("undo-type", "Render Stroke"),
                              gimp_context_get_opacity (context),
                              gimp_context_get_paint_mode (context),
                              NULL, NULL, x, y);

  tile_manager_unref (mask);
  tile_manager_unref (base);

  gimp_drawable_update (drawable, x, y, w, h);
}
