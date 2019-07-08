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

#ifdef _OPENMP
#include <omp.h>
#endif

#include "dds.h"
#include "mipmap.h"
#include "imath.h"
#include "color.h"

typedef float (*filterfunc_t)(float);
typedef int   (*wrapfunc_t)(int, int);
typedef void  (*mipmapfunc_t)(unsigned char *, int, int, unsigned char *, int, int, int, filterfunc_t, float, wrapfunc_t, int, float);
typedef void  (*volmipmapfunc_t)(unsigned char *, int, int, int, unsigned char *, int, int, int, int, filterfunc_t, float, wrapfunc_t, int, float);

/******************************************************************************
 * size functions                                                             *
 ******************************************************************************/

int
get_num_mipmaps (int width,
                 int height)
{
  int w = width << 1;
  int h = height << 1;
  int n = 0;

  while (w != 1 || h != 1)
    {
      if (w > 1) w >>= 1;
      if (h > 1) h >>= 1;
      ++n;
    }

  return n;
}

unsigned int
get_mipmapped_size (int width,
                    int height,
                    int bpp,
                    int level,
                    int num,
                    int format)
{
  int w, h, n = 0;
  unsigned int size = 0;

  w = width >> level;
  h = height >> level;
  w = MAX(1, w);
  h = MAX(1, h);
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

unsigned int
get_volume_mipmapped_size (int width,
                           int height,
                           int depth,
                           int bpp,
                           int level,
                           int num,
                           int format)
{
  int w, h, d, n = 0;
  unsigned int size = 0;

  w = width >> level;
  h = height >> level;
  d = depth >> level;
  w = MAX(1, w);
  h = MAX(1, h);
  d = MAX(1, d);
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

int
get_next_mipmap_dimensions (int *next_w,
                            int *next_h,
                            int  curr_w,
                            int  curr_h)
{
  if (curr_w == 1 || curr_h == 1)
    return 0;

  if (next_w) *next_w = curr_w >> 1;
  if (next_h) *next_h = curr_h >> 1;

  return 1;
}

/******************************************************************************
 * wrap modes                                                                 *
 ******************************************************************************/

static int
wrap_mirror (int x,
             int max)
{
  if (max == 1) x = 0;
  x = abs(x);
  while (x >= max)
    x = abs(max + max - x - 2);

  return x;
}

static int
wrap_repeat (int x,
             int max)
{
  if (x >= 0)
    return x % max;

  return (x + 1) % max + max - 1;
}

static int
wrap_clamp (int x,
            int max)
{
  return MAX(0, MIN(max - 1, x));
}

/******************************************************************************
 * gamma-correction                                                           *
 ******************************************************************************/

static int
linear_to_gamma (int   gc,
                 int   v,
                 float gamma)
{
  if (gc == 1)
    {
      v = (int)(powf((float)v / 255.0f, gamma) * 255);
      if (v > 255) v = 255;
    }
  else if (gc == 2)
    {
      v = linear_to_sRGB(v);
    }

  return v;
}

static int
gamma_to_linear (int   gc,
                 int   v,
                 float gamma)
{
  if (gc == 1)
    {
      v = (int)(powf((float)v / 255.0f, 1.0f / gamma) * 255);
      if(v > 255) v = 255;
    }
  else if (gc == 2)
    {
      v = sRGB_to_linear(v);
    }

  return v;
}

/******************************************************************************
 * filters                                                                    *
 ******************************************************************************/

static float
box_filter (float t)
{
  if ((t >= -0.5f) && (t < 0.5f))
    return 1.0f;

  return 0.0f;
}

static float
triangle_filter (float t)
{
  if (t < 0.0f) t = -t;
  if (t < 1.0f) return 1.0f - t;

  return 0.0f;
}

static float
quadratic_filter (float t)
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

static float
bspline_filter (float t)
{
  float tt;

  if (t < 0.0f)
    t = -t;

  if (t < 1.0f)
    {
      tt = t * t;
      return ((0.5f * tt * t) - tt + (2.0f / 3.0f));
    }
  else if (t < 2.0f)
    {
      t = 2.0f - t;
      return (1.0f / 6.0f) * (t * t * t);
    }

  return 0.0f;
}

static float
mitchell (float       t,
          const float B,
          const float C)
{
  float tt;

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

static float
mitchell_filter (float t)
{
  return mitchell(t, 1.0f / 3.0f, 1.0f / 3.0f);
}

static float
sinc (float x)
{
  x = (x * M_PI);
  if (fabsf(x) < 1e-04f)
    return 1.0f + x * x * (-1.0f / 6.0f + x * x * 1.0f / 120.0f);

  return sinf(x) / x;
}

static float
lanczos_filter (float t)
{
  if (t < 0.0f) t = -t;
  if (t < 3.0f) return sinc(t) * sinc(t / 3.0f);

  return 0.0f;
}

static float
bessel0 (float x)
{
  const float EPSILON = 1e-6f;
  float xh, sum, pow, ds;
  int k;

  xh = 0.5f * x;
  sum = 1.0f;
  pow = 1.0f;
  k = 0;
  ds = 1.0f;

  while (ds > sum * EPSILON)
    {
      ++k;
      pow = pow * (xh / k);
      ds = pow * pow;
      sum += ds;
    }

  return sum;
}

static float
kaiser_filter (float t)
{
  if (t < 0.0f) t = -t;

  if (t < 3.0f)
    {
      const float alpha = 4.0f;
      const float rb04 = 0.0884805322f; // 1.0f / bessel0(4.0f);
      const float ratio = t / 3.0f;
      if ((1.0f - ratio * ratio) >= 0)
        return sinc(t) * bessel0(alpha * sqrtf(1.0f - ratio * ratio)) * rb04;
    }

  return 0.0f;
}

/******************************************************************************
 * 2D image scaling                                                           *
 ******************************************************************************/

static void
scale_image_nearest (unsigned char *dst,
                     int            dw,
                     int            dh,
                     unsigned char *src,
                     int            sw,
                     int            sh,
                     int            bpp,
                     filterfunc_t   filter,
                     float          support,
                     wrapfunc_t     wrap,
                     int            gc,
                     float          gamma)
{
  int n, x, y;
  int ix, iy;
  int srowbytes = sw * bpp;
  int drowbytes = dw * bpp;

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
scale_image (unsigned char *dst,
             int            dw,
             int            dh,
             unsigned char *src,
             int            sw,
             int            sh,
             int            bpp,
             filterfunc_t   filter,
             float          support,
             wrapfunc_t     wrap,
             int            gc,
             float          gamma)
{
  const float blur = 1.0f;
  const float xfactor = (float)dw / (float)sw;
  const float yfactor = (float)dh / (float)sh;

  int x, y, start, stop, nmax, n, i;
  int sstride = sw * bpp;
  float center, contrib, density, s, r, t;

  unsigned char *d, *row, *col;

  float xscale = MIN(xfactor, 1.0f) / blur;
  float yscale = MIN(yfactor, 1.0f) / blur;
  float xsupport = support / xscale;
  float ysupport = support / yscale;
  unsigned char *tmp;

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
  tmp = g_malloc(sw * bpp * omp_get_max_threads());
#else
  tmp = g_malloc(sw * bpp);
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
      d += (sw * bpp * omp_get_thread_num());
#endif

      center = ((float)y + 0.5f) / yfactor;
      start = (int)(center - ysupport + 0.5f);
      stop  = (int)(center + ysupport + 0.5f);
      nmax = stop - start;
      s = (float)start - center + 0.5f;

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
                    t = col[(wrap(start + n, sh) * sstride) + i];
                  else
                    t = linear_to_gamma(gc, col[(wrap(start + n, sh) * sstride) + i], gamma);
                  r += t * contrib;
                }

              if (density != 0.0f && density != 1.0f)
                r /= density;

              r = MIN(255, MAX(0, r));

              if (i != 3)
                r = gamma_to_linear(gc, r, gamma);

              d[(x * bpp) + i] = (unsigned char)r;
            }
        }

      /* resample in X direction using temp buffer */
      row = d;
      d = dst;

      for (x = 0; x < dw; ++x)
        {
          center = ((float)x + 0.5f) / xfactor;
          start = (int)(center - xsupport + 0.5f);
          stop  = (int)(center + xsupport + 0.5f);
          nmax = stop - start;
          s = (float)start - center + 0.5f;

          for (i = 0; i < bpp; ++i)
            {
              density = 0.0f;
              r = 0.0f;

              for (n = 0; n < nmax; ++n)
                {
                  contrib = filter((s + n) * xscale);
                  density += contrib;
                  if (i == 3)
                    t = row[(wrap(start + n, sw) * bpp) + i];
                  else
                    t = linear_to_gamma(gc, row[(wrap(start + n, sw) * bpp) + i], gamma);
                  r += t * contrib;
                }

              if (density != 0.0f && density != 1.0f)
                r /= density;

              r = MIN(255, MAX(0, r));

              if (i != 3)
                r = gamma_to_linear(gc, r, gamma);

              d[(y * (dw * bpp)) + (x * bpp) + i] = (unsigned char)r;
            }
        }
    }

  g_free (tmp);
}

