/*
 * DDS GIMP plugin
 *
 * Copyright (C) 2004-2012 Shawn Kirst <skirst@gmail.com>,
 * with parts (C) 2003 Arne Reuter <homepage@arnereuter.de> where specified.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 51 Franklin Street, Fifth Floor
 * Boston, MA 02110-1301, USA.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "dds.h"
#include "mipmap.h"
#include "imath.h"


typedef gfloat (*filterfunc_t)    (gfloat);
typedef gint   (*wrapfunc_t)      (gint, gint);
typedef void   (*mipmapfunc_t)    (guchar*, gint, gint, guchar*, gint, gint, gint,
                                   filterfunc_t, gfloat, wrapfunc_t, gint, gfloat);
typedef void   (*volmipmapfunc_t) (guchar*, gint, gint, gint, guchar*, gint, gint, gint,
                                   gint, filterfunc_t, gfloat, wrapfunc_t, gint, gfloat);


/**
 * Size Functions
 */

gint
get_num_mipmaps (gint  width,
                 gint  height)
{
  gint w = width  << 1;
  gint h = height << 1;
  gint n = 0;

  while (w != 1 || h != 1)
    {
      if (w > 1) w >>= 1;
      if (h > 1) h >>= 1;
      ++n;
    }

  return n;
}

guint
get_mipmapped_size (gint  width,
                    gint  height,
                    gint  bpp,
                    gint  level,
                    gint  num,
                    gint  format)
{
  gint  w, h, n = 0;
  guint size    = 0;

  w = width  >> level;
  h = height >> level;
  w = MAX (1, w);
  h = MAX (1, h);
  w <<= 1;
  h <<= 1;

  while (n < num && (w != 1 || h != 1))
    {
      if (w > 1) w >>= 1;
      if (h > 1) h >>= 1;
      if (format == DDS_COMPRESS_NONE)
        size += (w * h);
      else
        size += ((w + 3) >> 2) * ((h + 3) >> 2);
      ++n;
    }

  if (format == DDS_COMPRESS_NONE)
    {
      size *= bpp;
    }
  else
    {
      if (format == DDS_COMPRESS_BC1 || format == DDS_COMPRESS_BC4)
        size *= 8;
      else
        size *= 16;
    }

  return size;
}

guint
get_volume_mipmapped_size (gint  width,
                           gint  height,
                           gint  depth,
                           gint  bpp,
                           gint  level,
                           gint  num,
                           gint  format)
{
  gint  w, h, d, n = 0;
  guint size       = 0;

  w = width >> level;
  h = height >> level;
  d = depth >> level;
  w = MAX (1, w);
  h = MAX (1, h);
  d = MAX (1, d);
  w <<= 1;
  h <<= 1;
  d <<= 1;

  while (n < num && (w != 1 || h != 1))
    {
      if (w > 1) w >>= 1;
      if (h > 1) h >>= 1;
      if (d > 1) d >>= 1;
      if (format == DDS_COMPRESS_NONE)
        size += (w * h * d);
      else
        size += (((w + 3) >> 2) * ((h + 3) >> 2) * d);
      ++n;
    }

  if (format == DDS_COMPRESS_NONE)
    {
      size *= bpp;
    }
  else
    {
      if (format == DDS_COMPRESS_BC1 || format == DDS_COMPRESS_BC4)
        size *= 8;
      else
        size *= 16;
    }

  return size;
}

gint
get_next_mipmap_dimensions (gint *next_w,
                            gint *next_h,
                            gint  curr_w,
                            gint  curr_h)
{
  if (curr_w == 1 || curr_h == 1)
    return 0;

  if (next_w) *next_w = curr_w >> 1;
  if (next_h) *next_h = curr_h >> 1;

  return 1;
}


/**
 * Wrap Modes
 */

