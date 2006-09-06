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

#include <string.h>

#include <glib-object.h>

#include "libgimpmath/gimpmath.h"

#include "paint-funcs-types.h"

#include "base/pixel-region.h"

#include "scale-funcs.h"


static void  scale_region_no_resample (PixelRegion           *srcPR,
                                       PixelRegion           *destPR);
static void  scale_region_lanczos     (PixelRegion           *srcPR,
                                       PixelRegion           *dstPR,
                                       GimpProgressFunc       progress_callback,
                                       gpointer               progress_data);

static void  expand_line              (gdouble               *dest,
                                       const gdouble         *src,
                                       gint                   bytes,
                                       gint                   old_width,
                                       gint                   width,
                                       GimpInterpolationType  interp);
static void  shrink_line              (gdouble               *dest,
                                       const gdouble         *src,
                                       gint                   bytes,
                                       gint                   old_width,
                                       gint                   width,
                                       GimpInterpolationType  interp);


/* Note: cubic function no longer clips result */
static inline gdouble
cubic (gdouble dx,
       gint    jm1,
       gint    j,
       gint    jp1,
       gint    jp2)
{
  /* Catmull-Rom - not bad */
  return (gdouble) ((( ( - jm1 + 3 * j - 3 * jp1 + jp2 ) * dx +
                       ( 2 * jm1 - 5 * j + 4 * jp1 - jp2 ) ) * dx +
                     ( - jm1 + jp1 ) ) * dx + (j + j) ) / 2.0;
}


/*
 * non-interpolating scale_region.  [adam]
 */
static void
scale_region_no_resample (PixelRegion *srcPR,
                          PixelRegion *destPR)
{
  const gint    width       = destPR->w;
  const gint    height      = destPR->h;
  const gint    orig_width  = srcPR->w;
  const gint    orig_height = srcPR->h;
  const gint    bytes       = srcPR->bytes;
  gint         *x_src_offsets;
  gint         *y_src_offsets;
  guchar       *src;
  guchar       *dest;
  gint          last_src_y;
  gint          row_bytes;
  gint          x, y;
  gint          b;

  /*  the data pointers...  */
  x_src_offsets = g_new (gint, width * bytes);
  y_src_offsets = g_new (gint, height);
  src  = g_new (guchar, orig_width * bytes);
  dest = g_new (guchar, width * bytes);

  /*  pre-calc the scale tables  */
  for (b = 0; b < bytes; b++)
    for (x = 0; x < width; x++)
      x_src_offsets [b + x * bytes] =
        b + bytes * ((x * orig_width + orig_width / 2) / width);

  for (y = 0; y < height; y++)
    y_src_offsets [y] = (y * orig_height + orig_height / 2) / height;

  /*  do the scaling  */
  row_bytes = width * bytes;
  last_src_y = -1;

  for (y = 0; y < height; y++)
    {
      /* if the source of this line was the same as the source
       *  of the last line, there's no point in re-rescaling.
       */
      if (y_src_offsets[y] != last_src_y)
        {
          pixel_region_get_row (srcPR, 0, y_src_offsets[y], orig_width, src, 1);

          for (x = 0; x < row_bytes ; x++)
            dest[x] = src[x_src_offsets[x]];

          last_src_y = y_src_offsets[y];
        }

      pixel_region_set_row (destPR, 0, y, width, dest);
    }

  g_free (x_src_offsets);
  g_free (y_src_offsets);
  g_free (src);
  g_free (dest);
}


