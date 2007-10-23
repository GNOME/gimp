/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 2005 Frederic Leroy <fredo@starox.org>
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
static const vector unsigned char combine_high_bytes = (const vector unsigned char)
  INIT_VECTOR(0,16,2,18,4,20,6,22,8,24,10,26,12,28,14,30);
static const vector unsigned short ox0080 = (const vector unsigned short)
  INIT_VECTOR(0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80);
static const vector unsigned short ox0008 = (const vector unsigned short)
  INIT_VECTOR(0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8);
static const vector signed short ox00ff = (const vector signed short)
  INIT_VECTOR(0x00ff,0x00ff,0x00ff,0x00ff,0x00ff,0x00ff,0x00ff,0x00ff);
static const vector signed short oxff80 = (const vector signed short)
  INIT_VECTOR(0xff80,0xff80,0xff80,0xff80,0xff80,0xff80,0xff80,0xff80);

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
}

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
}

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
}

void
gimp_composite_difference_rgba8_rgba8_rgba8_altivec (GimpCompositeContext *ctx)
{
  const guchar *A = ctx->A;
  const guchar *B = ctx->B;
  guchar *D = ctx->D;
  guint length = ctx->n_pixels;
  vector unsigned char a,b,d,e,alpha_a,alpha_b;

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
      e=vec_subs(b, a);
      d=vec_add(d,e);

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
  e=vec_subs(b, a);
  d=vec_add(d,e);

  StoreUnalignedLess(d, D, length);
}

void
gimp_composite_darken_rgba8_rgba8_rgba8_altivec (GimpCompositeContext *ctx)
{
  const guchar *A = ctx->A;
  const guchar *B = ctx->B;
  guchar *D = ctx->D;
  guint length = ctx->n_pixels;
  vector unsigned char a,b,d;

  while (length >= 4)
    {
      a=LoadUnaligned(A);
      b=LoadUnaligned(B);

      d=vec_min(a, b);

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

  d=vec_min(a, b);

  StoreUnalignedLess(d, D, length);
}

void
gimp_composite_lighten_rgba8_rgba8_rgba8_altivec (GimpCompositeContext *ctx)
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
      d=vec_max(a, b);

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
  d=vec_max(a, b);

  StoreUnalignedLess(d, D, length);
}

