/* GIMP - The GNU Image Manipulation Program
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

/* Much of the content of this file are derivative works of David
 * Monniaux which are Copyright (C) 1999, 2001 David Monniaux
 * Tip-o-the-hat to David for pioneering this effort.
 *
 * All of these functions use the mmx registers and expect them to
 * remain intact across multiple asm() constructs.  This may not work
 * in the future, if the compiler allocates mmx registers for it's own
 * use. XXX
 */

#include "config.h"

#include <glib-object.h>

#include "base/base-types.h"

#include "gimp-composite.h"
#include "gimp-composite-mmx.h"

#ifdef COMPILE_MMX_IS_OKAY

#include "gimp-composite-x86.h"

#define pminub(src,dst,tmp)  "\tmovq %%" #dst ", %%" #tmp ";" "psubusb %%" #src ", %%" #tmp ";" "psubb %%" #tmp ", %%" #dst "\n"
#define pmaxub(a,b,tmp)      "\tmovq %%" #a ", %%" #tmp ";" "psubusb %%" #b ", %%" #tmp ";" "paddb %%" #tmp ", %%" #b "\n"


#if 0
static void
debug_display_mmx(void)
{
#define mask32(x) ((x)& (unsigned long long) 0xFFFFFFFF)
#define print64(reg) { unsigned long long reg; asm("movq %%" #reg ",%0" : "=m" (reg)); g_print(#reg"=%08llx %08llx", mask32(reg>>32), mask32(reg)); }
  g_print("--------------------------------------------\n");
  print64(mm0); g_print("  "); print64(mm1); g_print("\n");
  print64(mm2); g_print("  "); print64(mm3); g_print("\n");
  print64(mm4); g_print("  "); print64(mm5); g_print("\n");
  print64(mm6); g_print("  "); print64(mm7); g_print("\n");
  g_print("--------------------------------------------\n");
}
#endif

const guint32 rgba8_alpha_mask_64[2] = { 0xFF000000, 0xFF000000 };
const guint32 rgba8_b1_64[2] =         { 0x01010101, 0x01010101 };
const guint32 rgba8_b255_64[2] =       { 0xFFFFFFFF, 0xFFFFFFFF };
const guint32 rgba8_w1_64[2] =         { 0x00010001, 0x00010001 };
const guint32 rgba8_w2_64[2] =         { 0x00020002, 0x00020002 };
const guint32 rgba8_w128_64[2] =       { 0x00800080, 0x00800080 };
const guint32 rgba8_w256_64[2] =       { 0x01000100, 0x01000100 };
const guint32 rgba8_w255_64[2] =       { 0X00FF00FF, 0X00FF00FF };

const guint32 va8_alpha_mask_64[2] =   { 0xFF00FF00, 0xFF00FF00 };
const guint32 va8_b255_64[2] =         { 0xFFFFFFFF, 0xFFFFFFFF };
const guint32 va8_w1_64[2] =           { 0x00010001, 0x00010001 };
const guint32 va8_w255_64[2] =         { 0x00FF00FF, 0x00FF00FF };
const guint32 va8_w128_64[2] =         { 0x00800080, 0x00800080 };

/*const static guint32 v8_alpha_mask[2] = { 0xFF00FF00, 0xFF00FF00};
  const static guint32 v8_mul_shift[2] = { 0x00800080, 0x00800080 };*/


/*
 *
 */
void
gimp_composite_addition_rgba8_rgba8_rgba8_mmx (GimpCompositeContext *_op)
{
  uint64 *d = (uint64 *) _op->D;
  uint64 *a = (uint64 *) _op->A;
  uint64 *b = (uint64 *) _op->B;
  gulong n_pixels = _op->n_pixels;

  asm volatile ("movq    %0,%%mm0"
                : /* empty */
                : "m" (*rgba8_alpha_mask_64)
                : "%mm0");

  for (; n_pixels >= 2; n_pixels -= 2)
    {
      asm volatile ("  movq       %1, %%mm2\n"
                    "\tmovq       %2, %%mm3\n"
                    "\tmovq    %%mm2, %%mm4\n"
                    "\tpaddusb %%mm3, %%mm4\n"
                    "\tmovq    %%mm0, %%mm1\n"
                    "\tpandn   %%mm4, %%mm1\n"
                    "\t" pminub(mm3, mm2, mm4) "\n"
                    "\tpand    %%mm0, %%mm2\n"
                    "\tpor     %%mm2, %%mm1\n"
                    "\tmovq    %%mm1, %0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm0", "%mm1", "%mm2", "%mm3", "%mm4");
      a++;
      b++;
      d++;
    }

  if (n_pixels > 0)
    {
      asm volatile ("  movd    %1, %%mm2\n"
                    "\tmovd    %2, %%mm3\n"
                    "\tmovq    %%mm2, %%mm4\n"
                    "\tpaddusb %%mm3, %%mm4\n"
                    "\tmovq    %%mm0, %%mm1\n"
                    "\tpandn   %%mm4, %%mm1\n"
                    "\t" pminub(mm3, mm2, mm4) "\n"
                    "\tpand    %%mm0, %%mm2\n"
                    "\tpor     %%mm2, %%mm1\n"
                    "\tmovd    %%mm1, %0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm0", "%mm1", "%mm2", "%mm3", "%mm4");
    }

  asm("emms");
}

#if 0
void
gimp_composite_burn_rgba8_rgba8_rgba8_mmx (GimpCompositeContext *_op)
{
  uint64 *d = (uint64 *) _op->D;
  uint64 *a = (uint64 *) _op->A;
  uint64 *b = (uint64 *) _op->B;
  gulong n_pixels = _op->n_pixels;

  for (; n_pixels >= 2; n_pixels -= 2)
    {
      asm volatile ("  movq         %1,%%mm0\n"
                    "\tmovq         %2,%%mm1\n"

                    "\tmovq         %3,%%mm2\n"
                    "\tpsubb     %%mm0,%%mm2\n" /* mm2 = 255 - A */
                    "\tpxor      %%mm4,%%mm4\n"
                    "\tpunpcklbw %%mm2,%%mm4\n" /* mm4 = (255- A) * 256  */

                    "\tmovq      %%mm1,%%mm3\n"
                    "\tpxor      %%mm5,%%mm5\n"
                    "\tpunpcklbw %%mm5,%%mm3\n"
                    "\tmovq         %4,%%mm5\n"
                    "\tpaddusw   %%mm3,%%mm5\n" /* mm5 = B + 1 */

                    "\t" pdivwqX(mm4,mm5,mm7) "\n"

                    "\tmovq         %3,%%mm2\n"
                    "\tpsubb     %%mm0,%%mm2\n" /* mm2 = 255 - A */
                    "\tpxor      %%mm4,%%mm4\n"
                    "\tpunpckhbw %%mm2,%%mm4\n" /* mm4 = (255- A) * 256  */

                    "\tmovq      %%mm1,%%mm3\n"
                    "\tpxor      %%mm5,%%mm5\n"
                    "\tpunpckhbw %%mm5,%%mm3\n"
                    "\tmovq         %4,%%mm5\n"
                    "\tpaddusw   %%mm3,%%mm5\n" /* mm5 = B + 1 */
                    "\t" pdivwqX(mm4,mm5,mm6) "\n"

                    "\tmovq         %5,%%mm4\n"
                    "\tmovq      %%mm4,%%mm5\n"
                    "\tpsubusw   %%mm6,%%mm4\n"
                    "\tpsubusw   %%mm7,%%mm5\n"

                    "\tpackuswb  %%mm4,%%mm5\n"

                    "\t" pminub(mm0,mm1,mm3) "\n" /* mm1 = min(mm0,mm1) clobber mm3 */

                    "\tmovq         %6,%%mm7\n" /* mm6 = rgba8_alpha_mask_64 */
                    "\tpand      %%mm7,%%mm1\n" /* mm1 = mm7 & alpha_mask */

                    "\tpandn     %%mm5,%%mm7\n" /* mm7 = ~mm7 & mm5 */
                    "\tpor       %%mm1,%%mm7\n" /* mm7 = mm7 | mm1 */

                    "\tmovq      %%mm7,%0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b), "m" (*rgba8_b255_64), "m" (*rgba8_w1_64), "m" (*rgba8_w255_64), "m" (*rgba8_alpha_mask_64)
                    : pdivwqX_clobber, "%mm0", "%mm1", "%mm2", "%mm3", "%mm4", "%mm5", "%mm6", "%mm7");
      d++;
      b++;
      a++;
    }

  if (n_pixels > 0)
    {
      asm volatile ("  movd         %1,%%mm0\n"
                    "\tmovd         %2,%%mm1\n"

                    "\tmovq         %3,%%mm2\n"
                    "\tpsubb     %%mm0,%%mm2\n" /* mm2 = 255 - A */
                    "\tpxor      %%mm4,%%mm4\n"
                    "\tpunpcklbw %%mm2,%%mm4\n" /* mm4 = (255- A) * 256  */

                    "\tmovq      %%mm1,%%mm3\n"
                    "\tpxor      %%mm5,%%mm5\n"
                    "\tpunpcklbw %%mm5,%%mm3\n"
                    "\tmovq         %4,%%mm5\n"
                    "\tpaddusw   %%mm3,%%mm5\n" /* mm5 = B + 1 */

                    "\t" pdivwqX(mm4,mm5,mm7) "\n"

                    "\tmovq         %3,%%mm2\n"
                    "\tpsubb     %%mm0,%%mm2\n" /* mm2 = 255 - A */
                    "\tpxor      %%mm4,%%mm4\n"
                    "\tpunpckhbw %%mm2,%%mm4\n" /* mm4 = (255- A) * 256  */

                    "\tmovq      %%mm1,%%mm3\n"
                    "\tpxor      %%mm5,%%mm5\n"
                    "\tpunpckhbw %%mm5,%%mm3\n"
                    "\tmovq         %4,%%mm5\n"
                    "\tpaddusw   %%mm3,%%mm5\n" /* mm5 = B + 1 */
                    "\t" pdivwqX(mm4,mm5,mm6) "\n"

                    "\tmovq         %5,%%mm4\n"
                    "\tmovq      %%mm4,%%mm5\n"
                    "\tpsubusw   %%mm6,%%mm4\n"
                    "\tpsubusw   %%mm7,%%mm5\n"

                    "\tpackuswb  %%mm4,%%mm5\n"

                    "\t" pminub(mm0,mm1,mm3) "\n" /* mm1 = min(mm0,mm1) clobber mm3 */

                    "\tmovq         %6,%%mm7\n"
                    "\tpand      %%mm7,%%mm1\n" /* mm1 = mm7 & alpha_mask */

                    "\tpandn     %%mm5,%%mm7\n" /* mm7 = ~mm7 & mm5 */
                    "\tpor       %%mm1,%%mm7\n" /* mm7 = mm7 | mm1 */

                    "\tmovd      %%mm7,%0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b), "m" (*rgba8_b255_64), "m" (*rgba8_w1_64), "m" (*rgba8_w255_64), "m" (*rgba8_alpha_mask_64)
                    : pdivwqX_clobber, "%mm0", "%mm1", "%mm2", "%mm3", "%mm4", "%mm5", "%mm6", "%mm7");
    }

  asm("emms");
}
#endif