static void
get_premultiplied_double_row (PixelRegion *srcPR,
                              gint         x,
                              gint         y,
                              gint         w,
                              gdouble     *row,
                              guchar      *tmp_src,
                              gint         n)
{
  const gint bytes = srcPR->bytes;
  gint       b;

  pixel_region_get_row (srcPR, x, y, w, tmp_src, n);

  if (pixel_region_has_alpha (srcPR))
    {
      /* premultiply the alpha into the double array */
      const gint  alpha = bytes - 1;
      gdouble    *irow  = row;

      for (x = 0; x < w; x++)
        {
          gdouble  mod_alpha = tmp_src[alpha] / 255.0;

          for (b = 0; b < alpha; b++)
            irow[b] = mod_alpha * tmp_src[b];

          irow[b] = tmp_src[alpha];
          irow += bytes;
          tmp_src += bytes;
        }
    }
  else /* no alpha */
    {
      for (x = 0; x < w * bytes; x++)
        row[x] = tmp_src[x];
    }

  /* set the off edge pixels to their nearest neighbor */
  for (b = 0; b < 2 * bytes; b++)
    row[b - 2 * bytes] = row[b % bytes];

  for (b = 0; b < 2 * bytes; b++)
    row[b + w * bytes] = row[(w - 1) * bytes + b % bytes];
}


static void
expand_line (gdouble               *dest,
             const gdouble         *src,
             gint                   bytes,
             gint                   old_width,
             gint                   width,
             GimpInterpolationType  interp)
{
  const gdouble *s;
  const gdouble  ratio = (gdouble) old_width / (gdouble) width;
  gint           x, b;
  gint           src_col;
  gdouble        frac;

  /* we can overflow src's boundaries, so we expect our caller to have
     allocated extra space for us to do so safely (see scale_region ()) */

  /* this could be optimized much more by precalculating the coefficients for
     each x */
  switch(interp)
    {
    case GIMP_INTERPOLATION_CUBIC:
      for (x = 0; x < width; x++)
        {
          src_col = ((gint) (x * ratio + 2.0 - 0.5)) - 2;
          /* +2, -2 is there because (int) rounds towards 0 and we need
             to round down */
          frac = (x * ratio - 0.5) - src_col;
          s = &src[src_col * bytes];

          for (b = 0; b < bytes; b++)
            dest[b] = cubic (frac, s[b - bytes], s[b], s[b + bytes],
                             s[b + bytes * 2]);

          dest += bytes;
        }

      break;

    case GIMP_INTERPOLATION_LINEAR:
      for (x = 0; x < width; x++)
        {
          src_col = ((gint) (x * ratio + 2.0 - 0.5)) - 2;
          /* +2, -2 is there because (int) rounds towards 0 and we need
             to round down */
          frac = (x * ratio - 0.5) - src_col;
          s = &src[src_col * bytes];

          for (b = 0; b < bytes; b++)
            dest[b] = ((s[b + bytes] - s[b]) * frac + s[b]);

          dest += bytes;
        }
      break;

    case GIMP_INTERPOLATION_NONE:
      g_assert_not_reached ();
      break;

    case GIMP_INTERPOLATION_LANCZOS:
      g_assert_not_reached ();
      break;
    }
}


static void
shrink_line (gdouble               *dest,
             const gdouble         *src,
             gint                   bytes,
             gint                   old_width,
             gint                   width,
             GimpInterpolationType  interp)
{
  const gdouble *srcp;
  gdouble       *destp;
  gdouble        accum[4];
  gdouble        slice;
  const gdouble  avg_ratio = (gdouble) width / old_width;
  const gdouble  inv_width = 1.0 / width;
  gint           slicepos;      /* slice position relative to width */
  gint           x;
  gint           b;

#if 0
  g_printerr ("shrink_line bytes=%d old_width=%d width=%d interp=%d "
              "avg_ratio=%f\n",
              bytes, old_width, width, interp, avg_ratio);
#endif

  g_return_if_fail (bytes <= 4);

  /* This algorithm calculates the weighted average of pixel data that
     each output pixel must receive, taking into account that it always
     scales down, i.e. there's always more than one input pixel per each
     output pixel.  */

  srcp = src;
  destp = dest;

  slicepos = 0;

  /* Initialize accum to the first pixel slice.  As there is no partial
     pixel at start, that value is 0.  The source data is interleaved, so
     we maintain BYTES accumulators at the same time to deal with that
     many channels simultaneously.  */
  for (b = 0; b < bytes; b++)
    accum[b] = 0.0;

  for (x = 0; x < width; x++)
    {
      /* Accumulate whole pixels.  */
      do
        {
          for (b = 0; b < bytes; b++)
            accum[b] += *srcp++;

          slicepos += width;
        }
      while (slicepos < old_width);
      slicepos -= old_width;

      if (! (slicepos < width))
        g_warning ("Assertion (slicepos < width) failed. Please report.");

      if (slicepos == 0)
        {
          /* Simplest case: we have reached a whole pixel boundary.  Store
             the average value per channel and reset the accumulators for
             the next round.

             The main reason to treat this case separately is to avoid an
             access to out-of-bounds memory for the first pixel.  */
          for (b = 0; b < bytes; b++)
            {
              *destp++ = accum[b] * avg_ratio;
              accum[b] = 0.0;
            }
        }
      else
        {
          for (b = 0; b < bytes; b++)
            {
              /* We have accumulated a whole pixel per channel where just a
                 slice of it was needed.  Subtract now the previous pixel's
                 extra slice.  */
              slice = srcp[- bytes + b] * slicepos * inv_width;
              *destp++ = (accum[b] - slice) * avg_ratio;

              /* That slice is the initial value for the next round.  */
              accum[b] = slice;
            }
        }
    }

  /* Sanity check: srcp should point to the next-to-last position, and
     slicepos should be zero.  */
  if (! (srcp - src == old_width * bytes && slicepos == 0))
    g_warning ("Assertion (srcp - src == old_width * bytes && slicepos == 0)"
               " failed. Please report.");
}