/******************************************************************************
 * 3D image scaling                                                           *
 ******************************************************************************/

static void
scale_volume_image_nearest (unsigned char *dst,
                            int            dw,
                            int            dh,
                            int            dd,
                            unsigned char *src,
                            int            sw,
                            int            sh,
                            int            sd,
                            int            bpp,
                            filterfunc_t   filter,
                            float          support,
                            wrapfunc_t     wrap,
                            int            gc,
                            float          gamma)
{
  int n, x, y, z;
  int ix, iy, iz;

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
scale_volume_image (unsigned char *dst,
                    int            dw,
                    int            dh,
                    int            dd,
                    unsigned char *src,
                    int            sw,
                    int            sh,
                    int            sd,
                    int            bpp,
                    filterfunc_t   filter,
                    float          support,
                    wrapfunc_t     wrap,
                    int            gc,
                    float          gamma)
{
  const float blur = 1.0f;
  const float xfactor = (float)dw / (float)sw;
  const float yfactor = (float)dh / (float)sh;
  const float zfactor = (float)dd / (float)sd;

  int x, y, z, start, stop, nmax, n, i;
  int sstride = sw * bpp;
  int zstride = sh * sw * bpp;
  float center, contrib, density, s, r, t;

  unsigned char *d, *row, *col, *slice;

  float xscale = MIN(xfactor, 1.0f) / blur;
  float yscale = MIN(yfactor, 1.0f) / blur;
  float zscale = MIN(zfactor, 1.0f) / blur;
  float xsupport = support / xscale;
  float ysupport = support / yscale;
  float zsupport = support / zscale;
  unsigned char *tmp1, *tmp2;

  /* down to a 2D image, use the faster 2D image resampler */
  if (dd == 1 && sd == 1)
    {
      scale_image(dst, dw, dh, src, sw, sh, bpp, filter, support, wrap, gc, gamma);
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

  tmp1 = g_malloc(sh * sw * bpp);
  tmp2 = g_malloc(dh * sw * bpp);

  for (z = 0; z < dd; ++z)
    {
      /* resample in Z direction */
      d = tmp1;

      center = ((float)z + 0.5f) / zfactor;
      start = (int)(center - zsupport + 0.5f);
      stop =  (int)(center + zsupport + 0.5f);
      nmax = stop - start;
      s = (float)start - center + 0.5f;

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
                        t = slice[(wrap(start + n, sd) * zstride) + i];
                      else
                        t = linear_to_gamma(gc, slice[(wrap(start + n, sd) * zstride) + i], gamma);
                      r += t * contrib;
                    }

                  if (density != 0.0f && density != 1.0f)
                    r /= density;

                  r = MIN(255, MAX(0, r));

                  if (i != 3)
                    r = gamma_to_linear(gc, r, gamma);

                  d[((y * sw) + x) * bpp + i] = (unsigned char)r;
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
          center = ((float)y + 0.5f) / yfactor;
          start = (int)(center - ysupport + 0.5f);
          stop =  (int)(center + ysupport + 0.5f);
          nmax = stop - start;
          s = (float)start - center + 0.5f;

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
                        t = col[(wrap(start + n, sh) * sstride) + i];
                      else
                        t = linear_to_gamma(gc, col[(wrap(start + n, sh) * sstride) + i], gamma);
                      r += t * contrib;
                    }

                  if (density != 0.0f && density != 1.0f)
                    r /= density;

                  r = MIN(255, MAX(0, r));

                  if (i != 3)
                    r = gamma_to_linear(gc, r, gamma);

                  d[((y * sw) + x) * bpp + i] = (unsigned char)r;
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
              center = ((float)x + 0.5f) / xfactor;
              start = (int)(center - xsupport + 0.5f);
              stop =  (int)(center + xsupport + 0.5f);
              nmax = stop - start;
              s = (float)start - center + 0.5f;

              for (i = 0; i < bpp; ++i)
                {
                  density = 0.0f;
                  r = 0.0f;

                  for (n = 0; n < nmax; ++n)
                    {
                      contrib = filter((s + n) * xscale);
                      density += contrib;
                      if (i == 3)
                        t = row[(wrap(start + n, sw) * bpp) + i];
                      else
                        t = linear_to_gamma(gc, row[(wrap(start + n, sw) * bpp) + i], gamma);
                      r += t * contrib;
                    }

                  if (density != 0.0f && density != 1.0f)
                    r /= density;

                  r = MIN(255, MAX(0, r));

                  if (i != 3)
                    r = gamma_to_linear(gc, r, gamma);

                  d[((z * dh * dw) + (y * dw) + x) * bpp + i] = (unsigned char)r;
                }
            }
        }
    }

  g_free (tmp1);
  g_free (tmp2);
}

