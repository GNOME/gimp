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

#ifndef __CPU_ACCEL_H__
#define __CPU_ACCEL_H__


/* x86 accelerations */
#define CPU_ACCEL_X86_MMX        0x80000000
#define CPU_ACCEL_X86_3DNOW      0x40000000
#define CPU_ACCEL_X86_MMXEXT     0x20000000
#define CPU_ACCEL_X86_SSE        0x10000000
#define CPU_ACCEL_X86_SSE2       0x08000000

/* powerpc accelerations */
#define CPU_ACCEL_PPC_ALTIVEC    0x04000000


guint32  cpu_accel               (void) G_GNUC_CONST;

void     cpu_accel_print_results (void);


#endif  /* __CPU_ACCEL_H__ */

