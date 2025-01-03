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

/*
 * Parts of this code have been generously released in the public domain
 * by Fabian 'ryg' Giesen.  The original code can be found (at the time
 * of writing) here:  http://mollyrocket.com/forums/viewtopic.php?t=392
 *
 * For more information about this code, see the README.dxt file that
 * came with the source.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>

#include <libgimp/gimp.h>

#include "bc7.h"
#include "dds.h"
#include "dxt.h"
#include "endian_rw.h"
#include "imath.h"
#include "mipmap.h"
#include "misc.h"
#include "vec.h"

#include "dxt_tables.h"

#define SWAP(a, b)  do { typeof(a) t; t = a; a = b; b = t; } while(0)

/* SIMD constants */
static const vec4_t V4ZERO      = VEC4_CONST1(0.0f);
static const vec4_t V4ONE       = VEC4_CONST1(1.0f);
static const vec4_t V4HALF      = VEC4_CONST1(0.5f);
static const vec4_t V4ONETHIRD  = VEC4_CONST3(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f);
static const vec4_t V4TWOTHIRDS = VEC4_CONST3(2.0f / 3.0f, 2.0f / 3.0f, 2.0f / 3.0f);
static const vec4_t V4GRID      = VEC4_CONST3(31.0f, 63.0f, 31.0f);
static const vec4_t V4GRIDRCP   = VEC4_CONST3(1.0f / 31.0f, 1.0f / 63.0f, 1.0f / 31.0f);
static const vec4_t V4EPSILON   = VEC4_CONST1(1e-04f);

typedef struct
{
  unsigned int single;
  unsigned int alphamask;
  vec4_t points[16];
  vec4_t palette[4];
  vec4_t max;
  vec4_t min;
  vec4_t metric;
} dxtblock_t;

/* extract 4x4 BGRA block */
static void
extract_block (const unsigned char *src,
               int                  x,
               int                  y,
               int                  w,
               int                  h,
               unsigned char       *block)
{
  int i, j;
  int bw = MIN(w - x, 4);
  int bh = MIN(h - y, 4);
  int bx, by;
  const int rem[] =
  {
    0, 0, 0, 0,
    0, 1, 0, 1,
    0, 1, 2, 0,
    0, 1, 2, 3
  };

  for (i = 0; i < 4; ++i)
    {
      by = rem[(bh - 1) * 4 + i] + y;
      for (j = 0; j < 4; ++j)
        {
          bx = rem[(bw - 1) * 4 + j] + x;
          block[(i * 4 * 4) + (j * 4) + 0] =
            src[(by * (w * 4)) + (bx * 4) + 0];
          block[(i * 4 * 4) + (j * 4) + 1] =
            src[(by * (w * 4)) + (bx * 4) + 1];
          block[(i * 4 * 4) + (j * 4) + 2] =
            src[(by * (w * 4)) + (bx * 4) + 2];
          block[(i * 4 * 4) + (j * 4) + 3] =
            src[(by * (w * 4)) + (bx * 4) + 3];
        }
    }
}

#if 0
/* Currently unused, hidden to avoid compilation warnings. */

/* pack BGR8 to RGB565 */
static inline unsigned short
pack_rgb565 (const unsigned char *c)
{
  return (mul8bit(c[2], 31) << 11) |
         (mul8bit(c[1], 63) <<  5) |
         (mul8bit(c[0], 31)      );
}
#endif

/* unpack RGB565 to BGR */
static void
unpack_rgb565 (unsigned char  *dst,
               unsigned short  v)
{
  int r = (v >> 11) & 0x1f;
  int g = (v >>  5) & 0x3f;
  int b = (v      ) & 0x1f;

  dst[0] = (b << 3) | (b >> 2);
  dst[1] = (g << 2) | (g >> 4);
  dst[2] = (r << 3) | (r >> 2);
}

/* linear interpolation at 1/3 point between a and b */
static void
lerp_rgb13 (unsigned char *dst,
            unsigned char *a,
            unsigned char *b)
{
#if 0
  dst[0] = blerp(a[0], b[0], 0x55);
  dst[1] = blerp(a[1], b[1], 0x55);
  dst[2] = blerp(a[2], b[2], 0x55);
#else
  /*
   * according to the S3TC/DX10 specs, this is the correct way to do the
   * interpolation (with no rounding bias)
   *
   * dst = (2 * a + b) / 3;
   */
  dst[0] = (2 * a[0] + b[0]) / 3;
  dst[1] = (2 * a[1] + b[1]) / 3;
  dst[2] = (2 * a[2] + b[2]) / 3;
#endif
}

