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

#include <stdio.h>

#include <glib-object.h>

#include "base/base-types.h"
#include "base/cpu-accel.h"

#include "gimp-composite.h"

#include "gimp-composite-3dnow.h"

#ifdef COMPILE_3DNOW_IS_OKAY

#endif

gboolean
gimp_composite_3dnow_init (void)
{
#ifdef COMPILE_3DNOW_IS_OKAY
  if (cpu_accel () & CPU_ACCEL_X86_3DNOW)
    {
      return (TRUE);
    }
#endif
  return (FALSE);
}