static inline void
rotate_pointers (guchar  **p,
                 guint32   n)
{
  guchar  *tmp = p[0];
  guint32  i;

  for (i = 0; i < n-1; i++)
    p[i] = p[i+1];

  p[i] = tmp;
}

static void
get_scaled_row (gdouble              **src,
                gint                   y,
                gint                   new_width,
                PixelRegion           *srcPR,
                gdouble               *row,
                guchar                *src_tmp,
                GimpInterpolationType  interpolation_type)
{
  /* get the necesary lines from the source image, scale them,
     and put them into src[] */

  rotate_pointers ((gpointer) src, 4);

  if (y < 0)
    y = 0;

  if (y < srcPR->h)
    {
      get_premultiplied_double_row (srcPR, 0, y, srcPR->w, row, src_tmp, 1);

      if (new_width > srcPR->w)
        expand_line (src[3], row, srcPR->bytes,
                     srcPR->w, new_width, interpolation_type);
      else if (srcPR->w > new_width)
        shrink_line (src[3], row, srcPR->bytes,
                     srcPR->w, new_width, interpolation_type);
      else /* no scailing needed */
        memcpy (src[3], row, sizeof (gdouble) * new_width * srcPR->bytes);
    }
  else
    {
      memcpy (src[3], src[2], sizeof (gdouble) * new_width * srcPR->bytes);
    }
}