static void
vec4_endpoints_to_565 (int          *start,
                       int          *end,
                       const vec4_t  a,
                       const vec4_t  b)
{
  int c[8] __attribute__((aligned(16)));
  vec4_t ta = a * V4GRID + V4HALF;
  vec4_t tb = b * V4GRID + V4HALF;

#ifdef USE_SSE
# ifdef __SSE2__
  const __m128i C565 = _mm_setr_epi16(31, 63, 31, 0, 31, 63, 31, 0);
  __m128i ia = _mm_cvttps_epi32(ta);
  __m128i ib = _mm_cvttps_epi32(tb);
  __m128i zero = _mm_setzero_si128();
  __m128i words = _mm_packs_epi32(ia, ib);
  words = _mm_min_epi16(C565, _mm_max_epi16(zero, words));
  *((__m128i *)&c[0]) = _mm_unpacklo_epi16(words, zero);
  *((__m128i *)&c[4]) = _mm_unpackhi_epi16(words, zero);
# else
  const __m64 C565 = _mm_setr_pi16(31, 63, 31, 0);
  __m64 lo, hi, c0, c1;
  __m64 zero = _mm_setzero_si64();
  lo = _mm_cvttps_pi32(ta);
  hi = _mm_cvttps_pi32(_mm_movehl_ps(ta, ta));
  c0 = _mm_packs_pi32(lo, hi);
  lo = _mm_cvttps_pi32(tb);
  hi = _mm_cvttps_pi32(_mm_movehl_ps(tb, tb));
  c1 = _mm_packs_pi32(lo, hi);
  c0 = _mm_min_pi16(C565, _mm_max_pi16(zero, c0));
  c1 = _mm_min_pi16(C565, _mm_max_pi16(zero, c1));
  *((__m64 *)&c[0]) = _mm_unpacklo_pi16(c0, zero);
  *((__m64 *)&c[2]) = _mm_unpackhi_pi16(c0, zero);
  *((__m64 *)&c[4]) = _mm_unpacklo_pi16(c1, zero);
  *((__m64 *)&c[6]) = _mm_unpackhi_pi16(c1, zero);
  _mm_empty();
# endif
#else
  c[0] = (int)ta[0]; c[4] = (int)tb[0];
  c[1] = (int)ta[1]; c[5] = (int)tb[1];
  c[2] = (int)ta[2]; c[6] = (int)tb[2];
  c[0] = MIN(31, MAX(0, c[0]));
  c[1] = MIN(63, MAX(0, c[1]));
  c[2] = MIN(31, MAX(0, c[2]));
  c[4] = MIN(31, MAX(0, c[4]));
  c[5] = MIN(63, MAX(0, c[5]));
  c[6] = MIN(31, MAX(0, c[6]));
#endif

  *start = ((c[2] << 11) | (c[1] << 5) | c[0]);
  *end   = ((c[6] << 11) | (c[5] << 5) | c[4]);
}

static void
dxtblock_init (dxtblock_t          *dxtb,
               const unsigned char *block,
               int                  flags)
{
  int i, c0, c;
  int bc1 = (flags & DXT_BC1);
  float x, y, z;
  vec4_t min, max, center, t, cov, inset;

  dxtb->single = 1;
  dxtb->alphamask = 0;

  if(flags & DXT_PERCEPTUAL)
    /* ITU-R BT.709 luma coefficients */
    dxtb->metric = vec4_set(0.2126f, 0.7152f, 0.0722f, 0.0f);
  else
    dxtb->metric = vec4_set(1.0f, 1.0f, 1.0f, 0.0f);

  c0 = GETL24(block);

  for (i = 0; i < 16; ++i)
    {
      if (bc1 && (block[4 * i + 3] < 128))
        dxtb->alphamask |= (3 << (2 * i));

      x = (float)block[4 * i + 0] / 255.0f;
      y = (float)block[4 * i + 1] / 255.0f;
      z = (float)block[4 * i + 2] / 255.0f;

      dxtb->points[i] = vec4_set(x, y, z, 0);

      c = GETL24(&block[4 * i]);
      dxtb->single = dxtb->single && (c == c0);
    }

  // no need to continue if this is a single color block
  if (dxtb->single)
    return;

  min = vec4_set1(1.0f);
  max = vec4_zero();

  // get bounding box extents
  for (i = 0; i < 16; ++i)
    {
      min = vec4_min(min, dxtb->points[i]);
      max = vec4_max(max, dxtb->points[i]);
    }

  // select diagonal
  center = (max + min) * V4HALF;
  cov = vec4_zero();
  for (i = 0; i < 16; ++i)
    {
      t = dxtb->points[i] - center;
      cov += t * vec4_splatz(t);
    }

#ifdef USE_SSE
  {
    __m128 mask, tmp;
    // get mask
    mask = _mm_cmplt_ps(cov, _mm_setzero_ps());
    // clear high bits (z, w)
    mask = _mm_movelh_ps(mask, _mm_setzero_ps());
    // mask and combine
    tmp = _mm_or_ps(_mm_and_ps(mask, min), _mm_andnot_ps(mask, max));
    min = _mm_or_ps(_mm_and_ps(mask, max), _mm_andnot_ps(mask, min));
    max = tmp;
  }
#else
  {
    float x0, x1, y0, y1;
    x0 = max[0];
    y0 = max[1];
    x1 = min[0];
    y1 = min[1];

    if (cov[0] < 0) SWAP(x0, x1);
    if (cov[1] < 0) SWAP(y0, y1);

    max[0] = x0;
    max[1] = y0;
    min[0] = x1;
    min[1] = y1;
  }
#endif

  // inset bounding box and clamp to [0,1]
  inset = (max - min) * vec4_set1(1.0f / 16.0f) - vec4_set1((8.0f / 255.0f) / 16.0f);
  max = vec4_min(V4ONE, vec4_max(V4ZERO, max - inset));
  min = vec4_min(V4ONE, vec4_max(V4ZERO, min + inset));

  // clamp to color space and save
  dxtb->max = vec4_trunc(V4GRID * max + V4HALF) * V4GRIDRCP;
  dxtb->min = vec4_trunc(V4GRID * min + V4HALF) * V4GRIDRCP;
}

