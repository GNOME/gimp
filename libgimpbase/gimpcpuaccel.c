/* The GIMP -- an image manipulation program
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

/*
 * CPU acceleration detection was taken from DirectFB but seems to be
 * originating from mpeg2dec with the following copyright:
 *
 * Copyright (C) 1999-2001 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 */

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <setjmp.h>

#include <glib.h>

#include "cpu-accel.h"


#ifdef ARCH_X86
static guint32
arch_accel (void)
{
  guint32 eax, ebx, ecx, edx;
  gint    AMD;
  guint32 caps = 0;

#define cpuid(op,eax,ebx,ecx,edx)  \
     asm ("pushl %%ebx\n\t"        \
          "cpuid\n\t"              \
          "movl %%ebx,%1\n\t"      \
          "popl %%ebx"             \
          : "=a" (eax),            \
            "=r" (ebx),            \
            "=c" (ecx),            \
            "=d" (edx)             \
          : "a" (op)               \
          : "cc")

  asm ("pushfl\n\t"
       "pushfl\n\t"
       "popl %0\n\t"
       "movl %0,%1\n\t"
       "xorl $0x200000,%0\n\t"
       "pushl %0\n\t"
       "popfl\n\t"
       "pushfl\n\t"
       "popl %0\n\t"
       "popfl"
       : "=r" (eax),
       "=r" (ebx)
       :
       : "cc");

  if (eax == ebx)             /* no cpuid */
    return 0;

  cpuid (0x00000000, eax, ebx, ecx, edx);
  if (!eax)                   /* vendor string only */
    return 0;

  AMD = (ebx == 0x68747541) && (ecx == 0x444d4163) && (edx == 0x69746e65);

  cpuid (0x00000001, eax, ebx, ecx, edx);
  if (! (edx & 0x00800000))   /* no MMX */
    return 0;

#ifdef USE_MMX
  caps = CPU_ACCEL_X86_MMX;
#ifdef USE_SSE
  if (edx & 0x02000000)       /* SSE - identical to AMD MMX extensions */
    caps |= CPU_ACCEL_X86_SSE | CPU_ACCEL_X86_MMXEXT;

  if (edx & 0x04000000)       /* SSE2 */
    caps |= CPU_ACCEL_X86_SSE2;

  cpuid (0x80000000, eax, ebx, ecx, edx);
  if (eax < 0x80000001)       /* no extended capabilities */
    return caps;

  cpuid (0x80000001, eax, ebx, ecx, edx);

  if (edx & 0x80000000)
    caps |= CPU_ACCEL_X86_3DNOW;

  if (AMD && (edx & 0x00400000))      /* AMD MMX extensions */
    caps |= CPU_ACCEL_X86_MMXEXT;
#endif /* USE_SSE */
#endif /* USE_MMX */

  return caps;
}

static jmp_buf sigill_return;

static void
sigill_handler (gint n)
{
  g_printerr ("OS lacks support for SSE instructions.\n");
  longjmp(sigill_return, 1);
}

#endif /* ARCH_X86 */


#if defined (ARCH_PPC) && defined (USE_ALTIVEC)

static          sigjmp_buf   jmpbuf;
static volatile sig_atomic_t canjump = 0;

static void
sigill_handler (gint sig)
{
  if (!canjump)
    {
      signal (sig, SIG_DFL);
      raise (sig);
    }

  canjump = 0;
  siglongjmp (jmpbuf, 1);
}

static guint32
arch_accel (void)
{
  signal (SIGILL, sigill_handler);
  if (sigsetjmp (jmpbuf, 1))
    {
      signal (SIGILL, SIG_DFL);
      return 0;
    }

  canjump = 1;

  asm volatile ("mtspr 256, %0\n\t"
		"vand %%v0, %%v0, %%v0"
		:
		: "r" (-1));

  signal (SIGILL, SIG_DFL);

  return CPU_ACCEL_PPC_ALTIVEC;
}

#endif /* ARCH_PPC */


guint32
cpu_accel (void)
{
#if defined (ARCH_X86) || (defined (ARCH_PPC) && defined (USE_ALTIVEC))
  static guint32 accel = ~0U;

  if (accel != ~0U)
    return accel;

  accel = arch_accel ();

#ifdef USE_SSE

  /* test OS support for SSE */
  if (accel & CPU_ACCEL_X86_SSE)
    {
      if (setjmp (sigill_return))
	{
	  accel &= ~(CPU_ACCEL_X86_SSE | CPU_ACCEL_X86_SSE2);
	}
      else
	{
	  signal (SIGILL, sigill_handler);
	  __asm __volatile ("xorps %xmm0, %xmm0");
	  signal (SIGILL, SIG_DFL);
	}
    }
#endif /* USE_SSE */

  return accel;

#else /* !ARCH_X86 && !ARCH_PPC/USE_ALTIVEC */
  return 0;
#endif
}