void
scale_region (PixelRegion           *srcPR,
              PixelRegion           *destPR,
              GimpInterpolationType  interpolation,
              GimpProgressFunc       progress_callback,
              gpointer               progress_data)
{
  gdouble *src[4];
  guchar  *src_tmp;
  guchar  *dest;
  gdouble *row, *accum;
  gint     bytes, b;
  gint     width, height;
  gint     orig_width, orig_height;
  gdouble  y_rat;
  gint     i;
  gint     old_y = -4;
  gint     new_y;
  gint     x, y;

  if (interpolation == GIMP_INTERPOLATION_NONE)
    {
      scale_region_no_resample (srcPR, destPR);
      return;
    }

  orig_width = srcPR->w;
  orig_height = srcPR->h;

  width = destPR->w;
  height = destPR->h;

  if (interpolation == GIMP_INTERPOLATION_LANCZOS && orig_height <= height)
    {
      scale_region_lanczos (srcPR, destPR, progress_callback, progress_data);
      return;
    }

#if 0
  g_printerr ("scale_region: (%d x %d) -> (%d x %d)\n",
              orig_width, orig_height, width, height);
#endif

  /*  find the ratios of old y to new y  */
  y_rat = (gdouble) orig_height / (gdouble) height;

  bytes = destPR->bytes;

  /*  the data pointers...  */
  for (i = 0; i < 4; i++)
    src[i] = g_new (gdouble, width * bytes);

  dest = g_new (guchar, width * bytes);

  src_tmp = g_new (guchar, orig_width * bytes);

  /* offset the row pointer by 2*bytes so the range of the array
     is [-2*bytes] to [(orig_width + 2)*bytes] */
  row = g_new (gdouble, (orig_width + 2 * 2) * bytes);
  row += bytes * 2;

  accum = g_new (gdouble, width * bytes);

  /*  Scale the selected region  */

  for (y = 0; y < height; y++)
    {
      if (progress_callback && !(y & 0xf))
        (* progress_callback) (0, height, y, progress_data);

      if (height < orig_height)
        {
          const gdouble inv_ratio = 1.0 / y_rat;
          gint          max;
          gdouble       frac;

          if (y == 0) /* load the first row if this is the first time through */
            get_scaled_row (&src[0], 0, width, srcPR, row,
                            src_tmp,
                            interpolation);

          new_y = (int) (y * y_rat);
          frac = 1.0 - (y * y_rat - new_y);

          for (x = 0; x < width * bytes; x++)
            accum[x] = src[3][x] * frac;

          max = (int) ((y + 1) * y_rat) - new_y - 1;

          get_scaled_row (&src[0], ++new_y, width, srcPR, row,
                          src_tmp,
                          interpolation);

          while (max > 0)
            {
              for (x = 0; x < width * bytes; x++)
                accum[x] += src[3][x];

              get_scaled_row (&src[0], ++new_y, width, srcPR, row,
                              src_tmp,
                              interpolation);
              max--;
            }

          frac = (y + 1) * y_rat - ((int) ((y + 1) * y_rat));

          for (x = 0; x < width * bytes; x++)
            {
              accum[x] += frac * src[3][x];
              accum[x] *= inv_ratio;
            }
        }
      else if (height > orig_height)
        {
          new_y = floor (y * y_rat - 0.5);

          while (old_y <= new_y)
            {
              /* get the necesary lines from the source image, scale them,
                 and put them into src[] */
              get_scaled_row (&src[0], old_y + 2, width, srcPR, row,
                              src_tmp,
                              interpolation);
              old_y++;
            }

          switch (interpolation)
            {
            case GIMP_INTERPOLATION_CUBIC:
              {
                gdouble p0, p1, p2, p3;
                gdouble dy = (y * y_rat - 0.5) - new_y;

                p0 = cubic (dy, 1, 0, 0, 0);
                p1 = cubic (dy, 0, 1, 0, 0);
                p2 = cubic (dy, 0, 0, 1, 0);
                p3 = cubic (dy, 0, 0, 0, 1);

                for (x = 0; x < width * bytes; x++)
                  accum[x] = (p0 * src[0][x] + p1 * src[1][x] +
                              p2 * src[2][x] + p3 * src[3][x]);
              }

              break;

            case GIMP_INTERPOLATION_LINEAR:
              {
                gdouble idy = (y * y_rat - 0.5) - new_y;
                gdouble dy  = 1.0 - idy;

                for (x = 0; x < width * bytes; x++)
                  accum[x] = dy * src[1][x] + idy * src[2][x];
              }

              break;

            case GIMP_INTERPOLATION_NONE:
              g_assert_not_reached ();
              break;

            case GIMP_INTERPOLATION_LANCZOS:
              g_assert_not_reached ();
              break;
            }
        }
      else /* height == orig_height */
        {
          get_scaled_row (&src[0], y, width, srcPR, row,
                          src_tmp,
                          interpolation);
          memcpy (accum, src[3], sizeof (gdouble) * width * bytes);
        }

      if (pixel_region_has_alpha (srcPR))
        {
          /* unmultiply the alpha */
          gdouble  inv_alpha;
          gdouble *p = accum;
          gint     alpha = bytes - 1;
          gint     result;
          guchar  *d = dest;

          for (x = 0; x < width; x++)
            {
              if (p[alpha] > 0.001)
                {
                  inv_alpha = 255.0 / p[alpha];

                  for (b = 0; b < alpha; b++)
                    {
                      result = RINT (inv_alpha * p[b]);

                      if (result < 0)
                        d[b] = 0;
                      else if (result > 255)
                        d[b] = 255;
                      else
                        d[b] = result;
                    }

                  result = RINT (p[alpha]);

                  if (result > 255)
                    d[alpha] = 255;
                  else
                    d[alpha] = result;
                }
              else /* alpha <= 0 */
                {
                  for (b = 0; b <= alpha; b++)
                    d[b] = 0;
                }

              d += bytes;
              p += bytes;
            }
        }
      else
        {
          gint w = width * bytes;

          for (x = 0; x < w; x++)
            {
              if (accum[x] < 0.0)
                dest[x] = 0;
              else if (accum[x] > 255.0)
                dest[x] = 255;
              else
                dest[x] = RINT (accum[x]);
            }
        }

      pixel_region_set_row (destPR, 0, y, width, dest);
    }

  /*  free up temporary arrays  */
  g_free (accum);

  for (i = 0; i < 4; i++)
    g_free (src[i]);

  g_free (src_tmp);
  g_free (dest);

  row -= 2 * bytes;
  g_free (row);
}