static void
construct_palette3 (dxtblock_t *dxtb)
{
  dxtb->palette[0] = dxtb->max;
  dxtb->palette[1] = dxtb->min;
  dxtb->palette[2] = (dxtb->max * V4HALF) + (dxtb->min * V4HALF);
  dxtb->palette[3] = vec4_zero();
}

static void
construct_palette4 (dxtblock_t *dxtb)
{
  dxtb->palette[0] = dxtb->max;
  dxtb->palette[1] = dxtb->min;
  dxtb->palette[2] = (dxtb->max * V4TWOTHIRDS) + (dxtb->min * V4ONETHIRD );
  dxtb->palette[3] = (dxtb->max * V4ONETHIRD ) + (dxtb->min * V4TWOTHIRDS);
}

/*
 * from nvidia-texture-tools; see LICENSE.nvtt for copyright information
 */
static void
optimize_endpoints3 (dxtblock_t   *dxtb,
                     unsigned int  indices,
                     vec4_t       *max,
                     vec4_t       *min)
{
  float alpha, beta;
  vec4_t alpha2_sum, alphax_sum;
  vec4_t beta2_sum, betax_sum;
  vec4_t alphabeta_sum, a, b, factor;
  int i, bits;

  alpha2_sum = beta2_sum = alphabeta_sum = vec4_zero();
  alphax_sum = vec4_zero();
  betax_sum = vec4_zero();

  for (i = 0; i < 16; ++i)
    {
      bits = indices >> (2 * i);

      // skip alpha pixels
      if ((bits & 3) == 3)
        continue;

      beta = (float)(bits & 1);
      if (bits & 2)
        beta = 0.5f;
      alpha = 1.0f - beta;

      a = vec4_set1(alpha);
      b = vec4_set1(beta);
      alpha2_sum += a * a;
      beta2_sum += b * b;
      alphabeta_sum += a * b;
      alphax_sum += dxtb->points[i] * a;
      betax_sum  += dxtb->points[i] * b;
    }

  factor = alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum;
  if (vec4_cmplt(factor, V4EPSILON))
    return;
  factor = vec4_rcp(factor);

  a = (alphax_sum * beta2_sum  - betax_sum  * alphabeta_sum) * factor;
  b = (betax_sum  * alpha2_sum - alphax_sum * alphabeta_sum) * factor;

  // clamp to the color space
  a = vec4_min(V4ONE, vec4_max(V4ZERO, a));
  b = vec4_min(V4ONE, vec4_max(V4ZERO, b));
  a = vec4_trunc(V4GRID * a + V4HALF) * V4GRIDRCP;
  b = vec4_trunc(V4GRID * b + V4HALF) * V4GRIDRCP;

  *max = a;
  *min = b;
}

/*
 * from nvidia-texture-tools; see LICENSE.nvtt for copyright information
 */
static void
optimize_endpoints4 (dxtblock_t   *dxtb,
                     unsigned int  indices,
                     vec4_t       *max,
                     vec4_t       *min)
{
  float alpha, beta;
  vec4_t alpha2_sum, alphax_sum;
  vec4_t beta2_sum, betax_sum;
  vec4_t alphabeta_sum, a, b, factor;
  int i, bits;

  alpha2_sum = beta2_sum = alphabeta_sum = vec4_zero();
  alphax_sum = vec4_zero();
  betax_sum = vec4_zero();

  for (i = 0; i < 16; ++i)
    {
      bits = indices >> (2 * i);

      beta = (float)(bits & 1);
      if (bits & 2)
        beta = (1.0f + beta) / 3.0f;
      alpha = 1.0f - beta;

      a = vec4_set1(alpha);
      b = vec4_set1(beta);
      alpha2_sum += a * a;
      beta2_sum += b * b;
      alphabeta_sum += a * b;
      alphax_sum += dxtb->points[i] * a;
      betax_sum  += dxtb->points[i] * b;
    }

  factor = alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum;
  if (vec4_cmplt(factor, V4EPSILON))
    return;
  factor = vec4_rcp(factor);

  a = (alphax_sum * beta2_sum  - betax_sum  * alphabeta_sum) * factor;
  b = (betax_sum  * alpha2_sum - alphax_sum * alphabeta_sum) * factor;

  // clamp to the color space
  a = vec4_min(V4ONE, vec4_max(V4ZERO, a));
  b = vec4_min(V4ONE, vec4_max(V4ZERO, b));
  a = vec4_trunc(V4GRID * a + V4HALF) * V4GRIDRCP;
  b = vec4_trunc(V4GRID * b + V4HALF) * V4GRIDRCP;

  *max = a;
  *min = b;
}