static gint
wrap_mirror (gint  x,
             gint  max)
{
  if (max == 1)
    x = 0;

  x = abs (x);
  while (x >= max)
    x = abs (max + max - x - 2);

  return x;
}

static gint
wrap_repeat (gint  x,
             gint  max)
{
  gfloat t;
  t = (gfloat) x / (gfloat) max;
  return (gint) ((t - floorf (t)) * (gfloat) max);
}

static gint
wrap_clamp (gint  x,
            gint  max)
{
  return MAX (0, MIN (max - 1, x));
}


/**
 * Gamma-correction
 */

static gfloat
linear_to_sRGB (gfloat c)
{
  gfloat v = (gfloat) c;

  if (v < 0.0f)
    v = 0.0f;
  else if (v > 1.0f)
    v = 1.0f;
  else if (v <= 0.0031308f)
    v = 12.92f * v;
  else
    v = 1.055f * powf (v, 0.41666f) - 0.055f;

  return v;
}

static gfloat
linear_to_gamma (gint    gc,
                 gfloat  v,
                 gfloat  gamma)
{
  if (gc == 1)
    {
      v = powf (v, 1.0f / gamma);
      if (v > 1.0f)
        v = 1.0f;
    }
  else if (gc == 2)
    {
      v = linear_to_sRGB (v);
    }

  return v;
}


static gfloat
sRGB_to_linear (gfloat c)
{
  gfloat v = (gfloat) c;

  if (v < 0.0f)
    v = 0.0f;
  else if (v > 1.0f)
    v = 1.0f;
  else if (v <= 0.04045f)
    v /= 12.92f;
  else
    v = powf ((v + 0.055f) / 1.055f, 2.4f);

  return v;
}

static gfloat
gamma_to_linear (gint    gc,
                 gfloat  v,
                 gfloat  gamma)
{
  if (gc == 1)
    {
      v = powf (v, gamma);
      if (v > 1.0f)
        v = 1.0f;
    }
  else if (gc == 2)
    {
      v = sRGB_to_linear (v);
    }

  return v;
}


/**
 * Filters
 */

static gfloat
box_filter (gfloat  t)
{
  if ((t >= -0.5f) && (t < 0.5f))
    return 1.0f;

  return 0.0f;
}

static gfloat
triangle_filter (gfloat  t)
{
  if (t < 0.0f) t = -t;
  if (t < 1.0f) return 1.0f - t;

  return 0.0f;
}

static gfloat
quadratic_filter (gfloat  t)
{
  if (t < 0.0f) t = -t;
  if (t < 0.5f) return 0.75f - t * t;
  if (t < 1.5f)
    {
      t -= 1.5f;
      return 0.5f * t * t;
    }

  return 0.0f;
}

static gfloat
mitchell (gfloat        t,
          const gfloat  B)
{
  gfloat C, tt;

  C  = 0.5f * (1.0f - B);
  tt = t * t;
  if (t < 0.0f)
    t = -t;

  if (t < 1.0f)
    {
      t = (((12.0f - 9.0f * B - 6.0f * C) * (t * tt)) +
           ((-18.0f + 12.0f * B + 6.0f * C) * tt) +
           (6.0f - 2.0f * B));

      return t / 6.0f;
    }
  else if (t < 2.0f)
    {
      t = (((-1.0f * B - 6.0f * C) * (t * tt)) +
           ((6.0f * B + 30.0f * C) * tt) +
           ((-12.0f * B - 48.0f * C) * t) +
           (8.0f * B + 24.0f * C));

      return t / 6.0f;
    }

  return 0.0f;
}

static gfloat
bspline_filter (gfloat  t)
{
  return mitchell (t, 1.0f);
}

static gfloat
mitchell_filter (gfloat  t)
{
  return mitchell (t, 1.0f / 3.0f);
}

static gfloat
catrom_filter (gfloat  t)
{
  return mitchell (t, 0.0f);
}