void
gimp_composite_darken_rgba8_rgba8_rgba8_mmx (GimpCompositeContext *_op)
{
  uint64 *d = (uint64 *) _op->D;
  uint64 *a = (uint64 *) _op->A;
  uint64 *b = (uint64 *) _op->B;
  gulong n_pixels = _op->n_pixels;

  for (; n_pixels >= 2; n_pixels -= 2)
    {
      asm volatile ("  movq    %1, %%mm2\n"
                    "\tmovq    %2, %%mm3\n"
                    "\t" pminub(mm3, mm2, mm4) "\n"
                    "\tmovq    %%mm2, %0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm1", "%mm2", "%mm3", "%mm4");
      a++;
      b++;
      d++;
    }

  if (n_pixels > 0)
    {
      asm volatile ("  movd    %1, %%mm2\n"
                    "\tmovd    %2, %%mm3\n"
                    "\t" pminub(mm3, mm2, mm4) "\n"
                    "\tmovd    %%mm2, %0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm2", "%mm3", "%mm4");
    }

  asm("emms");
}

void
gimp_composite_difference_rgba8_rgba8_rgba8_mmx (GimpCompositeContext *_op)
{
  uint64 *d = (uint64 *) _op->D;
  uint64 *a = (uint64 *) _op->A;
  uint64 *b = (uint64 *) _op->B;
  gulong n_pixels = _op->n_pixels;

  asm volatile ("movq    %0,%%mm0"     :  : "m" (*rgba8_alpha_mask_64) : "%mm0");

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
                    "\t" pminub(mm3,mm2,mm4) "\n"
                    "\tpand    %%mm0, %%mm2\n"
                    "\tpor     %%mm2, %%mm1\n"
                    "\tmovq    %%mm1, %0\n"
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
                    "\tmovq    %%mm3, %%mm5\n"
                    "\tpsubusb %%mm3, %%mm4\n"
                    "\tpsubusb %%mm2, %%mm5\n"
                    "\tpaddb   %%mm5, %%mm4\n"
                    "\tmovq    %%mm0, %%mm1\n"
                    "\tpandn   %%mm4, %%mm1\n"
                    "\t" pminub(mm3,mm2,mm4) "\n"
                    "\tpand    %%mm0, %%mm2\n"
                    "\tpor     %%mm2, %%mm1\n"
                    "\tmovd    %%mm1, %0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm1", "%mm2", "%mm3", "%mm4");
    }

  asm("emms");
}

#if 0
void
xxxgimp_composite_divide_rgba8_rgba8_rgba8_mmx (GimpCompositeContext *_op)
{
  uint64 *d = (uint64 *) _op->D;
  uint64 *a = (uint64 *) _op->A;
  uint64 *b = (uint64 *) _op->B;
  gulong n_pixels = _op->n_pixels;

  asm volatile ("  movq    %0, %%mm0\n"
                "\tmovq    %1, %%mm7\n"
                :
                : "m" (*rgba8_alpha_mask_64), "m" (*rgba8_w1_64)
                : "%mm0", "%mm7");

  for (; n_pixels >= 2; n_pixels -= 2)
    {
      asm volatile ("  movq         %1,%%mm0\n"
                    "\tmovq         %2,%%mm1\n"
                    "\tpxor      %%mm2,%%mm2\n"
                    "\tpunpcklbw %%mm0,%%mm2\n" /* mm2 = A*256 */

                    "\tmovq      %%mm1,%%mm3\n"
                    "\tpxor      %%mm5,%%mm5\n"
                    "\tpunpcklbw %%mm5,%%mm3\n"
                    "\tpaddw     %%mm7,%%mm3\n" /* mm3 = B+1 */

                    "\t" pdivwuqX(mm2,mm3,mm5) "\n" /* mm5 = (A*256)/(B+1) */

                    "\tpxor      %%mm2,%%mm2\n"
                    "\tpunpckhbw %%mm0,%%mm2\n" /* mm2 = A*256 */

                    "\tmovq      %%mm1,%%mm3\n"
                    "\tpxor      %%mm6,%%mm6\n"
                    "\tpunpckhbw %%mm6,%%mm3\n"
                    "\tpaddw     %%mm7,%%mm3\n" /* mm3 = B+1 */

                    "\t" pdivwuqX(mm2,mm3,mm4) "\n" /* mm4 = (A*256)/(B+1) */

                    "\tpackuswb  %%mm4,%%mm5\n" /* expects mm4 and mm5 to be signed values */

                    "\t" pminub(mm0,mm1,mm3) "\n"
                    "\tmovq         %3,%%mm3\n"
                    "\tmovq      %%mm3,%%mm2\n"

                    "\tpandn     %%mm5,%%mm3\n"

                    "\tpand      %%mm2,%%mm1\n"
                    "\tpor       %%mm1,%%mm3\n"

                    "\tmovq      %%mm3,%0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b), "m" (*rgba8_alpha_mask_64)
                    : pdivwuqX_clobber, "%mm1", "%mm2", "%mm3", "%mm4", "%mm5");
      a++;
      b++;
      d++;
    }

  if (n_pixels > 0)
    {
      asm volatile ("  movd         %1,%%mm0\n"
                    "\tmovd         %2,%%mm1\n"
                    "\tpxor      %%mm2,%%mm2\n"
                    "\tpunpcklbw %%mm0,%%mm2\n" /* mm2 = A*256 */

                    "\tmovq      %%mm1,%%mm3\n"
                    "\tpxor      %%mm5,%%mm5\n"
                    "\tpunpcklbw %%mm5,%%mm3\n"
                    "\tpaddw     %%mm7,%%mm3\n" /* mm3 = B+1 */

                    "\t" pdivwuqX(mm2,mm3,mm5) "\n" /* mm5 = (A*256)/(B+1) */

                    "\tpxor      %%mm2,%%mm2\n"
                    "\tpunpckhbw %%mm0,%%mm2\n" /* mm2 = A*256 */

                    "\tmovq      %%mm1,%%mm3\n"
                    "\tpxor      %%mm6,%%mm6\n"
                    "\tpunpckhbw %%mm6,%%mm3\n"
                    "\tpaddw     %%mm7,%%mm3\n" /* mm3 = B+1 */

                    "\t" pdivwuqX(mm2,mm3,mm4) "\n" /* mm4 = (A*256)/(B+1) */

                    "\tpackuswb  %%mm4,%%mm5\n" /* expects mm4 and mm5 to be signed values */

                    "\t" pminub(mm0,mm1,mm3) "\n"
                    "\tmovq         %3,%%mm3\n"
                    "\tmovq      %%mm3,%%mm2\n"

                    "\tpandn     %%mm5,%%mm3\n"

                    "\tpand      %%mm2,%%mm1\n"
                    "\tpor       %%mm1,%%mm3\n"

                    "\tmovd      %%mm3,%0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b), "m" (*rgba8_alpha_mask_64)
                    : pdivwuqX_clobber, "%mm1", "%mm2", "%mm3", "%mm4", "%mm5");
    }

  asm("emms");
}
#endif

#if 0
void
xxxgimp_composite_dodge_rgba8_rgba8_rgba8_mmx (GimpCompositeContext *_op)
{
  uint64 *d = (uint64 *) _op->D;
  uint64 *a = (uint64 *) _op->A;
  uint64 *b = (uint64 *) _op->B;
  gulong n_pixels = _op->n_pixels;

  for (; n_pixels >= 2; n_pixels -= 2)
    {
      asm volatile ("  movq         %1,%%mm0\n"
                    "\tmovq         %2,%%mm1\n"
                    "\tmovq      %%mm1,%%mm3\n"
                    "\tpxor      %%mm2,%%mm2\n"
                    "\tpunpcklbw %%mm2,%%mm3\n"
                    "\tpunpcklbw %%mm0,%%mm2\n"

                    "\tmovq         %3,%%mm4\n"
                    "\tpsubw     %%mm3,%%mm4\n"

                    "\t" pdivwuqX(mm2,mm4,mm5) "\n"

                    "\tmovq      %%mm1,%%mm3\n"
                    "\tpxor      %%mm2,%%mm2\n"
                    "\tpunpckhbw %%mm2,%%mm3\n"
                    "\tpunpckhbw %%mm0,%%mm2\n"

                    "\tmovq         %3,%%mm4\n"
                    "\tpsubw     %%mm3,%%mm4\n"

                    "\t" pdivwuqX(mm2,mm4,mm6) "\n"

                    "\tpackuswb  %%mm6,%%mm5\n"

                    "\tmovq         %4,%%mm6\n"
                    "\tmovq      %%mm1,%%mm7\n"
                    "\t" pminub(mm0,mm7,mm2) "\n"
                    "\tpand      %%mm6,%%mm7\n"
                    "\tpandn     %%mm5,%%mm6\n"

                    "\tpor       %%mm6,%%mm7\n"

                    "\tmovq      %%mm7,%0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b), "m" (*rgba8_w256_64), "m" (*rgba8_alpha_mask_64)
                    : pdivwuqX_clobber, "%mm0", "%mm1", "%mm2", "%mm3", "%mm4", "%mm5", "%mm6", "%mm7");
      a++;
      b++;
      d++;
    }

  if (n_pixels > 0)
    {
      asm volatile ("  movd         %0,%%mm0\n"
                    "\tmovq         %1,%%mm1\n"
                    "\tmovq      %%mm1,%%mm3\n"
                    "\tpxor      %%mm2,%%mm2\n"
                    "\tpunpcklbw %%mm2,%%mm3\n"
                    "\tpunpcklbw %%mm0,%%mm2\n"

                    "\tmovq         %3,%%mm4\n"
                    "\tpsubw     %%mm3,%%mm4\n"

                    "\t" pdivwuqX(mm2,mm4,mm5) "\n"

                    "\tmovq      %%mm1,%%mm3\n"
                    "\tpxor      %%mm2,%%mm2\n"
                    "\tpunpckhbw %%mm2,%%mm3\n"
                    "\tpunpckhbw %%mm0,%%mm2\n"

                    "\tmovq         %3,%%mm4\n"
                    "\tpsubw     %%mm3,%%mm4\n"

                    "\t" pdivwuqX(mm2,mm4,mm6) "\n"

                    "\tpackuswb  %%mm6,%%mm5\n"

                    "\tmovq         %4,%%mm6\n"
                    "\tmovq      %%mm1,%%mm7\n"
                    "\t" pminub(mm0,mm7,mm2) "\n"
                    "\tpand      %%mm6,%%mm7\n"
                    "\tpandn     %%mm5,%%mm6\n"

                    "\tpor       %%mm6,%%mm7\n"

                    "\tmovd      %%mm7,%2\n"
                    : /* empty */
                    : "m" (*a), "m" (*b), "m" (*d), "m" (*rgba8_w256_64), "m" (*rgba8_alpha_mask_64)
                    : pdivwuqX_clobber, "%mm1", "%mm2", "%mm3", "%mm4", "%mm5");
    }

  asm("emms");
}
#endif

void
gimp_composite_grain_extract_rgba8_rgba8_rgba8_mmx (GimpCompositeContext *_op)
{
  uint64 *d = (uint64 *) _op->D;
  uint64 *a = (uint64 *) _op->A;
  uint64 *b = (uint64 *) _op->B;
  gulong n_pixels = _op->n_pixels;

  asm volatile ("movq       %0,%%mm0\n"
                "pxor    %%mm6,%%mm6\n"
                "movq       %1,%%mm7\n"
                : /* no outputs */
                : "m" (*rgba8_alpha_mask_64), "m" (*rgba8_w128_64)
                : "%mm0",  "%mm7", "%mm6");

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

                    "\t" pminub(mm3,mm2,mm4) "\n"
                    "\tpand      %%mm0,%%mm2\n"

                    "\tpor       %%mm2,%%mm1\n"
                    "\tmovq      %%mm1,%0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm1", "%mm2", "%mm3", "%mm4");
      a++;
      b++;
      d++;
    }

  if (n_pixels > 0)
    {
      asm volatile ("  movd    %1, %%mm2\n"
                    "\tmovd    %2, %%mm3\n"

                    mmx_low_bytes_to_words(mm2,mm4,mm6)
                    mmx_low_bytes_to_words(mm3,mm5,mm6)

                    "\tpsubw     %%mm5, %%mm4\n"
                    "\tpaddw     %%mm7, %%mm4\n"
                    "\tmovq      %%mm4, %%mm1\n"

                    "\tpackuswb  %%mm6, %%mm1\n"

                    "\tmovq      %%mm1, %%mm4\n"

                    "\tmovq      %%mm0, %%mm1; pandn     %%mm4, %%mm1\n"

                    "\t" pminub(mm3,mm2,mm4) "\n"
                    "\tpand      %%mm0, %%mm2\n"

                    "\tpor       %%mm2, %%mm1\n"
                    "\tmovd      %%mm1, %0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm1", "%mm2", "%mm3", "%mm4");
    }

  asm("emms");
}

