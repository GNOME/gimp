/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * -*- mode: c tab-width: 2; c-basic-indent: 2; indent-tabs-mode: nil -*-
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

#if defined(USE_SSE)
#if defined(ARCH_X86)

#include <stdio.h>

#include <glib-object.h>

#include "base/base-types.h"

#include "gimp-composite.h"
#include "gimp-composite-sse2.h"

#if __GNUC__ >= 3

#define pminub(src,dst,tmp)  "pminub " "%%" #src ", %%" #dst
#define pmaxub(src,dst,tmp)  "pmaxub " "%%" #src ", %%" #dst

const static unsigned long rgba8_alpha_mask_128[4] = { 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000 };

void
debug_display_sse(void)
{
#define mask32(x) ((x)& (unsigned long long) 0xFFFFFFFF)
#define print128(reg) { \
		unsigned long long reg[2]; \
  asm("movdqu %%" #reg ",%0" : "=m" (reg)); \
  printf(#reg"=%08llx %08llx", mask32(reg[0]>>32), mask32(reg[0])); \
  printf(" %08llx %08llx", mask32(reg[1]>>32), mask32(reg[1])); \
 }
  printf("--------------------------------------------\n");
  print128(xmm0); printf("  "); print128(xmm1); printf("\n");
  print128(xmm2); printf("  "); print128(xmm3); printf("\n");
  print128(xmm4); printf("  "); print128(xmm5); printf("\n");
  print128(xmm6); printf("  "); print128(xmm7); printf("\n");
  printf("--------------------------------------------\n");
}

void
xxxgimp_composite_addition_rgba8_rgba8_rgba8_sse2 (GimpCompositeContext *_op)
{
  GimpCompositeContext op = *_op;

  asm volatile ("movdqu    %0,%%xmm0"
                : /* empty */
                : "m" (*rgba8_alpha_mask_128)
                : "%xmm0");

  for (; op.n_pixels >= 4; op.n_pixels -= 4)
    {
						asm ("  movdqu  %0, %%xmm2\n"
											"\tmovdqu  %1, %%xmm3\n"
											"\tmovdqu  %%xmm2, %%xmm4\n"
											"\tpaddusb %%xmm3, %%xmm4\n"

											"\tmovdqu  %%xmm0, %%xmm1\n"
											"\tpandn   %%xmm4, %%xmm1\n"
											"\t" pminub(xmm3, xmm2, xmm4) "\n"
											"\tpand    %%xmm0, %%xmm2\n"
											"\tpor     %%xmm2, %%xmm1\n"
											"\tmovdqu  %%xmm1, %2\n"
											: /* empty */
											: "m" (*op.A), "m" (*op.B), "m" (*op.D)
											: "0", "1", "2", "%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6", "%xmm7");
						op.A += 16;
						op.B += 16;
						op.D += 16;
				}

  if (op.n_pixels)
    {
      asm volatile ("  movd    (%0), %%mm2;\n"
                    "\tmovd    (%1), %%mm3;\n"
																				"\tmovq    %%mm2, %%mm4\n"
																				"\tpaddusb %%mm3, %%mm4\n"
																				"\tmovq    %%mm0, %%mm1\n"
																				"\tpandn   %%mm4, %%mm1\n"
																				"\t" pminub(mm3, mm2, mm4) "\n"
																				"\tpand    %%mm0, %%mm2\n"
																				"\tpor     %%mm2, %%mm1\n"
																				"\tmovd    %%mm1, (%2);\n"
																				: /* empty */
																				: "r" (op.A), "r" (op.B), "r" (op.D)
																				: "0", "1", "2", "%mm0", "%mm1", "%mm2", "%mm3", "%mm4", "%mm5", "%mm6", "%mm7");
				}

  asm("emms");
}

#endif /* __GNUC__ > 3 */
#endif /* defined(ARCH_X86) */
#endif /* defined(USE_SSE) */


void
gimp_composite_sse2_init (void)
{
}