static gfloat
sinc (gfloat  x)
{
  x = (x * M_PI);
  if (fabsf (x) < 1e-04f)
    return 1.0f + x * x * (-1.0f / 6.0f + x * x * 1.0f / 120.0f);

  return sinf (x) / x;
}

static gfloat
lanczos_filter (gfloat  t)
{
  if (t < 0.0f) t = -t;
  if (t < 3.0f) return sinc (t) * sinc (t / 3.0f);

  return 0.0f;
}

static gfloat
bessel0 (gfloat  x)
{
  const gfloat EPSILON = 1e-6f;
  gfloat xh, sum, pow, ds;
  gint   k;

  xh  = 0.5f * x;
  sum = 1.0f;
  pow = 1.0f;
  k   = 0;
  ds  = 1.0f;

  while (ds > sum * EPSILON)
    {
      ++k;
      pow = pow * (xh / k);
      ds = pow * pow;
      sum += ds;
    }

  return sum;
}

static gfloat
kaiser_filter (gfloat  t)
{
  if (t < 0.0f) t = -t;

  if (t < 3.0f)
    {
      const gfloat alpha = 4.0f;
      const gfloat rb04  = 0.0884805322f; // 1.0f / bessel0(4.0f);
      const gfloat ratio = t / 3.0f;
      if ((1.0f - ratio * ratio) >= 0)
        return sinc (t) * bessel0 (alpha * sqrtf (1.0f - ratio * ratio)) * rb04;
    }

  return 0.0f;
}


/**
 * 2D Scaling
 */

static void
scale_image_nearest (guchar       *dst,
                     gint          dw,
                     gint          dh,
                     guchar       *src,
                     gint          sw,
                     gint          sh,
                     gint          bpp,
                     filterfunc_t  filter,
                     gfloat        support,
                     wrapfunc_t    wrap,
                     gint          gc,
                     gfloat        gamma)
{
  gint n, x, y;
  gint ix, iy;
  gint srowbytes = sw * bpp;
  gint drowbytes = dw * bpp;

  for (y = 0; y < dh; ++y)
    {
      iy = (y * sh + sh / 2) / dh;
      for (x = 0; x < dw; ++x)
        {
          ix = (x * sw + sw / 2) / dw;
          for (n = 0; n < bpp; ++n)
            {
              dst[y * drowbytes + (x * bpp) + n] =
                src[iy * srowbytes + (ix * bpp) + n];
            }
        }
    }
}