void
gimp_composite_grain_merge_rgba8_rgba8_rgba8_mmx (GimpCompositeContext *_op)
{
  uint64 *d = (uint64 *) _op->D;
  uint64 *a = (uint64 *) _op->A;
  uint64 *b = (uint64 *) _op->B;
  gulong n_pixels = _op->n_pixels;

  asm volatile ("movq    %0, %%mm0\n"
                "pxor    %%mm6, %%mm6\n"
                "movq    %1, %%mm7\n"
                : /* empty */
                : "m" (*rgba8_alpha_mask_64), "m" (*rgba8_w128_64)
                : "%mm0", "%mm6", "%mm7");

  for (; n_pixels >= 2; n_pixels -= 2)
    {
      asm volatile ("  movq    %1, %%mm2\n"
                    "\tmovq    %2, %%mm3\n"

                    mmx_low_bytes_to_words(mm2,mm4,mm6)
                    mmx_low_bytes_to_words(mm3,mm5,mm6)
                    "\tpaddw     %%mm5, %%mm4\n"
                    "\tpsubw     %%mm7, %%mm4\n"

                    mmx_high_bytes_to_words(mm2,mm1,mm6)
                    mmx_high_bytes_to_words(mm3,mm5,mm6)
                    "\tpaddw     %%mm5, %%mm1\n"
                    "\tpsubw     %%mm7, %%mm1\n"

                    "\tpackuswb  %%mm1, %%mm4\n"

                    "\t" pminub(mm3,mm2,mm5) "\n"
                    "\tpand      %%mm0, %%mm2\n"

                    "\tmovq      %%mm0, %%mm1\n"
                    "\tpandn     %%mm4, %%mm1\n"
                    "\tpor       %%mm2, %%mm1\n"
                    "\tmovq      %%mm1, %0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm1", "%mm2", "%mm3", "%mm4");
      a++;
      b++;
      d++;
    }

  if (n_pixels > 0)
    {
      asm volatile ("  movd    %1, %%mm2\n"
                    "\tmovd    %2, %%mm3\n"

                    mmx_low_bytes_to_words(mm2,mm4,mm6)
                    mmx_low_bytes_to_words(mm3,mm5,mm6)

                    "\tpaddw     %%mm5, %%mm4\n"
                    "\tpsubw     %%mm7, %%mm4\n"
                    "\tmovq      %%mm4, %%mm1\n"
                    "\tpackuswb  %%mm6, %%mm1\n"

                    "\tmovq      %%mm1, %%mm4\n"

                    "\tmovq      %%mm0, %%mm1; pandn     %%mm4, %%mm1\n"

                    "\t" pminub(mm3,mm2,mm4) "\n"
                    "\tpand      %%mm0, %%mm2\n"

                    "\tpor       %%mm2, %%mm1\n"
                    "\tmovd      %%mm1, %0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm1", "%mm2", "%mm3", "%mm4");
    }

  asm("emms");
}

void
gimp_composite_lighten_rgba8_rgba8_rgba8_mmx (GimpCompositeContext *_op)
{
  uint64 *d = (uint64 *) _op->D;
  uint64 *a = (uint64 *) _op->A;
  uint64 *b = (uint64 *) _op->B;
  gulong n_pixels = _op->n_pixels;

  asm volatile ("movq    %0,%%mm0"     :  : "m" (*rgba8_alpha_mask_64) : "%mm0");

  for (; n_pixels >= 2; n_pixels -= 2)
    {
      asm volatile ("  movq       %1, %%mm2\n"
                    "\tmovq       %2, %%mm3\n"
                    "\tmovq    %%mm2, %%mm4\n"
                    "\t" pmaxub(mm3,mm4,mm5) "\n"
                    "\tmovq    %%mm0, %%mm1\n"
                    "\tpandn   %%mm4, %%mm1\n"
                    "\t" pminub(mm2,mm3,mm4) "\n"
                    "\tpand    %%mm0, %%mm3\n"
                    "\tpor     %%mm3, %%mm1\n"
                    "\tmovq    %%mm1, %0\n"
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
                    "\t" pmaxub(mm3,mm4,mm5) "\n"

                    "\tmovq    %%mm0, %%mm1\n"
                    "\tpandn   %%mm4, %%mm1\n"

                    "\t" pminub(mm2,mm3,mm4) "\n"

                    "\tpand    %%mm0, %%mm3\n"
                    "\tpor     %%mm3, %%mm1\n"
                    "\tmovd    %%mm1, %0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm1", "%mm2", "%mm3", "%mm4", "%mm5");
    }

  asm("emms");
}

void
gimp_composite_multiply_rgba8_rgba8_rgba8_mmx (GimpCompositeContext *_op)
{
  uint64 *d = (uint64 *) _op->D;
  uint64 *a = (uint64 *) _op->A;
  uint64 *b = (uint64 *) _op->B;
  gulong n_pixels = _op->n_pixels;

  asm volatile (
                "movq    %0,%%mm0\n"
                "movq    %1,%%mm7\n"
                "pxor    %%mm6,%%mm6\n"
                : /* empty */
                : "m" (*rgba8_alpha_mask_64), "m" (*rgba8_w128_64)
                : "%mm6", "%mm7", "%mm0");

  for (; n_pixels >= 2; n_pixels -= 2)
    {
      asm volatile ("  movq        %1, %%mm2\n"
                    "\tmovq        %2, %%mm3\n"

                    mmx_low_bytes_to_words(mm2,mm1,mm6)
                    mmx_low_bytes_to_words(mm3,mm5,mm6)
                    mmx_int_mult(mm5,mm1,mm7)

                    mmx_high_bytes_to_words(mm2,mm4,mm6)
                    mmx_high_bytes_to_words(mm3,mm5,mm6)
                    mmx_int_mult(mm5,mm4,mm7)

                    "\tpackuswb  %%mm4, %%mm1\n"

                    "\tmovq      %%mm0, %%mm4\n"
                    "\tpandn     %%mm1, %%mm4\n"
                    "\tmovq      %%mm4, %%mm1\n"
                    "\t" pminub(mm3,mm2,mm4) "\n"
                    "\tpand      %%mm0, %%mm2\n"
                    "\tpor       %%mm2, %%mm1\n"

                    "\tmovq      %%mm1, %0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm1", "%mm2", "%mm3", "%mm4", "%mm5");
      a++;
      b++;
      d++;
  }

  if (n_pixels > 0)
    {
      asm volatile ("  movd     %1, %%mm2\n"
                    "\tmovd     %2, %%mm3\n"

                    mmx_low_bytes_to_words(mm2,mm1,mm6)
                    mmx_low_bytes_to_words(mm3,mm5,mm6)
                    pmulwX(mm5,mm1,mm7)

                    "\tpackuswb  %%mm6, %%mm1\n"

                    "\tmovq      %%mm0, %%mm4\n"
                    "\tpandn     %%mm1, %%mm4\n"
                    "\tmovq      %%mm4, %%mm1\n"
                    "\t" pminub(mm3,mm2,mm4) "\n"
                    "\tpand      %%mm0, %%mm2\n"
                    "\tpor       %%mm2, %%mm1\n"

                    "\tmovd    %%mm1, %0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm1", "%mm2", "%mm3", "%mm4", "%mm5");
  }

  asm("emms");
}

#if 0
static void
mmx_op_overlay(void)
{
  asm volatile (
                /* low bytes */
                mmx_low_bytes_to_words(mm3,mm5,mm0)
                "\tpcmpeqb   %%mm4,%%mm4\n"
                "\tpsubb     %%mm2,%%mm4\n" /* mm4 = 255 - A */
                "\tpunpcklbw %%mm0,%%mm4\n" /* mm4 = (low bytes as word) mm4 */
                "\tmovq         %0,%%mm6\n"  /* mm6 = words of value 2 */
                "\tpmullw    %%mm5,%%mm6\n" /* mm6 = 2 * low bytes of B */
                mmx_int_mult(mm6,mm4,mm7)    /* mm4 = INT_MULT(mm6, mm4) */

                /* high bytes */
                mmx_high_bytes_to_words(mm3,mm5,mm0)
                "\tpcmpeqb   %%mm1,%%mm1\n"
                "\tpsubb     %%mm2,%%mm1\n" /* mm1 = 255 - A */
                "\tpunpckhbw %%mm0,%%mm1\n" /* mm1 = (high bytes as word) mm1 */
                "\tmovq         %0,%%mm6\n"  /* mm6 = words of value 2 */
                "\tpmullw    %%mm5,%%mm6\n" /* mm6 = 2 * high bytes of B */
                mmx_int_mult(mm6,mm1,mm7)    /* mm1 = INT_MULT(mm6, mm1) */

                "\tpackuswb  %%mm1,%%mm4\n"  /* mm4 = intermediate value */

                mmx_low_bytes_to_words(mm4,mm5,mm0)
                mmx_low_bytes_to_words(mm2,mm6,mm0)
                "\tpaddw     %%mm6,%%mm5\n"
                mmx_int_mult(mm6,mm5,mm7)   /* mm5 = INT_MULT(mm6, mm5) low bytes */

                mmx_high_bytes_to_words(mm4,mm1,mm0)
                mmx_high_bytes_to_words(mm2,mm6,mm0)
                "\tpaddw     %%mm6,%%mm1\n"
                mmx_int_mult(mm6,mm1,mm7)   /* mm1 = INT_MULT(mm6, mm1) high bytes */

                "\tpackuswb  %%mm1,%%mm5\n"

                "\tmovq         %1,%%mm0\n"
                "\tmovq      %%mm0,%%mm1\n"
                "\tpandn     %%mm5,%%mm1\n"

                "\t" pminub(mm2,mm3,mm4) "\n"
                "\tpand      %%mm0,%%mm3\n"

                "\tpor       %%mm3,%%mm1\n"

                : /* empty */
                : "m" (*rgba8_w2_64), "m" (*rgba8_alpha_mask_64)
                );
}
#endif

