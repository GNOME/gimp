/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__LIGMA_BASE_H_INSIDE__) && !defined (LIGMA_BASE_COMPILATION)
#error "Only <libligmabase/ligmabase.h> can be included directly."
#endif

#ifndef __LIGMA_CPU_ACCEL_H__
#define __LIGMA_CPU_ACCEL_H__

G_BEGIN_DECLS


/**
 * LigmaCpuAccelFlags:
 * @LIGMA_CPU_ACCEL_NONE:        None
 * @LIGMA_CPU_ACCEL_X86_MMX:     MMX
 * @LIGMA_CPU_ACCEL_X86_3DNOW:   3dNow
 * @LIGMA_CPU_ACCEL_X86_MMXEXT:  MMXEXT
 * @LIGMA_CPU_ACCEL_X86_SSE:     SSE
 * @LIGMA_CPU_ACCEL_X86_SSE2:    SSE2
 * @LIGMA_CPU_ACCEL_X86_SSE3:    SSE3
 * @LIGMA_CPU_ACCEL_X86_SSSE3:   SSSE3
 * @LIGMA_CPU_ACCEL_X86_SSE4_1:  SSE4_1
 * @LIGMA_CPU_ACCEL_X86_SSE4_2:  SSE4_2
 * @LIGMA_CPU_ACCEL_X86_AVX:     AVX
 * @LIGMA_CPU_ACCEL_PPC_ALTIVEC: Altivec
 *
 * Types of detectable CPU accelerations
 **/
typedef enum
{
  LIGMA_CPU_ACCEL_NONE        = 0x0,

  /* x86 accelerations */
  LIGMA_CPU_ACCEL_X86_MMX     = 0x80000000,
  LIGMA_CPU_ACCEL_X86_3DNOW   = 0x40000000,
  LIGMA_CPU_ACCEL_X86_MMXEXT  = 0x20000000,
  LIGMA_CPU_ACCEL_X86_SSE     = 0x10000000,
  LIGMA_CPU_ACCEL_X86_SSE2    = 0x08000000,
  LIGMA_CPU_ACCEL_X86_SSE3    = 0x02000000,
  LIGMA_CPU_ACCEL_X86_SSSE3   = 0x01000000,
  LIGMA_CPU_ACCEL_X86_SSE4_1  = 0x00800000,
  LIGMA_CPU_ACCEL_X86_SSE4_2  = 0x00400000,
  LIGMA_CPU_ACCEL_X86_AVX     = 0x00200000,

  /* powerpc accelerations */
  LIGMA_CPU_ACCEL_PPC_ALTIVEC = 0x04000000
} LigmaCpuAccelFlags;


LigmaCpuAccelFlags  ligma_cpu_accel_get_support (void);


/* for internal use only */
void               ligma_cpu_accel_set_use     (gboolean use);


G_END_DECLS

#endif  /* __LIGMA_CPU_ACCEL_H__ */