static void
scale_image (guchar       *dst,
             gint          dw,
             gint          dh,
             guchar       *src,
             gint          sw,
             gint          sh,
             gint          bpp,
             filterfunc_t  filter,
             gfloat        support,
             wrapfunc_t    wrap,
             gint          gc,
             gfloat        gamma)
{
  const gfloat xfactor = (gfloat) dw / (gfloat) sw;
  const gfloat yfactor = (gfloat) dh / (gfloat) sh;

  gint   x, y, start, stop, nmax, n, i;
  gfloat center, contrib, density, s, r, t;
  gint   sstride  = sw * bpp;
  gfloat xscale   = MIN (xfactor, 1.0f);
  gfloat yscale   = MIN (yfactor, 1.0f);
  gfloat xsupport = support / xscale;
  gfloat ysupport = support / yscale;
  guchar *d, *row, *col;
  guchar *tmp;

  if (xsupport <= 0.5f)
    {
      xsupport = 0.5f + 1e-10f;
      xscale = 1.0f;
    }

  if (ysupport <= 0.5f)
    {
      ysupport = 0.5f + 1e-10f;
      yscale = 1.0f;
    }

#ifdef _OPENMP
  tmp = g_malloc (sw * bpp * omp_get_max_threads ());
#else
  tmp = g_malloc (sw * bpp);
#endif

#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic)                              \
  private(x, y, d, row, col, center, start, stop, nmax, s, i, n, density, r, t, contrib)
#endif
  for (y = 0; y < dh; ++y)
    {
      /* resample in Y direction to temp buffer */
      d = tmp;
#ifdef _OPENMP
      d += (sw * bpp * omp_get_thread_num ());
#endif

      center = ((gfloat) y + 0.5f) / yfactor;
      start = (gint) roundf ((center - ysupport) + 0.5f);
      stop  = (gint) roundf ((center + ysupport) + 0.5f);
      nmax = stop - start;
      s = (gfloat) start - center + 0.5f;

      for (x = 0; x < sw; ++x)
        {
          col = src + (x * bpp);

          for (i = 0; i < bpp; ++i)
            {
              density = 0.0f;
              r = 0.0f;

              for (n = 0; n < nmax; ++n)
                {
                  contrib = filter((s + n) * yscale);
                  density += contrib;
                  if (i == 3)
                    t = (gfloat) col[(wrap (start + n, sh) * sstride) + i] / 255.0f;
                  else
                    t = gamma_to_linear (gc, (gfloat) col[(wrap (start + n, sh) * sstride) + i] / 255.0f, gamma);
                  r += t * contrib;
                }

              if (density != 0.0f && density != 1.0f)
                r /= density;

              r = MIN (1.0f, MAX (0.0f, r));

              if (i != 3)
                r = linear_to_gamma (gc, r, gamma);

              d[(x * bpp) + i] = (guchar) floorf (r * 255.0f + 0.5f);
            }
        }

      /* resample in X direction using temp buffer */
      row = d;
      d = dst;

      for (x = 0; x < dw; ++x)
        {
          center = ((gfloat) x + 0.5f) / xfactor;
          start = (gint) roundf ((center - xsupport) + 0.5f);
          stop  = (gint) roundf ((center + xsupport) + 0.5f);
          nmax = stop - start;
          s = (gfloat) start - center + 0.5f;

          for (i = 0; i < bpp; ++i)
            {
              density = 0.0f;
              r = 0.0f;

              for (n = 0; n < nmax; ++n)
                {
                  contrib = filter((s + n) * xscale);
                  density += contrib;
                  if (i == 3)
                    t = (gfloat) row[(wrap (start + n, sw) * bpp) + i] / 255.0f;
                  else
                    t = gamma_to_linear (gc, (gfloat) row[(wrap (start + n, sw) * bpp) + i] / 255.0f, gamma);
                  r += t * contrib;
                }

              if (density != 0.0f && density != 1.0f)
                r /= density;

              r = MIN (1.0f, MAX (0.0f, r));

              if (i != 3)
                r = linear_to_gamma (gc, r, gamma);

              d[(y * (dw * bpp)) + (x * bpp) + i] = (guchar) floorf (r * 255.0f + 0.5f);
            }
        }
    }

  g_free (tmp);
}


/**
 * 3D Scaling
 */

static void
scale_volume_image_nearest (guchar       *dst,
                            gint          dw,
                            gint          dh,
                            gint          dd,
                            guchar       *src,
                            gint          sw,
                            gint          sh,
                            gint          sd,
                            gint          bpp,
                            filterfunc_t  filter,
                            gfloat        support,
                            wrapfunc_t    wrap,
                            gint          gc,
                            gfloat        gamma)
{
  gint n, x, y, z;
  gint ix, iy, iz;

  for (z = 0; z < dd; ++z)
    {
      iz = (z * sd + sd / 2) / dd;
      for (y = 0; y < dh; ++y)
        {
          iy = (y * sh + sh / 2) / dh;
          for (x = 0; x < dw; ++x)
            {
              ix = (x * sw + sw / 2) / dw;
              for (n = 0; n < bpp; ++n)
                {
                  dst[(z * (dw * dh)) + (y * dw) + (x * bpp) + n] =
                    src[(iz * (sw * sh)) + (iy * sw) + (ix * bpp) + n];
                }
            }
        }
    }
}

