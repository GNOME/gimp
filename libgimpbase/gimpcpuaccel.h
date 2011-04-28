/* LIBGIMP - The GIMP Library
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
 * <http://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_BASE_H_INSIDE__) && !defined (GIMP_BASE_COMPILATION)
#error "Only <libgimpbase/gimpbase.h> can be included directly."
#endif

#ifndef __GIMP_CPU_ACCEL_H__
#define __GIMP_CPU_ACCEL_H__

G_BEGIN_DECLS


typedef enum
{
  GIMP_CPU_ACCEL_NONE        = 0x0,

  /* x86 accelerations */
  GIMP_CPU_ACCEL_X86_MMX     = 0x80000000,
  GIMP_CPU_ACCEL_X86_3DNOW   = 0x40000000,
  GIMP_CPU_ACCEL_X86_MMXEXT  = 0x20000000,
  GIMP_CPU_ACCEL_X86_SSE     = 0x10000000,
  GIMP_CPU_ACCEL_X86_SSE2    = 0x08000000,
  GIMP_CPU_ACCEL_X86_SSE3    = 0x02000000,

  /* powerpc accelerations */
  GIMP_CPU_ACCEL_PPC_ALTIVEC = 0x04000000
} GimpCpuAccelFlags;


GimpCpuAccelFlags  gimp_cpu_accel_get_support (void);


/* for internal use only */
void               gimp_cpu_accel_set_use     (gboolean use);


G_END_DECLS

#endif  /* __GIMP_CPU_ACCEL_H__ */