static unsigned int
match_colors3 (dxtblock_t *dxtb)
{
  int i, idx;
  unsigned int indices = 0;
  vec4_t t0, t1, t2;
#ifdef USE_SSE
  vec4_t d, bits, zero = _mm_setzero_ps();
  int mask;
#else
  float d0, d1, d2;
#endif

  // match each point to the closest color
  for (i = 0; i < 16; ++i)
    {
      // skip alpha pixels
      if (((dxtb->alphamask >> (2 * i)) & 3) == 3)
        {
          indices |= (3 << (2 * i));
          continue;
        }

      t0 = (dxtb->points[i] - dxtb->palette[0]) * dxtb->metric;
      t1 = (dxtb->points[i] - dxtb->palette[1]) * dxtb->metric;
      t2 = (dxtb->points[i] - dxtb->palette[2]) * dxtb->metric;

#ifdef USE_SSE
      _MM_TRANSPOSE4_PS(t0, t1, t2, zero);
      d = t0 * t0 + t1 * t1 + t2 * t2;
      bits = _mm_cmplt_ps(_mm_shuffle_ps(d, d, _MM_SHUFFLE(3, 1, 0, 0)),
                          _mm_shuffle_ps(d, d, _MM_SHUFFLE(3, 2, 2, 1)));
      mask = _mm_movemask_ps(bits);
      if((mask & 3) == 3) idx = 0;
      else if(mask & 4)   idx = 1;
      else                idx = 2;
#else
      d0 = vec4_dot(t0, t0);
      d1 = vec4_dot(t1, t1);
      d2 = vec4_dot(t2, t2);

      if ((d0 < d1) && (d0 < d2))
        idx = 0;
      else if (d1 < d2)
        idx = 1;
      else
        idx = 2;
#endif

      indices |= (idx << (2 * i));
    }

   return indices;
}

static unsigned int
match_colors4 (dxtblock_t *dxtb)
{
  int i;
  unsigned int idx, indices = 0;
  unsigned int b0, b1, b2, b3, b4;
  unsigned int x0, x1, x2;
  vec4_t t0, t1, t2, t3;
#ifdef USE_SSE
  vec4_t d;
#else
  float d[4];
#endif

  // match each point to the closest color
  for (i = 0; i < 16; ++i)
    {
      t0 = (dxtb->points[i] - dxtb->palette[0]) * dxtb->metric;
      t1 = (dxtb->points[i] - dxtb->palette[1]) * dxtb->metric;
      t2 = (dxtb->points[i] - dxtb->palette[2]) * dxtb->metric;
      t3 = (dxtb->points[i] - dxtb->palette[3]) * dxtb->metric;

#ifdef USE_SSE
      _MM_TRANSPOSE4_PS(t0, t1, t2, t3);
      d = t0 * t0 + t1 * t1 + t2 * t2;
#else
      d[0] = vec4_dot(t0, t0);
      d[1] = vec4_dot(t1, t1);
      d[2] = vec4_dot(t2, t2);
      d[3] = vec4_dot(t3, t3);
#endif

      b0 = d[0] > d[3];
      b1 = d[1] > d[2];
      b2 = d[0] > d[2];
      b3 = d[1] > d[3];
      b4 = d[2] > d[3];

      x0 = b1 & b2;
      x1 = b0 & b3;
      x2 = b0 & b4;

      idx = x2 | ((x0 | x1) << 1);

      indices |= (idx << (2 * i));
    }

   return indices;
}

static float
compute_error3 (dxtblock_t   *dxtb,
                unsigned int  indices)
{
  int i, idx;
  float error = 0;
  vec4_t t;

  // compute error
  for (i = 0; i < 16; ++i)
    {
      idx = (indices >> (2 * i)) & 3;
      // skip alpha pixels
      if(idx == 3)
        continue;
      t = (dxtb->points[i] - dxtb->palette[idx]) * dxtb->metric;
      error += vec4_dot(t, t);
    }

  return error;
}

static float
compute_error4 (dxtblock_t   *dxtb,
                unsigned int  indices)
{
  int i, idx;
  float error = 0;

#ifdef USE_SSE
  vec4_t a0, a1, a2, a3;
  vec4_t b0, b1, b2, b3;
  vec4_t d;

  for (i = 0; i < 4; ++i)
    {
      idx = indices >> (8 * i);
      a0 = dxtb->points[4 * i + 0];
      a1 = dxtb->points[4 * i + 1];
      a2 = dxtb->points[4 * i + 2];
      a3 = dxtb->points[4 * i + 3];
      b0 = dxtb->palette[(idx     ) & 3];
      b1 = dxtb->palette[(idx >> 2) & 3];
      b2 = dxtb->palette[(idx >> 4) & 3];
      b3 = dxtb->palette[(idx >> 6) & 3];
      a0 = (a0 - b0) * dxtb->metric;
      a1 = (a1 - b1) * dxtb->metric;
      a2 = (a2 - b2) * dxtb->metric;
      a3 = (a3 - b3) * dxtb->metric;
      _MM_TRANSPOSE4_PS(a0, a1, a2, a3);
      d = a0 * a0 + a1 * a1 + a2 * a2;
      error += vec4_accum(d);
    }
#else
  vec4_t t;

  // compute error
  for (i = 0; i < 16; ++i)
    {
      idx = (indices >> (2 * i)) & 3;
      t = (dxtb->points[i] - dxtb->palette[idx]) * dxtb->metric;
      error += vec4_dot(t, t);
    }
#endif

  return error;
}