#if 0
void
xxxgimp_composite_overlay_rgba8_rgba8_rgba8_mmx (GimpCompositeContext *_op)
{
  uint64 *d = (uint64 *) _op->D;
  uint64 *a = (uint64 *) _op->A;
  uint64 *b = (uint64 *) _op->B;
  gulong n_pixels = _op->n_pixels;

  asm volatile ("pxor    %%mm0,%%mm0\n"
                "movq       %0,%%mm7"
                : /* empty */
                : "m" (*rgba8_w128_64) : "%mm0");

  for (; n_pixels >= 2; n_pixels -= 2)
    {
      asm volatile ("  movq         %0,%%mm2\n"
                    "\tmovq         %1,%%mm3\n"

                    /* low bytes */
                    mmx_low_bytes_to_words(mm3,mm5,mm0)
                    "\tpcmpeqb   %%mm4,%%mm4\n"
                    "\tpsubb     %%mm2,%%mm4\n" /* mm4 = 255 - A */
                    "\tpunpcklbw %%mm0,%%mm4\n" /* mm4 = (low bytes as word) mm4 */
                    "\tmovq         %3,%%mm6\n"  /* mm6 = words of value 2 */
                    "\tpmullw    %%mm5,%%mm6\n" /* mm6 = 2 * low bytes of B */
                    mmx_int_mult(mm6,mm4,mm7)    /* mm4 = INT_MULT(mm6, mm4) */

                    /* high bytes */
                    mmx_high_bytes_to_words(mm3,mm5,mm0)
                    "\tpcmpeqb   %%mm1,%%mm1\n"
                    "\tpsubb     %%mm2,%%mm1\n" /* mm1 = 255 - A */
                    "\tpunpckhbw %%mm0,%%mm1\n" /* mm1 = (high bytes as word) mm1 */
                    "\tmovq         %3,%%mm6\n"  /* mm6 = words of value 2 */
                    "\tpmullw    %%mm5,%%mm6\n" /* mm6 = 2 * high bytes of B */
                    mmx_int_mult(mm6,mm1,mm7)    /* mm1 = INT_MULT(mm6, mm1) */

                    "\tpackuswb  %%mm1,%%mm4\n"  /* mm4 = intermediate value */

                    mmx_low_bytes_to_words(mm4,mm5,mm0)
                    mmx_low_bytes_to_words(mm2,mm6,mm0)
                    "\tpaddw     %%mm6,%%mm5\n"
                    mmx_int_mult(mm6,mm5,mm7)   /* mm5 = INT_MULT(mm6, mm5) low bytes */

                    mmx_high_bytes_to_words(mm4,mm1,mm0)
                    mmx_high_bytes_to_words(mm2,mm6,mm0)
                    "\tpaddw     %%mm6,%%mm1\n"
                    mmx_int_mult(mm6,mm1,mm7)   /* mm1 = INT_MULT(mm6, mm1) high bytes */

                    "\tpackuswb  %%mm1,%%mm5\n"

                    "\tmovq         %4,%%mm0\n"
                    "\tmovq      %%mm0,%%mm1\n"
                    "\tpandn     %%mm5,%%mm1\n"

                    "\t" pminub(mm2,mm3,mm4) "\n"
                    "\tpand      %%mm0,%%mm3\n"

                    "\tpor       %%mm3,%%mm1\n"

                    "\tmovq      %%mm1,%2\n"
                    : "+m" (*a), "+m" (*b), "+m" (*d)
                    : "m" (*rgba8_w2_64), "m" (*rgba8_alpha_mask_64)
                    : "%mm1", "%mm2", "%mm3", "%mm4");
      a++;
      b++;
      d++;
  }

  if (n_pixels > 0)
    {
      asm volatile ("  movd         %1,%%mm2\n"
                    "\tmovd         %2,%%mm3\n"

                    /* low bytes */
                    mmx_low_bytes_to_words(mm3,mm5,mm0)
                    "\tpcmpeqb   %%mm4,%%mm4\n"
                    "\tpsubb     %%mm2,%%mm4\n" /* mm4 = 255 - A */
                    "\tpunpcklbw %%mm0,%%mm4\n" /* mm4 = (low bytes as word) mm4 */
                    "\tmovq         %3,%%mm6\n"  /* mm6 = words of integer value 2 */
                    "\tpmullw    %%mm5,%%mm6\n" /* mm6 = 2 * low bytes of B */
                    mmx_int_mult(mm6,mm4,mm7)    /* mm4 = INT_MULT(mm6, mm4) */

                    /* high bytes */
                    mmx_high_bytes_to_words(mm3,mm5,mm0)
                    "\tpcmpeqb   %%mm1,%%mm1\n"
                    "\tpsubb     %%mm2,%%mm1\n" /* mm1 = 255 - A */
                    "\tpunpckhbw %%mm0,%%mm1\n" /* mm1 = (high bytes as word) mm1 */
                    "\tmovq         %3,%%mm6\n"  /* mm6 = words of integer value 2 */
                    "\tpmullw    %%mm5,%%mm6\n" /* mm6 = 2 * high bytes of B */
                    mmx_int_mult(mm6,mm1,mm7)    /* mm1 = INT_MULT(mm6, mm1) */

                    "\tpackuswb  %%mm1,%%mm4\n"  /* mm4 = intermediate value */

                    mmx_low_bytes_to_words(mm4,mm5,mm0)
                    mmx_low_bytes_to_words(mm2,mm6,mm0)
                    "\tpaddw     %%mm6,%%mm5\n"
                    mmx_int_mult(mm6,mm5,mm7)   /* mm5 = INT_MULT(mm6, mm5) low bytes */

                    mmx_high_bytes_to_words(mm4,mm1,mm0)
                    mmx_high_bytes_to_words(mm2,mm6,mm0)
                    "\tpaddw     %%mm6,%%mm1\n"
                    mmx_int_mult(mm6,mm1,mm7)   /* mm1 = INT_MULT(mm6, mm1) high bytes */

                    "\tpackuswb  %%mm1,%%mm5\n"

                    "\tmovq         %4,%%mm0\n"
                    "\tmovq      %%mm0,%%mm1\n"
                    "\tpandn     %%mm5,%%mm1\n"

                    "\t" pminub(mm2,mm3,mm4) "\n"
                    "\tpand      %%mm0,%%mm3\n"

                    "\tpor       %%mm3,%%mm1\n"

                    "\tmovd      %%mm1,%0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b), "m" (*rgba8_w2_64), "m" (*rgba8_alpha_mask_64)
                    : "%mm1", "%mm2", "%mm3", "%mm4");
    }

  asm("emms");
}
#endif

void
gimp_composite_scale_rgba8_rgba8_rgba8_mmx (GimpCompositeContext *_op)
{
  uint64 *d = (uint64 *) _op->D;
  uint64 *a = (uint64 *) _op->A;
  gulong n_pixels = _op->n_pixels;

  asm volatile ("pxor    %%mm0,%%mm0\n"
                "\tmovl     %0,%%eax\n"
                "\tmovl  %%eax,%%ebx\n"
                "\tshl     $16,%%ebx\n"
                "\torl   %%ebx,%%eax\n"
                "\tmovd  %%eax,%%mm5\n"
                "\tmovd  %%eax,%%mm3\n"
                "\tpsllq   $32,%%mm5\n"
                "\tpor   %%mm5,%%mm3\n"
                "\tmovq     %1,%%mm7\n"
                : /* empty */
                : "m" (_op->scale.scale), "m" (*rgba8_w128_64)
                : "%eax", "%ebx", "%mm0", "%mm5", "%mm6", "%mm7");

  for (; n_pixels >= 2; n_pixels -= 2)
    {
      asm volatile ("movq           %1,%%mm2\n"
                    "\tmovq      %%mm2,%%mm1\n"
                    "\tpunpcklbw %%mm0,%%mm1\n"
                    "\tmovq      %%mm3,%%mm5\n"

                    "\t" pmulwX(mm5,mm1,mm7) "\n"

                    "\tmovq      %%mm2,%%mm4\n"
                    "\tpunpckhbw %%mm0,%%mm4\n"
                    "\tmovq      %%mm3,%%mm5\n"

                    "\t" pmulwX(mm5,mm4,mm7) "\n"

                    "\tpackuswb  %%mm4,%%mm1\n"

                    "\tmovq      %%mm1,%0\n"
                    : "=m" (*d)
                    : "m" (*a)
                    : "%mm1", "%mm2", "%mm3", "%mm4", "%mm5", "%mm6", "%mm7");
      a++;
      d++;
    }

  if (n_pixels > 0)
    {
      asm volatile ("movd           %1,%%mm2\n"
                    "\tmovq      %%mm2,%%mm1\n"
                    "\tpunpcklbw %%mm0,%%mm1\n"
                    "\tmovq      %%mm3,%%mm5\n"

                    "\t" pmulwX(mm5,mm1,mm7) "\n"

                    "\tpackuswb  %%mm0,%%mm1\n"
                    "\tmovd      %%mm1,%0\n"
                    : "=m" (*d)
                    : "m" (*a)
                    : "%mm1", "%mm2", "%mm3", "%mm4", "%mm5", "%mm6", "%mm7");
  }

  asm("emms");
}

void
gimp_composite_screen_rgba8_rgba8_rgba8_mmx (GimpCompositeContext *_op)
{
  uint64 *d = (uint64 *) _op->D;
  uint64 *a = (uint64 *) _op->A;
  uint64 *b = (uint64 *) _op->B;
  gulong n_pixels = _op->n_pixels;

  asm volatile ("pxor    %%mm6,%%mm6\n"
                "movq       %0,%%mm0\n"
                "movq       %1,%%mm7\n"
                : /* empty */
                : "m" (*rgba8_alpha_mask_64), "m" (*rgba8_w128_64)
                : "%mm0", "%mm6", "%mm7");

  for (; n_pixels >= 2; n_pixels -= 2)
    {
      asm volatile ("  movq         %1,%%mm2\n"
                    "\tmovq         %2,%%mm3\n"

                    "\tpcmpeqb   %%mm4,%%mm4\n"
                    "\tpsubb     %%mm2,%%mm4\n"
                    "\tpcmpeqb   %%mm5,%%mm5\n"
                    "\tpsubb     %%mm3,%%mm5\n"

                    "\tpunpcklbw %%mm6,%%mm4\n"
                    "\tpunpcklbw %%mm6,%%mm5\n"
                    "\tpmullw    %%mm4,%%mm5\n"
                    "\tpaddw     %%mm7,%%mm5\n"
                    "\tmovq      %%mm5,%%mm1\n"
                    "\tpsrlw       $ 8,%%mm1\n"
                    "\tpaddw     %%mm5,%%mm1\n"
                    "\tpsrlw       $ 8,%%mm1\n"

                    "\tpcmpeqb   %%mm4,%%mm4\n"
                    "\tpsubb     %%mm2,%%mm4\n"
                    "\tpcmpeqb   %%mm5,%%mm5\n"
                    "\tpsubb     %%mm3,%%mm5\n"

                    "\tpunpckhbw %%mm6,%%mm4\n"
                    "\tpunpckhbw %%mm6,%%mm5\n"
                    "\tpmullw    %%mm4,%%mm5\n"
                    "\tpaddw     %%mm7,%%mm5\n"
                    "\tmovq      %%mm5,%%mm4\n"
                    "\tpsrlw       $ 8,%%mm4\n"
                    "\tpaddw     %%mm5,%%mm4\n"
                    "\tpsrlw       $ 8,%%mm4\n"

                    "\tpackuswb  %%mm4,%%mm1\n"

                    "\tpcmpeqb   %%mm4,%%mm4\n"
                    "\tpsubb     %%mm1,%%mm4\n"

                    "\tmovq      %%mm0,%%mm1\n"
                    "\tpandn     %%mm4,%%mm1\n"

                    "\t" pminub(mm2,mm3,mm4) "\n"
                    "\tpand      %%mm0,%%mm3\n"

                    "\tpor       %%mm3,%%mm1\n"

                    "\tmovq      %%mm1,%0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm1", "%mm2", "%mm3", "%mm4", "%mm5");
      a++;
      b++;
      d++;
  }

  if (n_pixels > 0)
    {
      asm volatile ("  movd         %1,%%mm2\n"
                    "\tmovd         %2,%%mm3\n"

                    "\tpcmpeqb   %%mm4,%%mm4\n"
                    "\tpsubb     %%mm2,%%mm4\n"
                    "\tpcmpeqb   %%mm5,%%mm5\n"
                    "\tpsubb     %%mm3,%%mm5\n"

                    "\tpunpcklbw %%mm6,%%mm4\n"
                    "\tpunpcklbw %%mm6,%%mm5\n"
                    "\tpmullw    %%mm4,%%mm5\n"
                    "\tpaddw     %%mm7,%%mm5\n"
                    "\tmovq      %%mm5,%%mm1\n"
                    "\tpsrlw       $ 8,%%mm1\n"
                    "\tpaddw     %%mm5,%%mm1\n"
                    "\tpsrlw       $ 8,%%mm1\n"

                    "\tpcmpeqb   %%mm4,%%mm4\n"
                    "\tpsubb     %%mm2,%%mm4\n"
                    "\tpcmpeqb   %%mm5,%%mm5\n"
                    "\tpsubb     %%mm3,%%mm5\n"

                    "\tpunpckhbw %%mm6,%%mm4\n"
                    "\tpunpckhbw %%mm6,%%mm5\n"
                    "\tpmullw    %%mm4,%%mm5\n"
                    "\tpaddw     %%mm7,%%mm5\n"
                    "\tmovq      %%mm5,%%mm4\n"
                    "\tpsrlw       $ 8,%%mm4\n"
                    "\tpaddw     %%mm5,%%mm4\n"
                    "\tpsrlw       $ 8,%%mm4\n"

                    "\tpackuswb  %%mm4,%%mm1\n"

                    "\tpcmpeqb   %%mm4,%%mm4\n"
                    "\tpsubb     %%mm1,%%mm4\n"

                    "\tmovq      %%mm0,%%mm1\n"
                    "\tpandn     %%mm4,%%mm1\n"

                    "\t" pminub(mm2,mm3,mm4) "\n"
                    "\tpand      %%mm0,%%mm3\n"

                    "\tpor       %%mm3,%%mm1\n"

                    "\tmovd      %%mm1,%0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm1", "%mm2", "%mm3", "%mm4", "%mm5");
    }

  asm volatile ("emms");
}


