/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 2005 Frederic Leroy <fredo@starox.org>
 *
 * -*- mode: c tab-width: 2; -*-
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

#include <stdio.h>

#include <glib-object.h>

#include "base/base-types.h"
#include "base/cpu-accel.h"

#include "gimp-composite.h"
#include "gimp-composite-altivec.h"

#ifdef COMPILE_ALTIVEC_IS_OKAY

#ifdef HAVE_ALTIVEC_H
#include <altivec.h>
#endif

/* Paper over differences between official gcc and Apple's weird gcc */
#ifdef HAVE_ALTIVEC_H
#define INIT_VECTOR(v...) {v}
#define CONST_BUFFER(b)   (b)
#else
#define INIT_VECTOR(v...) (v)
#define CONST_BUFFER(b)   ((guchar *)(b))
#endif

static const vector unsigned char alphamask = (const vector unsigned char)
  INIT_VECTOR(0,0,0,0xff,0,0,0,0xff,0,0,0,0xff,0,0,0,0xff);

/* Load a vector from an unaligned location in memory */
static inline vector unsigned char 
LoadUnaligned(const guchar *v)
{
  if ((long)v & 0x0f) 
    {
      vector unsigned char permuteVector = vec_lvsl(0, v);
      vector unsigned char low = vec_ld(0, v);
      vector unsigned char high = vec_ld(16, v);
      return vec_perm(low, high, permuteVector);
    }
  else
    return vec_ld(0, v); /* don't want overflow */
}

/* Load less than a vector from an unaligned location in memory */
static inline vector unsigned char 
LoadUnalignedLess(const guchar *v,
                  int n)
{
  vector unsigned char permuteVector = vec_lvsl(0, v);
  if (((long)v&0x0f)+n > 15) 
    {
      vector unsigned char low = vec_ld(0, v);
      vector unsigned char high = vec_ld(16, v);
      return vec_perm(low, high, permuteVector);
    }
  else
    {
      vector unsigned char tmp = vec_ld(0, v); 
      return vec_perm(tmp, tmp, permuteVector); /* don't want overflow */
    }
}

/* Store a vector to an unaligned location in memory */
static inline void 
StoreUnaligned (vector unsigned char v,
                 const guchar *where)
{
  if ((unsigned long)where & 0x0f)
    {
      /* Load the surrounding area */
      vector unsigned char low = vec_ld(0, where);
      vector unsigned char high = vec_ld(16, where);
      /* Prepare the constants that we need */
      vector unsigned char permuteVector = vec_lvsr(0, where);
      vector signed char oxFF = vec_splat_s8(-1);
      vector signed char ox00 = vec_splat_s8(0);
      /* Make a mask for which parts of the vectors to swap out */
      vector unsigned char mask = (vector unsigned char)vec_perm(ox00, oxFF, permuteVector);
      v = vec_perm(v, v, permuteVector);
      /* Insert our data into the low and high vectors */
      low = vec_sel(low, v, mask);
      high = vec_sel(v, high, mask);
      /* Store the two aligned result vectors */
      vec_st(low, 0, CONST_BUFFER(where));
      vec_st(high, 16, CONST_BUFFER(where));
    }
  else
    { /* prevent overflow */
      vec_st(v, 0, CONST_BUFFER(where));
    }
}

/* Store less than a vector to an unaligned location in memory */
static inline void 
StoreUnalignedLess (vector unsigned char v,
                    const guchar *where,
                    int n)
{
  int i;
  vector unsigned char permuteVector = vec_lvsr(0, where);
  v = vec_perm(v, v, permuteVector); 
  for (i=0; i<n; i++)
    vec_ste(v, i, CONST_BUFFER(where));
}

void 
gimp_composite_addition_rgba8_rgba8_rgba8_altivec (GimpCompositeContext *ctx)
{
  const guchar *A = ctx->A;
  const guchar *B = ctx->B;
  guchar *D = ctx->D;
  guint length = ctx->n_pixels;
  vector unsigned char a,b,d,alpha_a,alpha_b;

  while (length >= 4)
    {
      a=LoadUnaligned(A);
      b=LoadUnaligned(B);

      alpha_a=vec_and(a, alphamask);
      alpha_b=vec_and(b, alphamask);
      d=vec_min(alpha_a, alpha_b);
   
      a=vec_andc(a, alphamask);
      a=vec_adds(a, d);
      b=vec_andc(b, alphamask);
      d=vec_adds(a, b);

      StoreUnaligned(d, D);

      A+=16;
      B+=16;
      D+=16;
      length-=4;
    }
  /* process last pixels */
  length = length*4;
  a=LoadUnalignedLess(A, length);
  b=LoadUnalignedLess(B, length);
    
  alpha_a=vec_and(a,alphamask);
  alpha_b=vec_and(b,alphamask);
  d=vec_min(alpha_a,alpha_b);
   
  a=vec_andc(a,alphamask);
  a=vec_adds(a,d);
  b=vec_andc(b,alphamask);
  d=vec_adds(a,b);

  StoreUnalignedLess(d, D, length);
};

void 
gimp_composite_subtract_rgba8_rgba8_rgba8_altivec (GimpCompositeContext *ctx)
{
  const guchar *A = ctx->A;
  const guchar *B = ctx->B;
  guchar *D = ctx->D;
  guint length = ctx->n_pixels;
  vector unsigned char a,b,d,alpha_a,alpha_b;

  while (length >= 4)
    {
      a=LoadUnaligned(A);
      b=LoadUnaligned(B);

      alpha_a=vec_and(a, alphamask);
      alpha_b=vec_and(b, alphamask);
      d=vec_min(alpha_a, alpha_b);
   
      a=vec_andc(a, alphamask);
      a=vec_adds(a, d);
      b=vec_andc(b, alphamask);
      d=vec_subs(a, b);

      StoreUnaligned(d, D);

      A+=16;
      B+=16;
      D+=16;
      length-=4;
    }
  /* process last pixels */
  length = length*4;
  a=LoadUnalignedLess(A, length);
  b=LoadUnalignedLess(B, length);
    
  alpha_a=vec_and(a,alphamask);
  alpha_b=vec_and(b,alphamask);
  d=vec_min(alpha_a,alpha_b);
   
  a=vec_andc(a,alphamask);
  a=vec_adds(a,d);
  b=vec_andc(b,alphamask);
  d=vec_subs(a,b);

  StoreUnalignedLess(d, D, length);
};

void 
gimp_composite_swap_rgba8_rgba8_rgba8_altivec (GimpCompositeContext *ctx)
{
  const guchar *A = ctx->A;
  const guchar *B = ctx->B;
  guint length = ctx->n_pixels;
  vector unsigned char a,b;

  while (length >= 4)
    {
      a=LoadUnaligned(A);
      b=LoadUnaligned(B);
      StoreUnaligned(b, A);
      StoreUnaligned(a, B);
      A+=16;
      B+=16;
      length-=4;
    }
  /* process last pixels */
  length = length*4;
  a=LoadUnalignedLess(A, length);
  b=LoadUnalignedLess(B, length);
  StoreUnalignedLess(a, B, length);
  StoreUnalignedLess(b, A, length);
};


#endif /* COMPILE_IS_OKAY */

gboolean
gimp_composite_altivec_init (void)
{
#ifdef COMPILE_ALTIVEC_IS_OKAY
  if (cpu_accel () & CPU_ACCEL_PPC_ALTIVEC)
    {
      return (TRUE);
    }
#endif

  return (FALSE);
}