void
subsample_region (PixelRegion *srcPR,
                  PixelRegion *destPR,
                  gint         subsample)
{
  const gint     width       = destPR->w;
  const gint     height      = destPR->h;
  const gint     orig_width  = srcPR->w / subsample;
  const gint     orig_height = srcPR->h / subsample;
  const gdouble  x_rat       = (gdouble) orig_width / (gdouble) width;
  const gdouble  y_rat       = (gdouble) orig_height / (gdouble) height;
  const gint     bytes       = destPR->bytes;
  const gint     destwidth   = destPR->rowstride;
  guchar        *src,  *s;
  guchar        *dest, *d;
  gdouble       *row,  *r;
  gint           src_row, src_col;
  gdouble        x_cum, y_cum;
  gdouble        x_last, y_last;
  gdouble       *x_frac, y_frac, tot_frac;
  gint           i, j;
  gint           b;
  gint           frac;
  gint           advance_dest;

#if 0
  g_printerr ("subsample_region: (%d x %d) -> (%d x %d)\n",
              orig_width, orig_height, width, height);
#endif

  /*  the data pointers...  */
  src  = g_new (guchar, orig_width * bytes);
  dest = destPR->data;

  /*  allocate an array to help with the calculations  */
  row    = g_new (gdouble, width * bytes);
  x_frac = g_new (gdouble, width + orig_width);

  /*  initialize the pre-calculated pixel fraction array  */
  src_col = 0;
  x_cum = (gdouble) src_col;
  x_last = x_cum;

  for (i = 0; i < width + orig_width; i++)
    {
      if (x_cum + x_rat <= (src_col + 1 + EPSILON))
        {
          x_cum += x_rat;
          x_frac[i] = x_cum - x_last;
        }
      else
        {
          src_col ++;
          x_frac[i] = src_col - x_last;
        }

      x_last += x_frac[i];
    }

  /*  clear the "row" array  */
  memset (row, 0, sizeof (gdouble) * width * bytes);

  /*  counters...  */
  src_row = 0;
  y_cum = (gdouble) src_row;
  y_last = y_cum;

  pixel_region_get_row (srcPR,
                        srcPR->x, srcPR->y + src_row * subsample,
                        orig_width * subsample,
                        src, subsample);

  /*  Scale the selected region  */
  for (i = 0; i < height; )
    {
      src_col = 0;
      x_cum = (gdouble) src_col;

      /* determine the fraction of the src pixel we are using for y */
      if (y_cum + y_rat <= (src_row + 1 + EPSILON))
        {
          y_cum += y_rat;
          y_frac = y_cum - y_last;
          advance_dest = TRUE;
        }
      else
        {
          src_row ++;
          y_frac = src_row - y_last;
          advance_dest = FALSE;
        }

      y_last += y_frac;

      s = src;
      r = row;

      frac = 0;
      j = width;

      while (j)
        {
          tot_frac = x_frac[frac++] * y_frac;

          for (b = 0; b < bytes; b++)
            r[b] += s[b] * tot_frac;

          /*  increment the destination  */
          if (x_cum + x_rat <= (src_col + 1 + EPSILON))
            {
              r += bytes;
              x_cum += x_rat;
              j--;
            }

          /* increment the source */
          else
            {
              s += bytes;
              src_col++;
            }
        }

      if (advance_dest)
        {
          tot_frac = 1.0 / (x_rat * y_rat);

          /*  copy "row" to "dest"  */
          d = dest;
          r = row;

          j = width;
          while (j--)
            {
              b = bytes;

              while (b--)
                *d++ = (guchar) (*r++ * tot_frac + 0.5);
            }

          dest += destwidth;

          /*  clear the "row" array  */
          memset (row, 0, sizeof (gdouble) * destwidth);

          i++;
        }
      else
        {
          pixel_region_get_row (srcPR,
                                srcPR->x,
                                srcPR->y + src_row * subsample,
                                orig_width * subsample,
                                src, subsample);
        }
    }

  /*  free up temporary arrays  */
  g_free (row);
  g_free (x_frac);
  g_free (src);
}