static unsigned int
compress3 (dxtblock_t *dxtb)
{
  const int MAX_ITERATIONS = 8;
  int i;
  unsigned int indices, bestindices;
  float error, besterror = FLT_MAX;
  vec4_t oldmax, oldmin;

  construct_palette3(dxtb);

  indices = match_colors3(dxtb);
  bestindices = indices;

  for (i = 0; i < MAX_ITERATIONS; ++i)
    {
      oldmax = dxtb->max;
      oldmin = dxtb->min;

      optimize_endpoints3(dxtb, indices, &dxtb->max, &dxtb->min);
      construct_palette3(dxtb);
      indices = match_colors3(dxtb);
      error = compute_error3(dxtb, indices);

      if (error < besterror)
        {
          besterror = error;
          bestindices = indices;
        }
      else
        {
          dxtb->max = oldmax;
          dxtb->min = oldmin;
          break;
        }
    }

  return bestindices;
}

static unsigned int
compress4 (dxtblock_t *dxtb)
{
  const int MAX_ITERATIONS = 8;
  int i;
  unsigned int indices, bestindices;
  float error, besterror = FLT_MAX;
  vec4_t oldmax, oldmin;

  construct_palette4(dxtb);

  indices = match_colors4(dxtb);
  bestindices = indices;

  for (i = 0; i < MAX_ITERATIONS; ++i)
    {
      oldmax = dxtb->max;
      oldmin = dxtb->min;

      optimize_endpoints4(dxtb, indices, &dxtb->max, &dxtb->min);
      construct_palette4(dxtb);
      indices = match_colors4(dxtb);
      error = compute_error4(dxtb, indices);

      if (error < besterror)
        {
          besterror = error;
          bestindices = indices;
        }
      else
        {
          dxtb->max = oldmax;
          dxtb->min = oldmin;
          break;
        }
    }

  return bestindices;
}

static void
encode_color_block (unsigned char *dst,
                    unsigned char *block,
                    int            flags)
{
  dxtblock_t dxtb;
  int max16, min16;
  unsigned int indices, mask;

  dxtblock_init(&dxtb, block, flags);

  if (dxtb.single) // single color block
    {
      max16 = (omatch5[block[2]][0] << 11) |
              (omatch6[block[1]][0] <<  5) |
              (omatch5[block[0]][0]      );
      min16 = (omatch5[block[2]][1] << 11) |
              (omatch6[block[1]][1] <<  5) |
              (omatch5[block[0]][1]      );

      indices = 0xaaaaaaaa; // 101010...

      if ((flags & DXT_BC1) && dxtb.alphamask)
        {
          // DXT1 compression, non-opaque block.  Add alpha indices.
          indices |= dxtb.alphamask;
          if (max16 > min16)
            SWAP(max16, min16);
        }
      else if (max16 < min16)
        {
          SWAP(max16, min16);
          indices ^= 0x55555555; // 010101...
        }
    }
  else if ((flags & DXT_BC1) && dxtb.alphamask) // DXT1 compression, non-opaque block
    {
      indices = compress3(&dxtb);

      vec4_endpoints_to_565(&max16, &min16, dxtb.max, dxtb.min);

      if (max16 > min16)
        {
          SWAP(max16, min16);
          // remap indices 0 -> 1, 1 -> 0
          mask = indices & 0xaaaaaaaa;
          mask = mask | (mask >> 1);
          indices = (indices & mask) | ((indices ^ 0x55555555) & ~mask);
        }
    }
  else
    {
      indices = compress4(&dxtb);

      vec4_endpoints_to_565(&max16, &min16, dxtb.max, dxtb.min);

      if (max16 < min16)
        {
          SWAP(max16, min16);
          indices ^= 0x55555555; // 010101...
        }
    }

  PUTL16(dst + 0, max16);
  PUTL16(dst + 2, min16);
  PUTL32(dst + 4, indices);
}

/* write DXT3 alpha block */
static void
encode_alpha_block_BC2 (unsigned char       *dst,
                        const unsigned char *block)
{
  int i, a1, a2;

  block += 3;

  for (i = 0; i < 8; ++i)
    {
      a1 = mul8bit(block[8 * i + 0], 0x0f);
      a2 = mul8bit(block[8 * i + 4], 0x0f);
      *dst++ = (a2 << 4) | a1;
    }
}