static void
scale_volume_image (guchar       *dst,
                    gint          dw,
                    gint          dh,
                    gint          dd,
                    guchar       *src,
                    gint          sw,
                    gint          sh,
                    gint          sd,
                    gint          bpp,
                    filterfunc_t  filter,
                    gfloat        support,
                    wrapfunc_t    wrap,
                    gint          gc,
                    gfloat        gamma)
{
  const gfloat xfactor = (gfloat) dw / (gfloat) sw;
  const gfloat yfactor = (gfloat) dh / (gfloat) sh;
  const gfloat zfactor = (gfloat) dd / (gfloat) sd;

  gint   x, y, z, start, stop, nmax, n, i;
  gfloat center, contrib, density, s, r, t;
  gint   sstride  = sw * bpp;
  gint   zstride  = sh * sw * bpp;
  gfloat xscale   = MIN (xfactor, 1.0f);
  gfloat yscale   = MIN (yfactor, 1.0f);
  gfloat zscale   = MIN (zfactor, 1.0f);
  gfloat xsupport = support / xscale;
  gfloat ysupport = support / yscale;
  gfloat zsupport = support / zscale;
  guchar *d, *row, *col, *slice;
  guchar *tmp1, *tmp2;

  /* down to a 2D image, use the faster 2D image resampler */
  if (dd == 1 && sd == 1)
    {
      scale_image (dst, dw, dh, src, sw, sh, bpp, filter, support, wrap, gc, gamma);
      return;
    }

  if (xsupport <= 0.5f)
    {
      xsupport = 0.5f + 1e-10f;
      xscale = 1.0f;
    }

  if (ysupport <= 0.5f)
    {
      ysupport = 0.5f + 1e-10f;
      yscale = 1.0f;
    }

  if (zsupport <= 0.5f)
    {
      zsupport = 0.5f + 1e-10f;
      zscale = 1.0f;
    }

  tmp1 = g_malloc (sh * sw * bpp);
  tmp2 = g_malloc (dh * sw * bpp);

  for (z = 0; z < dd; ++z)
    {
      /* resample in Z direction */
      d = tmp1;

      center = ((gfloat) z + 0.5f) / zfactor;
      start = (gint) roundf ((center - zsupport) + 0.5f);
      stop =  (gint) roundf ((center + zsupport) + 0.5f);
      nmax = stop - start;
      s = (gfloat) start - center + 0.5f;

#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic)                      \
  private(x, y, slice, i, n, density, r, t, contrib)
#endif
      for (y = 0; y < sh; ++y)
        {
          for (x = 0; x < sw; ++x)
            {
              slice = src + (y * (sw * bpp)) + (x * bpp);

              for (i = 0; i < bpp; ++i)
                {
                  density = 0.0f;
                  r = 0.0f;

                  for (n = 0; n < nmax; ++n)
                    {
                      contrib = filter((s + n) * zscale);
                      density += contrib;
                      if (i == 3)
                        t = (gfloat) slice[(wrap (start + n, sd) * zstride) + i] / 255.0f;
                      else
                        t = gamma_to_linear (gc, (gfloat) slice[(wrap (start + n, sd) * zstride) + i] / 255.0f, gamma);
                      r += t * contrib;
                    }

                  if (density != 0.0f && density != 1.0f)
                    r /= density;

                  r = MIN (1.0f, MAX (0.0f, r));

                  if (i != 3)
                    r = linear_to_gamma (gc, r, gamma);

                  d[((y * sw) + x) * bpp + i] = (guchar) floorf (r * 255.0f + 0.5f);
                }
            }
        }

      /* resample in Y direction */
      d = tmp2;
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic)                              \
  private(x, y, col, center, start, stop, nmax, s, i, n, density, r, t, contrib)
#endif
      for (y = 0; y < dh; ++y)
        {
          center = ((gfloat) y + 0.5f) / yfactor;
          start = (gint) roundf ((center - ysupport) + 0.5f);
          stop =  (gint) roundf ((center + ysupport) + 0.5f);
          nmax = stop - start;
          s = (gfloat) start - center + 0.5f;

          for (x = 0; x < sw; ++x)
            {
              col = tmp1 + (x * bpp);

              for (i = 0; i < bpp; ++i)
                {
                  density = 0.0f;
                  r = 0.0f;

                  for (n = 0; n < nmax; ++n)
                    {
                      contrib = filter((s + n) * yscale);
                      density += contrib;
                      if (i == 3)
                        t = (gfloat) col[(wrap (start + n, sh) * sstride) + i] / 255.0f;
                      else
                        t = gamma_to_linear (gc, (gfloat) col[(wrap (start + n, sh) * sstride) + i] / 255.0f, gamma);
                      r += t * contrib;
                    }

                  if (density != 0.0f && density != 1.0f)
                    r /= density;

                  r = MIN (1.0f, MAX (0.0f, r));

                  if (i != 3)
                    r = linear_to_gamma (gc, r, gamma);

                  d[((y * sw) + x) * bpp + i] = (guchar) floorf (r * 255.0f + 0.5f);
                }
            }
        }

      /* resample in X direction */
      d = dst;
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic)                              \
  private(x, y, row, center, start, stop, nmax, s, i, n, density, r, t, contrib)