void
gimp_composite_subtract_rgba8_rgba8_rgba8_mmx (GimpCompositeContext *_op)
{
  uint64 *d = (uint64 *) _op->D;
  uint64 *a = (uint64 *) _op->A;
  uint64 *b = (uint64 *) _op->B;
  gulong n_pixels = _op->n_pixels;

  asm volatile ("movq    %0,%%mm0"     :  : "m" (*rgba8_alpha_mask_64) : "%mm0");

  for (; n_pixels >= 2; n_pixels -= 2)
    {
      asm volatile ("  movq       %1,%%mm2\n"
                    "\tmovq       %2,%%mm3\n"

                    "\tmovq    %%mm2,%%mm4\n"
                    "\tpsubusb %%mm3,%%mm4\n"

                    "\tmovq    %%mm0,%%mm1\n"
                    "\tpandn   %%mm4,%%mm1\n"

                    "\t" pminub(mm3,mm2,mm4) "\n"

                    "\tpand    %%mm0,%%mm2\n"
                    "\tpor     %%mm2,%%mm1\n"
                    "\tmovq    %%mm1,%0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm1", "%mm2", "%mm3", "%mm4", "%mm5");
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

                    "\t" pminub(mm3,mm2,mm4) "\n"

                    "\tpand    %%mm0,%%mm2\n"
                    "\tpor     %%mm2,%%mm1\n"
                    "\tmovd    %%mm1,%0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm1", "%mm2", "%mm3", "%mm4", "%mm5");
    }

  asm volatile ("emms");
}

void
gimp_composite_swap_rgba8_rgba8_rgba8_mmx (GimpCompositeContext *_op)
{
  uint64 *a = (uint64 *) _op->A;
  uint64 *b = (uint64 *) _op->B;
  gulong n_pixels = _op->n_pixels;

  for (; n_pixels >= 2; n_pixels -= 2)
    {
      asm volatile ("  movq       %0,%%mm2\n"
                    "\tmovq       %1,%%mm3\n"
                    "\tmovq    %%mm3,%0\n"
                    "\tmovq    %%mm2,%1\n"
                    : "+m" (*a), "+m" (*b)
                    :
                    : "%mm2", "%mm3");
      a++;
      b++;
    }

  if (n_pixels > 0)
    {
      asm volatile ("  movd       %0,%%mm2\n"
                    "\tmovd       %1,%%mm3\n"
                    "\tmovd    %%mm3,%0\n"
                    "\tmovd    %%mm2,%1\n"
                    : "+m" (*a), "+m" (*b)
                    :
                    : "%mm2", "%mm3");
    }

  asm("emms");
}



void
gimp_composite_addition_va8_va8_va8_mmx (GimpCompositeContext *_op)
{
  uint64 *d = (uint64 *) _op->D;
  uint64 *a = (uint64 *) _op->A;
  uint64 *b = (uint64 *) _op->B;
  uint32 *a32;
  uint32 *b32;
  uint32 *d32;
  uint16 *a16;
  uint16 *b16;
  uint16 *d16;
  gulong n_pixels = _op->n_pixels;

  asm volatile ("movq    %0,%%mm0"
                :
                : "m" (*va8_alpha_mask_64)
                : "%mm0");

  for (; n_pixels >= 4; n_pixels -= 4)
    {
      asm volatile ("  movq       %1, %%mm2\n"
                    "\tmovq       %2, %%mm3\n"
                    "\tmovq    %%mm2, %%mm4\n"
                    "\tpaddusb %%mm3, %%mm4\n"
                    "\tmovq    %%mm0, %%mm1\n"
                    "\tpandn   %%mm4, %%mm1\n"
                    "\t" pminub(mm3, mm2, mm4) "\n"
                    "\tpand    %%mm0, %%mm2\n"
                    "\tpor     %%mm2, %%mm1\n"
                    "\tmovq    %%mm1, %0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm0", "%mm1", "%mm2", "%mm3", "%mm4");
      a++;
      b++;
      d++;
    }

  a32 = (uint32 *) a;
  b32 = (uint32 *) b;
  d32 = (uint32 *) d;

  for (; n_pixels >= 2; n_pixels -= 2)
    {
      asm volatile ("  movd    %1, %%mm2\n"
                    "\tmovd    %2, %%mm3\n"
                    "\tmovq    %%mm2, %%mm4\n"
                    "\tpaddusb %%mm3, %%mm4\n"
                    "\tmovq    %%mm0, %%mm1\n"
                    "\tpandn   %%mm4, %%mm1\n"
                    "\t" pminub(mm3, mm2, mm4) "\n"
                    "\tpand    %%mm0, %%mm2\n"
                    "\tpor     %%mm2, %%mm1\n"
                    "\tmovd    %%mm1, %0\n"
                    : "=m" (*d32)
                    : "m" (*a32), "m" (*b32)
                    : "%mm0", "%mm1", "%mm2", "%mm3", "%mm4");
      a32++;
      b32++;
      d32++;
    }

  a16 = (uint16 *) a32;
  b16 = (uint16 *) b32;
  d16 = (uint16 *) d32;

  for (; n_pixels >= 1; n_pixels -= 1)
    {
      asm volatile ("  movw    %1, %%ax ; movd    %%eax, %%mm2\n"
                    "\tmovw    %2, %%ax ; movd    %%eax, %%mm3\n"
                    "\tmovq    %%mm2, %%mm4\n"
                    "\tpaddusb %%mm3, %%mm4\n"
                    "\tmovq    %%mm0, %%mm1\n"
                    "\tpandn   %%mm4, %%mm1\n"
                    "\t" pminub(mm3, mm2, mm4) "\n"
                    "\tpand    %%mm0, %%mm2\n"
                    "\tpor     %%mm2, %%mm1\n"
                    "\tmovd    %%mm1, %%eax\n"
                    "\tmovw    %%ax, %0\n"
                    : "=m" (*d16)
                    : "m" (*a16), "m" (*b16)
                    : "%eax", "%mm0", "%mm1", "%mm2", "%mm3", "%mm4");

      a16++;
      b16++;
      d16++;
    }

  asm("emms");
}

void
gimp_composite_subtract_va8_va8_va8_mmx (GimpCompositeContext *_op)
{
  uint64 *d = (uint64 *) _op->D;
  uint64 *a = (uint64 *) _op->A;
  uint64 *b = (uint64 *) _op->B;
  uint32 *a32;
  uint32 *b32;
  uint32 *d32;
  uint16 *a16;
  uint16 *b16;
  uint16 *d16;
  gulong n_pixels = _op->n_pixels;

  asm volatile ("movq    %0,%%mm0"
                :
                : "m" (*va8_alpha_mask_64)
                : "%mm0");

  for (; n_pixels >= 4; n_pixels -= 4)
    {
      asm volatile ("  movq       %1, %%mm2\n"
                    "\tmovq       %2, %%mm3\n"
                    "\tmovq    %%mm2, %%mm4\n"
                    "\tpsubusb %%mm3, %%mm4\n"
                    "\tmovq    %%mm0, %%mm1\n"
                    "\tpandn   %%mm4, %%mm1\n"
                    "\t" pminub(mm3, mm2, mm4) "\n"
                    "\tpand    %%mm0, %%mm2\n"
                    "\tpor     %%mm2, %%mm1\n"
                    "\tmovq    %%mm1, %0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm0", "%mm1", "%mm2", "%mm3", "%mm4");
      a++;
      b++;
      d++;
    }

  a32 = (uint32 *) a;
  b32 = (uint32 *) b;
  d32 = (uint32 *) d;

  for (; n_pixels >= 2; n_pixels -= 2)
    {
      asm volatile ("  movd    %1, %%mm2\n"
                    "\tmovd    %2, %%mm3\n"
                    "\tmovq    %%mm2, %%mm4\n"
                    "\tpsubusb %%mm3, %%mm4\n"
                    "\tmovq    %%mm0, %%mm1\n"
                    "\tpandn   %%mm4, %%mm1\n"
                    "\t" pminub(mm3, mm2, mm4) "\n"
                    "\tpand    %%mm0, %%mm2\n"
                    "\tpor     %%mm2, %%mm1\n"
                    "\tmovd    %%mm1, %0\n"
                    : "=m" (*d32)
                    : "m" (*a32), "m" (*b32)
                    : "%mm0", "%mm1", "%mm2", "%mm3", "%mm4");
      a32++;
      b32++;
      d32++;
    }

  a16 = (uint16 *) a32;
  b16 = (uint16 *) b32;
  d16 = (uint16 *) d32;

  for (; n_pixels >= 1; n_pixels -= 1)
    {
      asm volatile ("  movw    %1, %%ax ; movd    %%eax, %%mm2\n"
                    "\tmovw    %2, %%ax ; movd    %%eax, %%mm3\n"
                    "\tmovq    %%mm2, %%mm4\n"
                    "\tpsubusb %%mm3, %%mm4\n"
                    "\tmovq    %%mm0, %%mm1\n"
                    "\tpandn   %%mm4, %%mm1\n"
                    "\t" pminub(mm3, mm2, mm4) "\n"
                    "\tpand    %%mm0, %%mm2\n"
                    "\tpor     %%mm2, %%mm1\n"
                    "\tmovd    %%mm1, %%eax\n"
                    "\tmovw    %%ax, %0\n"
                    : "=m" (*d16)
                    : "m" (*a16), "m" (*b16)
                    : "%eax", "%mm0", "%mm1", "%mm2", "%mm3", "%mm4");

      a16++;
      b16++;
      d16++;
    }

  asm("emms");
}

