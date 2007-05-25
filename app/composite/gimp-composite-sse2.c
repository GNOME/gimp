/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Gimp image compositing
 * Copyright (C) 2003  Helvetix Victorinox, a pseudonym, <helvetix@gimp.org>
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

#include "base/base-types.h"

#include "gimp-composite.h"
#include "gimp-composite-sse2.h"

#ifdef COMPILE_SSE2_IS_OKAY

#include "gimp-composite-x86.h"

static const guint32 rgba8_alpha_mask_128[4] = { 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000 };
static const guint32 rgba8_b1_128[4] =         { 0x01010101, 0x01010101, 0x01010101, 0x01010101 };
static const guint32 rgba8_b255_128[4] =       { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
static const guint32 rgba8_w1_128[4] =         { 0x00010001, 0x00010001, 0x00010001, 0x00010001 };
static const guint32 rgba8_w2_128[4] =         { 0x00020002, 0x00020002, 0x00020002, 0x00020002 };
static const guint32 rgba8_w128_128[4] =       { 0x00800080, 0x00800080, 0x00800080, 0x00800080 };
static const guint32 rgba8_w256_128[4] =       { 0x01000100, 0x01000100, 0x01000100, 0x01000100 };
static const guint32 rgba8_w255_128[4] =       { 0X00FF00FF, 0X00FF00FF, 0X00FF00FF, 0X00FF00FF };

static const guint32 va8_alpha_mask_128[4] =   { 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00 };
static const guint32 va8_b255_128[4] =         { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
static const guint32 va8_w1_128[4] =           { 0x00010001, 0x00010001, 0x00010001, 0x00010001 };
static const guint32 va8_w255_128[4] =         { 0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF };

static const guint32 rgba8_alpha_mask_64[2] = { 0xFF000000, 0xFF000000 };
static const guint32 rgba8_b1_64[2] =         { 0x01010101, 0x01010101 };
static const guint32 rgba8_b255_64[2] =       { 0xFFFFFFFF, 0xFFFFFFFF };
static const guint32 rgba8_w1_64[2] =         { 0x00010001, 0x00010001 };
static const guint32 rgba8_w2_64[2] =         { 0x00020002, 0x00020002 };
static const guint32 rgba8_w128_64[2] =       { 0x00800080, 0x00800080 };
static const guint32 rgba8_w256_64[2] =       { 0x01000100, 0x01000100 };
static const guint32 rgba8_w255_64[2] =       { 0X00FF00FF, 0X00FF00FF };

static const guint32 va8_alpha_mask_64[2] =   { 0xFF00FF00, 0xFF00FF00 };
static const guint32 va8_b255_64[2] =         { 0xFFFFFFFF, 0xFFFFFFFF };
static const guint32 va8_w1_64[2] =           { 0x00010001, 0x00010001 };
static const guint32 va8_w255_64[2] =         { 0x00FF00FF, 0x00FF00FF };

#if 0
void
debug_display_sse (void)
{
#define mask32(x) ((x)& (unsigned long long) 0xFFFFFFFF)
#define print128(reg) { \
  unsigned long long reg[2]; \
  asm("movdqu %%" #reg ",%0" : "=m" (reg)); \
  g_print(#reg"=%08llx %08llx", mask32(reg[0]>>32), mask32(reg[0])); \
  g_print(" %08llx %08llx", mask32(reg[1]>>32), mask32(reg[1])); \
 }
  g_print("--------------------------------------------\n");
  print128(xmm0); g_print("  "); print128(xmm1); g_print("\n");
  print128(xmm2); g_print("  "); print128(xmm3); g_print("\n");
  print128(xmm4); g_print("  "); print128(xmm5); g_print("\n");
  print128(xmm6); g_print("  "); print128(xmm7); g_print("\n");
  g_print("--------------------------------------------\n");
}
#endif

void
gimp_composite_addition_rgba8_rgba8_rgba8_sse2 (GimpCompositeContext *_op)
{
  uint64 *d;
  uint64 *a;
  uint64 *b;
  uint128 *D = (uint128 *) _op->D;
  uint128 *A = (uint128 *) _op->A;
  uint128 *B = (uint128 *) _op->B;
  gulong n_pixels = _op->n_pixels;

  asm volatile ("  movdqu    %0,%%xmm0\n"
                "\tmovq      %1,%%mm0"
                : /* empty */
                : "m" (*rgba8_alpha_mask_128), "m" (*rgba8_alpha_mask_64)
                : "%xmm0", "%mm0");

  for (; n_pixels >= 4; n_pixels -= 4)
    {
      asm volatile ("  movdqu      %1,%%xmm2\n"
                    "\tmovdqu      %2,%%xmm3\n"
                    "\tmovdqu  %%xmm2,%%xmm4\n"
                    "\tpaddusb %%xmm3,%%xmm4\n"

                    "\tmovdqu  %%xmm0,%%xmm1\n"
                    "\tpandn   %%xmm4,%%xmm1\n"
                    "\tpminub  %%xmm3,%%xmm2\n"
                    "\tpand    %%xmm0,%%xmm2\n"
                    "\tpor     %%xmm2,%%xmm1\n"
                    "\tmovdqu  %%xmm1,%0\n"
                    : "=m" (*D)
                    : "m" (*A), "m" (*B)
                    : "%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6", "%xmm7");
      A++;
      B++;
      D++;
    }

  a = (uint64 *) A;
  b = (uint64 *) B;
  d = (uint64 *) D;

  for (; n_pixels >= 2; n_pixels -= 2)
    {
      asm volatile ("  movq       %1,%%mm2\n"
                    "\tmovq       %2,%%mm3\n"
                    "\tmovq    %%mm2,%%mm4\n"
                    "\tpaddusb %%mm3,%%mm4\n"
                    "\tmovq    %%mm0,%%mm1\n"
                    "\tpandn   %%mm4,%%mm1\n"
                    "\tpminub  %%mm3,%%mm2\n"
                    "\tpand    %%mm0,%%mm2\n"
                    "\tpor     %%mm2,%%mm1\n"
                    "\tmovntq  %%mm1,%0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm0", "%mm1", "%mm2", "%mm3", "%mm4", "%mm5", "%mm6", "%mm7");
      a++;
      b++;
      d++;
    }

  if (n_pixels > 0)
    {
      asm volatile ("  movd       %1,%%mm2\n"
                    "\tmovd       %2,%%mm3\n"
                    "\tmovq    %%mm2,%%mm4\n"
                    "\tpaddusb %%mm3,%%mm4\n"
                    "\tmovq    %%mm0,%%mm1\n"
                    "\tpandn   %%mm4,%%mm1\n"
                    "\tpminub  %%mm3,%%mm2\n"
                    "\tpand    %%mm0,%%mm2\n"
                    "\tpor     %%mm2,%%mm1\n"
                    "\tmovd    %%mm1,%0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm0", "%mm1", "%mm2", "%mm3", "%mm4", "%mm5", "%mm6", "%mm7");
    }

  asm("emms");
}

void
gimp_composite_darken_rgba8_rgba8_rgba8_sse2 (GimpCompositeContext *_op)
{
  uint64 *d;
  uint64 *a;
  uint64 *b;
  uint128 *D = (uint128 *) _op->D;
  uint128 *A = (uint128 *) _op->A;
  uint128 *B = (uint128 *) _op->B;
  gulong n_pixels = _op->n_pixels;

  for (; n_pixels >= 4; n_pixels -= 4)
    {
      asm volatile ("  movdqu          %1,%%xmm2\n"
                    "\tmovdqu          %2,%%xmm3\n"
                    "\tpminub      %%xmm3,%%xmm2\n"
                    "\tmovdqu      %%xmm2,%0\n"
                    : "=m" (*D)
                    : "m" (*A), "m" (*B)
                    : "%xmm2", "%xmm3");
      A++;
      B++;
      D++;
    }

  a = (uint64 *) A;
  b = (uint64 *) B;
  d = (uint64 *) D;

  for (; n_pixels >= 2; n_pixels -= 2)
    {
      asm volatile ("  movq   %1,%%mm2\n"
                    "\tmovq   %2,%%mm3\n"
                    "\tpminub %%mm3,%%mm2\n"
                    "\tmovntq %%mm2,%0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm2", "%mm3");
      a++;
      b++;
      d++;
    }

  if (n_pixels > 0)
    {
      asm volatile ("  movd       %1, %%mm2\n"
                    "\tmovd       %2, %%mm3\n"
                    "\tpminub  %%mm3, %%mm2\n"
                    "\tmovd    %%mm2, %0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm2", "%mm3", "%mm4");
    }

  asm("emms");
}

void
gimp_composite_difference_rgba8_rgba8_rgba8_sse2 (GimpCompositeContext *_op)
{
  uint64 *d;
  uint64 *a;
  uint64 *b;
  uint128 *D = (uint128 *) _op->D;
  uint128 *A = (uint128 *) _op->A;
  uint128 *B = (uint128 *) _op->B;
  gulong n_pixels = _op->n_pixels;

  asm volatile ("  movq   %0,%%mm0\n"
                "\tmovdqu %1,%%xmm0"
                :               /*  */
                : "m" (*rgba8_alpha_mask_64), "m" (*rgba8_alpha_mask_128)
                : "%mm0", "%xmm0");

  for (; n_pixels >= 4; n_pixels -= 4)
    {
      asm volatile ("  movdqu        %1,%%xmm2\n"
                    "\tmovdqu        %2,%%xmm3\n"
                    "\tmovdqu    %%xmm2,%%xmm4\n"
                    "\tmovdqu    %%xmm3,%%xmm5\n"
                    "\tpsubusb   %%xmm3,%%xmm4\n"
                    "\tpsubusb   %%xmm2,%%xmm5\n"
                    "\tpaddb     %%xmm5,%%xmm4\n"
                    "\tmovdqu    %%xmm0,%%xmm1\n"
                    "\tpandn     %%xmm4,%%xmm1\n"
                    "\tpminub    %%xmm3,%%xmm2\n"
                    "\tpand      %%xmm0,%%xmm2\n"
                    "\tpor       %%xmm2,%%xmm1\n"
                    "\tmovdqu    %%xmm1,%0\n"
                    : "=m" (*D)
                    : "m" (*A), "m" (*B)
                    : "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5");
      A++;
      B++;
      D++;
    }

  a = (uint64 *) A;
  b = (uint64 *) B;
  d = (uint64 *) D;

  for (; n_pixels >= 2; n_pixels -= 2)
    {
      asm volatile ("  movq       %1, %%mm2\n"
                    "\tmovq       %2, %%mm3\n"
                    "\tmovq    %%mm2, %%mm4\n"
                    "\tmovq    %%mm3, %%mm5\n"
                    "\tpsubusb %%mm3, %%mm4\n"
                    "\tpsubusb %%mm2, %%mm5\n"
                    "\tpaddb   %%mm5, %%mm4\n"
                    "\tmovq    %%mm0, %%mm1\n"
                    "\tpandn   %%mm4, %%mm1\n"
                    "\tpminub  %%mm3, %%mm2\n"
                    "\tpand    %%mm0, %%mm2\n"
                    "\tpor     %%mm2, %%mm1\n"
                    "\tmovntq  %%mm1, %0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm1", "%mm2", "%mm3", "%mm4", "%mm5");
      a++;
      b++;
      d++;
    }

  if (n_pixels > 0)
    {
      asm volatile ("  movd       %1, %%mm2\n"
                    "\tmovd       %2, %%mm3\n"
                    "\tmovq    %%mm2, %%mm4\n"
                    "\tmovq    %%mm3, %%mm5\n"
                    "\tpsubusb %%mm3, %%mm4\n"
                    "\tpsubusb %%mm2, %%mm5\n"
                    "\tpaddb   %%mm5, %%mm4\n"
                    "\tmovq    %%mm0, %%mm1\n"
                    "\tpandn   %%mm4, %%mm1\n"
                    "\tpminub  %%mm3, %%mm2\n"
                    "\tpand    %%mm0, %%mm2\n"
                    "\tpor     %%mm2, %%mm1\n"
                    "\tmovd    %%mm1, %0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm1", "%mm2", "%mm3", "%mm4", "%mm5");
    }

  asm("emms");
}


void
gimp_composite_grain_extract_rgba8_rgba8_rgba8_sse2 (GimpCompositeContext *_op)
{
  uint64 *d;
  uint64 *a;
  uint64 *b;
  uint128 *D = (uint128 *) _op->D;
  uint128 *A = (uint128 *) _op->A;
  uint128 *B = (uint128 *) _op->B;
  gulong n_pixels = _op->n_pixels;

  asm volatile ("  movq       %0,%%mm0\n"
                "\tpxor    %%mm6,%%mm6\n"
                "\tmovq       %1,%%mm7\n"
                "\tmovdqu     %2,%%xmm0\n"
                "\tpxor   %%xmm6,%%xmm6\n"
                "\tmovdqu     %3,%%xmm7\n"
                : /* empty */
                : "m" (*rgba8_alpha_mask_64), "m" (*rgba8_w128_64), "m" (*rgba8_alpha_mask_128), "m" (*rgba8_w128_128)
                : "%mm0", "%mm6", "%mm7", "%xmm0", "%xmm6", "%xmm7");

  for (; n_pixels >= 4; n_pixels -= 4)
    {
      asm volatile ("  movdqu       %1,%%xmm2\n"
                    "\tmovdqu       %2,%%xmm3\n"
                    xmm_low_bytes_to_words(xmm2,xmm4,xmm6)
                    xmm_low_bytes_to_words(xmm3,xmm5,xmm6)
                    "\tpsubw     %%xmm5,%%xmm4\n"
                    "\tpaddw     %%xmm7,%%xmm4\n"
                    "\tmovdqu    %%xmm4,%%xmm1\n"

                    xmm_high_bytes_to_words(xmm2,xmm4,xmm6)
                    xmm_high_bytes_to_words(xmm3,xmm5,xmm6)

                    "\tpsubw     %%xmm5,%%xmm4\n"
                    "\tpaddw     %%xmm7,%%xmm4\n"

                    "\tpackuswb  %%xmm4,%%xmm1\n"
                    "\tmovdqu    %%xmm1,%%xmm4\n"

                    "\tmovdqu    %%xmm0,%%xmm1\n"
                    "\tpandn     %%xmm4,%%xmm1\n"

                    "\tpminub    %%xmm3,%%xmm2\n"
                    "\tpand      %%xmm0,%%xmm2\n"

                    "\tpor       %%xmm2,%%xmm1\n"
                    "\tmovdqu    %%xmm1,%0\n"
                    : "=m" (*D)
                    : "m" (*A), "m" (*B)
                    : "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5");
      A++;
      B++;
      D++;
    }

  a = (uint64 *) A;
  b = (uint64 *) B;
  d = (uint64 *) D;

  for (; n_pixels >= 2; n_pixels -= 2)
    {
      asm volatile ("  movq         %1,%%mm2\n"
                    "\tmovq         %2,%%mm3\n"
                    mmx_low_bytes_to_words(mm2,mm4,mm6)
                    mmx_low_bytes_to_words(mm3,mm5,mm6)
                    "\tpsubw     %%mm5,%%mm4\n"
                    "\tpaddw     %%mm7,%%mm4\n"
                    "\tmovq      %%mm4,%%mm1\n"

                    mmx_high_bytes_to_words(mm2,mm4,mm6)
                    mmx_high_bytes_to_words(mm3,mm5,mm6)

                    "\tpsubw     %%mm5,%%mm4\n"
                    "\tpaddw     %%mm7,%%mm4\n"

                    "\tpackuswb  %%mm4,%%mm1\n"
                    "\tmovq      %%mm1,%%mm4\n"

                    "\tmovq      %%mm0,%%mm1\n"
                    "\tpandn     %%mm4,%%mm1\n"

                    "\tpminub    %%mm3,%%mm2\n"
                    "\tpand      %%mm0,%%mm2\n"

                    "\tpor       %%mm2,%%mm1\n"
                    "\tmovntq    %%mm1,%0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm1", "%mm2", "%mm3", "%mm4", "%mm5");
      a++;
      b++;
      d++;
    }

  if (n_pixels > 0)
    {
      asm volatile ("  movd         %1, %%mm2\n"
                    "\tmovd         %2, %%mm3\n"
                    mmx_low_bytes_to_words(mm2,mm4,mm6)
                    mmx_low_bytes_to_words(mm3,mm5,mm6)
                    "\tpsubw     %%mm5, %%mm4\n"
                    "\tpaddw     %%mm7, %%mm4\n"
                    "\tmovq      %%mm4, %%mm1\n"
                    "\tpackuswb  %%mm6, %%mm1\n"
                    "\tmovq      %%mm1, %%mm4\n"
                    "\tmovq      %%mm0, %%mm1\n"
                    "\tpandn     %%mm4, %%mm1\n"
                    "\tpminub    %%mm3, %%mm2\n"
                    "\tpand      %%mm0, %%mm2\n"
                    "\tpor       %%mm2, %%mm1\n"
                    "\tmovd      %%mm1, %0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm1", "%mm2", "%mm3", "%mm4", "%mm5");
    }

  asm("emms");
}

void
gimp_composite_lighten_rgba8_rgba8_rgba8_sse2 (GimpCompositeContext *_op)
{
  uint64 *d;
  uint64 *a;
  uint64 *b;
  uint128 *D = (uint128 *) _op->D;
  uint128 *A = (uint128 *) _op->A;
  uint128 *B = (uint128 *) _op->B;
  gulong n_pixels = _op->n_pixels;

  asm volatile ("  movdqu    %0,%%xmm0\n"
                "\tmovq      %1,%%mm0"
                : /* empty */
                : "m" (*rgba8_alpha_mask_128), "m" (*rgba8_alpha_mask_64)
                : "%xmm0", "%mm0");


  for (; n_pixels >= 4; n_pixels -= 4)
    {
      asm volatile ("  movdqu      %1, %%xmm2\n"
                    "\tmovdqu      %2, %%xmm3\n"
                    "\tmovdqu  %%xmm2, %%xmm4\n"
                    "\tpmaxub  %%xmm3, %%xmm4\n"
                    "\tmovdqu  %%xmm0, %%xmm1\n"
                    "\tpandn   %%xmm4, %%xmm1\n"
                    "\tpminub  %%xmm2, %%xmm3\n"
                    "\tpand    %%xmm0, %%xmm3\n"
                    "\tpor     %%xmm3, %%xmm1\n"
                    "\tmovdqu  %%xmm1, %0\n"
                    : "=m" (*D)
                    : "m" (*A), "m" (*B)
                    : "%xmm1", "%xmm2", "%xmm3", "%xmm4");
      A++;
      B++;
      D++;
    }

  a = (uint64 *) A;
  b = (uint64 *) B;
  d = (uint64 *) D;

  for (; n_pixels >= 2; n_pixels -= 2)
    {
      asm volatile ("  movq       %1, %%mm2\n"
                    "\tmovq       %2, %%mm3\n"
                    "\tmovq    %%mm2, %%mm4\n"
                    "\tpmaxub  %%mm3, %%mm4\n"
                    "\tmovq    %%mm0, %%mm1\n"
                    "\tpandn   %%mm4, %%mm1\n"
                    "\tpminub  %%mm2, %%mm3\n"
                    "\tpand    %%mm0, %%mm3\n"
                    "\tpor     %%mm3, %%mm1\n"
                    "\tmovntq  %%mm1, %0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm1", "%mm2", "%mm3", "%mm4");
      a++;
      b++;
      d++;
    }

  if (n_pixels > 0)
    {
      asm volatile ("  movd       %1, %%mm2\n"
                    "\tmovd       %2, %%mm3\n"
                    "\tmovq    %%mm2, %%mm4\n"
                    "\tpmaxub  %%mm3, %%mm4\n"
                    "\tmovq    %%mm0, %%mm1\n"
                    "\tpandn   %%mm4, %%mm1\n"
                    "\tpminub  %%mm2, %%mm3\n"
                    "\tpand    %%mm0, %%mm3\n"
                    "\tpor     %%mm3, %%mm1\n"
                    "\tmovd    %%mm1, %0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm1", "%mm2", "%mm3", "%mm4");
    }

  asm("emms");
}

void
gimp_composite_subtract_rgba8_rgba8_rgba8_sse2 (GimpCompositeContext *_op)
{
  uint64 *d;
  uint64 *a;
  uint64 *b;
  uint128 *D = (uint128 *) _op->D;
  uint128 *A = (uint128 *) _op->A;
  uint128 *B = (uint128 *) _op->B;
  gulong n_pixels = _op->n_pixels;

  asm volatile ("  movq    %0,%%mm0\n"
                "\tmovdqu  %1,%%xmm0\n"
                : /* empty */
                : "m" (*rgba8_alpha_mask_64), "m" (*rgba8_alpha_mask_128)
                : "%mm0", "%xmm0");

  for (; n_pixels >= 4; n_pixels -= 4)
    {
      asm volatile ("  movdqu       %1,%%xmm2\n"
                    "\tmovdqu       %2,%%xmm3\n"
                    "\tmovdqu   %%xmm2,%%xmm4\n"
                    "\tpsubusb  %%xmm3,%%xmm4\n"

                    "\tmovdqu   %%xmm0,%%xmm1\n"
                    "\tpandn    %%xmm4,%%xmm1\n"
                    "\tpminub   %%xmm3,%%xmm2\n"
                    "\tpand     %%xmm0,%%xmm2\n"
                    "\tpor      %%xmm2,%%xmm1\n"
                    "\tmovdqu   %%xmm1,%0\n"
                    : "=m" (*D)
                    : "m" (*A), "m" (*B)
                    : "%xmm1", "%xmm2", "%xmm3", "%xmm4");
      A++;
      B++;
      D++;
    }

  a = (uint64 *) A;
  b = (uint64 *) B;
  d = (uint64 *) D;

  for (; n_pixels >= 2; n_pixels -= 2)
    {
      asm volatile ("  movq       %1,%%mm2\n"
                    "\tmovq       %2,%%mm3\n"
                    "\tmovq    %%mm2,%%mm4\n"
                    "\tpsubusb %%mm3,%%mm4\n"
                    "\tmovq    %%mm0,%%mm1\n"
                    "\tpandn   %%mm4,%%mm1\n"
                    "\tpminub  %%mm3,%%mm2\n"
                    "\tpand    %%mm0,%%mm2\n"
                    "\tpor     %%mm2,%%mm1\n"
                    "\tmovntq  %%mm1,%0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm1", "%mm2", "%mm3", "%mm4");
      a++;
      b++;
      d++;
    }

  if (n_pixels > 0)
    {
      asm volatile ("  movd       %1,%%mm2\n"
                    "\tmovd       %2,%%mm3\n"
                    "\tmovq    %%mm2,%%mm4\n"
                    "\tpsubusb %%mm3,%%mm4\n"
                    "\tmovq    %%mm0,%%mm1\n"
                    "\tpandn   %%mm4,%%mm1\n"
                    "\tpminub  %%mm3,%%mm2\n"
                    "\tpand    %%mm0,%%mm2\n"
                    "\tpor     %%mm2,%%mm1\n"
                    "\tmovd    %%mm1,%0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm1", "%mm2", "%mm3", "%mm4");
    }

  asm("emms");
}

void
gimp_composite_swap_rgba8_rgba8_rgba8_sse2 (GimpCompositeContext *_op)
{
  GimpCompositeContext op = *_op;

  /*
   * Inhale one whole i686 cache line at once. 128 bytes == 32 rgba8
   * pixels == 8 128 bit xmm registers.
   */
  for (; op.n_pixels >= 16; op.n_pixels -= 16)
    {
      asm volatile ("  movdqu      %0,%%xmm0\n" : :"m" (op.A[0]) : "%xmm0");
      asm volatile ("  movdqu      %0,%%xmm1\n" : :"m" (op.B[0]) : "%xmm1");
      asm volatile ("  movdqu      %0,%%xmm2\n" : :"m" (op.A[1]) : "%xmm2");
      asm volatile ("  movdqu      %0,%%xmm3\n" : :"m" (op.B[1]) : "%xmm3");
      asm volatile ("  movdqu      %0,%%xmm4\n" : :"m" (op.A[2]) : "%xmm4");
      asm volatile ("  movdqu      %0,%%xmm5\n" : :"m" (op.B[2]) : "%xmm5");
      asm volatile ("  movdqu      %0,%%xmm6\n" : :"m" (op.A[3]) : "%xmm6");
      asm volatile ("  movdqu      %0,%%xmm7\n" : :"m" (op.B[3]) : "%xmm7");

      asm volatile ("\tmovdqu      %%xmm0,%0\n" : "=m" (op.A[0]));
      asm volatile ("\tmovdqu      %%xmm1,%0\n" : "=m" (op.B[0]));
      asm volatile ("\tmovdqu      %%xmm2,%0\n" : "=m" (op.A[1]));
      asm volatile ("\tmovdqu      %%xmm3,%0\n" : "=m" (op.B[1]));
      asm volatile ("\tmovdqu      %%xmm4,%0\n" : "=m" (op.A[2]));
      asm volatile ("\tmovdqu      %%xmm5,%0\n" : "=m" (op.B[2]));
      asm volatile ("\tmovdqu      %%xmm6,%0\n" : "=m" (op.A[3]));
      asm volatile ("\tmovdqu      %%xmm7,%0\n" : "=m" (op.B[3]));
      op.A += 64;
      op.B += 64;
    }

  for (; op.n_pixels >= 4; op.n_pixels -= 4)
    {
      asm volatile ("  movdqu      %0,%%xmm2\n"
                    "\tmovdqu      %1,%%xmm3\n"
                    "\tmovdqu  %%xmm3,%0\n"
                    "\tmovdqu  %%xmm2,%1\n"
                    : "+m" (*op.A), "+m" (*op.B)
                    : /* empty */
                    : "%xmm2", "%xmm3");
      op.A += 16;
      op.B += 16;
    }

  for (; op.n_pixels >= 2; op.n_pixels -= 2)
    {
      asm volatile ("  movq      %0,%%mm2\n"
                    "\tmovq      %1,%%mm3\n"
                    "\tmovq   %%mm3,%0\n"
                    "\tmovntq %%mm2,%1\n"
                    : "+m" (*op.A), "+m" (*op.B)
                    : /* empty */
                    : "%mm2", "%mm3");
      op.A += 8;
      op.B += 8;
    }

  if (op.n_pixels)
    {
      asm volatile ("  movd      %0,%%mm2\n"
                    "\tmovd      %1,%%mm3\n"
                    "\tmovd   %%mm3,%0\n"
                    "\tmovd   %%mm2,%1\n"
                    : "+m" (*op.A), "+m" (*op.B)
                    : /* empty */
                    : "%mm3", "%mm4");
    }

  asm("emms");
}

#endif /* COMPILE_SSE2_IS_OKAY */
