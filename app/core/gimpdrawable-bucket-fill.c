/* The GIMP -- an image manipulation program
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

#include <glib-object.h>

#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpcontext.h"
#include "gimpdrawable.h"
#include "gimpdrawable-bucket-fill.h"
#include "gimpimage.h"
#include "gimpimage-contiguous-region.h"
#include "gimpimage-mask.h"
#include "gimppattern.h"

#include "libgimp/gimpintl.h"


/*  local function prototypes  */

static void   gimp_drawable_bucket_fill_region   (GimpBucketFillMode  fill_mode,
                                                      PixelRegion     *bufPR,
                                                      PixelRegion     *maskPR,
                                                      guchar          *col,
                                                      TempBuf         *pattern,
                                                      gint             off_x,
                                                      gint             off_y,
                                                      gboolean         has_alpha);
static void   gimp_drawable_bucket_fill_line_color   (guchar          *buf,
                                                      guchar          *mask,
                                                      guchar          *col,
                                                      gboolean         has_alpha,
                                                      gint             bytes,
                                                      gint             width);
static void   gimp_drawable_bucket_fill_line_pattern (guchar          *buf,
                                                      guchar          *mask,
                                                      TempBuf         *pattern,
                                                      gboolean         has_alpha,
                                                      gint             bytes,
                                                      gint             x,
                                                      gint             y,
                                                      gint             width);


/*  public functions  */

void
gimp_drawable_bucket_fill (GimpDrawable       *drawable,
                           GimpBucketFillMode  fill_mode,
                           gint                paint_mode,
                           gdouble             opacity,
                           gboolean            do_seed_fill,
                           gboolean            fill_transparent,
                           gdouble             threshold,
                           gboolean            sample_merged,
                           gdouble             x,
                           gdouble             y)
{
  GimpImage   *gimage;
  GimpRGB      color;
  GimpPattern *pattern = NULL;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  gimage = gimp_item_get_image (GIMP_ITEM (drawable));

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  if (fill_mode == GIMP_FG_BUCKET_FILL)
    {
      gimp_context_get_foreground (gimp_get_current_context (gimage->gimp),
                                   &color);
    }
  else if (fill_mode == GIMP_BG_BUCKET_FILL)
    {
      gimp_context_get_background (gimp_get_current_context (gimage->gimp),
                                   &color);
    }
  else if (fill_mode == GIMP_PATTERN_BUCKET_FILL)
    {
      pattern =
        gimp_context_get_pattern (gimp_get_current_context (gimage->gimp));

      if (! pattern)
	{
	  g_message (_("No patterns available for this operation."));
	  return;
	}
    }
  else
    {
      g_warning ("%s: invalid fill_mode passed", G_GNUC_PRETTY_FUNCTION);
      return;
    }

  gimp_drawable_bucket_fill_full (drawable,
                                  fill_mode,
                                  paint_mode, opacity,
                                  do_seed_fill,
                                  fill_transparent,
                                  threshold, sample_merged,
                                  x, y,
                                  &color, pattern);
}