#if 0
void
gimp_composite_multiply_va8_va8_va8_mmx (GimpCompositeContext *_op)
{
  uint64 *d = (uint64 *) _op->D;
  uint64 *a = (uint64 *) _op->A;
  uint64 *b = (uint64 *) _op->B;
  gulong n_pixels = _op->n_pixels;

  asm volatile (
                "movq    %0,%%mm0\n"
                "movq    %1,%%mm7\n"
                "pxor    %%mm6,%%mm6\n"
                : /* empty */
                : "m" (*va8_alpha_mask_64), "m" (*va8_w128_64)
                : "%mm6", "%mm7", "%mm0");

  for (; n_pixels >= 2; n_pixels -= 2)
    {
      asm volatile ("  movq        %1, %%mm2\n"
                    "\tmovq        %2, %%mm3\n"

                    mmx_low_bytes_to_words(mm2,mm1,mm6)
                    mmx_low_bytes_to_words(mm3,mm5,mm6)
                    mmx_int_mult(mm5,mm1,mm7)

                    mmx_high_bytes_to_words(mm2,mm4,mm6)
                    mmx_high_bytes_to_words(mm3,mm5,mm6)
                    mmx_int_mult(mm5,mm4,mm7)

                    "\tpackuswb  %%mm4, %%mm1\n"

                    "\tmovq      %%mm0, %%mm4\n"
                    "\tpandn     %%mm1, %%mm4\n"
                    "\tmovq      %%mm4, %%mm1\n"
                    "\t" pminub(mm3,mm2,mm4) "\n"
                    "\tpand      %%mm0, %%mm2\n"
                    "\tpor       %%mm2, %%mm1\n"

                    "\tmovq      %%mm1, %0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm1", "%mm2", "%mm3", "%mm4", "%mm5");
      a++;
      b++;
      d++;
  }

  if (n_pixels > 0)
    {
      asm volatile ("  movd     %1, %%mm2\n"
                    "\tmovd     %2, %%mm3\n"

                    mmx_low_bytes_to_words(mm2,mm1,mm6)
                    mmx_low_bytes_to_words(mm3,mm5,mm6)
                    pmulwX(mm5,mm1,mm7)

                    "\tpackuswb  %%mm6, %%mm1\n"

                    "\tmovq      %%mm0, %%mm4\n"
                    "\tpandn     %%mm1, %%mm4\n"
                    "\tmovq      %%mm4, %%mm1\n"
                    "\t" pminub(mm3,mm2,mm4) "\n"
                    "\tpand      %%mm0, %%mm2\n"
                    "\tpor       %%mm2, %%mm1\n"

                    "\tmovd    %%mm1, %0\n"
                    : "=m" (*d)
                    : "m" (*a), "m" (*b)
                    : "%mm1", "%mm2", "%mm3", "%mm4", "%mm5");
  }

  asm("emms");
}
#endif

#if 0
void
gimp_composite_burn_va8_va8_va8_mmx (GimpCompositeContext *_op)
{
  GimpCompositeContext op = *_op;

  asm("movq   %0,%%mm1"
      :
      : "m" (*va8_alpha_mask)
      : "%mm1");

  for (; op.n_pixels >= 4; op.n_pixels -= 4)
    {
    asm volatile ("  movq         %0,%%mm0\n"
                  "\tmovq         %1,%%mm1\n"

                  "\tmovq         %3,%%mm2\n"
                  "\tpsubb     %%mm0,%%mm2\n" /* mm2 = 255 - A */
                  "\tpxor      %%mm4,%%mm4\n"
                  "\tpunpcklbw %%mm2,%%mm4\n" /* mm4 = (255- A) * 256  */

                  "\tmovq      %%mm1,%%mm3\n"
                  "\tpxor      %%mm5,%%mm5\n"
                  "\tpunpcklbw %%mm5,%%mm3\n"
                  "\tmovq         %4,%%mm5\n"
                  "\tpaddusw   %%mm3,%%mm5\n" /* mm5 = B + 1 */

                  "\t" pdivwqX(mm4,mm5,mm7) "\n"

                  "\tmovq         %3,%%mm2\n"
                  "\tpsubb     %%mm0,%%mm2\n" /* mm2 = 255 - A */
                  "\tpxor      %%mm4,%%mm4\n"
                  "\tpunpckhbw %%mm2,%%mm4\n" /* mm4 = (255- A) * 256  */

                  "\tmovq      %%mm1,%%mm3\n"
                  "\tpxor      %%mm5,%%mm5\n"
                  "\tpunpckhbw %%mm5,%%mm3\n"
                  "\tmovq         %4,%%mm5\n"
                  "\tpaddusw   %%mm3,%%mm5\n" /* mm5 = B + 1 */
                  "\t" pdivwqX(mm4,mm5,mm6) "\n"

                  "\tmovq         %5,%%mm4\n"
                  "\tmovq      %%mm4,%%mm5\n"
                  "\tpsubusw   %%mm6,%%mm4\n"
                  "\tpsubusw   %%mm7,%%mm5\n"

                  "\tpackuswb  %%mm4,%%mm5\n"

                  "\t" pminub(mm0,mm1,mm3) "\n" /* mm1 = min(mm0,mm1) clobber mm3 */

                  "\tmovq         %6,%%mm7\n"
                  "\tpand      %%mm7,%%mm1\n" /* mm1 = mm7 & alpha_mask */

                  "\tpandn     %%mm5,%%mm7\n" /* mm7 = ~mm7 & mm5 */
                  "\tpor       %%mm1,%%mm7\n" /* mm7 = mm7 | mm1 */

                  "\tmovq      %%mm7,%2\n"
                  : /* empty */
                  : "+m" (*op.A), "+m" (*op.B), "+m" (*op.D), "m" (*va8_b255), "m" (*va8_w1), "m" (*va8_w255_64), "m" (*va8_alpha_mask)
                  : "%mm1", "%mm2", "%mm3", "%mm4");
      op.A += 8;
      op.B += 8;
      op.D += 8;
  }

  if (op.n_pixels)
    {
    asm volatile ("  movd         %0,%%mm0\n"
                  "\tmovd         %1,%%mm1\n"
                  "\tmovq         %3,%%mm2\n"
                  "\tpsubb     %%mm0,%%mm2\n" /* mm2 = 255 - A */
                  "\tpxor      %%mm4,%%mm4\n"
                  "\tpunpcklbw %%mm2,%%mm4\n" /* mm4 = (255- A) * 256  */

                  "\tmovq      %%mm1,%%mm3\n"
                  "\tpxor      %%mm5,%%mm5\n"
                  "\tpunpcklbw %%mm5,%%mm3\n"
                  "\tmovq         %4,%%mm5\n"
                  "\tpaddusw   %%mm3,%%mm5\n" /* mm5 = B + 1 */

                  "\t" pdivwqX(mm4,mm5,mm7) "\n"

                  "\tmovq         %3,%%mm2\n"
                  "\tpsubb     %%mm0,%%mm2\n" /* mm2 = 255 - A */
                  "\tpxor      %%mm4,%%mm4\n"
                  "\tpunpckhbw %%mm2,%%mm4\n" /* mm4 = (255- A) * 256  */

                  "\tmovq      %%mm1,%%mm3\n"
                  "\tpxor      %%mm5,%%mm5\n"
                  "\tpunpckhbw %%mm5,%%mm3\n"
                  "\tmovq         %4,%%mm5\n"
                  "\tpaddusw   %%mm3,%%mm5\n" /* mm5 = B + 1 */
                  "\t" pdivwqX(mm4,mm5,mm6) "\n"

                  "\tmovq         %5,%%mm4\n"
                  "\tmovq      %%mm4,%%mm5\n"
                  "\tpsubusw   %%mm6,%%mm4\n"
                  "\tpsubusw   %%mm7,%%mm5\n"

                  "\tpackuswb  %%mm4,%%mm5\n"

                  "\t" pminub(mm0,mm1,mm3) "\n" /* mm1 = min(mm0,mm1) clobber mm3 */

                  "\tmovq         %6,%%mm7\n"
                  "\tpand      %%mm7,%%mm1\n" /* mm1 = mm7 & alpha_mask */

                  "\tpandn     %%mm5,%%mm7\n" /* mm7 = ~mm7 & mm5 */
                  "\tpor       %%mm1,%%mm7\n" /* mm7 = mm7 | mm1 */

                  "\tmovd      %%mm7,%2\n"
                  : /* empty */
                  : "m" (*op.A), "m" (*op.B), "m" (*op.D), "m" (*va8_b255), "m" (*va8_w1), "m" (*va8_w255_64), "m" (*va8_alpha_mask)
                  : "%mm0", "%mm1", "%mm2", "%mm3", "%mm4", "%mm5", "%mm6", "%mm7");
  }

  asm("emms");
}

void
xxxgimp_composite_coloronly_va8_va8_va8_mmx (GimpCompositeContext *_op)
{
  GimpCompositeContext op = *_op;

}

void
xxxgimp_composite_darken_va8_va8_va8_mmx (GimpCompositeContext *_op)
{
  GimpCompositeContext op = *_op;

  asm("pushl %edi");
  asm("pushl %ebx");
  asm("movl 12(%esp), %edi");
  asm("movq v8_alpha_mask, %mm0");
  asm("subl $ 4, %ecx");
  asm("jl .darken_pixels_1a_1a_last3");
  asm("movl $ 8, %ebx");
  asm(".darken_pixels_1a_1a_loop:");
  asm("movq (%eax), %mm2");
  asm("movq (%edx), %mm3");

  asm("movq %mm2, %mm4");
  asm("psubusb %mm3, %mm4");
  asm("psubb %mm4, %mm2");
  asm("movq %mm2, %mm1");
  asm("movq %mm1, (%edi)");
  asm("addl %ebx, %eax");
  asm("addl %ebx, %edx");
  asm("addl %ebx, %edi");
  asm("subl $ 4, %ecx");
  asm("jge .darken_pixels_1a_1a_loop");

  asm(".darken_pixels_1a_1a_last3:");
  asm("test $ 2, %ecx");
  asm("jz .darken_pixels_1a_1a_last1");
  asm("movd (%eax), %mm2");
  asm("movd (%edx), %mm3");

  asm("movq %mm2, %mm4");
  asm("psubusb %mm3, %mm4");
  asm("psubb %mm4, %mm2");
  asm("movq %mm2, %mm1");
  asm("addl $ 4, %eax");
  asm("addl $ 4, %edx");
  asm("addl $ 4, %edi");

  asm(".darken_pixels_1a_1a_last1:");
  asm("test $ 1, %ecx");
  asm("jz .darken_pixels_1a_1a_end");

  asm("movw (%eax), %bx");
  asm("movd %ebx, %mm2");
  asm("movw (%edx), %bx");
  asm("movd %ebx, %mm3");

  asm("movq %mm2, %mm4");
  asm("psubusb %mm3, %mm4");
  asm("psubb %mm4, %mm2");
  asm("movq %mm2, %mm1");
  asm("movd %mm1, %ebx");
  asm("movw %bx, (%edi)");

  asm(".darken_pixels_1a_1a_end:");

  asm("emms");
  asm("popl %ebx");
  asm("popl %edi");
}