/* Lanczos */
static inline gdouble
sinc (gdouble x)
{
  gdouble y = x * G_PI;

  if (ABS (x) < EPSILON)
    return 1.0;

  return sin (y) / y;
}

static inline gdouble
lanczos_sum (guchar         *ptr,
             const gdouble  *lu,
             gint            u,
             gint            bytes,
             gint            byte)
{
  gdouble sum = 0;
  gint    i;

  for (i = 0; i < LANCZOS_WIDTH2 ; i++)
    sum += lu[i] * ptr[ (u + i - LANCZOS_WIDTH) * bytes + byte ];

  return sum;
}

static inline gdouble
lanczos_sum_mul (guchar         *ptr,
                 const gdouble  *lu,
                 gint            u,
                 gint            bytes,
                 gint            byte,
                 gint            alpha)
{
  gdouble sum = 0;
  gint    i;

  for (i = 0; i < LANCZOS_WIDTH2 ; i++ )
    sum += lu[i] * ptr[ (u + i - LANCZOS_WIDTH) * bytes + byte ]
                 * ptr[ (u + i - LANCZOS_WIDTH) * bytes + alpha];

  return sum;
}

static gboolean
inv_lin_trans (const gdouble *t,
               gdouble       *it)
{
  gdouble d = (t[0] * t[4]) - (t[1] * t[3]);  /* determinant */

  if (fabs(d) < EPSILON )
    return FALSE;

  it[0] =    t[4] / d;
  it[1] =   -t[1] / d;
  it[2] = (( t[1] * t[5]) - (t[2] * t[4])) / d;
  it[3] =   -t[3] / d;
  it[4] =    t[0] / d;
  it[5] = (( t[2] * t[3]) - (t[0] * t[5])) / d;

  return TRUE;
}

static gdouble *
kernel_lanczos (void)
{
  const gdouble dx = (gdouble) LANCZOS_WIDTH / (gdouble) (LANCZOS_SAMPLES - 1);

  gdouble *kernel = g_new (gdouble, LANCZOS_SAMPLES);
  gdouble  x      = 0.0;
  gint     i;

  for (i = 0; i < LANCZOS_SAMPLES; i++)
    {
      kernel[i] = ((ABS (x) < LANCZOS_WIDTH) ?
                   (sinc (x) * sinc (x / LANCZOS_WIDTH)) : 0.0);
      x += dx;
    }

  return kernel;
}

static void
scale_region_lanczos (PixelRegion           *srcPR,
                      PixelRegion           *dstPR,
                      GimpProgressFunc       progress_callback,
                      gpointer               progress_data)