void
gimp_composite_multiply_rgba8_rgba8_rgba8_altivec (GimpCompositeContext *ctx)
{
  const guchar *A = ctx->A;
  const guchar *B = ctx->B;
  guchar *D = ctx->D;
  guint length = ctx->n_pixels;
  vector unsigned char a,b,d,alpha_a,alpha_b,alpha;
  vector unsigned short al,ah;

  while (length >= 4)
    {
      a=LoadUnaligned(A);
      b=LoadUnaligned(B);

      al=vec_mule(a,b);
      al=vec_add(al,ox0080);
      ah=vec_mulo(a,b);
      ah=vec_add(ah,ox0080);
      al=vec_add(al,vec_sr(al,ox0008));
      ah=vec_add(ah,vec_sr(ah,ox0008));
      d=vec_perm((vector unsigned char)al,(vector unsigned char)ah,combine_high_bytes);

      alpha_a=vec_and(a, alphamask);
      alpha_b=vec_and(b, alphamask);
      alpha=vec_min(alpha_a, alpha_b);

      d=vec_andc(d, alphamask);
      d=vec_or(d, alpha);

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

  al=vec_mule(a,b);
  al=vec_add(al,ox0080);
  ah=vec_mulo(a,b);
  ah=vec_add(ah,ox0080);
  al=vec_add(al,vec_sr(al,ox0008));
  ah=vec_add(ah,vec_sr(ah,ox0008));
  d=vec_perm((vector unsigned char)al,(vector unsigned char)ah,combine_high_bytes);

  alpha_a=vec_and(a, alphamask);
  alpha_b=vec_and(b, alphamask);
  alpha=vec_min(alpha_a, alpha_b);

  d=vec_andc(d, alphamask);
  d=vec_or(d, alpha);

  StoreUnalignedLess(d, D, length);
}

void
gimp_composite_blend_rgba8_rgba8_rgba8_altivec (GimpCompositeContext *ctx)
{
  const guchar *A = ctx->A;
  const guchar *B = ctx->B;
  guchar *D = ctx->D;
  guint length = ctx->n_pixels;
  guchar blend = ctx->blend.blend;
  union
    {
      vector unsigned char v;
      unsigned char u8[16];
    } vblend;

  vector unsigned char vblendc;
  vector unsigned char a,b,d;
  vector unsigned short al,ah,bl,bh,one=vec_splat_u16(1);
  guchar tmp;

  for (tmp=0; tmp<16; tmp++ )
    vblend.u8[tmp]=blend;
  vblendc=vec_nor(vblend.v,vblend.v);

  while (length >= 4)
    {
      a=LoadUnaligned(A);
      b=LoadUnaligned(B);

      /* dest[b] = (src1[b] * blend2 + src2[b] * blend) / 255;
       * to divide by 255 we use ((n+1)+(n+1)>>8)>>8
       * It works for all value but 0xffff
       * happily blending formula can't give this value */

      al=vec_mule(a,vblendc);
      ah=vec_mulo(a,vblendc);

      bl=vec_mule(b,vblend.v);
      bh=vec_mulo(b,vblend.v);

      al=vec_add(al,bl);
      al=vec_add(al,one);
      al=vec_add(al,vec_sr(al,ox0008));

      ah=vec_add(ah,bh);
      ah=vec_add(ah,one);
      ah=vec_add(ah,vec_sr(ah,ox0008));

      d=vec_perm((vector unsigned char)al,(vector unsigned char)ah,combine_high_bytes);

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

  al=vec_mule(a,vblendc);
  ah=vec_mulo(a,vblendc);

  bl=vec_mule(b,vblend.v);
  bh=vec_mulo(b,vblend.v);

  al=vec_add(al,bl);
  al=vec_add(al,one);
  al=vec_add(al,vec_sr(al,ox0008));

  ah=vec_add(ah,bh);
  ah=vec_add(ah,one);
  ah=vec_add(ah,vec_sr(ah,ox0008));

  d=vec_perm((vector unsigned char)al,(vector unsigned char)ah,combine_high_bytes);

  StoreUnalignedLess(d, D, length);
}

void
gimp_composite_screen_rgba8_rgba8_rgba8_altivec (GimpCompositeContext *ctx)
{
  const guchar *A = ctx->A;
  const guchar *B = ctx->B;
  guchar *D = ctx->D;
  guint length = ctx->n_pixels;
  vector unsigned char a,b,d,alpha_a,alpha_b,alpha;
  vector unsigned short ah,al;

  while (length >= 4)
    {
      a=LoadUnaligned(A);
      b=LoadUnaligned(B);

      alpha_a=vec_and(a, alphamask);
      alpha_b=vec_and(b, alphamask);
      alpha=vec_min(alpha_a, alpha_b);

      a=vec_nor(a,a);
      b=vec_nor(b,b);
      al=vec_mule(a,b);
      al=vec_add(al,ox0080);
      ah=vec_mulo(a,b);
      ah=vec_add(ah,ox0080);

      al=vec_add(al,vec_sr(al,ox0008));
      ah=vec_add(ah,vec_sr(ah,ox0008));

      d=vec_perm((vector unsigned char)al,(vector unsigned char)ah,combine_high_bytes);

      d=vec_nor(d,d);
      d=vec_andc(d, alphamask);
      d=vec_or(d, alpha);

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

  alpha_a=vec_and(a, alphamask);
  alpha_b=vec_and(b, alphamask);
  alpha=vec_min(alpha_a, alpha_b);

  a=vec_nor(a,a);
  b=vec_nor(b,b);
  al=vec_mule(a,b);
  al=vec_add(al,ox0080);
  ah=vec_mulo(a,b);
  ah=vec_add(ah,ox0080);

  al=vec_add(al,vec_sr(al,ox0008));
  ah=vec_add(ah,vec_sr(ah,ox0008));

  d=vec_perm((vector unsigned char)al,(vector unsigned char)ah,combine_high_bytes);
  d=vec_nor(d,d);

  d=vec_andc(d, alphamask);
  d=vec_or(d, alpha);

  StoreUnalignedLess(d, D, length);
}

void
gimp_composite_grain_merge_rgba8_rgba8_rgba8_altivec (GimpCompositeContext *ctx)
{
  const guchar *A = ctx->A;
  const guchar *B = ctx->B;
  guchar *D = ctx->D;
  guint length = ctx->n_pixels;
  vector unsigned char a,b,d,alpha_a,alpha_b,alpha;
  vector signed short ah,al,bh,bl;

  while (length >= 4)
    {
      a=LoadUnaligned(A);
      b=LoadUnaligned(B);

      alpha_a=vec_and(a, alphamask);
      alpha_b=vec_and(b, alphamask);
      alpha=vec_min(alpha_a, alpha_b);

      ah=vec_unpackh((vector signed char)a);
      ah=vec_and(ah,ox00ff);
      al=vec_unpackl((vector signed char)a);
      al=vec_and(al,ox00ff);
      bh=vec_unpackh((vector signed char)b);
      bh=vec_and(bh,ox00ff);
      bl=vec_unpackl((vector signed char)b);
      bl=vec_and(bl,ox00ff);

      ah=vec_add(ah,bh);
      al=vec_add(al,bl);
      ah=vec_add(ah,oxff80);
      al=vec_add(al,oxff80);

      d=vec_packsu(ah,al);

      d=vec_andc(d, alphamask);
      d=vec_or(d, alpha);

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

  alpha_a=vec_and(a, alphamask);
  alpha_b=vec_and(b, alphamask);
  alpha=vec_min(alpha_a, alpha_b);

  ah=vec_unpackh((vector signed char)a);
  ah=vec_and(ah,ox00ff);
  al=vec_unpackl((vector signed char)a);
  al=vec_and(al,ox00ff);
  bh=vec_unpackh((vector signed char)b);
  bh=vec_and(bh,ox00ff);
  bl=vec_unpackl((vector signed char)b);
  bl=vec_and(bl,ox00ff);

  ah=vec_add(ah,bh);
  al=vec_add(al,bl);
  ah=vec_add(ah,oxff80);
  al=vec_add(al,oxff80);

  d=vec_packsu(ah,al);

  d=vec_andc(d, alphamask);
  d=vec_or(d, alpha);

  StoreUnalignedLess(d, D, length);
}

void
gimp_composite_grain_extract_rgba8_rgba8_rgba8_altivec (GimpCompositeContext *ctx)
{
  const guchar *A = ctx->A;
  const guchar *B = ctx->B;
  guchar *D = ctx->D;
  guint length = ctx->n_pixels;
  vector unsigned char a,b,d,alpha_a,alpha_b,alpha;
  vector signed short ah,al,bh,bl;

  while (length >= 4)
    {
      a=LoadUnaligned(A);
      b=LoadUnaligned(B);

      alpha_a=vec_and(a, alphamask);
      alpha_b=vec_and(b, alphamask);
      alpha=vec_min(alpha_a, alpha_b);

      ah=vec_unpackh((vector signed char)a);
      ah=vec_and(ah,ox00ff);
      al=vec_unpackl((vector signed char)a);
      al=vec_and(al,ox00ff);
      bh=vec_unpackh((vector signed char)b);
      bh=vec_and(bh,ox00ff);
      bl=vec_unpackl((vector signed char)b);
      bl=vec_and(bl,ox00ff);

      ah=vec_sub(ah,bh);
      al=vec_sub(al,bl);
      ah=vec_sub(ah,oxff80);
      al=vec_sub(al,oxff80);

      d=vec_packsu(ah,al);

      d=vec_andc(d, alphamask);
      d=vec_or(d, alpha);

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

  alpha_a=vec_and(a, alphamask);
  alpha_b=vec_and(b, alphamask);
  alpha=vec_min(alpha_a, alpha_b);

  ah=vec_unpackh((vector signed char)a);
  ah=vec_and(ah,ox00ff);
  al=vec_unpackl((vector signed char)a);
  al=vec_and(al,ox00ff);
  bh=vec_unpackh((vector signed char)b);
  bh=vec_and(bh,ox00ff);
  bl=vec_unpackl((vector signed char)b);
  bl=vec_and(bl,ox00ff);

  ah=vec_sub(ah,bh);
  al=vec_sub(al,bl);
  ah=vec_sub(ah,oxff80);
  al=vec_sub(al,oxff80);

  d=vec_packsu(ah,al);

  d=vec_andc(d, alphamask);
  d=vec_or(d, alpha);

  StoreUnalignedLess(d, D, length);
}

void
gimp_composite_divide_rgba8_rgba8_rgba8_altivec (GimpCompositeContext *ctx)
{
  const guchar *A = ctx->A;
  const guchar *B = ctx->B;
  guchar *D = ctx->D;
  guint length = ctx->n_pixels;
  vector unsigned char a,b,d;
  vector unsigned char alpha_a,alpha_b,alpha;
  vector signed short ox0001=vec_splat_s16(1);
  union
    {
      vector signed short v;
      vector unsigned short vu;
      gushort u16[8];
    } ah,al,bh,bl;

  while (length >= 4)
    {
      a=LoadUnaligned(A);
      b=LoadUnaligned(B);

      alpha_a=vec_and(a, alphamask);
      alpha_b=vec_and(b, alphamask);
      alpha=vec_min(alpha_a, alpha_b);

      ah.v=vec_unpackh((vector signed char)a);
      ah.v=vec_sl(ah.v,ox0008);
      al.v=vec_unpackl((vector signed char)a);
      al.v=vec_sl(al.v,ox0008);

      bh.v=vec_unpackh((vector signed char)b);
      bh.v=vec_and(bh.v,ox00ff);
      bh.v=vec_add(bh.v,ox0001);
      bl.v=vec_unpackl((vector signed char)b);
      bl.v=vec_and(bl.v,ox00ff);
      bl.v=vec_add(bl.v,ox0001);

      ah.u16[0]=ah.u16[0]/bh.u16[0];
      ah.u16[1]=ah.u16[1]/bh.u16[1];
      ah.u16[2]=ah.u16[2]/bh.u16[2];
      ah.u16[4]=ah.u16[4]/bh.u16[4];
      ah.u16[5]=ah.u16[5]/bh.u16[5];
      ah.u16[6]=ah.u16[6]/bh.u16[6];

      al.u16[0]=al.u16[0]/bl.u16[0];
      al.u16[1]=al.u16[1]/bl.u16[1];
      al.u16[2]=al.u16[2]/bl.u16[2];
      al.u16[4]=al.u16[4]/bl.u16[4];
      al.u16[5]=al.u16[5]/bl.u16[5];
      al.u16[6]=al.u16[6]/bl.u16[6];

      d=vec_packs(ah.vu,al.vu);

      d=vec_andc(d, alphamask);
      d=vec_or(d, alpha);

      StoreUnaligned(d, D);
      A+=16;
      B+=16;
      D+=16;
      length-=4;
    }
  length = length*4;
  a=LoadUnalignedLess(A, length);
  b=LoadUnalignedLess(B, length);

  alpha_a=vec_and(a, alphamask);
  alpha_b=vec_and(b, alphamask);
  alpha=vec_min(alpha_a, alpha_b);

  ah.v=vec_unpackh((vector signed char)a);
  ah.v=vec_sl(ah.v,ox0008);
  al.v=vec_unpackl((vector signed char)a);
  al.v=vec_sl(al.v,ox0008);

  bh.v=vec_unpackh((vector signed char)b);
  bh.v=vec_and(bh.v,ox00ff);
  bh.v=vec_add(bh.v,ox0001);
  bl.v=vec_unpackl((vector signed char)b);
  bl.v=vec_and(bl.v,ox00ff);
  bl.v=vec_add(bl.v,ox0001);

  ah.u16[0]=ah.u16[0]/bh.u16[0];
  ah.u16[1]=ah.u16[1]/bh.u16[1];
  ah.u16[2]=ah.u16[2]/bh.u16[2];
  ah.u16[4]=ah.u16[4]/bh.u16[4];
  ah.u16[5]=ah.u16[5]/bh.u16[5];
  ah.u16[6]=ah.u16[6]/bh.u16[6];

  al.u16[0]=al.u16[0]/bl.u16[0];
  al.u16[1]=al.u16[1]/bl.u16[1];
  al.u16[2]=al.u16[2]/bl.u16[2];
  al.u16[4]=al.u16[4]/bl.u16[4];
  al.u16[5]=al.u16[5]/bl.u16[5];
  al.u16[6]=al.u16[6]/bl.u16[6];

  d=vec_packs(ah.vu,al.vu);

  d=vec_andc(d, alphamask);
  d=vec_or(d, alpha);

  StoreUnalignedLess(d, D, length);
}

void
gimp_composite_dodge_rgba8_rgba8_rgba8_altivec (GimpCompositeContext *ctx)
{
  const guchar *A = ctx->A;
  const guchar *B = ctx->B;
  guchar *D = ctx->D;
  guint length = ctx->n_pixels;
  vector unsigned char a,b,d;
  vector unsigned char alpha_a,alpha_b,alpha;
  vector signed short ox0001=vec_splat_s16(1);
  union
    {
      vector signed short v;
      vector unsigned short vu;
      gushort u16[8];
    } ah,al,bh,bl;

  while (length >= 4)
    {
      a=LoadUnaligned(A);
      b=LoadUnaligned(B);

      alpha_a=vec_and(a, alphamask);
      alpha_b=vec_and(b, alphamask);
      alpha=vec_min(alpha_a, alpha_b);

      ah.v=vec_unpackh((vector signed char)a);
      ah.v=vec_sl(ah.v,ox0008);
      al.v=vec_unpackl((vector signed char)a);
      al.v=vec_sl(al.v,ox0008);

      b=vec_nor(b,b);
      bh.v=vec_unpackh((vector signed char)b);
      bh.v=vec_and(bh.v,ox00ff);
      bh.v=vec_add(bh.v,ox0001);
      bl.v=vec_unpackl((vector signed char)b);
      bl.v=vec_and(bl.v,ox00ff);
      bl.v=vec_add(bl.v,ox0001);

      ah.u16[0]=ah.u16[0]/bh.u16[0];
      ah.u16[1]=ah.u16[1]/bh.u16[1];
      ah.u16[2]=ah.u16[2]/bh.u16[2];
      ah.u16[4]=ah.u16[4]/bh.u16[4];
      ah.u16[5]=ah.u16[5]/bh.u16[5];
      ah.u16[6]=ah.u16[6]/bh.u16[6];

      al.u16[0]=al.u16[0]/bl.u16[0];
      al.u16[1]=al.u16[1]/bl.u16[1];
      al.u16[2]=al.u16[2]/bl.u16[2];
      al.u16[4]=al.u16[4]/bl.u16[4];
      al.u16[5]=al.u16[5]/bl.u16[5];
      al.u16[6]=al.u16[6]/bl.u16[6];

      d=vec_packs(ah.vu,al.vu);

      d=vec_andc(d, alphamask);
      d=vec_or(d, alpha);

      StoreUnaligned(d, D);
      A+=16;
      B+=16;
      D+=16;
      length-=4;
    }
  length = length*4;
  a=LoadUnalignedLess(A, length);
  b=LoadUnalignedLess(B, length);

  alpha_a=vec_and(a, alphamask);
  alpha_b=vec_and(b, alphamask);
  alpha=vec_min(alpha_a, alpha_b);

  ah.v=vec_unpackh((vector signed char)a);
  ah.v=vec_sl(ah.v,ox0008);
  al.v=vec_unpackl((vector signed char)a);
  al.v=vec_sl(al.v,ox0008);

  b=vec_nor(b,b);
  bh.v=vec_unpackh((vector signed char)b);
  bh.v=vec_and(bh.v,ox00ff);
  bh.v=vec_add(bh.v,ox0001);
  bl.v=vec_unpackl((vector signed char)b);
  bl.v=vec_and(bl.v,ox00ff);
  bl.v=vec_add(bl.v,ox0001);

  ah.u16[0]=ah.u16[0]/bh.u16[0];
  ah.u16[1]=ah.u16[1]/bh.u16[1];
  ah.u16[2]=ah.u16[2]/bh.u16[2];
  ah.u16[4]=ah.u16[4]/bh.u16[4];
  ah.u16[5]=ah.u16[5]/bh.u16[5];
  ah.u16[6]=ah.u16[6]/bh.u16[6];

  al.u16[0]=al.u16[0]/bl.u16[0];
  al.u16[1]=al.u16[1]/bl.u16[1];
  al.u16[2]=al.u16[2]/bl.u16[2];
  al.u16[4]=al.u16[4]/bl.u16[4];
  al.u16[5]=al.u16[5]/bl.u16[5];
  al.u16[6]=al.u16[6]/bl.u16[6];

  d=vec_packs(ah.vu,al.vu);

  d=vec_andc(d, alphamask);
  d=vec_or(d, alpha);

  StoreUnalignedLess(d, D, length);
}

#endif /* COMPILE_IS_OKAY */