void
xxxgimp_composite_difference_va8_va8_va8_mmx (GimpCompositeContext *_op)
{
  GimpCompositeContext op = *_op;

  asm("pushl %edi");
  asm("pushl %ebx");
  asm("movl 12(%esp), %edi");
  asm("movq v8_alpha_mask, %mm0");
  asm("subl $ 4, %ecx");
  asm("jl .difference_pixels_1a_1a_last3");
  asm("movl $ 8, %ebx");
  asm(".difference_pixels_1a_1a_loop:");
  asm("movq (%eax), %mm2");
  asm("movq (%edx), %mm3");

  asm("movq %mm2, %mm4");
  asm("movq %mm3, %mm5");
  asm("psubusb %mm3, %mm4");
  asm("psubusb %mm2, %mm5");
  asm("movq %mm0, %mm1");
  asm("paddb %mm5, %mm4");
  asm("pandn %mm4, %mm1");
  asm("psubb %mm4, %mm2");
  asm("pand %mm0, %mm2");
  asm("por %mm2, %mm1");
  asm("movq %mm1, (%edi)");
  asm("addl %ebx, %eax");
  asm("addl %ebx, %edx");
  asm("addl %ebx, %edi");
  asm("subl $ 4, %ecx");
  asm("jge .difference_pixels_1a_1a_loop");

  asm(".difference_pixels_1a_1a_last3:");
  asm("test $ 2, %ecx");
  asm("jz .difference_pixels_1a_1a_last1");
  asm("movd (%eax), %mm2");
  asm("movd (%edx), %mm3");

  asm("movq %mm2, %mm4");
  asm("movq %mm3, %mm5");
  asm("psubusb %mm3, %mm4");
  asm("psubusb %mm2, %mm5");
  asm("movq %mm0, %mm1");
  asm("paddb %mm5, %mm4");
  asm("pandn %mm4, %mm1");
  asm("psubb %mm4, %mm2");
  asm("pand %mm0, %mm2");
  asm("por %mm2, %mm1");
  asm("addl $ 4, %eax");
  asm("addl $ 4, %edx");
  asm("addl $ 4, %edi");

  asm(".difference_pixels_1a_1a_last1:");
  asm("test $ 1, %ecx");
  asm("jz .difference_pixels_1a_1a_end");

  asm("movw (%eax), %bx");
  asm("movd %ebx, %mm2");
  asm("movw (%edx), %bx");
  asm("movd %ebx, %mm3");

  asm("movq %mm2, %mm4");
  asm("movq %mm3, %mm5");
  asm("psubusb %mm3, %mm4");
  asm("psubusb %mm2, %mm5");
  asm("movq %mm0, %mm1");
  asm("paddb %mm5, %mm4");
  asm("pandn %mm4, %mm1");
  asm("psubb %mm4, %mm2");
  asm("pand %mm0, %mm2");
  asm("por %mm2, %mm1");
  asm("movd %mm1, %ebx");
  asm("movw %bx, (%edi)");

  asm(".difference_pixels_1a_1a_end:");

  asm("emms");
  asm("popl %ebx");
  asm("popl %edi");
}

void
xxxgimp_composite_dissolve_va8_va8_va8_mmx (GimpCompositeContext *_op)
{
  GimpCompositeContext op = *_op;

}

void
xxxgimp_composite_divide_va8_va8_va8_mmx (GimpCompositeContext *_op)
{
  GimpCompositeContext op = *_op;

}

void
xxxgimp_composite_dodge_va8_va8_va8_mmx (GimpCompositeContext *_op)
{
  GimpCompositeContext op = *_op;

}

void
xxxgimp_composite_grain_extract_va8_va8_va8_mmx (GimpCompositeContext *_op)
{
  GimpCompositeContext op = *_op;

}

void
xxxgimp_composite_grain_merge_va8_va8_va8_mmx (GimpCompositeContext *_op)
{
  GimpCompositeContext op = *_op;

}

void
xxxgimp_composite_hardlight_va8_va8_va8_mmx (GimpCompositeContext *_op)
{
  GimpCompositeContext op = *_op;

}

void
xxxgimp_composite_hueonly_va8_va8_va8_mmx (GimpCompositeContext *_op)
{
  GimpCompositeContext op = *_op;

}

void
xxxgimp_composite_lighten_va8_va8_va8_mmx (GimpCompositeContext *_op)
{
  GimpCompositeContext op = *_op;

  asm("pushl %edi");
  asm("pushl %ebx");
  asm("movl 12(%esp), %edi");
  asm("movq v8_alpha_mask, %mm0");
  asm("subl $ 4, %ecx");
  asm("jl .lighten_pixels_1a_1a_last3");
  asm("movl $ 8, %ebx");
  asm(".lighten_pixels_1a_1a_loop:");
  asm("movq (%eax), %mm2");
  asm("movq (%edx), %mm3");

  asm("movq %mm2, %mm4");
  asm("psubusb %mm3, %mm4");
  asm("paddb %mm4, %mm3");
  asm("movq %mm0, %mm1");
  asm("pandn %mm3, %mm1");

  asm("psubb %mm4, %mm2");
  asm("pand %mm0, %mm2");
  asm("por %mm2, %mm1");
  asm("movq %mm1, (%edi)");
  asm("addl %ebx, %eax");
  asm("addl %ebx, %edx");
  asm("addl %ebx, %edi");
  asm("subl $ 4, %ecx");
  asm("jge .lighten_pixels_1a_1a_loop");

  asm(".lighten_pixels_1a_1a_last3:");
  asm("test $ 2, %ecx");
  asm("jz .lighten_pixels_1a_1a_last1");
  asm("movd (%eax), %mm2");
  asm("movd (%edx), %mm3");

  asm("movq %mm2, %mm4");
  asm("psubusb %mm3, %mm4");
  asm("paddb %mm4, %mm3");
  asm("movq %mm0, %mm1");
  asm("pandn %mm3, %mm1");

  asm("psubb %mm4, %mm2");
  asm("pand %mm0, %mm2");
  asm("por %mm2, %mm1");
  asm("addl $ 4, %eax");
  asm("addl $ 4, %edx");
  asm("addl $ 4, %edi");

  asm(".lighten_pixels_1a_1a_last1:");
  asm("test $ 1, %ecx");
  asm("jz .lighten_pixels_1a_1a_end");

  asm("movw (%eax), %bx");
  asm("movd %ebx, %mm2");
  asm("movw (%edx), %bx");
  asm("movd %ebx, %mm3");

  asm("movq %mm2, %mm4");
  asm("psubusb %mm3, %mm4");
  asm("paddb %mm4, %mm3");
  asm("movq %mm0, %mm1");
  asm("pandn %mm3, %mm1");

  asm("psubb %mm4, %mm2");
  asm("pand %mm0, %mm2");
  asm("por %mm2, %mm1");
  asm("movd %mm1, %ebx");
  asm("movw %bx, (%edi)");

  asm(".lighten_pixels_1a_1a_end:");

  asm("emms");
  asm("popl %ebx");
  asm("popl %edi");
}

void
xxxgimp_composite_multiply_va8_va8_va8_mmx (GimpCompositeContext *_op)
{
  GimpCompositeContext op = *_op;

  asm("pushl %edi");
  asm("pushl %ebx");
  asm("movl 12(%esp), %edi");
  asm("movq v8_alpha_mask, %mm0");
  asm("subl $ 4, %ecx");
  asm("jl .multiply_pixels_1a_1a_last3");
  asm("movl $ 8, %ebx");
  asm(".multiply_pixels_1a_1a_loop:");
  asm("movq (%eax), %mm2");
  asm("movq (%edx), %mm3");


  asm("movq %mm2, %mm1");
  asm("punpcklbw %mm6, %mm1");
  asm("movq %mm3, %mm5");
  asm("punpcklbw %mm6, %mm5");
  asm("pmullw %mm5, %mm1");
  asm("paddw %mm7, %mm1");
  asm("movq %mm1, %mm5");
  asm("psrlw $ 8, %mm5");
  asm("paddw %mm5, %mm1");
  asm("psrlw $ 8, %mm1");

  asm("movq %mm2, %mm4");
  asm("punpckhbw %mm6, %mm4");
  asm("movq %mm3, %mm5");
  asm("punpckhbw %mm6, %mm5");
  asm("pmullw %mm5, %mm4");
  asm("paddw %mm7, %mm4");
  asm("movq %mm4, %mm5");
  asm("psrlw $ 8, %mm5");
  asm("paddw %mm5, %mm4");
  asm("psrlw $ 8, %mm4");

  asm("packuswb %mm4, %mm1");

  asm("movq %mm0, %mm4");
  asm("pandn %mm1, %mm4");
  asm("movq %mm4, %mm1");

  asm("movq %mm2, %mm4");
  asm("psubusb %mm3, %mm4");
  asm("psubb %mm4, %mm2");
  asm("pand %mm0, %mm2");
  asm("por %mm2, %mm1");
  asm("movq %mm1, (%edi)");
  asm("addl %ebx, %eax");
  asm("addl %ebx, %edx");
  asm("addl %ebx, %edi");
  asm("subl $ 4, %ecx");
  asm("jge .multiply_pixels_1a_1a_loop");

  asm(".multiply_pixels_1a_1a_last3:");
  asm("test $ 2, %ecx");
  asm("jz .multiply_pixels_1a_1a_last1");
  asm("movd (%eax), %mm2");
  asm("movd (%edx), %mm3");


  asm("movq %mm2, %mm1");
  asm("punpcklbw %mm6, %mm1");
  asm("movq %mm3, %mm5");
  asm("punpcklbw %mm6, %mm5");
  asm("pmullw %mm5, %mm1");
  asm("paddw %mm7, %mm1");
  asm("movq %mm1, %mm5");
  asm("psrlw $ 8, %mm5");
  asm("paddw %mm5, %mm1");
  asm("psrlw $ 8, %mm1");

  asm("movq %mm2, %mm4");
  asm("punpckhbw %mm6, %mm4");
  asm("movq %mm3, %mm5");
  asm("punpckhbw %mm6, %mm5");
  asm("pmullw %mm5, %mm4");
  asm("paddw %mm7, %mm4");
  asm("movq %mm4, %mm5");
  asm("psrlw $ 8, %mm5");
  asm("paddw %mm5, %mm4");
  asm("psrlw $ 8, %mm4");

  asm("packuswb %mm4, %mm1");

  asm("movq %mm0, %mm4");
  asm("pandn %mm1, %mm4");
  asm("movq %mm4, %mm1");

  asm("movq %mm2, %mm4");
  asm("psubusb %mm3, %mm4");
  asm("psubb %mm4, %mm2");
  asm("pand %mm0, %mm2");
  asm("por %mm2, %mm1");
  asm("addl $ 4, %eax");
  asm("addl $ 4, %edx");
  asm("addl $ 4, %edi");

  asm(".multiply_pixels_1a_1a_last1:");
  asm("test $ 1, %ecx");
  asm("jz .multiply_pixels_1a_1a_end");

  asm("movw (%eax), %bx");
  asm("movd %ebx, %mm2");
  asm("movw (%edx), %bx");
  asm("movd %ebx, %mm3");


  asm("movq %mm2, %mm1");
  asm("punpcklbw %mm6, %mm1");
  asm("movq %mm3, %mm5");
  asm("punpcklbw %mm6, %mm5");
  asm("pmullw %mm5, %mm1");
  asm("paddw %mm7, %mm1");
  asm("movq %mm1, %mm5");
  asm("psrlw $ 8, %mm5");
  asm("paddw %mm5, %mm1");
  asm("psrlw $ 8, %mm1");

  asm("movq %mm2, %mm4");
  asm("punpckhbw %mm6, %mm4");
  asm("movq %mm3, %mm5");
  asm("punpckhbw %mm6, %mm5");
  asm("pmullw %mm5, %mm4");
  asm("paddw %mm7, %mm4");
  asm("movq %mm4, %mm5");
  asm("psrlw $ 8, %mm5");
  asm("paddw %mm5, %mm4");
  asm("psrlw $ 8, %mm4");

  asm("packuswb %mm4, %mm1");

  asm("movq %mm0, %mm4");
  asm("pandn %mm1, %mm4");
  asm("movq %mm4, %mm1");

  asm("movq %mm2, %mm4");
  asm("psubusb %mm3, %mm4");
  asm("psubb %mm4, %mm2");
  asm("pand %mm0, %mm2");
  asm("por %mm2, %mm1");
  asm("movd %mm1, %ebx");
  asm("movw %bx, (%edi)");

  asm(".multiply_pixels_1a_1a_end:");

  asm("emms");
  asm("popl %ebx");
  asm("popl %edi");
}