/* Write DXT5 alpha block */
static void
encode_alpha_block_BC3 (unsigned char       *dst,
                        const unsigned char *block,
                        const int            offset)
{
  int i, v, mn, mx;
  int dist, bias, dist2, dist4, bits, mask;
  int a, idx, t;

  block += offset;
  block += 3;

  /* find min/max alpha pair */
  mn = mx = block[0];
  for (i = 0; i < 16; ++i)
    {
      v = block[4 * i];
      if(v > mx) mx = v;
      if(v < mn) mn = v;
    }

  /* encode them */
  *dst++ = mx;
  *dst++ = mn;

  /*
   * determine bias and emit indices
   * given the choice of mx/mn, these indices are optimal:
   * http://fgiesen.wordpress.com/2009/12/15/dxt5-alpha-block-index-determination/
   */
  dist = mx - mn;
  dist4 = dist * 4;
  dist2 = dist * 2;
  bias = (dist < 8) ? (dist - 1) : (dist / 2 + 2);
  bias -= mn * 7;
  bits = 0;
  mask = 0;

  for (i = 0; i < 16; ++i)
    {
      a = block[4 * i] * 7 + bias;

      /* select index. this is a "linear scale" lerp factor between 0
         (val=min) and 7 (val=max). */
      t = (a >= dist4) ? -1 : 0; idx =  t & 4; a -= dist4 & t;
      t = (a >= dist2) ? -1 : 0; idx += t & 2; a -= dist2 & t;
      idx += (a >= dist);

      /* turn linear scale into DXT index (0/1 are extremal pts) */
      idx = -idx & 7;
      idx ^= (2 > idx);

      /* write index */
      mask |= idx << bits;
      if ((bits += 3) >= 8)
        {
          *dst++ = mask;
          mask >>= 8;
          bits -= 8;
        }
    }
}

#define BLOCK_COUNT(w, h)          ((((h) + 3) >> 2) * (((w) + 3) >> 2))
#define BLOCK_OFFSET(x, y, w, bs)  (((y) >> 2) * ((bs) * (((w) + 3) >> 2)) + ((bs) * ((x) >> 2)))

static void
compress_BC1 (unsigned char       *dst,
              const unsigned char *src,
              int                  w,
              int                  h,
              int                  flags)
{
  const unsigned int block_count = BLOCK_COUNT(w, h);
  unsigned int i;
  unsigned char block[64], *p;
  int x, y;

#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 256) private(block, p, x, y)
#endif
  for (i = 0; i < block_count; ++i)
    {
      x = (i % ((w + 3) >> 2)) << 2;
      y = (i / ((w + 3) >> 2)) << 2;
      p = dst + BLOCK_OFFSET(x, y, w, 8);
      extract_block(src, x, y, w, h, block);
      encode_color_block(p, block, DXT_BC1 | flags);
    }
}

static void
compress_BC2 (unsigned char       *dst,
              const unsigned char *src,
              int                  w,
              int                  h,
              int                  flags)
{
  const unsigned int block_count = BLOCK_COUNT(w, h);
  unsigned int i;
  unsigned char block[64], *p;
  int x, y;

#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 256) private(block, p, x, y)
#endif
  for (i = 0; i < block_count; ++i)
    {
      x = (i % ((w + 3) >> 2)) << 2;
      y = (i / ((w + 3) >> 2)) << 2;
      p = dst + BLOCK_OFFSET(x, y, w, 16);
      extract_block(src, x, y, w, h, block);
      encode_alpha_block_BC2(p, block);
      encode_color_block(p + 8, block, DXT_BC2 | flags);
    }
}

static void
compress_BC3 (unsigned char       *dst,
              const unsigned char *src,
              int                  w,
              int                  h,
              int                  flags)
{
  const unsigned int block_count = BLOCK_COUNT(w, h);
  unsigned int i;
  unsigned char block[64], *p;
  int x, y;

#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 256) private(block, p, x, y)
#endif
  for (i = 0; i < block_count; ++i)
    {
      x = (i % ((w + 3) >> 2)) << 2;
      y = (i / ((w + 3) >> 2)) << 2;
      p = dst + BLOCK_OFFSET(x, y, w, 16);
      extract_block(src, x, y, w, h, block);
      encode_alpha_block_BC3(p, block, 0);
      encode_color_block(p + 8, block, DXT_BC3 | flags);
    }
}

static void
compress_BC4 (unsigned char       *dst,
              const unsigned char *src,
              int                  w,
              int                  h)
{
  const unsigned int block_count = BLOCK_COUNT(w, h);
  unsigned int i;
  unsigned char block[64], *p;
  int x, y;

#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 256) private(block, p, x, y)
#endif
  for (i = 0; i < block_count; ++i)
    {
      x = (i % ((w + 3) >> 2)) << 2;
      y = (i / ((w + 3) >> 2)) << 2;
      p = dst + BLOCK_OFFSET(x, y, w, 8);
      extract_block(src, x, y, w, h, block);
      encode_alpha_block_BC3(p, block, -1);
    }
}

static void
compress_BC5 (unsigned char       *dst,
              const unsigned char *src,
              int                  w,
              int                  h)
{
  const unsigned int block_count = BLOCK_COUNT(w, h);
  unsigned int i;
  unsigned char block[64], *p;
  int x, y;

#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 256) private(block, p, x, y)
#endif
  for (i = 0; i < block_count; ++i)
    {
      x = (i % ((w + 3) >> 2)) << 2;
      y = (i / ((w + 3) >> 2)) << 2;
      p = dst + BLOCK_OFFSET(x, y, w, 16);
      extract_block(src, x, y, w, h, block);
      /* Pixels are ordered as BGRA (see write_layer)
       * First we encode red  -1+3: channel 2;
       * then we encode green -2+3: channel 1.
       */
      encode_alpha_block_BC3(p, block, -1);
      encode_alpha_block_BC3(p + 8, block, -2);
    }
}

