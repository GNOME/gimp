/*
 * DDS GIMP plugin
 *
 * Copyright (C) 2004-2012 Shawn Kirst <skirst@gmail.com>,
 * with parts (C) 2003 Arne Reuter <homepage@arnereuter.de> where specified.
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

#ifndef __VEC_H__
#define __VEC_H__

#include <math.h>

#ifdef __SSE__
#define USE_SSE 1
#endif

#ifdef USE_SSE
#include <immintrin.h>
#endif

#include "imath.h"

typedef float vec4_t __attribute__((vector_size(16)));
typedef float sym3x3_t[6];

#define VEC4_CONST4(x, y, z, w)  {x, y, z, w}
#define VEC4_CONST3(x, y, z)     {x, y, z, 0.0f}
#define VEC4_CONST1(x)           {x, x, x, x}

static inline vec4_t
vec4_set (float x,
          float y,
          float z,
          float w)
{
#ifdef USE_SSE
  return _mm_setr_ps(x, y, z, w);
#else
  vec4_t v = { x, y, z, w };
  return v;
#endif
}

static inline vec4_t
vec4_set1 (float f)
{
#ifdef USE_SSE
  return _mm_set1_ps(f);
#else
  vec4_t v = { f, f, f, f };
  return v;
#endif
}

static inline vec4_t
vec4_zero (void)
{
#ifdef USE_SSE
  return _mm_setzero_ps();
#else
  vec4_t v = { 0, 0, 0, 0 };
  return v;
#endif
}

static inline void
vec4_store (float        *f,
            const vec4_t  v)
{
#ifdef USE_SSE
  _mm_store_ps (f, v);
#else
  f[0] = v[0]; f[1] = v[1]; f[2] = v[2]; f[3] = v[3];
#endif
}

static inline vec4_t
vec4_splatx (const vec4_t v)
{
#ifdef USE_SSE
  return _mm_shuffle_ps(v, v, 0x00);
#else
  vec4_t r = { v[0], v[0], v[0], v[0] };
  return r;
#endif
}

static inline vec4_t
vec4_splaty (const vec4_t v)
{
#ifdef USE_SSE
  return _mm_shuffle_ps(v, v, 0x55);
#else
  vec4_t r = { v[1], v[1], v[1], v[1] };
  return r;
#endif
}

static inline vec4_t
vec4_splatz (const vec4_t v)
{
#ifdef USE_SSE
  return _mm_shuffle_ps(v, v, 0xaa);
#else
  vec4_t r = { v[2], v[2], v[2], v[2] };
  return r;
#endif
}

static inline vec4_t
vec4_splatw (const vec4_t v)
{
#ifdef USE_SSE
  return _mm_shuffle_ps(v, v, 0xff);
#else
  vec4_t r = { v[3], v[3], v[3], v[3] };
  return r;
#endif
}

static inline vec4_t
vec4_rcp (const vec4_t v)
{
#ifdef USE_SSE
  __m128 est  = _mm_rcp_ps (v);
  __m128 diff = _mm_sub_ps (_mm_set1_ps(1.0f), _mm_mul_ps(est, v));
  return _mm_add_ps(_mm_mul_ps(diff, est), est);
#else
  vec4_t one = { 1.0f, 1.0f, 1.0f, 1.0f };
  return one / v;
#endif
}

static inline vec4_t
vec4_min (const vec4_t a,
          const vec4_t b)
{
#ifdef USE_SSE
  return _mm_min_ps(a, b);
#else
  return vec4_set (MIN(a[0], b[0]), MIN(a[1], b[1]), MIN(a[2], b[2]), MIN(a[3], b[3]));
#endif
}

static inline vec4_t
vec4_max (const vec4_t a,
          const vec4_t b)
{
#ifdef USE_SSE
  return _mm_max_ps (a, b);
#else
  return vec4_set (MAX(a[0], b[0]), MAX(a[1], b[1]), MAX(a[2], b[2]), MAX(a[3], b[3]));
#endif
}

static inline vec4_t
vec4_trunc (const vec4_t v)
{
#ifdef USE_SSE
# ifdef __SSE4_1__
  return _mm_round_ps(v, _MM_FROUND_TRUNC);
# elif defined(__SSE2__)
  return _mm_cvtepi32_ps(_mm_cvttps_epi32(v));
# else
  // convert to ints
  __m128 in = v;
  __m64 lo = _mm_cvttps_pi32(in);
  __m64 hi = _mm_cvttps_pi32(_mm_movehl_ps(in, in));
  // convert to floats
  __m128 part = _mm_movelh_ps(in, _mm_cvtpi32_ps(in, hi));
  __m128 trunc = _mm_cvtpi32_ps(part, lo);
   // clear mmx state
  _mm_empty ();
  return trunc;
# endif
#else
  vec4_t r = { v[0] > 0.0f ? floorf(v[0]) : ceil(v[0]),
               v[1] > 0.0f ? floorf(v[1]) : ceil(v[1]),
               v[2] > 0.0f ? floorf(v[2]) : ceil(v[2]),
               v[3] > 0.0f ? floorf(v[3]) : ceil(v[3]), };
  return r;
#endif
}

static inline float
vec4_accum (const vec4_t v)
{
#ifdef USE_SSE
  float rv;
  __m128 t;
# ifdef __SSE3__
  t = _mm_hadd_ps(v, v);
  t = _mm_hadd_ps(t, t);
# else
  t = _mm_add_ps(v, _mm_movehl_ps(v, v));
  t = _mm_add_ss(t, _mm_shuffle_ps(t, t, 0x01));
# endif
  _mm_store_ss(&rv, t);
  return rv;
#else
  return v[0] + v[1] + v[2] + v[3];
#endif
}

static inline float
vec4_dot (const vec4_t a,
          const vec4_t b)
{
#if defined(USE_SSE) && defined(__SSE4_1__)
  float rv;
  __m128 t = _mm_dp_ps(a, b, 0xff);
  _mm_store_ss(&rv, t);
  return rv;
#else
  return vec4_accum(a * b);
#endif
}

static inline int
vec4_cmplt (const vec4_t a,
            const vec4_t b)
{
#ifdef USE_SSE
  __m128 bits = _mm_cmplt_ps(a, b);
  int val = _mm_movemask_ps(bits);
  return  val != 0;
#else
  return (a[0] < b[0]) || (a[1] < b[1]) || (a[2] < b[2]) || (a[3] < b[3]);
#endif
}

#endif /* __VEC_H__ */