void
gimp_drawable_bucket_fill_full (GimpDrawable       *drawable,
                                GimpBucketFillMode  fill_mode,
                                gint                paint_mode,
                                gdouble             opacity,
                                gboolean            do_seed_fill,
                                gboolean            fill_transparent,
                                gdouble             threshold,
                                gboolean            sample_merged,
                                gdouble             x,
                                gdouble             y,
                                const GimpRGB      *color,
                                GimpPattern        *pattern)
{
  GimpImage   *gimage;
  TileManager *buf_tiles;
  PixelRegion  bufPR, maskPR;
  GimpChannel *mask = NULL;
  gint         bytes;
  gboolean     has_alpha;
  gint         x1, y1, x2, y2;
  guchar       col[MAX_CHANNELS];
  TempBuf     *pat_buf = NULL;
  gboolean     new_buf = FALSE;
  gboolean     selection;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (fill_mode != GIMP_PATTERN_BUCKET_FILL ||
                    GIMP_IS_PATTERN (pattern));
  g_return_if_fail ((fill_mode != GIMP_FG_BUCKET_FILL &&
                     fill_mode != GIMP_BG_BUCKET_FILL) || color != NULL);

  gimage = gimp_item_get_image (GIMP_ITEM (drawable));

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  if (fill_mode == GIMP_FG_BUCKET_FILL ||
      fill_mode == GIMP_BG_BUCKET_FILL)
    {
      guchar tmp_col[MAX_CHANNELS];

      gimp_rgb_get_uchar (color, &tmp_col[0], &tmp_col[1], &tmp_col[2]);

      gimp_image_transform_color (gimage, drawable, tmp_col, col, GIMP_RGB);
    }
  else if (fill_mode == GIMP_PATTERN_BUCKET_FILL)
    {
      /*  If the pattern doesn't match the image in terms of color type,
       *  transform it.  (ie  pattern is RGB, image is indexed)
       */
      if (((pattern->mask->bytes == 3) && ! gimp_drawable_is_rgb  (drawable)) ||
	  ((pattern->mask->bytes == 1) && ! gimp_drawable_is_gray (drawable)))
	{
          guchar *d1, *d2;
	  gint    size;

	  if ((pattern->mask->bytes == 1) && gimp_drawable_is_rgb (drawable))
	    pat_buf = temp_buf_new (pattern->mask->width,
                                    pattern->mask->height,
				    3, 0, 0, NULL);
	  else
	    pat_buf = temp_buf_new (pattern->mask->width,
                                    pattern->mask->height,
				    1, 0, 0, NULL);

	  d1 = temp_buf_data (pattern->mask);
	  d2 = temp_buf_data (pat_buf);

	  size = pattern->mask->width * pattern->mask->height;
	  while (size--)
	    {
	      gimp_image_transform_color (gimage, drawable, d1, d2,
					  (pattern->mask->bytes == 3) ? 
                                          GIMP_RGB : GIMP_GRAY);
	      d1 += pattern->mask->bytes;
	      d2 += pat_buf->bytes;
	    }

	  new_buf = TRUE;
	}
      else
        {
          pat_buf = pattern->mask;
        }
    }
  else
    {
      g_warning ("%s: invalid fill_mode passed", G_GNUC_PRETTY_FUNCTION);
      return;
    }

  gimp_set_busy (gimage->gimp);

  bytes     = gimp_drawable_bytes (drawable);
  has_alpha = gimp_drawable_has_alpha (drawable);
  selection = gimp_drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

  /*  Do a seed bucket fill...To do this, calculate a new 
   *  contiguous region. If there is a selection, calculate the 
   *  intersection of this region with the existing selection.
   */
  if (do_seed_fill)
    {
      mask = gimp_image_contiguous_region_by_seed (gimage, drawable,
                                                   sample_merged,
                                                   TRUE,
                                                   (gint) threshold,
                                                   fill_transparent,
                                                   (gint) x,
                                                   (gint) y);

      if (selection)
        {
          gint off_x = 0;
          gint off_y = 0;

          if (! sample_merged)
            gimp_drawable_offsets (drawable, &off_x, &off_y);

          gimp_channel_combine_mask (mask, gimp_image_get_mask (gimage),
                                     GIMP_CHANNEL_OP_INTERSECT,
                                     -off_x, -off_y);
        }

      gimp_channel_bounds (mask, &x1, &y1, &x2, &y2);

      /*  make sure we handle the mask correctly if it was sample-merged  */
      if (sample_merged)
	{
	  gint off_x, off_y;

	  /*  Limit the channel bounds to the drawable's extents  */
	  gimp_drawable_offsets (drawable, &off_x, &off_y);
	  x1 = CLAMP (x1, off_x, (off_x + gimp_drawable_width (drawable)));
	  y1 = CLAMP (y1, off_y, (off_y + gimp_drawable_height (drawable)));
	  x2 = CLAMP (x2, off_x, (off_x + gimp_drawable_width (drawable)));
	  y2 = CLAMP (y2, off_y, (off_y + gimp_drawable_height (drawable)));

	  pixel_region_init (&maskPR, gimp_drawable_data (GIMP_DRAWABLE (mask)), 
			     x1, y1, (x2 - x1), (y2 - y1), TRUE);

	  /*  translate mask bounds to drawable coords  */
	  x1 -= off_x;
	  y1 -= off_y;
	  x2 -= off_x;
	  y2 -= off_y;
	}
      else
        {
          pixel_region_init (&maskPR, gimp_drawable_data (GIMP_DRAWABLE (mask)), 
                             x1, y1, (x2 - x1), (y2 - y1), TRUE);
        }

      /*  if the gimage doesn't have an alpha channel,
       *  make sure that the temp buf does.  We need the
       *  alpha channel to fill with the region calculated above
       */
      if (! has_alpha)
	{
	  bytes ++;
	  has_alpha = TRUE;
	}
    }

  buf_tiles = tile_manager_new ((x2 - x1), (y2 - y1), bytes);
  pixel_region_init (&bufPR, buf_tiles, 0, 0, (x2 - x1), (y2 - y1), TRUE);

  if (mask)
    gimp_drawable_bucket_fill_region (fill_mode,
                                      &bufPR, &maskPR,
                                      col, pat_buf,
                                      x1, y1,
                                      has_alpha);
  else
    gimp_drawable_bucket_fill_region (fill_mode,
                                      &bufPR, NULL,
                                      col, pat_buf,
                                      x1, y1,
                                      has_alpha);

  /*  Apply it to the image  */
  pixel_region_init (&bufPR, buf_tiles, 0, 0, (x2 - x1), (y2 - y1), FALSE);
  gimp_image_apply_image (gimage, drawable, &bufPR,
                          TRUE, _("Bucket Fill"),
			  opacity, paint_mode,
                          NULL, x1, y1);
  tile_manager_destroy (buf_tiles);

  /*  update the image  */
  gimp_drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));

  /*  free the mask  */
  if (mask)
    g_object_unref (mask);

  if (new_buf)
    temp_buf_free (pat_buf);

  gimp_unset_busy (gimage->gimp);
}


