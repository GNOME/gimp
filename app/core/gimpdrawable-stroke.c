/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdrawable-stroke.c
 * Copyright (C) 2003 Simon Budig  <simon@gimp.org>
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

#include <glib-object.h>

#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "gimp.h"
#include "gimpdrawable.h"
#include "gimpdrawable-stroke.h"
#include "gimpimage.h"
#include "gimppattern.h"
#include "gimpscanconvert.h"
#include "gimpstrokeoptions.h"
#include "gimpunit.h"

#include "vectors/gimpstroke.h"
#include "vectors/gimpvectors.h"

#include "gimp-intl.h"


/*  local function prototypes  */

void
gimp_drawable_stroke_vectors (GimpDrawable      *drawable,
                              GimpVectors       *vectors,
                              GimpStrokeOptions *options)
{
  /* Stroke options */
  gdouble               opacity;
  GimpLayerModeEffects  paint_mode;
  GimpStrokeStyle       style;
  gdouble               width;
  GimpJoinStyle         join;
  GimpCapStyle          cap;
  gdouble               miter;
  gboolean              antialias;
  GArray               *dash_array = NULL;

  gint             num_coords = 0;
  GimpStroke      *stroke;

  GimpImage       *gimage;
  GimpScanConvert *scan_convert;
  TileManager     *base;
  TileManager     *mask;
  gint             x1, x2, y1, y2, bytes, w, h;
  guchar           bg[1] = { 0, };
  PixelRegion      maskPR, basePR;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (GIMP_IS_VECTORS (vectors));
  g_return_if_fail (GIMP_IS_STROKE_OPTIONS (options));

  g_object_get (options,
                "opacity",    &opacity,
                "paint-mode", &paint_mode,
                "style",      &style,
                "width",      &width,
                "join-style", &join,
                "cap-style",  &cap,
                "miter",      &miter,
                "antialias",  &antialias,
                NULL);

  gimage = gimp_item_get_image (GIMP_ITEM (drawable));

  if (options->width_unit != GIMP_UNIT_PIXEL)
    {
      width = (width *
               _gimp_unit_get_factor (gimage->gimp,
                                      options->width_unit) *
               (gimage->xresolution + gimage->yresolution) / 2);
    }

  /* what area do we operate on? */
  gimp_drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

  w = x2 - x1;
  h = y2 - y1;

  gimp_item_offsets (GIMP_ITEM (drawable), &x2, &y2);

  scan_convert = gimp_scan_convert_new (w, h, antialias);

  /* For each Stroke in the vector, interpolate it, and add it to the
   * ScanConvert
   */
  for (stroke = gimp_vectors_stroke_get_next (vectors, NULL);
       stroke;
       stroke = gimp_vectors_stroke_get_next (vectors, stroke))
    {
      GimpVector2 *points;
      gboolean     closed;
      GArray      *coords;
      gint i;

      /* Get the interpolated version of this stroke, and add it to our
       * scanconvert.
       */
      coords = gimp_stroke_interpolate (stroke, 0.2, &closed);

      if (coords && coords->len)
        {
          points = g_new0 (GimpVector2, coords->len);

          for (i = 0; i < coords->len; i++)
            {
              points[i].x = g_array_index (coords, GimpCoords, i).x;
              points[i].y = g_array_index (coords, GimpCoords, i).y;
              num_coords++;
            }

          gimp_scan_convert_add_polyline (scan_convert, coords->len,
                                          points, closed);

          g_free (points);
        }
    }

  if (num_coords == 0)
    {
      gimp_scan_convert_free (scan_convert);
      return;
    }

  gimp_scan_convert_stroke (scan_convert, width, join, cap, miter,
                            0.0, dash_array);

  /* fill a 1-bpp Tilemanager with black, this will describe the shape
   * of the stroke.
   */
  mask = tile_manager_new (w, h, 1);
  tile_manager_set_offsets (mask, x1+x2, y1+y2);
  pixel_region_init (&maskPR, mask, 0, 0, w, h, TRUE);
  color_region (&maskPR, bg);

  /* render the stroke into it */
  gimp_scan_convert_render (scan_convert, mask);

  gimp_scan_convert_free (scan_convert);

  bytes = drawable->bytes;
  if (!gimp_drawable_has_alpha (drawable))
    bytes++;

  base = tile_manager_new (w, h, bytes);
  tile_manager_set_offsets (base, x1 + x2, y1 + y2);
  pixel_region_init (&basePR, base, 0, 0, w, h, TRUE);
  pixel_region_init (&maskPR, mask, 0, 0, w, h, FALSE);

  switch (style)
    {
    case GIMP_STROKE_STYLE_SOLID:
      {
        GimpRGB *color;
        guchar   tmp_col[MAX_CHANNELS] = { 0, };
        guchar   col[MAX_CHANNELS]     = { 0, };

        g_object_get (options, "foreground", &color, NULL);
        gimp_rgb_get_uchar (color,
                            &tmp_col[RED_PIX],
                            &tmp_col[GREEN_PIX],
                            &tmp_col[BLUE_PIX]);
        g_free (color);

        gimp_image_transform_color (GIMP_ITEM (drawable)->gimage, drawable,
                                    col, GIMP_RGB, tmp_col);
        col[bytes - 1] = OPAQUE_OPACITY;

        color_region_mask (&basePR, &maskPR, col);
      }
      break;

    case GIMP_STROKE_STYLE_PATTERN:
      {
        GimpPattern *pattern;
        TempBuf     *pat_buf;
        gboolean     new_buf;

        g_object_get (options, "pattern", &pattern, NULL);
        pat_buf = gimp_image_transform_temp_buf (gimage, drawable,
                                                 pattern->mask, &new_buf);
        g_object_unref (pattern);

        pattern_region (&basePR, &maskPR, pat_buf, x1, y1);

        if (new_buf)
          temp_buf_free (pat_buf);
      }
      break;
    }

  /* Apply to drawable */
  pixel_region_init (&basePR, base, 0, 0, w, h, FALSE);
  gimp_image_apply_image (gimp_item_get_image (GIMP_ITEM (drawable)),
                          drawable, &basePR,
                          TRUE, _("Render Stroke"), opacity, paint_mode,
                          NULL, x1, y1);

  tile_manager_unref (mask);
  tile_manager_unref (base);

  gimp_drawable_update (drawable, x1, y1, w, h);
}