#endif
      for (y = 0; y < dh; ++y)
        {
          row = tmp2 + (y * sstride);

          for (x = 0; x < dw; ++x)
            {
              center = ((gfloat) x + 0.5f) / xfactor;
              start = (gint) roundf ((center - xsupport) + 0.5f);
              stop =  (gint) roundf ((center + xsupport) + 0.5f);
              nmax = stop - start;
              s = (gfloat) start - center + 0.5f;

              for (i = 0; i < bpp; ++i)
                {
                  density = 0.0f;
                  r = 0.0f;

                  for (n = 0; n < nmax; ++n)
                    {
                      contrib = filter((s + n) * xscale);
                      density += contrib;
                      if (i == 3)
                        t = (gfloat) row[(wrap (start + n, sw) * bpp) + i] / 255.0f;
                      else
                        t = gamma_to_linear (gc, (gfloat) row[(wrap (start + n, sw) * bpp) + i] / 255.0f, gamma);
                      r += t * contrib;
                    }

                  if (density != 0.0f && density != 1.0f)
                    r /= density;

                  r = MIN (1.0f, MAX (0.0f, r));

                  if (i != 3)
                    r = linear_to_gamma (gc, r, gamma);

                  d[((z * dh * dw) + (y * dw) + x) * bpp + i] = (guchar) floorf (r * 255.0f + 0.5f);
                }
            }
        }
    }

  g_free (tmp1);
  g_free (tmp2);
}


/**
 * Filter Lookup-table
 */

static struct
{
  gint         filter;
  filterfunc_t func;
  gfloat       support;
} filters[] =
{
  { DDS_MIPMAP_FILTER_BOX,       box_filter,       0.5f },
  { DDS_MIPMAP_FILTER_TRIANGLE,  triangle_filter,  1.0f },
  { DDS_MIPMAP_FILTER_QUADRATIC, quadratic_filter, 1.5f },
  { DDS_MIPMAP_FILTER_BSPLINE,   bspline_filter,   2.0f },
  { DDS_MIPMAP_FILTER_MITCHELL,  mitchell_filter,  2.0f },
  { DDS_MIPMAP_FILTER_CATROM,    catrom_filter,    2.0f },
  { DDS_MIPMAP_FILTER_LANCZOS,   lanczos_filter,   3.0f },
  { DDS_MIPMAP_FILTER_KAISER,    kaiser_filter,    3.0f },
  { DDS_MIPMAP_FILTER_MAX,       NULL,             0.0f }
};