void
xxxgimp_composite_overlay_va8_va8_va8_mmx (GimpCompositeContext *_op)
{
  GimpCompositeContext op = *_op;

  asm("pushl %edi");
  asm("pushl %ebx");
  asm("movl 12(%esp), %edi");
  asm("movq v8_alpha_mask, %mm0");
  asm("subl $ 4, %ecx");
  asm("jl .overlay_pixels_1a_1a_last3");
  asm("movl $ 8, %ebx");
  asm(".overlay_pixels_1a_1a_loop:");
  asm("movq (%eax), %mm2");
  asm("movq (%edx), %mm3");
  asm("call op_overlay");
  asm("movq %mm1, (%edi)");
  asm("addl %ebx, %eax");
  asm("addl %ebx, %edx");
  asm("addl %ebx, %edi");
  asm("subl $ 4, %ecx");
  asm("jge .overlay_pixels_1a_1a_loop");

  asm(".overlay_pixels_1a_1a_last3:");
  asm("test $ 2, %ecx");
  asm("jz .overlay_pixels_1a_1a_last1");
  asm("movd (%eax), %mm2");
  asm("movd (%edx), %mm3");
  asm("call op_overlay");
  asm("addl $ 4, %eax");
  asm("addl $ 4, %edx");
  asm("addl $ 4, %edi");

  asm(".overlay_pixels_1a_1a_last1:");
  asm("test $ 1, %ecx");
  asm("jz .overlay_pixels_1a_1a_end");

  asm("movw (%eax), %bx");
  asm("movd %ebx, %mm2");
  asm("movw (%edx), %bx");
  asm("movd %ebx, %mm3");
  asm("call op_overlay");
  asm("movd %mm1, %ebx");
  asm("movw %bx, (%edi)");

  asm(".overlay_pixels_1a_1a_end:");

  asm("emms");
  asm("popl %ebx");
  asm("popl %edi");
}

void
xxxgimp_composite_replace_va8_va8_va8_mmx (GimpCompositeContext *_op)
{
  GimpCompositeContext op = *_op;

}

void
xxxgimp_composite_saturationonly_va8_va8_va8_mmx (GimpCompositeContext *_op)
{
  GimpCompositeContext op = *_op;

}

void
xxxgimp_composite_screen_va8_va8_va8_mmx (GimpCompositeContext *_op)
{
  GimpCompositeContext op = *_op;

  asm("pushl %edi");
  asm("pushl %ebx");
  asm("movl 12(%esp), %edi");
  asm("movq v8_alpha_mask, %mm0");
  asm("subl $ 4, %ecx");
  asm("jl .screen_pixels_1a_1a_last3");
  asm("movl $ 8, %ebx");
  asm(".screen_pixels_1a_1a_loop:");
  asm("movq (%eax), %mm2");
  asm("movq (%edx), %mm3");


  asm("pcmpeqb %mm4, %mm4");
  asm("psubb %mm2, %mm4");
  asm("pcmpeqb %mm5, %mm5");
  asm("psubb %mm3, %mm5");

  asm("movq %mm4, %mm1");
  asm("punpcklbw %mm6, %mm1");
  asm("movq %mm5, %mm3");
  asm("punpcklbw %mm6, %mm3");
  asm("pmullw %mm3, %mm1");
  asm("paddw %mm7, %mm1");
  asm("movq %mm1, %mm3");
  asm("psrlw $ 8, %mm3");
  asm("paddw %mm3, %mm1");
  asm("psrlw $ 8, %mm1");

  asm("movq %mm4, %mm2");
  asm("punpckhbw %mm6, %mm2");
  asm("movq %mm5, %mm3");
  asm("punpckhbw %mm6, %mm3");
  asm("pmullw %mm3, %mm2");
  asm("paddw %mm7, %mm2");
  asm("movq %mm2, %mm3");
  asm("psrlw $ 8, %mm3");
  asm("paddw %mm3, %mm2");
  asm("psrlw $ 8, %mm2");

  asm("packuswb %mm2, %mm1");

  asm("pcmpeqb %mm3, %mm3");
  asm("psubb %mm1, %mm3");

  asm("movq %mm0, %mm1");
  asm("pandn %mm3, %mm1");

  asm("movq %mm2, %mm4");
  asm("psubusb %mm5, %mm2");
  asm("paddb %mm2, %mm5");
  asm("pcmpeqb %mm3, %mm3");
  asm("psubb %mm5, %mm3");

  asm("pand %mm0, %mm3");
  asm("por %mm3, %mm1");
  asm("movq %mm1, (%edi)");
  asm("addl %ebx, %eax");
  asm("addl %ebx, %edx");
  asm("addl %ebx, %edi");
  asm("subl $ 4, %ecx");
  asm("jge .screen_pixels_1a_1a_loop");

  asm(".screen_pixels_1a_1a_last3:");
  asm("test $ 2, %ecx");
  asm("jz .screen_pixels_1a_1a_last1");
  asm("movd (%eax), %mm2");
  asm("movd (%edx), %mm3");


  asm("pcmpeqb %mm4, %mm4");
  asm("psubb %mm2, %mm4");
  asm("pcmpeqb %mm5, %mm5");
  asm("psubb %mm3, %mm5");

  asm("movq %mm4, %mm1");
  asm("punpcklbw %mm6, %mm1");
  asm("movq %mm5, %mm3");
  asm("punpcklbw %mm6, %mm3");
  asm("pmullw %mm3, %mm1");
  asm("paddw %mm7, %mm1");
  asm("movq %mm1, %mm3");
  asm("psrlw $ 8, %mm3");
  asm("paddw %mm3, %mm1");
  asm("psrlw $ 8, %mm1");

  asm("movq %mm4, %mm2");
  asm("punpckhbw %mm6, %mm2");
  asm("movq %mm5, %mm3");
  asm("punpckhbw %mm6, %mm3");
  asm("pmullw %mm3, %mm2");
  asm("paddw %mm7, %mm2");
  asm("movq %mm2, %mm3");
  asm("psrlw $ 8, %mm3");
  asm("paddw %mm3, %mm2");
  asm("psrlw $ 8, %mm2");

  asm("packuswb %mm2, %mm1");

  asm("pcmpeqb %mm3, %mm3");
  asm("psubb %mm1, %mm3");

  asm("movq %mm0, %mm1");
  asm("pandn %mm3, %mm1");

  asm("movq %mm2, %mm4");
  asm("psubusb %mm5, %mm2");
  asm("paddb %mm2, %mm5");
  asm("pcmpeqb %mm3, %mm3");
  asm("psubb %mm5, %mm3");

  asm("pand %mm0, %mm3");
  asm("por %mm3, %mm1");
  asm("addl $ 4, %eax");
  asm("addl $ 4, %edx");
  asm("addl $ 4, %edi");

  asm(".screen_pixels_1a_1a_last1:");
  asm("test $ 1, %ecx");
  asm("jz .screen_pixels_1a_1a_end");

  asm("movw (%eax), %bx");
  asm("movd %ebx, %mm2");
  asm("movw (%edx), %bx");
  asm("movd %ebx, %mm3");


  asm("pcmpeqb %mm4, %mm4");
  asm("psubb %mm2, %mm4");
  asm("pcmpeqb %mm5, %mm5");
  asm("psubb %mm3, %mm5");

  asm("movq %mm4, %mm1");
  asm("punpcklbw %mm6, %mm1");
  asm("movq %mm5, %mm3");
  asm("punpcklbw %mm6, %mm3");
  asm("pmullw %mm3, %mm1");
  asm("paddw %mm7, %mm1");
  asm("movq %mm1, %mm3");
  asm("psrlw $ 8, %mm3");
  asm("paddw %mm3, %mm1");
  asm("psrlw $ 8, %mm1");

  asm("movq %mm4, %mm2");
  asm("punpckhbw %mm6, %mm2");
  asm("movq %mm5, %mm3");
  asm("punpckhbw %mm6, %mm3");
  asm("pmullw %mm3, %mm2");
  asm("paddw %mm7, %mm2");
  asm("movq %mm2, %mm3");
  asm("psrlw $ 8, %mm3");
  asm("paddw %mm3, %mm2");
  asm("psrlw $ 8, %mm2");

  asm("packuswb %mm2, %mm1");

  asm("pcmpeqb %mm3, %mm3");
  asm("psubb %mm1, %mm3");

  asm("movq %mm0, %mm1");
  asm("pandn %mm3, %mm1");

  asm("movq %mm2, %mm4");
  asm("psubusb %mm5, %mm2");
  asm("paddb %mm2, %mm5");
  asm("pcmpeqb %mm3, %mm3");
  asm("psubb %mm5, %mm3");

  asm("pand %mm0, %mm3");
  asm("por %mm3, %mm1");
  asm("movd %mm1, %ebx");
  asm("movw %bx, (%edi)");

  asm(".screen_pixels_1a_1a_end:");

  asm("emms");
  asm("popl %ebx");
  asm("popl %edi");
}

void
xxxgimp_composite_softlight_va8_va8_va8_mmx (GimpCompositeContext *_op)
{
  GimpCompositeContext op = *_op;

}

void
xxxgimp_composite_subtract_va8_va8_va8_mmx (GimpCompositeContext *_op)
{
  GimpCompositeContext op = *_op;

  asm("pushl %edi");
  asm("pushl %ebx");
  asm("movl 12(%esp), %edi");
  asm("movq v8_alpha_mask, %mm0");
  asm("subl $ 4, %ecx");
  asm("jl .substract_pixels_1a_1a_last3");
  asm("movl $ 8, %ebx");
  asm(".substract_pixels_1a_1a_loop:");
  asm("movq (%eax), %mm2");
  asm("movq (%edx), %mm3");

  asm("movq %mm2, %mm4");
  asm("psubusb %mm3, %mm4");
  asm("movq %mm0, %mm1");
  asm("pandn %mm4, %mm1");
  asm("psubb %mm4, %mm2");
  asm("pand %mm0, %mm2");
  asm("por %mm2, %mm1");
  asm("movq %mm1, (%edi)");
  asm("addl %ebx, %eax");
  asm("addl %ebx, %edx");
  asm("addl %ebx, %edi");
  asm("subl $ 4, %ecx");
  asm("jge .substract_pixels_1a_1a_loop");

  asm(".substract_pixels_1a_1a_last3:");
  asm("test $ 2, %ecx");
  asm("jz .substract_pixels_1a_1a_last1");
  asm("movd (%eax), %mm2");
  asm("movd (%edx), %mm3");

  asm("movq %mm2, %mm4");
  asm("psubusb %mm3, %mm4");
  asm("movq %mm0, %mm1");
  asm("pandn %mm4, %mm1");
  asm("psubb %mm4, %mm2");
  asm("pand %mm0, %mm2");
  asm("por %mm2, %mm1");
  asm("addl $ 4, %eax");
  asm("addl $ 4, %edx");
  asm("addl $ 4, %edi");

  asm(".substract_pixels_1a_1a_last1:");
  asm("test $ 1, %ecx");
  asm("jz .substract_pixels_1a_1a_end");

  asm("movw (%eax), %bx");
  asm("movd %ebx, %mm2");
  asm("movw (%edx), %bx");
  asm("movd %ebx, %mm3");

  asm("movq %mm2, %mm4");
  asm("psubusb %mm3, %mm4");
  asm("movq %mm0, %mm1");
  asm("pandn %mm4, %mm1");
  asm("psubb %mm4, %mm2");
  asm("pand %mm0, %mm2");
  asm("por %mm2, %mm1");
  asm("movd %mm1, %ebx");
  asm("movw %bx, (%edi)");

  asm(".substract_pixels_1a_1a_end:");
  asm("emms");
  asm("popl %ebx");
  asm("popl %edi");
}

void
xxxgimp_composite_swap_va8_va8_va8_mmx (GimpCompositeContext *_op)
{
  GimpCompositeContext op = *_op;

}

void
xxxgimp_composite_valueonly_va8_va8_va8_mmx (GimpCompositeContext *_op)
{
  GimpCompositeContext op = *_op;

}
#endif

#endif /* COMPILE_IS_OKAY */