/******************************************************************************
 * filter lookup table                                                        *
 ******************************************************************************/

static struct
{
  int filter;
  filterfunc_t func;
  float support;
} filters[] =
{
  { DDS_MIPMAP_FILTER_BOX,       box_filter,       0.5f },
  { DDS_MIPMAP_FILTER_TRIANGLE,  triangle_filter,  1.0f },
  { DDS_MIPMAP_FILTER_QUADRATIC, quadratic_filter, 1.5f },
  { DDS_MIPMAP_FILTER_BSPLINE,   bspline_filter,   2.0f },
  { DDS_MIPMAP_FILTER_MITCHELL,  mitchell_filter,  2.0f },
  { DDS_MIPMAP_FILTER_LANCZOS,   lanczos_filter,   3.0f },
  { DDS_MIPMAP_FILTER_KAISER,    kaiser_filter,    3.0f },
  { DDS_MIPMAP_FILTER_MAX,       NULL,             0.0f }
};

/*
 * Alpha test coverage - portion of visible texels after alpha test:
 *   if (texel_alpha < alpha_test_threshold)
 *      discard;
 */
static float
calc_alpha_test_coverage (unsigned char *src,
                          unsigned int   width,
                          unsigned int   height,
                          int            bpp,
                          float          alpha_test_threshold,
                          float          alpha_scale)
{
  unsigned int x, y;
  int rowbytes = width * bpp;
  int coverage = 0;
  const int alpha_channel_idx = 3;

  if (bpp <= alpha_channel_idx)
    {
      /* No alpha channel */
      return 1.f;
    }

  for (y = 0; y < height; ++y)
    {
      for (x = 0; x < width; ++x)
        {
          const float alpha = src[y * rowbytes + (x * bpp) + alpha_channel_idx];
          if ((alpha * alpha_scale) >= (alpha_test_threshold * 255))
            {
              ++coverage;
            }
        }
    }

  return (float)coverage / (width * height);
}