/*  private functions  */

static void
gimp_drawable_bucket_fill_region (GimpBucketFillMode  fill_mode,
                                  PixelRegion        *bufPR,
                                  PixelRegion        *maskPR,
                                  guchar             *col,
                                  TempBuf            *pattern,
                                  gint                off_x,
                                  gint                off_y,
                                  gboolean            has_alpha)
{
  guchar *s, *m;
  gint    y;
  void   *pr;

  for (pr = pixel_regions_register (2, bufPR, maskPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      s = bufPR->data;
      if (maskPR)
	m = maskPR->data;
      else
	m = NULL;

      for (y = 0; y < bufPR->h; y++)
	{
	  switch (fill_mode)
	    {
	    case GIMP_FG_BUCKET_FILL:
	    case GIMP_BG_BUCKET_FILL:
	      gimp_drawable_bucket_fill_line_color (s, m,
                                                    col,
                                                    has_alpha,
                                                    bufPR->bytes, bufPR->w);
	      break;
	    case GIMP_PATTERN_BUCKET_FILL:
	      gimp_drawable_bucket_fill_line_pattern (s, m,
                                                      pattern,
                                                      has_alpha,
                                                      bufPR->bytes,
                                                      off_x + bufPR->x,
                                                      off_y + y + bufPR->y,
                                                      bufPR->w);
	      break;
	    }
	  s += bufPR->rowstride;

	  if (maskPR)
	    m += maskPR->rowstride;
	}
    }
}

static void
gimp_drawable_bucket_fill_line_color (guchar   *buf,
                                      guchar   *mask,
                                      guchar   *col,
                                      gboolean  has_alpha,
                                      gint      bytes,
                                      gint      width)
{
  gint alpha, b;

  alpha = (has_alpha) ? bytes - 1 : bytes;
  while (width--)
    {
      for (b = 0; b < alpha; b++)
	buf[b] = col[b];

      if (has_alpha)
	{
	  if (mask)
	    buf[alpha] = *mask++;
	  else
	    buf[alpha] = OPAQUE_OPACITY;
	}

      buf += bytes;
    }
}

static void
gimp_drawable_bucket_fill_line_pattern (guchar   *buf,
                                        guchar   *mask,
                                        TempBuf  *pattern,
                                        gboolean  has_alpha,
                                        gint      bytes,
                                        gint      x,
                                        gint      y,
                                        gint      width)
{
  guchar *pat, *p;
  gint    alpha, b;
  gint    i;

  /*  Get a pointer to the appropriate scanline of the pattern buffer  */
  pat = temp_buf_data (pattern) +
    (y % pattern->height) * pattern->width * pattern->bytes;

  alpha = (has_alpha) ? bytes - 1 : bytes;
  for (i = 0; i < width; i++)
    {
      p = pat + ((i + x) % pattern->width) * pattern->bytes;

      for (b = 0; b < alpha; b++)
	buf[b] = p[b];

      if (has_alpha)
	{
	  if (mask)
	    buf[alpha] = *mask++;
	  else
	    buf[alpha] = OPAQUE_OPACITY;
	}

      buf += bytes;
    }
}