/**
 * Alpha-test Coverage - portion of visible texels after alpha test:
 * if (texel_alpha < alpha_test_threshold) discard;
 */

static gfloat
calc_alpha_test_coverage (guchar *src,
                          guint   width,
                          guint   height,
                          gint    bpp,
                          gfloat  alpha_test_threshold,
                          gfloat  alpha_scale)
{
  const gint alpha_channel_idx = 3;
  gint  rowbytes = width * bpp;
  gint  coverage = 0;
  guint x, y;

  if (bpp <= alpha_channel_idx)
    {
      /* No alpha channel */
      return 1.0f;
    }

  for (y = 0; y < height; ++y)
    {
      for (x = 0; x < width; ++x)
        {
          const gfloat alpha = src[y * rowbytes + (x * bpp) + alpha_channel_idx];
          if ((alpha * alpha_scale) >= (alpha_test_threshold * 255))
            {
              ++coverage;
            }
        }
    }

  return (gfloat) coverage / (width * height);
}

static void
scale_alpha_to_coverage (guchar *img,
                         guint   width,
                         guint   height,
                         gint    bpp,
                         gfloat  desired_coverage,
                         gfloat  alpha_test_threshold)
{
  const gint rowbytes          = width * bpp;
  const gint alpha_channel_idx = 3;
  gfloat     min_alpha_scale   = 0.0f;
  gfloat     max_alpha_scale   = 4.0f;
  gfloat     alpha_scale       = 1.0f;
  guint      x, y;
  gint       i;

  if (bpp <= alpha_channel_idx)
    {
      /* No alpha channel */
      return;
    }

  /* Binary search */
  for (i = 0; i < 10; i++)
    {
      gfloat cur_coverage = calc_alpha_test_coverage (img, width, height, bpp, alpha_test_threshold, alpha_scale);

      if (cur_coverage < desired_coverage)
        {
          min_alpha_scale = alpha_scale;
        }
      else if (cur_coverage > desired_coverage)
        {
          max_alpha_scale = alpha_scale;
        }
      else
        {
          break;
        }

      alpha_scale = (min_alpha_scale + max_alpha_scale) / 2;
    }

  /* Scale alpha channel */
  for (y = 0; y < height; ++y)
    {
      for (x = 0; x < width; ++x)
        {
          gfloat new_alpha = img[y * rowbytes + (x * bpp) + alpha_channel_idx] * alpha_scale;
          if (new_alpha > 255.0f)
            {
              new_alpha = 255.0f;
            }

          img[y * rowbytes + (x * bpp) + alpha_channel_idx] = (guchar) new_alpha;
        }
    }
}


/**
 * Mipmap Generation
 */