{
  gdouble       *kernel  = NULL;        /* Lanczos kernel                    */
  gdouble        lu[LANCZOS_WIDTH2],    /* Lanczos sample value              */
                 lv[LANCZOS_WIDTH2];    /* Lanczos sample value              */
  gdouble        lusum, lvsum, weight;  /* Lanczos weighting vars            */

  gdouble        newval;                /* new interpolated RGB value */

  guchar        *win_buf = NULL;        /* Sliding window buffer             */
  guchar        *win_ptr[LANCZOS_WIDTH2];
                                        /* Ponters to sliding window rows    */

  guchar        *dst_buf = NULL;        /* Pointer to destination image data */

  gint           x, y;                  /* Position in destination image     */
  gint           i, byte;            /* loop vars to fill source window   */
  gint           row;

  gdouble        trans[6], itrans[6];   /* Scale transformations             */

  const gint     dst_width     = dstPR->w;
  const gint     dst_height    = dstPR->h;
  const gint     bytes         = dstPR->bytes;
  const gint     src_width     = srcPR->w;
  const gint     src_height    = srcPR->h;

  const gint     src_rowstride = src_width * bytes;
  const gint     dst_rowstride = dst_width * bytes;
  const gint     win_rowstride = (src_width + LANCZOS_WIDTH2) * bytes;

  const gdouble  sx            = (gdouble) dst_width / (gdouble) src_width;
  const gdouble  sy            = (gdouble) dst_height / (gdouble) src_height;

  for (i = 0; i < 6; i++)
    trans[i] = 0.0;

  trans[0] = sx;
  trans[4] = sy;

  if (! inv_lin_trans (trans, itrans))
    {
      g_warning ("transformation matrix is not invertible");
      return;
    }

  /* allocate buffer for destination row */
  dst_buf = g_new0 (guchar, dst_rowstride);
  /* if no scaling needed copy data */
  if ( dst_width == src_width && dst_height == src_height )
    {
       for ( i = 0 ; i < src_height ; i++ )
         {
           pixel_region_get_row (srcPR, 0, i, src_width, dst_buf, 1);
           pixel_region_set_row (dstPR, 0, i, dst_width, dst_buf);
         }
       g_free(dst_buf);
       return;
    }

  /* Calculate kernel */
  kernel = kernel_lanczos ();

  /* allocate buffer for source rows */
  win_buf = g_new0 (guchar, win_rowstride * LANCZOS_WIDTH2);

  /* Set the window pointers */
  for ( i = 0 ; i < LANCZOS_WIDTH2 ; i++ )
    win_ptr[i] = win_buf + ( win_rowstride * i ) + LANCZOS_WIDTH * bytes;

  /* fill the data for the first loop */
  for ( i = 0 ; i <= LANCZOS_WIDTH && i < src_height ; i++)
    pixel_region_get_row (srcPR, 0, i, src_width, win_ptr[i + LANCZOS_WIDTH], 1);

  for (row = y = 0; y < dst_height; y++)
    {
      if (progress_callback && !(y & 0xf))
        (* progress_callback) (0, dst_height, y, progress_data);

      pixel_region_get_row (dstPR, 0, y, dst_width, dst_buf, 1);
      for  (x = 0; x < dst_width; x++)
        {
          gdouble  du ,dv;   /* Transformed position in source image */
          gint     u, v;     /* Position in source image */
          gint     su, sv;   /* Lanczos kernel position  */
          /*
             Use linear trans. for determining source coordinates from
             destination. Coefficient -0.5 fixes the offset error.
           */
          du = itrans[0] * ((gdouble) x) + itrans[1] * ((gdouble) y) + itrans[2] - 0.5;
          dv = itrans[3] * ((gdouble) x) + itrans[4] * ((gdouble) y) + itrans[5] - 0.5;

          /* Coordinates in source image */
          u = (gint) du;
          v = (gint) dv;

          /* get weight for fractional error */
          su = (gint) ((du - u) * LANCZOS_SPP);
          sv = (gint) ((dv - v) * LANCZOS_SPP);

          /* Fill multipliers in lu[] and lv[]
           *
           *  kernel = Is a lookup table that contains half of the sinc func.
           *
           *  su, sv = shift from kernel center due to fractional part
           *           of interpollation
           *
           *  The for loop creates 2 1D kernels for convolution.
           *    - If the center position +/- LANCZOS_WIDTH is out of
           *      the source image coordinates set the value to 0.0
           *    - If the kernel index is out of range set value to 0.0
           *      ( caused by offset coef.)
           */
          lusum = lvsum = 0.0;

          for (i = LANCZOS_WIDTH; i >= -LANCZOS_WIDTH; i--)
            {
              gint pos = i * LANCZOS_SPP;

              if ( u + i >= 0 && u + i < src_width)
                lusum += lu[LANCZOS_WIDTH + i] = kernel[ABS (su - pos)];
              else
                lusum += lu[LANCZOS_WIDTH + i] = 0.0;

              if ( v + i >= 0 && v + i < src_height)
                lvsum += lv[LANCZOS_WIDTH + i] = kernel[ABS (sv - pos)];
              else
                lvsum += lv[LANCZOS_WIDTH + i] = 0.0;
            }

          weight = lvsum*lusum;
          /*
            Scaling up
            New determined source row is > than last read row
            rotate the pointers and get next source row from region.
            If no more source rows are available fill buffer with 0
            ( Probably not necessary because multipliers should be 0).
          */
          for ( ; row < v ; )
            {
              row++;
              rotate_pointers (win_ptr, LANCZOS_WIDTH2);
              if ( row + LANCZOS_WIDTH < src_height)
                pixel_region_get_row (srcPR, 0,
                                      row + LANCZOS_WIDTH, src_width,
                                      win_ptr[LANCZOS_WIDTH2 - 1], 1);
              else
                memset (win_ptr[LANCZOS_WIDTH2 - 1], 0,
                        sizeof (guchar) * src_rowstride);
            }
           /*
              Scaling down
            */
          for ( ; row > v ; )
            {
              row--;
              for ( i = 0 ; i < LANCZOS_WIDTH2 - 1 ; i++ )
                 rotate_pointers (win_ptr, LANCZOS_WIDTH2);
              if ( row >=  0)
                pixel_region_get_row (srcPR, 0,
                                      row, src_width,
                                      win_ptr[0], 1);
              else
                memset (win_ptr[0], 0,
                        sizeof (guchar) * src_rowstride);

            }


           if (pixel_region_has_alpha (srcPR))
             {
               const gint alpha = bytes - 1;
               gint       byte;
               gdouble    arecip;
               gdouble    aval;

               aval = 0.0;
               for (i = 0; i < LANCZOS_WIDTH2 ; i++ )
                 aval += lv[i] * lanczos_sum (win_ptr[i], lu,
                                                u, bytes, alpha);
               /* calculate alpha of result */
               aval /= weight;

               if (aval <= 0.0)
                 {
                   arecip = 0.0;
                   dst_buf[x * bytes + alpha] = 0;
                 }
               else if (aval > 255.0)
                 {
                   arecip = 1.0 / aval;
                   dst_buf[x * bytes + alpha] = 255;
                 }
               else
                 {
                   arecip = 1.0 / aval;
                   dst_buf[x * bytes + alpha] = RINT (aval);
                 }

               for (byte = 0; byte < alpha; byte++)
                 {
                   newval = 0.0;
                   for (i = 0; i < LANCZOS_WIDTH2; i++ )
                     newval += lv[i] * lanczos_sum_mul (win_ptr[i], lu,
                                                        u, bytes, byte, alpha);
                   newval *= arecip;
                   dst_buf[x * bytes + byte] = CLAMP (newval, 0, 255);
                 }
             }
           else
             {
               for (byte = 0; byte < bytes; byte++)
                 {
                   /* Calculate new value */
                   newval = 0.0;
                   for (i = 0; i < LANCZOS_WIDTH2; i++ )
                     newval += lv[i] * lanczos_sum (win_ptr[i], lu,
                                                    u, bytes, byte);
                   newval /= weight;
                   dst_buf[x * bytes + byte] = CLAMP ((gint) newval, 0, 255);
                 }
             }
        }

      pixel_region_set_row (dstPR, 0, y , dst_width, dst_buf);
    }

  g_free (dst_buf);
  g_free (win_buf);
  g_free (kernel);
}

