/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpmath.h
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_MATH_H__
#define __GIMP_MATH_H__

#include <math.h>

#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif

#ifdef G_OS_WIN32
#include <float.h>
#endif

#include <libgimpmath/gimpmathtypes.h>

#include <libgimpmath/gimpmatrix.h>
#include <libgimpmath/gimpmd5.h>
#include <libgimpmath/gimpvector.h>

G_BEGIN_DECLS

/* Some portability enhancing stuff. For use both by the gimp app
 * as well as plug-ins and modules.
 *
 * Include this instead of just <math.h>.
 */

/* Use RINT() instead of rint() */
#ifdef HAVE_RINT
#define RINT(x) rint(x)
#else
#define RINT(x) floor ((x) + 0.5)
#endif

#define ROUND(x) ((int) ((x) + 0.5))

/* Square */
#define SQR(x) ((x) * (x))

/* Limit a (0->511) int to 255 */
#define MAX255(a)  ((a) | (((a) & 256) - (((a) & 256) >> 8)))

/* Clamp a >>int32<<-range int between 0 and 255 inclusive */
#define CLAMP0255(a)  CLAMP(a,0,255)

#define gimp_deg_to_rad(angle) ((angle) * (2.0 * G_PI) / 360.0)
#define gimp_rad_to_deg(angle) ((angle) * 360.0 / (2.0 * G_PI))


G_END_DECLS

#endif /* __GIMP_MATH_H__ */