static void
compress_YCoCg (unsigned char       *dst,
                const unsigned char *src,
                int                  w,
                int                  h)
{
  const unsigned int block_count = BLOCK_COUNT(w, h);
  unsigned int i;
  unsigned char block[64], *p;
  int x, y;

#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 256) private(block, p, x, y)
#endif
  for (i = 0; i < block_count; ++i)
    {
      x = (i % ((w + 3) >> 2)) << 2;
      y = (i / ((w + 3) >> 2)) << 2;
      p = dst + BLOCK_OFFSET(x, y, w, 16);
      extract_block(src, x, y, w, h, block);
      encode_alpha_block_BC3(p, block, 0);
      encode_YCoCg_block(p + 8, block);
    }
}

int
dxt_compress (unsigned char *dst,
              unsigned char *src,
              int            format,
              unsigned int   width,
              unsigned int   height,
              int            bpp,
              int            mipmaps,
              int            flags)
{
  int i, size, w, h;
  unsigned int offset;
  unsigned char *tmp = NULL;
  int j;
  unsigned char *s;

  if (bpp == 1)
    {
      /* grayscale promoted to BGRA */

      size = get_mipmapped_size(width, height, 4, 0, mipmaps,
                                DDS_COMPRESS_NONE);
      tmp = g_malloc(size);

      for (i = j = 0; j < size; ++i, j += 4)
        {
          tmp[j + 0] = src[i];
          tmp[j + 1] = src[i];
          tmp[j + 2] = src[i];
          tmp[j + 3] = 255;
        }

      bpp = 4;
    }
  else if (bpp == 2)
    {
      /* gray-alpha promoted to BGRA */

      size = get_mipmapped_size(width, height, 4, 0, mipmaps,
                                DDS_COMPRESS_NONE);
      tmp = g_malloc(size);

      for (i = j = 0; j < size; i += 2, j += 4)
        {
          tmp[j + 0] = src[i];
          tmp[j + 1] = src[i];
          tmp[j + 2] = src[i];
          tmp[j + 3] = src[i + 1];
        }

      bpp = 4;
    }
  else if (bpp == 3)
    {
      size = get_mipmapped_size(width, height, 4, 0, mipmaps,
                                DDS_COMPRESS_NONE);
      tmp = g_malloc(size);

      for (i = j = 0; j < size; i += 3, j += 4)
        {
          tmp[j + 0] = src[i + 0];
          tmp[j + 1] = src[i + 1];
          tmp[j + 2] = src[i + 2];
          tmp[j + 3] = 255;
        }

      bpp = 4;
    }

  offset = 0;
  w = width;
  h = height;
  s = tmp ? tmp : src;

  for (i = 0; i < mipmaps; ++i)
    {
      switch (format)
        {
        case DDS_COMPRESS_BC1:
          compress_BC1(dst + offset, s, w, h, flags);
          break;
        case DDS_COMPRESS_BC2:
          compress_BC2(dst + offset, s, w, h, flags);
          break;
        case DDS_COMPRESS_BC3:
        case DDS_COMPRESS_BC3N:
        case DDS_COMPRESS_RXGB:
        case DDS_COMPRESS_AEXP:
        case DDS_COMPRESS_YCOCG:
          compress_BC3(dst + offset, s, w, h, flags);
          break;
        case DDS_COMPRESS_BC4:
          compress_BC4(dst + offset, s, w, h);
          break;
        case DDS_COMPRESS_BC5:
          compress_BC5(dst + offset, s, w, h);
          break;
        case DDS_COMPRESS_YCOCGS:
          compress_YCoCg(dst + offset, s, w, h);
          break;
        default:
          compress_BC3(dst + offset, s, w, h, flags);
          break;
        }
      s += (w * h * bpp);
      offset += get_mipmapped_size(w, h, 0, 0, 1, format);
      w = MAX(1, w >> 1);
      h = MAX(1, h >> 1);
    }

  if (tmp)
    g_free(tmp);

  return 1;
}

static void
decode_color_block (guchar *block,
                    guchar *src,
                    gint    format)
{
  guchar  *d = block;
  guint    indices, idx;
  guchar   colors[4][3];
  gushort  c0, c1;
  gint     i, x, y;

  c0 = GETL16 (&src[0]);
  c1 = GETL16 (&src[2]);

  unpack_rgb565 (colors[0], c0);
  unpack_rgb565 (colors[1], c1);

  if ((c0 > c1) || (format == DDS_COMPRESS_BC3))
    {
      /* Four-color mode */
      lerp_rgb13 (colors[2], colors[0], colors[1]);
      lerp_rgb13 (colors[3], colors[1], colors[0]);
    }
  else
    {
      /* Three-color mode */
      for (i = 0; i < 3; ++i)
        {
          colors[2][i] = (colors[0][i] + colors[1][i] + 1) >> 1;
          colors[3][i] = 0;  /* Three-color mode index 11 is always black */
        }
    }

  src += 4;
  for (y = 0; y < 4; ++y)
    {
      indices = src[y];
      for (x = 0; x < 4; ++x)
        {
          idx = indices & 0x03;
          d[0] = colors[idx][2];
          d[1] = colors[idx][1];
          d[2] = colors[idx][0];
          if (format == DDS_COMPRESS_BC1)
            d[3] = ((c0 <= c1) && idx == 3) ? 0 : 255;
          indices >>= 2;
          d += 4;
        }
    }
}