gint
generate_mipmaps (guchar *dst,
                  guchar *src,
                  guint   width,
                  guint   height,
                  gint    bpp,
                  gint    indexed,
                  gint    mipmaps,
                  gint    filter,
                  gint    wrap,
                  gint    gc,
                  gfloat  gamma,
                  gint    preserve_alpha_coverage,
                  gfloat  alpha_test_threshold)
{
  const gint   has_alpha   = (bpp >= 3);
  mipmapfunc_t mipmap_func = NULL;
  filterfunc_t filter_func = NULL;
  wrapfunc_t   wrap_func   = NULL;
  gfloat       coverage    = 1.0f;
  gfloat       support     = 0.0f;
  guint        sw, sh, dw, dh;
  guchar      *s, *d;
  gint         i;

  if (indexed || filter == DDS_MIPMAP_FILTER_NEAREST)
    {
      mipmap_func = scale_image_nearest;
    }
  else
    {
      if ((filter < DDS_MIPMAP_FILTER_NEAREST) ||
          (filter >= DDS_MIPMAP_FILTER_MAX))
        filter = DDS_MIPMAP_FILTER_BOX;

      mipmap_func = scale_image;

      for (i = 0; filters[i].filter != DDS_MIPMAP_FILTER_MAX; ++i)
        {
          if (filter == filters[i].filter)
            {
              filter_func = filters[i].func;
              support = filters[i].support;
              break;
            }
        }
    }

  switch (wrap)
    {
    case DDS_MIPMAP_WRAP_MIRROR: wrap_func = wrap_mirror; break;
    case DDS_MIPMAP_WRAP_REPEAT: wrap_func = wrap_repeat; break;
    case DDS_MIPMAP_WRAP_CLAMP:  wrap_func = wrap_clamp;  break;
    default:                     wrap_func = wrap_clamp;  break;
    }

  if (has_alpha && preserve_alpha_coverage)
    {
      coverage = calc_alpha_test_coverage (src, width, height, bpp,
                                           alpha_test_threshold,
                                           1.0f);
    }

  memcpy (dst, src, width * height * bpp);

  s = dst;
  d = dst + (width * height * bpp);

  dw = sw = width;
  dh = sh = height;

  for (i = 1; i < mipmaps; ++i)
    {
      dw = MAX (1, dw >> 1);
      dh = MAX (1, dh >> 1);

      mipmap_func (d, dw, dh, s, sw, sh, bpp, filter_func, support, wrap_func, gc, gamma);

      if (has_alpha && preserve_alpha_coverage)
        {
          scale_alpha_to_coverage (d, dw, dh, bpp, coverage, alpha_test_threshold);
        }

      s = d;
      sw = dw;
      sh = dh;
      d += (dw * dh * bpp);
    }

  return 1;
}

gint
generate_volume_mipmaps (guchar *dst,
                         guchar *src,
                         guint   width,
                         guint   height,
                         guint   depth,
                         gint    bpp,
                         gint    indexed,
                         gint    mipmaps,
                         gint    filter,
                         gint    wrap,
                         gint    gc,
                         gfloat  gamma)
{
  volmipmapfunc_t mipmap_func = NULL;
  filterfunc_t    filter_func = NULL;
  wrapfunc_t      wrap_func   = NULL;
  gfloat          support     = 0.0f;
  guint           sw, sh, sd;
  guint           dw, dh, dd;
  guchar         *s, *d;
  gint            i;

  if (indexed || filter == DDS_MIPMAP_FILTER_NEAREST)
    {
      mipmap_func = scale_volume_image_nearest;
    }
  else
    {
      if ((filter < DDS_MIPMAP_FILTER_NEAREST) ||
          (filter >= DDS_MIPMAP_FILTER_MAX))
        filter = DDS_MIPMAP_FILTER_BOX;

      mipmap_func = scale_volume_image;

      for (i = 0; filters[i].filter != DDS_MIPMAP_FILTER_MAX; ++i)
        {
          if (filter == filters[i].filter)
            {
              filter_func = filters[i].func;
              support = filters[i].support;
              break;
            }
        }
    }

  switch (wrap)
    {
    case DDS_MIPMAP_WRAP_MIRROR: wrap_func = wrap_mirror; break;
    case DDS_MIPMAP_WRAP_REPEAT: wrap_func = wrap_repeat; break;
    case DDS_MIPMAP_WRAP_CLAMP:  wrap_func = wrap_clamp;  break;
    default:                     wrap_func = wrap_clamp;  break;
    }

  memcpy (dst, src, width * height * depth * bpp);

  s = dst;
  d = dst + (width * height * depth * bpp);

  sw = width;
  sh = height;
  sd = depth;

  for (i = 1; i < mipmaps; ++i)
    {
      dw = MAX (1, sw >> 1);
      dh = MAX (1, sh >> 1);
      dd = MAX (1, sd >> 1);

      mipmap_func (d, dw, dh, dd, s, sw, sh, sd, bpp, filter_func, support, wrap_func, gc, gamma);

      s = d;
      sw = dw;
      sh = dh;
      sd = dd;
      d += (dw * dh * dd * bpp);
    }

  return 1;
}