static void
scale_alpha_to_coverage (unsigned char *img,
                         unsigned int   width,
                         unsigned int   height,
                         int            bpp,
                         float          desired_coverage,
                         float          alpha_test_threshold)
{
  int i;
  unsigned int x, y;
  const int rowbytes = width * bpp;
  const int alpha_channel_idx = 3;
  float min_alpha_scale = 0.0f;
  float max_alpha_scale = 4.0f;
  float alpha_scale = 1.0f;

  if (bpp <= alpha_channel_idx)
    {
      /* No alpha channel */
      return;
    }

  /* Binary search */
  for (i = 0; i < 10; i++)
    {
      float cur_coverage = calc_alpha_test_coverage(img, width, height, bpp, alpha_test_threshold, alpha_scale);

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
          float new_alpha = img[y * rowbytes + (x * bpp) + alpha_channel_idx] * alpha_scale;
          if (new_alpha > 255.0f)
            {
              new_alpha = 255.0f;
            }

          img[y * rowbytes + (x * bpp) + alpha_channel_idx] = (unsigned char)new_alpha;
        }
    }
}

/******************************************************************************
 * mipmap generation                                                          *
 ******************************************************************************/

int
generate_mipmaps (unsigned char *dst,
                  unsigned char *src,
                  unsigned int   width,
                  unsigned int   height,
                  int            bpp,
                  int            indexed,
                  int            mipmaps,
                  int            filter,
                  int            wrap,
                  int            gc,
                  float          gamma,
                  int            preserve_alpha_coverage,
                  float          alpha_test_threshold)
{
  int i;
  unsigned int sw, sh, dw, dh;
  unsigned char *s, *d;
  mipmapfunc_t mipmap_func = NULL;
  filterfunc_t filter_func = NULL;
  wrapfunc_t wrap_func = NULL;
  float support = 0.0f;
  const int has_alpha = (bpp >= 3);
  float alpha_test_coverage = 1;

  if (indexed || filter == DDS_MIPMAP_FILTER_NEAREST)
    {
      mipmap_func = scale_image_nearest;
    }
  else
    {
      if ((filter <= DDS_MIPMAP_FILTER_DEFAULT) ||
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
      alpha_test_coverage = calc_alpha_test_coverage(src, width, height, bpp,
                                                     alpha_test_threshold,
                                                     1.0f);
    }

  memcpy (dst, src, width * height * bpp);

  s = dst;
  d = dst + (width * height * bpp);

  sw = width;
  sh = height;

  for (i = 1; i < mipmaps; ++i)
    {
      dw = MAX(1, sw >> 1);
      dh = MAX(1, sh >> 1);

      mipmap_func(d, dw, dh, s, sw, sh, bpp, filter_func, support, wrap_func, gc, gamma);

      if (has_alpha && preserve_alpha_coverage)
        {
          scale_alpha_to_coverage(d, dw, dh, bpp, alpha_test_coverage, alpha_test_threshold);
        }

      s = d;
      sw = dw;
      sh = dh;
      d += (dw * dh * bpp);
    }

  return 1;
}

int
generate_volume_mipmaps (unsigned char *dst,
                         unsigned char *src,
                         unsigned int   width,
                         unsigned int   height,
                         unsigned int   depth,
                         int            bpp,
                         int            indexed,
                         int            mipmaps,
                         int            filter,
                         int            wrap,
                         int            gc,
                         float          gamma)
{
  int i;
  unsigned int sw, sh, sd;
  unsigned int dw, dh, dd;
  unsigned char *s, *d;
  volmipmapfunc_t mipmap_func = NULL;
  filterfunc_t filter_func = NULL;
  wrapfunc_t wrap_func = NULL;
  float support = 0.0f;

  if (indexed || filter == DDS_MIPMAP_FILTER_NEAREST)
    {
      mipmap_func = scale_volume_image_nearest;
    }
  else
    {
      if ((filter <= DDS_MIPMAP_FILTER_DEFAULT) ||
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
      dw = MAX(1, sw >> 1);
      dh = MAX(1, sh >> 1);
      dd = MAX(1, sd >> 1);

      mipmap_func (d, dw, dh, dd, s, sw, sh, sd, bpp, filter_func, support, wrap_func, gc, gamma);

      s = d;
      sw = dw;
      sh = dh;
      sd = dd;
      d += (dw * dh * dd * bpp);
    }

  return 1;
}