static void
decode_alpha_block_BC2 (unsigned char *block,
                        unsigned char *src)
{
  int x, y;
  unsigned char *d = block;
  unsigned int bits;

  for (y = 0; y < 4; ++y)
    {
      bits = GETL16(&src[2 * y]);
      for (x = 0; x < 4; ++x)
        {
          d[0] = (bits & 0x0f) * 17;
          bits >>= 4;
          d += 4;
        }
    }
}

static void
decode_alpha_block_BC3 (unsigned char *block,
                        unsigned char *src,
                        int            w)
{
  int x, y, code;
  unsigned char *d = block;
  unsigned char a0 = src[0];
  unsigned char a1 = src[1];
  unsigned long long bits = GETL64(src) >> 16;

  for (y = 0; y < 4; ++y)
    {
      for (x = 0; x < 4; ++x)
        {
          code = ((unsigned int)bits) & 0x07;
          if (code == 0)
            d[0] = a0;
          else if (code == 1)
            d[0] = a1;
          else if (a0 > a1)
            d[0] = ((8 - code) * a0 + (code - 1) * a1) / 7;
          else if (code >= 6)
            d[0] = (code == 6) ? 0 : 255;
          else
            d[0] = ((6 - code) * a0 + (code - 1) * a1) / 5;
          bits >>= 3;
          d += 4;
        }

      if (w < 4)
        bits >>= (3 * (4 - w));
    }
}

static void
make_normal (unsigned char *dst,
             unsigned char  x,
             unsigned char  y)
{
  float nx = 2.0f * ((float)x / 255.0f) - 1.0f;
  float ny = 2.0f * ((float)y / 255.0f) - 1.0f;
  float nz = 0.0f;
  float d = 1.0f - nx * nx + ny * ny;
  int z;

  if (d > 0)
    nz = sqrtf(d);

  z = (int)(255.0f * (nz + 1) / 2.0f);
  z = MAX(0, MIN(255, z));

  dst[0] = x;
  dst[1] = y;
  dst[2] = z;
}

static void
normalize_block (unsigned char *block,
                 int            format)
{
  int x, y, tmp;

  for (y = 0; y < 4; ++y)
    {
      for (x = 0; x < 4; ++x)
        {
          if (format == DDS_COMPRESS_BC3)
            {
              tmp = block[y * 16 + (x * 4)];
              make_normal(&block[y * 16 + (x * 4)],
                          block[y * 16 + (x * 4) + 3],
                          block[y * 16 + (x * 4) + 1]);
              block[y * 16 + (x * 4) + 3] = tmp;
            }
          else if (format == DDS_COMPRESS_BC5)
            {
              make_normal(&block[y * 16 + (x * 4)],
                          block[y * 16 + (x * 4)],
                          block[y * 16 + (x * 4) + 1]);
            }
        }
    }
}

static void
put_block (unsigned char *dst,
           unsigned char *block,
           unsigned int   bx,
           unsigned int   by,
           unsigned int   width,
           unsigned       height,
           int            bpp)
{
  int x, y, i;
  unsigned char *d;

  for (y = 0; y < 4 && ((by + y) < height); ++y)
    {
      d = dst + ((y + by) * width + bx) * bpp;
      for (x = 0; x < 4 && ((bx + x) < width); ++x)
        {
          for (i = 0; i < bpp; ++ i)
            *d++ = block[y * 16 + (x * 4) + i];
        }
    }
}

int
dxt_decompress (unsigned char *dst,
                unsigned char *src,
                int            format,
                unsigned int   size,
                unsigned int   width,
                unsigned int   height,
                int            bpp,
                int            normals)
{
  unsigned char *s;
  unsigned int x, y;
  unsigned char block[16 * 4];

  s = src;

  for (y = 0; y < height; y += 4)
    {
      for (x = 0; x < width; x += 4)
        {
          memset(block, 0, 16 * 4);

          if (format == DDS_COMPRESS_BC1)
            {
              decode_color_block(block, s, format);
              s += 8;
            }
          else if (format == DDS_COMPRESS_BC2)
            {
              decode_alpha_block_BC2(block + 3, s);
              decode_color_block(block, s + 8, format);
              s += 16;
            }
          else if (format == DDS_COMPRESS_BC3)
            {
              decode_alpha_block_BC3(block + 3, s, width);
              decode_color_block(block, s + 8, format);
              s += 16;
            }
          else if (format == DDS_COMPRESS_BC4)
            {
              decode_alpha_block_BC3(block, s, width);
              s += 8;
            }
          else if (format == DDS_COMPRESS_BC5)
            {
              decode_alpha_block_BC3(block, s, width);
              decode_alpha_block_BC3(block + 1, s + 8, width);
              s += 16;
            }
          else if (format == DDS_COMPRESS_BC7)
            {
              bc7_decompress (s, size, block);
              s += 16;
            }

          if (normals)
            normalize_block(block, format);

          put_block(dst, block, x, y, width, height, bpp);
        }
    }

  return 1;
}
