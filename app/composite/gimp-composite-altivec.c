/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifdef ARCH_PPC
#include <stdio.h>

#include <glib-object.h>

#include "base/base-types.h"

#include "gimp-composite.h"
#include "gimp-composite-altivec.h"

#if __GNUC__ >= 3

#endif /* __GNUC__ > 3 */
#endif  /* ARCH_PPC */

int
gimp_composite_altivec_init (void)
{
#ifdef ARCH_PPC
		if (cpu & CPU_ACCEL_PPC_ALTIVEC)
				{
						return (1);
				}
#endif

		return (0);
}
